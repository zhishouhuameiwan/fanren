# -*- coding: utf-8 -*-
"""战斗音效：打击、破绽、破势、蓄劲、治疗、中毒、五行术、落空、倒下、遇敌。

**破势（break）是全作最用心的一声**：八方旅人的破盾声之所以抓人，是它在极短的时间里
叠了四件事——先「吸」一口（倒放的气流涌上来），然后一记低沉的重击把人按下去，
重击上面炸开一片清脆的碎裂声向两侧飞散，最后是一个拖长的余韵。这里逐层照做：
  吸（倒放气流 110 ms）→ 裂（3 ms 宽带「啪」）→ 砸（95→36 Hz 下滑正弦，削波加重，外加 48 Hz 次低音）
  → 碎（一百多粒高频小叮当，指数变稀，左右散开）+ 砂（3–9 kHz 的碎屑噪声）
  → 余韵：一记滤暗了的大锣「哐」——这是把它变成「修仙」而不是「打碎玻璃」的那一笔。

蓄劲三档用同一套积木、逐档加码：音高（A5 → E6+A6 → A6+E7+A7）、气流（更宽、更长、更亮）、
第三档再加一口次低音与铃的颤光。玩家闭着眼也能听出是第几档。
"""
from __future__ import annotations

import math

import numpy as np

import percussion
from dsp import (LN1000, SR, TWO_PI, bandpass, highpass, lowpass, pan_mono, samples, soft_clip,
                 time_axis)
from sfx_common import (Sfx, env_ar, noise_sweep, place, pluck, reverb, shards, stereo, sweep_sine,
                        widen)


def _noise_path(dur: float, rng: np.random.Generator, center, q: float = 1.2,
                steps: int = 40) -> np.ndarray:
    """中心频率沿任意曲线 center(t) 移动的带通噪声（风的回旋、刀风的起落）。"""
    n = samples(dur)
    noise = rng.standard_normal(n + samples(0.05))
    out = np.zeros(n)
    seg = n / steps
    win_len = int(seg * 2)
    window = np.hanning(win_len)
    for k in range(steps + 1):
        c = float(center(k / steps * dur))
        lo = max(30.0, c / (1.0 + 0.5 / q))
        hi = min(0.45 * SR, c * (1.0 + 0.5 / q))
        start = int(k * seg - seg)
        a, b = max(start, 0), min(start + win_len, n)
        if b <= a:
            continue
        out[a:b] += bandpass(noise[a:b + samples(0.05)], lo, hi)[:b - a] * window[a - start:b - start]
    return out / max(float(np.max(np.abs(out))), 1e-9)


def _ring(freqs, amps, t60s, dur: float, rng: np.random.Generator, beat: float = 0.0) -> np.ndarray:
    """金属的「嗡——」：几组衰减正弦，可带孪生的拍频。"""
    n = samples(dur)
    t = time_axis(n)
    out = np.zeros(n)
    for f, a, t60 in zip(freqs, amps, t60s):
        env = np.exp(-LN1000 * t / t60)
        out += a * env * np.sin(TWO_PI * f * t + rng.uniform(0, TWO_PI))
        if beat:
            out += 0.7 * a * env * np.sin(TWO_PI * (f + beat * rng.uniform(0.6, 1.4)) * t + rng.uniform(0, TWO_PI))
    return out


def _crack(rng: np.random.Generator, dur: float = 0.03, lo: float = 2000.0, hi: float = 9500.0,
           tau: float = 0.006) -> np.ndarray:
    n = samples(dur)
    return bandpass(rng.standard_normal(n), lo, hi) * np.exp(-time_axis(n) / tau)


