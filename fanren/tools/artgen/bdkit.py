"""战斗背景与标题背景的小工具：天空渐变、横向无缝的山脊线、积云、透视地面、剪影。

画布仍是「渐层 + 明度级」（pix.Canvas），与地图同一套色板与抖动——战斗里站在前景的人物
与背景是同一种像素质感，不会一个是像素画、一个是糊的渐变图。

层与视差：每套背景四层 320×180（sky / far / mid / ground），运行时 ×4 放大到 1280×720。
sky、far、mid 三层横向首尾相接（运行时做视差平移与云雾漂移会卷回来），ground 层有透视
（消失点在画面正中），不平移。站位带在画面 55%–85% 高（y≈99–153），中央不放抢眼的东西。
"""

from __future__ import annotations

import numpy as np

import mapmat as mm
import pix
from palette import c, mix

W, H = 320, 180
HORIZON = 96          # 地平线（ground 层从这里往下）
STAND_Y0, STAND_Y1 = 99, 153


def canvas(opaque: bool = False) -> pix.Canvas:
    return pix.Canvas(W, H, opaque=opaque)


def ramp(name: str, cols) -> int:
    return pix.ramp_id("bd_" + name, [tuple(int(v) for v in col) for col in cols])


# ---------------------------------------------------------------------------
# 天空
# ---------------------------------------------------------------------------
SKIES = {
    "day": [mix("water2", "ink3", 0.3), c("water2"), mix("water3", "water2", 0.4), c("water3"),
            mix("water4", "water3", 0.4), c("water4"), mix("water5", "paper", 0.3)],
    "dusk": [mix("violet0", "ink1", 0.3), c("violet1"), mix("violet2", "red2", 0.4), mix("red3", "violet2", 0.3),
             mix("red4", "gold2", 0.4), c("gold3"), mix("gold4", "paper", 0.4)],
    "night": [c("ink0"), mix("ink0", "water0", 0.5), c("water0"), mix("water0", "water1", 0.5), c("water1"),
              mix("water1", "violet1", 0.5), mix("water2", "ink4", 0.4)],
    "dawn": [mix("violet0", "water0", 0.5), c("violet1"), mix("violet2", "water2", 0.4), mix("blossom1", "violet2", 0.4),
             c("blossom2"), mix("gold3", "blossom2", 0.4), mix("gold4", "paper", 0.5)],
    "soul": [c("ink0"), c("violet0"), mix("violet0", "jade0", 0.4), c("violet1"), mix("violet2", "jade1", 0.4),
             c("jade2"), c("jade3")],
}


def sky(time: str, top_lv: float = 0.5, bottom_lv: float = 5.5, y1: int = HORIZON + 8) -> pix.Canvas:
    """竖向渐变天空（抖动过渡），y1 以下铺满最亮一档（会被远景盖住）。"""
    cv = canvas(opaque=True)
    rid = ramp("sky_" + time, SKIES[time])
    ys = np.arange(H, dtype=np.float32)[:, None]
    t = np.clip(ys / y1, 0, 1)
    lv = top_lv + (bottom_lv - top_lv) * t ** 1.2
    cv.paint(0, 0, np.ones((H, W), bool), rid, np.broadcast_to(lv, (H, W)).astype(np.float32))
    return cv


def stars(cv: pix.Canvas, seed: int, y1: int, rid: int, n: int = 70) -> None:
    g = np.random.default_rng(seed)
    for _ in range(n):
        x, y = int(g.integers(0, W)), int(g.integers(0, y1))
        cv.px(x, y, rid, 5 if g.random() < 0.3 else 4)


def moon(cv: pix.Canvas, cx: int, cy: int, r: int) -> None:
    rid = ramp("moon", [c("ink6"), c("ink7"), c("ink8"), c("paper"), mix("paper", "gold4", 0.3)])
    yy, xx = np.mgrid[-r - 3:r + 4, -r - 3:r + 4].astype(np.float32)
    d = np.sqrt(xx * xx + yy * yy)
    halo = (d <= r + 3) & (d > r)
    cv.shade(cx - r - 3, cy - r - 3, 0.6, halo)
    disc = d <= r
    lv = np.clip(np.round(3.6 - (xx + yy) / r * 0.8), 1, 4)
    spots = pix.value_noise(disc.shape[0], disc.shape[1], 3.0, 7) > 0.66
    lv = np.where(spots, lv - 1, lv)
    cv.paint(cx - r - 3, cy - r - 3, disc, rid, lv.astype(np.float32))


def sun_glow(cv: pix.Canvas, cx: int, cy: int, r: int, k: float = 1.2) -> None:
    yy, xx = np.mgrid[0:H, 0:W].astype(np.float32)
    d = np.sqrt((xx - cx) ** 2 + (yy - cy) ** 2)
    add = np.clip(1.0 - d / r, 0, 1) ** 1.5 * k
    cv.shade(0, 0, add.astype(np.float32), add > 0.05)


