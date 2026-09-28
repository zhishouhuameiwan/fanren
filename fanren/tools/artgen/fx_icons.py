"""UI 小图标（16×16，运行时 ×2）：十种攻击类别、未知、架势、劲珠满/空、选择指针、物件（修为/碎银/药）。

与人物同一套模板 + 色阶 + 自动描边，所以和世界里的像素是同一种笔触；
颜色取主色板，UI 的墨金主题（施工图 1.6）在运行时由面板负责，这里只管图标本身。
每个模板按剪影的包围盒自动居中到 16×16 格里——手画时不用数边距。
"""

from __future__ import annotations

from PIL import Image

import palette as P
from sprites_core import Canvas, Materials, add_outline, to_image, tpl

SIZE = 16


def _r(*names: str) -> list[tuple[int, int, int]]:
    return [P.c(n) for n in names]


RAMPS = {
    "steel": _r("ink3", "ink5", "ink7", "ink8"),
    "hilt": _r("wood0", "wood1", "wood2", "wood3"),
    "gold": _r("gold0", "gold1", "gold2", "gold3"),
    "skin": _r("skin0", "skin1", "skin2", "skin3"),
    "wrapw": _r("stone3", "stone4", "plaster2", "plaster3"),
    "red": _r("red1", "red2", "red3", "red4"),
    "violet": [P.mix("violet0", "ink1", 0.3), P.c("violet1"), P.c("violet2"), P.c("violet3")],
    "venom": _r("leaf3", "leaf5", "leaf6", "leaf7"),
    "leaf": _r("leaf2", "leaf3", "leaf5", "leaf6"),
    "water": _r("water1", "water2", "water3", "water4"),
    "flame": _r("red1", "red3", "gold3", "gold4"),
    "earth": _r("soil1", "soil2", "soil3", "soil4"),
    "disc": _r("ink3", "ink4", "ink5", "ink6"),
    "paper": _r("stone4", "stone5", "plaster3", "paper"),
    "bronze": [P.mix("gold0", "jade0", 0.55), P.mix("gold1", "jade0", 0.45), P.mix("gold1", "jade2", 0.3), P.mix("gold2", "jade3", 0.25)],
    "jade": _r("jade0", "jade1", "jade2", "jade3"),
    "silver": _r("stone2", "stone4", "ink7", "ink8"),
    "porcelain": _r("stone3", "plaster1", "plaster2", "plaster3"),
    "pattern": _r("water1", "water2", "water2", "water3"),
    "hollow": _r("ink2", "ink3", "ink4", "ink5"),
    "white": _r("plaster2", "plaster3", "paper", "paper"),
    "chest": [P.mix("red0", "wood0", 0.5), P.mix("red1", "wood1", 0.5), P.mix("red2", "wood2", 0.5), P.mix("red3", "wood3", 0.5)],
}
MATS = Materials.build(RAMPS)


def codes(main: str, sub: str | None = None, **extra: tuple[str, int]) -> dict:
    d = {"A": (main, 3), "B": (main, 2), "C": (main, 1), "D": (main, 0), "W": ("white", 2)}
    if sub:
        d.update({"E": (sub, 3), "F": (sub, 2), "G": (sub, 1)})
    d.update(extra)
    return d


