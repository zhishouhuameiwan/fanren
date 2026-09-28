"""立面上的小件：柱、梁、窗、门、台基、匾、幌子。屋（maproof）与院墙、殿墙（mapwall）共用。

尺度：一格 16px，人物 16×24。两格高的立面（32px）上门高约 20px、窗约 8×7px——
门比人略高，读得出「人走得进去」；一格高的立面只放小窗与矮门。
"""

from __future__ import annotations

import numpy as np

import mapmat as mm
import pix

FACADES = {
    # wall: 墙面渐层名；post: 柱子；beam: 梁；base: 台基；win: 窗框；door: 门板
    "village": dict(wall="earthwall", wall_lv=3, post="wood", post_lv=2, beam="wood", base="rock_cliff",
                    door="wood", door_lv=2, win_frame="wood"),
    "town": dict(wall="plaster", wall_lv=3, post="wood", post_lv=1, beam="wood", base="rock_cliff",
                 door="wood", door_lv=2, win_frame="wood"),
    "valley": dict(wall="plaster", wall_lv=3, post="wood", post_lv=2, beam="wood", base="rock_valley",
                   door="wood", door_lv=2, win_frame="wood"),
    "sect": dict(wall="plaster", wall_lv=4, post="lacquer", post_lv=2, beam="lacquer", base="rock_cliff",
                 door="lacquer", door_lv=2, win_frame="lacquer"),
    "manor": dict(wall="plaster", wall_lv=4, post="darkwood", post_lv=3, beam="darkwood", base="rock_valley",
                  door="darkwood", door_lv=2, win_frame="darkwood"),
    "stone": dict(wall="rock_cliff", wall_lv=3, post="wood", post_lv=1, beam="wood", base="rock_cliff",
                  door="wood", door_lv=1, win_frame="wood"),
}


def rid(name: str) -> int:
    """按名字取材质渐层（先确保已登记）。"""
    table = {
        "earthwall": mm.earthwall, "plaster": mm.plaster, "wood": mm.wood, "darkwood": mm.darkwood,
        "lacquer": mm.lacquer, "rock_cliff": lambda: mm.rock("cliff"), "rock_valley": lambda: mm.rock("valley"),
        "paper": mm.paper, "gold": mm.gold, "brick": mm.brick, "metal": mm.metal,
    }
    return table[name]()


def facade_style(name: str) -> dict:
    return FACADES.get(name, FACADES["town"])


def paint_window(cv: pix.Canvas, x: int, y: int, w: int, h: int, frame: int, night_glow: bool = False) -> None:
    """格子窗：外框 1px、窗纸、每 2px 一根窗棂。窗纸偏暗一档——夜里亮的窗由运行时补光。"""
    paper = mm.paper()
    cv.rect(x, y, w, h, frame, 1)
    cv.rect(x + 1, y + 1, w - 2, h - 2, paper, 2)
    for xx in range(x + 2, x + w - 1, 2):
        cv.rect(xx, y + 1, 1, h - 2, frame, 2)
    cv.rect(x + 1, y + h // 2, w - 2, 1, frame, 2)
    # 上沿一线窗楣的影子
    cv.rect(x + 1, y + 1, w - 2, 1, paper, 1)


def paint_door(cv: pix.Canvas, x: int, y: int, w: int, h: int, style: dict, open_: bool = False) -> None:
    """对开的木门（或敞开的门洞）。门楣一道横木，门缝一线暗，门环两点金。"""
    frame = rid(style["post"])
    cv.rect(x - 1, y - 2, w + 2, 2, frame, style["post_lv"] + 1)
    cv.rect(x - 1, y, 1, h, frame, style["post_lv"])
    cv.rect(x + w, y, 1, h, frame, max(0, style["post_lv"] - 1))
    if open_:
        dark = mm.void()
        cv.rect(x, y, w, h, dark, 1)
        cv.rect(x, y + h - 2, w, 2, dark, 2)
        return
    door = rid(style["door"])
    lv = style["door_lv"]
    cv.rect(x, y, w, h, door, lv)
    half = w // 2
    cv.rect(x + half, y, 1, h, door, 0)
    for yy in range(y + 3, y + h - 1, 4):
        cv.rect(x, yy, w, 1, door, max(0, lv - 1))
    g = mm.gold()
    if h >= 12:
        cv.px(x + half - 2, y + h // 2, g, 3)
        cv.px(x + half + 2, y + h // 2, g, 3)
    cv.rect(x, y, 1, h, door, lv + 1)


def damp(cv: pix.Canvas, x: int, y_plinth: int, w: int, salt: int) -> None:
    """墙根返潮：台基上方参差的一溜暗（每列 1–4px 高）。高度按绝对 x 取、相邻列线性过渡，
    一格一格的墙接得上；只压在墙根，不满墙撒斑——满墙暗斑 ×3 放大后就是脏点。"""
    xs = np.arange(x, x + w)
    k = xs // 6
    f = (xs % 6) / 6.0
    a = pix.hash01(k, 0, salt)
    b = pix.hash01(k + 1, 0, salt)
    hts = 1 + ((a + (b - a) * f) * 4).astype(np.int64)
    rows = np.arange(4)[:, None]
    m = rows >= (4 - hts[None, :])
    cv.shade(x, y_plinth - 4, -1.0, m)


def paint_plinth(cv: pix.Canvas, x: int, y: int, w: int, h: int, base_rid: int) -> None:
    """台基：上沿一线亮、下沿一线暗。"""
    cv.rect(x, y, w, h, base_rid, 3)
    cv.rect(x, y, w, 1, base_rid, 4)
    cv.rect(x, y + h - 1, w, 1, base_rid, 1)


def paint_banner(cv: pix.Canvas, x: int, y: int, kind: str) -> None:
    """幌子：一根挑杆挂一幅布（酒 / 药 / 客栈），只画布与杆，不写字。"""
    pole = mm.wood()
    cloth = mm.cloth({"shop": "indigo", "inn": "ochre", "drug": "white"}.get(kind, "red"))
    cv.rect(x, y, 6, 1, pole, 2)
    cv.rect(x + 1, y + 1, 4, 8, cloth, 3)
    cv.rect(x + 1, y + 1, 1, 8, cloth, 4)
    cv.rect(x + 4, y + 1, 1, 8, cloth, 2)
    cv.rect(x + 2, y + 3, 2, 3, cloth, 1)
    cv.px(x + 1, y + 9, cloth, 2)
    cv.px(x + 3, y + 9, cloth, 2)


def paint_plaque(cv: pix.Canvas, x: int, y: int, w: int) -> None:
    """匾额：深底金边。"""
    g = mm.gold()
    dw = mm.darkwood()
    cv.rect(x, y, w, 5, g, 2)
    cv.rect(x + 1, y + 1, w - 2, 3, dw, 1)
    for xx in range(x + 2, x + w - 2, 3):
        cv.px(xx, y + 2, g, 3)


def lantern(cv: pix.Canvas, x: int, y: int) -> None:
    """一只红灯笼（烘焙里只画灯罩本身；夜里的光晕由 meta.json 的光源交给运行时）。"""
    red = mm.lacquer()
    cv.px(x + 1, y, mm.wood(), 1)
    cv.rect(x, y + 1, 3, 4, red, 3)
    cv.px(x, y + 1, red, 4)
    cv.rect(x + 2, y + 2, 1, 3, red, 2)
    cv.rect(x, y + 5, 3, 1, mm.gold(), 2)
    cv.px(x + 1, y + 6, mm.gold(), 1)