def hit_slash(rng: np.random.Generator) -> np.ndarray:
    """刀剑斩击：刀风「嗖」地升上去，接着刃口入肉的「嚓」与一丝金属的「铮」。"""
    out = np.zeros((2, samples(0.6)))
    swish = noise_sweep(0.09, rng, 1500.0, 7500.0, q=1.3) * env_ar(samples(0.09), 0.065, 0.06)
    place(out, stereo(0.45 * swish, -0.35), 0.0)
    shing = _ring(rng.uniform(2800.0, 7200.0, 6), [1.0, 0.8, 0.6, 0.5, 0.4, 0.3],
                  [0.22, 0.18, 0.15, 0.12, 0.1, 0.08], 0.3, rng)
    body = sweep_sine(0.08, 230.0, 90.0) * env_ar(samples(0.08), 0.002, 0.07)
    hit = np.zeros(samples(0.3))
    hit[:samples(0.03)] += 0.9 * _crack(rng)
    hit += 0.18 * shing
    hit[:body.size] += 0.7 * body
    place(out, stereo(hit, 0.1), 0.075)
    return reverb(out, "plate", "hit_slash", 0.12, 0.3)


def hit_blunt(rng: np.random.Generator) -> np.ndarray:
    """钝击（拳脚、棍棒）：一记往下沉的闷响加一点骨肉的「噗」。"""
    n = samples(0.45)
    t = time_axis(n)
    body = sweep_sine(0.45, 160.0, 42.0, curve=0.55) * env_ar(n, 0.002, 0.24)
    crunch = lowpass(rng.standard_normal(n), 1600.0) * np.exp(-t / 0.022)
    slap = np.zeros(n)
    slap[:samples(0.004)] = highpass(rng.standard_normal(samples(0.004)), 1500.0)
    mono = soft_clip(1.3 * body + 0.5 * crunch + 0.6 * slap, 1.6)
    return reverb(stereo(mono), "plate", "hit_blunt", 0.08, 0.25)


def hit_weak(rng: np.random.Generator) -> np.ndarray:
    """打中破绽：短促的斩击 + 两只小铃在一个五度上「叮！」——清脆、带金属光。"""
    out = np.zeros((2, samples(1.0)))
    hit = np.zeros(samples(0.2))
    hit[:samples(0.03)] += _crack(rng, lo=2500.0)
    hit[:samples(0.08)] += 0.5 * sweep_sine(0.08, 260.0, 110.0) * env_ar(samples(0.08), 0.002, 0.07)
    place(out, stereo(hit), 0.0)
    ting = (percussion.bell(93.0, 0.9, rng, "chime") + 0.7 * percussion.bell(100.0, 0.85, rng, "chime"))
    clang = _ring([2450.0, 3900.0, 5600.0], [1.0, 0.7, 0.4], [0.35, 0.25, 0.18], 0.4, rng, beat=9.0)
    glint = sweep_sine(0.07, 3200.0, 6400.0) * env_ar(samples(0.07), 0.01, 0.05)
    place(out, widen(0.9 * ting, rng, 0.6), 0.012)
    place(out, stereo(0.25 * clang), 0.012)
    place(out, stereo(0.08 * glint, 0.3), 0.02)
    return reverb(out, "plate", "hit_weak", 0.2, 0.4)


def break_(rng: np.random.Generator) -> np.ndarray:
    """破势：吸—裂—砸—碎—余韵，见文件头。"""
    t_hit = 0.11
    out = np.zeros((2, samples(2.3)))
    suck_n = samples(t_hit)
    suck = noise_sweep(t_hit, rng, 500.0, 4500.0, q=0.9) * np.linspace(0.0, 1.0, suck_n) ** 2.2
    place(out, stereo(0.35 * suck), 0.0)
    place(out, stereo(1.1 * highpass(_crack(rng, 0.008, 1200.0, 16000.0, 0.0015), 1000.0)), t_hit)
    boom = sweep_sine(0.8, 95.0, 36.0, curve=0.5) * env_ar(samples(0.8), 0.003, 0.6)
    sub = np.sin(TWO_PI * 48.0 * time_axis(samples(1.0))) * env_ar(samples(1.0), 0.006, 0.85)
    thump = soft_clip(1.6 * boom, 2.2)
    low = np.zeros(samples(1.0))
    low[:thump.size] += thump
    low += 0.55 * sub
    place(out, stereo(0.95 * low), t_hit)
    place(out, 1.4 * shards(rng, 1.4, 150, 2500.0, 9500.0, decay=0.2), t_hit + 0.004)
    grit_n = samples(0.6)
    grit = bandpass(rng.standard_normal(grit_n), 3000.0, 9000.0) * np.exp(-time_axis(grit_n) / 0.13)
    place(out, widen(0.35 * grit, rng, 0.8), t_hit)
    gong = lowpass(percussion.daluo(0.9, rng, 0.82), 2600.0)
    place(out, widen(0.28 * gong, rng, 0.5), t_hit + 0.01)
    return reverb(out, "plate", "break", 0.18, 0.6)


