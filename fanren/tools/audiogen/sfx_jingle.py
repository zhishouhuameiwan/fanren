# -*- coding: utf-8 -*-
"""短曲：胜利、战败、章节卡、境界提升。

它们是「有旋律的音效」，用与 BGM 同一套乐器与配平办法（sfx_common.render_ensemble），
只是不循环、带自然的尾音。时间都写在「拍」上；章节卡与境界提升用 ♩=60，拍即是秒，好对齐。
"""
from __future__ import annotations

import numpy as np

from score import Key, Note, chords, hits, seq
from sfx_common import Part, Sfx, render_ensemble


def _gliss(key: Key, beat: float, start_deg: int, octave: int, count: int, span: float, vel: float,
           down: bool = False) -> tuple[Note, ...]:
    """古筝刮奏：沿五声音阶扫过 count 根弦，力度渐强。"""
    out = []
    for k in range(count):
        step = count - 1 - k if down else k
        midi = float(key.penta(start_deg, octave, step))
        out.append(Note(beat=beat + span * k / count, beats=1.0, midi=midi,
                        vel=vel * (0.45 + 0.55 * k / max(count - 1, 1))))
    return tuple(out)


def victory(rng: np.random.Generator) -> np.ndarray:
    """胜利（♩=144，约 4.5 秒）：大鼓一记开场、古筝刮上去，笛子一句扬到高八度的宫音收住。"""
    key = Key.of("A4")
    m = 4
    # 末音多留两拍（第三小节的前两拍）：收在高宫音上要让它亮一会儿，不能一到就断。
    dizi = seq(key, m, "| 1'_ 2'_ 3'_ 5'_ 6' 5'_ 6'_ | 1''~ - - - | - - 0 0 |", where="victory.dizi")
    erhu = seq(key, m, "| 5_ 6_ 1'_ 2'_ 3' 2'_ 3'_ | 5'~ - - - | - - 0 0 |", vel=0.6, where="victory.erhu")
    zheng = _gliss(key, 0.0, 1, -1, 14, 1.0, 0.8) + tuple(
        Note(beat=4.0, beats=3.0, midi=float(key.penta(1, -2, s)), vel=0.8, shift=0.018 * i)
        for i, s in enumerate((0, 3, 5, 7, 8)))
    pad = chords(key, m, "1 1", (0, 3, 5, 7), octave=-1, vel=0.5)[4:]
    drum = hits(m, "X... x.x. x.x. xxrr | X... .... .... ....", 16, vel=0.9)
    gong = hits(m, ".... | x...", 4, vel=0.85)
    cym = hits(m, ".... | x...", 4, vel=0.8)
    sparkle = hits(m, ".... | ..x.", 4, midi=105.0, vel=0.5)
    parts = (
        Part("dizi", "dizi", dizi, level=0.0, pan=0.08, send=0.3),
        Part("erhu", "erhu", erhu, level=-4.0, pan=-0.2, send=0.3),
        Part("zheng", "guzheng", zheng, level=-5.0, pan=-0.3, send=0.3, human=(3.0, 0.04)),
        Part("sheng", "sheng", pad, level=-9.0, pan=0.2, send=0.4, human=(0.0, 0.0)),
        Part("drum", "perc:tanggu", drum, level=-6.0, pan=0.0, send=0.25, human=(3.0, 0.05)),
        Part("gong", "perc:xiaoluo", gong, level=-10.0, pan=0.25, send=0.3, human=(0.0, 0.0)),
        Part("cym", "perc:bo", cym, level=-11.0, pan=-0.15, send=0.3, human=(0.0, 0.0)),
        Part("sparkle", "bell:pengling", sparkle, level=-15.0, pan=0.4, send=0.5, human=(0.0, 0.0)),
    )
    return render_ensemble(parts, 144.0, 4.9, "hall", "victory")


def defeat(rng: np.random.Generator) -> np.ndarray:
    """战败（♩=90，3/4 两小节）：古琴一句下行落到低羽音，二胡长音往下滑，一记闷锣。"""
    key = Key.of("C4")
    m = 3
    qin = seq(key, m, "| 3 2 1 | 6,~ - - |", octave=-1, vel=0.75, where="defeat.qin")
    erhu = seq(key, m, "| 6~ - 5s | 3~ - - |", vel=0.55, where="defeat.erhu")
    drone = seq(key, m, "| 0 0 0 | 6, - - |", octave=-1, vel=0.45, where="defeat.drone")
    gong = hits(m, "... | x..", 3, vel=0.6)
    parts = (
        Part("qin", "guqin", qin, level=0.0, pan=-0.1, send=0.35, human=(10.0, 0.04)),
        Part("erhu", "erhu", erhu, level=-2.0, pan=0.2, send=0.35, human=(6.0, 0.03)),
        Part("drone", "dahu", drone, level=-9.0, pan=0.0, send=0.3, human=(0.0, 0.0)),
        Part("gong", "perc:daluo", gong, level=-9.0, pan=0.0, send=0.4, human=(0.0, 0.0),
             params={"eq": (("lp", 1800.0),)}),
    )
    return render_ensemble(parts, 90.0, 5.2, "hall", "defeat", meter=m)


