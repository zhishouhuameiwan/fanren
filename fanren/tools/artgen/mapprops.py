"""gid 8 的东西（篱笆、石栏、摊架、兵器架、家具……）与 maps.json 指认的道具、贴花。

每件道具画在自己的格子里（below.png）；比一格高的（石灯、兵器架、药柜、柱子、亭顶）
超出格子上沿的那一截另画一份进 above.png——走到它北边的人，脚会被它的顶挡住，
这是斜俯视里「东西比人高」该有的样子。

它们全是实心挡路的，所以都画得「立得起来」：有顶面、有朝南的正面、脚下有接触阴影；
与地面装饰（gid 6/7，贴地、矮小）一眼分得开。
"""

from __future__ import annotations

import numpy as np

import mapfacade as fa
import mapmat as mm
import pix

T = 16


# ---------------------------------------------------------------------------
# 斜俯视小几何
# ---------------------------------------------------------------------------
def box(cv, x: int, y: int, w: int, h: int, rid: int, top_lv: int, front_h: int, front_lv: int | None = None) -> None:
    """一块「有顶面有正面」的方块：上 h-front_h 行是顶面，下 front_h 行是朝南的正面。"""
    if front_lv is None:
        front_lv = max(0, top_lv - 1)
    cv.rect(x, y, w, h - front_h, rid, top_lv)
    cv.rect(x, y, w, 1, rid, min(top_lv + 1, 5))
    cv.rect(x, y + h - front_h, w, front_h, rid, front_lv)
    cv.rect(x, y + h - front_h, w, 1, rid, max(0, front_lv - 1))
    cv.rect(x, y, 1, h, rid, max(0, top_lv - 2))
    cv.rect(x + w - 1, y, 1, h, rid, max(0, front_lv - 2))
    cv.rect(x, y + h - 1, w, 1, rid, 0)


def disc_fill(cv, cx: float, cy: float, rx: float, ry: float, rid: int, base: float, spread: float = 1.2) -> None:
    n = int(max(rx, ry)) + 1
    yy, xx = np.mgrid[-n:n + 1, -n:n + 1].astype(np.float32)
    d = (xx / rx) ** 2 + (yy / ry) ** 2
    m = d <= 1.0
    lv = np.clip(np.round(base - (xx / rx + yy / ry) * spread * 0.5), 0, 5)
    edge = m & ~pix.erode(m, 1)
    lv = np.where(edge, np.maximum(lv - 2, 0), lv)
    cv.paint(int(round(cx)) - n, int(round(cy)) - n, m, rid, lv.astype(np.float32))


def both(ctx, top_y: int, fn, solid: bool = False, tall: bool = False) -> None:
    """fn(canvas) 画进 below；再画一份进 above、只留 top_y 以上那一截（高出格子的部分）。

    solid/tall：按「实际画了的像素」登记实体——投影照着东西本身的轮廓落，
    而不是一个方框（假山、石灯的影子是方的，一眼假）。
    """
    fn(ctx.below)
    tmp = pix.Canvas(ctx.above.w, ctx.above.h, opaque=False)
    fn(tmp)
    drawn = tmp.on.copy()
    if solid or tall:
        ctx.solid_px |= drawn
    if tall:
        ctx.tall_px |= drawn
    tmp.on[max(0, top_y):, :] = False
    m = tmp.on
    if m.any():
        ctx.above.rid[m] = tmp.rid[m]
        ctx.above.lv[m] = tmp.lv[m]
        ctx.above.on[m] = True


def mark(ctx, x: int, y: int, w: int, h: int, tall: bool = False) -> None:
    ctx.solid_px[max(0, y):y + h, max(0, x):x + w] = True
    if tall:
        ctx.tall_px[max(0, y):y + h, max(0, x):x + w] = True


# ---------------------------------------------------------------------------
# 线状：篱笆、栏杆、石栏、兵器架一排、书架、柜台
# ---------------------------------------------------------------------------
def conn(ctx, pid: int, x: int, y: int):
    sc = ctx.sc

    def same(xx, yy):
        return 0 <= xx < sc.W and 0 <= yy < sc.H and sc.prop_at[yy, xx] == pid

    return same(x - 1, y), same(x + 1, y), same(x, y - 1), same(x, y + 1)


def fence_cell(ctx, pid: int, x: int, y: int, kind: str) -> None:
    cv = ctx.below
    L, R, U, D = conn(ctx, pid, x, y)
    px, py = x * T, y * T
    if kind == "bamboo_fence":
        rid = mm.bamboo()
        rail_lv, post_lv = 2, 3
    elif kind == "balustrade":
        rid = mm.paving(ctx.sc.th["paving"]) if ctx.sc.theme != "cliff" else mm.rock("cliff")
        rail_lv, post_lv = 3, 3
    else:
        rid = mm.wood()
        rail_lv, post_lv = 2, 3
    horiz = (L or R) and not (U or D and not (L or R))
    vert = (U or D) and not (L or R)
    if vert:
        # 南北走向：从侧面看，一串柱头 + 中间一道栏
        cv.rect(px + 7, py, 2, T, rid, rail_lv)
        cv.rect(px + 7, py, 1, T, rid, rail_lv + 1)
        for yy in (py + 2, py + 10):
            box(cv, px + 5, yy - 3, 6, 7, rid, post_lv + 1, 3, post_lv - 1)
        mark(ctx, px + 5, py, 6, T)
        return
    if kind == "balustrade":
        cv.rect(px, py + 4, T, 3, rid, 4)
        cv.rect(px, py + 7, T, 1, rid, 1)
        for xx in range(px + 1, px + T, 4):
            cv.rect(xx, py + 8, 2, 6, rid, 3)
            cv.rect(xx + 1, py + 8, 1, 6, rid, 2)
        cv.rect(px, py + 14, T, 2, rid, 2)
        if not L or not R or (x % 3 == 0):
            box(cv, px + (0 if not L else 6), py + 1, 5, 15, rid, 4, 10, 3)
        mark(ctx, px, py + 1, T, 15)
        return
    if kind == "bamboo_fence":
        for i, xx in enumerate(range(px, px + T, 3)):
            top = py + 2 + (i % 2)
            cv.rect(xx, top, 2, T - 2 - (top - py), rid, 3)
            cv.px(xx, top, rid, 4)
            cv.rect(xx + 1, top + 1, 1, T - 3 - (top - py), rid, 2)
            for yy in range(top + 4, py + T, 5):
                cv.px(xx, yy, rid, 1)
        cv.rect(px, py + 6, T, 1, mm.wood(), 2)
        cv.rect(px, py + 11, T, 1, mm.wood(), 2)
        mark(ctx, px, py + 2, T, 14)
        return
    # railing（木栏杆）
    cv.rect(px, py + 5, T, 2, rid, 3)
    cv.rect(px, py + 5, T, 1, rid, 4)
    cv.rect(px, py + 10, T, 1, rid, 2)
    for xx in (px + 1, px + 9):
        cv.rect(xx, py + 3, 2, 12, rid, post_lv)
        cv.px(xx, py + 3, rid, 4)
        cv.rect(xx + 1, py + 4, 1, 11, rid, 1)
    mark(ctx, px, py + 3, T, 13)


