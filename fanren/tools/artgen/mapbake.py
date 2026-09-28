"""地图烘焙：maps/<id>.tmj + data/visual/maps.json → assets/art/maps/<id>/{below,above}.png + meta.json。

画的顺序（后画的压在先画的上面）：
  地面 → 水面 → 花草碎石 → 山石 / 院墙 / 屋 → 篱笆与家具 → 地面痕迹（脚印、长影）→ 树干 → 门洞门框
  → 统一投影与接触阴影 → 树冠投在地上的影子
  above.png：树冠、屋脊那一段、檐口、门楼、崖檐松枝、伸出墙的树枝
"""

from __future__ import annotations

import json
from pathlib import Path

import numpy as np

import mapdeco
import mapeaves
import mapflora
import mapground
import mapinfer
import maplights
import mapmarks
import mapprops
import maprock
import maproof
import mapshade
import mapwall
import mapwater
import maptmj
import pix

T = 16
ROOT = Path(__file__).resolve().parents[2]
PRESETS = ROOT / "data" / "visual" / "maps.json"


def load_presets() -> dict:
    return json.loads(PRESETS.read_text(encoding="utf-8"))["maps"]


def render(map_id: str, preset: dict):
    """烘焙一张图，返回 (below RGB, above RGBA, meta dict, scene)。"""
    md = maptmj.load(map_id)
    sc = mapinfer.infer(md, preset)
    W, H = md.w * T, md.h * T
    below = pix.Canvas(W, H, opaque=True)
    above = pix.Canvas(W, H, opaque=False)
    ctx = maproof.Ctx(sc, below, above)

    ps = mapground.paint_ground(sc, below)
    mapwater.paint_water(ctx)
    mapdeco.paint_overlay(ctx, ps)
    mapdeco.paint_room_interiors(ctx)
    for st in sc.structs:
        if st.kind == "rock":
            maprock.paint_rock(ctx, st)
        elif st.kind == "wall":
            mapwall.paint_wall(ctx, st)
        elif st.kind == "roofed":
            maproof.paint_roofed(ctx, st)
    mapprops.paint_props(ctx)
    mapprops.paint_facilities(ctx)
    mapmarks.paint_marks(ctx)
    mapflora.paint_trunks(ctx)
    mapeaves.paint_gaps(ctx)
    mapshade.apply(ctx)
    mapflora.paint_canopies(ctx)
    mapeaves.paint_eaves(ctx)
    mapeaves.paint_decos(ctx)
    mapprops.paint_decals(ctx)

    meta = maplights.meta(ctx, preset)
    return below.to_array(False), above.to_array(True), meta, sc


def bake(map_id: str, presets: dict, out_root: Path, fast: bool = False) -> list[Path]:
    below, above, meta, _ = render(map_id, presets[map_id])
    d = out_root / "maps" / map_id
    paths = [pix.save_png(below, d / "below.png", fast), pix.save_png(above, d / "above.png", fast)]
    mp = d / "meta.json"
    mp.write_bytes((json.dumps(meta, ensure_ascii=False, indent=1) + "\n").encode("utf-8"))
    paths.append(mp)
    return paths


def build(out_root: Path, only_map: str | None = None, fast: bool = False) -> list[Path]:
    presets = load_presets()
    ids = [only_map] if only_map else maptmj.list_map_ids()
    missing = [m for m in ids if m not in presets]
    if missing:
        raise KeyError("data/visual/maps.json 缺这些图的预设：%s" % ", ".join(missing))
    out = []
    for mid in ids:
        out += bake(mid, presets, out_root, fast)
    return out
