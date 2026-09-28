"""有屋顶的房子：村屋、铺面、殿、屋顶群里的每一户。

一户（lot）的画法，自上而下：
  屋脊（北侧若有一行 front，屋脊画在那一行里、进 above.png）
  → 瓦坡（青瓦/黛瓦/宗门深青：竖向瓦垄明暗相间，每 5px 一道瓦节；茅草：竖向草纹 + 层线）
  → 檐口（一排瓦当 + 一线暗 + 檐下 2px 影子落在立面上）
  → 立面（南邻可走时才有：两格高的房子立面占两格，一格厚的只占一格）
     梁、柱、窗、门、台基；门的位置按「门前是路 / 传送点 / 设施 / 人」挑，挑不出就放中间。
  屋顶左右两端是博风（左亮右暗各一线），檐口两角各翘出 2px（飞檐）。

立面判据仍是那一条：墙格南邻不是墙 → 立面；南邻也是墙 → 屋顶。只是房子深 ≥3 格时
立面取两格高——一格（16px）的立面比人（24px）还矮，门画不进去。
"""

from __future__ import annotations

import numpy as np

import mapfacade as fa
import mapmat as mm
import pix
from mapinfer import Scene, Structure, label4

T = 16


class Ctx:
    """一张图烘焙期间共享的东西：两张画布、结构推断、收集起来的光源候选。"""

    def __init__(self, sc: Scene, below: pix.Canvas, above: pix.Canvas) -> None:
        self.sc = sc
        self.below = below
        self.above = above
        self.lights: list[dict] = []     # 由各画法顺手登记：灯笼、窗、门
        self.solid_px = np.zeros((below.h, below.w), bool)   # 画了实体的像素（投影用）
        self.tall_px = np.zeros((below.h, below.w), bool)    # 高的实体（屋、墙、崖）——投影更长
        self._patterns: dict[str, np.ndarray] = {}
        self.hips: dict[int, tuple[int, int, int, int, int]] = {}   # 歇山顶：sid → (X0, X1, 收进, 屋脊y, 檐口y)
        self.rock_top: dict[int, str] = {}   # 山石：sid → 顶面画法（forest / rocky / grass …），挂崖檐装饰时要看
        self.objects_at: dict[tuple[int, int], list] = {}
        for o in sc.md.objects:
            for cell in o.cells():
                self.objects_at.setdefault(cell, []).append(o)

    def roof_tex(self, style: str) -> tuple[int, np.ndarray]:
        """瓦坡：(渐层, 整图大小的明度级纹理)。同一种瓦纹，换渐层就是换瓦色。"""
        kind = "thatch" if style.startswith("thatch") else "tile"
        if kind not in self._patterns:
            self._patterns[kind] = roof_pattern(kind, self.below.h, self.below.w,
                                                pix.seed_of(self.sc.md.map_id, "roof", kind))
        return mm.roof(style), self._patterns[kind]


# ---------------------------------------------------------------------------
# 瓦坡纹理（整图算一次，按绝对坐标取，相邻两户的瓦垄对得齐）
# ---------------------------------------------------------------------------
# 青瓦：每 4px 一道瓦垄——瓦沟一像素暗线从檐口通到屋脊，筒瓦三像素（左受光、中最亮、
# 右背光）；筒瓦每 6px 一节，节头一线亮（上一片瓦压下来的瓦口）、节尾一线影。
# 瓦沟不断、瓦节只落在筒瓦上：×3 放大后读得出一道道竖垄。
# （上一版每 5px 一道横贯全坡的暗层线，瓦面被切成一格格小方块，远看像针织纹、近看是噪点。）
_TILE_LUT = np.array([
    # yb: 0  1  2  3  4  5
    [1, 1, 1, 1, 1, 1],   # xm 0：瓦沟
    [4, 3, 3, 3, 3, 2],   # xm 1：筒瓦受光
    [5, 4, 4, 4, 3, 2],   # xm 2：筒瓦脊
    [3, 2, 2, 2, 2, 1],   # xm 3：筒瓦背光
], np.float32)


