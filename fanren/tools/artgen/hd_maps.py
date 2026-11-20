"""手绘高清地图打包（第 7 章美术重构，2026-11-19 用户定：地图整张用 Seedream 重绘、换成 24 像素一格）。

Seedream 以现有烘焙图为布局底稿整张重画，定稿原图（不入库）在这里缩成真像素，写成游戏读的两层：

  assets/art/maps_hd/<图 id>/below.png    下层：整张地图，24 像素一格（宽 = 格数 × 24）
  assets/art/maps_hd/<图 id>/above.png    上层：盖在人物上面的部分（树冠、屋檐）；没有就是全透明
  assets/art/maps_hd/index.json           索引：每张图的来源、色数与文件的 sha256

    python tools/artgen/hd_maps.py [--src <定稿目录>]           # 从源图写出上面这些
    python tools/artgen/hd_maps.py [--src <定稿目录>] --check   # 核：文件与索引对得上、尺寸与地图对得上；有源图再重打一遍比像素

引擎（WorldView）先找 art/maps_hd/<图 id>/，找到就按 24 像素一格、×2 画（屏幕上仍是一格 48 像素，与手绘人物同一种
像素颗粒）；找不到照旧用 art/maps/<图 id>/ 的 16 像素烘焙图。meta.json（光、粒子、色调）仍取 art/maps/ 那份，
那一份照旧由 artgen.py 生成、受 ARTGEN_IN_SYNC 管；这里的图不再由程序重生成，只核 sha256。

源目录：--src 优先，其次环境变量 FANREN_HD_SRC，再其次仓库里的 docs/companions-overhaul（不入库）；
源图在其下的 maps/<源名>.png，可另给 maps/<源名>.above.png（与源图同尺寸的 RGBA，不透明处就是上层）。
历练图与主线图共用底图：ALIASES 里的图写一份同样的副本（引擎只按图 id 找目录，不读索引）。

缩图：先按格子整块平均（BOX）缩到格数 × 24，再中位切分取有限色板、不抖动——Seedream 的原稿带柔和渐变，
直接取最近点会出雪花点。确定性：不读时钟、没有随机数；JSON 键排序、UTF-8 无 BOM、LF。
"""

from __future__ import annotations

import argparse
import hashlib
import io
import json
import os
import sys
from dataclasses import dataclass
from pathlib import Path

from PIL import Image

REPO = Path(__file__).resolve().parents[2]
SRC_ENV = "FANREN_HD_SRC"
REPO_SRC = REPO / "docs" / "companions-overhaul"
OUT_REL = Path("assets") / "art" / "maps_hd"
CELL = 24           # 一格几像素（屏幕上 ×2 = 48，与 16 像素 ×3 的旧图一格同大）


@dataclass(frozen=True)
class HdMap:
    map_id: str
    src: str            # 源名：<src>/maps/<src>.png
    colors: int = 96    # 色板大小
    gain: float = 1.0   # 整体提亮倍数：引擎的环境光、暗角是照旧烘焙图的亮度调的，Seedream 画得偏暗时按旧图可走格的平均亮度配回来
    note: str = ""
    above_from_bake: bool = False   # 上层照旧烘焙图 above.png 的不透明处（树冠、屋檐）从新图里取；零星草点不要


