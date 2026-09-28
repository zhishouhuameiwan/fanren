# -*- coding: utf-8 -*-
"""界面与日常音效：光标、确定、取消、开合菜单、错误、得物、存档、开门、脚步。

界面音是全作听得最多的声音——一局游戏光标声要响几千次，所以原则是「轻、短、有音高、不刺」：
全部用 BGM 里同一把古筝的高音区拨弦做底（音色统一，又都在五声音阶上，
叠在任何一首 BGM 上都不会走调——BGM 的宫音各不相同，所以只取 A 宫的五个音，
它们与大多数曲子的调式音都在一个五度圈的近邻上，不会撞出小二度）。
"""
from __future__ import annotations

import numpy as np

import percussion
from dsp import SR, bandpass, lowpass, samples, time_axis
from sfx_common import Sfx, env_ar, noise_sweep, place, pluck, reverb, stereo

# A 宫五声音阶的 MIDI：A B C# E F#
_A5, _B5, _CS6, _E6, _FS6, _A6 = 81.0, 83.0, 85.0, 88.0, 90.0, 93.0


def ui_cursor(rng: np.random.Generator) -> np.ndarray:
    """光标：一声极短的高音拨弦，甲片轻擦，几乎只剩「嗒」里的一点音高。"""
    x = pluck("guzheng", _E6, 0.03, 0.45, rng, "!")
    x = lowpass(x, 7000.0)
    return reverb(x[:samples(0.1)], "room", "ui_cursor", 0.08, 0.12)


def ui_confirm(rng: np.random.Generator) -> np.ndarray:
    """确定：两声上行的拨弦（徵→宫），第二声亮一点，带一丝碰铃的余光。"""
    n = samples(0.5)
    out = np.zeros(n)
    place(out, pluck("guzheng", _E6, 0.08, 0.6, rng, "!"), 0.0)
    place(out, pluck("guzheng", _A6, 0.18, 0.75, rng), 0.055)
    bell = percussion.bell(_A6 + 12.0, 0.5, rng, "chime")
    place(out, 0.12 * bell, 0.055)
    return reverb(out, "room", "ui_confirm", 0.15, 0.3)


def ui_cancel(rng: np.random.Generator) -> np.ndarray:
    """取消：两声下行、闷一点的拨弦（宫→羽），尾巴收得快。"""
    n = samples(0.4)
    out = np.zeros(n)
    place(out, pluck("guzheng", _CS6, 0.05, 0.5, rng, "!"), 0.0)
    place(out, pluck("guzheng", _A5, 0.08, 0.45, rng, "!"), 0.06)
    out = lowpass(out, 4500.0)
    return reverb(out, "room", "ui_cancel", 0.1, 0.2)


def ui_open(rng: np.random.Generator) -> np.ndarray:
    """打开菜单：一串上行的轻拨（小刮奏）加一口「展卷」的气声。"""
    n = samples(0.7)
    out = np.zeros(n)
    for k, midi in enumerate((_A5, _B5, _CS6, _E6, _FS6)):
        place(out, pluck("guzheng", midi, 0.12, 0.35 + 0.08 * k, rng), 0.028 * k)
    air = noise_sweep(0.25, rng, 900.0, 3500.0, q=1.2) * env_ar(samples(0.25), 0.12, 0.2)
    place(out, 0.18 * air, 0.0)
    return reverb(out, "room", "ui_open", 0.18, 0.35)


def ui_close(rng: np.random.Generator) -> np.ndarray:
    """关闭菜单：下行的轻拨加一口收拢的气声，比打开短、比打开轻。"""
    n = samples(0.55)
    out = np.zeros(n)
    for k, midi in enumerate((_E6, _CS6, _B5, _A5)):
        place(out, pluck("guzheng", midi, 0.08, 0.5 - 0.06 * k, rng, "!"), 0.026 * k)
    air = noise_sweep(0.18, rng, 3000.0, 900.0, q=1.2) * env_ar(samples(0.18), 0.05, 0.15)
    place(out, 0.14 * air, 0.0)
    return reverb(lowpass(out, 6000.0), "room", "ui_close", 0.14, 0.25)


def ui_error(rng: np.random.Generator) -> np.ndarray:
    """错误 / 不可用：低沉的木鱼两下「笃笃」，外加一声被按住的闷弦——明确地「不行」，但不吓人。"""
    n = samples(0.45)
    out = np.zeros(n)
    wood = percussion.muyu(0.7, rng, 0.62)
    place(out, wood, 0.0)
    place(out, 0.8 * percussion.muyu(0.6, rng, 0.6), 0.09)
    dull = lowpass(pluck("pipa", 57.0, 0.05, 0.5, rng, "!"), 1800.0)
    place(out, 0.35 * dull, 0.0)
    return reverb(out, "room", "ui_error", 0.05, 0.08)  # 错误提示要干脆，只带一点点房间感