def weapon_rack(ctx, x: int, y: int, w_cells: int = 1, seed: int = 0) -> None:
    """兵器架：两根立柱两道横木，架上斜靠着几杆枪、戟、刀。"""
    px, py = x * T, y * T
    wd = mm.wood()
    mt = mm.metal()
    W = w_cells * T
    g = np.random.default_rng(seed)

    def draw(cv):
        cv.rect(px + 1, py - 6, 2, 21, wd, 2)
        cv.rect(px + W - 3, py - 6, 2, 21, wd, 2)
        cv.rect(px + 1, py - 4, W - 2, 2, wd, 3)
        cv.rect(px + 1, py + 7, W - 2, 2, wd, 2)
        for xx in range(px + 4, px + W - 3, 3):
            top = py - 9 - int(g.integers(0, 3))
            cv.rect(xx, top + 3, 1, py + 14 - top - 3, wd, 3)
            cv.rect(xx, top, 1, 3, mt, 4)
            cv.px(xx + 1, top + 2, mt, 3)
            cv.px(xx - 1, top + 2, mt, 2)
        cv.rect(px, py + 14, W, 1, wd, 0)

    both(ctx, py, draw, solid=True)


def target(ctx, x: int, y: int) -> None:
    px, py = x * T, y * T

    def draw(cv):
        wd = mm.wood()
        cv.rect(px + 3, py + 6, 2, 10, wd, 2)
        cv.rect(px + 11, py + 6, 2, 10, wd, 1)
        disc_fill(cv, px + 8, py + 3, 7, 7, mm.roof("thatch"), 3)
        disc_fill(cv, px + 8, py + 3, 5, 5, mm.cloth("white"), 3, 0.6)
        disc_fill(cv, px + 8, py + 3, 3, 3, mm.lacquer(), 3, 0.6)
        cv.px(px + 8, py + 3, mm.gold(), 3)

    both(ctx, py, draw, solid=True)


def stake(ctx, x: int, y: int, stone: bool) -> None:
    """桩：石桩（落日峰习法处）或木人桩（客栈后院）。高出格子上沿半格。"""
    px, py = x * T, y * T
    rid = mm.rock("cliff") if stone else mm.wood()

    def draw(cv):
        cv.rect(px + 5, py - 6, 6, 20, rid, 3)
        cv.rect(px + 5, py - 6, 1, 20, rid, 4)
        cv.rect(px + 10, py - 6, 1, 20, rid, 1)
        cv.rect(px + 5, py - 7, 6, 2, rid, 4)
        if stone:
            for yy in range(py - 2, py + 12, 5):
                cv.rect(px + 5, yy, 6, 1, rid, 2)
        else:
            cv.rect(px + 2, py - 1, 3, 2, rid, 2)
            cv.rect(px + 11, py + 3, 3, 2, rid, 2)
            cv.rect(px + 5, py + 6, 6, 2, mm.roof("thatch"), 3)
        cv.rect(px + 4, py + 13, 8, 2, rid, 1)

    both(ctx, py, draw, tall=True)


def rock(ctx, x: int, y: int, cave: bool, seed: int) -> None:
    """一块石头：不规则的圆团，左上亮，顶上几点苔。"""
    cv = ctx.below
    rid = mm.rock("cave" if cave else "cliff")
    px, py = x * T, y * T
    g = np.random.default_rng(seed)
    disc_fill(cv, px + 8, py + 8, 7 + g.random(), 6 + g.random(), rid, 3, 1.6)
    disc_fill(cv, px + 5 + g.integers(0, 3), py + 6, 4, 3, rid, 4, 1.0)
    if not cave:
        moss = mm.grass(ctx.sc.season)
        for _ in range(3):
            cv.px(px + 4 + int(g.integers(0, 8)), py + 3 + int(g.integers(0, 3)), moss, 3)
    cv.rect(px + 2, py + 14, 12, 1, rid, 0)
    mark(ctx, px + 1, py + 2, 14, 13)


def stump(ctx, x: int, y: int) -> None:
    cv = ctx.below
    bark = mm.bark()
    wd = mm.wood()
    px, py = x * T, y * T
    cv.rect(px + 3, py + 6, 10, 8, bark, 2)
    cv.rect(px + 3, py + 6, 1, 8, bark, 3)
    cv.rect(px + 12, py + 6, 1, 8, bark, 1)
    disc_fill(cv, px + 8, py + 6, 5, 3, wd, 4, 0.4)
    cv.rect(px + 6, py + 6, 4, 1, wd, 2)
    cv.px(px + 8, py + 6, wd, 1)
    cv.rect(px + 2, py + 13, 3, 2, bark, 2)
    cv.rect(px + 11, py + 13, 3, 2, bark, 1)
    mark(ctx, px + 2, py + 3, 12, 12)


def stone_lantern(ctx, x: int, y: int) -> None:
    """石灯：台座、灯柱、灯室（一个暗口）、伞盖、宝珠。高出格子上沿约半格。"""
    px, py = x * T, y * T
    rid = mm.paving(ctx.sc.th["paving"])

    def draw(cv):
        box(cv, px + 3, py + 11, 10, 4, rid, 3, 2)
        cv.rect(px + 6, py + 3, 4, 8, rid, 3)
        cv.rect(px + 6, py + 3, 1, 8, rid, 4)
        box(cv, px + 4, py - 3, 8, 6, rid, 3, 4)
        cv.rect(px + 6, py - 1, 4, 3, mm.void(), 1)
        cv.px(px + 7, py, mm.glow(), 2)
        cv.rect(px + 2, py - 6, 12, 3, rid, 4)
        cv.rect(px + 2, py - 4, 12, 1, rid, 1)
        cv.px(px + 1, py - 4, rid, 3)
        cv.px(px + 14, py - 4, rid, 2)
        cv.rect(px + 7, py - 8, 2, 2, rid, 4)

    both(ctx, py, draw, tall=True)
    ctx.lights.append({"kind": "stone_lantern", "x": x + 0.5, "y": y + 0.1})


def pillar(ctx, x: int, y: int) -> None:
    """石柱（密室）：比人高一截，柱头压在北邻格上（above）。"""
    px, py = x * T, y * T
    rid = mm.rock("cliff") if ctx.sc.theme != "cave" else mm.rock("cave")

    def draw(cv):
        cv.rect(px + 3, py + 12, 10, 3, rid, 2)
        cv.rect(px + 4, py - 12, 8, 24, rid, 3)
        cv.rect(px + 4, py - 12, 2, 24, rid, 4)
        cv.rect(px + 10, py - 12, 2, 24, rid, 1)
        cv.rect(px + 3, py - 15, 10, 3, rid, 4)
        cv.rect(px + 3, py - 13, 10, 1, rid, 1)

    both(ctx, py, draw, tall=True)


