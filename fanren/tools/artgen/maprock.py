"""山石：崖壁、谷壁、土坡、洞壁，以及落日峰四周的崖外云海。

判据：岩格南邻可走（或是水、篱笆）→ 崖面（立面，岩层横纹），岩体纵向 ≥3 格时崖面
取两格高；其余是岩顶，按主题画成松林（山道、野外）、石坡疏草（炼骨崖、崖壁）、草坡
（谷壁、渡口土坡）或洞顶暗部（暗道）。

岩顶挨着可走格的那几条边：北边是坡顶（一线亮 + 几簇草探出来），西边是受光面（一线亮），
东边交给统一投影（mapshade）去压暗旁边的地面——左上光，影子往右下落。
"""

from __future__ import annotations

import numpy as np

import mapground as mg
import mapmat as mm
import pix
from mapinfer import Structure, runs

T = 16


MAX_FACE = {"cliff": 3, "dusk": 3, "valley": 2, "earth": 2, "cave": 2}


def face_rows(sc, m: np.ndarray, max_fh: int):
    """每格岩体的角色：1 = 崖面，0 = 岩顶。崖面格另记它在崖面里的第几行（0 顶行）。

    崖面高度随岩体厚度走，最高 max_fh 格：两格（32px）的崖面只比人高一点，站在崖下
    看不出「高」；三格高的崖面一眼就是一堵走不上去的石壁。岩体至少留一格岩顶。
    """
    H, W = m.shape
    role = np.zeros((H, W), np.int8)
    row_in_face = np.zeros((H, W), np.int8)
    fh_of = np.zeros((H, W), np.int8)
    for x in range(W):
        y = 0
        while y < H:
            if not m[y, x]:
                y += 1
                continue
            y1 = y
            while y1 + 1 < H and m[y1 + 1, x]:
                y1 += 1
            below = y1 + 1
            opened = below < H and (sc.walk[below, x] or sc.water[below, x] or sc.fence[below, x])
            if opened:
                L = y1 - y + 1
                fh = 1 if L <= 2 else min(max_fh, L - 1)
                for k in range(fh):
                    role[y1 - k, x] = 1
                    row_in_face[y1 - k, x] = fh - 1 - k
                    fh_of[y1 - k, x] = fh
            y = y1 + 1
    return role, row_in_face, fh_of


def tex_face(h: int, w: int, seed: int) -> np.ndarray:
    """崖面：一根根竖向的岩柱（宽 5–11px，柱缝最暗、左沿受光、右沿背光），
    柱面上 4px 一层的横向岩层——节理分明的石壁，而不是一张横条纹布。"""
    xs = np.arange(w)
    g = np.random.default_rng(seed)
    cuts = [-int(g.integers(0, 10))]
    while cuts[-1] < w:
        cuts.append(cuts[-1] + int(g.integers(5, 12)))
    cuts = np.array(cuts)
    cid = np.searchsorted(cuts, xs, side="right") - 1
    xin = xs - cuts[cid]
    cw = cuts[np.minimum(cid + 1, len(cuts) - 1)] - cuts[cid]
    tone = pix.hash01(cid, 1, seed)
    base = 2.0 + np.where(tone < 0.25, -1.0, 0.0) + np.where(tone > 0.8, 1.0, 0.0)
    ys = np.arange(h)[:, None]
    shift = (pix.hash01(cid, 2, seed) * 5).astype(np.int64)[None, :]
    band = (ys + shift) // 4
    bt = pix.hash01(band, cid[None, :], seed + 3)
    lv = base[None, :] + np.where(bt < 0.2, -1.0, 0.0)
    lv = np.where(((ys + shift) % 4) == 0, lv - 1.0, lv)
    lv = np.where(xin[None, :] == 0, 0.0, lv)
    lv = np.where(xin[None, :] == 1, lv + 1.0, lv)
    lv = np.where(xin[None, :] == (cw - 1)[None, :], lv - 1.0, lv)
    return np.clip(np.broadcast_to(lv, (h, w)), 0, 5).astype(np.float32).copy()