# ---------------------------------------------------------------------------
# 云：一团团积云（横向无缝）
# ---------------------------------------------------------------------------
def clouds(cv: pix.Canvas, seed: int, y0: int, y1: int, rid: int, count: int = 9,
           rmin: float = 5.0, rmax: float = 11.0, base: float = 4.0, flat: float = 1.8) -> None:
    g = np.random.default_rng(seed)
    for _ in range(count):
        cx = g.random() * W
        cy = y0 + g.random() * (y1 - y0)
        puffs = int(g.integers(4, 8))
        span = g.random() * 26 + 18
        pts = []
        for k in range(puffs):
            px = cx + (k / max(1, puffs - 1) - 0.5) * span
            r = rmin + g.random() * (rmax - rmin) * (1.0 - abs(k / max(1, puffs - 1) - 0.5))
            pts.append((cy - r * 0.35 + g.random() * 2, px, r))
        pts.sort()
        for py, px, r in pts:
            n = int(np.ceil(r)) + 1
            yy, xx = np.mgrid[-n:n + 1, -n:n + 1].astype(np.float32)
            d = np.sqrt(xx * xx / flat + yy * yy)
            disc = (d <= r) & (yy <= r * 0.5)
            lv = np.clip(np.round(base - yy / r * 1.2 - xx / r * 0.4), base - 2, 5)
            for dx in (-W, 0, W):
                cv.paint(int(round(px)) - n + dx, int(round(py)) - n, disc, rid, lv.astype(np.float32))


# ---------------------------------------------------------------------------
# 山脊（横向无缝）
# ---------------------------------------------------------------------------
def ridge_line(seed: int, base: float, amp: float, cells=((64, 0.6), (24, 0.3), (9, 0.1))) -> np.ndarray:
    """每列一个山脊高度（y），横向首尾相接。"""
    n = np.zeros(W, np.float32)
    tot = 0.0
    for i, (cell, wt) in enumerate(cells):
        n += wt * pix.value_noise(1, W, cell, seed + i * 11, wrap_x=True)[0]
        tot += wt
    n /= tot
    return base - (n - 0.5) * 2 * amp


def mountains(cv: pix.Canvas, seed: int, rid: int, base: float, amp: float, lv_top: float = 3.0,
              lv_body: float = 2.0, snow: bool = False, cells=((64, 0.6), (24, 0.3), (9, 0.1))) -> np.ndarray:
    """一道山：山脊线以下填满，受光面（脊线左侧上坡）亮一档，山体往下渐暗。返回脊线。"""
    ridge = ridge_line(seed, base, amp, cells)
    ys = np.arange(H, dtype=np.float32)[:, None]
    body = ys >= ridge[None, :]
    slope = np.roll(ridge, 1) - ridge          # >0：往右升高 → 朝左上，受光
    lit = (slope[None, :] > 0.15) & (ys < ridge[None, :] + 6)
    depth = np.clip((ys - ridge[None, :]) / 40.0, 0, 1)
    lv = lv_body - depth * 0.8
    lv = np.where(lit, lv_top, lv)
    lv = np.where(ys < ridge[None, :] + 1, lv_top + (1 if snow else 0), lv)
    cv.paint(0, 0, body, rid, np.broadcast_to(lv, (H, W)).astype(np.float32))
    return ridge


def haze(cv: pix.Canvas, y0: int, y1: int, amount: float) -> None:
    """近地平线的一层雾：提亮若干级（抖动）。"""
    ys = np.arange(H, dtype=np.float32)[:, None]
    t = np.clip((ys - y0) / max(1, y1 - y0), 0, 1)
    add = np.broadcast_to(np.sin(t * np.pi) * amount, (H, W)).astype(np.float32)
    cv.shade(0, 0, add, (add > 0.05) & cv.on)


# ---------------------------------------------------------------------------
# 透视地面
# ---------------------------------------------------------------------------
def perspective_uv(horizon: int = HORIZON, cx: float = W / 2, scale: float = 64.0):
    """地面像素 → 世界坐标 (u 横, w 纵深)。越近（越往下）世界坐标变化越慢——近大远小。"""
    ys = np.arange(H, dtype=np.float32)[:, None]
    xs = np.arange(W, dtype=np.float32)[None, :]
    v = np.maximum(ys - horizon + 1.0, 0.6)
    w = scale * 18.0 / v
    u = (xs - cx) / v * 18.0
    return np.broadcast_to(u, (H, W)), np.broadcast_to(w, (H, W)), np.broadcast_to(ys >= horizon, (H, W))


