"""全作像素美术的主色板。

为什么要一张主色板、而不是每个生成器各挑各的颜色：
地图烘焙（maps_bake / backdrops）与人物精灵（sprites / fx）由两条生成线分头写，
两边若各自调色，放进同一帧里就是两种画风——草地是一种绿、人物衣服是另一种
「看起来也是绿」的绿，HD-2D 最忌讳的正是这种「贴上去的」感觉。

规矩：
  * 只许**追加**颜色，不许改已有名字的值（改了会让已经烘焙好的图与新图不一致，
    而 ARTGEN_IN_SYNC 只会告诉你「变了」，不会告诉你「哪边才是对的」）。
  * 取色一律经过 `c(name)`，拼错的名字当场抛 KeyError，不静默退到黑色。
  * 需要中间色时用 `mix(a, b, t)`，不要在各处手写 RGB 常量。

取向：墨色打底、宣纸白提亮、青瓦粉墙、松竹苔绿、朱砂与金线点睛。
饱和度整体压低一档，靠光照与辉光在运行时把「亮」补回来——这是 HD-2D 的做法：
底图偏灰，光是后加的。
"""

from __future__ import annotations

# (r, g, b)。按用途分组，组内由暗到亮。
PALETTE: dict[str, tuple[int, int, int]] = {
    # ---- 墨（描边、阴影、夜色） ----
    "ink0": (14, 15, 20),
    "ink1": (27, 29, 38),
    "ink2": (42, 45, 58),
    "ink3": (59, 63, 79),
    "ink4": (80, 85, 106),
    "ink5": (107, 113, 137),
    "ink6": (142, 148, 168),
    "ink7": (181, 185, 198),
    "ink8": (220, 221, 227),
    # ---- 宣纸 / 石 / 土 ----
    "paper": (241, 234, 216),
    "stone0": (58, 54, 50),
    "stone1": (84, 79, 72),
    "stone2": (112, 106, 96),
    "stone3": (143, 136, 122),
    "stone4": (176, 168, 150),
    "stone5": (209, 201, 182),
    "soil0": (58, 40, 26),
    "soil1": (84, 60, 38),
    "soil2": (112, 83, 54),
    "soil3": (141, 108, 72),
    "soil4": (172, 139, 97),
    "soil5": (201, 172, 128),
    # ---- 木（梁柱、篱笆、家具、地板） ----
    "wood0": (43, 26, 16),
    "wood1": (74, 46, 26),
    "wood2": (110, 69, 38),
    "wood3": (149, 99, 58),
    "wood4": (189, 138, 85),
    "wood5": (219, 176, 122),
    # ---- 草木（草地、树冠、竹、苔） ----
    "leaf0": (15, 42, 30),
    "leaf1": (26, 61, 40),
    "leaf2": (39, 84, 53),
    "leaf3": (55, 107, 64),
    "leaf4": (79, 138, 76),
    "leaf5": (111, 166, 90),
    "leaf6": (149, 194, 111),
    "leaf7": (194, 220, 142),
    "moss": (98, 118, 62),
    "bamboo": (122, 158, 84),
    "pine": (34, 70, 58),
    # ---- 青 / 玉（青瓦、玉佩、灵光） ----
    "jade0": (29, 74, 74),
    "jade1": (46, 107, 98),
    "jade2": (74, 148, 128),
    "jade3": (124, 191, 163),
    "jade4": (186, 226, 204),
    "tile0": (38, 44, 56),     # 青瓦暗面
    "tile1": (58, 66, 82),
    "tile2": (82, 92, 110),
    "tile3": (112, 122, 140),  # 青瓦受光面
    # ---- 水 ----
    "water0": (19, 35, 61),
    "water1": (31, 58, 95),
    "water2": (47, 91, 134),
    "water3": (79, 134, 173),
    "water4": (134, 183, 208),
    "water5": (196, 226, 234),
    # ---- 朱（漆柱、灯笼、血、敌方） ----
    "red0": (61, 20, 20),
    "red1": (107, 31, 26),
    "red2": (154, 47, 36),
    "red3": (194, 72, 47),
    "red4": (224, 116, 90),
    # ---- 金（金线、灯火、法光） ----
    "gold0": (90, 63, 18),
    "gold1": (138, 100, 32),
    "gold2": (194, 154, 58),
    "gold3": (232, 199, 102),
    "gold4": (247, 230, 166),
    # ---- 紫（黄昏、识海、邪气） ----
    "violet0": (42, 29, 58),
    "violet1": (71, 48, 96),
    "violet2": (110, 74, 138),
    "violet3": (157, 122, 184),
    # ---- 肤色 ----
    "skin0": (126, 82, 56),
    "skin1": (178, 126, 92),
    "skin2": (217, 168, 130),
    "skin3": (242, 208, 176),
    # ---- 白墙（粉墙黛瓦的「粉」） ----
    "plaster0": (150, 144, 132),
    "plaster1": (190, 184, 170),
    "plaster2": (222, 216, 202),
    "plaster3": (240, 236, 226),
}