def haystack(ctx, x: int, y: int, w: int) -> None:
    cv = ctx.below
    rid = mm.roof("thatch")
    px, py = x * T, y * T
    for i in range(w):
        disc_fill(cv, px + i * T + 8, py + 9, 9, 7, rid, 3, 1.4)
    for xx in range(px + 1, px + w * T - 1, 2):
        cv.px(xx, py + 4 + (xx % 3), rid, 4)
    cv.rect(px + 1, py + 15, w * T - 2, 1, rid, 0)
    mark(ctx, px, py + 2, w * T, 14)


# ---------------------------------------------------------------------------
# 摊位、棚
# ---------------------------------------------------------------------------
def goods(cv, x: int, y: int, w: int, seed: int) -> None:
    """摊上的货：一格一两样（果蔬、陶罐、布匹），只用色板里的颜色。"""
    g = np.random.default_rng(seed)
    choices = [mm.herb(), mm.flower("red"), mm.flower("yellow"), mm.cloth("indigo"), mm.earthwall(),
               mm.cloth("ochre")]
    for xx in range(x + 1, x + w - 2, 3):
        rid = choices[int(g.integers(0, len(choices)))]
        cv.rect(xx, y, 2, 2, rid, 3)
        cv.px(xx, y, rid, 4)
        cv.px(xx + 1, y + 1, rid, 1)