MAPS = (
    HdMap("ch07_dixia_zhaoze", "ch07_dixia_zhaoze", 96, 1.42,
          "地下沼泽。Seedream 以烘焙图为底稿重画；改第二遍删掉可走地面上多出的石柱与灰斑；改第三遍照 meta.json 的六盏 torch "
          "在岩壁、岩柱上画火把（灯光是 artgen 按旧图摆的，图上得有光源）。可走格平均亮度 40.8 → 旧图 58.2，gain 1.42（2026-11-19 样张）"),
    # 以下 2026-11-19 批量（gain 都按旧烘焙图可走格的平均亮度配，夹在 0.8–1.6）。「手工」指 Python 挪像素补的，不是 Seedream 改的。
    HdMap("ch07_dihuo", "ch07_dihuo", 96, 1.60, "地火屋。出图 4 次、改 1 次（补左上壁灯）"),
    HdMap("ch07_yuelu_dian", "ch07_yuelu_dian", 96, 1.36,
          "岳麓殿。出图 1 次、改 1 次；右上偏室北墙手工上移 48 像素、补一块重复地砖，北墙仍压住第 1 行几格可走格"),
    HdMap("ch07_fangshi", "ch07_fangshi", 96, 0.86, "坊市。出图 1 次、改 1 次（删存档圆盘、小货摊、小牌子）；南门两侧挡路石墩没画出来",
          above_from_bake=True),
    HdMap("ch07_shandong", "ch07_shandong", 96, 1.60, "山洞（夜）。出图 1 次；存档格圆盘手工盖掉", above_from_bake=True),
    HdMap("ch07_jindi_wai", "ch07_jindi_wai", 96, 0.94, "禁地之外。出图 1 次；存档格小标记手工盖掉"),
    HdMap("ch07_jindi_waiwei", "ch07_jindi_waiwei", 96, 0.90,
          "禁地外围。出图 1 次、改 3 次（改白天、删灯笼、把压进第 1 行的崖面退回去）", above_from_bake=True),
    HdMap("ch07_jindi_zhongxin", "ch07_jindi_zhongxin", 96, 0.82, "禁地中心区外。出图 1 次、改 1 次（删灯笼、神龛、灰石）",
          above_from_bake=True),
    HdMap("ch07_huanxingshan", "ch07_huanxingshan", 96, 1.04,
          "环形山。出图 1 次、改 4 次；小石殿手工上移 64 像素、上缘岩石退回第 0 行、右缘崖壁外移一列，接缝处做过模糊",
          above_from_bake=True),
    HdMap("ch07_dixia_shiku", "ch07_dixia_shiku", 96, 0.85, "地底石窟。出图 1 次、改 2 次；六支火把手工挪到 meta.json 的光源处"),
    HdMap("ch07_zhaoze_shenchu", "ch07_zhaoze_shenchu", 96, 1.35,
          "沼泽深处（以地下沼泽为第三张参考图）。出图 1 次、改 2 次；三支火把手工补、一支挪位"),
    HdMap("ch06_baiyaoyuan", "ch06_baiyaoyuan", 96, 0.80,
          "百药园（第 6 章的图，第 6 章起就是新画面，用户 2026-11-19 定一起重做）。出图 1 次、改 1 次", above_from_bake=True),
)

# 历练图 → 共用底图的主线图（genmaps_ch07.py：六张 lilian_jindi_* 是原图的底图、另一套对象）
ALIASES: dict[str, str] = {
    "lilian_jindi_zhaoze": "ch07_dixia_zhaoze",
    "lilian_jindi_waiwei": "ch07_jindi_waiwei",
    "lilian_jindi_zhongxin": "ch07_jindi_zhongxin",
    "lilian_jindi_huanxingshan": "ch07_huanxingshan",
    "lilian_jindi_shenchu": "ch07_zhaoze_shenchu",
    "lilian_jindi_shiku": "ch07_dixia_shiku",
}
ABOVE_MIN_PIXELS = 60   # 旧上层里小于这么多像素（24 像素一格的尺度）的连通块是草点、碎石，不算遮挡


def default_src() -> Path:
    env = os.environ.get(SRC_ENV, "").strip()
    return Path(env) if env else REPO_SRC


def map_cells(map_id: str) -> tuple[int, int]:
    with open(REPO / "maps" / f"{map_id}.tmj", encoding="utf-8") as f:
        d = json.load(f)
    return int(d["width"]), int(d["height"])


def png_bytes(img: Image.Image) -> bytes:
    buf = io.BytesIO()
    img.save(buf, format="PNG", optimize=True)
    return buf.getvalue()


