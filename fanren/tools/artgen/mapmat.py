"""材质渐层：地图、战斗背景、标题共用的一套「每种材质 4–7 级明暗」。

取色一律经 palette.c() / mix()，不写 RGB 常量——色板统一靠这一条守着。
底图整体偏灰一档（与主色板的取向一致）：HD-2D 的亮部由运行时光照补回来，
烘焙图若先把饱和度拉满，夜里一压光就发脏，白天再一加辉光就刺眼。

渐层按「暗 → 亮」排；明度级 0 是最暗那格。各材质的「常用中间级」写在注释里，
画材质时以它为基准上下浮动。
"""

from __future__ import annotations

from palette import c, mix

from pix import BOOK, ramp_id


def _g(col, amt: float, toward: str = "stone2"):
    """往灰里压一点：amt 越大越灰。"""
    return mix(col, toward, amt)


def shifted(rid: int, t: float) -> int:
    """整条渐层挪 t 档（0<t<1 往亮挪，-1<t<0 往暗挪），相邻两级混色得来。

    给「整片比常用级亮 / 暗一点点」的材质用（夯土院子比土路亮一点、屋里地板比栈桥暗一点）。
    直接用小数明度级（3.4、2.6）的话，整片会被 Bayer 抖动成满地棋盘格，×3 放大后就是噪点。
    """
    name = BOOK.name_of(rid)
    cols = BOOK.colors(name)
    n = len(cols)
    if t >= 0:
        out = [mix(cols[i], cols[min(i + 1, n - 1)], t) for i in range(n)]
    else:
        out = [mix(cols[i], cols[max(i - 1, 0)], -t) for i in range(n)]
    return ramp_id("%s%+.2f" % (name, t), out)


def grass(season: str) -> int:
    """草地。常用中间级 2–3；5 只给草尖高光。"""
    if season == "autumn":
        cols = [mix("leaf1", "soil0", 0.55), mix("leaf2", "soil1", 0.5), mix("leaf3", "soil3", 0.5),
                mix("leaf4", "soil4", 0.48), mix("leaf5", "gold2", 0.5), mix("leaf6", "gold3", 0.5)]
    elif season == "summer":
        cols = [mix("leaf0", "ink1", 0.25), _g("leaf1", 0.1), _g(mix("leaf2", "leaf3", 0.5), 0.14),
                _g(mix("leaf3", "leaf4", 0.6), 0.14), _g("leaf5", 0.2, "stone3"), _g("leaf6", 0.22, "stone4")]
    else:  # spring
        cols = [mix("leaf1", "ink1", 0.3), _g("leaf2", 0.18, "stone1"), _g("leaf3", 0.16),
                _g("leaf4", 0.16, "stone3"), _g("leaf5", 0.18, "stone4"), mix("leaf6", "paper", 0.2)]
    return ramp_id("grass_" + season, cols)


def dirt() -> int:
    """土路 / 夯土。常用中间级 2–3。"""
    return ramp_id("dirt", [c("soil0"), c("soil1"), _g("soil2", 0.2), _g("soil3", 0.22, "stone3"),
                            _g("soil4", 0.28, "stone4"), mix("soil5", "paper", 0.3)])


def sand() -> int:
    """演武场沙地、河滩。"""
    return ramp_id("sand", [c("soil1"), _g("soil2", 0.3), mix("soil3", "stone3", 0.5),
                            mix("soil4", "stone4", 0.45), mix("soil5", "paper", 0.35), c("paper")])


def paving(kind: str) -> int:
    """石板。town 偏暖灰；sect 偏青灰（青石）；manor 偏冷白。常用中间级 2–3。"""
    if kind == "sect":
        cols = [mix("tile0", "stone0", 0.5), mix("tile1", "stone1", 0.5), mix("tile2", "stone2", 0.45),
                mix("tile3", "stone3", 0.45), mix("stone4", "ink7", 0.4), mix("stone5", "ink8", 0.4)]
    elif kind == "manor":
        cols = [mix("stone0", "ink1", 0.3), mix("stone1", "tile1", 0.3), mix("stone2", "tile2", 0.25),
                mix("stone3", "ink6", 0.25), mix("stone4", "ink7", 0.3), mix("stone5", "plaster3", 0.3)]
    else:
        cols = [c("stone0"), mix("stone1", "soil1", 0.2), mix("stone2", "soil2", 0.18),
                mix("stone3", "soil3", 0.15), mix("stone4", "soil4", 0.12), c("stone5")]
    return ramp_id("paving_" + kind, cols)


