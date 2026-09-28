"""标题背景：assets/art/title/{sky,far,mid,near}.png，320×180，运行时做视差。

晨光里的云海：天是由深靛到金粉的晨色，远处两重山，山腰以下一片云海，几座近峰
从云里探出来；右下角压着一角飞檐亭台（近景，只露一个翘角、一根柱、一截栏杆），
左上斜进来一枝松。画面正中偏上留白——那里是「凡人修仙传」四个字与菜单。

sky / far / mid 横向首尾相接（云海可以慢慢漂），near 是一角亭台，不平移。
"""

from __future__ import annotations

from pathlib import Path

import numpy as np

import bdkit as bk
import mapmat as mm
import pix
from palette import c, mix

W, H = bk.W, bk.H


def render() -> dict:
    seed = pix.seed_of("title") % 100000
    # ---- 天：晨色 + 初升的日晕 + 几抹被照亮的薄云 ----
    sky = bk.sky("dawn", 0.3, 6.0, 120)
    bk.sun_glow(sky, 236, 104, 120, 1.6)
    thin = bk.ramp("title_cloud", [c("violet1"), mix("violet2", "blossom1", 0.5), c("blossom2"),
                                   mix("gold3", "blossom2", 0.5), c("gold4"), mix("gold4", "paper", 0.6)])
    g = np.random.default_rng(seed)
    for _ in range(9):
        y = int(g.integers(16, 70))
        x = int(g.integers(0, W))
        ln = int(g.integers(30, 80))
        for k in range(ln):
            yy = y + int(np.sin(k / 9.0) * 1.5)
            lv = 3.0 + (1 if k % 7 < 3 else 0)
            for dx in (-W, 0, W):
                sky.px(x + k + dx, yy, thin, lv)
                if k % 3 == 0:
                    sky.px(x + k + dx, yy + 1, thin, 2.0)
    # ---- 远山：两重，越远越淡 ----
    far = bk.canvas()
    back = bk.ramp("title_far1", [mix("violet1", "water1", 0.5), c("violet2"), mix("violet2", "blossom1", 0.4),
                                  mix("violet3", "blossom2", 0.4), mix("blossom2", "gold3", 0.4)])
    near_m = bk.ramp("title_far2", [c("violet0"), mix("violet1", "water0", 0.5), c("violet1"),
                                    mix("violet2", "water1", 0.4), mix("violet3", "gold2", 0.3)])
    bk.mountains(far, seed + 1, back, 78, 26, 4.0, 3.0, cells=((52, 0.6), (20, 0.3), (8, 0.1)))
    bk.mountains(far, seed + 2, near_m, 96, 22, 3.6, 2.4, cells=((40, 0.6), (16, 0.3), (7, 0.1)))
    bk.haze(far, 70, 118, 1.2)
    # ---- 中景：云海，几座近峰探出云头 ----
    mid = bk.canvas()
    peak = bk.ramp("title_peak", [c("ink1"), mix("pine", "ink1", 0.5), c("pine"), mix("pine", "violet2", 0.4),
                                  mix("jade1", "blossom2", 0.4)])
    # 近峰：左右两坡不对称、坡线参差；一道山脊把受光的左坡（亮）与背光的右坡（暗）分开。
    # （第一版是左右对称的平涂三角，远看像一个个绿锥子。）
    for i, (px, h, w) in enumerate(((40, 60, 46), (118, 44, 34), (286, 70, 52))):
        top = H - 50 - h
        for dx in (-W, 0, W):
            for y in range(top, H - 30):
                t = (y - top) / h
                jl = int(pix.hash01(y // 3, i, seed + 5) * 3)
                jr = int(pix.hash01(y // 3, i, seed + 6) * 3)
                left = int(w * 0.46 * min(1.0, t * 1.5) ** 0.75) + jl
                right = int(w * 0.54 * min(1.0, t * 1.3) ** 0.85) + jr
                ridge_x = px + dx + int(np.sin(t * 5.0 + i) * 2) - int(t * 4)
                mid.rect(px + dx - left, y, left + right, 1, peak, 2.0)
                mid.rect(px + dx - left, y, max(1, ridge_x - (px + dx - left)), 1, peak, 3.0)
                mid.px(px + dx - left, y, peak, 4.0)
                mid.rect(ridge_x + 1, y, max(0, px + dx + right - ridge_x - 1), 1, peak, 1.0)
    for px, base in ((40, 60), (286, 60)):
        for dx in (-W, 0, W):
            bk.pine_silhouette(mid, px + dx - 8, H - 50 - base + 8, 16, mm.foliage("pine"), 1.0, seed)
    cloud = bk.ramp("title_sea", [mix("violet1", "water1", 0.4), c("violet2"), mix("violet3", "blossom2", 0.5),
                                  c("blossom3"), mix("gold4", "paper", 0.5), c("paper")])
    sea = np.zeros((H, W), bool)
    sea[118:, :] = True
    mid.paint(0, 0, sea, cloud, 2.0)
    pts = []
    for gy in range(104, H + 10, 9):
        for gx in range(-12, W + 12, 12):
            pts.append((gy + g.random() * 8, gx + g.random() * 12, 7 + g.random() * 7))
    pts.sort()
    for cy, cx, r in pts:
        n = int(np.ceil(r)) + 1
        yy, xx = np.mgrid[-n:n + 1, -n:n + 1].astype(np.float32)
        d = np.sqrt(xx * xx / 1.8 + yy * yy)
        disc = (d <= r) & (yy <= r * 0.4)
        # 朝着日出的右上沿最亮；云海里最暗只到粉白：再暗的紫从团缝里漏出来是一粒粒脏点
        lv = np.clip(np.round(3.4 - yy / r * 1.5 + xx / r * 0.4 + (cy - 104) / 90.0), 3, 5)
        # 团的上沿一线最亮、下沿只暗到 3：下沿压到最暗那级的话，团与团交叠处满是紫色碎点
        lv = np.where(d > r - 1, np.where(yy < 0, 4.0, 3.0), lv)
        for dx in (-W, 0, W):
            mid.paint(int(cx) - n + dx, int(cy) - n, disc, cloud, lv.astype(np.float32))
    # ---- 近景：右下一角飞檐亭台，左上一枝松 ----
    near = bk.canvas()
    pavilion_corner(near)
    pine_bough(near, seed)
    return {"sky": sky, "far": far, "mid": mid, "near": near}


def pavilion_corner(near) -> None:
    """右下一角亭子：从画面右边斜伸进来的一片瓦坡，一道戗脊压着坡的上沿、到檐角高高翘起；
    檐下一溜暗的椽子头，一根朱柱，一截石栏。（第一版的瓦坡是一整块方格，左沿还是圆的，
    远看像一个蒙着格子布的圆顶，读不出「翘角」。）"""
    rr = mm.roof("black")
    red = mm.lacquer()
    stone = mm.paving("manor")
    tip_x, tip_y = 226, 98             # 檐角尖
    eave_x0 = 244                      # 檐口从这里起往右是平的
    eave_y = 130

    def eave(x):                       # 檐口下沿：右段平直，近檐角处起翘
        if x >= eave_x0:
            return eave_y
        t = (eave_x0 - x) / (eave_x0 - tip_x)
        return int(round(eave_y - (eave_y - tip_y) * t ** 2.2))

    def hip(x):                        # 戗脊：从檐角斜上到画面右上
        return int(round(tip_y - (x - tip_x) * 0.42))

    for x in range(tip_x, W):
        y0, y1 = max(0, hip(x)), eave(x)
        for y in range(y0, y1):
            # 瓦垄随坡斜：越近檐角越斜
            k = (x + (y1 - y) // 3) % 5
            near.px(x, y, rr, [1, 3, 4, 3, 2][k] - (1 if (y1 - y) % 6 == 0 and k in (2, 3) else 0))
        # 檐口：一排瓦当（亮点）+ 下沿一线暗
        near.px(x, y1 - 2, rr, 5 if x % 5 in (2, 3) else 2)
        near.px(x, y1 - 1, rr, 0)
        # 戗脊：三像素的脊，上沿亮
        for d, lvv in ((0, 5), (1, 4), (2, 3), (3, 1)):
            near.px(x, max(0, hip(x)) + d, rr, lvv)
    # 檐下：一道暗的椽子底，椽头一个个浅点
    for x in range(tip_x + 4, W):
        y = eave(x)
        near.rect(x, y, 1, 5, mm.darkwood(), 1)
        if x % 4 == 0:
            near.px(x, y + 1, mm.darkwood(), 4)
    # 翘角尖上一只小兽（剪影一粒）与风铃
    near.rect(tip_x - 2, tip_y - 3, 3, 3, rr, 4)
    near.rect(tip_x + 3, tip_y + 2, 1, 9, mm.metal(), 2)
    near.rect(tip_x + 1, tip_y + 11, 5, 5, mm.gold(), 3)
    near.px(tip_x + 3, tip_y + 16, mm.gold(), 2)
    # 朱柱与石栏
    near.rect(270, eave_y + 5, 7, H - eave_y - 5, red, 2)
    near.rect(270, eave_y + 5, 2, H - eave_y - 5, red, 3)
    near.rect(275, eave_y + 5, 2, H - eave_y - 5, red, 1)
    near.rect(214, 160, W - 214, 3, stone, 4)
    near.rect(214, 163, W - 214, 1, stone, 1)
    for x in range(217, W, 8):
        near.rect(x, 164, 3, H - 164, stone, 3)
        near.rect(x + 2, 164, 1, H - 164, stone, 1)


def pine_bough(near, seed: int) -> None:
    """左上斜进来的一枝松：枝干从画外伸进来，沿枝挂四五簇针叶，每簇左上亮、右下一线暗，
    簇边参差。（第一版是几片压扁的平涂椭圆，像浮着的绿盘子。）"""
    pine = mm.foliage("pine")
    bark = mm.bark()
    pts = [(k, 10 + int(k * 0.32) + int(np.sin(k / 9.0) * 2)) for k in range(76)]
    for k, (x, y) in enumerate(pts):
        near.rect(x, y, 1, 3 if k < 40 else 2, bark, 2 if k % 6 else 1)
        near.px(x, y, bark, 3)
    for i, (k, r) in enumerate(((6, 9.0), (24, 10.0), (44, 8.5), (62, 7.0), (75, 5.0))):
        cx, cy = pts[k]
        cy -= 3
        n = int(np.ceil(r)) + 2
        yy, xx = np.mgrid[-n:n + 1, -n:n + 1].astype(np.float32)
        wob = (pix.hash01((xx + cx).astype(np.int64) // 2, (yy + cy).astype(np.int64) // 2, seed + i) - 0.5) * 2.4
        d = np.sqrt((xx / 1.5) ** 2 + yy ** 2) * 1.5 + wob
        disc = (d <= r) & (yy <= r * 0.45)
        lv = np.clip(np.round(3.2 - (xx / 1.5 + yy) / r * 1.6), 1, 4)
        needle = pix.hash01((xx + cx).astype(np.int64), (yy + cy).astype(np.int64), seed + 7) < 0.22
        lv = np.where(needle & (lv >= 2) & (lv < 4), lv + 1, lv)
        lv = np.where((d > r - 1.2) & (xx + yy > 0), 0.0, lv)
        near.paint(cx - n, cy - n, disc, pine, lv.astype(np.float32))


def build(out_root: Path, fast: bool = False) -> list[Path]:
    return bk.save_layers(render(), out_root / "title", fast)