def chapter_card(rng: np.random.Generator) -> np.ndarray:
    """章节卡（♩=60，拍即秒）：一记大锣与编钟同响，古筝从低到高刮上去，笙缓缓托起一个宫音和声。"""
    key = Key.of("A4")
    gong = (Note(beat=0.0, beats=1.0, midi=60.0, vel=0.9),)
    bells = (Note(beat=0.0, beats=1.0, midi=57.0, vel=0.8), Note(beat=0.0, beats=1.0, midi=64.0, vel=0.6))
    zheng = _gliss(key, 0.3, 1, -2, 16, 1.1, 0.85)
    pad = (Note(beat=0.5, beats=2.6, midi=57.0, vel=0.5), Note(beat=0.5, beats=2.6, midi=64.0, vel=0.45),
           Note(beat=0.6, beats=2.5, midi=69.0, vel=0.45), Note(beat=0.6, beats=2.5, midi=71.0, vel=0.35))
    sparkle = (Note(beat=1.38, beats=1.0, midi=105.0, vel=0.55),)
    parts = (
        Part("gong", "perc:daluo", gong, level=-3.0, pan=0.0, send=0.35, human=(0.0, 0.0)),
        Part("bells", "bell:bianzhong", bells, level=-5.0, pan=0.2, send=0.45, human=(0.0, 0.0)),
        Part("zheng", "guzheng", zheng, level=0.0, pan=-0.2, send=0.35, human=(4.0, 0.04)),
        Part("sheng", "sheng", pad, level=-8.0, pan=0.15, send=0.45, human=(0.0, 0.0)),
        Part("sparkle", "bell:pengling", sparkle, level=-14.0, pan=0.4, send=0.5, human=(0.0, 0.0)),
    )
    return render_ensemble(parts, 60.0, 4.2, "temple", "chapter_card")


def realm_up(rng: np.random.Generator) -> np.ndarray:
    """境界提升（♩=60）：古筝急刮而上，清铃一串接住，编钟三音同鸣，笙与笛把最后一个宫音托高、托长。"""
    key = Key.of("A4")
    zheng = _gliss(key, 0.0, 1, -1, 16, 0.55, 0.8)
    chimes = tuple(Note(beat=0.5 + 0.09 * k, beats=1.0, midi=m, vel=0.6 + 0.08 * k)
                   for k, m in enumerate((81.0, 85.0, 88.0, 93.0)))
    bells = tuple(Note(beat=0.85, beats=1.0, midi=m, vel=v) for m, v in ((57.0, 0.8), (64.0, 0.65), (69.0, 0.55)))
    pad = tuple(Note(beat=0.3, beats=2.3, midi=m, vel=0.5) for m in (57.0, 64.0, 69.0, 73.0, 76.0))
    dizi = (Note(beat=0.85, beats=1.7, midi=93.0, vel=0.7, orn="~"),)
    parts = (
        Part("zheng", "guzheng", zheng, level=-2.0, pan=-0.2, send=0.35, human=(3.0, 0.04)),
        Part("chimes", "bell:chime", chimes, level=-4.0, pan=0.3, send=0.45, human=(0.0, 0.0)),
        Part("bells", "bell:bianzhong", bells, level=-3.0, pan=0.0, send=0.45, human=(0.0, 0.0)),
        Part("sheng", "sheng", pad, level=-7.0, pan=0.15, send=0.45, human=(0.0, 0.0)),
        Part("dizi", "dizi", dizi, level=-2.0, pan=0.05, send=0.4, human=(0.0, 0.0)),
    )
    return render_ensemble(parts, 60.0, 4.0, "hall", "realm_up")


SFX = (
    Sfx("victory", "战斗胜利（短曲，不循环）", "♩=144 两小节半：堂鼓开场 + 古筝刮奏 + 笛子扬到高八度宫音（二胡在下方和），"
        "第二小节大锣小锣与钹、古筝扫弦、笙和声、碰铃", victory, -15.0, 3.0, 5.0),
    Sfx("defeat", "战斗失败", "♩=90 3/4 两小节：古琴下行落到低羽音 + 二胡长音下滑 + 大胡 + 闷锣", defeat, -18.0,
        2.5, 6.0),
    Sfx("chapter_card", "章节标题卡", "大锣与编钟同响 + 古筝从低到高十六弦刮奏 + 笙托起宫音和声 + 碰铃", chapter_card,
        -16.0, 2.5, 5.0),
    Sfx("realm_up", "境界提升/功法突破", "古筝急刮 + 清铃四声 + 编钟三音同鸣 + 笙 + 笛子高宫音长音", realm_up, -15.0,
        2.0, 4.5),
)