def rock(kind: str) -> int:
    """岩体。cliff 灰褐；valley 带苔的青灰；cave 冷暗；earth 黄土坡（渡口）。"""
    if kind == "cave":
        cols = [c("ink0"), c("ink1"), mix("ink2", "stone1", 0.4), mix("ink3", "stone2", 0.45),
                mix("ink4", "stone3", 0.45), mix("ink5", "stone4", 0.5)]
    elif kind == "earth":
        cols = [mix("soil0", "ink1", 0.3), c("soil1"), mix("soil2", "stone2", 0.3),
                mix("soil3", "stone3", 0.35), mix("soil4", "stone4", 0.35), mix("soil5", "paper", 0.3)]
    elif kind == "valley":
        cols = [mix("stone0", "ink1", 0.35), mix("stone1", "jade0", 0.25), mix("stone2", "moss", 0.22),
                mix("stone3", "jade1", 0.15), mix("stone4", "ink7", 0.2), mix("stone5", "paper", 0.2)]
    elif kind == "dusk":  # 落日峰：岩面偏暖
        cols = [mix("stone0", "violet0", 0.3), mix("stone1", "soil1", 0.3), mix("stone2", "soil2", 0.3),
                mix("stone3", "soil3", 0.3), mix("stone4", "soil4", 0.3), mix("stone5", "gold4", 0.25)]
    else:
        cols = [mix("stone0", "ink1", 0.3), c("stone1"), mix("stone2", "soil2", 0.18),
                mix("stone3", "soil3", 0.15), mix("stone4", "ink7", 0.15), mix("stone5", "paper", 0.15)]
    return ramp_id("rock_" + kind, cols)


def roof(kind: str) -> int:
    """屋瓦。grey 青瓦；jade 宗门深青琉璃；black 墨府黛瓦；thatch 茅草。

    成片的屋顶群要「一户一种瓦色」才读得出是一栋栋屋子，所以每种瓦另有两三个变体：
      grey_indigo 偏蓝的黛青、grey_warm 旧瓦（偏暖的褐灰）、black_indigo、jade_dark、thatch_old。
    常用：瓦垄 3–4、瓦沟 1–2、层间的影 1。
    """
    table = {
        "jade": [c("ink1"), mix("jade0", "ink1", 0.4), mix("jade0", "tile1", 0.35), mix("jade1", "tile2", 0.45),
                 mix("jade2", "tile3", 0.5), mix("jade3", "ink7", 0.5)],
        "jade_dark": [c("ink1"), mix("jade0", "ink1", 0.6), mix("jade0", "tile0", 0.5), mix("jade1", "tile1", 0.55),
                      mix("jade2", "tile2", 0.6), mix("jade3", "tile3", 0.6)],
        "black": [c("ink1"), mix("ink2", "tile0", 0.5), mix("ink3", "tile1", 0.5), mix("ink4", "tile2", 0.5),
                  mix("ink5", "tile3", 0.5), mix("ink6", "tile3", 0.4)],
        "black_indigo": [c("ink1"), mix("ink2", "water0", 0.4), mix("ink3", "water1", 0.4),
                         mix("ink4", "water2", 0.35), mix("ink5", "water3", 0.3), mix("ink6", "water4", 0.3)],
        "thatch": [mix("soil0", "ink1", 0.35), c("soil1"), mix("soil2", "gold1", 0.3), mix("soil3", "gold1", 0.35),
                   mix("soil4", "gold2", 0.35), mix("soil5", "gold3", 0.3)],
        "thatch_old": [mix("soil0", "ink1", 0.4), mix("soil1", "stone1", 0.3), mix("soil2", "stone2", 0.35),
                       mix("soil3", "stone3", 0.35), mix("soil4", "stone4", 0.35), mix("soil5", "stone5", 0.3)],
        # 青瓦整体比主色板的 tile 渐层提亮一档：大片屋顶离主角远、面积大，偏暗就读成一片黑洞
        "grey": [c("tile0"), c("tile1"), mix("tile1", "tile2", 0.6), mix("tile2", "tile3", 0.6),
                 mix("tile3", "ink7", 0.3), mix("tile3", "ink8", 0.55)],
        # 黛青只带一点蓝：蓝多了一整排屋顶像上了釉的蓝瓦，和灰调的底图不是一路
        "grey_indigo": [mix("tile0", "water0", 0.2), mix("tile1", "water1", 0.2), mix("tile2", "water1", 0.2),
                        mix("tile2", "water2", 0.22), mix("tile3", "water3", 0.2), mix("tile3", "water4", 0.25)],
        "grey_warm": [mix("tile0", "soil1", 0.35), mix("tile1", "soil1", 0.35), mix("tile1", "soil2", 0.4),
                      mix("tile2", "soil3", 0.4), mix("tile3", "soil4", 0.4), mix("tile3", "stone5", 0.5)],
    }
    return ramp_id("roof_" + kind, table[kind])