def _boost(rng: np.random.Generator, level: int) -> np.ndarray:
    """蓄劲一到三档：逐档升高的「叮」+ 逐档更宽更亮的气流。"""
    pitches = {1: (81.0,), 2: (88.0, 93.0), 3: (93.0, 100.0, 105.0)}[level]
    lo, hi, dur = {1: (400.0, 1300.0, 0.3), 2: (500.0, 2300.0, 0.36), 3: (600.0, 3900.0, 0.44)}[level]
    out = np.zeros((2, samples(1.3)))
    air = noise_sweep(dur, rng, lo, hi, q=1.0) * env_ar(samples(dur), dur * 0.7, dur * 0.8)
    n = air.size
    pan = np.linspace(-0.6, 0.6, n)
    air_st = np.stack([air * np.cos((pan + 1) * math.pi / 4), air * np.sin((pan + 1) * math.pi / 4)])
    place(out, (0.35 + 0.1 * level) * air_st, 0.0)
    for k, midi in enumerate(pitches):
        bell = percussion.bell(midi, 0.75, rng, "chime")
        place(out, widen((0.9 - 0.15 * k) * bell, rng, 0.5), dur * 0.55 + 0.035 * k)
    if level == 3:
        m = samples(0.5)
        t = time_axis(m)
        thoom = np.sin(TWO_PI * 68.0 * t) * env_ar(m, 0.01, 0.4)
        place(out, stereo(0.5 * thoom), dur * 0.55)
        shimmer = percussion.bell(112.0, 0.6, rng, "pengling")
        shimmer = shimmer * (0.6 + 0.4 * np.sin(TWO_PI * 11.0 * time_axis(shimmer.size)))
        place(out, widen(0.35 * shimmer, rng, 0.7), dur * 0.55 + 0.1)
    return reverb(out, "hall", f"boost_{level}", 0.22, 0.4)