def _int_base(rid: int, base: float) -> tuple[int, float]:
    """小数的基准明度级换成「整数级 + 挪过的渐层」：整片地面用 2.6 这种级数会被抖动成满地棋盘格。"""
    fb = float(np.floor(base))
    t = base - fb
    return (mm.shifted(rid, t) if t > 0.01 else rid), fb


def _cell_px(horizon: int) -> tuple[np.ndarray, np.ndarray]:
    """每一行上，世界坐标一个单位横向、纵深各占几像素（越近越大）。"""
    ys = np.arange(H, dtype=np.float32)[:, None]
    v = np.maximum(ys - horizon + 1.0, 1.0)
    return v / 18.0, v * v / (64.0 * 18.0)


def _haze(ys_from_horizon: np.ndarray) -> np.ndarray:
    """地平线处一窄条抖动的暗（远处的空气）；只有 8px 高——整片地面按远近抖动过渡就是满屏麻点。"""
    return np.clip(1.0 - ys_from_horizon / 8.0, 0, 1)


def ground_tiles(cv: pix.Canvas, rid: int, tile_u: float, tile_w: float, seed: int, base: float = 3.0,
                 horizon: int = HORIZON, bond: bool = True, mortar: float = 1.5) -> None:
    """透视石板 / 地板：按世界坐标切块，块缝暗，每块整档明暗。
    一块板在画面上窄于 3px 的地方（近地平线）不再画缝与深浅块——那里一像素里挤着好几块板，
    逐像素取样只会得到一片噪点；远处就是一片平的，交给地平线那一条雾。"""
    rid, base = _int_base(rid, base)
    u, w, m = perspective_uv(horizon)
    row = np.floor(w / tile_w)
    off = np.where(bond, (row % 2) * 0.5, 0.0)
    col = np.floor(u / tile_u + off)
    fu = u / tile_u + off - col
    fw = w / tile_w - row
    pu, pw = _cell_px(horizon)
    su, sw = tile_u * pu, tile_w * pw          # 一块板在这一行上的像素宽、高
    tone = pix.hash01(col.astype(np.int64), row.astype(np.int64), seed)
    detail = (su >= 3.0) & (sw >= 3.0)
    lv = base + np.where(detail & (tone < 0.12), -1.0, 0.0) + np.where(detail & (tone > 0.9), 1.0, 0.0)
    # 缝的像素宽度随远近变：近处缝宽、远处细到只剩一线
    edge_u = np.minimum(fu, 1 - fu) * su
    edge_w = np.minimum(fw, 1 - fw) * sw
    seam = ((edge_u < 0.8) & (su >= 3.0)) | ((edge_w < 0.7) & (sw >= 3.0))
    lv = np.where(seam, base - int(round(mortar)), lv)
    ys = np.arange(H, dtype=np.float32)[:, None]
    lv = lv - _haze(ys - horizon)
    cv.paint(0, 0, m, rid, lv.astype(np.float32))


def ground_noise(cv: pix.Canvas, rid: int, seed: int, base: float = 3.0, horizon: int = HORIZON,
                 speck: float = 0.2) -> None:
    """透视软质地面（土、沙、草）：世界坐标里撒斑点，近处斑点大、远处细。
    斑点在画面上小于 2px 的地方越往远越稀，到地平线附近就只剩底色——
    不然一像素里挤着好几颗斑点，逐像素取样是一片雪花（演武场沙地第一版就是这样）。"""
    rid, base = _int_base(rid, base)
    u, w, m = perspective_uv(horizon)
    pu, pw = _cell_px(horizon)
    cell_n = pix.hash01((u * 1.5).astype(np.int64), (w * 1.5).astype(np.int64), seed)
    big = pix.hash01((u * 0.25).astype(np.int64), (w * 0.25).astype(np.int64), seed + 1)
    fine = np.clip((np.minimum(pu, pw) / 1.5 - 1.0) / 1.5, 0, 1)      # 斑点 ≥1.5px 起淡入、≥3.75px 满额
    coarse = np.clip((np.minimum(pu, pw) * 4.0 / 1.5 - 1.0) / 1.5, 0, 1)
    k = speck * fine
    lv = base + np.where(cell_n < k, -1.0, 0.0) + np.where(cell_n > 1 - k * 0.6, 1.0, 0.0)
    lv = lv + np.where(big < 0.2 * coarse, -1.0, 0.0)
    ys = np.arange(H, dtype=np.float32)[:, None]
    lv = lv - _haze(ys - horizon)
    cv.paint(0, 0, m, rid, lv.astype(np.float32))


