"""院墙、城墙、隔墙、殿墙、室内墙、竹篱墙。

判据仍是南邻：
  * 南邻不是墙 → 立面（墙面朝着镜头）。这一列若只有这一格厚，立面顶上压一道墙帽
    （一格厚的院墙＝墙顶压瓦 + 墙面）；上面还有墙格，墙帽画在上面那一格里。
  * 南邻也是墙 → 墙顶（从上往下看）。走向按这一格所在的横竖连续段谁长：横着的墙帽
    脊线横走，竖着的墙帽脊线竖走，拐角与丁字口画一块压顶。
  * 室内（outdoor=false）：墙顶是暗部（屋里看不见的梁上），贴着地面的一侧留一道木作
    或石作的边；立面是粉墙 + 木护墙板（木作屋）或整块条石（石室）。
"""

from __future__ import annotations

import numpy as np

import mapfacade as fa
import mapmat as mm
import pix
from mapinfer import Structure, runs

T = 16

# 各做法：墙帽用哪种瓦、墙面用什么、台基什么色
STYLES = {
    "earth": dict(cap="thatch", face="earthwall", face_lv=3, base="rock_cliff", layers=True),
    "brick": dict(cap="grey", face="brick", face_lv=3, base="rock_cliff", bricks=True),
    "sect": dict(cap="jade", face="plaster", face_lv=4, base="lacquer"),
    "manor": dict(cap="black", face="plaster", face_lv=4, base="rock_valley"),
    "fortress": dict(cap="grey", face="rock_cliff", face_lv=3, base="rock_cliff", blocks=True),
    "indoor_wood": dict(cap="void", face="plaster", face_lv=3, base="wood", wainscot=True),
    "indoor_stone": dict(cap="void", face="rock_cliff", face_lv=3, base="rock_cliff", blocks=True),
    "hall_sect": dict(cap="jade", face="plaster", face_lv=4, base="rock_cliff", hall="sect"),
    "hall_town": dict(cap="grey", face="plaster", face_lv=3, base="rock_cliff", hall="town"),
    "hall_valley": dict(cap="grey", face="plaster", face_lv=3, base="rock_valley", hall="valley"),
    "hall_village": dict(cap="thatch", face="earthwall", face_lv=3, base="rock_cliff", hall="village", layers=True),
    "hall_manor": dict(cap="black", face="plaster", face_lv=4, base="rock_valley", hall="manor"),
    "hall_stone": dict(cap="grey", face="rock_cliff", face_lv=3, base="rock_cliff", hall="stone", blocks=True),
}


def style_of(name: str) -> dict:
    return STYLES[name]


def cap_rid(cap: str) -> int:
    return mm.void() if cap == "void" else mm.roof(cap)


