"""战斗特效帧：刀光、钝击、火弹爆裂、风旋、毒雾、治愈、蓄劲气焰、首领蓄势气焰。

全部程序生成（解析几何 + 整数哈希噪声），alpha 量化到几级——这样放大四倍后
仍是「像素画」而不是一团糊；也保证每台机器逐像素一致。
刀光、钝击、风旋、毒雾、治愈是白/灰度（运行时乘色：刀光乘金或乘朱、毒雾乘紫绿……）；
火弹、蓄劲气焰（金）、首领蓄势气焰（暗红）自带颜色。

朝向：刀光的弧背朝左（我方在右、向左砍）；敌方出招时运行时水平翻转。
"""

from __future__ import annotations

import math

from PIL import Image

import palette as P
from fx_textures import hash01, smoothstep

ALPHA_LEVELS = (0, 56, 112, 168, 224, 255)


def qa(a: float) -> int:
    """把 alpha 量化到几级（像素画的「硬」）。"""
    a = max(0.0, min(255.0, a))
    best = 0
    for lv in ALPHA_LEVELS:
        if abs(lv - a) < abs(best - a):
            best = lv
    return best


def strip(frames: list[Image.Image]) -> Image.Image:
    w, h = frames[0].width, frames[0].height
    out = Image.new("RGBA", (w * len(frames), h), (0, 0, 0, 0))
    for i, f in enumerate(frames):
        out.paste(f, (i * w, 0))
    return out


def _white_frame(size: int, fn) -> Image.Image:
    img = Image.new("RGBA", (size, size), (255, 255, 255, 0))
    px = img.load()
    for y in range(size):
        for x in range(size):
            a = fn(x + 0.5, y + 0.5)
            if a > 0:
                v = 255
                px[x, y] = (v, v, v, qa(a))
    return img


def noise1(a: float, seed: int, period: int = 16) -> float:
    """一维周期值噪声（角度上用，首尾相接）。"""
    x = a * period
    i = math.floor(x)
    f = x - i
    f = f * f * (3 - 2 * f)
    v0 = hash01(i % period, 0, seed)
    v1 = hash01((i + 1) % period, 0, seed)
    return v0 + (v1 - v0) * f


# ---------------------------------------------------------------------------
# 刀光：月牙形的一道弧，从右上扫到左下，前缘最亮
# ---------------------------------------------------------------------------

def slash(size: int = 48) -> Image.Image:
    cx, cy = size * 0.62, size * 0.5
    R = size * 0.40
    specs = [
        # (起角, 止角, 内圆偏移, 亮度)
        (250.0, 205.0, 3.0, 1.0),
        (272.0, 120.0, 6.0, 1.0),
        (275.0, 110.0, 2.5, 0.7),
        (262.0, 125.0, 1.3, 0.38),
    ]
    frames = []
    for a0, a1, off, bright in specs:
        def fn(x: float, y: float, a0=a0, a1=a1, off=off, bright=bright) -> float:
            do = math.hypot(x - cx, y - cy)
            di = math.hypot(x - (cx + off), y - cy)
            if do > R or di <= R:
                return 0.0
            th = math.degrees(math.atan2(y - cy, x - cx)) % 360.0
            if not (a1 <= th <= a0):
                return 0.0
            t = (a0 - th) / (a0 - a1)          # 0 尾 → 1 前缘
            edge = 1.0 - (R - do) / max(1.0, off + 0.5)   # 越靠外缘越亮
            return 255.0 * bright * (0.3 + 0.7 * t) * (0.45 + 0.55 * max(0.0, edge))

        frames.append(_white_frame(size, fn))
    return strip(frames)


# ---------------------------------------------------------------------------
# 钝击：一点星芒 → 一圈冲击环加八道速度线 → 碎环
# ---------------------------------------------------------------------------