def pixelize(src: Image.Image, size: tuple[int, int], colors: int, gain: float = 1.0) -> Image.Image:
    small = src.convert("RGB").resize(size, Image.Resampling.BOX)
    if gain != 1.0:
        small = small.point(lambda v: min(255, int(round(v * gain))))
    q = small.quantize(colors=colors, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE)
    return q.convert("RGBA")


def bake_above_mask(m: HdMap, size: tuple[int, int]) -> Image.Image:
    """旧烘焙图 above.png 的不透明处，放到 24 像素一格、去掉零星小块、向外扩一像素。"""
    import numpy as np
    from scipy import ndimage
    old = Image.open(REPO / "assets" / "art" / "maps" / m.map_id / "above.png").convert("RGBA").getchannel("A")
    a = np.asarray(old.resize(size, Image.Resampling.NEAREST)) > 0
    labels, n = ndimage.label(a)
    if n:
        sizes = ndimage.sum(a, labels, index=range(1, n + 1))
        keep = np.isin(labels, [i + 1 for i, s in enumerate(sizes) if s >= ABOVE_MIN_PIXELS])
        a = ndimage.binary_dilation(keep, iterations=1)
    return Image.fromarray((a * 255).astype("uint8"), "L")


def above_layer(below: Image.Image, src_dir: Path, m: HdMap) -> Image.Image:
    mask_path = src_dir / "maps" / f"{m.src}.above.png"
    if m.above_from_bake:
        mask = bake_above_mask(m, below.size)
    elif mask_path.exists():
        mask = Image.open(mask_path).convert("RGBA").getchannel("A").resize(below.size, Image.Resampling.BOX)
        mask = mask.point(lambda a: 255 if a >= 128 else 0)
    else:
        return Image.new("RGBA", below.size, (0, 0, 0, 0))
    out = Image.new("RGBA", below.size, (0, 0, 0, 0))
    out.paste(below, (0, 0), mask)
    return out


def build_files(src_dir: Path) -> dict[str, bytes]:
    """返回 相对 maps_hd/ 的路径 → 文件字节（不含 index.json）。"""
    files: dict[str, bytes] = {}
    for m in MAPS:
        w, h = map_cells(m.map_id)
        src_path = src_dir / "maps" / f"{m.src}.png"
        if not src_path.exists():
            raise FileNotFoundError(f"{m.map_id} 的源图 {src_path} 不在")
        below = pixelize(Image.open(src_path), (w * CELL, h * CELL), m.colors, m.gain)
        above = above_layer(below, src_dir, m)
        files[f"{m.map_id}/below.png"] = png_bytes(below)
        files[f"{m.map_id}/above.png"] = png_bytes(above)
    for alias, base in sorted(ALIASES.items()):
        if map_cells(alias) != map_cells(base):
            raise ValueError(f"历练图 {alias} 的格数与 {base} 不同，不能共用底图")
        files[f"{alias}/below.png"] = files[f"{base}/below.png"]
        files[f"{alias}/above.png"] = files[f"{base}/above.png"]
    return files


def index_json(files: dict[str, bytes]) -> bytes:
    doc = {
        "_doc": "手绘高清地图（tools/artgen/hd_maps.py 写出，勿手改）。cell：一格几像素；maps：每张图的源名与色数；"
                "aliases：历练图共用哪张主线图的底图；files：相对本目录的文件与 sha256。",
        "cell": CELL,
        "maps": {m.map_id: {"src": m.src, "colors": m.colors, "gain": m.gain, "above_from_bake": m.above_from_bake,
                            "note": m.note} for m in MAPS},
        "aliases": dict(sorted(ALIASES.items())),
        "files": {name: hashlib.sha256(data).hexdigest() for name, data in sorted(files.items())},
    }
    return (json.dumps(doc, ensure_ascii=False, indent=1, sort_keys=True) + "\n").encode("utf-8")


