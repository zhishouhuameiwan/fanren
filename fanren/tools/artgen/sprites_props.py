"""人物配件：随身带的（背剑、腰刀、药篓、扁担……）与战斗里握在手上的兵刃。

配件是区分身份的第二手段（第一是发型头饰）：同一身短打，背着篓子的是采药的，
扛着扁担的是脚夫，腰里别着刀的是马贼。

随身配件按朝向给模板、锚点、偏移与层次：
  锚点  head（头框左上角）、belt_l / belt_r（腰带最左 / 最右一格）、hand1 / hand2（近手 / 远手）
  层次  behind（先于身体画，被身子挡住）、body（身体之后、头之前）、front（最后画）
战斗兵刃按姿态给模板，模板里的 '+' 是握持点，对齐到近手 hand1。

码表：
  A B C D  主料 3/2/1/0 级（兵刃是钢，篓子是竹，扇子是纸……由配件自己定）
  E F G    副料 3/2/1 级（刀柄、剑鞘、篓口）
  Z        金（护手、饰件）
  r        红（剑穗、刀彩）
  W        白（拂尘的马尾）
  +        握持点，不画
"""

from __future__ import annotations

from dataclasses import dataclass

from sprites_core import tpl


@dataclass(frozen=True)
class Placed:
    rows: tuple[str, ...]
    anchor: str
    dx: int
    dy: int
    z: str = "front"   # behind / body / front


def placed(text: str, anchor: str, dx: int, dy: int, z: str = "front") -> Placed:
    return Placed(tuple(tpl(text)), anchor, dx, dy, z)


@dataclass(frozen=True)
class Carry:
    """随身配件：主料/副料材质 + 三个朝向的摆放。"""

    main: str
    sub: str
    views: dict[str, tuple[Placed, ...]]
    weapon: str | None = None   # 这件配件本身就是兵刃（战斗帧里改由手持兵刃出场，背上/腰间不再画）


CARRY: dict[str, Carry] = {}

