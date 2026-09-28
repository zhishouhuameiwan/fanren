"""战斗背景：assets/art/battle/<backdrop>/{sky,far,mid,ground}.png，每层 320×180 带 alpha。

构图守三条（横版战斗：敌左我右，站位带在画面 55%–85% 高）：
  * 地面有透视：石板、地砖、木板、草纹近大远小，消失点在画面正中的地平线上；
  * 中景、远景给层次（远山、屋脊、树影），东西放在两侧，画面中央留给打斗；
  * 天空与远景偏灰、偏淡（空气透视），前景地面明度适中——人物站上去才压得住。

背景 id 与 docs/art-maps.md 的「战斗背景表」一致；time 变体（_dusk / _night / manor_court）
是 battles.json 为了剧情时辰指定的，同一套画法换天空与灯火。
"""

from __future__ import annotations

from pathlib import Path

import numpy as np

import bdkit as bk
import mapmat as mm
import pix
from palette import c, mix

W, H = bk.W, bk.H
HZ = bk.HORIZON


# ---------------------------------------------------------------------------
# 共用色阶
# ---------------------------------------------------------------------------
def far_ramp(kind: str) -> int:
    """远山：越远越灰越蓝（空气透视）。"""
    if kind == "dusk":
        cols = [mix("violet1", "ink2", 0.3), c("violet1"), mix("violet2", "red2", 0.3), mix("violet3", "red3", 0.4),
                mix("red4", "violet3", 0.5)]
    elif kind == "night":
        cols = [c("ink0"), c("ink1"), mix("ink2", "water0", 0.5), mix("ink3", "water1", 0.5), c("ink4")]
    elif kind == "green":
        cols = [mix("pine", "ink2", 0.5), mix("leaf1", "water1", 0.4), mix("leaf2", "water2", 0.45),
                mix("leaf3", "water3", 0.5), mix("leaf4", "water4", 0.55)]
    elif kind == "soul":
        cols = [c("violet0"), c("violet1"), mix("violet2", "jade1", 0.4), mix("jade2", "violet3", 0.4), c("jade3")]
    else:
        cols = [mix("ink3", "water1", 0.5), mix("ink4", "water2", 0.5), mix("ink5", "water3", 0.5),
                mix("ink6", "water4", 0.5), mix("ink7", "water5", 0.5)]
    return bk.ramp("far_" + kind, cols)


def cloud_ramp(kind: str) -> int:
    if kind == "dusk":
        cols = [c("violet2"), mix("red3", "violet3", 0.5), mix("red4", "gold3", 0.4), c("gold3"), c("gold4"),
                mix("gold4", "paper", 0.5)]
    elif kind == "night":
        cols = [c("ink1"), c("ink2"), c("ink3"), mix("ink4", "water2", 0.4), c("ink5"), c("ink6")]
    else:
        cols = [mix("ink5", "water3", 0.5), c("ink6"), c("ink7"), c("ink8"), mix("paper", "ink8", 0.5), c("paper")]
    return bk.ramp("cloud_" + kind, cols)


def wrap(fn, x: float, *a, **k) -> None:
    """中景的东西横跨左右边时两边各画一份——层是横向首尾相接的。fn(x, ...)。"""
    for dx in (-W, 0, W):
        fn(x + dx, *a, **k)


def wrapc(fn, cv: pix.Canvas, x: float, *a, **k) -> None:
    """同 wrap，fn(cv, x, ...) 的形式。"""
    for dx in (-W, 0, W):
        fn(cv, x + dx, *a, **k)


def standard_sky(time: str, seed: int) -> pix.Canvas:
    sky = bk.sky({"day": "day", "dusk": "dusk", "night": "night"}[time])
    if time == "night":
        bk.stars(sky, seed, 70, bk.ramp("stars", [c("ink5"), c("ink6"), c("ink7"), c("ink8"), c("paper"), c("paper")]))
        bk.moon(sky, 250, 30, 9)
        bk.clouds(sky, seed + 1, 30, 70, cloud_ramp("night"), 5, 4, 8, 3.0)
    elif time == "dusk":
        bk.sun_glow(sky, 160, 92, 90, 1.4)
        bk.clouds(sky, seed + 1, 20, 72, cloud_ramp("dusk"), 8, 4, 9, 3.5)
    else:
        bk.clouds(sky, seed + 1, 12, 64, cloud_ramp("day"), 9, 5, 11, 4.0)
    return sky


def far_mountains(time: str, seed: int, green: bool = True) -> pix.Canvas:
    far = bk.canvas()
    kind = time if time in ("dusk", "night") else ("green" if green else "day")
    back = far_ramp("day" if time == "day" else kind)
    bk.mountains(far, seed, back, 64, 20, 3.4, 3.0)
    bk.mountains(far, seed + 1, far_ramp(kind), 80, 12, 3.0, 2.0)
    bk.haze(far, 60, 100, 1.0 if time != "night" else 0.4)
    return far