ROOF_VARIANTS = {
    "grey": ["grey", "grey_indigo", "grey_warm"],
    "black": ["black", "black_indigo", "grey_indigo"],
    "jade": ["jade", "jade_dark"],
    "thatch": ["thatch", "thatch_old"],
}


def plaster() -> int:
    """粉墙。常用 3–4；檐下阴影落到 1–2。"""
    return ramp_id("plaster", [mix("stone1", "ink2", 0.3), mix("stone2", "plaster0", 0.4), c("plaster0"),
                               c("plaster1"), c("plaster2"), c("plaster3")])


def earthwall() -> int:
    """夯土墙（村舍）。"""
    return ramp_id("earthwall", [mix("soil0", "ink1", 0.3), c("soil1"), mix("soil2", "stone2", 0.35),
                                 mix("soil3", "stone3", 0.4), mix("soil4", "plaster0", 0.45),
                                 mix("soil5", "plaster1", 0.5)])


def brick() -> int:
    """青砖（镇墙、室内砖地）。"""
    return ramp_id("brick", [mix("ink1", "tile0", 0.5), mix("tile0", "stone1", 0.5), mix("tile1", "stone2", 0.5),
                             mix("tile2", "stone3", 0.5), mix("tile3", "stone4", 0.5), mix("stone5", "ink8", 0.4)])


def wood() -> int:
    """木作：梁柱、篱笆、家具、地板。常用 2–3。"""
    return ramp_id("wood", [c("wood0"), c("wood1"), c("wood2"), c("wood3"), c("wood4"), c("wood5")])


def darkwood() -> int:
    """深色木作：室内墙顶、檀木家具。"""
    return ramp_id("darkwood", [c("ink0"), mix("wood0", "ink0", 0.4), c("wood0"), c("wood1"),
                                mix("wood2", "wood1", 0.5), c("wood2")])


def floorwood() -> int:
    """木地板：比梁柱浅、偏灰，免得满屋子一片橙。"""
    return ramp_id("floorwood", [c("wood0"), mix("wood1", "stone1", 0.2), mix("wood2", "stone2", 0.25),
                                 mix("wood3", "stone3", 0.3), mix("wood4", "stone4", 0.35),
                                 mix("wood5", "paper", 0.35)])


def bamboo() -> int:
    return ramp_id("bamboo", [mix("leaf0", "ink1", 0.3), c("leaf1"), mix("leaf3", "bamboo", 0.5),
                              c("bamboo"), mix("bamboo", "leaf6", 0.5), mix("leaf7", "paper", 0.3)])


def lacquer() -> int:
    """朱漆柱、灯笼。"""
    return ramp_id("lacquer", [c("red0"), c("red1"), c("red2"), c("red3"), c("red4")])


def gold() -> int:
    return ramp_id("gold", [c("gold0"), c("gold1"), c("gold2"), c("gold3"), c("gold4")])


def water() -> int:
    """水面。深处 0–1，浅处 2–3，泡沫 5。"""
    return ramp_id("water", [c("water0"), c("water1"), c("water2"), c("water3"), c("water4"), c("water5")])


def foliage(kind: str) -> int:
    """树冠。broad 阔叶；pine 松（偏青）；willow 柳（偏黄绿）；bamboo 竹叶。"""
    if kind == "pine":
        cols = [mix("leaf0", "ink0", 0.35), mix("pine", "leaf0", 0.5), c("pine"),
                mix("pine", "jade1", 0.4), mix("pine", "leaf4", 0.5), mix("leaf5", "jade3", 0.35)]
    elif kind == "willow":
        cols = [mix("leaf1", "ink1", 0.3), c("leaf2"), mix("leaf3", "moss", 0.3), mix("leaf5", "moss", 0.2),
                mix("leaf6", "leaf7", 0.4), mix("leaf7", "paper", 0.25)]
    elif kind == "bamboo":
        cols = [mix("leaf0", "ink1", 0.2), c("leaf1"), mix("leaf3", "bamboo", 0.3), c("bamboo"),
                mix("bamboo", "leaf6", 0.6), mix("leaf7", "paper", 0.2)]
    elif kind == "autumn":
        cols = [mix("autumn0", "ink1", 0.3), c("autumn0"), c("autumn1"), c("autumn2"), c("autumn3"),
                mix("autumn3", "gold4", 0.5)]
    elif kind == "ginkgo":
        cols = [mix("soil1", "ink1", 0.3), mix("gold0", "leaf1", 0.4), c("gold1"), mix("gold2", "leaf5", 0.25),
                c("gold3"), c("gold4")]
    elif kind == "dark":  # 夜里的林子、野外密林：更深更冷
        cols = [c("ink0"), mix("leaf0", "ink0", 0.3), c("leaf0"), mix("leaf1", "pine", 0.5), c("leaf2"),
                mix("leaf3", "jade2", 0.3)]
    else:
        cols = [mix("leaf0", "ink0", 0.25), c("leaf1"), c("leaf2"), mix("leaf3", "leaf4", 0.4),
                mix("leaf4", "leaf5", 0.5), mix("leaf6", "leaf7", 0.35)]
    return ramp_id("foliage_" + kind, cols)