def impact(size: int = 48) -> Image.Image:
    c = size / 2.0

    def f0(x: float, y: float) -> float:
        dx, dy = x - c, y - c
        d = math.hypot(dx, dy)
        th = math.atan2(dy, dx)
        star = 5.5 + 3.0 * abs(math.cos(4 * th))
        if d <= star:
            return 255.0 * (1.0 - 0.5 * smoothstep(0.0, star, d))
        if abs(d - 9.0) < 0.9:
            return 150.0
        return 0.0

    def f1(x: float, y: float) -> float:
        dx, dy = x - c, y - c
        d = math.hypot(dx, dy)
        th = math.degrees(math.atan2(dy, dx)) % 45.0
        a = 0.0
        if 12.0 <= d <= 15.0:
            a = 255.0 if d > 13.5 else 190.0
        if 17.0 <= d <= 22.0 and (th < 3.0 or th > 42.0):
            a = max(a, 220.0 * (1.0 - (d - 17.0) / 6.0))
        if d < 4.0:
            a = max(a, 120.0)
        return a

    def f2(x: float, y: float) -> float:
        dx, dy = x - c, y - c
        d = math.hypot(dx, dy)
        th = math.degrees(math.atan2(dy, dx)) % 30.0
        a = 0.0
        if 18.5 <= d <= 20.0 and th < 18.0:
            a = 140.0
        thl = math.degrees(math.atan2(dy, dx)) % 45.0
        if 21.0 <= d <= 23.0 and (thl < 2.5 or thl > 42.5):
            a = max(a, 90.0)
        return a

    return strip([_white_frame(size, f0), _white_frame(size, f1), _white_frame(size, f2)])


# ---------------------------------------------------------------------------
# 火弹爆裂（带颜色）
# ---------------------------------------------------------------------------

FIRE = [P.mix("gold4", "paper", 0.45), P.c("gold3"), P.mix("gold3", "red3", 0.5), P.c("red3"), P.c("red2"), P.c("red1")]
SMOKE = [P.c("stone2"), P.c("stone1"), P.mix("ink3", "red0", 0.3)]


