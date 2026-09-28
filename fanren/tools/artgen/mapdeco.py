"""叠加层装饰：gid 6 花草、gid 7 碎石裂纹。只画在这两种格上。

守一条：装饰不能改变「看上去能不能走」。所以
  * 一律贴地、矮小（≤7px 高），不画成一丛灌木或一块大石头；
  * 画在可走格上（生成器只往可走格上撒叠加层，这里再按碰撞层确认一遍）；
  * 满地都是的（广场一整片碎石、药圃一整片花草）按「地面质感」处理：稀疏、低对比，
    不一格一簇地排队，免得一片空地看上去像铺满了杂物。
"""

from __future__ import annotations

import numpy as np

import mapmat as mm
import pix
from mapinfer import S

T = 16
FLOWERS = ("white", "yellow", "pink", "violet", "red")


def flower_cluster(cv: pix.Canvas, px: int, py: int, seed: int, season: str) -> None:
    """一小丛花：3–5 朵一像素的花点，底下一两片叶子。"""
    g = np.random.default_rng(seed)
    kind = FLOWERS[int(g.integers(0, len(FLOWERS)))]
    if season == "autumn" and kind in ("pink", "red"):
        kind = "yellow"
    fr = mm.flower(kind)
    leaf = mm.grass(season)
    for _ in range(int(g.integers(3, 6))):
        x = px + int(g.integers(0, 6))
        y = py + int(g.integers(0, 5))
        cv.px(x, y + 1, leaf, 4)
        cv.px(x - 1, y + 2, leaf, 3)
        cv.px(x, y, fr, 3 if g.random() < 0.6 else 2)
        cv.px(x, y + 2, leaf, 1)