def stall(ctx, x: int, y: int, w_cells: int, seed: int) -> None:
    """摊架：一张摆满货的案子 + 一幅布棚（布棚高出格子，压在北邻上）。"""
    px, py = x * T, y * T
    W = w_cells * T
    wd = mm.wood()
    kinds = ("indigo", "ochre", "white", "red")
    cloth = mm.cloth(kinds[int(pix.hash01(x, y, seed) * len(kinds))])
    cv = ctx.below
    box(cv, px + 1, py + 6, W - 2, 9, wd, 3, 4, 2)
    goods(cv, px + 1, py + 7, W - 2, seed)
    mark(ctx, px + 1, py + 4, W - 2, 12)

    def canopy(c):
        c.rect(px + 1, py - 6, 1, 13, wd, 2)
        c.rect(px + W - 2, py - 6, 1, 13, wd, 1)
        c.rect(px - 1, py - 9, W + 2, 1, cloth, 0)
        for xx in range(px - 1, px + W + 1):
            stripe = ((xx - px) // 3) % 2
            c.rect(xx, py - 8, 1, 4, cloth, 3 if stripe else 2)
            if (xx - px) % 4 in (1, 2):
                c.px(xx, py - 4, cloth, 1 + stripe)
            c.px(xx, py - 4 + (1 if (xx - px) % 4 in (1, 2) else 0), cloth, 0)

    both(ctx, py, canopy)


def shed(ctx, p) -> None:
    """竹棚：竹柱、苇席顶（顶占上半截），棚下堆着货箱。"""
    cv = ctx.below
    px, py = p.x * T, p.y * T
    W, H = p.w * T, p.h * T
    b = mm.bamboo()
    roof = mm.roof("thatch")
    wd = mm.wood()
    roof_h = max(10, H // 2)
    for i in range(p.w):
        box(cv, px + i * T + 2, py + roof_h, 12, H - roof_h - 1, wd, 3, 5, 2)
    for xx in (px + 1, px + W - 3):
        cv.rect(xx, py + 4, 2, H - 4, b, 3)
    cv.rect(px - 1, py, W + 2, roof_h, roof, 3)
    for xx in range(px - 1, px + W + 1, 2):
        cv.rect(xx, py + 1, 1, roof_h - 2, roof, 2 + (xx // 2) % 2)
    cv.rect(px - 1, py, W + 2, 1, roof, 1)
    cv.rect(px - 1, py + roof_h - 1, W + 2, 1, roof, 0)
    cv.rect(px - 1, py + roof_h, W + 2, 1, roof, 1)
    mark(ctx, px, py, W, H, True)


# ---------------------------------------------------------------------------
# 家具
# ---------------------------------------------------------------------------
def table(ctx, p, stone: bool = False, items: str = "tea") -> None:
    cv = ctx.below
    rid = mm.rock("cliff") if stone else mm.darkwood()
    px, py = p.x * T, p.y * T
    W, H = p.w * T, p.h * T
    box(cv, px + 1, py + 2, W - 2, H - 4, rid, 4 if not stone else 3, 4, 2)
    cv.rect(px + 2, py + H - 3, 2, 3, rid, 1)
    cv.rect(px + W - 4, py + H - 3, 2, 3, rid, 1)
    g = np.random.default_rng(pix.seed_of(ctx.sc.md.map_id, "table", p.x, p.y))
    if items == "tea":
        porcelain = mm.cloth("white")
        cx, cy = px + W // 2, py + 2 + (H - 8) // 2
        cv.rect(cx - 2, cy - 1, 4, 3, porcelain, 3)
        cv.px(cx - 2, cy - 1, porcelain, 4)
        cv.px(cx + 2, cy, porcelain, 2)
        for dx in (-6, 5):
            cv.rect(cx + dx, cy + 1, 2, 2, porcelain, 3)
    elif items == "desk":
        pap = mm.paper()
        cv.rect(px + 4, py + 4, 9, 6, pap, 4)
        for yy in range(py + 5, py + 10, 2):
            cv.rect(px + 5, yy, 6, 1, pap, 2)
        cv.rect(px + W - 9, py + 5, 4, 3, mm.void(), 2)
        cv.rect(px + W - 12, py + 4, 1, 6, mm.wood(), 3)
    elif items == "herbs":
        for _ in range(4):
            cv.rect(px + 3 + int(g.integers(0, W - 8)), py + 4 + int(g.integers(0, max(1, H - 12))), 3, 2,
                    mm.herb(), 2 + int(g.integers(0, 2)))
    mark(ctx, px + 1, py + 2, W - 2, H - 2)


def counter(ctx, p) -> None:
    """柜台：朝南一面立板，台面上摆着算盘与几只陶罐。"""
    cv = ctx.below
    rid = mm.wood()
    px, py = p.x * T, p.y * T
    W, H = p.w * T, p.h * T
    box(cv, px, py + 2, W, H - 2, rid, 4, 8, 2)
    for xx in range(px + 4, px + W - 2, 6):
        cv.rect(xx, py + H - 7, 1, 6, rid, 1)
    g = np.random.default_rng(pix.seed_of(ctx.sc.md.map_id, "counter", p.x))
    for xx in range(px + 3, px + W - 6, 9):
        jar = mm.earthwall()
        cv.rect(xx, py + 2, 4, 4, jar, 3)
        cv.px(xx, py + 2, jar, 4)
        cv.rect(xx + 1, py + 1, 2, 1, jar, 2)
    cv.rect(px + W - 12, py + 3, 8, 3, mm.darkwood(), 3)
    for xx in range(px + W - 11, px + W - 5, 2):
        cv.px(xx, py + 4, mm.gold(), 3)
    mark(ctx, px, py + 2, W, H - 2)


def bed(ctx, p) -> None:
    """床：床架、被褥（靛蓝）、枕头（素白），床头在北。"""
    cv = ctx.below
    wd = mm.darkwood()
    q = mm.cloth("indigo")
    pl = mm.cloth("white")
    px, py = p.x * T, p.y * T
    W, H = p.w * T, p.h * T
    box(cv, px, py, W, H, wd, 3, 3, 2)
    cv.rect(px + 2, py + 2, W - 4, H - 6, q, 3)
    cv.rect(px + 2, py + 2, W - 4, 1, q, 4)
    for xx in range(px + 4, px + W - 3, 5):
        cv.rect(xx, py + 3, 1, H - 7, q, 2)
    cv.rect(px + 3, py + 2, 7, 3, pl, 3)
    cv.rect(px + 3, py + 2, 7, 1, pl, 4)
    cv.rect(px, py - 3, W, 4, wd, 2)
    cv.rect(px, py - 3, W, 1, wd, 3)
    mark(ctx, px, py, W, H)


def canopy_bed(ctx, p) -> None:
    """架子床（居所那两张，gid 5 实心块 + 北侧一行帐幔）：四根床柱、垂下的帐子、床顶（above）。"""
    cv = ctx.below
    wd = mm.darkwood()
    cur = mm.cloth("white")
    px, py = p.x * T, p.y * T
    W, H = p.w * T, p.h * T
    q = mm.cloth("ochre" if pix.hash01(p.x, p.y, 17) < 0.5 else "indigo")
    box(cv, px, py + 2, W, H - 2, wd, 2, 4, 1)
    # 被褥：一整片平涂在 ×3 下是一块空白的色板——压一道被边、几道横褶，右侧背光，
    # 床头翻出一截素白被里，枕头压在上面
    qx, qy, qw, qh = px + 3, py + 5, W - 6, H - 12
    cv.rect(qx, qy, qw, qh, q, 3)
    cv.rect(qx, qy + qh - 1, qw, 1, q, 1)
    cv.rect(qx + qw - 2, qy, 2, qh, q, 2)
    cv.rect(qx + 1, qy + 1, 1, qh - 2, q, 2)
    for yy in range(qy + 9, qy + qh - 2, 6):
        cv.rect(qx + 2, yy, qw - 4, 1, q, 2)
        cv.rect(qx + 2, yy - 1, qw - 5, 1, q, 4)
    cv.rect(qx, qy, qw, 4, cur, 3)
    cv.rect(qx, qy + 4, qw, 1, cur, 1)
    cv.rect(qx + 1, qy, 8, 3, cur, 4)
    cv.rect(qx + 1, qy + 3, 8, 1, cur, 2)
    # 帐子：两侧垂下的素帐，收在床柱上
    for xx in (px + 1, px + W - 4):
        cv.rect(xx, py, 3, H - 4, cur, 3)
        cv.rect(xx, py, 1, H - 4, cur, 4)
    for xx in (px, px + W - 2):
        cv.rect(xx, py - 4, 2, H + 2, wd, 2)
    mark(ctx, px, py, W, H, True)

    def top(c):
        c.rect(px - 1, py - 12, W + 2, 3, wd, 3)
        c.rect(px - 1, py - 12, W + 2, 1, wd, 4)
        for xx in range(px, px + W, 2):
            c.rect(xx, py - 9, 1, 4 + (xx // 2) % 2, cur, 3 if (xx // 2) % 2 else 2)
        c.rect(px - 1, py - 9, 2, 12, wd, 2)
        c.rect(px + W - 1, py - 9, 2, 12, wd, 1)

    both(ctx, py, top)


def cabinet(ctx, p, drawers: bool = True) -> None:
    """药柜 / 衣柜：高出格子一截，正面一格格抽屉（药柜）或两扇门（衣柜）。"""
    px, py = p.x * T, p.y * T
    W, H = p.w * T, p.h * T
    wd = mm.darkwood()
    g = mm.gold()

    def draw(cv):
        cv.rect(px, py - 10, W, H + 8, wd, 3)
        cv.rect(px - 1, py - 12, W + 2, 3, wd, 4)
        cv.rect(px - 1, py - 10, W + 2, 1, wd, 1)
        if drawers:
            for yy in range(py - 8, py + H - 3, 4):
                for xx in range(px + 1, px + W - 1, 4):
                    cv.rect(xx, yy, 3, 3, wd, 2)
                    cv.px(xx + 1, yy + 1, g, 3)
        else:
            mid = px + W // 2
            cv.rect(px + 1, py - 8, W - 2, H + 3, wd, 2)
            cv.rect(mid, py - 8, 1, H + 3, wd, 0)
            cv.px(mid - 2, py, g, 3)
            cv.px(mid + 2, py, g, 3)
        cv.rect(px, py + H - 2, W, 2, wd, 1)

    both(ctx, py, draw, tall=True)


def bookshelf_cell(ctx, pid: int, x: int, y: int) -> None:
    """书架（南北向的一排）：从上往下看是一条深色木顶，两沿露出书脊；最南一格是架子的端面。"""
    cv = ctx.below
    L, R, U, D = conn(ctx, pid, x, y)
    px, py = x * T, y * T
    wd = mm.darkwood()
    books = [mm.cloth("indigo"), mm.cloth("red"), mm.cloth("ochre"), mm.paper(), mm.herb()]
    cv.rect(px + 2, py, 12, T, wd, 3)
    cv.rect(px + 2, py, 1, T, wd, 4)
    cv.rect(px + 13, py, 1, T, wd, 1)
    for yy in range(py, py + T, 2):
        r = books[int(pix.hash01(x, yy, 7) * len(books))]
        cv.rect(px + 3, yy, 2, 2, r, 3)
        r2 = books[int(pix.hash01(x + 99, yy, 7) * len(books))]
        cv.rect(px + 11, yy, 2, 2, r2, 2)
    cv.rect(px + 6, py, 4, T, wd, 4)
    if not D:
        # 端面：三层隔板，每层一排书脊
        cv.rect(px + 2, py + 6, 12, 10, wd, 2)
        for k, yy in enumerate((py + 7, py + 10, py + 13)):
            for xx in range(px + 3, px + 13):
                r = books[int(pix.hash01(xx, yy, 11) * len(books))]
                cv.rect(xx, yy, 1, 2, r, 3 if xx % 2 else 2)
            cv.rect(px + 2, yy + 2, 12, 1, wd, 1)
    mark(ctx, px + 2, py, 12, T, True)


def shelf_top(ctx, p) -> None:
    """书架北端那一格 front：架子比人高，顶头伸进北边那一格（above）。"""
    x, y = p.x, p.y - 1
    px, py = x * T, y * T
    wd = mm.darkwood()
    ctx.above.rect(px + 2, py + 6, 12, 10, wd, 3)
    ctx.above.rect(px + 2, py + 6, 12, 1, wd, 4)
    ctx.above.rect(px + 6, py + 7, 4, 9, wd, 4)


def chest(ctx, p, fancy: bool = False) -> None:
    cv = ctx.below
    wd = mm.darkwood() if fancy else mm.wood()
    px, py = p.x * T, p.y * T
    W, H = p.w * T, p.h * T
    box(cv, px + 2, py + 3, W - 4, H - 5, wd, 3, 6, 2)
    g = mm.gold() if fancy else mm.metal()
    for (xx, yy) in ((px + 2, py + 3), (px + W - 4, py + 3), (px + 2, py + H - 4), (px + W - 4, py + H - 4)):
        cv.rect(xx, yy, 2, 2, g, 3)
    cv.rect(px + W // 2 - 1, py + H - 8, 3, 3, g, 3)
    mark(ctx, px + 2, py + 3, W - 4, H - 4)


def water_jar(ctx, p) -> None:
    cv = ctx.below
    px, py = p.x * T, p.y * T
    W, H = p.w * T, p.h * T
    glaze = mm.roof("black")
    disc_fill(cv, px + W / 2, py + H / 2, W / 2 - 1, H / 2 - 2, glaze, 3, 1.6)
    disc_fill(cv, px + W / 2, py + H / 2 - 1, W / 2 - 4, H / 2 - 5, mm.water(), 2, 0.8)
    cv.rect(px + W // 2 - 4, py + H // 2 - 3, 4, 1, mm.water(), 5)
    mark(ctx, px + 1, py + 2, W - 2, H - 3)


def drying_rack(ctx, p) -> None:
    """晒药架：木架上几只竹匾，匾里摊着药材。"""
    cv = ctx.below
    px, py = p.x * T, p.y * T
    W, H = p.w * T, p.h * T
    wd = mm.wood()
    b = mm.bamboo()
    for xx in (px + 1, px + W - 3):
        cv.rect(xx, py, 2, H, wd, 2)
    for i, (cx, cy) in enumerate(((px + 8, py + 8), (px + W - 8, py + 8), (px + 8, py + H - 8),
                                  (px + W - 8, py + H - 8))):
        disc_fill(cv, cx, cy, 6, 5, b, 3, 0.6)
        disc_fill(cv, cx, cy, 4, 3, mm.herb() if i % 2 == 0 else mm.dirt(), 3, 1.0)
    mark(ctx, px, py, W, H)


def furnace(ctx, p) -> None:
    """丹炉 / 药炉：石台上一只三足铜鼎，鼎腹下一口火。比格子高出一截。"""
    px, py = p.x * T, p.y * T
    W, H = p.w * T, p.h * T
    bronze = mm.gold()
    st = mm.rock("cliff")
    cx = px + W // 2

    def draw(cv):
        box(cv, px + 1, py + H - 8, W - 2, 7, st, 3, 3)
        cv.rect(cx - 9, py + H - 12, 2, 5, bronze, 1)
        cv.rect(cx + 7, py + H - 12, 2, 5, bronze, 1)
        disc_fill(cv, cx, py + H - 17, 10, 8, bronze, 2, 1.5)
        cv.rect(cx - 8, py + H - 25, 16, 3, bronze, 3)
        cv.rect(cx - 8, py + H - 25, 16, 1, bronze, 4)
        cv.rect(cx - 2, py + H - 29, 4, 4, bronze, 3)
        cv.rect(cx - 11, py + H - 22, 2, 3, bronze, 2)
        cv.rect(cx + 9, py + H - 22, 2, 3, bronze, 1)
        cv.rect(cx - 4, py + H - 11, 8, 3, mm.glow(), 2)
        cv.rect(cx - 3, py + H - 10, 6, 1, mm.glow(), 4)

    both(ctx, py, draw, tall=True)
    ctx.lights.append({"kind": "furnace", "x": (px + W / 2) / T, "y": (py + H - 10) / T})


def dais(ctx, p) -> None:
    """矮榻上一只蒲团（客栈上房的打坐处）。"""
    cv = ctx.below
    px, py = p.x * T, p.y * T
    W, H = p.w * T, p.h * T
    box(cv, px + 1, py + 3, W - 2, H - 4, mm.darkwood(), 3, 4, 2)
    for k, cx in enumerate((px + W // 4 + 1, px + 3 * W // 4 - 1)):
        disc_fill(cv, cx, py + 7, 5, 3, mm.roof("thatch"), 4, 0.8)
        cv.px(cx, py + 7, mm.roof("thatch"), 2)
    mark(ctx, px + 1, py + 3, W - 2, H - 3)


def stone_bed(ctx, p) -> None:
    cv = ctx.below
    px, py = p.x * T, p.y * T
    W, H = p.w * T, p.h * T
    st = mm.rock("cliff")
    box(cv, px + 1, py + 2, W - 2, H - 3, st, 3, 6, 2)
    cv.rect(px + 4, py + 4, W - 8, H - 13, mm.roof("thatch"), 3)
    for yy in range(py + 5, py + H - 9, 2):
        cv.rect(px + 4, yy, W - 8, 1, mm.roof("thatch"), 2)
    mark(ctx, px + 1, py + 2, W - 2, H - 2)


def coffin(ctx, p) -> None:
    cv = ctx.below
    px, py = p.x * T, p.y * T
    W, H = p.w * T, p.h * T
    st = mm.rock("cave")
    box(cv, px + 1, py + 2, W - 2, H - 3, st, 4, 5, 2)
    cv.rect(px + 3, py + 4, W - 6, 1, st, 5)
    cv.rect(px + 3, py + 8, W - 6, 1, st, 2)
    mark(ctx, px + 1, py + 2, W - 2, H - 2)


def bird_perch(ctx, p) -> None:
    """鸟架与云翅鸟：一根横杆两根立柱，杆上停着一只青灰的鸟。"""
    px, py = p.x * T, p.y * T
    W = p.w * T
    wd = mm.wood()
    bird = mm.cloth("indigo")

    def draw(cv):
        cv.rect(px + 3, py - 2, 2, 17, wd, 2)
        cv.rect(px + W - 5, py - 2, 2, 17, wd, 1)
        cv.rect(px + 2, py - 3, W - 4, 2, wd, 3)
        bx = px + W // 2 - 3
        cv.rect(bx, py - 9, 7, 5, bird, 3)
        cv.rect(bx + 5, py - 11, 3, 3, bird, 4)
        cv.px(bx + 8, py - 10, mm.gold(), 3)
        cv.rect(bx - 3, py - 7, 3, 2, bird, 2)
        cv.px(bx + 2, py - 9, bird, 4)

    both(ctx, py, draw, solid=True)


def cage(ctx, p) -> None:
    cv = ctx.below
    px, py = p.x * T, p.y * T
    W, H = p.w * T, p.h * T
    st = mm.rock("cave")
    mt = mm.metal()
    box(cv, px + 1, py + 1, W - 2, H - 2, st, 2, 4, 1)
    for xx in range(px + 2, px + W - 2, 3):
        cv.rect(xx, py + 2, 1, H - 4, mt, 3)
    cv.rect(px + 1, py + 1, W - 2, 1, mt, 4)
    mark(ctx, px + 1, py + 1, W - 2, H - 2)


def stone_mill(ctx, p) -> None:
    """石碾：圆碾盘、中间一根轴、一道推杆。"""
    cv = ctx.below
    px, py = p.x * T, p.y * T
    W, H = p.w * T, p.h * T
    st = mm.rock("cliff")
    disc_fill(cv, px + W / 2, py + H / 2 + 1, W / 2 - 2, H / 2 - 2, st, 3, 1.2)
    disc_fill(cv, px + W / 2 - 4, py + H / 2, 5, 4, st, 4, 1.0)
    cv.rect(px + W // 2 - 4, py + 2, 2, 5, mm.wood(), 3)
    cv.rect(px + W // 2 - 4, py + 2, 12, 2, mm.wood(), 3)
    mark(ctx, px + 2, py + 2, W - 4, H - 3)


def hutch(ctx, p) -> None:
    cv = ctx.below
    px, py = p.x * T, p.y * T
    W, H = p.w * T, p.h * T
    wd = mm.wood()
    box(cv, px + 1, py + 2, W - 2, H - 3, wd, 3, 7, 2)
    for xx in range(px + 3, px + W - 2, 3):
        cv.rect(xx, py + H - 8, 1, 6, wd, 1)
    cv.rect(px + 4, py + 4, W - 8, 3, mm.roof("thatch"), 3)
    cv.rect(px + W // 2, py + H - 6, 2, 2, mm.cloth("white"), 4)
    mark(ctx, px + 1, py + 2, W - 2, H - 2)


def well(ctx, p) -> None:
    """井台：方石井栏，井口一汪暗水，上面一座木辘轳（above）。"""
    px, py = p.x * T, p.y * T
    W, H = p.w * T, p.h * T
    st = mm.rock("cliff")
    wd = mm.wood()
    cv = ctx.below
    box(cv, px + 2, py + 3, W - 4, H - 4, st, 3, 5, 2)
    cv.rect(px + 7, py + 7, W - 14, H - 16, mm.water(), 1)
    cv.rect(px + 8, py + 8, 3, 1, mm.water(), 3)
    mark(ctx, px + 2, py + 3, W - 4, H - 3, True)

    def frame(c):
        c.rect(px + 4, py - 6, 2, 14, wd, 2)
        c.rect(px + W - 6, py - 6, 2, 14, wd, 1)
        c.rect(px + 3, py - 7, W - 6, 2, wd, 3)
        c.rect(px + W // 2 - 3, py - 5, 6, 3, wd, 2)

    both(ctx, py + 4, frame)


def pavilion(ctx, p) -> None:
    """亭：石台、四根朱柱、当中一张石桌；亭顶（带翘角的攒尖顶）压在北侧那一行 front 上（above）。"""
    px, py = p.x * T, p.y * T
    W, H = p.w * T, p.h * T
    st = mm.paving("manor")
    red = mm.lacquer()
    cv = ctx.below
    box(cv, px - 4, py + 2, W + 8, H - 2, st, 3, 4, 2)
    disc_fill(cv, px + W / 2, py + H / 2 + 2, 5, 4, mm.rock("cliff"), 3, 1.0)
    mark(ctx, px - 4, py, W + 8, H, True)

    def roof(c):
        for xx in (px - 2, px + W):
            c.rect(xx, py - 4, 2, H + 2, red, 2)
            c.rect(xx, py - 4, 1, H + 2, red, 3)
        rid = mm.roof("black" if ctx.sc.theme in ("manor", "wild") else "grey")
        top = py - 22
        for k in range(12):
            half = 4 + k * 2
            y = top + k
            c.rect(px + W // 2 - half, y, half * 2, 1, rid, 3 if k % 3 else 2)
            c.px(px + W // 2 - half, y, rid, 4)
            c.px(px + W // 2 + half - 1, y, rid, 1)
        c.rect(px + W // 2 - 26, top + 12, 52, 2, rid, 1)
        c.rect(px + W // 2 - 26, top + 12, 52, 1, rid, 4)
        c.rect(px + W // 2 - 1, top - 3, 2, 3, mm.gold(), 3)
        for side, xe in ((-1, px + W // 2 - 27), (1, px + W // 2 + 26)):
            c.px(xe, top + 11, rid, 3)
            c.px(xe + side, top + 10, rid, 3)

    both(ctx, py + 2, roof)


def rockery(ctx, p) -> None:
    """假山：几块叠起来的太湖石，孔洞处最暗，顶上几丛草；石头高出格子一截（above）。"""
    px, py = p.x * T, p.y * T
    W, H = p.w * T, p.h * T
    st = mm.rock("valley")
    g = np.random.default_rng(pix.seed_of(ctx.sc.md.map_id, "rockery", p.x, p.y))
    blobs = [(px + W * (0.2 + 0.6 * g.random()), py + H * (0.2 + 0.7 * g.random()) - 6,
              5 + g.random() * 5, 5 + g.random() * 4) for _ in range(7)]
    blobs.sort(key=lambda b: b[1])

    def draw(c):
        for bx, by, rx, ry in blobs:
            disc_fill(c, bx, by, rx, ry, st, 3, 1.8)
        for bx, by, rx, ry in blobs[::2]:
            c.rect(int(bx) - 1, int(by), 2, 2, st, 0)
        moss = mm.grass(ctx.sc.season)
        for bx, by, rx, ry in blobs[:3]:
            c.px(int(bx), int(by - ry) + 1, moss, 3)
            c.px(int(bx) + 1, int(by - ry) + 1, moss, 4)

    both(ctx, py, draw, tall=True)


def side_door(ctx, p) -> None:
    cv = ctx.below
    px, py = p.x * T, p.y * T
    wd = mm.darkwood()
    cv.rect(px + 9, py + 1, 5, 14, wd, 2)
    cv.rect(px + 9, py + 1, 1, 14, wd, 3)
    cv.px(px + 12, py + 8, mm.gold(), 3)
    mark(ctx, px, py, T, T, True)


def gate_doors(ctx, p) -> None:
    """一道关着的大门（演武场东辕门）：厚木门板、门钉、两端门柱。"""
    cv = ctx.below
    px, py = p.x * T, p.y * T
    W, H = p.w * T, p.h * T
    wd = mm.darkwood()
    red = mm.lacquer()
    cv.rect(px + 2, py, W - 4, H, wd, 2)
    for yy in range(py + 3, py + H, 4):
        for xx in range(px + 4, px + W - 3, 4):
            cv.px(xx, yy, mm.gold(), 3)
    cv.rect(px + 2, py + H // 2, W - 4, 1, wd, 0)
    for yy in (py - 2, py + H - 4):
        box(cv, px, yy, W, 6, red, 3, 3)
    mark(ctx, px, py, W, H, True)


def bollard(ctx, x: int, y: int) -> None:
    cv = ctx.below
    px, py = x * T, y * T
    wd = mm.wood()
    cv.rect(px + 6, py + 4, 4, 11, wd, 2)
    cv.rect(px + 6, py + 4, 1, 11, wd, 3)
    cv.rect(px + 5, py + 3, 6, 2, wd, 3)
    cv.rect(px + 6, py + 8, 4, 1, mm.roof("thatch"), 3)
    mark(ctx, px + 5, py + 3, 6, 12)


def woodpile(ctx, p) -> None:
    cv = ctx.below
    px, py = p.x * T, p.y * T
    W, H = p.w * T, p.h * T
    bark = mm.bark()
    wd = mm.wood()
    for row, yy in enumerate(range(py + H - 5, py + 2, -4)):
        for xx in range(px + 1 + (row % 2) * 2, px + W - 3, 4):
            cv.rect(xx, yy, 4, 4, bark, 2)
            cv.rect(xx + 1, yy + 1, 2, 2, wd, 4)
    mark(ctx, px, py + 2, W, H - 2)


# ---------------------------------------------------------------------------
# 分派
# ---------------------------------------------------------------------------
LINE_KINDS = ("bamboo_fence", "railing", "balustrade")


def paint_props(ctx) -> None:
    sc = ctx.sc
    mid = sc.md.map_id
    for pid, p in enumerate(sc.props):
        seed = pix.seed_of(mid, "prop", p.x, p.y)
        k = p.kind
        cells = [(x, y) for y in range(p.y, p.y + p.h) for x in range(p.x, p.x + p.w)
                 if sc.prop_at[y, x] == pid]
        if k in LINE_KINDS:
            for x, y in cells:
                fence_cell(ctx, pid, x, y, k)
        elif k == "rock_row":
            for x, y in cells:
                rock(ctx, x, y, True, seed + x * 7 + y)
        elif k == "bookshelf":
            for x, y in cells:
                bookshelf_cell(ctx, pid, x, y)
            if any(e.pid == pid for e in sc.eaves):
                shelf_top(ctx, p)
        elif k in ("stall", "stall_row"):
            if p.h == 1:
                stall(ctx, p.x, p.y, p.w, seed)
            else:
                for x, y in cells:
                    stall(ctx, x, y, 1, seed + x)
        elif k == "weapon_rack":
            for x, y in cells:
                weapon_rack(ctx, x, y, 1, seed + x)
        elif k == "weapon_rack_row":
            for x0 in range(p.x, p.x + p.w, 3):
                weapon_rack(ctx, x0, p.y, min(3, p.x + p.w - x0), seed + x0)
        elif k == "target":
            target(ctx, p.x, p.y)
        elif k == "stake":
            stake(ctx, p.x, p.y, sc.theme == "cliff")
        elif k in ("rock", "fallen_rock"):
            for x, y in cells:
                rock(ctx, x, y, sc.th["kind"] == "cave" or k == "fallen_rock", seed + x)
        elif k == "stump":
            stump(ctx, p.x, p.y)
        elif k == "stone_lantern":
            for x, y in cells:
                stone_lantern(ctx, x, y)
        elif k == "pillar":
            pillar(ctx, p.x, p.y)
        elif k == "haystack":
            haystack(ctx, p.x, p.y, p.w)
        elif k == "shed":
            shed(ctx, p)
        elif k == "table":
            table(ctx, p, items="tea")
        elif k == "herb_table":
            table(ctx, p, items="herbs")
        elif k == "stone_table":
            table(ctx, p, stone=True, items="herbs")
        elif k == "desk":
            table(ctx, p, items="desk")
        elif k == "counter":
            counter(ctx, p)
        elif k == "bed":
            bed(ctx, p)
        elif k == "canopy_bed":
            canopy_bed(ctx, p)
        elif k == "cabinet":
            cabinet(ctx, p, True)
        elif k == "wardrobe":
            cabinet(ctx, p, False)
        elif k == "chest":
            chest(ctx, p)
        elif k == "book_chest":
            chest(ctx, p, fancy=True)
        elif k == "water_jar":
            water_jar(ctx, p)
        elif k == "drying_rack":
            drying_rack(ctx, p)
        elif k == "furnace":
            furnace(ctx, p)
        elif k == "dais":
            dais(ctx, p)
        elif k == "stone_bed":
            stone_bed(ctx, p)
        elif k == "coffin":
            coffin(ctx, p)
        elif k == "bird_perch":
            bird_perch(ctx, p)
        elif k == "cage":
            cage(ctx, p)
        elif k == "stone_mill":
            stone_mill(ctx, p)
        elif k == "hutch":
            hutch(ctx, p)
        elif k == "well":
            well(ctx, p)
        elif k == "pavilion":
            pavilion(ctx, p)
        elif k == "rockery":
            rockery(ctx, p)
        elif k == "side_door":
            side_door(ctx, p)
        elif k == "gate_doors":
            gate_doors(ctx, p)
        elif k == "bollard":
            for x, y in cells:
                bollard(ctx, x, y)
        elif k == "woodpile":
            woodpile(ctx, p)
        elif k == "stool":
            for x, y in cells:
                table(ctx, type(p)("stool", x, y, 1, 1), items="none")
        else:
            raise ValueError("%s：道具 %s 没有画法（maps.json 拼错了？）" % (mid, k))


# ---------------------------------------------------------------------------
# 贴花：maps.json 的 decals（船）。只许贴在挡路的格上——贴花不能让能走的地方看着像挡路。
# ---------------------------------------------------------------------------
def boat(ctx, x: int, y: int, w: int, h: int) -> None:
    """一条乌篷船：木船身横在水上，当中一截弧形的篷，船头一根缆绳。"""
    cv = ctx.below
    px, py = x * T, y * T
    W, H = w * T, h * T
    wd = mm.wood()
    cy = py + H // 2
    for k in range(W - 4):
        xx = px + 2 + k
        f = abs((k - (W - 4) / 2) / ((W - 4) / 2))
        half = int(round((H / 2 - 5) * (1 - f ** 3)))
        cv.rect(xx, cy - half, 1, half * 2 + 2, wd, 2)
        cv.px(xx, cy - half, wd, 4)
        cv.px(xx, cy + half + 1, wd, 0)
        cv.px(xx, cy - half + 1, wd, 3)
    cv.rect(px + W // 2 - 10, cy - 3, 20, 6, wd, 1)
    roof = mm.roof("black")
    for k in range(18):
        xx = px + W // 2 - 9 + k
        cv.rect(xx, cy - 7, 1, 10, roof, 2 + (k % 3 == 0))
        cv.px(xx, cy - 7, roof, 4)
    cv.rect(px + W // 2 - 9, cy + 3, 18, 1, roof, 0)
    ctx.below.shade(px + 3, cy + H // 2 - 3, -1.0, np.ones((2, W - 6), bool))


def paint_decals(ctx) -> None:
    sc = ctx.sc
    for d in sc.preset.get("decals", []):
        x, y = d["at"]
        w, h = d.get("w", 1), d.get("h", 1)
        if sc.walk[y:y + h, x:x + w].any():
            raise ValueError("%s：贴花 %s 压到了可走格，会让能走的地方看着像挡路" % (sc.md.map_id, d))
        if d["kind"] == "boat":
            boat(ctx, x, y, w, h)
        else:
            raise ValueError("%s：不认识的贴花 %s" % (sc.md.map_id, d["kind"]))


# ---------------------------------------------------------------------------
# 设施（facility 对象）：落在可走格上的只画贴地的一层，不画成立起来的东西——
# 设施不挡路，画成一只立着的炉子，玩家会以为那里走不过去。
# ---------------------------------------------------------------------------
def save_seal(cv, cx: int, cy: int) -> None:
    """存档点：地上嵌一块圆石，石面一圈青玉纹（运行时再给一点青光）。"""
    st = mm.rock("cliff")
    jade = mm.jadeglow()
    disc_fill(cv, cx, cy, 6, 4, st, 3, 0.6)
    ring = pix.ellipse(9, 5) & ~pix.erode(pix.ellipse(9, 5), 1)
    cv.paint(cx - 4, cy - 2, ring, jade, 3.0)
    cv.px(cx, cy, jade, 4)


def cushion(cv, cx: int, cy: int, mat: tuple[int, int, int, int] | None) -> None:
    """打坐处：（两格见方的铺一张草席）当中一只蒲团。都是贴地的。"""
    straw = mm.roof("thatch")
    if mat is not None:
        x, y, w, h = mat
        cv.rect(x + 2, y + 3, w - 4, h - 6, straw, 3)
        for yy in range(y + 4, y + h - 3, 2):
            cv.rect(x + 3, yy, w - 6, 1, straw, 2)
        cv.rect(x + 2, y + 3, w - 4, 1, straw, 4)
    disc_fill(cv, cx, cy, 5, 3, straw, 4, 0.6)
    ring = pix.ellipse(7, 4) & ~pix.erode(pix.ellipse(7, 4), 1)
    cv.paint(cx - 3, cy - 2, ring, straw, 2.0)


def hearth(cv, x: int, y: int, w: int, h: int) -> None:
    """地火：石块围成的方灶口，里面一层暗红的炭（亮处由运行时的火光补）。"""
    st = mm.rock("cliff")
    glow = mm.glow()
    cv.rect(x + 3, y + 4, w - 6, h - 8, st, 3)
    cv.rect(x + 5, y + 6, w - 10, h - 12, mm.void(), 1)
    g = np.random.default_rng(x * 131 + y)
    for _ in range((w - 10) * (h - 12) // 5):
        cv.px(x + 5 + int(g.integers(0, w - 10)), y + 6 + int(g.integers(0, h - 12)), glow, int(g.integers(0, 3)))
    for xx in range(x + 3, x + w - 3, 4):
        cv.px(xx, y + 4, st, 4)
        cv.px(xx, y + h - 5, st, 1)


def notice_board(cv, x: int, y: int, w: int) -> None:
    """告示板：木框里贴着几张纸。"""
    wd = mm.wood()
    pap = mm.paper()
    cv.rect(x, y, w, 10, wd, 2)
    cv.rect(x, y, w, 1, wd, 3)
    for i, xx in enumerate(range(x + 2, x + w - 4, 5)):
        cv.rect(xx, y + 2 + (i % 2), 4, 6, pap, 3 + (i % 2))
        cv.rect(xx, y + 3 + (i % 2), 3, 1, pap, 1)


def paint_facilities(ctx) -> None:
    sc = ctx.sc
    cv = ctx.below
    for o in sc.md.objects:
        if o.type != "facility":
            continue
        kind = o.props.get("kind")
        cells = [(x, y) for x, y in o.cells() if 0 <= x < sc.W and 0 <= y < sc.H]
        on_floor = all(sc.walk[y, x] for x, y in cells)
        X, Y, Wd, Hd = o.x * T, o.y * T, o.w * T, o.h * T
        cx, cy = X + Wd // 2, Y + Hd // 2
        if kind == "save" and on_floor:
            save_seal(cv, cx, cy + 2)
        elif kind == "meditate" and on_floor:
            cushion(cv, cx, cy, (X, Y, Wd, Hd) if o.w >= 2 and o.h >= 2 else None)
        elif kind in ("alchemy", "forge") and on_floor:
            hearth(cv, X, Y, Wd, Hd)
        elif kind in ("board", "shop"):
            above_wall = [(x, o.y - 1) for x in range(o.x, o.x + o.w)
                          if o.y - 1 >= 0 and sc.wall[o.y - 1, x] and sc.walk[o.y, x]]
            if above_wall:
                # 挂在门脸上：告示板贴墙，铺子挑一面幌子
                wx = above_wall[0][0] * T
                wy = (o.y - 1) * T
                if kind == "board":
                    notice_board(cv, wx + 1, wy + 3, len(above_wall) * T - 2)
                else:
                    fa.paint_banner(cv, wx + 5, wy + 2, "shop")
            elif kind == "board" and on_floor:
                # 空地上的告示牌：两根矮柱撑一块板，立在这块地的北沿
                wd = mm.wood()
                cv.rect(X + 3, Y + 2, 2, 10, wd, 2)
                cv.rect(X + Wd - 5, Y + 2, 2, 10, wd, 1)
                notice_board(cv, X + 1, Y - 2, Wd - 2)
                cv.shade(X + 3, Y + 12, -1.0, np.ones((2, Wd - 4), bool))