def write(out: Path, files: dict[str, bytes]) -> None:
    for name, data in files.items():
        p = out / name
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_bytes(data)
    (out / "index.json").write_bytes(index_json(files))


def check_committed(out: Path) -> list[str]:
    problems: list[str] = []
    index_path = out / "index.json"
    if not index_path.exists():
        return [f"缺 {(OUT_REL / 'index.json').as_posix()}"]
    with open(index_path, encoding="utf-8") as f:
        index = json.load(f)
    listed = index.get("files", {})
    for name, digest in sorted(listed.items()):
        p = out / name
        if not p.exists():
            problems.append(f"缺 {(OUT_REL / name).as_posix()}")
            continue
        if hashlib.sha256(p.read_bytes()).hexdigest() != digest:
            problems.append(f"变 {(OUT_REL / name).as_posix()}（sha256 与 index.json 不一致）")
        map_id = name.split("/")[0]
        try:
            w, h = map_cells(map_id)
        except FileNotFoundError:
            problems.append(f"{(OUT_REL / name).as_posix()}：没有地图 maps/{map_id}.tmj")
            continue
        size = Image.open(p).size
        if size != (w * CELL, h * CELL):
            problems.append(f"{(OUT_REL / name).as_posix()} 是 {size[0]}×{size[1]}，地图 {w}×{h} 格应为 "
                            f"{w * CELL}×{h * CELL}")
    for p in sorted(out.rglob("*")):
        if p.is_file() and p.name != "index.json":
            rel = p.relative_to(out).as_posix()
            if rel not in listed:
                problems.append(f"多 {(OUT_REL / rel).as_posix()}（index.json 没登记）")
    for map_id in sorted({n.split("/")[0] for n in listed}):
        for layer in ("below.png", "above.png"):
            if f"{map_id}/{layer}" not in listed:
                problems.append(f"{map_id} 缺 {layer}（两层要成对）")
    return problems


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--src", type=Path, default=None,
                    help=f"定稿图目录（不入库；缺省取环境变量 {SRC_ENV}，再没有就是 docs/companions-overhaul）")
    ap.add_argument("--check", action="store_true", help="只比对，不写")
    args = ap.parse_args(argv)
    src = args.src or default_src()
    out = REPO / OUT_REL
    if not args.check:
        write(out, build_files(src))
        print(f"HD_MAPS_WRITTEN：{len(MAPS)} 张图、{len(ALIASES)} 张历练副本 → {OUT_REL.as_posix()}")
        return 0
    problems = check_committed(out)
    if problems:
        for p in problems:
            print(p)
        print("HD_MAPS_OUT_OF_SYNC")
        return 1
    if not (src / "maps").exists():
        print(f"HD_MAPS_IN_SYNC_PARTIAL：文件与索引的 sha256、尺寸一致；源目录 {src / 'maps'} 不在，跳过从源图重打（没核）")
        return 0
    fresh = build_files(src)
    with open(out / "index.json", encoding="utf-8") as f:
        listed = json.load(f)["files"]
    diffs = []
    if sorted(fresh) != sorted(listed):
        diffs.append("从源图重打出来的文件清单与 index.json 不同")
    for name, data in sorted(fresh.items()):
        p = out / name
        if p.exists():
            a = Image.open(io.BytesIO(data)).convert("RGBA")
            b = Image.open(p).convert("RGBA")
            if a.size != b.size or a.tobytes() != b.tobytes():
                diffs.append(f"{(OUT_REL / name).as_posix()}：从源图重打出来像素不同")
    if diffs:
        for d in diffs:
            print(d)
        print("HD_MAPS_OUT_OF_SYNC")
        return 1
    print("HD_MAPS_IN_SYNC：文件与索引的 sha256、尺寸一致；从源图重打，像素相同")
    return 0


if __name__ == "__main__":
    sys.exit(main())
