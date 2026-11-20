"""手绘战斗背景（第 7 章美术重构，2026-11-19 用户定「战斗背景也用 Seedream 重做」）。

每套背景 Seedream 出两张（不入库，<源目录>/raw/backdrop_<原 id>/）：
  sky.png     天空（或洞顶）：缩成 640×360，左右边缘做成首尾相接（天空层在战斗里一直慢慢往左漂）
  scene.png   场景：天空透明，地平线以下是站位的地面——整张放进 ground 层（不动）

写成 assets/art/battle/hd07_<原 id>/{sky,far,mid,ground}.png（far、mid 全透明）与 assets/art/battle/hd07_manifest.json。
640×360 是 1280×720 的一半：战斗里拉满全屏，等于 ×2，与手绘人物同一种像素颗粒。
id 加 hd07_ 前缀、只指给第 7 章的战斗（data/visual/battles.json），别的章节照旧用程序画的那一套；
时辰、粒子、雾照原 id（game/BattleFx.cpp 的 backdropLook 去掉前缀再判）。

    python tools/artgen/hd_backdrops.py [--src <定稿目录>]
    python tools/artgen/hd_backdrops.py [--src <定稿目录>] --check

确定性：不读时钟、没有随机数；JSON 键排序、UTF-8 无 BOM、LF。
"""

from __future__ import annotations

import argparse
import hashlib
import io
import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parent))
import hd_frames  # noqa: E402

REPO = Path(__file__).resolve().parents[2]
OUT = REPO / "assets" / "art" / "battle"
PREFIX = "hd07_"
SIZE = (640, 360)
SEAM = 48          # 天空左右边缘交融的宽度
COLORS = 96
BACKDROPS = ("mountain_forest", "mountain_forest_dusk", "cave_tunnel", "cliff_top")


def png(img: Image.Image) -> bytes:
    buf = io.BytesIO()
    img.save(buf, format="PNG", optimize=True)
    return buf.getvalue()


def quantize(img: Image.Image) -> Image.Image:
    rgb = img.convert("RGB").quantize(colors=COLORS, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE)
    out = rgb.convert("RGBA")
    out.putalpha(img.getchannel("A"))
    return out


def seamless(img: Image.Image) -> Image.Image:
    """右边 SEAM 列渐变到左边缘的镜像：最右一列与最左一列相同，平铺时首尾接得上。"""
    a = np.asarray(img.convert("RGBA")).astype(np.float64)
    w = a.shape[1]
    for i in range(SEAM):
        j = w - SEAM + i
        t = (i + 1) / SEAM
        a[:, j] = (1 - t) * a[:, j] + t * a[:, w - 1 - j]
    return Image.fromarray(np.round(a).astype(np.uint8), "RGBA")


def build(src: Path) -> dict[str, bytes]:
    files: dict[str, bytes] = {}
    empty = png(Image.new("RGBA", SIZE, (0, 0, 0, 0)))
    for b in BACKDROPS:
        raw = src / "raw" / f"backdrop_{b}"
        sky = Image.open(raw / "sky.png").convert("RGBA").resize(SIZE, Image.Resampling.BOX)
        sky.putalpha(255)
        scene = Image.open(raw / "scene.png").convert("RGBA")
        scene = hd_frames.clean_downscale(scene, *SIZE)
        d = f"{PREFIX}{b}"
        files[f"{d}/sky.png"] = png(quantize(seamless(sky)))
        files[f"{d}/far.png"] = empty
        files[f"{d}/mid.png"] = empty
        files[f"{d}/ground.png"] = png(quantize(scene))
    return files


def manifest(files: dict[str, bytes]) -> bytes:
    doc = {"_说明": "第 7 章手绘战斗背景（tools/artgen/hd_backdrops.py 写出，勿手改）：文件与 sha256。",
           "files": {n: hashlib.sha256(d).hexdigest() for n, d in sorted(files.items())}}
    return (json.dumps(doc, ensure_ascii=False, indent=1, sort_keys=True) + "\n").encode("utf-8")


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--src", type=Path, default=None)
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args(argv)
    src = args.src or hd_frames.default_src()
    man = OUT / f"{PREFIX}manifest.json"
    if not args.check:
        files = build(src)
        for name, data in files.items():
            (OUT / name).parent.mkdir(parents=True, exist_ok=True)
            (OUT / name).write_bytes(data)
        man.write_bytes(manifest(files))
        print(f"HD_BACKDROPS_WRITTEN：{len(BACKDROPS)} 套 → assets/art/battle/{PREFIX}*")
        return 0
    problems = []
    if not man.exists():
        problems.append(f"缺 assets/art/battle/{man.name}")
    else:
        listed = json.loads(man.read_text(encoding="utf-8"))["files"]
        for name, digest in listed.items():
            p = OUT / name
            if not p.exists() or hashlib.sha256(p.read_bytes()).hexdigest() != digest:
                problems.append(f"变 assets/art/battle/{name}")
        on_disk = {p.relative_to(OUT).as_posix() for p in OUT.glob(f"{PREFIX}*/*") if p.is_file()}
        problems += [f"多 assets/art/battle/{n}" for n in sorted(on_disk - set(listed))]
        if not problems and all((src / "raw" / f"backdrop_{b}").exists() for b in BACKDROPS):
            for name, data in build(src).items():
                a = Image.open(io.BytesIO(data)).convert("RGBA")
                bb = Image.open(OUT / name).convert("RGBA")
                if a.tobytes() != bb.tobytes():
                    problems.append(f"assets/art/battle/{name}：从源图重打出来像素不同")
        elif not problems:
            print("HD_BACKDROPS_IN_SYNC_PARTIAL：sha256 一致；源图不在，跳过从源图重打（没核）")
            return 0
    if problems:
        for p in problems:
            print(p)
        print("HD_BACKDROPS_OUT_OF_SYNC")
        return 1
    print("HD_BACKDROPS_IN_SYNC：sha256 一致；从源图重打，像素相同")
    return 0


if __name__ == "__main__":
    sys.exit(main())