def grass_clump(cv: pix.Canvas, px: int, py: int, seed: int, season: str) -> None:
    """一簇高一点的草：五六片叶子从同一处发出来，叶尖最亮，根部一点影。"""
    g = np.random.default_rng(seed)
    leaf = mm.grass(season)
    for i, dx in enumerate((-2, -1, 0, 1, 2)):
        hgt = int(g.integers(3, 6)) - abs(dx) // 2
        for j in range(hgt):
            xx = px + dx + (dx // 2 if j > 1 else 0)
            cv.px(xx, py - j, leaf, 5 if j == hgt - 1 else (4 if j > 0 else 3))
    cv.rect(px - 2, py + 1, 5, 1, leaf, 0)


def pebbles(cv: pix.Canvas, px: int, py: int, seed: int, rid: int, n: int = 3) -> None:
    g = np.random.default_rng(seed)
    for _ in range(n):
        x = px + int(g.integers(0, 9))
        y = py + int(g.integers(0, 8))
        big = g.random() < 0.4
        cv.px(x, y, rid, 4)
        cv.px(x + 1, y, rid, 3)
        cv.px(x, y + 1, rid, 2)
        cv.px(x + 1, y + 1, rid, 1)
        if big:
            cv.px(x + 2, y, rid, 2)
            cv.px(x + 2, y + 1, rid, 1)
            cv.px(x - 1, y, rid, 4)
        cv.px(x + 1, y + 2, rid, 0)


def crack(cv: pix.Canvas, px: int, py: int, seed: int) -> None:
    """石面上一道裂纹：几段折线，暗一档。"""
    g = np.random.default_rng(seed)
    x, y = px + int(g.integers(2, 12)), py + int(g.integers(2, 10))
    for _ in range(int(g.integers(4, 8))):
        cv.shade(x, y, -1.0, np.ones((1, 1), bool))
        x += int(g.integers(-1, 2))
        y += int(g.integers(0, 2))


def sprouts(cv: pix.Canvas, px: int, py: int, seed: int) -> None:
    """药苗：垄顶上每 3px 一株两叶的小苗。"""
    herb = mm.herb()
    g = np.random.default_rng(seed)
    off = int(g.integers(0, 3))
    for yy in range(py, py + T):
        if yy % 5 != 0:
            continue
        for xx in range(px + off, px + T, 3):
            if g.random() < 0.2:
                continue
            cv.px(xx, yy - 1, herb, 4)
            cv.px(xx - 1, yy, herb, 3)
            cv.px(xx + 1, yy, herb, 3)
            cv.px(xx, yy, herb, 2)


def vegetables(cv: pix.Canvas, px: int, py: int, seed: int) -> None:
    """菜畦：垄上一颗颗圆菜。"""
    herb = mm.herb()
    g = np.random.default_rng(seed)
    for yy in range(py, py + T):
        if yy % 5 != 0:
            continue
        for xx in range(px + int(g.integers(0, 3)), px + T - 2, 5):
            if g.random() < 0.25:
                continue
            cv.rect(xx, yy - 2, 3, 2, herb, 3)
            cv.px(xx, yy - 2, herb, 4)
            cv.px(xx + 1, yy - 3, herb, 4)
            cv.rect(xx, yy, 3, 1, herb, 1)


def paint_overlay(ctx, ps: np.ndarray) -> None:
    sc = ctx.sc
    cv = ctx.below
    md = sc.md
    mid = md.map_id
    season = sc.season
    stone = mm.rock("cave" if sc.th["kind"] == "cave" else "cliff")
    indoor = sc.th["kind"] in ("indoor", "cave")
    for y in range(sc.H):
        for x in range(sc.W):
            if not sc.walk[y, x]:
                continue
            o = md.overlay[y, x]
            surf = sc.surface[y, x]
            px, py = x * T, y * T
            s = pix.seed_of(mid, "deco", x, y)
            h = pix.hash01(x, y, pix.salt_of(mid, "deco"))
            if surf == S["tilled"]:
                if o == 6:
                    sprouts(cv, px, py, s)
                elif sc.theme == "village":
                    vegetables(cv, px, py, s)
                continue
            if o == 6:
                if surf in (S["grass"], S["earth"], S["dirt_road"], S["sand"], S["courtyard"]):
                    if sc.dense6[y, x]:
                        # 满院子的草：一格一簇太挤，隔格来、换着花样
                        if h < 0.45:
                            grass_clump(cv, px + 4 + int(h * 16) % 8, py + 10, s, season)
                        elif h < 0.6:
                            flower_cluster(cv, px + 4, py + 5, s, season)
                    elif h < 0.55:
                        flower_cluster(cv, px + 3 + int(h * 20) % 5, py + 4 + int(h * 40) % 5, s, season)
                    else:
                        grass_clump(cv, px + 5 + int(h * 30) % 6, py + 9 + int(h * 50) % 4, s, season)
                elif indoor and surf in (S["stone_floor"], S["tile_floor"], S["flagstone"], S["cave_floor"]):
                    # 石地缝里的一点苔
                    if h < 0.5:
                        moss = mm.grass("summer")
                        cv.px(px + 5 + int(h * 12), py + 7, moss, 2)
                        cv.px(px + 6 + int(h * 12), py + 7, moss, 1)
                elif surf in (S["stone_road"], S["flagstone"], S["steps"], S["stone_floor"]):
                    if h < 0.6:
                        moss = mm.grass(season)
                        cv.px(px + 3 + int(h * 16), py + 8, moss, 3)
                        cv.px(px + 4 + int(h * 16), py + 8, moss, 2)
                        cv.px(px + 3 + int(h * 16), py + 9, moss, 1)
            elif o == 7:
                if sc.dense7[y, x]:
                    if h < 0.35:
                        pebbles(cv, px + 2, py + 3, s, stone, 1)
                    elif h < 0.5 and surf in (S["stone_floor"], S["tile_floor"], S["flagstone"], S["stone_road"]):
                        crack(cv, px, py, s)
                elif surf in (S["stone_road"], S["flagstone"], S["stone_floor"], S["tile_floor"], S["steps"],
                              S["wood_floor"], S["plank"]):
                    crack(cv, px, py, s)
                    if h < 0.4:
                        pebbles(cv, px + 3, py + 4, s, stone, 1)
                else:
                    pebbles(cv, px + 2 + int(h * 10) % 4, py + 2 + int(h * 30) % 5, s, stone,
                            2 + int(h * 3))


def paint_room_interiors(ctx) -> None:
    """hall() 掏出来的屋内（剖开画、看得见地面）：地面整体暗一档——头顶有屋顶；
    宗门与府邸的堂屋从门口铺一条红毡到后墙前。
    （上一版整体暗半级、贴墙再暗半级：半级是抖动出来的，满屋地面一片棋盘格。）"""
    from mapinfer import label4
    sc = ctx.sc
    if sc.th["kind"] in ("indoor", "cave") or not sc.rooms.any():
        return
    lab, n = label4(sc.rooms)
    red = mm.cloth("red")
    gold = mm.gold()
    for i in range(n):
        m = lab == i
        rpx = pix.upsample_cells(m, T, smooth=False) > 0.5
        ctx.below.shade(0, 0, -1.0, rpx)
        if sc.theme not in ("sect", "manor"):
            continue
        ys, xs = np.nonzero(m)
        x0, x1, y0, y1 = xs.min(), xs.max(), ys.min(), ys.max()
        door = [c for g in sc.gaps if g.orient == "h" for c in g.cells if c[1] == y1 + 1 and x0 <= c[0] <= x1]
        if not door:
            continue
        dx0 = min(c[0] for c in door) * T + 2
        dx1 = (max(c[0] for c in door) + 1) * T - 2
        top = (y0 + 1) * T
        bot = (y1 + 1) * T
        cv = ctx.below
        cv.rect(dx0, top, dx1 - dx0, bot - top, red, 2)
        for yy in range(top, bot, 4):
            cv.rect(dx0 + 2, yy, dx1 - dx0 - 4, 1, red, 3)
        cv.rect(dx0, top, 1, bot - top, gold, 2)
        cv.rect(dx1 - 1, top, 1, bot - top, gold, 1)
        cv.rect(dx0, top, dx1 - dx0, 1, gold, 3)
