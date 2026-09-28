"""树：树干画进 below.png，树冠画进 above.png（压在人物之上），树冠的影子烘进 below.png。

树冠不贴着 3×3 的 front 框画成方块，而是按树的样子：冠底落在树干中段（露出一截树干
与根），冠往上长出框外一两格——斜 45° 俯视里，树冠本来就在树干的上方。front 框只是
「这里的人会被树冠压住」的提示，不是树冠的轮廓。

树种（按主题与季节轮换，近水的一律柳，maps.json 可逐棵指定）：
  broad 阔叶 / locust 老槐（更大更深）/ pine 松（层层叠叠的伞盖）/ willow 柳（垂丝）
  bamboo 竹丛 / peach 桃花 / maple 枫 / ginkgo 银杏
"""

from __future__ import annotations

import numpy as np

import mapmat as mm
import pix
from mapinfer import Tree

T = 16

SPECIES = {
    #          冠宽 冠高 干宽  叶色渐层
    "broad":  (48, 40, 5, "broad"),
    "locust": (52, 44, 6, "broad"),
    "pine":   (46, 46, 4, "pine"),
    "willow": (50, 42, 4, "willow"),
    "bamboo": (36, 46, 0, "bamboo"),
    "peach":  (46, 38, 4, "blossom"),
    "maple":  (48, 40, 5, "autumn"),
    "ginkgo": (46, 42, 5, "ginkgo"),
}


def foliage_rid(kind: str) -> int:
    if kind == "blossom":
        return mm.blossom()
    return mm.foliage(kind)


def crown_geometry(t: Tree, species: str):
    cw, ch, tw, _ = SPECIES[species]
    cx = t.x * T + 8
    bottom = t.y * T + 7
    return cx, bottom, cw, ch, tw


def clusters_for(t: Tree, species: str, seed: int):
    """树冠由几团叶簇拼成：(cx, cy, r)。排列按树种不同。"""
    cx, bottom, cw, ch, _ = crown_geometry(t, species)
    g = np.random.default_rng(seed)
    out = []
    if species == "pine":
        tiers = [(0.36, 10, 1.0), (0.62, 12, 0.82), (0.86, 12, 0.6), (1.0, 9, 0.34)]
        for frac, hgt, wf in reversed(tiers):
            cy = bottom - ch * frac + hgt
            out.append(("tier", cx + g.normal(0, 1.2), cy, cw * wf / 2, hgt / 2))
        return out
    if species == "bamboo":
        return [("bamboo", cx, bottom, cw / 2, ch)]
    r0 = min(cw, ch) * 0.36
    ccy = bottom - ch * 0.5
    out.append(("ball", cx, ccy + 2, r0 * 1.05, 0))
    n = 7 if species != "locust" else 9
    for i in range(n):
        ang = -np.pi * (0.05 + 0.9 * i / (n - 1)) + g.normal(0, 0.15)
        rr = r0 * (0.62 + g.random() * 0.2)
        dist = r0 * (0.72 + g.random() * 0.15)
        out.append(("ball", cx + np.cos(ang) * dist * (cw / ch), ccy + np.sin(ang) * dist * 0.9, rr, 0))
    # 下沿两团，把冠底收圆
    for s in (-1, 1):
        out.append(("ball", cx + s * r0 * 0.75, ccy + r0 * 0.55, r0 * 0.62, 0))
    return out