# ---------------------------------------------------------------------------
# 中景小件
# ---------------------------------------------------------------------------
def cottage(cv: pix.Canvas, x: float, base: int, w: int, thatch: bool, lit: bool = False) -> None:
    x = int(x)
    roof = mm.roof("thatch" if thatch else "grey")
    wall = mm.earthwall() if thatch else mm.plaster()
    wd = mm.wood()
    hh = 12
    cv.rect(x, base - hh, w, hh, wall, 3)
    cv.rect(x, base - hh, 2, hh, wd, 2)
    cv.rect(x + w - 2, base - hh, 2, hh, wd, 1)
    cv.rect(x + w // 2 - 3, base - 9, 6, 9, wd, 1 if not lit else 2)
    win = mm.glow() if lit else mm.paper()
    for wx in (x + 4, x + w - 9):
        cv.rect(wx, base - 9, 5, 4, win, 2 if lit else 2)
    bk.chinese_roof(cv, x - 3, x + w + 3, base - hh - 12, base - hh, roof, 2 if thatch else 3, 2.5)
    cv.rect(x - 1, base, w + 2, 1, wall, 0)


def lantern_post(cv: pix.Canvas, x: float, base: int, lit: bool) -> None:
    x = int(x)
    wd = mm.wood()
    red = mm.lacquer() if not lit else mm.glow()
    cv.rect(x, base - 26, 2, 26, wd, 1)
    cv.rect(x - 4, base - 26, 10, 1, wd, 2)
    cv.rect(x - 4, base - 24, 4, 6, red, 3)
    cv.px(x - 3, base - 23, red, 4)


def banner(cv: pix.Canvas, x: float, base: int, cloth: str) -> None:
    x = int(x)
    wd = mm.wood()
    cl = mm.cloth(cloth)
    cv.rect(x, base - 40, 2, 40, wd, 2)
    cv.rect(x + 2, base - 38, 9, 18, cl, 3)
    cv.rect(x + 2, base - 38, 9, 1, cl, 4)
    cv.rect(x + 10, base - 38, 1, 18, cl, 2)
    for k in range(0, 9, 3):
        cv.px(x + 3 + k, base - 20, cl, 2)


def fence_run(cv: pix.Canvas, x0: int, x1: int, base: int) -> None:
    b = mm.bamboo()
    for x in range(x0, x1, 3):
        cv.rect(x, base - 10, 2, 10, b, 3)
        cv.px(x, base - 10, b, 4)
    cv.rect(x0, base - 7, x1 - x0, 1, mm.wood(), 2)
    cv.rect(x0, base - 3, x1 - x0, 1, mm.wood(), 2)


def rock_lump(cv: pix.Canvas, x: float, base: int, r: int, rid: int) -> None:
    yy, xx = np.mgrid[-r:r + 1, -r:r + 1].astype(np.float32)
    d = np.sqrt((xx / 1.3) ** 2 + yy * yy)
    m = (d <= r * 0.8) & (yy <= r * 0.5)
    lv = np.clip(np.round(3.2 - (xx + yy) / r * 1.3), 1, 5)
    lv = np.where(m & ~pix.erode(m, 1), 1.0, lv)
    cv.paint(int(x) - r, base - int(r * 1.3), m, rid, lv.astype(np.float32))


def hall_facade(cv: pix.Canvas, x0: int, x1: int, base: int, roof: str, post: int, wall_h: int = 14,
                lit: bool = False) -> None:
    """一座殿的正面（中景，横在后面）：柱、格窗、门，上压一座大屋顶。"""
    rr = mm.roof(roof)
    pl = mm.plaster()
    cv.rect(x0, base - wall_h, x1 - x0, wall_h, pl, 3)
    cv.rect(x0, base - wall_h, x1 - x0, 2, pl, 1)
    for x in range(x0, x1, 12):
        cv.rect(x, base - wall_h, 2, wall_h, post, 2)
    win = mm.glow() if lit else mm.paper()
    for x in range(x0 + 4, x1 - 6, 12):
        cv.rect(x, base - wall_h + 4, 6, 5, win, 2)
        cv.rect(x + 2, base - wall_h + 4, 1, 5, post, 1)
    bk.chinese_roof(cv, x0 - 5, x1 + 5, base - wall_h - 16, base - wall_h, rr, 4, 2.5)
    cv.rect(x0 - 2, base, x1 - x0 + 4, 2, mm.rock("cliff"), 3)


# ---------------------------------------------------------------------------
# 各主题
# ---------------------------------------------------------------------------
def village_road(seed: int, time: str = "day") -> dict:
    sky = standard_sky(time, seed)
    far = far_mountains(time, seed + 10)
    mid = bk.canvas()
    fol = mm.foliage("broad")
    blos = mm.blossom()
    wrapc(cottage, mid, 14, 104, 44, True)
    wrapc(cottage, mid, 262, 106, 40, True)
    fence_run(mid, 64, 112, 106)
    fence_run(mid, 214, 258, 106)
    for x, r, kind in ((70, 12, blos), (236, 13, fol), (300, 11, blos), (120, 9, fol)):
        wrap(lambda xx, rr=r, kk=kind, x0=x: bk.broad_tree(mid, int(xx), 104, rr, kk, seed + x0), x)
    ground = bk.canvas()
    grass = mm.grass("spring")
    bk.ground_noise(ground, grass, seed + 20, 2.6)
    u, w, m = bk.perspective_uv()
    road = m & (np.abs(u) < 26 + np.sin(w * 0.05) * 3)
    dirt = mm.dirt()
    ground.paint(0, 0, road, dirt, 3.0)
    edge = road & ~pix.erode(road, 1)
    ground.shade(0, 0, -1.0, edge)
    bk.tufts(ground, grass, seed + 21, n=140, x_avoid=(70, 250))
    return {"sky": sky, "far": far, "mid": mid, "ground": ground}


def mountain_forest(seed: int, time: str = "day") -> dict:
    sky = standard_sky(time, seed)
    far = bk.canvas()
    bk.mountains(far, seed + 1, far_ramp("day" if time == "day" else time), 58, 24, 3.4, 3.0)
    bk.mountains(far, seed + 2, far_ramp("green" if time == "day" else time), 76, 16, 3.0, 2.2)
    for x in range(0, W, 9):
        bk.pine_silhouette(far, x, 92 + (x * 7) % 5, 16 + (x * 13) % 8, far_ramp("green" if time == "day" else time),
                           1.0, seed)
    bk.haze(far, 50, 100, 1.2 if time != "night" else 0.5)
    mid = bk.canvas()
    pine = mm.foliage("pine")
    for x, h in ((18, 70), (44, 56), (272, 66), (300, 74), (118, 40), (206, 44)):
        wrap(lambda xx, hh=h: bk.pine_silhouette(mid, int(xx), 106, hh, pine, 2.0, seed), x)
    rock = mm.rock("cliff")
    wrapc(rock_lump, mid, 90, 108, 8, rock)
    wrapc(rock_lump, mid, 236, 106, 6, rock)
    ground = bk.canvas()
    grass = mm.grass("autumn" if time == "dusk" else "summer")
    bk.ground_noise(ground, grass, seed + 20, 2.4)
    u, w, m = bk.perspective_uv()
    path = m & (np.abs(u - np.sin(w * 0.08) * 6) < 20)
    ground.paint(0, 0, path, mm.dirt(), 2.6)
    bk.tufts(ground, grass, seed + 21, n=160, x_avoid=(60, 260))
    return {"sky": sky, "far": far, "mid": mid, "ground": ground}


def cliff_top(seed: int, time: str = "day") -> dict:
    sky = standard_sky(time, seed)
    far = bk.canvas()
    # 远峰只露尖，底下是云海
    bk.mountains(far, seed + 1, far_ramp("day" if time == "day" else time), 70, 30, 3.4, 2.8,
                 cells=((40, 0.7), (16, 0.3)))
    cl = cloud_ramp("dusk" if time == "dusk" else "day")
    far.paint(0, 82, np.ones((20, W), bool), cl, 2.0)
    bk.clouds(far, seed + 3, 78, 96, cl, 16, 6, 12, 3.5)
    mid = bk.canvas()
    pine = mm.foliage("pine")
    rock = mm.rock("cliff")
    for x, r in ((20, 16), (58, 10), (262, 12), (300, 18)):
        wrapc(rock_lump, mid, x, 110, r, rock)
    wrap(lambda xx: bk.pine_silhouette(mid, int(xx), 100, 46, pine, 2.0, seed), 30)
    wrap(lambda xx: bk.pine_silhouette(mid, int(xx), 104, 38, pine, 2.0, seed + 1), 292)
    ground = bk.canvas()
    bk.ground_tiles(ground, rock, 30, 26, seed + 20, 3.0, bond=False, mortar=1.0)
    grass = mm.grass("autumn" if time == "dusk" else "spring")
    bk.tufts(ground, grass, seed + 21, n=70, x_avoid=(60, 260))
    return {"sky": sky, "far": far, "mid": mid, "ground": ground}


def sect_courtyard(seed: int, time: str = "day") -> dict:
    sky = standard_sky(time, seed)
    far = far_mountains(time, seed + 10)
    mid = bk.canvas()
    hall_facade(mid, 96, 224, 100, "jade", mm.lacquer(), 16, time == "night")
    for x in (40, 280):
        wrap(lambda xx: stone_lantern_bd(mid, int(xx), 108), x)
    pine = mm.foliage("pine")
    wrap(lambda xx: bk.pine_silhouette(mid, int(xx), 104, 52, pine, 2.0, seed), 14)
    wrap(lambda xx: bk.pine_silhouette(mid, int(xx), 106, 58, pine, 2.0, seed + 2), 306)
    ground = bk.canvas()
    bk.ground_tiles(ground, mm.paving("sect"), 24, 20, seed + 20, 3.0)
    return {"sky": sky, "far": far, "mid": mid, "ground": ground}


def stone_lantern_bd(cv: pix.Canvas, x: int, base: int) -> None:
    rid = mm.paving("sect")
    cv.rect(x - 5, base - 4, 10, 4, rid, 3)
    cv.rect(x - 2, base - 14, 4, 10, rid, 3)
    cv.rect(x - 4, base - 20, 8, 6, rid, 3)
    cv.rect(x - 2, base - 18, 4, 3, mm.glow(), 2)
    cv.rect(x - 6, base - 23, 12, 3, rid, 4)
    cv.rect(x - 1, base - 26, 2, 3, rid, 4)


def drill_ground(seed: int, time: str = "day") -> dict:
    sky = standard_sky(time, seed)
    far = far_mountains(time, seed + 10)
    mid = bk.canvas()
    wd = mm.wood()
    # 校场边的木栅栏、兵器架、旗
    for x in range(0, W, 4):
        mid.rect(x, 90, 2, 16, wd, 2 if x % 8 else 3)
    mid.rect(0, 94, W, 1, wd, 1)
    mid.rect(0, 101, W, 1, wd, 1)
    for x in (30, 290):
        wrap(lambda xx: weapon_rack_bd(mid, int(xx), 110), x)
    wrapc(banner, mid, 62, 106, "red")
    wrapc(banner, mid, 250, 106, "indigo")
    if time == "night":
        for x in (46, 272):
            wrap(lambda xx: brazier_bd(mid, int(xx), 108), x)
    ground = bk.canvas()
    bk.ground_noise(ground, mm.sand(), seed + 20, 3.0, speck=0.25)
    return {"sky": sky, "far": far, "mid": mid, "ground": ground}


def weapon_rack_bd(cv: pix.Canvas, x: int, base: int) -> None:
    wd = mm.wood()
    mt = mm.metal()
    cv.rect(x - 10, base - 20, 2, 20, wd, 2)
    cv.rect(x + 8, base - 20, 2, 20, wd, 1)
    cv.rect(x - 10, base - 18, 20, 2, wd, 3)
    cv.rect(x - 10, base - 8, 20, 2, wd, 2)
    for k in range(-7, 8, 4):
        cv.rect(x + k, base - 28, 1, 28, wd, 3)
        cv.rect(x + k, base - 31, 1, 3, mt, 4)


def brazier_bd(cv: pix.Canvas, x: int, base: int) -> None:
    mt = mm.metal()
    gl = mm.glow()
    cv.rect(x - 1, base - 12, 2, 12, mt, 1)
    cv.rect(x - 5, base - 16, 10, 4, mt, 2)
    for k in range(-4, 5, 2):
        cv.rect(x + k, base - 21 + abs(k) // 2, 2, 5 - abs(k) // 2, gl, 3 + (k == 0))


def herb_valley(seed: int, time: str = "day") -> dict:
    sky = standard_sky(time, seed)
    far = bk.canvas()
    bk.mountains(far, seed + 1, far_ramp("green"), 52, 28, 3.2, 2.6)
    bk.mountains(far, seed + 2, far_ramp("green"), 74, 12, 3.4, 3.0)
    bk.haze(far, 50, 100, 1.0)
    mid = bk.canvas()
    wrapc(cottage, mid, 250, 104, 46, False)
    wrap(lambda xx: drying_rack_bd(mid, int(xx), 108), 40)
    wrap(lambda xx: drying_rack_bd(mid, int(xx), 108), 72)
    fol = mm.foliage("willow")
    wrap(lambda xx: bk.broad_tree(mid, int(xx), 104, 14, fol, seed + 3), 14)
    ground = bk.canvas()
    grass = mm.grass("spring")
    bk.ground_noise(ground, grass, seed + 20, 2.6)
    u, w, m = bk.perspective_uv()
    rows = m & (np.abs(u) > 34) & ((np.floor(w / 5) % 2) == 0)
    ground.paint(0, 0, rows, mm.dirt(), 2.0)
    herb = mm.herb()
    sprout = rows & (pix.hash01((u * 2).astype(np.int64), (w * 2).astype(np.int64), seed) < 0.18)
    ground.paint(0, 0, sprout, herb, 4.0)
    bk.tufts(ground, grass, seed + 21, n=60, x_avoid=(60, 260))
    return {"sky": sky, "far": far, "mid": mid, "ground": ground}


def drying_rack_bd(cv: pix.Canvas, x: int, base: int) -> None:
    wd = mm.wood()
    b = mm.bamboo()
    herb = mm.herb()
    cv.rect(x - 9, base - 18, 2, 18, wd, 2)
    cv.rect(x + 7, base - 18, 2, 18, wd, 1)
    for y in (base - 16, base - 9):
        cv.rect(x - 11, y, 22, 3, b, 3)
        for k in range(-9, 9, 3):
            cv.px(x + k, y, herb, 3)


def indoor_back(time: str, rid_wall: int, seed: int, beams: bool = True) -> pix.Canvas:
    """室内的「天空层」：后墙整面（上暗下亮一点），梁、柱的剪影。"""
    sky = bk.canvas(opaque=True)
    ys = np.arange(H, dtype=np.float32)[:, None]
    lv = 1.0 + np.clip(ys / 100.0, 0, 1) * 1.8
    sky.paint(0, 0, np.ones((H, W), bool), rid_wall, np.broadcast_to(lv, (H, W)).astype(np.float32))
    if beams:
        dw = mm.darkwood()
        sky.rect(0, 10, W, 5, dw, 2)
        sky.rect(0, 10, W, 1, dw, 3)
        for x in range(0, W, 64):
            sky.rect(x + 20, 0, 6, H, dw, 1)
            sky.rect(x + 20, 0, 1, H, dw, 2)
    return sky


def secret_room(seed: int, time: str = "indoor") -> dict:
    rock = mm.rock("cliff")
    sky = indoor_back(time, rock, seed, beams=False)
    # 后墙：条石砌的
    for y in range(20, HZ, 9):
        off = 0 if (y // 9) % 2 else 11
        sky.rect(0, y, W, 1, rock, 0)
        for x in range(-off, W, 22):
            sky.rect(x, y, 1, 9, rock, 0)
    far = bk.canvas()
    dw = mm.darkwood()
    # 药柜与书架（贴后墙，两侧）
    for x0 in (6, 244):
        far.rect(x0, 30, 70, 64, dw, 2)
        for yy in range(34, 92, 8):
            far.rect(x0 + 2, yy, 66, 1, dw, 0)
            for xx in range(x0 + 4, x0 + 66, 5):
                far.rect(xx, yy + 2, 3, 5, mm.paper() if (xx + yy) % 3 else mm.cloth("red"), 2)
    mid = bk.canvas()
    for x in (104, 216):
        mid.rect(x, 0, 10, 108, rock, 3)
        mid.rect(x, 0, 2, 108, rock, 4)
        mid.rect(x + 8, 0, 2, 108, rock, 1)
    for x in (60, 262):
        candle_bd(mid, x, 104)
    ground = bk.canvas()
    bk.ground_tiles(ground, mm.paving("sect"), 20, 18, seed + 20, 2.6)
    return {"sky": sky, "far": far, "mid": mid, "ground": ground}


def candle_bd(cv: pix.Canvas, x: int, base: int) -> None:
    wd = mm.darkwood()
    cv.rect(x - 1, base - 18, 2, 18, wd, 2)
    cv.rect(x - 4, base - 18, 8, 2, wd, 3)
    cv.rect(x - 1, base - 23, 2, 5, mm.paper(), 4)
    cv.px(x, base - 25, mm.glow(), 4)
    cv.px(x, base - 26, mm.glow(), 5)


def cave_tunnel(seed: int, time: str = "indoor") -> dict:
    rock = mm.rock("cave")
    sky = bk.canvas(opaque=True)
    sky.paint(0, 0, np.ones((H, W), bool), rock, 1.0)
    n = pix.fbm(H, W, seed, ((30, 0.6), (12, 0.4)), wrap_x=True)
    sky.shade(0, 0, (np.round(n * 3) - 1.0).astype(np.float32), np.ones((H, W), bool))
    far = bk.canvas()
    # 洞壁：左右两道岩壁往中间收，中间深处更暗（隧道）
    xs = np.arange(W)[None, :]
    ys = np.arange(H)[:, None]
    wall_l = xs < 60 + (pix.value_noise(H, 1, 12.0, seed + 1)[:, :1] * 30 - ys * 0.2)
    wall_r = xs > 260 - (pix.value_noise(H, 1, 12.0, seed + 2)[:, :1] * 30 - ys * 0.2)
    wm = (wall_l | wall_r) & (ys < HZ + 10)
    lv = 2.0 + np.round(pix.fbm(H, W, seed + 3, ((14, 0.6), (6, 0.4))) * 2) - 1
    far.paint(0, 0, wm, rock, lv.astype(np.float32))
    mid = bk.canvas()
    g = np.random.default_rng(seed + 4)
    for _ in range(14):
        x = int(g.integers(0, W))
        ln = int(g.integers(8, 26))
        for k in range(ln):
            wdt = max(1, (ln - k) // 5)
            mid.rect(x - wdt // 2, k, wdt + 1, 1, rock, 3 if k < ln // 2 else 2)
    for x, r in ((30, 12), (290, 14), (90, 6)):
        wrapc(rock_lump, mid, x, 112, r, rock)
    for x in (50, 272):
        torch_bd(mid, x, 70)
    ground = bk.canvas()
    bk.ground_noise(ground, rock, seed + 20, 2.4, speck=0.25)
    u, w, m = bk.perspective_uv()
    # 水洼只在两侧、站位带以外：落在人脚下的一大片蓝读起来像地上的窟窿
    ys = np.arange(H)[:, None]
    puddle = m & (pix.value_noise(H, W, 18.0, seed + 5) > 0.76) & (np.abs(u) > 60) & ((ys < bk.STAND_Y0 + 8) | (ys > bk.STAND_Y1))
    ground.paint(0, 0, puddle, mm.water(), 1.0)
    return {"sky": sky, "far": far, "mid": mid, "ground": ground}


def torch_bd(cv: pix.Canvas, x: int, y: int) -> None:
    wd = mm.wood()
    gl = mm.glow()
    cv.rect(x - 1, y, 2, 12, wd, 2)
    for k in range(-2, 3):
        cv.rect(x + k, y - 6 + abs(k), 1, 6 - abs(k), gl, 3 + (k == 0))
    cv.px(x, y - 8, gl, 5)


def soulsea(seed: int, time: str = "soul") -> dict:
    """识海：夺舍的元神之战——没有地，没有天，只有一片星云般翻涌的精神空间。
    中间一片镜面似的「地」，上面浮着碎光；远处是缓缓旋开的青紫星云。"""
    sky = bk.sky("soul", 0.2, 3.2, 150)
    stars_rid = bk.ramp("soul_star", [c("violet2"), c("violet3"), c("jade3"), c("jade4"), c("paper"), c("paper")])
    bk.stars(sky, seed, 150, stars_rid, 110)
    neb = bk.ramp("nebula", [c("violet0"), c("violet1"), c("violet2"), mix("violet3", "jade2", 0.4), c("jade3"),
                             c("jade4")])
    n = pix.fbm(H, W, seed + 1, ((70, 0.5), (30, 0.3), (12, 0.2)), wrap_x=True)
    swirl = pix.fbm(H, W, seed + 2, ((40, 0.6), (16, 0.4)), wrap_x=True)
    m = (n > 0.5) & (swirl > 0.42)
    lv = np.clip(np.round((n - 0.5) * 12 + (swirl - 0.4) * 4), 0, 5).astype(np.float32)
    sky.paint(0, 0, m, neb, lv)
    far = bk.canvas()
    # 远处几座倒悬的浮岛（记忆的碎块）
    isl = bk.ramp("isle", [c("ink1"), c("violet0"), c("violet1"), c("violet2"), c("violet3")])
    g = np.random.default_rng(seed + 3)
    for _ in range(6):
        cx = int(g.integers(0, W))
        cy = int(g.integers(30, 80))
        r = int(g.integers(5, 12))
        for dx in (-W, 0, W):
            for k in range(r * 2):
                wdt = int(r * 2 * (1 - k / (r * 2)) ** 0.7)
                far.rect(cx + dx - wdt // 2, cy + k, wdt, 1, isl, 3 if k < 2 else 2)
            far.rect(cx + dx - r, cy - 1, r * 2, 1, isl, 4)
    mid = bk.canvas()
    motes = bk.ramp("mote", [c("jade2"), c("jade3"), c("jade4"), c("gold3"), c("gold4"), c("paper")])
    for _ in range(90):
        x, y = int(g.integers(0, W)), int(g.integers(20, HZ + 20))
        mid.px(x, y, motes, float(g.integers(2, 6)))
        if g.random() < 0.3:
            mid.px(x, y - 1, motes, 2.0)
    ground = bk.canvas()
    # 镜面：深色、横向的涟漪亮线，越近越宽
    u, w, mm_ = bk.perspective_uv()
    base = bk.ramp("mirror", [c("ink0"), c("violet0"), c("violet1"), mix("violet2", "jade1", 0.5), c("jade2"),
                              c("jade3")])
    ys = np.arange(H, dtype=np.float32)[:, None]
    lv = np.broadcast_to(1.0 + (ys - HZ) / 84.0, (H, W)).astype(np.float32).copy()
    ripple = (np.floor(w / 3.0) % 5 == 0) & (pix.hash01((u / 6).astype(np.int64), (w / 3).astype(np.int64), seed) < 0.5)
    lv = np.where(ripple, lv + 2.0, lv)
    ground.paint(0, 0, mm_, base, lv)
    return {"sky": sky, "far": far, "mid": mid, "ground": ground}


def town_street(seed: int, time: str = "day") -> dict:
    sky = standard_sky(time, seed)
    far = bk.canvas()
    rr = mm.roof("grey")
    # 远处层层叠叠的屋脊
    for k, (base, h) in enumerate(((70, 8), (78, 9), (86, 10))):
        x = -int(seed % 17)
        while x < W + 40:
            w_ = 26 + (x * 7 + k * 13) % 20
            bk.chinese_roof(far, x, x + w_, base - h, base, rr, 2, 1.5 + k * 0.5)
            far.rect(x + 2, base + 1, w_ - 4, 10, mm.plaster(), 1 + k)
            x += w_ + 6
    bk.haze(far, 60, 100, 0.8 if time == "day" else 0.4)
    mid = bk.canvas()
    lit = time != "day"
    wrap(lambda xx: shopfront(mid, int(xx), 106, 58, "indigo", lit), 0)
    wrap(lambda xx: shopfront(mid, int(xx), 106, 58, "ochre", lit), 262)
    for x in (72, 248):
        wrapc(lantern_post, mid, x, 108, lit)
    ground = bk.canvas()
    bk.ground_tiles(ground, mm.paving("town"), 26, 18, seed + 20, 3.0)
    return {"sky": sky, "far": far, "mid": mid, "ground": ground}


def shopfront(cv: pix.Canvas, x: int, base: int, w: int, cloth: str, lit: bool) -> None:
    wd = mm.wood()
    rr = mm.roof("grey")
    cv.rect(x, base - 22, w, 22, wd, 2)
    cv.rect(x + 3, base - 18, w - 6, 12, mm.void() if not lit else mm.glow(), 1)
    cv.rect(x + 3, base - 7, w - 6, 3, wd, 3)
    for xx in range(x + 5, x + w - 5, 6):
        cv.rect(xx, base - 9, 3, 2, mm.herb() if xx % 12 else mm.flower("red"), 3)
    bk.chinese_roof(cv, x - 4, x + w + 4, base - 38, base - 22, rr, 3, 2.5)
    aw = mm.cloth(cloth)
    for xx in range(x, x + w):
        cv.rect(xx, base - 22, 1, 4, aw, 3 if (xx // 3) % 2 else 2)
    banner(cv, x + w - 8, base - 4, cloth)


def manor(seed: int, time: str = "night") -> dict:
    sky = standard_sky(time, seed)
    far = bk.canvas()
    rr = mm.roof("black")
    for k, (x0, x1, base) in enumerate(((10, 110, 78), (130, 250, 74), (230, 330, 80))):
        bk.chinese_roof(far, x0, x1, base - 14, base, rr, 4, 2.0)
        far.rect(x0 + 4, base + 1, x1 - x0 - 8, 16, mm.plaster(), 2)
    fol = mm.foliage("dark" if time == "night" else "broad")
    for x in (40, 180, 290):
        wrap(lambda xx, x0=x: bk.broad_tree(far, int(xx), 96, 12, fol, seed + x0), x)
    mid = bk.canvas()
    # 白墙黛瓦的院墙，一道月洞门在侧边
    pl = mm.plaster()
    mid.rect(0, 86, W, 20, pl, 3)
    mid.rect(0, 86, W, 2, rr, 2)
    mid.rect(0, 84, W, 2, rr, 4)
    yy, xx = np.mgrid[0:20, 0:20].astype(np.float32)
    gate = ((xx - 9.5) ** 2 + (yy - 10) ** 2) <= 81
    mid.erase(254, 86, gate)
    ring = gate & ~pix.erode(gate, 1)
    mid.paint(254, 86, ring, mm.rock("valley"), 3.0)
    for x in (80, 232):
        wrapc(lantern_post, mid, x, 110, time == "night")
    wrap(lambda xx: bamboo_bd(mid, int(xx), 108), 20)
    wrap(lambda xx: bamboo_bd(mid, int(xx), 108), 300)
    ground = bk.canvas()
    bk.ground_tiles(ground, mm.paving("manor"), 22, 20, seed + 20, 3.0)
    return {"sky": sky, "far": far, "mid": mid, "ground": ground}


def bamboo_bd(cv: pix.Canvas, x: int, base: int) -> None:
    b = mm.bamboo()
    leaf = mm.foliage("bamboo")
    g = np.random.default_rng(x % W)
    for k in range(5):
        xx = x + (k - 2) * 3
        top = base - 44 - int(g.integers(0, 10))
        cv.rect(xx, top, 2, base - top, b, 3)
        for yy in range(top, base, 7):
            cv.rect(xx, yy, 2, 1, b, 1)
        for j in range(4):
            ly = top + j * 6 + int(g.integers(0, 4))
            for q in range(5):
                cv.px(xx + q * (1 if j % 2 else -1), ly + q // 2, leaf, 3 if q else 4)


def dock_river(seed: int, time: str = "dusk") -> dict:
    sky = standard_sky(time, seed)
    far = bk.canvas()
    bk.mountains(far, seed + 1, far_ramp(time if time != "day" else "day"), 72, 12, 3.2, 2.6)
    water = mm.water()
    far.paint(0, 84, np.ones((14, W), bool), water, 3.0)
    for y in range(86, 98, 3):
        for x in range((y * 7) % 11, W, 11):
            far.rect(x, y, 4, 1, water, 4)
    mid = bk.canvas()
    wd = mm.wood()
    # 泊着的船、栈桥的桩、芦苇、一角仓房
    for x in (40, 214):
        boat_bd(mid, x, 100)
    for x in range(120, 200, 10):
        mid.rect(x, 96, 2, 10, wd, 2)
    reed = mm.grass("autumn")
    g = np.random.default_rng(seed + 3)
    for _ in range(40):
        x = int(g.integers(0, W))
        if 90 < x < 230:
            continue
        hgt = int(g.integers(8, 18))
        for k in range(hgt):
            mid.px(x, 108 - k, reed, 4 if k > hgt - 3 else 2)
    wrapc(cottage, mid, 270, 104, 44, False, time != "day")
    ground = bk.canvas()
    bk.ground_tiles(ground, mm.floorwood(), 60, 7, seed + 20, 3.0, bond=False, mortar=2.0)
    return {"sky": sky, "far": far, "mid": mid, "ground": ground}


def boat_bd(cv: pix.Canvas, x: int, base: int) -> None:
    wd = mm.wood()
    rr = mm.roof("black")
    for k in range(50):
        f = abs(k - 25) / 25
        hh = int(6 * (1 - f ** 2)) + 1
        cv.rect(x + k, base - hh, 1, hh, wd, 2)
        cv.px(x + k, base - hh, wd, 4)
    for k in range(18):
        cv.rect(x + 16 + k, base - 14, 1, 9, rr, 2 + (k % 3 == 0))
        cv.px(x + 16 + k, base - 14, rr, 4)


def wild_manor(seed: int, time: str = "night") -> dict:
    sky = standard_sky(time, seed)
    far = bk.canvas()
    fr = far_ramp(time)
    bk.mountains(far, seed + 1, fr, 70, 16, 3.0, 2.0)
    for x in range(0, W, 8):
        bk.pine_silhouette(far, x, 94, 18 + (x * 11) % 10, fr, 1.0, seed)
    mid = bk.canvas()
    rock = mm.rock("cliff")
    # 庄墙：夯土墙 + 瓦顶，墙上火把；墙后露出屋顶
    mid.rect(0, 80, W, 24, rock, 3)
    for y in range(84, 104, 5):
        mid.rect(0, y, W, 1, rock, 2)
    rr = mm.roof("grey")
    mid.rect(0, 78, W, 2, rr, 4)
    mid.rect(0, 80, W, 1, rr, 1)
    bk.chinese_roof(mid, 110, 210, 54, 76, rr, 4, 2.0)
    for x in (60, 170, 270):
        torch_bd(mid, x, 84)
    fol = mm.foliage("dark" if time == "night" else "autumn")
    for x in (16, 300):
        wrap(lambda xx, x0=x: bk.broad_tree(mid, int(xx), 108, 16, fol, seed + x0), x)
    ground = bk.canvas()
    grass = mm.grass("autumn")
    bk.ground_noise(ground, grass, seed + 20, 2.4)
    bk.tufts(ground, grass, seed + 21, n=120, x_avoid=(60, 260))
    return {"sky": sky, "far": far, "mid": mid, "ground": ground}


def inn_hall(seed: int, time: str = "indoor") -> dict:
    wall = mm.plaster()
    sky = indoor_back(time, wall, seed)
    dw = mm.darkwood()
    # 后墙：格子窗、酒坛架、挂灯
    for x in (40, 200):
        sky.rect(x, 30, 80, 40, dw, 2)
        sky.rect(x + 3, 33, 74, 34, mm.paper(), 2)
        for xx in range(x + 3, x + 77, 6):
            sky.rect(xx, 33, 1, 34, dw, 2)
        for yy in range(33, 67, 6):
            sky.rect(x + 3, yy, 74, 1, dw, 2)
    far = bk.canvas()
    for x0 in (4, 270):
        far.rect(x0, 60, 46, 36, dw, 2)
        for yy in (64, 78):
            far.rect(x0 + 2, yy + 11, 42, 2, dw, 3)
            for xx in range(x0 + 4, x0 + 42, 9):
                jar = mm.earthwall()
                far.rect(xx, yy, 7, 11, jar, 3)
                far.rect(xx + 2, yy - 2, 3, 2, jar, 2)
                far.rect(xx + 1, yy + 3, 5, 3, mm.cloth("red"), 3)
    mid = bk.canvas()
    for x in (100, 214):
        mid.rect(x, 0, 8, 110, dw, 3)
        mid.rect(x, 0, 2, 110, dw, 4)
        lantern_post(mid, x + 20, 60, True)
    for x in (30, 290):
        table_bd(mid, x, 112)
    ground = bk.canvas()
    bk.ground_tiles(ground, mm.floorwood(), 70, 6, seed + 20, 3.0, bond=True, mortar=2.0)
    return {"sky": sky, "far": far, "mid": mid, "ground": ground}


def table_bd(cv: pix.Canvas, x: int, base: int) -> None:
    dw = mm.darkwood()
    cv.rect(x - 16, base - 14, 32, 4, dw, 4)
    cv.rect(x - 16, base - 10, 32, 2, dw, 2)
    cv.rect(x - 14, base - 10, 2, 10, dw, 2)
    cv.rect(x + 12, base - 10, 2, 10, dw, 1)
    cv.rect(x - 4, base - 18, 5, 4, mm.earthwall(), 3)


# ---------------------------------------------------------------------------
BACKDROPS = {
    "village_road": (village_road, "day"),
    "mountain_forest": (mountain_forest, "day"),
    "mountain_forest_dusk": (mountain_forest, "dusk"),
    "cliff_top": (cliff_top, "day"),
    "cliff_top_dusk": (cliff_top, "dusk"),
    "sect_courtyard": (sect_courtyard, "day"),
    "drill_ground": (drill_ground, "day"),
    "drill_ground_night": (drill_ground, "night"),
    "herb_valley": (herb_valley, "day"),
    "secret_room": (secret_room, "indoor"),
    "cave_tunnel": (cave_tunnel, "indoor"),
    "soulsea": (soulsea, "soul"),
    "town_street": (town_street, "day"),
    "town_street_dusk": (town_street, "dusk"),
    "manor_night": (manor, "night"),
    "manor_court": (manor, "day"),
    "dock_river": (dock_river, "dusk"),
    "wild_manor": (wild_manor, "night"),
    "inn_hall": (inn_hall, "indoor"),
}


def render(name: str) -> dict:
    fn, time = BACKDROPS[name]
    return fn(pix.seed_of("backdrop", name) % 100000, time)


def build(out_root: Path, only: str | None = None, fast: bool = False) -> list[Path]:
    out = []
    for name in BACKDROPS:
        if only and name != only:
            continue
        layers = render(name)
        out += bk.save_layers(layers, out_root / "battle" / name, fast)
    return out
