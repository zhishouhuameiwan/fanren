"""front 层里不是树冠的那些：屋脊那一段、檐口/布棚、门楼，以及崖檐松枝、伸出墙的树枝。
外加墙上的门洞：门框、门槛、门楼。

  * 北侧屋檐行（hall()/村屋那一行 front）：屋脊 + 半截瓦坡进 above.png，与 below.png 里
    的屋顶按绝对坐标接上——走到屋后的人被屋脊压住。
  * 南侧檐格（镇上「屋檐压在街上」、马厩前檐）：一道短檐或一幅布棚，只压住路面上沿
    几个像素，路仍一眼看得出能走；布棚下的路面烘一道影子。生成器是随机撒的格子，
    这里按「一段立面有四成以上撒到」整段画、否则不画——参差的檐口比没有更难看。
  * 门洞：横墙上的门洞两侧立柱、顶上一道门楣（或一座门楼），脚下一道门槛。
"""

from __future__ import annotations

import numpy as np

import mapfacade as fa
import mapmat as mm
import pix
from maprock import crowns
from mapwall import STYLES

T = 16


def roof_style_of(ctx, sid: int) -> str:
    st = ctx.sc.structs[sid]
    if st.kind == "roofed":
        return st.style
    if st.kind == "wall":
        cap = STYLES.get(st.style, {}).get("cap", "grey")
        return "grey" if cap == "void" else cap
    return "grey"


# ---------------------------------------------------------------------------
# 屋脊那一段（北侧屋檐行）
# ---------------------------------------------------------------------------
def ridge_strip(ctx, y: int, x0: int, x1: int, style: str, sid: int = -1) -> None:
    cv = ctx.above
    rid, tex = ctx.roof_tex(style)
    X0, X1 = x0 * T, (x1 + 1) * T
    Y = y * T
    ridge_y = Y + 5
    if sid in ctx.hips:
        hip_strip(ctx, y, sid, style)
        return
    # 瓦坡：从屋脊下沿一直到这一行底
    region = np.ones((Y + T - (ridge_y + 4), X1 - X0), bool)
    cv.paint_tex(X0, ridge_y + 4, region, rid, tex)
    if style == "thatch":
        cv.rect(X0 - 1, ridge_y, X1 - X0 + 2, 1, rid, 0)
        cv.rect(X0 - 1, ridge_y + 1, X1 - X0 + 2, 2, rid, 2)
        cv.rect(X0 - 1, ridge_y + 3, X1 - X0 + 2, 1, rid, 1)
        for xx in range(X0, X1, 3):
            cv.px(xx, ridge_y + 1, rid, 4)
        # 草顶两端圆收
        cv.rect(X0 - 1, ridge_y, 1, T - 5, rid, 1)
        cv.rect(X1, ridge_y, 1, T - 5, rid, 1)
        return
    cv.rect(X0 - 1, ridge_y, X1 - X0 + 2, 1, rid, 0)
    cv.rect(X0 - 1, ridge_y + 1, X1 - X0 + 2, 1, rid, 4)
    cv.rect(X0 - 1, ridge_y + 2, X1 - X0 + 2, 1, rid, 2)
    cv.rect(X0 - 1, ridge_y + 3, X1 - X0 + 2, 1, rid, 1)
    for xx in range(X0 + 2, X1 - 2, 6):
        cv.px(xx, ridge_y + 2, rid, 3)
    # 两端博风
    cv.rect(X0, ridge_y + 4, 1, Y + T - ridge_y - 4, rid, 0)
    cv.rect(X0 + 1, ridge_y + 4, 1, Y + T - ridge_y - 4, rid, 4)
    cv.rect(X1 - 1, ridge_y + 4, 1, Y + T - ridge_y - 4, rid, 0)
    cv.rect(X1 - 2, ridge_y + 4, 1, Y + T - ridge_y - 4, rid, 1)
    # 脊端：宗门与墨府是金 / 深色的鸱吻，别处是一道上翘
    orn = mm.gold() if style == "jade" else rid
    olv = 3 if style == "jade" else 1
    for side, xe in ((-1, X0 - 1), (1, X1)):
        cv.px(xe, ridge_y - 1, orn, olv)
        cv.px(xe + side, ridge_y - 2, orn, olv + 1)
        cv.px(xe + side, ridge_y - 3, orn, olv)
        cv.px(xe, ridge_y, orn, olv)
        cv.px(xe - side, ridge_y - 1, orn, olv - 1 if olv > 0 else 0)


