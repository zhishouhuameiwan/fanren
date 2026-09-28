"""读 maps/*.tmj：图层成 numpy 格阵，对象成字典列表。

只读，且只认 genmaps*.py 产出的那几层（ground / overlay / building / front /
collision / objects）。瓦片语义与 src/game/MapArt.h 的 TileRole、genmaps.py 文件头
那张表是同一份契约：

    1 主地表   2 道路   3 特殊地表（挡路＝水面，可走＝翻土/碎石）   4 front 遮挡
    5 墙体     6 花草   7 碎石裂纹   8 篱笆/摊架/家具（实心）
"""

from __future__ import annotations

import json
from dataclasses import dataclass, field
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parents[2]
MAPS_DIR = ROOT / "maps"
TILE_LOGIC = 32  # tmj 里对象坐标的单位（逻辑格像素）


@dataclass
class MapObject:
    name: str
    type: str
    x: int          # 格
    y: int
    w: int
    h: int
    props: dict

    def cells(self):
        for yy in range(self.y, self.y + self.h):
            for xx in range(self.x, self.x + self.w):
                yield xx, yy


@dataclass
class MapData:
    map_id: str
    w: int
    h: int
    props: dict
    ground: np.ndarray
    overlay: np.ndarray
    building: np.ndarray
    front: np.ndarray
    collision: np.ndarray
    objects: list[MapObject] = field(default_factory=list)

    @property
    def outdoor(self) -> bool:
        return bool(self.props.get("outdoor", True))


def list_map_ids() -> list[str]:
    return sorted(p.stem for p in MAPS_DIR.glob("*.tmj"))


def load(map_id: str) -> MapData:
    raw = json.loads((MAPS_DIR / (map_id + ".tmj")).read_text(encoding="utf-8"))
    w, h = raw["width"], raw["height"]
    layers = {l["name"]: l for l in raw["layers"]}

    def grid(name: str) -> np.ndarray:
        return np.asarray(layers[name]["data"], np.int32).reshape(h, w)

    objs = []
    for o in layers["objects"]["objects"]:
        props = {p["name"]: p["value"] for p in o.get("properties", [])}
        objs.append(MapObject(o["name"], o["type"], o["x"] // TILE_LOGIC, o["y"] // TILE_LOGIC,
                              max(1, o["width"] // TILE_LOGIC), max(1, o["height"] // TILE_LOGIC),
                              props))
    return MapData(
        map_id=map_id, w=w, h=h,
        props={p["name"]: p["value"] for p in raw.get("properties", [])},
        ground=grid("ground"), overlay=grid("overlay"), building=grid("building"),
        front=grid("front"), collision=grid("collision"), objects=objs,
    )