# ---- 背剑 ----
CARRY["sword_back"] = Carry("steel_sheath", "hilt", {
    "F": (placed("""
        ..F
        .F.
        Z..
        """, "head", 10, 6, "behind"),),
    "L": (placed("""
        .F..
        ..F.
        ..FZ
        ...C
        ...C
        ....C
        ....C
        ....D
        """, "head", 8, 5, "behind"),),
    "B": (placed("""
        F......
        .F.....
        ..Z....
        ...C...
        ....C..
        .....C.
        ......D
        """, "head", 7, 6, "body"),),
}, weapon="sword")
# ---- 背刀：比剑宽一格，刀柄缠红 ----
CARRY["dao_back"] = Carry("dao_sheath", "hilt", {
    "F": (placed("""
        ..rF
        .FF.
        ZZ..
        """, "head", 9, 5, "behind"),),
    "L": (placed("""
        rF...
        .FF..
        ..ZZ.
        ..BCC
        ...BC
        ...BC
        ....BC
        ....CD
        """, "head", 8, 4, "behind"),),
    "B": (placed("""
        rF......
        .FF.....
        ..ZZ....
        ...BC...
        ....BC..
        .....BC.
        ......CD
        """, "head", 6, 5, "body"),),
}, weapon="dao")
# ---- 腰剑 ----
CARRY["sword_hip"] = Carry("steel_sheath", "hilt", {
    "F": (placed("""
        .F
        Z.
        C.
        C.
        D.
        """, "belt_l", -1, -1, "front"),),
    "L": (placed("""
        F....
        .Z...
        ..CC.
        ....CD
        """, "belt_l", 1, 0, "front"),),
    "B": (placed("""
        F.
        .Z
        .C
        .C
        .D
        """, "belt_r", 0, -1, "front"),),
}, weapon="sword")
# ---- 腰刀 ----
CARRY["dao_hip"] = Carry("dao_sheath", "hilt", {
    "F": (placed("""
        rF
        ZZ
        BC
        BC
        CD
        """, "belt_l", -1, -1, "front"),),
    "L": (placed("""
        rF....
        .ZZ...
        ..BBC.
        ...BCCD
        """, "belt_l", 1, -1, "front"),),
    "B": (placed("""
        Fr
        ZZ
        BC
        BC
        CD
        """, "belt_r", 0, -1, "front"),),
}, weapon="dao")
# ---- 背篓 / 药篓 ----
CARRY["basket"] = Carry("straw", "straw_dark", {
    # 正面只露出篓口两角（在肩后），背带压在肩上
    "F": (placed("""
        E........E
        A........B
        """, "head", 1, 9, "behind"),),
    "L": (placed("""
        EEEE.
        ABFBC
        BBBBC
        BFBCC
        BBBCD
        .CCD.
        """, "head", 9, 10, "front"),),
    "B": (placed("""
        .EEEEEE.
        ABBFBBBC
        BFBBBFBC
        BBBBBBCC
        BFBBFBCD
        .CCCCDD.
        """, "head", 2, 10, "front"),),
})
# ---- 扁担：扛在右肩（画面左侧的肩），前低后高 ----
CARRY["pole"] = Carry("wood", "wood", {
    # 扁担压在肩上：正面只露出肩后翘起的一截；侧面前低后高，画在身子后面，
    # 中段被头和身子挡住——不然一根杆子正好从脸上穿过去
    "F": (placed("""
        B.
        B.
        .B
        .C
        """, "head", -1, 5, "behind"),),
    "L": (placed("""
        ...............B
        .............BC.
        ...........BC...
        .........BC.....
        .......BC.......
        .....BC.........
        ...BC...........
        .BC.............
        """, "head", -2, 6, "behind"),),
    "B": (placed("""
        .B
        .B
        B.
        C.
        """, "head", 11, 5, "front"),),
})
# ---- 锄头：扛在肩上 ----
CARRY["hoe"] = Carry("wood", "steel_dark", {
    "F": (placed("""
        EF
        B.
        B.
        .B
        .C
        """, "head", -1, 4, "behind"),),
    "L": (placed("""
        ..............EF
        ..............FB
        .............BC.
        ...........BC...
        .........BC.....
        .......BC.......
        .....BC.........
        ...BC...........
        .BC.............
        """, "head", -2, 5, "behind"),),
    "B": (placed("""
        FE
        .B
        .B
        B.
        C.
        """, "head", 11, 4, "front"),),
})
# ---- 朴刀：长木杆头上一口宽刃，扛在肩上（剪径贼）----
CARRY["podao"] = Carry("wood", "steel", {
    "F": (placed("""
        E.
        EF
        B.
        B.
        .B
        .C
        """, "head", -1, 3, "behind"),),
    "L": (placed("""
        ..............EE
        .............EFB
        ............BC..
        ..........BC....
        ........BC......
        ......BC........
        ....BC..........
        ..BC............
        """, "head", -2, 3, "behind"),),
    "B": (placed("""
        .E
        FE
        .B
        .B
        B.
        C.
        """, "head", 11, 3, "front"),),
}, weapon="podao")
# ---- 酒葫芦：挂在腰间 ----
CARRY["gourd"] = Carry("gourd", "cord", {
    "F": (placed("""
        .E
        AB
        BC
        """, "belt_r", 0, 0, "front"),),
    "L": (placed("""
        E.
        AB
        BC
        """, "belt_r", 0, 0, "front"),),
    "B": (placed("""
        E.
        AB
        BC
        """, "belt_l", -1, 0, "front"),),
})
# ---- 折扇：拿在近手 ----
CARRY["fan"] = Carry("paper_fan", "darkwood", {
    "F": (placed("""
        A
        B
        E
        """, "hand1", 0, -2, "front"),),
    "L": (placed("""
        AB
        .E
        """, "hand1", -1, -1, "front"),),
    "B": (),
})
# ---- 拂尘：搭在臂弯 ----
CARRY["whisk"] = Carry("horsetail", "darkwood", {
    "F": (placed("""
        E.
        E.
        AB
        AB
        BC
        """, "hand1", 0, -3, "front"),),
    "L": (placed("""
        .E
        .E
        AB
        AB
        BC
        """, "hand1", -1, -3, "front"),),
    "B": (),
})
# ---- 算盘：挂在腰前 ----
CARRY["abacus"] = Carry("darkwood", "abacus_bead", {
    "F": (placed("""
        BBBB
        FEFE
        BBBB
        """, "belt_l", 0, 1, "front"),),
    "L": (placed("""
        BB
        FE
        BB
        """, "belt_l", 0, 1, "front"),),
    "B": (),
})
# ---- 飞刀囊：斜挎在腰侧，露三把刀柄 ----
CARRY["knife_pouch"] = Carry("leather_c", "steel_sheath", {
    "F": (placed("""
        E.E
        BBC
        BCC
        """, "belt_r", -1, -1, "front"),),
    "L": (placed("""
        E.E
        BBC
        BCC
        """, "belt_r", -1, -1, "front"),),
    "B": (placed("""
        E.E
        BBC
        BCC
        """, "belt_l", -1, -1, "front"),),
})
# ---- 包袱：斜背在背上 ----
CARRY["bundle"] = Carry("bundle", "bundle", {
    "F": (placed("""
        C.........
        .C........
        ..C.......
        ...C......
        """, "head", 2, 10, "front"),),
    "L": (placed("""
        .ABB.
        ABBBC
        BBBCC
        .CCC.
        """, "head", 9, 9, "front"),),
    "B": (placed("""
        .ABBBB.
        ABBBBBC
        BBBDBCC
        BBBBBCC
        .CCCCC.
        """, "head", 3, 10, "front"),),
})
# ---- 金项圈 / 金链 ----
CARRY["gold_chain"] = Carry("gold_c", "gold_c", {
    "F": (placed("""
        A....B
        .BBBC.
        """, "head", 3, 10, "front"),),
    "L": (placed("""
        AB.
        .BC
        """, "head", 2, 10, "front"),),
    "B": (),
})
# ---- 药箱：斜挎，挂在身侧 ----
CARRY["medbox"] = Carry("darkwood", "gold_c", {
    "F": (placed("""
        C........
        .C.......
        ..C......
        ...C.....
        ....C....
        """, "head", 3, 10, "front"),
          placed("""
        ABB
        BFC
        CCD
        """, "belt_r", 0, 0, "front")),
    "L": (placed("""
        ABB
        BFC
        CCD
        """, "belt_r", 0, 0, "behind"),),
    "B": (placed("""
        ....C
        ...C.
        ..C..
        .C...
        C....
        """, "head", 5, 10, "front"),
          placed("""
        ABB
        BFC
        CCD
        """, "belt_l", -2, 0, "front")),
})
# ---- 灯笼：巡夜的庄丁提在手上 ----
CARRY["lantern"] = Carry("lantern", "darkwood", {
    "F": (placed("""
        .F.
        ABB
        BZC
        BBC
        .F.
        """, "hand1", -1, 0, "front"),),
    "L": (placed("""
        .F.
        ABB
        BZC
        BBC
        .F.
        """, "hand1", -1, 0, "front"),),
    "B": (),
})