def roof_pattern(kind: str, h: int, w: int, seed: int) -> np.ndarray:
    ys = np.arange(h)[:, None]
    xs = np.arange(w)[None, :]
    if kind == "thatch":
        # 茅草：一层一层压上去的草束。每层 6px：层顶压在上一层的草梢底下（暗），
        # 往下草茎渐亮，层底草梢参差（每列长短不一），最末一像素是下一层投的影。
        yl = ys % 6
        tip = (pix.hash01(xs, ys // 6, seed) * 3).astype(np.int64)
        strand = pix.hash01(xs, ys // 6, seed + 1)
        lv = np.select([yl == 0, yl == 1, yl <= 3, yl == 4, yl == 5],
                       [1.0, 2.0, 3.0, 3.0, 2.0]).astype(np.float32)
        lv = np.broadcast_to(lv, (h, w)).copy()
        lv = np.where((yl >= 2) & (strand > 0.72), lv + 1.0, lv)
        lv = np.where((yl >= 2) & (strand < 0.18), lv - 1.0, lv)
        lv = np.where((yl == 5) & (tip >= 1), 3.0, lv)
        lv = np.where((yl == 0) & (tip == 2), 2.0, lv)
        return lv.astype(np.float32)
    lv = _TILE_LUT[xs % 4, ys % 6]
    # 风化：零星几处苔斑、积灰（只落在瓦垄上，成片而不是逐点）
    wear = pix.value_noise(h, w, 7.0, seed + 3)
    lv = np.where((wear > 0.85) & ((xs % 4) != 0), lv - 1.0, lv)
    return lv.astype(np.float32)


# ---------------------------------------------------------------------------
# 一户
# ---------------------------------------------------------------------------
def column_runs(mask: np.ndarray, x: int):
    col = mask[:, x]
    y = 0
    H = len(col)
    while y < H:
        if not col[y]:
            y += 1
            continue
        y1 = y
        while y1 + 1 < H and col[y1 + 1]:
            y1 += 1
        yield y, y1
        y = y1 + 1


def open_below(sc: Scene, x: int, y: int) -> bool:
    """这一格下面是不是「看得见立面」的地方：可走、水面、篱笆道具。图外不算。"""
    if y >= sc.H:
        return False
    return bool(sc.walk[y, x] or sc.water[y, x] or sc.fence[y, x])


def door_score(ctx: Ctx, x: int, y: int) -> float:
    """门前这一格（立面南邻）有多像门口：传送点 > 设施/人/触发 > 路 > 空地。"""
    sc = ctx.sc
    if y >= sc.H:
        return 0.0
    s = 0.0
    for o in ctx.objects_at.get((x, y), []):
        s += {"portal": 5.0, "facility": 3.0, "npc": 2.0, "trigger": 2.0, "spawn": 1.5}.get(o.type, 0.0)
    if sc.md.ground[y, x] == 2:
        s += 1.0
    return s


def paint_roofed(ctx: Ctx, st: Structure) -> None:
    lots = st.lots
    ids = np.unique(lots[st.mask])
    for lid in ids:
        paint_lot(ctx, st, lots == lid, int(lid), len(ids) > 1)


def paint_lot(ctx: Ctx, st: Structure, lm: np.ndarray, lid: int, multi: bool) -> None:
    sc = ctx.sc
    below = ctx.below
    th = sc.th
    style = st.style
    seed = pix.salt_of(sc.md.map_id, "lot", st.sid, lid)
    # 屋顶群里一户一种瓦色（青灰 / 黛蓝 / 旧瓦），一栋栋才分得开；单栋的屋也一栋一种——
    # 镇上一排排铺面若全是同一片青灰，读起来是复制粘贴出来的
    vs = mm.ROOF_VARIANTS[style]
    if multi:
        var = vs[int(pix.hash01(lid, 1, seed) * len(vs))]
    else:
        var = vs[int(pix.hash01(st.sid, 2, pix.salt_of(sc.md.map_id, "roofvar")) * len(vs))]
    rid, tex = ctx.roof_tex(var)
    fstyle = fa.facade_style(th["facade"])
    if style == "black":
        fstyle = fa.facade_style("manor")
    elif style == "jade":
        fstyle = fa.facade_style("sect")

    # 嵌在立面上的门洞格（神手谷堂屋南墙那一格、墨府大门）并进来
    ext = lm.copy()
    doors = set()
    for g in sc.gaps:
        if g.orient != "h":
            continue
        xs_ = [c[0] for c in g.cells]
        y_ = g.cells[0][1]
        if lm[y_, xs_[0]] or (0 <= y_ - 1 and lm[y_ - 1, xs_[0]] and min(xs_) - 1 >= 0 and max(xs_) + 1 < sc.W
                              and lm[y_, min(xs_) - 1] and lm[y_, max(xs_) + 1]):
            for cx, cy in g.cells:
                ext[cy, cx] = True
                doors.add((cx, cy))

    ys, xs = np.nonzero(ext)
    x0, x1 = int(xs.min()), int(xs.max())

    cols = {}
    for x in range(x0, x1 + 1):
        for a, b in column_runs(ext, x):
            op = open_below(sc, x, b + 1)
            L = b - a + 1
            fh = (2 if L >= 3 else 1) if op else 0
            cols.setdefault(x, []).append((a, b, fh))

    has_north_eave = any(e.side == "north" and e.sid == st.sid and e.y == int(ys.min()) - 1
                         and e.x0 <= x0 + 1 and e.x1 >= x1 - 1 for e in sc.eaves)
    # 屋脊高低错落：每户的屋脊离屋顶上沿差几像素
    ridge_jit = int(pix.hash01(lid, 3, seed) * 5) - 2 if multi else 0

    def same_lot(x, y):
        return 0 <= x < sc.W and 0 <= y < sc.H and bool(ext[y, x])

    def other_lot(x, y):
        """左右邻是同一片屋顶群里的另一户（中间要一道山墙隔开）。"""
        return (0 <= x < sc.W and 0 <= y < sc.H and bool(st.mask[y, x]) and not ext[y, x]
                and st.lots is not None and st.lots[y, x] >= 0)

    ridge_cells = []
    for x, runs_ in cols.items():
        px = x * T
        for a, b, fh in runs_:
            top = a * T
            rb = (b - fh + 1) * T        # 屋顶下沿（不含）
            if rb <= top:                # 整列都是立面（一格厚的一截）：顶上画一道檐
                rb = top + 5
            below.paint_tex(px, top, np.ones((rb - top, T), bool), rid, tex)
            ctx.solid_px[top:(b + 1) * T, px:px + T] = True
            ctx.tall_px[top:(b + 1) * T, px:px + T] = True
            is_top = a == 0 or not same_lot(x, a - 1)
            R = rb - top
            ridge_y = top
            if not has_north_eave and is_top and R >= 24:
                # 屋脊落在上方三成处（每户差几像素），脊后一截缩短的后坡背光暗一档
                ridge_y = top + int(np.clip(int(R * 0.3) + ridge_jit, 5, 18))
                below.shade(px, top, -1.0, np.ones((ridge_y - top, T), bool))
                below.rect(px, top, T, 1, rid, 0)
            if not has_north_eave and is_top and R >= 8:
                ridge_cells.append((x, ridge_y))
            # 前坡不再做「近脊亮、近檐暗」的渐变：小数明度级整坡抖动，×3 下是满屋顶的棋盘噪点。
            # 坡度感交给屋脊、檐口两道硬线和后坡那一档暗。
            eave_line(below, px, rb, T, rid, var, seed)
            if fh == 0 and b + 1 < sc.H and sc.wall[b + 1, x]:
                # 下面是别的屋顶：檐影落在它的后坡上
                below.shade(px, (b + 1) * T, -2.0, np.ones((1, T), bool))
                below.shade(px, (b + 1) * T + 1, -1.0, np.ones((2, T), bool))

    # 屋脊：连成一道，两端上翘；少数人家脊中一只宝顶
    if ridge_cells:
        ridge_cells.sort()
        segs = [[ridge_cells[0]]]
        for cell in ridge_cells[1:]:
            if cell[0] == segs[-1][-1][0] + 1 and cell[1] == segs[-1][-1][1]:
                segs[-1].append(cell)
            else:
                segs.append([cell])
        for seg in segs:
            rx0, ry = seg[0][0] * T, seg[0][1]
            rx1 = (seg[-1][0] + 1) * T
            ridge(below, rx0, ry, rx1 - rx0, rid, var)
            if not var.startswith("thatch"):
                for side, xe in ((-1, rx0 + 1), (1, rx1 - 2)):
                    below.px(xe, ry - 1, rid, 3)
                    below.px(xe + side, ry - 2, rid, 4)
                    below.px(xe + side, ry - 1, rid, 1)
                if rx1 - rx0 >= 48 and pix.hash01(lid, 5, seed) < 0.35:
                    cxr = (rx0 + rx1) // 2
                    below.rect(cxr - 1, ry - 3, 3, 3, mm.gold() if style == "jade" else rid,
                               3 if style == "jade" else 4)

    # 大殿（宗门、府邸，宽 ≥8 格）：歇山顶——两端各一片斜下来的侧坡，屋脊收短
    width_cells = x1 - x0 + 1
    open_cols = [r for rs in cols.values() for r in rs if r[2] > 0]
    if style in ("jade", "black") and width_cells >= 8 and open_cols and len(cols) == width_cells:
        a0 = int(ys.min())
        bmode = max(r[1] for r in open_cols)
        fh0 = max(r[2] for r in open_cols)
        rb_h = (bmode - fh0 + 1) * T
        R0 = rb_h - a0 * T
        ry0 = (a0 - 1) * T + 5 if has_north_eave else a0 * T + int(np.clip(int(R0 * 0.3) + ridge_jit, 5, 18))
        inset = min(3 * T, width_cells * T // 4)
        hip_roof(below, x0 * T, (x1 + 1) * T, inset, ry0 + 4, rb_h - 3, max(a0 * T, ry0 + 4), rb_h - 3, rid)
        ctx.hips[st.sid] = (x0 * T, (x1 + 1) * T, inset, ry0, rb_h)

    # 两端：挨着同一片屋顶群的另一户 → 一道白山墙（马头墙）隔开；挨着巷子 → 博风
    for x, runs_ in cols.items():
        for a, b, fh in runs_:
            rb = (b - fh + 1) * T
            top = a * T
            if rb - top < 4:
                continue
            if x == x0 or not same_lot(x - 1, a):
                if other_lot(x - 1, a) or other_lot(x - 1, b):
                    firewall(below, x * T, top, rb, rid)
                else:
                    below.rect(x * T, top, 1, rb - top, rid, 0)
                    below.rect(x * T + 1, top + 1, 1, rb - top - 2, rid, 4 if not var.startswith("thatch") else 3)
                    corner_tip(below, x * T - 1, rb - 4, rid, var, -1)
            if x == x1 or not same_lot(x + 1, a):
                below.rect(x * T + T - 1, top, 1, rb - top, rid, 0)
                if not (other_lot(x + 1, a) or other_lot(x + 1, b)):
                    below.rect(x * T + T - 2, top + 1, 1, rb - top - 2, rid, 1)
                    corner_tip(below, x * T + T, rb - 4, rid, var, 1)

    if multi and sc.theme in ("town", "village", "dock", "manor"):
        roof_details(ctx, cols, lid, seed, rid, washing=not st.compound)

    # 立面：按「同一行收底、同样高」的连续列分段
    segs = []
    for x in sorted(cols):
        for a, b, fh in cols[x]:
            if fh == 0:
                continue
            if segs and segs[-1][2] == b and segs[-1][3] == fh and segs[-1][1] == x - 1:
                segs[-1][1] = x
            else:
                segs.append([x, x, b, fh])
    for xa, xb, b, fh in segs:
        if st.compound:
            paint_compound_face(ctx, xa, xb, b, fh, st, doors)
        else:
            paint_facade(ctx, xa, xb, b, fh, fstyle, doors, seed)


def firewall(cv: pix.Canvas, X: int, top: int, bottom: int, rid: int) -> None:
    """两户之间的山墙：从上往下看是一道窄窄的墙头——两侧一线暗、当中一溜压顶瓦（瓦缝一格一格）。
    屋顶群因此是一栋一栋的。（第一版整条画成白的，×3 放大后像一根根旗杆。）"""
    cv.rect(X - 1, top - 2, 1, bottom - top + 2, rid, 0)
    cv.rect(X, top - 2, 2, bottom - top + 2, rid, 3)
    cv.rect(X + 2, top - 2, 1, bottom - top + 2, rid, 1)
    for y in range(top - 2, bottom, 4):
        cv.px(X, y, rid, 5)
        cv.px(X + 1, y, rid, 4)
    cv.rect(X - 1, top - 3, 4, 1, rid, 4)


def roof_details(ctx: Ctx, cols: dict, lid: int, seed: int, rid: int, washing: bool = True) -> None:
    """屋顶上零星的人间烟火：天窗、烟囱、晾在竹竿上的衣裳。一户最多一样，多数什么也没有。"""
    cv = ctx.below
    xs = sorted(cols)
    if len(xs) < 3:
        return
    a, b, fh = cols[xs[len(xs) // 2]][0]
    top, rb = a * T, (b - fh + 1) * T
    if rb - top < 28:
        return
    h = pix.hash01(lid, 9, seed)
    cx = xs[len(xs) // 2] * T + int(pix.hash01(lid, 10, seed) * 8)
    cy = top + (rb - top) // 2 + 2
    if h < 0.14:
        # 天窗：暗的方口，一圈亮框
        cv.rect(cx, cy, 6, 4, mm.void(), 1)
        cv.rect(cx, cy, 6, 1, rid, 4)
        cv.rect(cx, cy, 1, 4, rid, 4)
        cv.rect(cx, cy + 4, 6, 1, rid, 0)
    elif h < 0.24:
        # 烟囱：一截砖垛，口子发黑，右下一小片影
        br = mm.brick()
        cv.rect(cx, cy - 8, 4, 7, br, 3)
        cv.rect(cx, cy - 8, 1, 7, br, 4)
        cv.rect(cx, cy - 9, 4, 1, mm.void(), 0)
        cv.shade(cx + 4, cy - 6, -1.0, np.ones((6, 2), bool))
    elif h < 0.32 and washing:
        # 晾衣竹竿：一根竿横在坡上，挂两三件（府邸的屋顶上不晾衣裳）
        b_ = mm.bamboo()
        x0 = xs[0] * T + 4
        x1 = xs[-1] * T + 10
        for x in range(x0, x1):
            cv.px(x, cy - 2, b_, 3)
        g = np.random.default_rng(seed + lid)
        for k in range(3):
            xx = x0 + 3 + k * max(5, (x1 - x0) // 3)
            if xx + 4 >= x1:
                break
            cl = mm.cloth(("indigo", "white", "ochre", "red")[int(g.integers(0, 4))])
            cv.rect(xx, cy - 1, 4, 5, cl, 3)
            cv.rect(xx, cy - 1, 4, 1, cl, 4)
            cv.shade(xx + 1, cy + 4, -1.0, np.ones((1, 4), bool))


def hip_roof(cv: pix.Canvas, X0: int, X1: int, inset: int, y_ridge: int, y_eave: int,
             y_from: int, y_to: int, rid: int) -> None:
    """歇山顶两端的侧坡与垂脊，只画 [y_from, y_to) 这几行（屋身在 below，屋脊那一段在 above，
    两边各画各的一截，按同一条斜线接上）。两片侧坡都暗一档（整档，不抖动），
    分得出「前坡 + 两片侧坡」三个面；左右的差别交给垂脊线（左亮右暗）。"""
    span = max(1, y_eave - y_ridge)
    for y in range(max(y_from, y_ridge), min(y_to, y_eave)):
        f = (y_eave - y) / span
        xl = int(round(X0 + inset * f))
        xr = int(round(X1 - 1 - inset * f))
        if xl > X0:
            cv.shade(X0, y, -1.0, np.ones((1, xl - X0), bool))
        if xr < X1 - 1:
            cv.shade(xr + 1, y, -1.0, np.ones((1, X1 - 1 - xr), bool))
        cv.px(xl, y, rid, 4)
        cv.px(xl + 1, y, rid, 1)
        cv.px(xr, y, rid, 3)
        cv.px(xr - 1, y, rid, 0)


def ridge(cv: pix.Canvas, px: int, top: int, w: int, rid: int, style: str) -> None:
    """屋脊（5px）：上沿一线暗、一线亮，两行脊身（隔几像素一道脊瓦缝），下沿一线暗。"""
    if style.startswith("thatch"):
        # 草顶的脊：一道压脊的草把，每隔几像素一根横绑的竹篾
        cv.rect(px, top, w, 1, rid, 0)
        cv.rect(px, top + 1, w, 3, rid, 2)
        cv.rect(px, top + 1, w, 1, rid, 3)
        cv.rect(px, top + 4, w, 1, rid, 0)
        for xx in range(px + ((-px) % 5), px + w, 5):
            cv.rect(xx, top + 1, 1, 3, mm.wood(), 1)
        return
    cv.rect(px, top, w, 1, rid, 0)
    cv.rect(px, top + 1, w, 1, rid, 5)
    cv.rect(px, top + 2, w, 2, rid, 3)
    cv.rect(px, top + 4, w, 1, rid, 0)
    for xx in range(px + ((-px) % 5), px + w, 5):
        cv.px(xx, top + 2, rid, 1)
        cv.px(xx, top + 3, rid, 1)


def eave_line(cv: pix.Canvas, px: int, rb: int, w: int, rid: int, style: str, seed: int) -> None:
    if style.startswith("thatch"):
        # 茅檐：参差的草梢，下沿一线深
        for i in range(w):
            x = px + i
            hang = int(pix.hash01(x, rb, seed) * 3)
            cv.rect(x, rb - 3, 1, 2 + hang, rid, 2 if (x % 2) else 3)
            cv.px(x, rb - 1 + hang, rid, 0)
        return
    for i in range(w):
        x = px + i
        xm = x % 4
        cv.px(x, rb - 3, rid, 5 if xm in (1, 2) else 1)
        cv.px(x, rb - 2, rid, 3 if xm in (1, 2) else 0)
        cv.px(x, rb - 1, rid, 0)


def corner_tip(cv: pix.Canvas, x: int, y: int, rid: int, style: str, side: int) -> None:
    """檐角上翘：往外 2px、往上 3px。茅草屋不翘。"""
    if style.startswith("thatch"):
        return
    cv.px(x, y + 2, rid, 1)
    cv.px(x + side, y + 1, rid, 2)
    cv.px(x + side, y, rid, 3)
    cv.px(x + 2 * side, y - 1, rid, 4)


# ---------------------------------------------------------------------------
# 立面
# ---------------------------------------------------------------------------
def paint_facade(ctx: Ctx, xa: int, xb: int, b: int, fh: int, style: dict, doors: set, seed: int) -> None:
    sc = ctx.sc
    cv = ctx.below
    X0, X1 = xa * T, (xb + 1) * T
    Y0, Y1 = (b - fh + 1) * T, (b + 1) * T
    wall = fa.rid(style["wall"])
    wl = style["wall_lv"]
    cv.rect(X0, Y0, X1 - X0, Y1 - Y0, wall, wl)
    # 墙面：墙根一道返潮（台基上方参差的一溜暗）+ 夯土的层线。
    # （上一版在下半墙随机撒暗斑，×3 下满墙脏点。）
    if style["wall"] == "earthwall":
        for yy in range(Y0 + 5, Y1, 4):
            cv.rect(X0, yy, X1 - X0, 1, wall, wl - 1)
    fa.damp(cv, X0, Y1 - 3, X1 - X0, pix.salt_of(sc.md.map_id, "damp"))
    # 檐下阴影：屋檐与墙交界处要看得出一道深影（瓦当压在墙头上）
    cv.shade(X0, Y0, -3.0, np.ones((1, X1 - X0), bool))
    cv.shade(X0, Y0 + 1, -2.0, np.ones((1, X1 - X0), bool))
    cv.shade(X0, Y0 + 2, -1.0, np.ones((2, X1 - X0), bool))
    beam = fa.rid(style["beam"])
    post = fa.rid(style["post"])
    pl = style["post_lv"]
    if fh == 2:
        cv.rect(X0, Y0 + 3, X1 - X0, 2, beam, max(0, pl - 1))
        cv.rect(X0, Y0 + 3, X1 - X0, 1, beam, pl)
    base = fa.rid(style["base"])
    fa.paint_plinth(cv, X0, Y1 - 3, X1 - X0, 3, base)

    # 门：门洞格优先；否则挑门前最像门口的那一格；太窄的一截（≤1 格）不开门
    ncols = xb - xa + 1
    gap_cols = [x for x in range(xa, xb + 1) if (x, b) in doors]
    door_cols: list[int] = []
    if gap_cols:
        door_cols = gap_cols
    elif ncols >= 2 or fh == 2:
        scores = [(door_score(ctx, x, b + 1), -abs(x - (xa + xb) / 2.0), x) for x in range(xa, xb + 1)]
        best = max(scores)
        door_cols = [best[2]]
    # 柱：两端各一根，中间每两格一根（避开门）
    post_x = [X0, X1 - 2]
    for x in range(xa + 2, xb, 2):
        if x not in door_cols and x - 1 not in door_cols:
            post_x.append(x * T - 1)
    for px_ in post_x:
        cv.rect(px_, Y0 + 2, 2, Y1 - Y0 - 5, post, pl)
        cv.rect(px_, Y0 + 2, 1, Y1 - Y0 - 5, post, pl + 1)
    # 窗
    for x in range(xa, xb + 1):
        if x in door_cols:
            continue
        if fh == 2:
            fa.paint_window(cv, x * T + 4, Y0 + 9, 8, 8, fa.rid(style["win_frame"]))
            ctx.lights.append({"kind": "window", "x": x + 0.5, "y": b - 0.5})
        elif ncols >= 2 and (x - xa) % 2 == 1:
            fa.paint_window(cv, x * T + 5, Y0 + 5, 6, 5, fa.rid(style["win_frame"]))
    # 门
    for x in door_cols:
        dw = 10 if fh == 2 else 8
        dh = 19 if fh == 2 else 10
        dx = x * T + (T - dw) // 2
        dy = Y1 - 3 - dh
        is_gap = (x, b) in doors
        fa.paint_door(cv, dx, dy, dw, dh, style, open_=is_gap)
        if is_gap:
            # 门洞格本身可走：门槛画成一道低矮的石条，一眼看得出跨得过去
            cv.rect(x * T + 2, Y1 - 3, T - 4, 3, base, 2)
            cv.rect(x * T + 2, Y1 - 3, T - 4, 1, base, 4)
        ctx.lights.append({"kind": "door", "x": x + 0.5, "y": b + 0.2})
    # 地面接触阴影由 mapshade 统一做


def paint_compound_face(ctx: Ctx, xa: int, xb: int, b: int, fh: int, st: Structure, doors: set) -> None:
    """院墙式的立面（墨府临街一面、独霸山庄外墙）：粉墙 / 石墙、台基，不开窗。
    墙上的门洞画成府门：黑漆对开大门、门钉、门槛，两侧一对石狮，门上一座门楼。"""
    from mapwall import wall_face
    for x in range(xa, xb + 1):
        for k in range(fh):
            if (x, b - k) in doors:
                continue
            wall_face(ctx, x, b - k, st.wall_style, coping=False, top_shadow=(k == fh - 1))
    gate_cols = sorted(x for x in range(xa, xb + 1) if (x, b) in doors)
    if gate_cols:
        grand_gate(ctx, gate_cols[0], gate_cols[-1], b, fh, st)


def grand_gate(ctx: Ctx, x0: int, x1: int, b: int, fh: int, st: Structure) -> None:
    from mapeaves import gate_roof
    cv = ctx.below
    X0, X1 = x0 * T, (x1 + 1) * T
    Y1 = (b + 1) * T
    Ytop = (b - fh + 1) * T
    door = mm.darkwood()
    red = mm.lacquer()
    gold = mm.gold()
    # 门框上方（门洞上面那一截墙）：门楣与匾
    if fh >= 2:
        cv.rect(X0 - 2, Ytop, X1 - X0 + 4, T, red, 2)
        fa.paint_plaque(cv, X0 + 4, Ytop + 5, X1 - X0 - 8)
    # 门：两扇黑漆门，每扇三行门钉；门缝一线；门槛
    top = Y1 - T - 2 if fh >= 2 else Y1 - T + 2
    cv.rect(X0, top, X1 - X0, Y1 - 3 - top, door, 1)
    mid = (X0 + X1) // 2
    cv.rect(mid, top, 1, Y1 - 3 - top, door, 0)
    for yy in range(top + 3, Y1 - 5, 4):
        for xx in list(range(X0 + 3, mid - 1, 4)) + list(range(mid + 3, X1 - 1, 4)):
            cv.px(xx, yy, gold, 3)
    cv.px(mid - 2, top + 8, gold, 4)
    cv.px(mid + 2, top + 8, gold, 4)
    for xx in (X0 - 2, X1):
        cv.rect(xx, Ytop, 2, Y1 - Ytop - 2, red, 3)
    base = mm.rock("cliff")
    cv.rect(X0 - 2, Y1 - 3, X1 - X0 + 4, 3, base, 3)
    cv.rect(X0 - 2, Y1 - 3, X1 - X0 + 4, 1, base, 4)
    # 石狮一对（蹲在门两侧的墙脚）
    for sx in (X0 - 12, X1 + 3):
        lion(cv, sx, Y1 - 12)
    gate_roof(ctx, X0 - 8, X1 + 8, Ytop + 3, "black" if st.style == "black" else st.style)


def lion(cv: pix.Canvas, x: int, y: int) -> None:
    """一只石狮：方座、蹲身、圆头（9×12）。"""
    st = mm.rock("cliff")
    cv.rect(x, y + 8, 9, 4, st, 3)
    cv.rect(x, y + 8, 9, 1, st, 4)
    cv.rect(x + 1, y + 3, 7, 5, st, 3)
    cv.rect(x + 1, y + 3, 1, 5, st, 4)
    cv.rect(x + 2, y, 5, 4, st, 4)
    cv.px(x + 3, y + 1, st, 1)
    cv.px(x + 5, y + 1, st, 1)
    cv.rect(x + 1, y + 11, 8, 1, st, 1)