def paint_crown(cv: pix.Canvas, t: Tree, species: str, seed: int) -> np.ndarray:
    """画树冠，返回树冠的像素掩码（整图大小，用来投影）。"""
    H, W = cv.h, cv.w
    cx, bottom, cw, ch, _ = crown_geometry(t, species)
    kind = SPECIES[species][3]
    rid = foliage_rid(kind)
    x0 = int(cx - cw / 2 - 6)
    y0 = int(bottom - ch - 6)
    bw, bh = cw + 12, ch + 14
    lv = np.full((bh, bw), -9.0, np.float32)
    yy, xx = np.mgrid[0:bh, 0:bw].astype(np.float32)
    ax = xx + x0
    ay = yy + y0
    if species == "bamboo":
        return paint_bamboo(cv, t, seed)
    if species == "pine":
        return paint_pine(cv, t, seed)
    parts = clusters_for(t, species, seed)
    for p in parts:
        kind_p, pcx, pcy, rx, ry = p
        if kind_p == "tier":
            dx = (ax - pcx) / rx
            dy = (ay - pcy) / ry
            # 松的每一层：上沿尖、下沿呈锯齿（针叶）
            jag = (pix.hash01(ax.astype(np.int64) // 2, 1, seed) - 0.5) * 0.5
            inside = (dx * dx + (dy + 0.25 * dx * dx) ** 2 * 1.4) <= 1.0 + jag * (dy > 0)
            shade = 3.2 - dx * 0.9 - dy * 1.1
            needle = (pix.hash01(ax.astype(np.int64) // 3, ay.astype(np.int64), seed + 2) < 0.25)
            shade = shade + np.where(needle, 1.0, 0.0)
            edge = inside & ~pix.erode(inside, 1)
            val = np.where(edge, 0.0, np.clip(np.round(shade), 1, 5))
            lv = np.where(inside, val, lv)
        else:
            r = rx
            d = np.sqrt((ax - pcx) ** 2 + (ay - pcy) ** 2)
            wob = (pix.hash01(ax.astype(np.int64) // 2, ay.astype(np.int64) // 2, seed + 1) - 0.5) * 2.4
            inside = d <= r + wob
            nx = (ax - pcx) / r
            ny = (ay - pcy) / r
            shade = 2.8 - (nx + ny) * 1.25 + (1.0 - np.clip(d / r, 0, 1)) * 0.7
            leaf = pix.hash01(ax.astype(np.int64) // 2, ay.astype(np.int64) // 2, seed + 3)
            shade = shade + np.where(leaf < 0.18, 1.0, 0.0) - np.where(leaf > 0.9, 1.0, 0.0)
            val = np.clip(np.round(shade), 1, 5)
            lv = np.where(inside, val, lv)
    mask = lv > -5
    # 叶缘：外轮廓最暗一线（左上受光处轮廓淡一档），轮廓外随机补几粒散叶
    outline = mask & ~pix.erode(mask, 1)
    tl = outline & (yy + xx < (bw + bh) * 0.45)
    lv = np.where(outline, np.where(tl, 1.0, 0.0), lv)
    if species == "willow":
        lv, mask = willow_strands(lv, mask, ax, ay, bottom, seed)
    if species == "peach":
        # 花团亮处再点几粒最亮的花
        hi = mask & (pix.hash01(ax.astype(np.int64), ay.astype(np.int64), seed + 9) < 0.06) & (lv >= 3)
        lv = np.where(hi, 4.0, lv)
    cv.paint(x0, y0, mask, rid, lv)
    if species == "peach":
        # 桃树：花团背光处透出零星叶绿（单个像素，不成块），冠里隐约两三道枝
        ih = pix.hash01(ax.astype(np.int64), ay.astype(np.int64), seed + 4)
        leafm = mask & ~outline & (ih < 0.07) & (lv <= 2)
        cv.paint(x0, y0, leafm, mm.foliage("broad"), 2.0)
        bark = mm.bark()
        cxl = int(cx) - x0
        for k, dx in enumerate((-9, 0, 8)):
            for j in range(10):
                yy_ = bh - 12 - j
                xx_ = cxl + dx * j // 10
                if 0 <= yy_ < bh and 0 <= xx_ < bw and mask[yy_, xx_] and ih[yy_, xx_] < 0.55:
                    cv.px(x0 + xx_, y0 + yy_, bark, 1)
    full = np.zeros((H, W), bool)
    sl = cv._clip(x0, y0, bh, bw)
    if sl is not None:
        sy, sx, my, mx = sl
        full[sy, sx] = mask[my, mx]
    return full


def willow_strands(lv, mask, ax, ay, bottom, seed):
    """柳：冠下半截垂下一缕缕细丝，比冠底再长出 6–10px。"""
    bh, bw = lv.shape
    lv = lv.copy()
    mask = mask.copy()
    cols = np.nonzero(mask.any(axis=0))[0]
    for c in cols:
        if pix.hash01(int(ax[0, c]), 7, seed) < 0.45:
            continue
        rows = np.nonzero(mask[:, c])[0]
        start = rows.max()
        extra = 5 + int(pix.hash01(int(ax[0, c]), 9, seed) * 7)
        for k in range(1, extra):
            r = start + k
            if r >= bh:
                break
            mask[r, c] = True
            lv[r, c] = 3.0 if k < extra - 2 else 4.0
    return lv, mask


def paint_pine(cv: pix.Canvas, t: Tree, seed: int) -> np.ndarray:
    """松：一根拧着长上去的干，干上左右错开托着四五片扁平的针叶云（黄山松的样子）。

    每片针叶云上沿亮、底面暗，面上是一道道短横的针叶纹；片与片之间露出枝干。
    """
    H, W = cv.h, cv.w
    rid = mm.foliage("pine")
    bark = mm.bark()
    g = np.random.default_rng(seed)
    cx = t.x * T + 8
    bottom = t.y * T + 7
    side = 1 if g.random() < 0.5 else -1
    pads = [(-7 * side, 8, 15, 7), (8 * side, 15, 14, 6), (-4 * side, 23, 15, 7),
            (3 * side, 31, 12, 6), (0, 38, 8, 5)]
    full = np.zeros((H, W), bool)

    def put(x, y, r, lv):
        if 0 <= y < H and 0 <= x < W:
            cv.px(x, y, r, lv)
            full[y, x] = True

    # 枝干：从树干顶一路拧到最高那片
    x = cx
    prev = (cx, bottom)
    for dx, up, rx, ry in pads:
        tx_, ty_ = cx + dx // 2, bottom - up + ry
        steps = max(1, abs(ty_ - prev[1]))
        for k in range(steps + 1):
            xx = int(round(prev[0] + (tx_ - prev[0]) * k / steps))
            yy = int(round(prev[1] + (ty_ - prev[1]) * k / steps))
            put(xx, yy, bark, 2)
            put(xx + 1, yy, bark, 1)
        # 伸向这片针叶云的横枝
        sgn = 1 if dx > 0 else -1
        for k in range(abs(dx) // 2 + 2):
            put(tx_ + sgn * k, ty_ - k // 4, bark, 2)
        prev = (tx_, ty_)
    for dx, up, rx, ry in pads:
        pcx = cx + dx + g.normal(0, 0.8)
        pcy = bottom - up
        n = int(rx) + 3
        yy, xx = np.mgrid[-ry - 4:ry + 4, -n:n + 1].astype(np.float32)
        ax = (xx + pcx).astype(np.int64)
        ay = (yy + pcy).astype(np.int64)
        # 一片针叶云由三四团扁圆拼成，上沿一团团鼓起，下沿平
        inside = np.zeros(xx.shape, bool)
        shade = np.full(xx.shape, -9.0, np.float32)
        lobes = int(g.integers(3, 5))
        for k in range(lobes):
            lx = (k / (lobes - 1) - 0.5) * rx * 1.3 + g.normal(0, 1.0)
            lr = ry * (0.9 + g.random() * 0.4)
            lrx = lr * 1.5
            ly = -g.random() * 1.5
            e = ((xx - lx) / lrx) ** 2 + ((yy - ly) / lr) ** 2
            m = e <= 1.0
            sh = 3.0 - ((xx - lx) / lrx + (yy - ly) / lr) * 1.2
            shade = np.where(m & (sh > shade), sh, shade)
            inside |= m
        inside &= yy <= ry * 0.55
        needle = pix.hash01(ax // 3, ay, seed + 5) < 0.3
        lv = np.clip(np.round(shade), 1, 4) + np.where(needle, 1.0, 0.0)
        lv = np.where(yy > ry * 0.2, np.minimum(lv, 2.0), lv)
        edge = inside & ~pix.erode(inside, 1)
        lv = np.where(edge & (yy >= 0), 0.0, np.where(edge, 1.0, lv))
        x0 = int(round(pcx)) - n
        y0 = int(round(pcy)) - ry - 4
        cv.paint(x0, y0, inside, rid, np.clip(lv, 0, 5).astype(np.float32))
        cl = cv._clip(x0, y0, inside.shape[0], inside.shape[1])
        if cl is not None:
            sy, sx, my, mx = cl
            full[sy, sx] |= inside[my, mx]
    return full


def paint_bamboo(cv: pix.Canvas, t: Tree, seed: int) -> np.ndarray:
    """竹丛：五六根竹竿从根部散开往上长，梢头一簇簇斜挑的竹叶。"""
    H, W = cv.h, cv.w
    b = mm.bamboo()
    leaf = mm.foliage("bamboo")
    g = np.random.default_rng(seed)
    base_x = t.x * T + 8
    base_y = t.y * T + 13
    full = np.zeros((H, W), bool)

    def put(x, y, rid, lv):
        if 0 <= y < H and 0 <= x < W:
            cv.px(x, y, rid, lv)
            full[y, x] = True

    tops = []
    for i in range(12):
        off = (i - 5.5) * 1.8 + g.normal(0, 0.8)
        top = base_y - 34 - int(g.integers(0, 14))
        lean = off * 0.55
        for y in range(top, base_y):
            f = (base_y - y) / (base_y - top)
            x = int(round(base_x + off * 0.35 + lean * f))
            put(x, y, b, 2 if i % 2 else 3)
            put(x + 1, y, b, 1)
            if (y - top) % 6 == 0:
                put(x, y, b, 1)
                put(x + 1, y, b, 1)
        tops.append((int(round(base_x + off * 0.35 + lean)), top))
    # 竹叶：一簇簇从竹节上挑出去的细长叶，梢头最密
    sprays = [(tx + int(g.integers(-4, 5)), ty + int(g.integers(0, 12))) for tx, ty in tops for _ in range(4)]
    sprays += [(base_x + int(g.integers(-15, 16)), base_y - 12 - int(g.integers(0, 26))) for _ in range(22)]
    for sx, sy in sprays:
        for _ in range(int(g.integers(4, 7))):
            dirx = int(g.choice((-1, 1)))
            ln = int(g.integers(4, 7))
            droop = g.random() < 0.5
            for k in range(ln):
                put(sx + k * dirx, sy + (k // 2 if droop else -(k // 3)), leaf, 4 if k == 0 else 3)
            put(sx + dirx, sy + 1, leaf, 1)
    return full


def paint_trunk(cv: pix.Canvas, t: Tree, species: str) -> tuple[int, int, int, int]:
    """树干（below）：左一线亮、右一线暗，根部两侧各外扩 1px。返回树干矩形。"""
    _, _, _, _, tw = crown_geometry(t, species)
    if tw == 0:
        return (0, 0, 0, 0)
    bark = mm.bark()
    cx = t.x * T + 8
    y_bot = t.y * T + 14
    y_top = t.y * T - 2
    x0 = cx - tw // 2
    cv.rect(x0, y_top, tw, y_bot - y_top, bark, 2)
    cv.rect(x0, y_top, 1, y_bot - y_top, bark, 3)
    cv.rect(x0 + tw - 1, y_top, 1, y_bot - y_top, bark, 1)
    for yy in range(y_top + 2, y_bot, 4):
        cv.px(x0 + 1 + (yy // 4) % max(1, tw - 2), yy, bark, 1)
    cv.rect(x0 - 1, y_bot - 2, 1, 2, bark, 2)
    cv.rect(x0 + tw, y_bot - 2, 1, 2, bark, 1)
    cv.rect(x0 - 1, y_bot, tw + 2, 1, bark, 0)
    return (x0 - 1, y_top, tw + 2, y_bot - y_top + 1)


def paint_trunks(ctx) -> None:
    sc = ctx.sc
    for t in sc.trees:
        x, y, w, h = paint_trunk(ctx.below, t, t.species)
        if w:
            ctx.solid_px[y:y + h, x:x + w] = True


def crown_shadow(ctx, crown: np.ndarray, t: Tree) -> None:
    """树冠投在地上：冠形压扁到约四成高，落在树干根部偏右，抖动的一片暗。"""
    H, W = crown.shape
    ys, xs = np.nonzero(crown)
    if len(xs) == 0:
        return
    x0, x1, y0, y1 = xs.min(), xs.max(), ys.min(), ys.max()
    sub = crown[y0:y1 + 1, x0:x1 + 1]
    sh_h = max(4, int((y1 - y0 + 1) * 0.42))
    idx = (np.arange(sh_h) * (y1 - y0 + 1) / sh_h).astype(np.int64)
    squashed = sub[idx]
    cx = t.x * T + 8 + 5
    base = t.y * T + 14
    top = base - sh_h // 2 - 2
    left = cx - (x1 - x0 + 1) // 2
    m = np.zeros((H, W), bool)
    cl = ctx.below._clip(left, top, squashed.shape[0], squashed.shape[1])
    if cl is None:
        return
    sy, sx, my, mx = cl
    m[sy, sx] = squashed[my, mx]
    soft = pix.box_blur(m.astype(np.float32), 2)
    soft = np.where(ctx.solid_px, 0.0, soft)
    ctx.below.shade(0, 0, -np.minimum(soft * 1.5, 1.0), soft > 0.1)


def paint_canopies(ctx) -> None:
    sc = ctx.sc
    # 从北往南画：南边的树冠压在北边的上面
    for t in sorted(sc.trees, key=lambda t: (t.y, t.x)):
        seed = pix.seed_of(sc.md.map_id, "tree", t.x, t.y)
        crown = paint_crown(ctx.above, t, t.species, seed)
        crown_shadow(ctx, crown, t)
        if t.species in ("peach",):
            ctx.lights.append({"emitter": "petal", "x": t.x + 0.5, "y": t.y - 1.0})
        elif t.species in ("maple", "ginkgo") or (sc.season == "autumn" and t.species == "broad"):
            ctx.lights.append({"emitter": "leaf", "x": t.x + 0.5, "y": t.y - 1.0})