ICONS: list[tuple[str, str, str, dict]] = [
    ("attack.sword", "剑", """
        ...........AA
        ..........ABA
        .........ABC.
        ........ABC..
        .......ABC...
        ......ABC....
        .Z...ABC.....
        ..Z.ABC......
        ...ZBC.......
        ...FZZ.......
        ..FG..Z......
        .FG..........
        rG...........
        r............
        """, codes("steel", "hilt", Z=("gold", 3), r=("red", 2))),
    ("attack.blade", "刀", """
        ..........AA.
        .........AABB
        ........AABBC
        .......AABBC.
        ......AABBC..
        .....AABBC...
        ....AABBC....
        ...ZZBBC.....
        ..ZZZCC......
        ..FZ.........
        .FG..........
        FG...........
        rr...........
        """, codes("steel", "hilt", Z=("gold", 3), r=("red", 2))),
    ("attack.fist", "拳", """
        ....AAAAAA....
        ...ABBBBBBC...
        ..ABsBBsBBsC..
        ..ABBBBBBBBC..
        ..ABsssssssC..
        ..ABBBBBBBBC..
        ...ABBBBBBC...
        ...ABBBBBCC...
        ....CBBBBC....
        ....EFFFFG....
        ....EFFFFG....
        ....GGGGGG....
        """, codes("skin", "wrapw", s=("skin", 1))),
    ("attack.hidden", "暗器", """
        ............A.
        ...........ABA
        ..........ABC.
        .........ABC..
        ........ABC...
        .......BCC....
        ......FG......
        .....FG.......
        ....FG........
        ...rr.........
        ..r.r.........
        .r..r.........
        """, codes("steel", "hilt", r=("red", 2))),
    ("attack.poison", "毒", """
        .....EEE.....
        .....FFF.....
        ....ABBBC....
        .....ABC.....
        ...AABBBCC...
        ..ABBBBBBBC..
        .ABBjjjjBBBC.
        .ABBjJJjBBBCj
        .ABBjjjjBBBCj
        .ABBBBBBBBBC.
        ..CBBBBBBBC.j
        ...CCCCCCC...
        """, codes("violet", "hilt", j=("venom", 2), J=("venom", 3))),
    ("element.metal", "金", """
        ......AA......
        .....ZZZZ.....
        ....ABBBBC....
        ...ABBBBBBC...
        ...ABWBBBBC...
        ...ABWBBBBC...
        ..ABBBBBBBBC..
        ..ABBBBBBBBC..
        .ABBBBBBBBBBC.
        .DDDDDDDDDDDD.
        ......CC......
        """, codes("gold", None, Z=("gold", 3))),
    ("element.wood", "木", """
        ........AAA..
        ......AABBBC.
        .....ABBBBBC.
        ....ABBBBBC..
        ....ABBBBC...
        ...ABBBBC....
        ...ABBCC.AA..
        ...ACC..ABBC.
        ..E....ABBC..
        .E....CCC....
        .E...........
        .G...........
        """, codes("leaf", "earth")),
    ("element.water", "水", """
        ......A......
        ......AB.....
        .....ABBC....
        .....ABBC....
        ....ABBBBC...
        ...ABWBBBBC..
        ...ABWBBBBC..
        ...ABBBBBBC..
        ...CBBBBBBC..
        ....CBBBBC...
        .....CCCC....
        """, codes("water")),
    ("element.fire", "火", """
        ......A......
        .....AB....A.
        .....ABA..AB.
        ....ABBBAABB.
        ...ABByBBBBC.
        ...ABByyBBBC.
        ..ABByyyyBBC.
        ..ABByyWyyBC.
        ..ABByyyyyBC.
        ...CByyyyBC..
        ....CCCCCC...
        """, codes("flame", None, y=("flame", 3), W=("white", 3))),
    ("element.earth", "土", """
        .....AA......
        ....ABBC.....
        ...ABBBBC..A.
        ..ABBWBBBCABC
        .ABBBBBBBBBBC
        ABBBBBBBBBBBC
        ABBBlBBBBlBBC
        CCCCCCCCCCCCC
        """, codes("earth", None, l=("leaf", 2))),
    ("mark.unknown", "未知", """
        ....AAAAAA....
        ...ABBBBBBC...
        ..ABBWWWBBBC..
        ..ABWBBBWBBC..
        ..ABBBBBWBBC..
        ..ABBBBWBBBC..
        ..ABBBWBBBBC..
        ..ABBBWBBBBC..
        ..ABBBBBBBBC..
        ..ABBBWBBBBC..
        ...CBBBBBBC...
        ....CCCCCC....
        """, codes("disc", None, W=("paper", 3))),
    ("mark.stance", "架势", """
        ....AAAAAA....
        ...ABBBBBBC...
        ..ABBEEEEBBC..
        .ABBEBBBBEBBC.
        .ABEBBZZBBEBC.
        .ABEBZWZZBEBC.
        .ABEBBZZBBEBC.
        .ABBEBBBBEBBC.
        ..ABBEEEEBBC..
        ...CBBBBBBC...
        ....CCCCCC....
        """, codes("bronze", "gold", Z=("gold", 3))),
    ("mark.bp_full", "劲·满", """
        ...AAAA...
        ..ABWWBC..
        .ABWBBBBC.
        .ABBBBBBC.
        .ABBBBBBC.
        .CBBBBBBC.
        ..CBBBBC..
        ...CCCC...
        """, codes("gold")),
    ("mark.bp_empty", "劲·空", """
        ...AAAA...
        ..A....C..
        .A......C.
        .A......C.
        .A......C.
        .C......C.
        ..C....C..
        ...CCCC...
        """, codes("hollow")),
    ("mark.pointer", "指针", """
        ....A....
        ...ABA...
        ..ABWBA..
        .ABBWBBC.
        ABBBBBBBC
        .CBBBBBC.
        ..CBBBC..
        ...CBC...
        ....C....
        """, codes("gold")),
    ("item.cultivation", "修为", """
        ....AAAAAA....
        ...ABBBBBBC...
        ..ABWWBBBBBC..
        ..ABWBBjjBBC..
        ..ABBBjBBjBC..
        ..ABBjBBBjBC..
        ..ABBjBBjBBC..
        ..ABBBjjBBBC..
        ...CBBBBBBC...
        ....CCCCCC....
        """, codes("jade", None, j=("jade", 0))),
    ("item.silver", "碎银", """
        .....AAA.....
        ....ABBBC....
        AAA.ABWBBC...
        BBBCABBBBC...
        BWBBCCBBCC...
        BBBBC.CCCAAA.
        CBBCC...ABWBC
        .CC.....CBBCC
        .........CC..
        """, codes("silver")),
    ("item.chest", "宝箱", """
        .AAAAAAAAAA.
        ABBBBBBBBBBC
        ABZZZZZZZZBC
        ABBBBZZBBBBC
        ZZZZZZkZZZZZ
        ABBBBZZBBBBC
        ABBBBBBBBBBC
        ABZZZZZZZZBC
        CCCCCCCCCCCC
        """, codes("chest", None, Z=("gold", 3), k=("hollow", 0))),
    ("item.medicine", "药", """
        .....rrr.....
        .....rrr.....
        ....AAAAA....
        .....ABC.....
        ....ABBBC....
        ...ABBjBBC...
        ..ABBjjjBBC..
        ..ABBBjBBBC..
        ..ABBBBBBBC..
        ...CBBBBBC...
        ....CCCCC....
        """, codes("porcelain", None, j=("pattern", 2), r=("red", 2))),
]

