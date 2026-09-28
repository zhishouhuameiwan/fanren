"""曲魂专用的身体模板：24 宽的连帽斗篷巨汉。

为什么单独画、不走通用的体型手术：
  通用做法是把 16×24 的模板插行插列撑成 24×32。撑出来的斗篷是一整块灰——
  终审（docs/octopath-review.md LOW-5）说它在战斗画面里「像一块墓碑」。
  斗篷要读成「里面有个人」，靠的是褶子的走向、两肩的受光、手臂在斗篷底下顶出来的
  轮廓、从斗篷边上露出来的拳头——这些都得按 24 宽一笔一笔画，插列插不出来。

坐标：每张 24 宽 × 22 高，盖在 24×32 帧的第 9–30 行（第 31 行留给脚底描边）；
头（低兜帽）照旧由头部图层画在 (6, 1)。码表与 sprites_body 相同：
  A B C D 斗篷（上身材质）受光/底色/暗面/深影；N O P R 斗篷里、斗篷下的黑袍；
  Q S s 皮肤（死人皮色的大拳头）；M m 鞋；'1' 近手握点（出招的拳头）。
斗篷的褶：竖着走的 C 线是褶沟，左边一格 A/B 是褶脊的受光面（光从左上来）。
"""

from __future__ import annotations

from sprites_body import BodyFrame
from sprites_core import tpl

GIANT_W, GIANT_ROWS = 24, 22


def G(text: str, **kw) -> BodyFrame:
    rows = tpl(text)
    assert len(rows) == GIANT_ROWS, f"曲魂模板须 {GIANT_ROWS} 行，实得 {len(rows)}"
    for r in rows:
        assert len(r) == GIANT_W, f"曲魂模板须 {GIANT_W} 列：{r!r}"
    return BodyFrame(rows=tuple(rows), **kw)


def _step(frame: BodyFrame, feet: str, hem: str | None = None) -> BodyFrame:
    """行走迈步帧：整个身子下沉一行（删掉袍子中段一行、顶上补一行空），换掉脚那一行。"""
    rows = list(frame.rows)
    del rows[18]
    rows = ["." * GIANT_W] + rows
    rows[-1] = feet.ljust(GIANT_W, ".")
    if hem is not None:
        rows[-2] = hem.ljust(GIANT_W, ".")
    return BodyFrame(rows=tuple(rows), head=frame.head, head_dx=frame.head_dx, head_dy=frame.head_dy + 1,
                     weapon=frame.weapon, torso_row=frame.torso_row, leg_row=frame.leg_row, mid_col=frame.mid_col)


CLOAK_GIANT: dict[str, BodyFrame] = {}

# ---- 正面：斗篷从兜帽底下披到两肩，前襟分开露出黑袍，两只拳头从斗篷两侧垂下来 ----
CLOAK_GIANT["F0"] = G("""
........................
........................
.....AAABBBBBBBBBBCC....
...AAABBBBBBBBBBBBBBCC..
..AABBDABBBOOBBBDBBBBCD.
..ABBDABBBBOOBBBBDBBBCD.
.AABDABBBBOOOOBBBBDBBCCD
.ABBDABBBBOOOOBBBBDBBCCD
.ABDABBBBBOOOOBBBBBDBCCD
.ABDABBBBOOOOOOBBBBDBCCD
.ABDABBBBOOOOOOBBBBBDCCD
.AADABBBBOOOOOOBBBBBDCCD
.AADABBBBOOOOOOBBBBBDCD.
.QQSDABBBOOOOOOBBBDCSsQ.
.QSsDDADBOOOOOOBDBDCSss.
..ss.DDDDOOOOOODDDD.ss..
.....NOOOOOOOOOOOOOPR...
.....NOOOOOOOOOOOOOPR...
.....NOOOOOOOOOOOOPPR...
.....NOOOOOOOOOOOOPPR...
.....NOOOOOOOOOOOOPPR...
.....MMMMm.....MMMMm....
""", head="F")
CLOAK_GIANT["F1"] = _step(CLOAK_GIANT["F0"], "....MMMMMm..............", ".....NOOOOOOOOOOOOPPR...")
CLOAK_GIANT["F2"] = _step(CLOAK_GIANT["F0"], "..............MMMMMm....", ".....NOOOOOOOOOOOOPPR...")