def hip_strip(ctx, y: int, sid: int, style: str) -> None:
    """歇山大殿的屋脊那一段：正脊收短在两片侧坡之间，两端鸱吻，侧坡与垂脊接着 below 里那一截。"""
    from maproof import hip_roof
    cv = ctx.above
    rid, tex = ctx.roof_tex(style)
    X0, X1, inset, ry0, rb = ctx.hips[sid]
    Y = y * T
    # 侧坡在这一行里也有：整行都铺瓦，交给 hip_roof 压暗两端、画垂脊
    region = np.ones((Y + T - (ry0 + 4), X1 - X0), bool)
    cv.paint_tex(X0, ry0 + 4, region, rid, tex)
    hip_roof(cv, X0, X1, inset, ry0 + 4, rb - 3, ry0 + 4, Y + T, rid)
    rx0, rx1 = X0 + inset, X1 - inset
    cv.rect(rx0 - 1, ry0, rx1 - rx0 + 2, 1, rid, 0)
    cv.rect(rx0 - 1, ry0 + 1, rx1 - rx0 + 2, 1, rid, 4)
    cv.rect(rx0 - 1, ry0 + 2, rx1 - rx0 + 2, 1, rid, 2)
    cv.rect(rx0 - 1, ry0 + 3, rx1 - rx0 + 2, 1, rid, 1)
    orn = mm.gold() if style == "jade" else rid
    olv = 3 if style == "jade" else 1
    for side, xe in ((-1, rx0 - 2), (1, rx1 + 1)):
        for k in range(4):
            cv.px(xe + side * (k // 2), ry0 - k, orn, olv + (1 if k == 2 else 0))
        cv.px(xe - side, ry0 - 1, orn, max(0, olv - 1))


# ---------------------------------------------------------------------------
# 南侧檐口 / 布棚
# ---------------------------------------------------------------------------
def facade_segments(ctx, y_face: int, sid: int):
    """立面行 y_face 上属于结构 sid、南邻可走的连续段；有分户的按户再切。"""
    sc = ctx.sc
    st = sc.structs[sid]
    segs = []
    x = 0
    while x < sc.W:
        ok = st.mask[y_face, x] and y_face + 1 < sc.H and sc.walk[y_face + 1, x]
        if not ok:
            x += 1
            continue
        lot = st.lots[y_face, x] if st.lots is not None else 0
        x1 = x
        while (x1 + 1 < sc.W and st.mask[y_face, x1 + 1] and sc.walk[y_face + 1, x1 + 1]
               and (st.lots is None or st.lots[y_face, x1 + 1] == lot)):
            x1 += 1
        segs.append((x, x1))
        x = x1 + 1
    return segs


def lip(ctx, y: int, x0: int, x1: int, style: str) -> None:
    """一道短檐：压在立面下沿与路面上沿之间，瓦当朝下，路面上烘一道影。"""
    cv = ctx.above
    rid, tex = ctx.roof_tex(style)
    X0, X1 = x0 * T, (x1 + 1) * T
    Y = y * T
    region = np.ones((5, X1 - X0 + 2), bool)
    cv.paint_tex(X0 - 1, Y - 4, region, rid, tex)
    cv.rect(X0 - 1, Y - 5, X1 - X0 + 2, 1, rid, 0)
    if style == "thatch":
        for xx in range(X0 - 1, X1 + 1):
            hang = int(pix.hash01(xx, Y, 5) * 3)
            cv.rect(xx, Y + 1, 1, 1 + hang, rid, 2)
            cv.px(xx, Y + 2 + hang, rid, 0)
    else:
        for xx in range(X0 - 1, X1 + 1):
            xm = xx % 4
            cv.px(xx, Y + 1, rid, 4 if xm in (1, 2) else 1)
            cv.px(xx, Y + 2, rid, 3 if xm in (1, 2) else 0)
            cv.px(xx, Y + 3, rid, 0)
    sh = np.ones((5, X1 - X0), bool)
    ctx.below.shade(X0, Y + 3, -1.0, sh)
    ctx.below.shade(X0 + 2, Y + 8, -1.0, np.ones((1, X1 - X0 - 2), bool))


def awning(ctx, y: int, x0: int, x1: int, seed: int) -> None:
    """布棚：条纹布，下沿波浪；棚下路面一道影。"""
    cv = ctx.above
    kinds = ("indigo", "ochre", "white", "red", "indigo")
    kind = kinds[int(pix.hash01(x0, y, seed) * len(kinds))]
    cloth = mm.cloth(kind)
    X0, X1 = x0 * T + 1, (x1 + 1) * T - 1
    Y = y * T
    cv.rect(X0, Y - 4, X1 - X0, 1, cloth, 0)
    for xx in range(X0, X1):
        stripe = ((xx - X0) // 3) % 2
        cv.rect(xx, Y - 3, 1, 6, cloth, 3 if stripe else 2)
        cv.px(xx, Y - 3, cloth, 4 if stripe else 3)
        scal = (xx - X0) % 4
        if scal in (1, 2):
            cv.px(xx, Y + 3, cloth, 1 + stripe)
        cv.px(xx, Y + 3 + (1 if scal in (1, 2) else 0), cloth, 0)
    ctx.below.shade(X0, Y + 4, -1.0, np.ones((4, X1 - X0), bool))
    ctx.below.shade(X0 + 2, Y + 8, -1.0, np.ones((1, X1 - X0 - 2), bool))


def paint_eaves(ctx) -> None:
    sc = ctx.sc
    mid = sc.md.map_id
    seed = pix.salt_of(mid, "eave")
    by_face: dict[tuple[int, int], set] = {}
    for e in sc.eaves:
        if e.pid >= 0:
            continue   # 压在道具上的（床帐、书架顶、亭顶）由道具自己画
        if e.side == "north":
            ridge_strip(ctx, e.y, e.x0, e.x1, roof_style_of(ctx, e.sid), e.sid)
        else:
            by_face.setdefault((e.y, e.sid), set()).update(range(e.x0, e.x1 + 1))
    for (y, sid), cells in by_face.items():
        style = roof_style_of(ctx, sid)
        for xa, xb in facade_segments(ctx, y - 1, sid):
            cover = sum(1 for x in range(xa, xb + 1) if x in cells) / (xb - xa + 1)
            if cover < 0.4:
                continue
            if sc.theme == "town" and pix.hash01(xa, y, seed) < 0.6:
                awning(ctx, y, xa, xb, seed)
            else:
                lip(ctx, y, xa, xb, style)


# ---------------------------------------------------------------------------
# 门洞与城门
# ---------------------------------------------------------------------------
def post(cv, x: int, y: int, h: int, rid: int, lv: int) -> None:
    cv.rect(x, y, 3, h, rid, lv)
    cv.rect(x, y, 1, h, rid, lv + 1)
    cv.rect(x + 2, y, 1, h, rid, max(0, lv - 1))


def gate_roof(ctx, X0: int, X1: int, Y: int, style: str) -> None:
    """门楼：一座小屋顶横跨门洞，两端翘角。Y 是屋顶下沿。"""
    cv = ctx.above
    rid, tex = ctx.roof_tex(style)
    top = Y - 9
    w = X1 - X0
    cv.paint_tex(X0, top + 3, np.ones((5, w), bool), rid, tex)
    cv.rect(X0 + 2, top, w - 4, 1, rid, 0)
    cv.rect(X0 + 1, top + 1, w - 2, 1, rid, 4)
    cv.rect(X0 + 1, top + 2, w - 2, 1, rid, 1)
    for xx in range(X0, X1):
        xm = xx % 4
        cv.px(xx, top + 8, rid, 4 if xm in (1, 2) else 1)
        cv.px(xx, top + 9, rid, 0)
    for side, xe in ((-1, X0 - 1), (1, X1)):
        cv.px(xe, top + 7, rid, 1)
        cv.px(xe + side, top + 6, rid, 2)
    if style == "jade":
        g = mm.gold()
        cv.px(X0 + 1, top - 1, g, 3)
        cv.px(X1 - 2, top - 1, g, 3)


def paint_gaps(ctx) -> None:
    sc = ctx.sc
    th = sc.th
    cv = ctx.below
    for g in sc.gaps:
        xs = [c[0] for c in g.cells]
        ys = [c[1] for c in g.cells]
        if g.orient == "h":
            y = ys[0]
            xl, xr = min(xs) - 1, max(xs) + 1
            wall_l = sc.wall[y, xl] if 0 <= xl < sc.W else False
            fence_l = sc.fence[y, xl] if 0 <= xl < sc.W else False
            stacked = (y + 1 < sc.H and sc.gap_at[y + 1, xs[0]] >= 0)
            if stacked:
                continue   # 两层厚的门洞只在靠南那一层画
            X0, X1 = min(xs) * T, (max(xs) + 1) * T
            Y = y * T
            if fence_l:
                # 篱笆门：两根高一点的木柱
                wd = mm.wood()
                post(cv, X0 - 2, Y - 6, 20, wd, 2)
                post(cv, X1 - 1, Y - 6, 20, wd, 2)
                ctx.solid_px[Y - 6:Y + 14, X0 - 2:X0 + 1] = True
                continue
            if not wall_l:
                continue
            sid = sc.sid[y, xl]
            st = sc.structs[sid] if sid >= 0 else None
            style_name = st.style if st is not None else th["wall"]
            if st is not None and st.kind == "roofed":
                continue   # 屋上的门洞由立面画
            s = STYLES.get(style_name, STYLES.get(th["wall"]))
            indoor = th["kind"] in ("indoor", "cave")
            prid = mm.lacquer() if s.get("cap") in ("jade",) or s.get("hall") == "sect" else (
                mm.darkwood() if s.get("cap") == "black" or indoor else mm.wood())
            depth = 1 + sum(1 for k in range(1, 3) if y - k >= 0 and sc.gap_at[y - k, xs[0]] >= 0)
            h = T * depth
            post(cv, X0 - 3, Y - T * (depth - 1), h - 2, prid, 2)
            post(cv, X1, Y - T * (depth - 1), h - 2, prid, 2)
            # 门槛：一道低矮的石条（跨得过去）
            base = mm.rock("cave" if th["kind"] == "cave" else "cliff")
            cv.rect(X0, Y + T - 3, X1 - X0, 3, base, 2)
            cv.rect(X0, Y + T - 3, X1 - X0, 1, base, 4)
            if th["kind"] == "cave":
                continue
            big = y in (0, sc.H - 1) or depth >= 2
            if big and not indoor:
                cap = s.get("cap", "grey")
                gate_roof(ctx, X0 - 6, X1 + 6, Y - T * (depth - 1) + 2, "grey" if cap in ("void",) else cap)
            else:
                # 门楣：一道横木压在门洞上沿（进 above，人走过去从门楣下钻过）
                ctx.above.rect(X0 - 3, Y - T * (depth - 1) - 1, X1 - X0 + 6, 3, prid, 2)
                ctx.above.rect(X0 - 3, Y - T * (depth - 1) - 1, X1 - X0 + 6, 1, prid, 3)
        else:
            x = xs[0]
            y0, y1 = min(ys), max(ys)
            base = mm.rock("cave" if th["kind"] == "cave" else "cliff")
            for y in range(y0, y1 + 1):
                cv.rect(x * T + 7, y * T, 2, T, base, 3)
                cv.rect(x * T + 7, y * T, 1, T, base, 4)
    paint_wall_gates(ctx)


def paint_wall_gates(ctx) -> None:
    """传送点紧贴着一堵墙（七玄门那两处：墙上没挖洞，出口就在墙根）：墙上画一道门。"""
    sc = ctx.sc
    cv = ctx.below
    for o in sc.md.objects:
        if o.type != "portal":
            continue
        cells = list(o.cells())
        if any(sc.gap_at[y, x] >= 0 for x, y in cells if 0 <= x < sc.W and 0 <= y < sc.H):
            continue
        for dx, dy in ((0, 1), (0, -1)):
            wallcells = [(x + dx, y + dy) for x, y in cells
                         if 0 <= y + dy < sc.H and sc.wall[y + dy, x + dx] and sc.walk[y, x]]
            if len(wallcells) != len(cells):
                continue
            xs = [c[0] for c in wallcells]
            yw = wallcells[0][1]
            X0, X1 = min(xs) * T, (max(xs) + 1) * T
            style = fa.facade_style("sect" if sc.theme == "sect" else "town")
            fa.paint_door(cv, X0 + 2, yw * T + 1, X1 - X0 - 4, 13, style)
            if yw == 0:
                gate_roof(ctx, X0 - 6, X1 + 6, yw * T + 3,
                          "jade" if sc.theme == "sect" else "grey")


# ---------------------------------------------------------------------------
# 伸出来的树枝、崖檐松枝、藤、洞顶垂石
# ---------------------------------------------------------------------------
def bough(ctx, x: int, y: int, seed: int) -> None:
    """墙里伸出来的一枝：一根斜枝，枝上挂两三团叶；影子落在路面上。
    （第一版把整格顶部填满树冠，远看是一道道色带。）"""
    sc = ctx.sc
    cv = ctx.above
    g = np.random.default_rng(seed)
    if sc.season == "autumn":
        kind = "autumn" if g.random() < 0.5 else ("dark" if sc.time == "night" else "broad")
    else:
        kind = "dark" if sc.time == "night" else "broad"
    rid = mm.foliage(kind)
    bark = mm.bark()
    px, py = x * T, y * T
    if y > 0 and sc.wall[y - 1, x]:
        py -= 10   # 北边是墙：枝子从墙头探出来，挂在墙面上半截，不是长在路边的矮树丛
    bx = px + int(g.integers(0, 6))
    for k in range(10):
        cv.px(bx + k, py - 2 + k // 3, bark, 1 if k % 2 else 2)
    shadow = np.zeros((ctx.below.h, ctx.below.w), bool)
    for _ in range(int(g.integers(2, 4))):
        cx = px + int(g.integers(2, 15))
        cy = py + int(g.integers(-2, 5))
        r = 2.5 + g.random() * 2.0
        n = int(np.ceil(r)) + 1
        yy, xx = np.mgrid[-n:n + 1, -n:n + 1].astype(np.float32)
        d = np.sqrt(xx * xx + yy * yy) + (pix.hash01((xx + cx).astype(np.int64), (yy + cy).astype(np.int64), seed) - 0.5)
        disc = d <= r
        lv = np.clip(np.round(3.0 - (xx + yy) / r * 1.1), 1, 5)
        lv = np.where(d > r - 1.0, 0.0, lv)
        cv.paint(cx - n, cy - n, disc, rid, lv.astype(np.float32))
        sy, sx = cy + 7, cx + 3
        if 0 <= sy < shadow.shape[0] - n and 0 <= sx < shadow.shape[1] - n and sy - n >= 0 and sx - n >= 0:
            shadow[sy - n:sy + n + 1, sx - n:sx + n + 1] |= disc
    ctx.below.shade(0, 0, -1.0, shadow & ~ctx.solid_px)


def pine_spray(ctx, x: int, y: int, seed: int, face: bool) -> None:
    """崖檐松枝：从崖沿斜伸出去的一根枝，枝上挂两三簇针叶（每簇一团，左上亮、右下一线暗），
    枝梢微微上挑。（上一版是一片压扁的椭圆描一圈暗边，×3 下像一顶顶扁帽子浮在崖上。）"""
    cv = ctx.above
    rid = mm.foliage("pine")
    bark = mm.bark()
    g = np.random.default_rng(seed)
    side = 1 if g.random() < 0.5 else -1
    x0 = x * T + (int(g.integers(1, 5)) if side > 0 else int(g.integers(11, 15)))
    y0 = y * T + (int(g.integers(9, 12)) if face else int(g.integers(5, 10)))
    L = 10 + int(g.integers(0, 5))
    pts = [(x0 + side * k, y0 - (k * k) // 45) for k in range(L)]
    for k, (bx, by) in enumerate(pts):
        cv.px(bx, by, bark, 1)
        if k < L // 2:
            cv.px(bx, by - 1, bark, 2)
    for frac, r in ((0.4, 3.0 + g.random() * 0.6), (0.72, 3.2 + g.random() * 0.8), (1.0, 2.4 + g.random() * 0.6)):
        cx, cy = pts[int(frac * (L - 1))]
        cy -= 1
        n = int(np.ceil(r)) + 1
        yy, xx = np.mgrid[-n:n + 1, -n:n + 1].astype(np.float32)
        wob = (pix.hash01((xx + cx).astype(np.int64), (yy + cy).astype(np.int64), seed) - 0.5) * 1.4
        d = np.sqrt(xx * xx + (yy * 1.25) ** 2) + wob
        disc = d <= r
        lv = np.clip(np.round(3.2 - (xx + yy) / r * 1.2), 1, 4)
        needle = pix.hash01((xx + cx).astype(np.int64), (yy + cy).astype(np.int64), seed + 1) < 0.25
        lv = np.where(needle & (lv >= 2) & (lv < 4), lv + 1, lv)
        lv = np.where((d > r - 1.0) & (xx + yy > 0), 0.0, lv)
        cv.paint(cx - n, cy - n, disc, rid, lv.astype(np.float32))


def vines(ctx, x: int, y: int, seed: int) -> None:
    """藤 / 垂草：从崖沿垂下几缕。颜色取草色而不是树叶色——秋天的红叶藤在土坡上是一道道红线，扎眼。"""
    cv = ctx.above
    rid = mm.grass(ctx.sc.season)
    g = np.random.default_rng(seed)
    for _ in range(int(g.integers(3, 6))):
        vx = x * T + int(g.integers(1, 15))
        top = y * T + int(g.integers(0, 5))
        ln = int(g.integers(6, 14))
        for k in range(ln):
            xx = vx + (1 if (k // 3) % 2 else 0)
            cv.px(xx, top + k, rid, 2 if k % 3 else 4)
            if k % 4 == 2:
                cv.px(xx - 1, top + k, rid, 3)


def drips(ctx, x: int, y: int, seed: int) -> None:
    """洞顶垂下的石钟乳：几根 2–5px 的尖。"""
    cv = ctx.above
    rid = mm.rock("cave")
    g = np.random.default_rng(seed)
    for _ in range(int(g.integers(2, 4))):
        dx = x * T + int(g.integers(2, 14))
        ln = int(g.integers(2, 6))
        base = y * T + T - 2
        for k in range(ln):
            cv.px(dx, base + k, rid, 3 if k == 0 else 2)
            if k < ln - 2:
                cv.px(dx + 1, base + k, rid, 1)


def paint_decos(ctx) -> None:
    sc = ctx.sc
    mid = sc.md.map_id
    for x, y in sc.boughs:
        bough(ctx, x, y, pix.seed_of(mid, "bough", x, y))
    for x, y in sc.walldeco:
        sid = sc.sid[y, x]
        if sid < 0:
            continue
        st = sc.structs[sid]
        s = pix.seed_of(mid, "wdeco", x, y)
        face = y + 1 < sc.H and not sc.wall[y + 1, x]
        if st.kind == "rock":
            if st.style == "cave":
                drips(ctx, x, y, s)
            elif st.style in ("valley", "earth"):
                vines(ctx, x, y, s)
            elif ctx.rock_top.get(sid) == "forest":
                continue   # 林子的边已经是伸出来的树冠（maprock.edge_canopy），再挂松枝就是一堆浮在林冠上的绿条
            else:
                pine_spray(ctx, x, y, s, face)
        elif st.kind == "wall" and sc.theme == "cave" and face:
            drips(ctx, x, y, s)
