"""美术生成入口：地图烘焙、战斗背景、标题背景，以及人物那一路的精灵与特效。

    python tools/artgen/artgen.py                    # 全部生成到 assets/art/
    python tools/artgen/artgen.py --only maps        # 只生成一类：maps|backdrops|title|sprites|fx（可逗号分隔）
    python tools/artgen/artgen.py --map ch01_hanjiacun   # 只烘一张图
    python tools/artgen/artgen.py --check            # 门禁：重生成到临时目录，与 assets/art/ 逐像素比对

--check 全一致打印 ARTGEN_IN_SYNC、退出 0；否则列出差异文件、退出 1。
比对 PNG 用 PIL 读出像素比（不比文件字节：同样的像素，zlib 版本不同压出来的字节可以不同）；
JSON 比解析后的内容。生成物多出来（某个生成器负责的目录里有、这次没生成）也算差异——
那是改名或删掉之后留下的旧图，运行时照样会读到它。

人物那一路（tools/artgen/sprites.py、fx.py）各自暴露 build(out_root) -> list[Path]，
在的话照调、--check 一并覆盖；导入不到（还没落地）就跳过并打印一行。
一切都确定：同输入同输出，随机一律由 (图 id, 格坐标, 用途) 派生的固定种子。
"""

from __future__ import annotations

import argparse
import importlib
import json
import shutil
import sys
import tempfile
import time
from pathlib import Path

_HERE = Path(__file__).resolve().parent
if str(_HERE) not in sys.path:
    sys.path.insert(0, str(_HERE))

import numpy as np  # noqa: E402
from PIL import Image  # noqa: E402

ROOT = _HERE.parents[1]
ART = ROOT / "assets" / "art"

# 名字 → (模块, 它负责的 assets/art 下的目录, 是本路自己的（带 fast/only 参数）)
BUILDERS = {
    "maps": ("mapbake", ["maps"], True),
    "backdrops": ("backdrops", ["battle"], True),
    "title": ("title", ["title"], True),
    "sprites": ("sprites", ["sprites"], False),
    "fx": ("fx", ["fx"], False),
}


def run_builders(names: list[str], out_root: Path, only_map: str | None, fast: bool) -> tuple[list[Path], set[str]]:
    """跑一组生成器，返回 (产出文件, 这次负责的目录名)。"""
    produced: list[Path] = []
    owned: set[str] = set()
    for name in names:
        mod_name, dirs, ours = BUILDERS[name]
        t0 = time.time()
        try:
            mod = importlib.import_module(mod_name)
        except ModuleNotFoundError as e:
            if ours or e.name != mod_name:
                raise
            print("[跳过] %s：tools/artgen/%s.py 还没有（人物那一路的产物），这一类不生成也不比对" % (name, mod_name))
            continue
        if name == "maps":
            paths = mod.build(out_root, only_map=only_map, fast=fast)
        elif ours:
            paths = mod.build(out_root, fast=fast)
        else:
            paths = mod.build(out_root)
        paths = [Path(p) for p in paths]
        produced += paths
        if not (name == "maps" and only_map):
            owned |= set(dirs)
            for p in paths:
                rel = p.resolve().relative_to(out_root.resolve())
                owned.add(rel.parts[0])
        print("%-10s %4d 个文件  %.1fs" % (name, len(paths), time.time() - t0))
    return produced, owned


def same_file(a: Path, b: Path) -> bool:
    if a.suffix.lower() == ".png":
        ia = np.asarray(Image.open(a).convert("RGBA"))
        ib = np.asarray(Image.open(b).convert("RGBA"))
        return ia.shape == ib.shape and np.array_equal(ia, ib)
    if a.suffix.lower() == ".json":
        return json.loads(a.read_text(encoding="utf-8")) == json.loads(b.read_text(encoding="utf-8"))
    return a.read_bytes() == b.read_bytes()


def check(names: list[str], only_map: str | None) -> int:
    t0 = time.time()
    tmp = Path(tempfile.mkdtemp(prefix="artgen_check_"))
    try:
        produced, owned = run_builders(names, tmp, only_map, fast=True)
        diffs = []
        rels = set()
        for p in produced:
            rel = p.resolve().relative_to(tmp.resolve())
            rels.add(rel.as_posix())
            target = ART / rel
            if not target.exists():
                diffs.append("缺  %s（assets/art 里没有）" % rel.as_posix())
            elif not same_file(p, target):
                diffs.append("变  %s" % rel.as_posix())
        for d in sorted(owned):
            base = ART / d
            if not base.exists():
                continue
            for f in sorted(base.rglob("*")):
                if f.is_file() and f.relative_to(ART).as_posix() not in rels:
                    diffs.append("多  %s（生成器已不产出它）" % f.relative_to(ART).as_posix())
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    if diffs:
        print()
        for line in diffs:
            print("  " + line)
        print()
        print("%d 个文件与生成器不一致：重跑 python tools/artgen/artgen.py 会改动它们。" % len(diffs))
        return 1
    print("ARTGEN_IN_SYNC：%d 个文件与生成器逐像素一致（%.1fs）" % (len(produced), time.time() - t0))
    return 0


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description="美术生成：地图烘焙 / 战斗背景 / 标题背景 / 精灵 / 特效")
    ap.add_argument("--only", help="maps|backdrops|title|sprites|fx，可逗号分隔")
    ap.add_argument("--map", help="只烘这一张图（隐含 --only maps）")
    ap.add_argument("--check", action="store_true", help="重生成到临时目录并逐像素比对")
    args = ap.parse_args(argv)
    if args.map:
        names = ["maps"]
    elif args.only:
        names = [n.strip() for n in args.only.split(",") if n.strip()]
        bad = [n for n in names if n not in BUILDERS]
        if bad:
            ap.error("不认识的类别：%s" % ", ".join(bad))
    else:
        names = list(BUILDERS)
    if args.check:
        return check(names, args.map)
    t0 = time.time()
    produced, _ = run_builders(names, ART, args.map, fast=False)
    print("共 %d 个文件，%.1fs → %s" % (len(produced), time.time() - t0, ART))
    return 0


if __name__ == "__main__":
    sys.exit(main())