def c(name: str) -> tuple[int, int, int]:
    """按名字取色；名字不存在当场抛 KeyError。"""
    return PALETTE[name]


def ca(name: str, alpha: int) -> tuple[int, int, int, int]:
    """带 alpha 的取色。"""
    r, g, b = PALETTE[name]
    return (r, g, b, alpha)


def mix(a: str | tuple[int, int, int], b: str | tuple[int, int, int], t: float) -> tuple[int, int, int]:
    """两色线性混合，t=0 为 a、t=1 为 b。参数可以是色名或 RGB 三元组。"""
    ra = PALETTE[a] if isinstance(a, str) else a
    rb = PALETTE[b] if isinstance(b, str) else b
    return tuple(int(round(ra[i] + (rb[i] - ra[i]) * t)) for i in range(3))  # type: ignore[return-value]


# 常用的「一组渐层」，给需要 3–5 级明暗的地方直接取用。
RAMPS: dict[str, list[str]] = {
    "grass": ["leaf1", "leaf2", "leaf3", "leaf4", "leaf5"],
    "foliage": ["leaf0", "leaf1", "leaf2", "leaf3", "leaf4", "leaf6"],
    "stone": ["stone0", "stone1", "stone2", "stone3", "stone4", "stone5"],
    "soil": ["soil0", "soil1", "soil2", "soil3", "soil4", "soil5"],
    "wood": ["wood0", "wood1", "wood2", "wood3", "wood4", "wood5"],
    "roof": ["tile0", "tile1", "tile2", "tile3"],
    "water": ["water0", "water1", "water2", "water3", "water4", "water5"],
    "plaster": ["plaster0", "plaster1", "plaster2", "plaster3"],
    "lacquer": ["red0", "red1", "red2", "red3", "red4"],
    "gold": ["gold0", "gold1", "gold2", "gold3", "gold4"],
    "ink": ["ink0", "ink1", "ink2", "ink3", "ink4", "ink5", "ink6", "ink7", "ink8"],
    "jade": ["jade0", "jade1", "jade2", "jade3", "jade4"],
    "skin": ["skin0", "skin1", "skin2", "skin3"],
    "violet": ["violet0", "violet1", "violet2", "violet3"],
}


def ramp(name: str) -> list[tuple[int, int, int]]:
    return [PALETTE[n] for n in RAMPS[name]]


# ---------------------------------------------------------------------------
# 追加（A1 地图美术）：只加新名字，上面已有的值一个不动。
#
# 桃花粉与秋色两组，是因为主色板里没有这两种色相：用 mix() 从朱砂与宣纸调出来的
# 粉发灰发脏，撑不起「一树桃花」；枫叶若只用朱砂那一组，远看就是一团血色。
# ---------------------------------------------------------------------------
PALETTE.update({
    "blossom0": (110, 58, 74),
    "blossom1": (168, 92, 112),
    "blossom2": (214, 142, 156),
    "blossom3": (240, 200, 204),
    "autumn0": (92, 42, 26),
    "autumn1": (146, 66, 32),
    "autumn2": (192, 104, 44),
    "autumn3": (226, 156, 70),
})
RAMPS.update({
    "blossom": ["blossom0", "blossom1", "blossom2", "blossom3"],
    "autumn": ["autumn0", "autumn1", "autumn2", "autumn3"],
})