def item_get(rng: np.random.Generator) -> np.ndarray:
    """得到物品：三声上行的清亮小铃（宫、角、徵），末一声带碰铃的闪光。"""
    n = samples(1.3)
    out = np.zeros(n)
    for k, midi in enumerate((_A5, _CS6, _E6)):
        place(out, percussion.bell(midi + 12.0, 0.6 + 0.1 * k, rng, "chime") * (0.7 + 0.15 * k), 0.07 * k)
    place(out, 0.35 * percussion.bell(_A6 + 12.0, 0.5, rng, "pengling"), 0.21)
    return reverb(out, "hall", "item_get", 0.25, 0.4)


def save(rng: np.random.Generator) -> np.ndarray:
    """存档：一声小磬，然后是毛笔在纸上一抹的沙沙声。"""
    n = samples(1.6)
    out = np.zeros(n)
    place(out, 0.8 * percussion.bell(_E6, 0.55, rng, "qing"), 0.0)
    m = samples(0.38)
    brush = noise_sweep(0.38, rng, 2200.0, 5200.0, q=0.8)
    t = time_axis(m)
    brush *= np.sin(np.pi * t / t[-1]) ** 1.5 * (0.6 + 0.4 * np.sin(2 * np.pi * 7.0 * t))
    place(out, 0.22 * brush, 0.18)
    return reverb(out, "room", "save", 0.12, 0.2)


def door_open(rng: np.random.Generator) -> np.ndarray:
    """开门：门闩「咔」一下，木门轴「吱——呀」（黏滑摩擦：一串不规则的脉冲过木头共鸣），末了一声轻碰。"""
    n = samples(1.0)
    out = np.zeros(n)
    click = percussion.paiban(0.6, rng, 1.3)[:samples(0.06)]
    place(out, 0.5 * click, 0.0)
    dur = 0.55
    m = samples(dur)
    t = time_axis(m)
    f = 95.0 + 140.0 * (t / dur) ** 0.7 + 25.0 * np.sin(2 * np.pi * 3.3 * t)
    f *= 1.0 + 0.08 * rng.standard_normal(m).cumsum() / np.sqrt(np.arange(1, m + 1))
    phase = np.cumsum(f) / SR
    pulses = (np.diff(np.floor(phase), prepend=0.0) > 0).astype(float)
    pulses *= 0.6 + 0.4 * rng.random(m)
    creak = np.zeros(m)
    for fc, g in ((620.0, 1.0), (1350.0, 0.6), (2600.0, 0.35)):
        creak += g * bandpass(pulses, fc * 0.85, fc * 1.15)
    creak *= np.sin(np.pi * t / dur) ** 0.6
    place(out, 0.9 * creak / max(float(np.max(np.abs(creak))), 1e-9), 0.08)
    thump = np.sin(2 * np.pi * 85.0 * time_axis(samples(0.12))) * env_ar(samples(0.12), 0.004, 0.09)
    place(out, 0.5 * thump, 0.66)
    return reverb(out, "room", "door_open", 0.2, 0.3)


def footstep_soft(rng: np.random.Generator) -> np.ndarray:
    """脚步：踩在土路上的一下，极轻、极短，只有一点低频的「噗」与沙粒声。"""
    n = samples(0.14)
    t = time_axis(n)
    grit = lowpass(rng.standard_normal(n), 2400.0) * np.exp(-t / 0.018)
    thud = np.sin(2 * np.pi * 110.0 * t) * np.exp(-t / 0.03)
    return stereo(0.6 * grit + thud)


SFX = (
    Sfx("ui_cursor", "光标在菜单/选项间移动", "古筝高音角音（E6）的顿音拨弦，低通到 7 kHz，一点点小室混响",
        ui_cursor, -27.0, 0.05, 0.4),
    Sfx("ui_confirm", "确定、选中", "古筝徵→宫（E6→A6）两声上行拨弦，第二声带清亮小铃的余光",
        ui_confirm, -21.0, 0.2, 0.9),
    Sfx("ui_cancel", "取消、返回", "古筝宫→羽（C#6→A5）两声下行顿音，低通发闷", ui_cancel, -23.0, 0.15, 0.7),
    Sfx("ui_open", "打开主菜单/面板", "五声上行的小刮奏 + 一口展卷般的气声（带通噪声 0.9→3.5 kHz）",
        ui_open, -22.0, 0.3, 1.2),
    Sfx("ui_close", "关闭主菜单/面板", "四声下行的轻拨 + 收拢的气声", ui_close, -23.0, 0.2, 1.0),
    Sfx("ui_error", "不可用、条件不足", "低木鱼「笃笃」两下 + 一声按住的琵琶闷弦", ui_error, -22.0, 0.15, 0.6),
    Sfx("item_get", "获得物品", "清亮小铃三声上行（宫、角、徵）+ 碰铃闪光", item_get, -19.0, 0.6, 1.8),
    Sfx("save", "存档完成", "一声小磬 + 毛笔抹过纸面的沙沙声", save, -20.0, 0.8, 2.2),
    Sfx("door_open", "开门", "门闩咔哒 + 黏滑摩擦的「吱呀」（不规则脉冲过三个木头共振峰）+ 轻碰",
        door_open, -20.0, 0.6, 1.6),
    Sfx("footstep_soft", "脚步（很轻，可能不用）", "低通沙粒噪声 + 110 Hz 的一点「噗」", footstep_soft, -30.0,
        0.05, 0.3),
)
