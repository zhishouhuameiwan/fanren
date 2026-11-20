"""手绘地图物件（第 7 章美术重构，2026-11-19 用户定「地图物件也用 Seedream 重做」）。

Seedream 出的一张物件表（13 个物件，三行 5 / 5 / 3，透明底，不入库，在 <源目录>/raw/objects/sheet.png）
→ 按连通块切开、缩成真像素（hd_frames.py 的「深色优先」缩法）→ 拼成一张图集：

  assets/art/sprites/hd_objects/objects.png     图集
  assets/art/sprites/hd_objects/objects.json    物件表：与 sprites/index.json 的 objects / facilities 同一个写法，
                                                另带 files（sha256）；sheets 为空

引擎（WorldView）只在高清地图（art/maps_hd/）上用这张表，×2 画（kHdObjectScale）；没登记的物件照旧用生成的图。

    python tools/artgen/hd_objects.py [--src <定稿目录>]           # 写
    python tools/artgen/hd_objects.py [--src <定稿目录>] --check   # 核 sha256；有源图再重打一遍比像素

确定性：不读时钟、没有随机数；JSON 键排序、UTF-8 无 BOM、LF。
"""

from __future__ import annotations

import argparse
import hashlib
import io
import json
import sys
from dataclasses import dataclass
from pathlib import Path

import numpy as np
from PIL import Image
from scipy import ndimage

sys.path.insert(0, str(Path(__file__).resolve().parent))
import hd_frames  # noqa: E402

REPO = Path(__file__).resolve().parents[2]
OUT_REL = Path("assets") / "art" / "sprites" / "hd_objects"
GAP = 2   # 图集里物件之间的透明间隔


@dataclass(frozen=True)
class Obj:
    sprite: str        # 物件 id（与 sprites/index.json 的 objects.items 同名，引擎按它找）
    name: str
    height: int = 0    # 缩到多高（与 width 二选一）；0 = 跟 pair 用同一个比例
    width: int = 0
    pair: str = ""     # 与哪一个物件共用缩放比例（开 / 关两态要一样大）


# 物件表上的次序（三行 5 / 5 / 3，从左到右）。尺寸是原生像素，游戏里 ×2：人约 47 高。
OBJECTS = (
    Obj("chest_closed", "宝箱（关）", height=20),
    Obj("chest_open", "宝箱（开）", pair="chest_closed"),
    Obj("facility.alchemy", "丹炉", height=40),
    Obj("facility.field", "灵田木牌", height=28),
    Obj("facility.lilian", "历练入口", height=40),
    Obj("facility.meditate", "蒲团", width=26),
    Obj("facility.save", "香炉（存档）", height=34),
    Obj("facility.shop", "货摊", height=40),
    Obj("facility.board", "告示板", height=36),
    Obj("gate_closed", "机关（关）", height=40),
    Obj("gate_open", "机关（开）", pair="gate_closed"),
    Obj("gather_ready", "采集点（可采）", height=22),
    Obj("gather_spent", "采集点（采过）", pair="gather_ready"),
)
ROWS = (5, 5, 3)
FACILITIES = {o.sprite.split(".", 1)[1]: o.sprite for o in OBJECTS if o.sprite.startswith("facility.")}


def cut(sheet: Image.Image) -> list[Image.Image]:
    rgba = np.asarray(sheet.convert("RGBA"))
    solid = rgba[:, :, 3] >= hd_frames.ALPHA_SOLID
    labels, n = ndimage.label(ndimage.binary_dilation(solid, iterations=10))
    sizes = ndimage.sum(solid, labels, index=range(1, n + 1))
    keep = sorted(range(1, n + 1), key=lambda k: (-sizes[k - 1], k))[:len(OBJECTS)]
    if len(keep) < len(OBJECTS):
        raise SystemExit(f"hd_objects: 只切出 {len(keep)} 块，要 {len(OBJECTS)} 个物件")
    boxes = {k: ndimage.find_objects((labels == k).astype(np.int32))[0] for k in keep}
    cy = {k: (b[0].start + b[0].stop) / 2 for k, b in boxes.items()}
    cx = {k: (b[1].start + b[1].stop) / 2 for k, b in boxes.items()}
    by_y = sorted(keep, key=lambda k: (cy[k], cx[k]))
    ordered: list[int] = []
    start = 0
    for count in ROWS:
        ordered += sorted(by_y[start:start + count], key=lambda k: cx[k])
        start += count
    out = []
    for k in ordered:
        ys, xs = boxes[k]
        part = rgba[ys, xs].copy()
        mine = (labels[ys, xs] == k) & solid[ys, xs]
        part[~mine] = 0
        part[mine, 3] = 255
        out.append(hd_frames.crop(Image.fromarray(part, "RGBA")))
    return out