def fireburst(size: int = 48) -> Image.Image:
    c = size / 2.0
    frames = []
    for f in range(4):
        img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
        px = img.load()
        for y in range(size):
            for x in range(size):
                dx, dy = x + 0.5 - c, y + 0.5 - c
                d = math.hypot(dx, dy)
                th = (math.atan2(dy, dx) / (2 * math.pi)) % 1.0
                n = noise1(th, 11 + f * 7, 12)
                n2 = noise1(th, 31 + f * 5, 24)
                col, a = None, 0
                if f == 0:
                    r = 6.0 + 2.0 * n
                    if d <= r * 0.5:
                        col, a = FIRE[0], 255
                    elif d <= r * 0.8:
                        col, a = FIRE[1], 255
                    elif d <= r:
                        col, a = FIRE[3], 230
                    elif d <= r + 2.0:
                        col, a = FIRE[4], 120
                elif f == 1:
                    r = 13.0 + 5.0 * n + 2.0 * n2
                    if d <= r * 0.35:
                        col, a = FIRE[0], 255
                    elif d <= r * 0.6:
                        col, a = FIRE[1], 255
                    elif d <= r * 0.8:
                        col, a = FIRE[2], 255
                    elif d <= r * 0.95:
                        col, a = FIRE[3], 240
                    elif d <= r:
                        col, a = FIRE[5], 200
                elif f == 2:
                    r = 18.0 + 4.0 * n + 2.0 * n2
                    inner = r * 0.55
                    if d <= inner:
                        k = hash01(x, y, 5)
                        col, a = (SMOKE[2], 150) if k > 0.35 else (FIRE[4], 170)
                    elif d <= r * 0.75:
                        col, a = FIRE[2], 230
                    elif d <= r * 0.9:
                        col, a = FIRE[3], 220
                    elif d <= r:
                        col, a = FIRE[5], 170
                else:
                    r = 20.0 + 4.0 * n
                    k = hash01(x // 2, y // 2, 9)
                    if d <= r and k > 0.55:
                        col, a = (SMOKE[0] if d < r * 0.6 else SMOKE[1]), 110
                    if abs(d - (r - 1.0)) < 1.0 and hash01(x, y, 13) > 0.8:
                        col, a = FIRE[1], 220
                if col is not None and a > 0:
                    px[x, y] = col + (a,)
        frames.append(img)
    return strip(frames)


# ---------------------------------------------------------------------------
# 风旋：三道螺旋弧，逐帧转
# ---------------------------------------------------------------------------

def wind(size: int = 48) -> Image.Image:
    c = size / 2.0
    frames = []
    for f in range(3):
        rot = f * 40.0

        def fn(x: float, y: float, rot=rot) -> float:
            dx, dy = x - c, y - c
            r = math.hypot(dx, dy)
            if r < 3.0 or r > 21.0:
                return 0.0
            th = math.degrees(math.atan2(dy, dx))
            best = 0.0
            for arm in range(3):
                th_s = rot + arm * 120.0 + (r - 3.0) * 11.0
                dth = (th - th_s + 180.0) % 360.0 - 180.0
                dist = abs(dth) * math.pi / 180.0 * r
                if dist < 1.2 and dth < 0 or dist < 0.7:
                    taper = 1.0 - smoothstep(12.0, 21.0, r)
                    best = max(best, 255.0 * (0.35 + 0.65 * taper))
            return best

        frames.append(_white_frame(size, fn))
    return strip(frames)


# ---------------------------------------------------------------------------
# 毒雾：一团团的雾泡，左上受光，逐帧翻滚胀大
# ---------------------------------------------------------------------------

_PUFFS = [
    [(24, 28, 8), (16, 30, 6), (31, 30, 6), (21, 22, 6), (28, 23, 5)],
    [(24, 27, 9), (14, 30, 7), (33, 29, 7), (20, 20, 7), (29, 19, 6), (24, 34, 5)],
    [(24, 26, 10), (12, 29, 7), (35, 28, 7), (18, 17, 7), (31, 16, 6), (24, 35, 6), (9, 22, 4), (39, 21, 4)],
]


def poison(size: int = 48) -> Image.Image:
    frames = []
    for f, puffs in enumerate(_PUFFS):
        img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
        px = img.load()
        for y in range(size):
            for x in range(size):
                best = None
                for (pxc, pyc, pr) in puffs:
                    d = math.hypot(x + 0.5 - pxc, y + 0.5 - pyc)
                    if d <= pr:
                        # 受光：朝左上的一侧更亮
                        lit = ((x + 0.5 - pxc) * -0.7 + (y + 0.5 - pyc) * -0.7) / pr
                        v = 200 + int(45 * max(-1.0, min(1.0, lit)))
                        edge = d > pr - 1.2
                        cand = (v - (40 if edge else 0), 150 if edge else 200 - 25 * f)
                        if best is None or cand[0] > best[0]:
                            best = cand
                if best is not None:
                    v, a = best
                    px[x, y] = (v, v, v, qa(a))
        frames.append(img)
    return strip(frames)


# ---------------------------------------------------------------------------
# 治愈：往上飘的十字光点
# ---------------------------------------------------------------------------

_HEAL_POINTS = [(14, 34), (24, 30), (33, 36), (19, 24), (29, 22), (24, 38), (11, 26), (37, 28)]


def heal(size: int = 48) -> Image.Image:
    frames = []
    for f in range(3):
        img = Image.new("RGBA", (size, size), (255, 255, 255, 0))
        px = img.load()
        for i, (x0, y0) in enumerate(_HEAL_POINTS):
            y = y0 - f * 6 - (i % 3) * 2
            fade = 1.0 - smoothstep(6.0, 30.0, 40.0 - y)
            big = (i + f) % 3 == 0
            pts = [(0, 0, 255)]
            if big:
                pts += [(1, 0, 190), (-1, 0, 190), (0, 1, 190), (0, -1, 190), (0, -2, 110), (0, 2, 110),
                        (2, 0, 110), (-2, 0, 110)]
            else:
                pts += [(1, 0, 120), (0, 1, 120)]
            for dx, dy, a in pts:
                xx, yy = x0 + dx, y + dy
                if 0 <= xx < size and 0 <= yy < size:
                    aa = qa(a * fade)
                    if aa > px[xx, yy][3]:
                        px[xx, yy] = (255, 255, 255, aa)
        if f == 0:
            # 脚下一圈柔光
            for y in range(size):
                for x in range(size):
                    d = math.hypot((x + 0.5 - 24) / 14.0, (y + 0.5 - 40) / 4.0)
                    if 0.75 < d <= 1.0:
                        px[x, y] = (255, 255, 255, max(px[x, y][3], 112))
        frames.append(img)
    return strip(frames)


# ---------------------------------------------------------------------------
# 气焰：蓄劲（金，三档）与首领蓄势（暗红）
# ---------------------------------------------------------------------------

AURA_W, AURA_H = 32, 40
AURA_FOOT = (16, 38)   # 人物脚底那一行（描边行）落在这一行
# 导出的锚点与人物表同一口径：人物的脚底点是「帧底边中点」（脚底描边行的下沿），
# 所以气焰的锚点是脚底行的下一行
AURA_ANCHOR = (AURA_FOOT[0], AURA_FOOT[1] + 1)


def _capsule_dist(x: float, y: float, top: float, bottom: float, cx: float, r: float) -> float:
    yy = max(top, min(bottom, y))
    return math.hypot(x - cx, y - yy) - r


def aura(level: int, frame: int, palette_kind: str) -> Image.Image:
    """人形外面一圈往上窜的火舌。level 1–3 决定圈厚与火舌高度；frame 0–2 火舌上移。

    做法：以包住人形的胶囊为距离场，再把距离场按「x 处的火舌高度」往下错位采样——
    等于把胶囊上半圈的边往上拉出一根根尖，尖的位置随帧沿 x 漂移，就是火苗在舔。
    """
    img = Image.new("RGBA", (AURA_W, AURA_H), (0, 0, 0, 0))
    px = img.load()
    if palette_kind == "boost":
        cols = [P.c("gold1"), P.c("gold2"), P.c("gold3"), P.mix("gold4", "paper", 0.55)]
        band = {1: 1.6, 2: 2.4, 3: 3.2}[level]
        tongue = {1: 4.0, 2: 7.0, 3: 10.0}[level]
        seed = 40 + level
    else:
        cols = [P.mix("violet0", "ink1", 0.2), P.c("red0"), P.c("red1"), P.c("red3")]
        band = 3.0
        tongue = 9.0
        seed = 90
    cx = AURA_FOOT[0]
    # 包住 16×24 人形的胶囊：人物脚底在 y=38，头顶约在 y=15
    top, bottom, r = 22.5, 31.5, 7.0
    for y in range(AURA_H):
        for x in range(AURA_W):
            fx_, fy_ = x + 0.5, y + 0.5
            if fy_ > AURA_FOOT[1] + 0.5:
                continue
            # 火舌高度：沿 x 的噪声取平方，峰更尖；随帧向一侧漂
            u = (fx_ / AURA_W + frame * 0.137) % 1.0
            n = noise1(u, seed, 7)
            lift = tongue * (n ** 2) * 1.6
            # 只有上半身往上拉（腰以下的边不动）
            upness = max(0.0, min(1.0, (top + 4.0 - fy_) / (top + 4.0 - 8.0)))
            d = _capsule_dist(fx_, fy_ + lift * upness, top, bottom, cx, r)
            if d > band:
                continue
            d0 = _capsule_dist(fx_, fy_, top, bottom, cx, r)   # 不拉火舌时的距离：<0 就在人身后
            if d < -1.0 and d0 > 0.0:
                # 火舌里面（人身之外）：实心的火，离身子越远越淡
                k0 = d0 / max(1.0, tongue)
                if k0 < 0.35:
                    c, a = cols[3], 200
                elif k0 < 0.7:
                    c, a = cols[2], 168
                else:
                    c, a = cols[1], 112
                px[x, y] = c + (a,)
                continue
            if d < -2.0:
                continue                           # 人身后不铺底：铺了就是一张垫在人后面的卡片
            if d < -1.0:
                px[x, y] = cols[2] + (56,)        # 贴着剪影内侧一圈淡光，人物边缘因此发亮
                continue
            k = (d + 1.0) / (band + 1.0)         # 0 贴身 → 1 最外
            flick = hash01(x, y + frame * 5, seed + 7)
            if k < 0.30:
                c, a = cols[3], 224
            elif k < 0.60:
                c, a = cols[2], 168
            elif k < 0.85:
                c, a = cols[1], 112
            else:
                c, a = cols[0], (56 if flick > 0.3 else 0)
            if a:
                px[x, y] = c + (a,)
    # 往上飘的火星：二、三档与首领才有
    if palette_kind == "boss" or level >= 2:
        count = 3 if palette_kind == "boss" else (level - 1) * 2
        for i in range(count):
            sx = int(6 + hash01(i, 0, seed + 20) * 20)
            sy0 = int(6 + hash01(i, 1, seed + 20) * 14)
            sy = (sy0 - frame * 4) % 22 + 1
            col = P.mix(cols[3], "paper", 0.5) if palette_kind == "boost" else cols[3]
            px[sx, sy] = col + (255,)
            if sy + 1 < AURA_H:
                px[sx, sy + 1] = cols[2] + (140,)
    return img


def boost_aura() -> Image.Image:
    """三档 × 三帧，排成三行：第 n 行是第 n 档。"""
    out = Image.new("RGBA", (AURA_W * 3, AURA_H * 3), (0, 0, 0, 0))
    for lv in (1, 2, 3):
        for f in range(3):
            out.paste(aura(lv, f, "boost"), (f * AURA_W, (lv - 1) * AURA_H))
    return out


def boss_aura() -> Image.Image:
    return strip([aura(2, f, "boss") for f in range(3)])