def heal(rng: np.random.Generator) -> np.ndarray:
    """治疗：一串上行的清铃（A 宫五声），底下一团缓缓升起的暖和声，再撒一把细碎的光。"""
    out = np.zeros((2, samples(2.0)))
    for k, midi in enumerate((81.0, 83.0, 85.0, 88.0, 90.0, 93.0)):
        place(out, widen(0.55 * percussion.bell(midi, 0.55 + 0.05 * k, rng, "chime"), rng, 0.6), 0.065 * k)
    m = samples(1.5)
    t = time_axis(m)
    pad = sum(a * np.sin(TWO_PI * 440.0 * 2 ** (s / 12) * t + rng.uniform(0, TWO_PI))
              for s, a in ((0, 1.0), (7, 0.7), (12, 0.6), (16, 0.35)))
    pad *= np.sin(np.pi * np.clip(t / 1.5, 0, 1)) ** 1.5
    place(out, stereo(0.28 * pad), 0.05)
    sparkle = highpass(rng.standard_normal(m), 6000.0)
    flick = (rng.random(m // 441 + 1) > 0.6).astype(float)
    sparkle *= np.repeat(flick, 441)[:m] * np.sin(np.pi * t / 1.5) * 0.05
    place(out, widen(sparkle, rng, 0.9), 0.1)
    return reverb(out, "hall", "heal", 0.3, 0.5)


def poison(rng: np.random.Generator) -> np.ndarray:
    """中毒：浑浊的「咕嘟」气泡，底下两条相差半音、摇摆不定的低音——恶心、发晕。"""
    dur = 1.2
    out = np.zeros((2, samples(dur + 0.2)))
    for _ in range(20):
        d = rng.uniform(0.03, 0.07)
        m = samples(d)
        t = time_axis(m)
        f = rng.uniform(180.0, 320.0) * (1.0 + 1.3 * t / d)
        blip = np.sin(TWO_PI * np.cumsum(f) / SR) * np.sin(np.pi * t / d)
        place(out, pan_mono(blip * rng.uniform(0.3, 0.8), rng.uniform(-0.7, 0.7)), rng.uniform(0.0, dur - 0.1))
    n = samples(dur)
    t = time_axis(n)
    wob = 1.0 + 0.012 * np.sin(TWO_PI * 5.5 * t)
    drone = (np.sin(TWO_PI * 185.0 * np.cumsum(wob) / SR) + 0.8 * np.sin(TWO_PI * 196.0 * np.cumsum(wob) / SR))
    drone *= np.sin(np.pi * t / dur) ** 0.8
    gurgle = lowpass(rng.standard_normal(n), 320.0) * (0.5 + 0.5 * np.sin(TWO_PI * 3.1 * t) ** 2)
    place(out, stereo(0.25 * drone + 0.6 * gurgle / max(float(np.max(np.abs(gurgle))), 1e-9)), 0.0)
    return reverb(out, "room", "poison", 0.2, 0.3)


def fire_cast(rng: np.random.Generator) -> np.ndarray:
    """火：「呼」地一下点着，接着是翻滚的火舌与噼啪的火星。"""
    dur = 1.5
    out = np.zeros((2, samples(dur + 0.3)))
    ignite = noise_sweep(0.5, rng, 250.0, 2800.0, q=0.7) * env_ar(samples(0.5), 0.3, 0.3)
    place(out, widen(0.6 * ignite, rng, 0.6), 0.0)
    n = samples(1.2)
    flicker = 0.6 + 0.4 * np.abs(lowpass(rng.standard_normal(n), 12.0)) / 0.05
    flicker = np.clip(flicker / max(float(np.max(flicker)), 1e-9), 0.2, 1.0)
    roar = lowpass(rng.standard_normal(n), 900.0) * flicker * env_ar(n, 0.15, 1.0)
    place(out, widen(0.55 * roar / max(float(np.max(np.abs(roar))), 1e-9), rng, 0.7), 0.2)
    for _ in range(45):
        m = samples(rng.uniform(0.001, 0.004))
        pop = bandpass(rng.standard_normal(m + 64), 1500.0, 7000.0)[:m] * np.hanning(m)
        place(out, pan_mono(pop * rng.uniform(0.3, 1.0), rng.uniform(-0.8, 0.8)), rng.uniform(0.25, 1.3))
    boom = sweep_sine(0.4, 75.0, 40.0) * env_ar(samples(0.4), 0.01, 0.35)
    place(out, stereo(0.45 * boom), 0.28)
    return reverb(out, "plate", "fire_cast", 0.15, 0.3)


def wind_cast(rng: np.random.Generator) -> np.ndarray:
    """风：三圈回旋的风，中心频率上下翻卷，声像左右掠过，带一丝口哨。"""
    dur = 1.3
    swirl = _noise_path(dur, rng, lambda t: 1000.0 * 2.0 ** (1.3 * math.sin(TWO_PI * 2.3 * t - 1.2)), q=1.6)
    n = swirl.size
    t = time_axis(n)
    env = np.sin(np.pi * t / dur) ** 0.8
    whistle_f = 1400.0 * 2.0 ** (0.9 * np.sin(TWO_PI * 2.3 * t - 1.2))
    whistle = np.sin(TWO_PI * np.cumsum(whistle_f) / SR) * 0.06
    mono = (swirl + whistle) * env
    pan = np.sin(TWO_PI * 1.15 * t)
    st = np.stack([mono * np.cos((pan + 1) * math.pi / 4), mono * np.sin((pan + 1) * math.pi / 4)])
    return reverb(st, "hall", "wind_cast", 0.2, 0.4)


def metal_cast(rng: np.random.Generator) -> np.ndarray:
    """金：一声剑鸣——短促的「铿」，然后是长长的、带拍频的金属余音，末尾一道「锃」地上扬的锋光。"""
    out = np.zeros((2, samples(1.8)))
    clang = np.zeros(samples(0.05))
    clang += highpass(_crack(rng, 0.05, 1500.0, 12000.0, 0.004), 1200.0)
    place(out, stereo(0.7 * clang), 0.0)
    ring = _ring([1180.0, 2845.0, 3660.0, 5550.0], [1.0, 0.6, 0.45, 0.25], [1.3, 0.9, 0.7, 0.45], 1.6, rng,
                 beat=3.5)
    place(out, widen(0.5 * ring, rng, 0.7), 0.003)
    shing = noise_sweep(0.3, rng, 3000.0, 9000.0, q=2.0) * env_ar(samples(0.3), 0.22, 0.12)
    place(out, stereo(0.18 * shing, 0.3), 0.25)
    return reverb(out, "plate", "metal_cast", 0.18, 0.3)


def miss(rng: np.random.Generator) -> np.ndarray:
    """落空：一口掠过的空气，起落都快，没有任何撞击。"""
    dur = 0.24
    x = _noise_path(dur, rng, lambda t: 800.0 * 2.0 ** (1.7 * math.sin(math.pi * t / dur)), q=1.0, steps=20)
    n = x.size
    t = time_axis(n)
    x *= np.sin(np.pi * t / dur) ** 1.2
    pan = np.linspace(-0.7, 0.7, n)
    st = np.stack([x * np.cos((pan + 1) * math.pi / 4), x * np.sin((pan + 1) * math.pi / 4)])
    return reverb(st, "room", "miss", 0.08, 0.15)


def enemy_down(rng: np.random.Generator) -> np.ndarray:
    """敌人倒下：往下沉的「呜——」，化作一阵烟散掉，一记很轻的大锣收尾。"""
    out = np.zeros((2, samples(1.8)))
    whoom = soft_clip(sweep_sine(0.55, 320.0, 55.0, curve=0.8) * env_ar(samples(0.55), 0.01, 0.5), 1.5)
    place(out, stereo(0.6 * whoom), 0.0)
    m = samples(0.6)
    poof = lowpass(rng.standard_normal(m), 1400.0) * env_ar(m, 0.06, 0.45)
    place(out, widen(0.4 * poof, rng, 0.7), 0.1)
    place(out, 0.35 * shards(rng, 0.8, 30, 1500.0, 5000.0, decay=0.3), 0.15)
    gong = lowpass(percussion.daluo(0.5, rng, 1.1), 1500.0)
    place(out, widen(0.2 * gong, rng, 0.5), 0.12)
    return reverb(out, "hall", "enemy_down", 0.18, 0.3)


def ally_down(rng: np.random.Generator) -> np.ndarray:
    """我方倒下：古琴最低处一声，按住往下滑（走手音），一记低磬陪着它沉下去。"""
    # 记谱时值取短（0.6 s）：古琴自己的余韵有七八秒，要让它在缓冲区里按止音包络自然收掉，
    # 而不是被缓冲区的尾巴截断（截断处哪怕做了淡出，也听得出是「掐掉」的）。
    out = np.zeros((2, samples(3.0)))
    qin = pluck("guqin", 45.0, 0.6, 0.85, rng, "v")
    place(out, stereo(0.9 * qin), 0.0)
    place(out, stereo(0.3 * percussion.bell(57.0, 0.5, rng, "qing"), 0.2), 0.05)
    return reverb(out, "hall", "ally_down", 0.25, 0.4)


def encounter(rng: np.random.Generator) -> np.ndarray:
    """遇敌转场：一口越来越急的呼啸（外加倒放的钹）直冲上去，砸在一记鼓 + 大锣 + 琵琶扫弦上。"""
    t_hit = 0.75
    out = np.zeros((2, samples(2.1)))
    rise = noise_sweep(t_hit, rng, 180.0, 5200.0, q=0.8) * np.linspace(0.0, 1.0, samples(t_hit)) ** 2.5
    place(out, widen(0.55 * rise, rng, 0.8), 0.0)
    cym = percussion.bo(0.8, rng)[:samples(t_hit)][::-1]
    place(out, widen(0.3 * cym, rng, 0.6), 0.0)
    place(out, stereo(1.0 * percussion.tanggu(1.0, rng, 0.9)), t_hit)
    place(out, widen(0.55 * percussion.daluo(0.9, rng, 0.95), rng, 0.4), t_hit)
    for k, midi in enumerate((45.0, 52.0, 57.0, 58.0)):
        place(out, stereo(0.35 * pluck("pipa", midi, 0.25, 0.9, rng), -0.3 + 0.2 * k), t_hit + 0.012 * k)
    return reverb(out, "plate", "encounter", 0.18, 0.4)


SFX = (
    Sfx("hit_slash", "刀剑类攻击命中", "带通噪声刀风上扬 + 宽带「嚓」+ 六个随机高频金属模态 + 230→90 Hz 的肉感",
        hit_slash, -15.0, 0.2, 1.1),
    Sfx("hit_blunt", "拳脚/钝器命中", "160→42 Hz 下沉正弦 + 低通碎响 + 巴掌声，轻度削波", hit_blunt, -15.0, 0.2, 1.0),
    Sfx("hit_weak", "打中破绽（弱点）", "短斩击 + 两只小铃在五度上「叮」（A6 与 E7）+ 带拍频的金属泛音 + 上扬锋光",
        hit_weak, -14.0, 0.4, 1.6),
    Sfx("break", "破势（护盾被打破）", "吸（倒放气流）→ 裂（3 ms 宽带）→ 砸（95→36 Hz 削波 + 48 Hz 次低音）→ "
        "碎（150 粒高频小叮当）+ 碎屑噪声 → 滤暗的大锣余韵", break_, -11.0, 1.2, 3.2),
    Sfx("boost_1", "蓄劲一档", "A5 清铃 + 0.4→1.3 kHz 的气流由左至右", lambda r: _boost(r, 1), -18.0, 0.5, 1.9),
    Sfx("boost_2", "蓄劲二档", "E6 + A6 两只清铃 + 更宽的气流", lambda r: _boost(r, 2), -17.0, 0.5, 2.0),
    Sfx("boost_3", "蓄劲三档", "A6 + E7 + A7 三只清铃 + 最亮的气流 + 68 Hz 次低音 + 碰铃颤光",
        lambda r: _boost(r, 3), -15.5, 0.6, 2.2),
    Sfx("heal", "回复气血/法力", "A 宫五声清铃上行 + 暖和声缓起 + 细碎的高频闪光", heal, -18.0, 1.0, 2.8),
    Sfx("poison", "中毒/毒伤", "二十个上滑的「咕嘟」+ 相差半音、带摇摆的低音 + 低通咕噜声", poison, -18.0, 0.8, 2.0),
    Sfx("fire_cast", "火系法术", "点火的上扬呼声 + 闪烁的低通火舌 + 四十五粒火星噼啪 + 低音「轰」", fire_cast, -15.0,
        1.0, 2.4),
    Sfx("wind_cast", "风系法术（兼木系）", "中心频率回旋三圈的带通噪声 + 口哨 + 声像左右掠过", wind_cast, -16.0, 1.0, 2.2),
    Sfx("metal_cast", "金系法术", "宽带「铿」+ 四个带拍频的长金属模态（剑鸣）+ 3→9 kHz 的锋光", metal_cast, -15.0,
        1.0, 2.6),
    Sfx("miss", "攻击落空/闪避", "中心频率一起一落的气流，声像掠过，无撞击", miss, -21.0, 0.15, 0.8),
    Sfx("enemy_down", "敌人被击倒", "320→55 Hz 的下沉 + 低通烟散声 + 少量碎光 + 很轻的大锣", enemy_down, -17.0, 0.8, 2.4),
    Sfx("ally_down", "我方倒下", "古琴最低音（A2）按弦下滑 + 低磬", ally_down, -18.0, 1.0, 3.8),
    Sfx("encounter", "遇敌转场的呼啸", "0.75 秒渐急的上扬呼啸 + 倒放的钹 → 堂鼓 + 大锣 + 琵琶小二度扫弦", encounter,
        -14.0, 1.2, 2.8),
)