def tufts(cv: pix.Canvas, rid: int, seed: int, horizon: int = HORIZON, n: int = 120, x_avoid=None) -> None:
    """草簇：近处高、远处矮。x_avoid=(x0,x1) 这段不放（站位中央留干净）。"""
    g = np.random.default_rng(seed)
    for _ in range(n):
        y = int(horizon + 2 + (H - horizon - 2) * g.random() ** 0.7)
        x = int(g.integers(0, W))
        if x_avoid and x_avoid[0] < x < x_avoid[1] and STAND_Y0 < y < STAND_Y1:
            continue
        hgt = 1 + int((y - horizon) / 18)
        for k in range(-1, 2):
            for j in range(hgt + (1 if k == 0 else 0)):
                cv.px(x + k * 2, y - j, rid, 4 if j == hgt - 1 else 3)
        cv.px(x, y + 1, rid, 1)


# ---------------------------------------------------------------------------
# 剪影与近景
# ---------------------------------------------------------------------------
def pine_silhouette(cv: pix.Canvas, x: int, base_y: int, h: int, rid: int, lv: float = 1.0, seed: int = 0) -> None:
    """一棵松的剪影（远景 / 中景用）：一根干、几层扁平的针叶云，左上沿亮一线。"""
    g = np.random.default_rng(seed + (x % W) * 7 + base_y)
    bark = mm.bark()
    cv.rect(x, base_y - h, 2, h, bark, 1)
    layers = max(3, h // 9)
    for k in range(layers):
        y = base_y - h + int(k * h * 0.8 / layers) + 3
        half = int((h * 0.28) * (0.45 + 0.55 * (k + 1) / layers))
        ox = int(g.integers(-2, 3))
        for yy in range(-2, 3):
            ww = half - abs(yy) * 2
            if ww <= 0:
                continue
            for dx in (-W, 0, W):
                cv.rect(x - ww + ox + dx, y + yy, ww * 2 + 2, 1, rid, lv + (1 if yy == -2 else 0))


def broad_tree(cv: pix.Canvas, x: int, base_y: int, r: int, rid: int, seed: int, lv_base: float = 2.8) -> None:
    """阔叶树（中景）：几团叶簇，左上亮；一截树干。"""
    g = np.random.default_rng(seed)
    bark = mm.bark()
    cv.rect(x - 1, base_y - r, 3, r, bark, 2)
    cv.rect(x - 1, base_y - r, 1, r, bark, 3)
    for k in range(6):
        cx = x + g.normal(0, r * 0.45)
        cy = base_y - r * 1.2 + g.normal(0, r * 0.3)
        rr = r * (0.45 + g.random() * 0.25)
        n = int(np.ceil(rr)) + 1
        yy, xx = np.mgrid[-n:n + 1, -n:n + 1].astype(np.float32)
        d = np.sqrt(xx * xx + yy * yy)
        disc = d <= rr
        lvv = np.clip(np.round(lv_base - (xx + yy) / rr * 1.1), 1, 5)
        lvv = np.where(d > rr - 1, 1.0, lvv)
        leaf = pix.hash01((xx + cx).astype(np.int64) // 2, (yy + cy).astype(np.int64) // 2, seed) < 0.2
        lvv = np.where(leaf & (lvv >= 2), lvv + 1, lvv)
        cv.paint(int(cx) - n, int(cy) - n, disc, rid, lvv.astype(np.float32))


def chinese_roof(cv: pix.Canvas, x0: int, x1: int, y_ridge: int, y_eave: int, rid: int, upturn: int = 3,
                 lv: float = 2.0) -> None:
    """一座中式屋顶的正面剪影：屋脊一道、坡面瓦垄竖纹、檐口两端上翘。"""
    for y in range(y_ridge, y_eave + 1):
        t = (y - y_ridge) / max(1, y_eave - y_ridge)
        inset = int(round((1 - t) * (x1 - x0) * 0.12))
        xa, xb = x0 + inset, x1 - inset
        for x in range(xa, xb):
            xm = x % 4
            cv.px(x, y, rid, lv + [-1, 1, 0.5, 0][xm] if y > y_ridge + 1 else lv + 1.5)
    cv.rect(x0 + int((x1 - x0) * 0.12) - 2, y_ridge - 1, int((x1 - x0) * 0.76) + 4, 2, rid, lv - 1)
    cv.rect(x0, y_eave, x1 - x0, 1, rid, 0)
    for k in range(upturn):
        cv.px(x0 - 1 - k, y_eave - k, rid, lv)
        cv.px(x1 + k, y_eave - k, rid, lv)


def save_layers(layers: dict, out_dir, fast: bool = False) -> list:
    out = []
    for name in ("sky", "far", "mid", "ground", "near"):
        if name in layers:
            cv = layers[name]
            out.append(pix.save_png(cv.to_array(True), out_dir / (name + ".png"), fast))
    return out
