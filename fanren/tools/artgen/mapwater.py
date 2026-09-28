"""水面：ground=3 且碰撞挡路（「水面不画墙，只挡路」）。

深浅按离岸距离：岸边浅（亮）、中间深（暗），抖动过渡；水面上零星几道横向波光；
岸线一道白沫、岸上一线湿土。渡口的岸边长芦苇，谷里的水潭浮几片荷叶。
水面格另列进 meta.json 的 water，运行时再加动态波光——烘焙图里的是静止的底。
"""

from __future__ import annotations

import numpy as np

import mapground as mg
import mapmat as mm
import pix

T = 16


def shore_distance(wpx: np.ndarray, limit: int = 24) -> np.ndarray:
    """每个水像素离岸（非水像素）的距离（像素，≤limit）。图边之外当作水，免得图边多出一道岸。"""
    pad = np.pad(wpx, limit, mode="edge")
    dist = np.zeros(pad.shape, np.float32)
    cur = pad.copy()
    for k in range(1, limit + 1):
        cur = pix.erode(cur, 1)
        dist[cur] = k
    return dist[limit:-limit, limit:-limit]


def paint_water(ctx) -> None:
    sc = ctx.sc
    cv = ctx.below
    if not sc.water.any():
        return
    seed = pix.seed_of(sc.md.map_id, "water")
    H, W = cv.h, cv.w
    cells = pix.upsample_cells(sc.water, T, smooth=False) > 0.5
    # 岸：水格边缘 1–3px 画成湿泥与卵石（仍在挡路的水格里，不会让能走的地方看着像水），
    # 凸角处岸更宽——方池子因此是圆角的，江岸是弯的。图边之外当作水。
    land = ~np.pad(cells, 4, mode="edge")
    landfrac = pix.box_blur(land.astype(np.float32), 3)[4:-4, 4:-4]
    wob = pix.value_noise(H, W, 7.0, seed + 11) * 0.16
    bank = cells & (landfrac > 0.2 + wob)
    wpx = cells & ~bank
    bank_rid = mm.dirt()
    blv = np.full((H, W), 2.0, np.float32)
    xs, ys = mg.jitter_points(H, W, 3, 0.5, seed + 12)
    blv[ys, xs] = 4.0
    blv[np.clip(ys + 1, 0, H - 1), xs] = 1.0
    cv.paint(0, 0, bank, bank_rid, blv)
    d = shore_distance(wpx)
    rid = mm.water()
    lv = 3.4 - np.minimum(d, 22.0) / 22.0 * 2.2
    # 波光：短横线，深处稀、浅处密
    xs, ys = mg.jitter_points(H, W, 7, 0.55, seed + 1)
    keep = wpx[ys, xs] & (d[ys, xs] > 3)
    xs, ys = xs[keep], ys[keep]
    for k in range(4):
        x = np.clip(xs + k, 0, W - 1)
        lv[ys, x] = np.maximum(lv[ys, x], 3.6 if k in (1, 2) else 3.0)
    # 暗一线的波谷紧贴在亮线下面
    lv[np.clip(ys + 1, 0, H - 1), xs] -= 0.8
    # 岸线白沫
    foam = wpx & (d <= 1)
    gaps = pix.hash01(np.arange(W)[None, :] // 2, np.arange(H)[:, None] // 2, seed + 3) < 0.25
    lv = np.where(foam & ~gaps, 5.0, lv)
    lv = np.where(wpx & (d == 2), np.maximum(lv, 4.0), lv)
    cv.paint(0, 0, wpx, rid, lv.astype(np.float32))
    # 岸上一线湿：水边第一圈地面暗一档
    wet = pix.dilate(wpx, 1) & ~wpx
    cv.shade(0, 0, -1.0, wet)
    wet2 = pix.dilate(wpx, 2) & ~pix.dilate(wpx, 1)
    cv.shade(0, 0, -0.5, wet2)
    if sc.theme == "dock":
        reeds(ctx, wpx, d, seed + 5)
    elif sc.theme in ("valley", "manor"):
        lotus(ctx, wpx, d, seed + 7)


def reeds(ctx, wpx: np.ndarray, d: np.ndarray, seed: int) -> None:
    """芦苇：沿岸浅水里一丛丛竖笔，秋天梢头发黄。"""
    cv = ctx.below
    H, W = wpx.shape
    rid = mm.grass("autumn" if ctx.sc.season == "autumn" else "summer")
    xs, ys = mg.jitter_points(H, W, 5, 0.6, seed)
    keep = wpx[ys, xs] & (d[ys, xs] >= 2) & (d[ys, xs] <= 7)
    g = np.random.default_rng(seed)
    for x, y in zip(xs[keep], ys[keep]):
        for k in range(int(g.integers(2, 5))):
            bx = int(x + g.integers(-2, 3))
            hgt = int(g.integers(5, 10))
            lean = int(g.integers(-1, 2))
            for j in range(hgt):
                xx = bx + (lean if j > hgt * 0.6 else 0)
                cv.px(xx, y - j, rid, 4 if j >= hgt - 2 else (2 if j < 2 else 3))
            cv.px(bx + lean, y - hgt, mm.dirt(), 4)


def lotus(ctx, wpx: np.ndarray, d: np.ndarray, seed: int) -> None:
    """荷叶：几片圆叶带一道缺口，偶有一朵粉花。"""
    cv = ctx.below
    H, W = wpx.shape
    leaf = mm.herb()
    xs, ys = mg.jitter_points(H, W, 11, 0.4, seed)
    keep = wpx[ys, xs] & (d[ys, xs] >= 4)
    g = np.random.default_rng(seed)
    for x, y in zip(xs[keep], ys[keep]):
        r = 1.6 + g.random() * 1.6
        disc = pix.disc(r)
        n = disc.shape[0] // 2
        yy, xx = np.mgrid[-n:n + 1, -n:n + 1]
        lv = np.where(yy + xx < -1, 4.0, np.where(yy + xx > 1, 2.0, 3.0))
        m = disc.copy()
        m[n, n + 1:] = False   # 荷叶那道缺口
        cv.paint(int(x) - n, int(y) - n, m, leaf, lv)
        if g.random() < 0.3:
            f = mm.flower("pink")
            cv.px(int(x), int(y) - 1, f, 3)
            cv.px(int(x) - 1, int(y), f, 2)
            cv.px(int(x) + 1, int(y), f, 2)