# 战斗里的攻击类别（施工图 2.2）→ 图标
CATEGORY_ICONS = {
    "剑": "attack.sword", "刀": "attack.blade", "拳": "attack.fist", "暗器": "attack.hidden", "毒": "attack.poison",
    "金": "element.metal", "木": "element.wood", "水": "element.water", "火": "element.fire", "土": "element.earth",
}


def render_icon(text: str, cmap: dict) -> Image.Image:
    rows = tpl(text)
    c = Canvas(SIZE, SIZE)
    w = max(len(r) for r in rows)
    h = len(rows)
    ox = (SIZE - w) // 2
    oy = (SIZE - h) // 2
    c.stamp(rows, ox, oy, cmap)
    # 实际剪影再居中一次（模板两侧常带空列）
    bb = c.bbox()
    if bb is not None:
        x0, y0, x1, y1 = bb
        dx = (SIZE - 1 - x1 - x0) // 2
        dy = (SIZE - 1 - y1 - y0) // 2
        c = c.shifted(dx, dy)
    return to_image(add_outline(c, MATS), MATS)


def build_icons(cols: int = 8) -> tuple[Image.Image, dict]:
    n = len(ICONS)
    rows = (n + cols - 1) // cols
    sheet = Image.new("RGBA", (cols * SIZE, rows * SIZE), (0, 0, 0, 0))
    index: dict[str, dict] = {}
    for i, (iid, name, text, cmap) in enumerate(ICONS):
        img = render_icon(text, cmap)
        x, y = (i % cols) * SIZE, (i // cols) * SIZE
        sheet.paste(img, (x, y))
        index[iid] = {"name": name, "rect": [x, y, SIZE, SIZE]}
    return sheet, index
