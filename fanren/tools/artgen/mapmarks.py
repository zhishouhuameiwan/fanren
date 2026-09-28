"""地面痕迹（maps.json 的 marks）：画在可走地面上的平贴花，给大片空地一点「有人用过」的样子。

脚印、拖痕、零星碎石与草、兵器架 / 箭靶 / 旗杆投在地上的长影。守两条：
  * 全是贴地的：只把地面压暗 / 提亮一档，或画 1–2px 的小石子、矮草，不立起任何东西——
    看上去仍是能走的地，碰撞一格不动；
  * 只落在 marks 指定的方框里、可走、不是实体（道具、墙）的像素上。

maps.json 写法（格坐标）：
    "marks": [{"rect": [x0, y0, x1, y1], "trails": 6, "drags": 3, "pebbles": 40, "tufts": 30,
               "pole_shadows": [[x, y], ...], "why": "..."}]
pole_shadows 是旗杆影的起点格（旗杆在墙外，只有影子横过场地落进来）。
兵器架与箭靶的长影按方框里的道具自动画。
"""

from __future__ import annotations

import numpy as np

import mapmat as mm
import pix
from mapinfer import rect_mask

T = 16
# 左上光：影子往右下落。长影（兵器架、旗杆）取午后偏斜的方向，比统一投影长得多。
SUN = np.array([1.0, 0.5]) / np.hypot(1.0, 0.5)


class Marks:
    """暗 / 亮两张掩码：各种痕迹先记下来，最后一次压暗或提亮一档——
    两道痕迹交叠处不会叠成两档暗的黑点。"""

    def __init__(self, ok: np.ndarray) -> None:
        self.ok = ok
        self.dark = np.zeros_like(ok)
        self.light = np.zeros_like(ok)

    def put(self, x: float, y: float, dark: bool = True) -> None:
        xi, yi = int(round(x)), int(round(y))
        if 0 <= yi < self.ok.shape[0] and 0 <= xi < self.ok.shape[1] and self.ok[yi, xi]:
            (self.dark if dark else self.light)[yi, xi] = True