def build(src: Path) -> dict[str, bytes]:
    raw = cut(Image.open(src / "raw" / "objects" / "sheet.png"))
    factors: dict[str, float] = {}
    for o, img in zip(OBJECTS, raw):
        if o.height:
            factors[o.sprite] = o.height / img.height
        elif o.width:
            factors[o.sprite] = o.width / img.width
    small = [hd_frames.crop(hd_frames.scaled(img, factors[o.pair or o.sprite])) for o, img in zip(OBJECTS, raw)]
    # 一行排开，共用一个色板
    width = sum(s.width for s in small) + GAP * (len(small) + 1)
    height = max(s.height for s in small) + GAP * 2
    atlas = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    items = {}
    x = GAP
    for o, s in zip(OBJECTS, small):
        y = height - GAP - s.height
        atlas.alpha_composite(s, (x, y))
        # 锚点：底边中点往上一像素（物件脚底落在对象格的下沿）
        items[o.sprite] = {"name": o.name, "frames": [[x, y, s.width, s.height]],
                           "anchor": [s.width // 2, s.height - 1], "fps": 0.0, "blend": "alpha"}
        x += s.width + GAP
    atlas = hd_frames.limit_palette(atlas, 64)
    png = io.BytesIO()
    atlas.save(png, format="PNG", optimize=True)
    files = {"objects.png": png.getvalue()}
    doc = {
        "_说明": "手绘地图物件（tools/artgen/hd_objects.py 写出，勿手改）。只在高清地图上用、×2 画；"
                 "写法同 sprites/index.json 的 objects / facilities，sheets 为空。files：文件与 sha256。",
        "sheets": {},
        "objects": {"file": "objects.png", "items": items},
        "facilities": dict(sorted(FACILITIES.items())),
        "files": {name: hashlib.sha256(data).hexdigest() for name, data in files.items()},
    }
    files["objects.json"] = (json.dumps(doc, ensure_ascii=False, indent=1, sort_keys=True) + "\n").encode("utf-8")
    return files


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--src", type=Path, default=None)
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args(argv)
    src = args.src or hd_frames.default_src()
    out = REPO / OUT_REL
    if not args.check:
        out.mkdir(parents=True, exist_ok=True)
        for name, data in build(src).items():
            (out / name).write_bytes(data)
        print(f"HD_OBJECTS_WRITTEN：{len(OBJECTS)} 个物件 → {OUT_REL.as_posix()}")
        return 0
    problems = []
    index_path = out / "objects.json"
    if not index_path.exists():
        problems.append(f"缺 {(OUT_REL / 'objects.json').as_posix()}")
    else:
        listed = json.loads(index_path.read_text(encoding="utf-8")).get("files", {})
        for name, digest in listed.items():
            p = out / name
            if not p.exists() or hashlib.sha256(p.read_bytes()).hexdigest() != digest:
                problems.append(f"变 {(OUT_REL / name).as_posix()}（缺或 sha256 不一致）")
        extra = sorted(p.name for p in out.iterdir() if p.is_file() and p.name not in listed and p.name != "objects.json")
        problems += [f"多 {(OUT_REL / n).as_posix()}" for n in extra]
    if not problems and (src / "raw" / "objects" / "sheet.png").exists():
        fresh = build(src)
        a = Image.open(io.BytesIO(fresh["objects.png"])).convert("RGBA")
        b = Image.open(out / "objects.png").convert("RGBA")
        if a.size != b.size or a.tobytes() != b.tobytes():
            problems.append("从源图重打出来像素不同")
        if json.loads(fresh["objects.json"])["objects"] != json.loads(index_path.read_text(encoding="utf-8"))["objects"]:
            problems.append("从源图重打出来物件表不同")
    elif not problems:
        print("HD_OBJECTS_IN_SYNC_PARTIAL：sha256 一致；源图不在，跳过从源图重打（没核）")
        return 0
    if problems:
        for p in problems:
            print(p)
        print("HD_OBJECTS_OUT_OF_SYNC")
        return 1
    print("HD_OBJECTS_IN_SYNC：sha256 一致；从源图重打，像素与物件表相同")
    return 0


if __name__ == "__main__":
    sys.exit(main())
