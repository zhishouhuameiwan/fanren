"""柔光类特效贴图：光斑、柔圆、光束、可平铺的雾纹。

这些图运行时都是「乘色 + 加色混合」用的，所以一律白色，靠 alpha 表达形状；
渐变一律按像素中心取样的解析式算，不用 PIL 的滤镜——滤镜的实现细节随版本变，
而 ARTGEN_IN_SYNC 逐像素比对，差一个值就红。
"""

from __future__ import annotations

import math

from PIL import Image


def smoothstep(e0: float, e1: float, x: float) -> float:
    if e1 == e0:
        return 0.0 if x < e0 else 1.0
    t = max(0.0, min(1.0, (x - e0) / (e1 - e0)))
    return t * t * (3.0 - 2.0 * t)


def _q(a: float) -> int:
    """alpha 量化到 0–255（四舍五入，夹紧）。"""
    return max(0, min(255, int(round(a))))


def white(w: int, h: int, alpha_fn) -> Image.Image:
    img = Image.new("RGBA", (w, h), (255, 255, 255, 0))
    px = img.load()
    for y in range(h):
        for x in range(w):
            px[x, y] = (255, 255, 255, _q(alpha_fn(x + 0.5, y + 0.5)))
    return img


def glow(size: int = 64) -> Image.Image:
    """径向光斑：中心 255，平滑降到边缘 0（(1-r²)² 的钟形，边缘斜率也是 0，叠多层不出棱）。"""
    c = size / 2.0

    def a(x: float, y: float) -> float:
        r = math.hypot(x - c, y - c) / c
        if r >= 1.0:
            return 0.0
        return 255.0 * (1.0 - r * r) ** 2

    return white(size, size, a)


def softcircle(size: int = 32) -> Image.Image:
    """柔边圆：半径六成以内实心，外圈平滑淡出。粒子、光点、影子都用它。"""
    c = size / 2.0

    def a(x: float, y: float) -> float:
        r = math.hypot(x - c, y - c) / c
        return 255.0 * (1.0 - smoothstep(0.55, 1.0, r))

    return white(size, size, a)


def shaft(w: int = 32, h: int = 128) -> Image.Image:
    """光束：顶端最亮、往下渐隐；横向两边柔化。运行时旋转成斜射的体积光。"""
    cx = w / 2.0

    def a(x: float, y: float) -> float:
        across = 1.0 - smoothstep(0.15, 1.0, abs(x - cx) / cx)
        t = y / h
        along = smoothstep(0.0, 0.08, t) * (1.0 - t) ** 1.6
        return 230.0 * across * along

    return white(w, h, a)


def shadow(w: int = 24, h: int = 8) -> Image.Image:
    """脚下柔影：扁椭圆，中心 170。运行时乘黑色画在人物脚下。"""
    cx, cy = w / 2.0, h / 2.0

    def a(x: float, y: float) -> float:
        r = math.hypot((x - cx) / cx, (y - cy) / cy)
        return 170.0 * (1.0 - smoothstep(0.35, 1.0, r))

    return white(w, h, a)


# ---------------------------------------------------------------------------
# 可平铺的值噪声
# ---------------------------------------------------------------------------

def hash01(ix: int, iy: int, seed: int) -> float:
    """整数哈希 → [0,1)。纯整数运算，跨机器、跨 Python 版本一致。"""
    h = (ix * 374761393 + iy * 668265263 + seed * 2147483647) & 0xFFFFFFFF
    h = ((h ^ (h >> 13)) * 1274126177) & 0xFFFFFFFF
    h ^= h >> 16
    return (h & 0xFFFFFF) / float(0x1000000)


def value_noise_tile(x: float, y: float, period: int, seed: int) -> float:
    """周期为 period 的平滑值噪声（格点在整数上，坐标超出周期自动绕回）。"""
    x0, y0 = math.floor(x), math.floor(y)
    fx, fy = x - x0, y - y0
    sx, sy = fx * fx * (3 - 2 * fx), fy * fy * (3 - 2 * fy)

    def g(ix: int, iy: int) -> float:
        return hash01(ix % period, iy % period, seed)

    a = g(x0, y0) + (g(x0 + 1, y0) - g(x0, y0)) * sx
    b = g(x0, y0 + 1) + (g(x0 + 1, y0 + 1) - g(x0, y0 + 1)) * sx
    return a + (b - a) * sy


def fbm_tile(u: float, v: float, base_period: int, octaves: int, seed: int) -> float:
    """分形噪声，u/v ∈ [0,1)，每一层的周期都是整数，因而整张图四边无缝。"""
    total, amp, norm = 0.0, 1.0, 0.0
    period = base_period
    for o in range(octaves):
        total += amp * value_noise_tile(u * period, v * period, period, seed + o * 101)
        norm += amp
        amp *= 0.5
        period *= 2
    return total / norm


def fog_noise(size: int = 128, seed: int = 7) -> Image.Image:
    """柔噪声雾纹：白色，alpha 是分形噪声（对比拉开、底部压掉），上下左右都可无缝平铺。"""
    img = Image.new("RGBA", (size, size), (255, 255, 255, 0))
    px = img.load()
    vals = []
    for y in range(size):
        for x in range(size):
            vals.append(fbm_tile((x + 0.5) / size, (y + 0.5) / size, 4, 4, seed))
    lo, hi = min(vals), max(vals)
    for y in range(size):
        for x in range(size):
            n = (vals[y * size + x] - lo) / (hi - lo)
            a = smoothstep(0.30, 0.95, n)
            px[x, y] = (255, 255, 255, _q(a * 235.0))
    return img
