"""地面：草地、土路、石板、石阶、木板、翻土、碎石、室内地板……

每种地面是一张「整图大小」的纹理场（渐层 + 明度级），按格子的地面类型挑像素贴上去。
整图算纹理而不是逐格拼：石板的错缝、土路的斑驳天然跨格连续，没有接缝可言。

接边：
  * 软质地面（草、土、沙、碎石、洞底……）彼此之间按「格心双线性插值 + 噪声」取最大者，
    边界是一条弯弯曲曲的线，土路不会是一格一格的硬方块；
  * 硬质地面（石板、石阶、木板、地板）照格子边界切齐——铺出来的东西本来就是直边，
    但交界处压一道 1px 的暗线（路沿），草地一侧随机伸几根草叶过来。
"""

from __future__ import annotations

import numpy as np

import mapmat as mm
import pix
from mapinfer import S, SURFACES, Scene

T = 16
SOFT = {"grass", "earth", "sand", "gravel", "dirt_road", "scree", "scorched", "cave_floor",
        "cave_path", "courtyard"}


# ---------------------------------------------------------------------------
# 点状小图案（草簇、石子）：在一组坐标上按偏移表写明度级
# ---------------------------------------------------------------------------
def jitter_points(h: int, w: int, step: int, prob: float, seed: int):
    """错位网格上的随机点：比纯白噪声均匀，不会扎堆也不会出现大片空白。"""
    g = np.random.default_rng(seed)
    gy, gx = np.mgrid[0:h:step, 0:w:step]
    gy = gy.ravel()
    gx = gx.ravel()
    keep = g.random(gy.size) < prob
    oy = g.integers(0, step, gy.size)
    ox = g.integers(0, step, gx.size)
    ys = (gy + oy)[keep]
    xs = (gx + ox)[keep]
    inside = (ys < h) & (xs < w)
    return xs[inside], ys[inside]


def stamp(lv: np.ndarray, xs, ys, pattern, mode: str = "add", mask: np.ndarray | None = None) -> None:
    h, w = lv.shape
    for dx, dy, val in pattern:
        x = xs + dx
        y = ys + dy
        ok = (x >= 0) & (x < w) & (y >= 0) & (y < h)
        x, y = x[ok], y[ok]
        if mask is not None:
            keep = mask[y, x]
            x, y = x[keep], y[keep]
        if mode == "add":
            np.add.at(lv, (y, x), val)
        else:
            lv[y, x] = val


# 纹理只用整数明度级：成簇的像素，而不是棋盘抖动。小数明度级留给阴影一类的明暗过渡。
TUFT = [(0, 0, 2.0), (-1, 1, 1.0), (1, 1, 1.0), (0, 1, 1.0), (0, 2, -1.0)]
TUFT_SMALL = [(0, 0, 1.0), (0, 1, -1.0)]
PEBBLE = [(0, 0, -1.0), (1, 0, -1.0), (-1, -1, 1.0)]
SPECK_DARK = [(0, 0, -1.0)]
SPECK_LIGHT = [(0, 0, 1.0)]


# ---------------------------------------------------------------------------
# 纹理
# ---------------------------------------------------------------------------
def patches(h: int, w: int, seed: int, hi: float = 0.66, lo: float = 0.32, cells=(24, 10)) -> np.ndarray:
    """低频斑块：-1 / 0 / +1 三档，成团出现（像素画的「簇」，不是逐点噪声）。"""
    n = pix.fbm(h, w, seed, ((cells[0], 0.62), (cells[1], 0.38)))
    return (n > hi).astype(np.float32) - (n < lo).astype(np.float32)


def density_field(h: int, w: int, seed: int, cells=(26, 11)) -> np.ndarray:
    return pix.fbm(h, w, seed, ((cells[0], 0.62), (cells[1], 0.38)))