def paint_marks(ctx) -> None:
    sc = ctx.sc
    for k, mk in enumerate(sc.preset.get("marks", [])):
        x0, y0, x1, y1 = mk["rect"]
        cells = rect_mask(sc.H, sc.W, x0, y0, x1, y1) & sc.walk & ~sc.water
        ok = (pix.upsample_cells(cells, T, smooth=False) > 0.5) & ~ctx.solid_px
        g = np.random.default_rng(pix.seed_of(sc.md.map_id, "marks", k))
        m = Marks(ok)
        for _ in range(int(mk.get("trails", 0))):
            footprints(m, g)
        for _ in range(int(mk.get("drags", 0))):
            drag_mark(m, g)
        for px_, py_ in mk.get("pole_shadows", []):
            pole_shadow(m, px_ * T, py_ * T + T // 2, g)
        long_shadows(ctx, m, (x0, y0, x1, y1))
        ctx.below.shade(0, 0, -1.0, m.dark)
        ctx.below.shade(0, 0, 1.0, m.light & ~m.dark)
        pebbles(ctx, ok & ~m.dark, g, int(mk.get("pebbles", 0)))
        tufts(ctx, ok & ~m.dark, g, int(mk.get("tufts", 0)))


def _pick(ok: np.ndarray, g) -> tuple[float, float]:
    ys, xs = np.nonzero(ok)
    i = int(g.integers(0, len(xs)))
    return float(xs[i]), float(ys[i])


def footprints(m: Marks, g) -> None:
    """一行脚印：左右交替，步幅 5px，走向慢慢拐弯；每只脚印是两像素的一小道凹。"""
    x, y = _pick(m.ok, g)
    ang = g.random() * 2 * np.pi
    for i in range(int(g.integers(14, 30))):
        ang += g.normal(0, 0.18)
        dx, dy = np.cos(ang), np.sin(ang)
        x += dx * 5
        y += dy * 5
        side = 1.5 if i % 2 else -1.5
        fx, fy = x - dy * side, y + dx * side
        m.put(fx, fy)
        m.put(fx + dx, fy + dy)


def drag_mark(m: Marks, g) -> None:
    """拖痕：拖过兵器、木桩留下的两道浅沟（暗），一侧沟外翻起一线沙（亮），缓缓弯。"""
    x, y = _pick(m.ok, g)
    ang = g.random() * 2 * np.pi
    for _ in range(int(g.integers(30, 64))):
        ang += g.normal(0, 0.04)
        dx, dy = np.cos(ang), np.sin(ang)
        x += dx
        y += dy
        m.put(x + dy * 2, y - dx * 2)
        m.put(x - dy * 2, y + dx * 2)
        m.put(x - dy * 3, y + dx * 3, dark=False)


def pole_shadow(m: Marks, X: int, Y: int, g) -> None:
    """旗杆影：一根 2px 的长影斜着横过场地，梢头挂一面旗的影（一小块，尾边波浪）。"""
    L = int(g.integers(90, 140))
    for t in range(L):
        m.put(X + SUN[0] * t, Y + SUN[1] * t)
        m.put(X + SUN[0] * t, Y + SUN[1] * t + 1)
    for t in range(L - 14, L - 3):
        for k in range(1, 7):
            wav = int(np.sin((t + k) * 0.7) * 1.2)
            m.put(X + SUN[0] * t + k * 0.3, Y + SUN[1] * t + 1 + k + wav)


def long_shadows(ctx, m: Marks, rect) -> None:
    """兵器架上一根根枪杆、箭靶的靶杆在地上投出斜长的影（统一投影只有两像素，
    ×3 下看不出是架子的影）。枪杆每 3px 一根。"""
    x0, y0, x1, y1 = rect
    for p in ctx.sc.props:
        if not (x0 <= p.x <= x1 and y0 <= p.y <= y1):
            continue
        base = (p.y + 1) * T - 2
        if p.kind in ("weapon_rack", "weapon_rack_row"):
            for X in range(p.x * T + 2, (p.x + p.w) * T - 1, 3):
                for t in range(2, 13):
                    m.put(X + SUN[0] * t, base + SUN[1] * t)
        elif p.kind == "target":
            X = p.x * T + 8
            for t in range(2, 16):
                m.put(X + SUN[0] * t, base + SUN[1] * t)
            cx, cy = X + SUN[0] * 18, base + SUN[1] * 18
            for yy in range(-2, 3):
                for xx in range(-4, 5):
                    if (xx / 4.2) ** 2 + (yy / 2.4) ** 2 <= 1.0:
                        m.put(cx + xx, cy + yy)


def pebbles(ctx, ok: np.ndarray, g, n: int) -> None:
    """零星碎石：两三颗一撮，每颗 2px（左上一点亮、右下一点影）。"""
    st = mm.rock("cliff")
    for _ in range(n):
        cx, cy = _pick(ok, g)
        for _ in range(int(g.integers(1, 4))):
            x = int(cx + g.integers(-3, 4))
            y = int(cy + g.integers(-2, 3))
            if not (0 <= y < ok.shape[0] - 1 and 0 <= x < ok.shape[1] - 1 and ok[y, x] and ok[y + 1, x + 1]):
                continue
            ctx.below.px(x, y, st, 4)
            ctx.below.px(x + 1, y, st, 3)
            ctx.below.shade(x + 1, y + 1, -1.0, np.ones((1, 1), bool))


def tufts(ctx, ok: np.ndarray, g, n: int) -> None:
    """零星几簇矮草（踩不死的场边草）：一茎两像素、两侧各一像素，贴地。"""
    grass = mm.grass(ctx.sc.season)
    for _ in range(n):
        x, y = (int(v) for v in _pick(ok, g))
        if not (2 <= y < ok.shape[0] - 1 and 1 <= x < ok.shape[1] - 1 and ok[y - 2:y + 1, x - 1:x + 2].all()):
            continue
        ctx.below.px(x, y - 1, grass, 4)
        ctx.below.px(x, y, grass, 3)
        ctx.below.px(x - 1, y, grass, 2)
        ctx.below.px(x + 1, y, grass, 2)
        ctx.below.shade(x, y + 1, -1.0, np.ones((1, 1), bool))