def blossom() -> int:
    return ramp_id("blossom", [c("blossom0"), c("blossom1"), c("blossom2"), c("blossom3"),
                               mix("blossom3", "paper", 0.6)])


def bark() -> int:
    return ramp_id("bark", [mix("wood0", "ink0", 0.4), c("wood0"), mix("wood1", "stone1", 0.3),
                            mix("wood2", "stone2", 0.35), mix("wood3", "stone3", 0.4)])


def void() -> int:
    """室内墙顶 / 洞顶：几乎是黑，但留两级给边沿。"""
    return ramp_id("void", [c("ink0"), mix("ink0", "ink1", 0.5), c("ink1"), c("ink2"), c("ink3")])


def paper() -> int:
    """窗纸、灯笼纸、告示。"""
    return ramp_id("paper", [mix("plaster0", "soil2", 0.3), c("plaster0"), mix("plaster1", "gold4", 0.3),
                             mix("paper", "gold4", 0.3), c("paper")])


def cloth(kind: str) -> int:
    """布棚、幌子、帐幔。indigo 靛蓝；ochre 赭黄；red 朱；white 素。"""
    if kind == "ochre":
        cols = [c("soil1"), mix("soil2", "gold1", 0.5), c("gold1"), mix("gold2", "soil4", 0.4), c("gold3")]
    elif kind == "red":
        cols = [c("red0"), c("red1"), c("red2"), mix("red3", "red4", 0.4), c("red4")]
    elif kind == "white":
        cols = [mix("stone2", "ink3", 0.3), c("stone3"), c("plaster1"), c("plaster2"), c("plaster3")]
    elif kind == "violet":
        cols = [c("violet0"), c("violet1"), c("violet2"), c("violet3"), mix("violet3", "paper", 0.4)]
    else:
        cols = [mix("water0", "ink1", 0.4), c("water1"), mix("water2", "ink4", 0.3), mix("water3", "ink5", 0.3),
                mix("water4", "ink7", 0.3)]
    return ramp_id("cloth_" + kind, cols)


def metal() -> int:
    """兵刃、铜钉、铁器。"""
    return ramp_id("metal", [c("ink1"), c("ink3"), c("ink5"), c("ink6"), c("ink8")])


def herb() -> int:
    """药苗、菜蔬：比草地更鲜一点，一眼看得出是种的。"""
    return ramp_id("herb", [c("leaf1"), c("leaf3"), mix("leaf4", "jade2", 0.3), c("leaf5"), c("leaf6"),
                            c("leaf7")])


def flower(kind: str) -> int:
    if kind == "yellow":
        cols = [c("gold1"), c("gold2"), c("gold3"), c("gold4")]
    elif kind == "white":
        cols = [c("stone3"), c("plaster1"), c("plaster3"), c("paper")]
    elif kind == "violet":
        cols = [c("violet1"), c("violet2"), c("violet3"), mix("violet3", "paper", 0.5)]
    elif kind == "red":
        cols = [c("red1"), c("red2"), c("red3"), c("red4")]
    else:
        cols = [c("blossom0"), c("blossom1"), c("blossom2"), c("blossom3")]
    return ramp_id("flower_" + kind, cols)


def glow() -> int:
    """灯火、丹炉的火口、萤光：只给「本身发光」的像素用。"""
    return ramp_id("glow", [c("red2"), c("red3"), mix("red4", "gold3", 0.5), c("gold3"), c("gold4"),
                            mix("gold4", "paper", 0.6)])


def jadeglow() -> int:
    return ramp_id("jadeglow", [c("jade0"), c("jade1"), c("jade2"), c("jade3"), c("jade4")])