# ---- 侧面（朝左）：斗篷后摆比前襟长，前襟露一线黑袍，近手的拳头在斗篷前沿露出来 ----
CLOAK_GIANT["L0"] = G("""
........................
........................
.......AABBBBBBBCC......
.....AAABBBBBBBBBBCC....
....AABBBBBBBBBBBBBBCD..
...AABBDABBBBDBBBBBCCD..
...TABBDABBBBBDBBBBCCD..
...TABDABBBBBBBDBBBCCD..
...TABDABBBBBBBBDBBCCCD.
...TADABBBBBBBBBDBBCCCD.
...TADABBBBBBBBBBDBCCCD.
..QQDABBBBBBBBBBBDBCCCD.
.QQSSDBBBBBBBBBBBBDCCD..
.QSSsDBDBDBBBBBBDBDCCD..
.SsssTDADADBDBDBDBDDD...
..ss.NOOOOOOOOOOOOOPR...
...NOOOOOOOOOOOOOOPR....
...NOOOOOOOOOOOOOPPR....
....NOOOOOOOOOOOOPPR....
....NOOOOOOOOOOOOPPR....
....NOOOOOOOOOOOPPR.....
...MMMMm.....mmmm.......
""", head="L", mid_col=11)
CLOAK_GIANT["L1"] = _step(CLOAK_GIANT["L0"], ".MMMMm..........mmmm....", "..NOOOO.....OOOOOPPR....")
CLOAK_GIANT["L2"] = _step(CLOAK_GIANT["L0"], "....MMMMm..mmmm.........", "...NOOOOO..OOOOPPR......")