def tex_grass(h, w, seed, season):
    """草地：底色只有一档；明暗变化靠「草簇的疏密」——亮处草叶密、暗处暗点密。

    大块整档的明暗斑在 ×3 放大后像迷彩，这是第一版的教训。
    """
    lv = np.full((h, w), 2.0, np.float32)
    dens = density_field(h, w, seed)
    # 草叶：错位网格上的一笔一像素竖划，亮处是亮叶（偶有更亮的叶尖）、暗处是暗叶，
    # 亮叶下方一像素是它的影子
    xs, ys = jitter_points(h, w, 3, 0.75, seed + 9)
    d = dens[ys, xs]
    r = pix.hash01(xs, ys, seed + 1)
    # 亮区：亮叶为主、间有更亮的叶尖；暗区：暗叶为主；中间两者混着
    p_light = np.clip((d - 0.34) / 0.3, 0.0, 1.0)
    tip = np.where(r < p_light, np.where(r < p_light * 0.3, 2.0, 1.0), -1.0)
    np.add.at(lv, (ys, xs), tip)
    below = ys + 1 < h
    np.add.at(lv, (ys[below] + 1, xs[below]), np.where(tip[below] > 0, -1.0, 0.0))
    xs, ys = jitter_points(h, w, 7, 0.55, seed + 13)
    keep = dens[ys, xs] > 0.52
    stamp(lv, xs[keep], ys[keep], TUFT)
    return mm.grass(season), np.clip(lv, 0, 5)


def tex_tallgrass(h, w, seed, season):
    """荒草（土坡顶、山坡顶这些走不上去的地方）：密密的竖草，每 2px 一茎、高 3–6px，
    草尖亮、根部暗。比路边的矮草密得多、高得多——一眼看得出是「长满了草、没人走的坡」。"""
    lv = np.full((h, w), 1.0, np.float32)
    xs, ys = jitter_points(h, w, 2, 0.85, seed)
    hts = 3 + (pix.hash01(xs, ys, seed + 1) * 4).astype(np.int64)
    tipc = np.where(pix.hash01(xs, ys, seed + 2) < 0.25, 4.0, 3.0)
    dens = density_field(h, w, seed + 3, (20, 9))
    for k in range(6):
        on = hts > k
        y = ys - k
        ok = on & (y >= 0)
        val = np.where(k == hts - 1, tipc, np.where(k == 0, 1.0, 2.0))
        lv[y[ok], xs[ok]] = np.maximum(lv[y[ok], xs[ok]], val[ok] - (dens[ys[ok], xs[ok]] < 0.35))
    return mm.grass(season), lv


def tex_dirt(h, w, seed, base=3.0):
    lv = np.full((h, w), base, np.float32)
    dens = density_field(h, w, seed, (22, 9))
    lv -= (dens < 0.27) * 1.0
    xs, ys = jitter_points(h, w, 6, 0.32, seed + 7)
    stamp(lv, xs, ys, PEBBLE)
    xs, ys = jitter_points(h, w, 4, 0.30, seed + 8)
    d = dens[ys, xs]
    stamp(lv, xs[d > 0.55], ys[d > 0.55], SPECK_LIGHT)
    xs, ys = jitter_points(h, w, 4, 0.25, seed + 9)
    d = dens[ys, xs]
    stamp(lv, xs[d < 0.45], ys[d < 0.45], SPECK_DARK)
    return mm.dirt(), lv


def tex_sand(h, w, seed):
    lv = 3.0 + patches(h, w, seed, 0.78, 0.2, (28, 12))
    xs, ys = jitter_points(h, w, 6, 0.18, seed + 7)
    stamp(lv, xs, ys, SPECK_DARK)
    xs, ys = jitter_points(h, w, 9, 0.2, seed + 9)
    stamp(lv, xs, ys, PEBBLE)
    return mm.sand(), lv


def qtone(r, lo: float = 0.1, hi: float = 0.93):
    """[0,1) → {-1, 0, +1}：每块石板 / 每条木板的整档明暗差（多数是 0）。

    第一版取 18% / 16%，满地明暗块跳来跳去，×3 放大后像棋盘——石板路抢了整张图的戏。
    """
    return np.where(r < lo, -1.0, np.where(r > hi, 1.0, 0.0))


def tex_scree(h, w, seed, rid, base=2.0):
    """碎石坡：错位网格上一颗颗石子（左上一点亮、右下一点暗），石缝里是更暗的土。
    石子不能太密：每 4px 就一颗的话 ×3 下是一片雪花噪点。"""
    lv = np.full((h, w), base - 1.0, np.float32)
    lv += (patches(h, w, seed, 0.7, 0.28) > 0) * 1.0
    xs, ys = jitter_points(h, w, 5, 0.55, seed + 3)
    tone = qtone(pix.hash01(xs, ys, seed + 1))
    for dx, dy, dv in ((0, 0, 1.0), (1, 0, 0.0), (0, 1, 0.0), (1, 1, -1.0), (-1, 0, 0.0)):
        x = np.clip(xs + dx, 0, w - 1)
        y = np.clip(ys + dy, 0, h - 1)
        lv[y, x] = base + dv + tone
    big = pix.hash01(xs, ys, seed) < 0.3
    bx, by = xs[big], ys[big]
    for dx, dy, dv in ((2, 0, 0.0), (2, 1, -1.0), (0, 2, -1.0), (1, 2, -1.0), (-1, -1, 1.0)):
        x = np.clip(bx + dx, 0, w - 1)
        y = np.clip(by + dy, 0, h - 1)
        lv[y, x] = base + dv
    return rid, lv