def tex_face_earth(h: int, w: int, seed: int, face_px: np.ndarray) -> np.ndarray:
    """土坡的坡面（渡口两道黄土坡）：上亮下暗的斜坡，缓缓起伏的横向土层，零星石子与草根。
    不用岩柱——土坡画成一根根竖条，远看像一排木栅栏。"""
    ys = np.arange(h)[:, None].astype(np.float32)
    xs = np.arange(w)[None, :]
    # 这一像素在坡面里往下第几像素：用于上亮下暗
    depth = np.zeros((h, w), np.float32)
    cur = face_px & ~pix.shift(face_px, 0, 1)
    k = 0
    while cur.any() and k < 64:
        depth[cur] = k
        cur = pix.shift(cur, 0, 1) & face_px
        k += 1
    wav = (pix.value_noise(1, w, 11.0, seed)[0] * 3).astype(np.int64)[None, :]
    band = (ys.astype(np.int64) + wav) // 5
    t = pix.hash01(band, xs // 13, seed + 1)
    lv = 3.0 - np.floor(depth / 12.0) + np.where(t < 0.2, -1.0, 0.0)
    lv = np.where(((ys.astype(np.int64) + wav) % 5) == 0, lv - 1.0, lv)
    xs_, ys_ = mg.jitter_points(h, w, 6, 0.35, seed + 2)
    mg.stamp(lv, xs_, ys_, mg.PEBBLE)
    return np.clip(lv, 0, 5).astype(np.float32)


def rim_offset(w: int, seed: int) -> np.ndarray:
    """崖顶沿的参差：每一小段（3–6px）往下缩 0–3px，崖沿因此不是一条直线。"""
    g = np.random.default_rng(seed)
    out = np.zeros(w, np.int64)
    x = 0
    while x < w:
        span = int(g.integers(3, 7))
        out[x:x + span] = int(g.integers(0, 4))
        x += span
    return out


def relief_shade(hgt: np.ndarray, k: float) -> np.ndarray:
    """高度场 → 左上光的明暗：朝左上（高度往右下升高）的面亮。"""
    dx = hgt - pix.shift(hgt, 1, 0, fill=0.0)
    dy = hgt - pix.shift(hgt, 0, 1, fill=0.0)
    dx[:, 0] = 0
    dy[0, :] = 0
    return (dx + dy) * k


def tex_rocky_top(h: int, w: int, seed: int, cell: int = 20) -> np.ndarray:
    """岩顶：大大小小隆起的岩块，按左上光打明暗，块与块之间的缝最暗。

    第一版用一种大小（11px）的岩块，排得匀，远看像一地卵石铺的路——而岩顶是挡路的，
    像路就是大错。现在是大块（20px）为主、小块（8px）为辅，高度场再叠一层缓起伏，
    明暗拉开到 4 级：受光面亮、背光面暗，看得出是一堆堆凸起的山石。
    """
    bid, edge, dist = mg.voronoi_full(h, w, cell, seed + 3)
    size = 0.75 + pix.hash01(bid, 5, seed) * 0.5
    dome = np.clip(1.0 - (dist / (cell * 0.72 * size)) ** 2, 0.0, 1.0)
    bid2, edge2, dist2 = mg.voronoi_full(h, w, 8, seed + 4)
    dome2 = np.clip(1.0 - (dist2 / 6.0) ** 2, 0.0, 1.0)
    big = pix.fbm(h, w, seed + 7, ((34, 0.6), (16, 0.4)))
    hgt = dome * 1.3 + dome2 * 0.35 + big * 1.4
    sh = relief_shade(hgt, 6.0)
    lv = 3.0 + np.clip(np.round(sh), -2, 1)
    lv = np.where(edge < 1.4, 0.0, lv)
    lv = np.where((edge >= 1.4) & (edge < 2.6), np.minimum(lv, 2.0), lv)
    lv = np.where((edge2 < 1.0) & (dome < 0.5), lv - 1.0, lv)
    return np.clip(lv, 0, 5).astype(np.float32)


def tex_hill(h: int, w: int, seed: int) -> np.ndarray:
    """草坡的起伏：一张缓的高度场，只给 ±1 级的明暗（草色另贴）。"""
    big = pix.fbm(h, w, seed + 9, ((26, 0.6), (12, 0.4)))
    return np.clip(np.round(relief_shade(big, 9.0)), -1, 1).astype(np.float32)


def crowns(cv: pix.Canvas, region: np.ndarray, seed: int, kind: str, step: int = 9,
           rmin: float = 4.5, rmax: float = 7.5, fill_bg: bool = True) -> None:
    """一片林冠（从上往下看的密林）：错位网格上一团团树冠，南边的压在北边的上面，
    每团左上亮、右下暗，团与团之间的缝最暗。region 是允许画的像素区。"""
    H, W = region.shape
    rid = mm.foliage(kind)
    ys_r, xs_r = np.nonzero(region)
    if len(xs_r) == 0:
        return
    y0, y1, x0, x1 = ys_r.min(), ys_r.max(), xs_r.min(), xs_r.max()
    sub = region[y0:y1 + 1, x0:x1 + 1]
    if fill_bg:
        cv.paint(int(x0), int(y0), sub, rid, 0.0)
    g = np.random.default_rng(seed)
    pts = []
    for gy in range(int(y0) - step, int(y1) + step, step):
        for gx in range(int(x0) - step, int(x1) + step, step):
            cx = gx + g.random() * step
            cy = gy + g.random() * step
            r = rmin + g.random() * (rmax - rmin)
            pts.append((cy, cx, r))
    pts.sort()
    for cy, cx, r in pts:
        n = int(np.ceil(r)) + 1
        yy, xx = np.mgrid[-n:n + 1, -n:n + 1].astype(np.float32)
        wob = (pix.hash01((xx + cx).astype(np.int64), (yy + cy).astype(np.int64), seed) - 0.5) * 1.6
        d = np.sqrt(xx * xx + yy * yy) + wob
        disc = d <= r
        if not disc.any():
            continue
        # 球面受光：左上亮
        nx, ny = xx / r, yy / r
        shade = -(nx + ny) * 1.1 + (1.0 - d / r) * 0.6
        lv = np.clip(np.round(2.8 + shade), 1, 5)
        lv = np.where(d > r - 1.0, 1.0, lv)   # 团边一线暗，团与团分得开
        leaf = pix.hash01((xx + cx).astype(np.int64) // 2, (yy + cy).astype(np.int64) // 2, seed + 7) < 0.22
        lv = np.where(leaf & (lv >= 2), lv + 1, lv)
        px, py = int(round(cx)) - n, int(round(cy)) - n
        # 只画在 region 里
        cl = cv._clip(px, py, disc.shape[0], disc.shape[1])
        if cl is None:
            continue
        sy, sx, my, mx = cl
        ok = disc[my, mx] & region[sy, sx]
        tmp = np.zeros_like(disc)
        tmp[my, mx] = ok
        cv.paint(px, py, tmp, rid, lv.astype(np.float32))


def tex_plateau(h: int, w: int, seed: int) -> np.ndarray:
    """岩顶台面：低对比的石面（大片一档、零星暗斑与石子）。台面是远处的、挡路的，
    不能比路还抢眼——第一版把它画成一块块凸起的圆石，远看是一地卵石路。"""
    lv = np.full((h, w), 3.0, np.float32)
    d = mg.density_field(h, w, seed, (22, 9))
    # 明暗斑都只留零星几块：三档斑块铺满台面，×3 下像迷彩
    lv -= (d < 0.24) * 1.0
    lv += (d > 0.8) * 1.0
    xs, ys = mg.jitter_points(h, w, 6, 0.3, seed + 1)
    mg.stamp(lv, xs, ys, mg.PEBBLE)
    crack = pix.value_noise(h, w, 3.0, seed + 2) > 0.87
    lv = np.where(crack, lv - 1.0, lv)
    return lv


def scatter_blades(cv: pix.Canvas, region: np.ndarray, seed: int, season: str, density: np.ndarray) -> None:
    """在岩面上零星长草：按密度场逐点撒草叶（不是一整块贴草地）——边缘自然是疏的。"""
    H, W = region.shape
    grass = mm.grass(season)
    xs, ys = mg.jitter_points(H, W, 2, 0.8, seed)
    keep = region[ys, xs] & (pix.hash01(xs, ys, seed + 1) < density[ys, xs])
    xs, ys = xs[keep], ys[keep]
    lvl = np.where(pix.hash01(xs, ys, seed + 2) < 0.3, 4.0, 3.0)
    for x, y, l in zip(xs, ys, lvl):
        cv.px(int(x), int(y), grass, float(l))
        if y + 1 < H and region[y + 1, x]:
            cv.px(int(x), int(y) + 1, grass, 1.0)


def bushes(cv: pix.Canvas, region: np.ndarray, seed: int, kind: str, step: int = 18, prob: float = 0.4) -> None:
    """台面上一丛丛分开的灌木 / 矮松：每丛两三团小冠，底下一小片影子（落在右下）。
    一丛一丛分得开、有影子，远看才是「长在上面的东西」而不是地上的一块黑斑。"""
    H, W = region.shape
    rid = mm.foliage(kind)
    xs, ys = mg.jitter_points(H, W, step, prob, seed)
    inner = pix.erode(region, 4)
    keep = inner[ys, xs]
    g = np.random.default_rng(seed + 1)
    for x, y in zip(xs[keep], ys[keep]):
        sh = pix.ellipse(12, 6)
        cv.shade(int(x) - 3, int(y) + 2, -1.0, sh)
        for k in range(int(g.integers(2, 4))):
            cx = x + g.integers(-3, 4)
            cy = y - k * 2 + g.integers(-1, 2)
            r = 2.5 + g.random() * 2.0
            n = int(np.ceil(r)) + 1
            yy, xx = np.mgrid[-n:n + 1, -n:n + 1].astype(np.float32)
            d = np.sqrt(xx * xx + yy * yy)
            disc = d <= r
            lv = np.clip(np.round(3.2 - (xx + yy) / r * 1.1), 1, 5)
            lv = np.where(d > r - 1.0, 1.0, lv)
            cv.paint(int(cx) - n, int(cy) - n, disc, rid, lv.astype(np.float32))


def edge_canopy(ctx, top_px: np.ndarray, walkpx: np.ndarray, seed: int, kind: str) -> None:
    """林子边上的树冠伸到路上方（above）：路两边是林子时，路才像是从林子里穿过去的。
    北沿（林子在路南）也要：不然林冠在格线上齐刷刷切一刀，像一块绿地毯的边。"""
    H, W = top_px.shape
    edge = top_px & (pix.shift(walkpx, 1, 0) | pix.shift(walkpx, -1, 0) | pix.shift(walkpx, 0, 1))
    # 冠心落在林缘往里 3px 的带子里（只取一像素宽的边线，错位网格的点几乎落不上去）
    band = pix.dilate(edge, 3) & top_px
    rid = mm.foliage(kind)
    xs, ys = mg.jitter_points(H, W, 6, 0.8, seed)
    keep = band[ys, xs]
    g = np.random.default_rng(seed + 1)
    for x, y in zip(xs[keep], ys[keep]):
        r = 3.0 + g.random() * 2.5
        n = int(np.ceil(r)) + 1
        yy, xx = np.mgrid[-n:n + 1, -n:n + 1].astype(np.float32)
        d = np.sqrt(xx * xx + yy * yy) + (pix.hash01((xx + x).astype(np.int64), (yy + y).astype(np.int64), seed) - 0.5)
        disc = d <= r
        lv = np.clip(np.round(3.0 - (xx + yy) / r * 1.1), 1, 5)
        lv = np.where(d > r - 1.0, 0.0, lv)
        ctx.above.paint(int(x) - n, int(y) - n, disc, rid, lv.astype(np.float32))


def paint_rock(ctx, st: Structure) -> None:
    sc = ctx.sc
    cv = ctx.below
    th = sc.th
    m = st.mask
    style = st.style
    seed = pix.seed_of(sc.md.map_id, "rock", st.sid)
    H, W = cv.h, cv.w
    mpx = pix.upsample_cells(m, T, smooth=False) > 0.5
    ctx.solid_px |= mpx
    ctx.tall_px |= mpx
    if style == "abyss":
        paint_abyss(ctx, st, mpx, seed)
        return
    rock_kind = {"cliff": "cliff", "valley": "valley", "cave": "cave", "earth": "earth",
                 "dusk": "dusk"}.get(style, "cliff")
    rid = mm.rock(rock_kind)
    role, rif, fh_of = face_rows(sc, m, MAX_FACE.get(style, 2))
    face_cells = pix.upsample_cells(role == 1, T, smooth=False) > 0.5
    # 崖沿参差：崖面最上一格的上沿按列往下缩 0–3px，缩掉的部分画成台面
    top_row_cells = pix.upsample_cells((role == 1) & (rif == 0), T, smooth=False) > 0.5
    off = rim_offset(W, seed + 1)
    yy = np.arange(H)[:, None]
    in_row = (yy % T) < off[None, :]
    face_px = face_cells & ~(top_row_cells & in_row)
    top_px = mpx & ~face_px
    top_kind = th["rock_top"]
    if style in ("cliff", "dusk") and top_kind == "grass":
        top_kind = "rocky"
    if style == "valley":
        top_kind = "shrubby"
    if style == "cave":
        top_kind = "void"
    ctx.rock_top[st.sid] = top_kind
    leaf_kind = "autumn" if sc.season == "autumn" else ("dark" if sc.time == "night" else "broad")
    pine_kind = "pine" if sc.season != "autumn" or sc.theme != "wild" else "autumn"

    # ---- 岩顶 ----
    if top_kind == "void":
        v = mm.void()
        lv = np.full((H, W), 1.0, np.float32)
        lv -= (mg.density_field(H, W, seed, (14, 7)) < 0.4) * 1.0
        cv.paint(0, 0, top_px, v, lv)
    elif top_kind == "forest":
        cv.paint(0, 0, top_px, rid, tex_plateau(H, W, seed) - 1.0)
        kind = "pine" if sc.theme in ("mountain_path", "cliff") else ("dark" if sc.time == "night" else "broad")
        crowns(cv, top_px, seed + 11, kind)
        if sc.season == "autumn" and sc.theme == "wild":
            red = top_px & (pix.value_noise(H, W, 30.0, seed + 12) > 0.66)
            crowns(cv, red, seed + 13, "autumn", 11, 4.0, 6.5, fill_bg=False)
        walk_only = pix.upsample_cells(sc.walk, T, smooth=False) > 0.5
        edge_canopy(ctx, top_px, walk_only, seed + 14, kind)
    elif top_kind == "grass":
        # 坡顶是荒草：比路边的矮草密、高、暗一档，缓起伏，零星几丛大灌木
        grid, glv = mg.tex_tallgrass(H, W, seed + 1, sc.season)
        cv.paint(0, 0, top_px, grid, np.clip(glv + tex_hill(H, W, seed), 0, 5))
        bushes(cv, top_px, seed + 5, "broad" if sc.time != "night" else "dark", 30, 0.35)
        if sc.season == "autumn":
            bushes(cv, top_px, seed + 6, "autumn", 34, 0.3)
    elif top_kind == "shrubby":
        # 谷壁：岩面上爬满草与灌木，岩石只在缝里露出来
        cv.paint(0, 0, top_px, rid, tex_plateau(H, W, seed))
        dens = np.clip((pix.value_noise(H, W, 12.0, seed + 2) - 0.25) * 1.6, 0.0, 0.9)
        scatter_blades(cv, top_px, seed + 3, sc.season, dens)
        shrubs = top_px & (pix.value_noise(H, W, 14.0, seed + 4) > 0.5)
        crowns(cv, shrubs, seed + 5, leaf_kind, 7, 3.0, 5.5)
    else:  # rocky：台面 + 疏草 + 几丛矮松与灌木
        cv.paint(0, 0, top_px, rid, tex_plateau(H, W, seed))
        dens = np.clip((pix.value_noise(H, W, 14.0, seed + 6) - 0.55) * 1.5, 0.0, 0.5)
        scatter_blades(cv, top_px, seed + 7, sc.season, dens)
        bushes(cv, top_px, seed + 9, pine_kind, 20, 0.45)
        if sc.season == "autumn":
            bushes(cv, top_px, seed + 10, "autumn", 34, 0.22)

    # 岩顶挨着可走格：北沿与西沿是受光的棱（亮），东沿是背光的棱（暗）
    walkpx = pix.upsample_cells(sc.walk | sc.water | sc.fence, T, smooth=False) > 0.5
    north_rim = top_px & pix.shift(walkpx, 0, 1)
    north_rim2 = top_px & pix.shift(walkpx, 0, 2) & ~north_rim
    west_rim = top_px & pix.shift(walkpx, 1, 0)
    west_rim2 = top_px & pix.shift(walkpx, 2, 0) & ~west_rim
    east_rim = top_px & pix.shift(walkpx, -1, 0)
    east_rim2 = top_px & pix.shift(walkpx, -2, 0) & ~east_rim
    if top_kind == "void":
        cv.paint(0, 0, north_rim | west_rim, rid, 3.0)
        cv.paint(0, 0, north_rim2 | east_rim, rid, 2.0)
    elif top_kind == "forest":
        # 林子没有岩棱：边上是伸出来的树冠（edge_canopy），再画一线亮的石边就像地毯的包边
        pass
    else:
        cv.paint(0, 0, north_rim | west_rim, rid, 4.0)
        cv.paint(0, 0, north_rim2 | west_rim2, rid, 3.0)
        cv.paint(0, 0, east_rim, rid, 1.0)
        cv.paint(0, 0, east_rim2, rid, 2.0)

    # ---- 崖面 ----
    if style == "earth":
        cv.paint(0, 0, face_px, rid, tex_face_earth(H, W, seed + 21, face_px))
    else:
        cv.paint(0, 0, face_px, rid, tex_face(H, W, seed + 21))
    # 崖顶沿：一线亮的岩棱，下面一线暗
    top_edge = face_px & ~pix.shift(face_px, 0, 1)
    cv.paint(0, 0, top_edge, rid, 4.0)
    cv.shade(0, 0, -1.0, face_px & pix.shift(top_edge, 0, 1))
    if top_kind in ("grass", "forest", "rocky", "shrubby"):
        # 崖沿垂下几簇草 / 藤
        col_h = pix.hash01(np.arange(W)[None, :], 1, seed)
        hang = face_px & pix.shift(top_edge, 0, 1) & (col_h < 0.4)
        hang2 = face_px & pix.shift(hang, 0, 1) & (pix.hash01(np.arange(W)[None, :], 2, seed) < 0.55)
        hang3 = face_px & pix.shift(hang2, 0, 1) & (pix.hash01(np.arange(W)[None, :], 3, seed) < 0.4)
        grid = mm.grass(sc.season) if top_kind != "forest" else mm.foliage(
            "pine" if sc.theme != "wild" else "broad")
        cv.paint(0, 0, hang, grid, 3.0)
        cv.paint(0, 0, hang2, grid, 2.0)
        cv.paint(0, 0, hang3, grid, 1.0)
    # 崖脚两行暗（接地）
    bottom = face_px & ~pix.shift(face_px, 0, -1)
    cv.shade(0, 0, -1.0, bottom | (face_px & pix.shift(bottom, 0, -1)))
    # 崖面左端受光、右端背光
    left_end = face_px & pix.shift(~mpx, 1, 0)
    right_end = face_px & pix.shift(~mpx, -1, 0)
    cv.shade(0, 0, 1.0, left_end)
    cv.shade(0, 0, -1.0, right_end)


# ---------------------------------------------------------------------------
# 崖外云海（落日峰）
# ---------------------------------------------------------------------------
def paint_abyss(ctx, st: Structure, mpx: np.ndarray, seed: int) -> None:
    """峰顶四周不是「一圈墙」，是崖边：挨着峰顶的一道岩棱，岩棱外面是脚下的云海。
    南侧（镜头这边）看得见崖面往下掉进云里；北、东、西三面只看得见崖沿。"""
    sc = ctx.sc
    cv = ctx.below
    H, W = cv.h, cv.w
    # 云海用中性的灰白（黄昏的暖色交给运行时光照去染，烘焙里再染一遍就成了一片品红）。
    # 一团团积云：高度场取阈值得云团，云团朝上的一沿亮、团与团之间的缝最暗。
    cloud = cloud_ramp("day")
    cv.paint(0, 0, mpx, cloud, 1.0)
    billows(cv, mpx, seed, cloud)
    rid = mm.rock("dusk" if sc.time == "dusk" else "cliff")
    walkpx = pix.upsample_cells(sc.walk, T, smooth=False) > 0.5
    # 岩棱：离可走区 ≤ 5px 的岩格像素
    near = mpx & pix.dilate(walkpx, 5)
    wob = pix.value_noise(H, W, 6.0, seed + 2) > 0.5
    rim = mpx & (pix.dilate(walkpx, 3) | (near & wob))
    cv.paint(0, 0, rim, rid, tex_rocky_top(H, W, seed + 3))
    edge = rim & ~pix.dilate(walkpx, 1)
    outer = rim & ~pix.erode(rim | walkpx, 1)
    cv.paint(0, 0, outer, rid, 1.0)
    cv.paint(0, 0, rim & pix.shift(walkpx, 0, 1), rid, 4.0)
    # 南侧崖面：可走区以南的岩格，从崖沿往下 20px 是岩面，再往下渐渐没入云里
    south = mpx & pix.shift(walkpx, 0, 1)
    face = np.zeros_like(mpx)
    cur = south
    for _ in range(22):
        face |= cur
        cur = pix.shift(cur, 0, 1) & mpx
    flv = tex_face(H, W, seed + 5)
    fade = np.zeros((H, W), np.float32)
    ys = np.arange(H)[:, None]
    depth = np.zeros((H, W), np.float32)
    cur = south
    for k in range(22):
        depth[cur] = k
        cur = pix.shift(cur, 0, 1) & mpx
    fade = np.where(depth > 12, -(depth - 12) * 0.25, 0.0).astype(np.float32)
    keep = face & ~((depth > 16) & (pix.hash01(np.arange(W)[None, :], ys, seed) < (depth - 16) / 6.0))
    cv.paint(0, 0, keep, rid, flv + fade)
    cv.paint(0, 0, south & ~pix.shift(south, 0, 1), rid, 4.0)
    del edge


def billows(cv: pix.Canvas, region: np.ndarray, seed: int, rid: int, step: int = 13,
            rmin: float = 6.0, rmax: float = 12.0) -> None:
    """云海：一团团圆鼓鼓的积云，南边的压在北边的上面；每团上沿亮、下沿暗，
    团与团之间露出底下更暗的一层（山谷里的雾）。与林冠同一个画法，只是换了颜色、放大了团。"""
    ys_r, xs_r = np.nonzero(region)
    if len(xs_r) == 0:
        return
    y0, y1, x0, x1 = ys_r.min(), ys_r.max(), xs_r.min(), xs_r.max()
    g = np.random.default_rng(seed)
    pts = []
    for gy in range(int(y0) - step, int(y1) + step, step):
        for gx in range(int(x0) - step, int(x1) + step, step):
            pts.append((gy + g.random() * step, gx + g.random() * step, rmin + g.random() * (rmax - rmin)))
    pts.sort()
    for cy, cx, r in pts:
        n = int(np.ceil(r)) + 1
        yy, xx = np.mgrid[-n:n + 1, -n:n + 1].astype(np.float32)
        d = np.sqrt((xx / 1.25) ** 2 + yy * yy)
        disc = d <= r * 0.85
        lv = np.clip(np.round(3.6 - yy / r * 1.6 - xx / r * 0.5), 2, 5)
        lv = np.where(d > r * 0.85 - 1.0, np.where(yy < 0, 3.0, 1.0), lv)
        px, py = int(round(cx)) - n, int(round(cy)) - n
        cl = cv._clip(px, py, disc.shape[0], disc.shape[1])
        if cl is None:
            continue
        sy, sx, my, mx = cl
        tmp = np.zeros_like(disc)
        tmp[my, mx] = disc[my, mx] & region[sy, sx]
        cv.paint(px, py, tmp, rid, lv.astype(np.float32))


def cloud_ramp(time: str) -> int:
    from palette import c, mix
    if time == "dusk":
        cols = [mix("violet1", "ink2", 0.4), c("violet2"), mix("violet3", "red4", 0.3), mix("red4", "gold3", 0.5),
                mix("gold3", "paper", 0.4), mix("gold4", "paper", 0.6)]
        return pix.ramp_id("cloud_dusk", cols)
    if time == "night":
        cols = [c("ink1"), c("ink2"), mix("ink3", "water1", 0.4), mix("ink4", "water2", 0.4), c("ink5"), c("ink6")]
        return pix.ramp_id("cloud_night", cols)
    cols = [mix("ink4", "water2", 0.4), mix("ink5", "water3", 0.4), c("ink6"), c("ink7"), c("ink8"), c("paper")]
    return pix.ramp_id("cloud_day", cols)