# ---------------------------------------------------------------------------
# 墙面
# ---------------------------------------------------------------------------
def wall_face(ctx, x: int, y: int, style_name: str, coping: bool, top_shadow: bool = True,
              base_row: bool = True) -> None:
    """画一格墙立面。coping：这一格顶上要不要自带墙帽（一格厚的墙，或两格高立面的上一格）；
    base_row：这一格是不是立面的最下一格（台基 / 护墙板画在这一格）。"""
    cv = ctx.below
    s = STYLES[style_name]
    px, py = x * T, y * T
    face = fa.rid(s["face"]) if s["face"] not in ("brick",) else mm.brick()
    fl = s["face_lv"]
    cv.rect(px, py, T, T, face, fl)
    y_face0 = py
    if coping:
        crid = cap_rid(s["cap"])
        if s["cap"] == "void":
            cv.rect(px, py, T, 3, crid, 0)
            cv.rect(px, py + 3, T, 1, fa.rid(s["base"]) if s["base"] != "lacquer" else mm.darkwood(), 3)
            y_face0 = py + 4
        elif s["cap"] == "thatch":
            cv.rect(px, py, T, 4, crid, 3)
            cv.rect(px, py, T, 1, crid, 1)
            for i in range(T):
                if pix.hash01(px + i, py, 7) < 0.5:
                    cv.px(px + i, py + 4, crid, 2)
            y_face0 = py + 4
        else:
            cv.rect(px, py, T, 1, crid, 0)
            cv.rect(px, py + 1, T, 1, crid, 4)
            for i in range(T):
                xm = (px + i) % 4
                cv.px(px + i, py + 2, crid, 3 if xm in (1, 2) else 1)
                cv.px(px + i, py + 3, crid, 4 if xm in (1, 2) else 1)
            cv.rect(px, py + 4, T, 1, crid, 0)
            y_face0 = py + 5
    # 墙面纹理
    if s.get("bricks"):
        for yy in range(y_face0, py + T):
            row = (yy // 4)
            if yy % 4 == 0:
                cv.rect(px, yy, T, 1, face, fl - 2)
            else:
                off = 0 if row % 2 else 4
                for xx in range(px, px + T):
                    if (xx + off) % 8 == 0:
                        cv.px(xx, yy, face, fl - 2)
    elif s.get("blocks"):
        for yy in range(y_face0, py + T):
            row = yy // 6
            if yy % 6 == 0:
                cv.rect(px, yy, T, 1, face, fl - 2)
            else:
                off = 0 if row % 2 else 6
                for xx in range(px, px + T):
                    if (xx + off) % 12 == 0:
                        cv.px(xx, yy, face, fl - 2)
                    elif (xx + off) % 12 == 1 and yy % 6 == 1:
                        cv.px(xx, yy, face, fl + 1)
    elif s.get("layers"):
        for yy in range(y_face0 + 3, py + T, 4):
            cv.rect(px, yy, T, 1, face, fl - 1)
    elif base_row and not s.get("wainscot"):
        fa.damp(cv, px, py + T - 3, T, pix.salt_of(ctx.sc.md.map_id, "damp"))
    if not base_row:
        pass
    elif s.get("wainscot"):
        wd = mm.wood()
        cv.rect(px, py + 9, T, 7, wd, 2)
        cv.rect(px, py + 9, T, 1, wd, 3)
        for xx in range(px + 3, px + T, 5):
            cv.rect(xx, py + 10, 1, 5, wd, 1)
        cv.rect(px, py + 15, T, 1, wd, 0)
    else:
        base = fa.rid(s["base"])
        fa.paint_plinth(cv, px, py + T - 3, T, 3, base)
    if top_shadow and not coping:
        cv.shade(px, py, -2.0, np.ones((1, T), bool))
        cv.shade(px, py + 1, -1.0, np.ones((1, T), bool))
    elif coping:
        cv.shade(px, y_face0, -1.0, np.ones((1, T), bool))
    ctx.solid_px[py:py + T, px:px + T] = True
    ctx.tall_px[py:py + T, px:px + T] = True


# ---------------------------------------------------------------------------
# 墙顶
# ---------------------------------------------------------------------------
def wall_top(ctx, x: int, y: int, style_name: str, orient: str, st_mask: np.ndarray) -> None:
    cv = ctx.below
    s = STYLES[style_name]
    px, py = x * T, y * T
    crid = cap_rid(s["cap"])
    ctx.solid_px[py:py + T, px:px + T] = True
    ctx.tall_px[py:py + T, px:px + T] = True
    if s["cap"] == "void":
        void_top(ctx, x, y, s, st_mask)
        return
    if s["cap"] == "thatch":
        cv.rect(px, py, T, T, crid, 3)
        for i in range(T):
            for j in range(0, T, 3):
                if pix.hash01(px + i, py + j, 3) < 0.3:
                    cv.px(px + i, py + j, crid, 2)
        if orient == "v":
            cv.rect(px, py, 1, T, crid, 1)
            cv.rect(px + T - 1, py, 1, T, crid, 1)
            cv.rect(px + 7, py, 2, T, crid, 4)
        else:
            cv.rect(px, py, T, 1, crid, 1)
            cv.rect(px, py + 6, T, 2, crid, 4)
            cv.rect(px, py + T - 1, T, 1, crid, 1)
        return
    if orient == "v":
        cv.rect(px, py, T, T, crid, 2)
        for j in range(T):
            yy = py + j
            joint = (yy % 4) == 0
            cv.rect(px + 1, yy, 6, 1, crid, 2 if joint else 3)
            cv.rect(px + 9, yy, 6, 1, crid, 1 if joint else 2)
        cv.rect(px + 7, py, 1, T, crid, 4)
        cv.rect(px + 8, py, 1, T, crid, 1)
        cv.rect(px, py, 1, T, crid, 0)
        cv.rect(px + T - 1, py, 1, T, crid, 0)
    elif orient == "h":
        cv.rect(px, py, T, 1, crid, 0)
        cv.rect(px, py + 1, T, 4, crid, 3)
        cv.rect(px, py + 5, T, 1, crid, 4)
        cv.rect(px, py + 6, T, 1, crid, 1)
        for j in range(7, 13):
            for i in range(T):
                xm = (px + i) % 4
                cv.px(px + i, py + j, crid, [1, 4, 3, 2][xm] - (1 if (py + j) % 5 == 0 and xm else 0))
        for i in range(T):
            xm = (px + i) % 4
            cv.px(px + i, py + 13, crid, 4 if xm in (1, 2) else 1)
            cv.px(px + i, py + 14, crid, 3 if xm in (1, 2) else 0)
        cv.rect(px, py + 15, T, 1, crid, 0)
    else:
        # 拐角 / 丁字口：一块方压顶，四面一线暗，中间一点亮
        cv.rect(px, py, T, T, crid, 2)
        cv.rect(px + 1, py + 1, T - 2, T - 2, crid, 3)
        cv.rect(px + 2, py + 2, T - 4, T - 4, crid, 2)
        cv.rect(px + 1, py + 1, T - 2, 1, crid, 4)
        cv.rect(px + 1, py + 1, 1, T - 2, crid, 4)
        cv.rect(px + 7, py + 7, 2, 2, crid, 4)
        cv.rect(px, py, T, 1, crid, 0)
        cv.rect(px, py + T - 1, T, 1, crid, 0)
        cv.rect(px, py, 1, T, crid, 0)
        cv.rect(px + T - 1, py, 1, T, crid, 0)


def void_top(ctx, x: int, y: int, s: dict, st_mask: np.ndarray) -> None:
    """室内墙顶：暗部。挨着可走格的那几条边留一道 2px 的木作 / 石作边沿。"""
    sc = ctx.sc
    cv = ctx.below
    px, py = x * T, y * T
    v = mm.void()
    cv.rect(px, py, T, T, v, 1)
    speck = pix.hash01(np.arange(T)[None, :] + px, np.arange(T)[:, None] + py, 11) < 0.06
    cv.shade(px, py, 1.0, speck)
    rim = fa.rid(s["base"]) if s["base"] != "lacquer" else mm.darkwood()
    if s.get("wainscot"):
        rim = mm.darkwood()

    def open_(xx, yy):
        return 0 <= xx < sc.W and 0 <= yy < sc.H and not st_mask[yy, xx] and not sc.wall[yy, xx]

    if open_(x, y - 1):
        cv.rect(px, py, T, 2, rim, 2)
        cv.rect(px, py, T, 1, rim, 3)
    if open_(x - 1, y):
        cv.rect(px, py, 2, T, rim, 3)
    if open_(x + 1, y):
        cv.rect(px + T - 2, py, 2, T, rim, 1)
    # 下面一格是立面：这一格的下沿是墙头，亮一线（图外当作还是墙）
    if y + 2 < sc.H and st_mask[y + 1, x] and not st_mask[y + 2, x]:
        cv.rect(px, py + T - 3, T, 3, rim, 2)
        cv.rect(px, py + T - 3, T, 1, rim, 3)


# ---------------------------------------------------------------------------
# 竹篱墙（墨府后园）
# ---------------------------------------------------------------------------
def bamboo_cell(ctx, x: int, y: int, face: bool) -> None:
    cv = ctx.below
    px, py = x * T, y * T
    b = mm.bamboo()
    leaf = mm.foliage("bamboo")
    cv.rect(px, py, T, T, leaf, 1)
    for i, cx in enumerate(range(px + 1, px + T, 4)):
        cv.rect(cx, py, 2, T, b, 3)
        cv.rect(cx, py, 1, T, b, 4)
        node = (py + i * 3) % 6
        for yy in range(py + node, py + T, 6):
            cv.rect(cx, yy, 2, 1, b, 1)
    # 竹叶：一簇簇斜挑的叶子压在竹竿上
    g = np.random.default_rng(pix.seed_of(ctx.sc.md.map_id, "bamboo", x, y))
    for _ in range(9 if not face else 6):
        lx = px + int(g.integers(0, T))
        ly = py + int(g.integers(0, T if not face else 9))
        for k in range(3):
            cv.px(lx + k, ly - k // 2, leaf, 3 + (k == 0))
        cv.px(lx + 1, ly + 1, leaf, 1)
    if face:
        cv.shade(px, py + T - 3, -1.0, np.ones((3, T), bool))
    ctx.solid_px[py:py + T, px:px + T] = True
    ctx.tall_px[py:py + T, px:px + T] = True


# ---------------------------------------------------------------------------
# 一整段墙
# ---------------------------------------------------------------------------
def paint_wall(ctx, st: Structure) -> None:
    sc = ctx.sc
    m = st.mask
    hr, vr = runs(m)
    rooms = sc.rooms
    indoor = STYLES.get(st.style, {}).get("cap") == "void"
    for y, x in zip(*np.nonzero(m)):
        south_open = y + 1 < sc.H and not sc.wall[y + 1, x]
        up_in = y > 0 and m[y - 1, x]
        if st.style == "bamboo":
            bamboo_cell(ctx, x, y, south_open)
            continue
        # 屋里的厚墙：立面取两格高（一格 16px 的墙比人还矮，屋子像地窖）
        upper_face = (indoor and not south_open and y + 2 < sc.H and m[y + 1, x]
                      and not sc.wall[y + 2, x] and vr[y, x] >= 3)
        if upper_face:
            wall_face(ctx, x, y, st.style, coping=True, base_row=False)
            continue
        if south_open:
            tall_face = indoor and up_in and vr[y, x] >= 3
            wall_face(ctx, x, y, st.style, coping=not up_in, top_shadow=not tall_face)
            if STYLES[st.style].get("hall"):
                hall_face_details(ctx, x, y, st, up_in)
        else:
            if hr[y, x] >= 3 and vr[y, x] >= 3:
                orient = "c"
            elif vr[y, x] > hr[y, x]:
                orient = "v"
            elif hr[y, x] > vr[y, x]:
                orient = "h"
            else:
                orient = "c"
            wall_top(ctx, x, y, st.style, orient, m)


def hall_face_details(ctx, x: int, y: int, st: Structure, up_in: bool) -> None:
    """殿墙的立面：外墙每两格一根柱、柱间开窗；朝屋里的那一面（北墙内侧）挂一幅匾。"""
    sc = ctx.sc
    cv = ctx.below
    s = STYLES[st.style]
    fs = fa.facade_style(s["hall"])
    post = fa.rid(fs["post"])
    px, py = x * T, y * T
    inside_below = y + 1 < sc.H and sc.rooms[y + 1, x]
    if (x % 2) == 0:
        cv.rect(px, py + 2, 2, T - 5, post, fs["post_lv"])
        cv.rect(px, py + 2, 1, T - 5, post, fs["post_lv"] + 1)
    if inside_below:
        # 北墙内侧：屋里的后墙，挂匾、不开窗
        if (x % 3) == 1:
            fa.paint_plaque(cv, px + 3, py + 4, 10)
        return
    if (x % 2) == 1 and not up_in:
        fa.paint_window(cv, px + 4, py + 6, 8, 6, fa.rid(fs["win_frame"]))