def tex_slabs(h, w, seed, rid, row_h=8, wmin=8, wmax=15, base=3.0):
    """错缝石板：每行一条横缝，缝在行内随机错开；板面左上角一点高光、底边暗一档。"""
    lv = np.zeros((h, w), np.float32)
    ys = np.arange(h)
    band = ys // row_h
    yin = ys % row_h
    xs = np.arange(w)
    for b in range(int(band.max()) + 1):
        g = np.random.default_rng(seed + b * 101)
        cuts = [-int(g.integers(0, wmax))]
        while cuts[-1] < w:
            cuts.append(cuts[-1] + int(g.integers(wmin, wmax + 1)))
        cuts = np.array(cuts)
        sid = np.searchsorted(cuts, xs, side="right") - 1
        xin = xs - cuts[sid]
        tone = qtone(g.random(len(cuts)))
        for r in np.nonzero(band == b)[0]:
            yi = yin[r]
            if yi == 0:
                lv[r] = base - 2
                continue
            v = base + tone[sid]
            v = np.where(xin == 0, base - 2, v)
            if yi == 1:
                v = np.where((xin >= 1) & (xin <= 2), v + 1, v)
            if yi == row_h - 1:
                v = np.where(xin > 0, v - 1, v)
            lv[r] = v
    wear = pix.value_noise(h, w, 3.0, seed + 9)
    lv += np.where(wear > 0.86, -1.0, 0.0)
    return rid, lv


def voronoi_ids(h: int, w: int, cell: int, seed: int):
    """错位网格点的最近点编号与到最近/次近点的距离差（石缝）。"""
    g = np.random.default_rng(seed)
    gh, gw = h // cell + 3, w // cell + 3
    px = (np.arange(gw)[None, :] + g.random((gh, gw)) * 0.8 + 0.1) * cell - cell
    py = (np.arange(gh)[:, None] + g.random((gh, gw)) * 0.8 + 0.1) * cell - cell
    yy, xx = np.mgrid[0:h, 0:w].astype(np.float32)
    cy = (yy + cell) // cell
    cx = (xx + cell) // cell
    best = np.full((h, w), 1e9, np.float32)
    second = np.full((h, w), 1e9, np.float32)
    bid = np.zeros((h, w), np.int32)
    for dy in (-1, 0, 1):
        for dx in (-1, 0, 1):
            iy = np.clip(cy + dy, 0, gh - 1).astype(np.int32)
            ix = np.clip(cx + dx, 0, gw - 1).astype(np.int32)
            d = (xx - px[iy, ix]) ** 2 + (yy - py[iy, ix]) ** 2
            closer = d < best
            second = np.where(closer, best, np.minimum(second, d))
            bid = np.where(closer, iy * gw + ix, bid)
            best = np.where(closer, d, best)
    edge = np.sqrt(second) - np.sqrt(best)
    return bid, edge


def voronoi_full(h: int, w: int, cell: int, seed: int):
    """同 voronoi_ids，另返回到本块中心的距离（给「一块块圆石」的隆起用）。"""
    g = np.random.default_rng(seed)
    gh, gw = h // cell + 3, w // cell + 3
    px = (np.arange(gw)[None, :] + g.random((gh, gw)) * 0.8 + 0.1) * cell - cell
    py = (np.arange(gh)[:, None] + g.random((gh, gw)) * 0.8 + 0.1) * cell - cell
    yy, xx = np.mgrid[0:h, 0:w].astype(np.float32)
    cy = (yy + cell) // cell
    cx = (xx + cell) // cell
    best = np.full((h, w), 1e9, np.float32)
    second = np.full((h, w), 1e9, np.float32)
    bid = np.zeros((h, w), np.int32)
    for dy in (-1, 0, 1):
        for dx in (-1, 0, 1):
            iy = np.clip(cy + dy, 0, gh - 1).astype(np.int32)
            ix = np.clip(cx + dx, 0, gw - 1).astype(np.int32)
            d = (xx - px[iy, ix]) ** 2 + (yy - py[iy, ix]) ** 2
            closer = d < best
            second = np.where(closer, best, np.minimum(second, d))
            bid = np.where(closer, iy * gw + ix, bid)
            best = np.where(closer, d, best)
    return bid, np.sqrt(second) - np.sqrt(best), np.sqrt(best)