# ---- 战斗（侧身朝左） ----
# 待机：重心压低，近手的拳头提在胸前（粗大、死人皮色），斗篷前襟被胳膊顶开
CLOAK_GIANT["stance"] = G("""
........................
........................
........AABBBBBBCC......
......AAABBBBBBBBBCC....
.....AABBBBBBBBBBBBBCD..
....AABBDABBBBDBBBBCCD..
...AABBDABBBBBDBBBBCCDD.
..QQAADABBBBBBBDBBBCCCD.
.QQSSDABBBBBBBBDBBBCCCD.
.QSSsDBBBBBBBBBBDBBCCCD.
.SsssTABBBBBBBBBDBBCCCD.
..ss.TABBBBBBBBBBDBCCCD.
.....TABBBDBBBBBBDBCCD..
.....TABBDABBBBBBBDCCD..
.....TDADADBDBDBDBDDD...
....NOOOOOOOOOOOOOOPR...
...NOOOOOOOOOOOOOOOPPR..
..NOOOOOO....OOOOOOPPR..
..NOOOOO......OOOOOPPR..
.NOOOOO........OOOOPPR..
.NOOOO..........OOOPPR..
MMMMMm..........mmmmmm..
""", head="L", head_dx=1, head_dy=1, weapon="stance", torso_row=9, leg_row=17, mid_col=11)
# 出招：往前一扑，近手整条胳膊捣出去，拳头顶到帧边；斗篷袖被带得往后飘
CLOAK_GIANT["attack"] = G("""
........................
........................
.........AABBBBBBCC.....
.......AAABBBBBBBBBCC...
QQ...AAABBBBBBBBBBBBBCD.
QQSSAAAAAAABBBDBBBBBBCD.
QSSSBBBBBBBBBBBDBBBBBCCD
SSssDDDDDDBBBBBBDBBBBCCD
.sss.....TABBBBBDBBBBCCD
.........TABBBBBBDBBBCCD
.........TABBBBBBDBBBCCD
........TABBBBBBBBDBBCCD
........TABBDBBBBBDBBCD.
.......TDADADBDBDBDBDDD.
......NOOOOOOOOOOOOOOPR.
.....NOOOOOOOOOOOOOOOPR.
....NOOOOOOO....OOOOOPPR
...NOOOOOO.......OOOOPPR
..NOOOOO.........OOOOPPR
.NOOOO............OOOPPR
NOOO...............OOPR.
MMMm..............mmmmm.
""", head="L", head_dx=-2, head_dy=2, weapon="attack", torso_row=9, leg_row=16, mid_col=11)
# 受击：上身往后仰，兜帽跟着一甩，拳头松开往后甩
CLOAK_GIANT["hurt"] = G("""
........................
........................
........................
...........AABBBBBBCC...
.........AAABBBBBBBBBCC.
........AABBBBBBDBBBBBCD
.......AABBDABBBBDBBBBCD
......TABBDABBBBBBDBBBQQ
......TABBDABBBBBBBDBQSS
......TABBBDABBBBBBBDSSs
......TABBBBDABBBBBBBDss
......TABBBBBDBBBBBBBCD.
.....TABBBBBBDBBBBBBCD..
.....TDADADBDBDBDBDDD...
....NOOOOOOOOOOOOOOPR...
...NOOOOOOOOOOOOOOOPR...
..NNOOOOO....OOOOOOPR...
..NOOOOO.......OOOOPR...
...NOOO........OOOPPR...
...NOOO........OOOPPR...
....NO.........OOPPR....
...MMm.........mmmmm....
""", head="L", head_dx=4, head_dy=3, weapon="hurt", torso_row=9, leg_row=17, mid_col=12)
# 倒地：单膝跪倒，兜帽垂到胸前，一只拳头撑在地上
CLOAK_GIANT["down"] = G("""
........................
........................
........................
........................
........................
........................
........AABBBBBBCC......
......AAABBBBBBBBBCC....
.....AABBBBBBBBBBBBBCD..
....AABBDABBBBDBBBBBCD..
...AABBDABBBBBBDBBBBCCD.
..AABBDABBBBBBBBDBBBCCD.
..ABBDABBBBBBBBBBDBBCCD.
..ABDABBBBBBBBBBBBDBCCD.
..TDADADBDBDBDBDBDDDDD..
.QQ.NOOOOOOOOOOOOOOPR...
QQSSNOOOOOOOOOOOOOOPPR..
QSSsNOOOOOOOOOOOOOOPPR..
SsssNOOOOOOOOOOOOOPPPR..
.ss.NOOOOOOOOOOOOOOPPR..
...MMOOOOOOOOOOOOOOPPRm.
...MMMMMMMMMMMMMMMMMMm..
""", head="L", head_dx=-1, head_dy=7, weapon="down", torso_row=11, leg_row=18, mid_col=11)
# 胜利：直起身，两只拳头攥着垂在身侧，斗篷从肩上直直地垂下来
CLOAK_GIANT["victory"] = G("""
........................
........................
.......AABBBBBBBCC......
.....AAABBBBBBBBBBCC....
....AABBBBBBBBBBBBBBCD..
...AABBDABBBBDBBBBBCCD..
...TABBDABBBBBDBBBBCCD..
...TABDABBBBBBBDBBBCCD..
...TABDABBBBBBBBDBBCCCD.
..QQADABBBBBBBBBDBBCCCD.
.QQSSDBBBBBBBBBBBDBCCCD.
.QSSsDBBBBBBBBBBBDBCCCD.
.SsssTBBBBBBBBBBBBDCCD..
..ss.TDADADBDBDBDBDDD...
...NOOOOOOOOOOOOOOPR....
...NOOOOOOOOOOOOOOPR....
...NOOOOOOOOOOOOOPPR....
....NOOOOOOOOOOOOPPR....
....NOOOOOOOOOOOOPPR....
....NOOOOOOOOOOOOPPR....
....NOOOOOOOOOOOPPR.....
...MMMMm.....mmmm.......
""", head="L", weapon="victory", torso_row=9, leg_row=17, mid_col=11)


def _variant(base: BodyFrame, **kw) -> BodyFrame:
    d = dict(head=base.head, head_dx=base.head_dx, head_dy=base.head_dy, weapon=base.weapon,
             torso_row=base.torso_row, leg_row=base.leg_row, mid_col=base.mid_col)
    d.update(kw)
    return BodyFrame(rows=base.rows, **d)


# 蓄势、施法、防御：曲魂不会法术、也没有兵刃，三帧借待机的身形——蓄势压低一行，施法/防御原样。
# 真正要读出来的是气焰（蓄劲金焰）与待机/出招的对比，这三帧不另起剪影。
CLOAK_GIANT["ready"] = BodyFrame(rows=tuple(["." * GIANT_W] + list(CLOAK_GIANT["stance"].rows[:17])
                                            + list(CLOAK_GIANT["stance"].rows[18:])),
                                 head="L", head_dx=1, head_dy=2, weapon="ready", torso_row=9, leg_row=17, mid_col=11)
CLOAK_GIANT["cast"] = _variant(CLOAK_GIANT["stance"], weapon="cast")
CLOAK_GIANT["guard"] = _variant(CLOAK_GIANT["stance"], weapon="guard")