# ===========================================================================
# 战斗兵刃
# ===========================================================================

WEAPON_STATES = ("stance", "ready", "attack", "cast", "hurt", "down", "victory", "guard")


@dataclass(frozen=True)
class Held:
    rows: tuple[str, ...]
    z: str = "front"


def held(text: str, z: str = "front") -> Held:
    return Held(tuple(tpl(text)), z)


@dataclass(frozen=True)
class Weapon:
    main: str
    sub: str
    states: dict[str, Held]


WEAPONS: dict[str, Weapon] = {}

# 剑：细长，一像素的刃，金色护手，柄尾一缕剑穗
WEAPONS["sword"] = Weapon("steel", "hilt", {
    "stance": held("""
        A.....
        .A....
        ..B...
        ...B..
        ....Z.
        .....+
        ......r
        """),
    "ready": held("""
        .....A
        ....A.
        ...B..
        ..B...
        .Z....
        +.....
        """),
    "attack": held("""
        AABBBBZ+Fr
        """),
    "hurt": held("""
        A
        A
        B
        Z
        +
        """, z="behind"),
    "down": held("""
        ..+...
        ......
        AABBZF
        """, z="behind"),
    "victory": held("""
        +.
        Z.
        .C
        .C
        .D
        """, z="behind"),
    "guard": held("""
        A
        A
        B
        B
        B
        Z
        +
        F
        """),
})
# 刀：宽刃单面开锋，刀背暗、刃口亮，柄尾扎红
WEAPONS["dao"] = Weapon("steel", "hilt", {
    "stance": held("""
        AA.....
        ABB....
        .ABB...
        ..ABC..
        ...ZZ..
        .....+.
        ......r
        """),
    "ready": held("""
        ....AA
        ...ABB
        ..ABC.
        .ZZ...
        +.....
        """),
    "attack": held("""
        .AAAAB....
        ABBBBBZ+Fr
        """),
    "hurt": held("""
        AA
        AB
        BC
        ZZ
        +.
        """, z="behind"),
    "down": held("""
        ..+....
        .......
        AABBBZF
        .CCC...
        """, z="behind"),
    "victory": held("""
        +..
        ZZ.
        .BA
        .BA
        .CB
        """, z="behind"),
    "guard": held("""
        AA
        BA
        BA
        BA
        CB
        ZZ
        +.
        F.
        """),
})
# 飞刀：手里扣着一把短刃
WEAPONS["knife"] = Weapon("steel", "hilt", {
    "stance": held("""
        A..
        .B.
        ..+
        """),
    "ready": held("""
        ..A
        .B.
        +..
        """),
    "attack": held("""
        AB+
        """),
    "hurt": held("""
        A
        B
        +
        """, z="behind"),
    "down": held("""
        +.
        ..
        AB
        """, z="behind"),
    "victory": held("""
        +
        """),
    "guard": held("""
        A
        B
        +
        """),
})
# 棍：一根齐眉棍，斜握
WEAPONS["staff"] = Weapon("wood", "wood", {
    "stance": held("""
        A........
        .B.......
        ..B......
        ...B.....
        ....B....
        .....+...
        ......C..
        .......C.
        ........C
        """),
    "ready": held("""
        .....A
        ....B.
        ...B..
        ..B...
        .+....
        C.....
        """),
    "attack": held("""
        ABBBBBB+CCC
        """),
    "hurt": held("""
        A
        B
        B
        +
        C
        C
        """, z="behind"),
    "down": held("""
        ..+.....
        ........
        ABBBBBCC
        """, z="behind"),
    "victory": held("""
        A
        B
        B
        +
        C
        C
        C
        """, z="behind"),
    "guard": held("""
        A
        B
        B
        B
        +
        C
        C
        C
        """),
})
# 朴刀：齐眉长的木杆，头上一口宽刃——剪径贼、庄丁一类使的长家伙
WEAPONS["podao"] = Weapon("steel", "hilt", {
    "stance": held("""
        AA.......
        ABB......
        .BBF.....
        ...F.....
        ....F....
        .....+...
        ......G..
        .......G.
        ........G
        """),
    "ready": held("""
        ....AA
        ...ABB
        ...F..
        ..F...
        .+....
        G.....
        """),
    "attack": held("""
        AAB.......
        ABBFFFF+GG
        """),
    "hurt": held("""
        AA
        AB
        F.
        +.
        G.
        G.
        """, z="behind"),
    "down": held("""
        ..+.....
        ........
        AABFFFGG
        """, z="behind"),
    "victory": held("""
        AA
        AB
        F.
        F.
        +.
        G.
        G.
        """, z="behind"),
    "guard": held("""
        AA
        AB
        BF
        .F
        .+
        .G
        .G
        """),
})
# 扇：合着的折扇当短兵用
WEAPONS["fan"] = Weapon("paper_fan", "darkwood", {
    "stance": held("""
        A.
        .B
        ..+
        """),
    "ready": held("""
        ..A
        .B.
        +..
        """, z="behind"),
    "attack": held("""
        ABB+
        """),
    "hurt": held("""
        A
        B
        +
        """),
    "down": held("""
        +
        """),
    "victory": held("""
        +
        """),
    "guard": held("""
        A
        B
        +
        """),
})

# 每把兵刃的姿态名都得在 WEAPON_STATES 里：身体模板按这些名字要兵刃，拼错一个就是那一帧空着手
for _wid, _w in WEAPONS.items():
    _extra = set(_w.states) - set(WEAPON_STATES)
    assert not _extra, f"兵刃 {_wid} 有未知姿态 {sorted(_extra)}"