def tex_flagstone(h, w, seed, rid, cell=11, base=3.0):
    """不规则石板（山道、洞道、室内甬道）：错位网格的 Voronoi 块，缝暗、块面整档明暗。"""
    bid, edge = voronoi_ids(h, w, cell, seed)
    lv = base + qtone(pix.hash01(bid, 7, seed), 0.06, 0.96)
    # 石缝一线暗（缝的正中再暗一档）；块面不再勾亮边——每块都描一圈亮边，满地碎花砖，比路还抢眼
    lv = np.where(edge < 1.3, base - 1, lv)
    lv = np.where(edge < 0.6, base - 2, lv)
    return rid, lv.astype(np.float32)


def tex_steps(h, w, seed, rid):
    """石阶：每 6px 一级。上三行是踏面，第四行是踏面前沿（最亮），下面两行是立面与阴影。"""
    ys = np.arange(h)[:, None]
    xs = np.arange(w)[None, :]
    jit = (pix.hash01(xs // 8, ys // 6, seed) < 0.25).astype(np.int32)
    yi = (ys + jit) % 6
    lv = np.select([yi == 0, yi == 1, yi == 2, yi == 3, yi == 4, yi == 5],
                   [3, 3, 3, 4, 1, 2]).astype(np.float32)
    lv = np.broadcast_to(lv, (h, w)).copy()
    crack = pix.value_noise(h, w, 3.0, seed + 3)
    lv += np.where((crack > 0.84) & (lv >= 3), -1.0, 0.0)
    return rid, lv


def tex_planks(h, w, seed, rid, vertical: bool, base=3.0, seam=2, lower=1, quiet=False):
    """木板（栈桥、码头、地板）：板宽 4px（3px 板 + 1px 缝），板头错开，偶见钉眼。
    seam 是板缝比板面暗几档，lower 是板面下半截（背光的那一线）暗几档。"""
    if vertical:
        rid_, lv = tex_planks(w, h, seed, rid, False, base, seam, lower, quiet)
        return rid_, lv.T.copy()
    lv = np.zeros((h, w), np.float32)
    g = np.random.default_rng(seed)
    xs = np.arange(w)
    for y0 in range(0, h, 4):
        cuts = [-int(g.integers(0, 40))]
        while cuts[-1] < w:
            cuts.append(cuts[-1] + int(g.integers(22, 44)))
        cuts = np.array(cuts)
        pid = np.searchsorted(cuts, xs, side="right") - 1
        tone = qtone(g.random(len(cuts)), 0.05, 0.97) if quiet else qtone(g.random(len(cuts)))
        xin = xs - cuts[pid]
        for k in range(4):
            y = y0 + k
            if y >= h:
                break
            if k == 3:
                lv[y] = base - seam
                continue
            v = base + tone[pid] - (lower if k == 2 else 0.0)
            v = np.where(xin == 0, base - seam, v)
            if not quiet:
                v = np.where((xin == 2) & (k == 1), base - 1, v)   # 钉眼
            lv[y] = v
    grain = pix.value_noise(h, w, 5, seed + 3)
    lv += np.where((grain > (0.88 if quiet else 0.8)) & (lv >= base - 1), -1.0, 0.0)
    return rid, lv


def tex_tiles(h, w, seed, rid, size=8, off=4, base=3.0):
    """方砖地：size 见方的砖，1px 灰缝（只比砖面暗一档），零星几块深浅砖。"""
    ys = (np.arange(h)[:, None] + off)
    xs = (np.arange(w)[None, :] + off)
    tid = (ys // size) * 1000 + (xs // size)
    # 深浅砖只零星几块：一成的暗砖在殿里满地跳，像棋盘
    lv = base + qtone(pix.hash01(tid, 3, seed), 0.03, 0.985)
    yi = ys % size
    xi = xs % size
    lv = np.where((yi == 0) | (xi == 0), base - 1, lv)
    return rid, np.broadcast_to(lv, (h, w)).astype(np.float32).copy()


def tex_tilled(h, w, seed):
    """翻土：横向垄沟，5px 一个周期（垄顶、垄身、垄坡、沟底）。垄顶随机缀土块、
    沟底随机断开，免得像一排木板。"""
    ys = np.arange(h)[:, None]
    yi = ys % 5
    lv = np.select([yi == 0, yi == 1, yi == 2, yi == 3, yi == 4], [3, 3, 2, 1, 2]).astype(np.float32)
    lv = np.broadcast_to(lv, (h, w)).copy()
    xs, yv = jitter_points(h, w, 3, 0.35, seed + 5)
    ridge = (yv % 5) <= 1
    stamp(lv, xs[ridge], yv[ridge], [(0, 0, 1.0)])
    clod = pix.value_noise(h, w, 2.0, seed + 6)
    lv = np.where((clod > 0.8) & (np.broadcast_to(yi, (h, w)) == 3), 2.0, lv)
    return mm.dirt(), lv


def tex_scorched(h, w, seed):
    rid = mm.rock("cave")
    lv = 2.0 + patches(h, w, seed, 0.62, 0.34)
    crack = pix.value_noise(h, w, 3.0, seed + 3)
    lv += np.where(crack > 0.82, -1.0, 0.0)
    xs, ys = jitter_points(h, w, 7, 0.3, seed + 7)
    stamp(lv, xs, ys, PEBBLE)
    return rid, lv


def tex_cave(h, w, seed, base=2.0):
    rid = mm.rock("cave")
    lv = base + patches(h, w, seed, 0.64, 0.32)
    crack = pix.value_noise(h, w, 2.5, seed + 3)
    lv += np.where(crack > 0.86, -1.0, 0.0)
    xs, ys = jitter_points(h, w, 6, 0.25, seed + 7)
    stamp(lv, xs, ys, PEBBLE)
    return rid, lv


def tex_carpet(h, w, seed):
    """草席 / 地毯：细密的经纬纹。"""
    ys = np.arange(h)[:, None]
    xs = np.arange(w)[None, :]
    weave = ((xs // 2 + ys // 2) % 2).astype(np.float32)
    lv = 2.0 + weave
    return mm.cloth("red"), np.broadcast_to(lv, (h, w)).astype(np.float32).copy()


# ---------------------------------------------------------------------------
# 主过程
# ---------------------------------------------------------------------------
def pixel_surface(sc: Scene, seed: int) -> np.ndarray:
    """逐像素的地面类型：硬质按格切齐，软质按插值场 + 噪声取最大者。"""
    H, W = sc.H * T, sc.W * T
    ps = pix.upsample_cells(sc.surface, T, smooth=False).astype(np.int32)
    present = [int(v) for v in np.unique(sc.surface) if SURFACES[int(v)] in SOFT]
    if len(present) < 2:
        return ps
    wobble = (pix.fbm(H, W, seed + 21, ((9, 0.6), (4, 0.4))) - 0.5) * 0.55
    soft_px = np.isin(ps, present)
    best = np.full((H, W), -9.0, np.float32)
    choice = ps.copy()
    for k, code in enumerate(present):
        f = pix.upsample_cells(sc.surface == code, T, smooth=True)
        # 路比草「强势」一点：交界处路面略往草里伸，而不是被草啃掉
        bias = 0.04 if SURFACES[code] in ("dirt_road", "cave_path") else 0.0
        f = f + (wobble if k % 2 == 0 else -wobble) + bias
        better = f > best
        best = np.where(better, f, best)
        choice = np.where(better, code, choice)
    return np.where(soft_px, choice, ps)


def plank_orientation(sc: Scene) -> np.ndarray:
    """栈桥 / 码头木板的走向：窄长的一段板子横着铺（垂直于走的方向）。"""
    from mapinfer import label4
    m = sc.surface == S["plank"]
    vert = np.zeros_like(m)
    lab, n = label4(m)
    for i in range(n):
        ys, xs = np.nonzero(lab == i)
        w_ = xs.max() - xs.min() + 1
        h_ = ys.max() - ys.min() + 1
        if w_ > h_:
            vert[lab == i] = True
    return vert


def paint_ground(sc: Scene, cv: pix.Canvas) -> np.ndarray:
    seed = pix.seed_of(sc.md.map_id, "ground")
    H, W = sc.H * T, sc.W * T
    ps = pixel_surface(sc, seed)
    th = sc.th
    paving = mm.paving(th["paving"])
    for code in np.unique(ps):
        name = SURFACES[int(code)]
        m = ps == code
        s = seed + int(code) * 977
        if name == "grass":
            rid, lv = tex_grass(H, W, s, sc.season)
        elif name in ("dirt_road",):
            rid, lv = tex_dirt(H, W, s, 3.0)
        elif name in ("earth", "courtyard"):
            # 夯土院子比土路亮小半档：挪渐层，不用小数明度级（3.4 会抖成满地棋盘格）
            rid, lv = tex_dirt(H, W, s, 3.0)
            rid = mm.shifted(rid, 0.4)
        elif name == "sand":
            rid, lv = tex_sand(H, W, s)
        elif name == "gravel":
            rid, lv = tex_scree(H, W, s, mm.paving(th["paving"]), 3.0)
        elif name == "scree":
            rid, lv = tex_scree(H, W, s, mm.rock(th["rock"] if th["rock"] != "earth" else "cliff"), 2.0)
        elif name == "stone_road" and th["paving"] == "manor":
            # 府邸院子铺大方砖：错缝的长条石板在院墙、粉墙边上读起来就是一面砌砖墙
            rid, lv = tex_tiles(H, W, s, paving, size=14, off=5, base=4.0)
        elif name == "stone_road":
            rid, lv = tex_slabs(H, W, s, paving)
        elif name == "flagstone":
            rid, lv = tex_flagstone(H, W, s, paving if sc.th["kind"] != "cave" else mm.rock("cave"))
        elif name == "steps":
            rid, lv = tex_steps(H, W, s, paving)
        elif name == "plank":
            vert = pix.upsample_cells(plank_orientation(sc), T, smooth=False) > 0.5
            rid, lv_h = tex_planks(H, W, s, mm.floorwood(), False)
            _, lv_v = tex_planks(H, W, s + 1, mm.floorwood(), True)
            lv = np.where(vert, lv_v, lv_h)
        elif name == "wood_floor":
            # 屋里的地板：板缝浅（只暗一档），板面不分上下两截——满屋子一道道深缝会成条纹噪点
            rid, lv = tex_planks(H, W, s, mm.shifted(mm.floorwood(), -0.4), False, 3.0, seam=1, lower=0, quiet=True)
        elif name == "stone_floor":
            rid, lv = tex_tiles(H, W, s, mm.paving("sect" if th["paving"] != "manor" else "manor"), 8, 4)
        elif name == "tile_floor":
            # 屋里的砖地：12px 大方砖、浅缝、比原先亮一档（上一版 4px 一行的小条砖，满地读成砖墙）
            rid, lv = tex_tiles(H, W, s, mm.shifted(mm.brick(), -0.4), size=12, off=6, base=4.0)
        elif name == "tilled":
            rid, lv = tex_tilled(H, W, s)
        elif name == "scorched":
            rid, lv = tex_scorched(H, W, s)
        elif name == "cave_floor":
            rid, lv = tex_cave(H, W, s, 2.0)
        elif name == "cave_path":
            rid, lv = tex_cave(H, W, s, 3.0)
        elif name == "carpet":
            rid, lv = tex_carpet(H, W, s)
        elif name == "water":
            continue  # 水面归 mapwater
        else:
            raise ValueError("地面类型 %s 没有画法" % name)
        cv.paint(0, 0, m, rid, lv)

    # 路沿：硬质路面与软质地面交界处压一线暗；软质路面边缘一线暗、再往里半级
    soft = np.isin(ps, [S[n] for n in SOFT if n in S])
    hard_road = np.isin(ps, [S["stone_road"], S["flagstone"], S["steps"], S["plank"]])
    rim = hard_road & pix.dilate(soft, 1)
    cv.shade(0, 0, -1.0, rim)
    road = np.isin(ps, [S["dirt_road"], S["cave_path"]])
    edge1 = road & pix.dilate(~road & soft, 1)
    edge2 = road & pix.dilate(~road & soft, 2) & ~edge1
    cv.shade(0, 0, -1.0, edge1)
    cv.shade(0, 0, -0.5, edge2)
    # 草地一侧伸几根草叶到路面上：交界处因此是「草压着路」，不是一刀切
    grass = ps == S["grass"]
    rnd = pix.hash01(np.arange(W)[None, :], np.arange(H)[:, None], seed + 5)
    blade1 = (rim | edge1) & pix.dilate(grass, 1) & (rnd < 0.3)
    blade2 = edge2 & (rnd < 0.1)
    if blade1.any():
        cv.paint(0, 0, blade1, mm.grass(sc.season), 3.0)
    if blade2.any():
        cv.paint(0, 0, blade2, mm.grass(sc.season), 2.0)
    return ps
