# -*- coding: utf-8 -*-
"""音效的公共部分：条目定义、收尾（起音干净 + 响度配平 + 限幅）、几样常用的合成积木。

**音效之间的响度协调**：每条音效写一个目标「最大瞬时响度」（LUFS-M，400 ms 窗），
收尾时按它归一再限峰值。短促的音效用瞬时响度比用积分响度更接近「听起来多响」；
目标值按角色分档：界面音最轻（光标 -27 左右，别盖过 BGM），打击 -15 上下，
破势是全作最响的一声（-11），胜利短曲与 BGM 同量级（-15）。BGM 统一在 -16 LUFS。
"""
from __future__ import annotations

import math
from dataclasses import dataclass
from typing import Callable

import numpy as np
from scipy import signal

from dsp import (LN1000, ROOMS, SR, TWO_PI, apply_sos, bandpass, convolve_ir, db_to_amp, eq_sos, fade,
                 highpass, integrated_lufs, limiter, make_ir, momentary_max_lufs, pan_mono,
                 raised_cosine, samples, time_axis, true_peak_db)
from score import Note, humanize, seed_of

SFX_CEILING_DB = -1.5    # 真峰值上限（dBTP）
_TAIL_FLOOR_DB = -62.0   # 尾巴低于峰值这么多就算「响完了」，之后的静音剪掉
PREROLL_S = 0.008        # 开头留的静音，理由见 finalize_sfx


@dataclass(frozen=True)
class Sfx:
    id: str
    title: str                                  # 什么时候响
    how: str                                    # 怎么做的（进文档）
    render: Callable[[np.random.Generator], np.ndarray]
    target: float                               # 目标最大瞬时响度（LUFS-M）
    min_s: float
    max_s: float


def trim_tail(x: np.ndarray, floor_db: float = _TAIL_FLOOR_DB) -> np.ndarray:
    """剪掉尾部已经听不见的部分，并在剪口前做 25 ms 淡出——既不留一长段静音，也不硬切。"""
    y = np.atleast_2d(x)
    env = np.max(np.abs(y), axis=0)
    k = samples(0.005)
    env = np.convolve(env, np.ones(k) / k, mode="same")
    peak = float(np.max(env))
    if peak <= 0:
        return y
    alive = np.nonzero(env > peak * db_to_amp(floor_db))[0]
    end = min(y.shape[1], int(alive[-1]) + samples(0.02)) if alive.size else y.shape[1]
    out = y[:, :end].copy()
    f = min(end, samples(0.025))
    out[:, end - f:] *= raised_cosine(f)[::-1]
    return out


def finalize_sfx(x: np.ndarray, target: float) -> np.ndarray:
    """收尾：去直流、剪尾、0.5 ms 起音淡入、响度归一、真峰值限幅，最后在开头垫 8 ms 静音。

    为什么垫静音：Vorbis 的 MDCT 会把起音的能量往前「抹」一点（预回声）。起音若正好
    落在流的第 0 个样本上，解码出来的第一个样本就不是 0——触发音效的那一瞬会有一个
    小台阶。垫 8 ms 静音后解码结果从 0 起，预回声落在起音前两三毫秒内，被起音本身掩蔽。
    8 ms 的延迟人耳觉察不到（声卡缓冲本身就有二三十毫秒）。
    """
    y = np.atleast_2d(np.asarray(x, dtype=np.float64))
    y = highpass(y, 22.0, order=2)
    y = trim_tail(y)
    y = fade(y, 0.0005, 0.008)
    for _ in range(4):
        y = y * db_to_amp(target - momentary_max_lufs(y))
        if true_peak_db(y) <= SFX_CEILING_DB:
            break
        y = limiter(y, SFX_CEILING_DB - 0.1, attack_ms=1.0, release_ms=60.0, true_peak=True)
    if true_peak_db(y) > SFX_CEILING_DB:
        y = limiter(y, SFX_CEILING_DB - 0.2, attack_ms=1.0, release_ms=60.0, true_peak=True)
    return np.concatenate([np.zeros((y.shape[0], samples(PREROLL_S))), y], axis=1)


# ============================================================== 积木

def env_ar(n: int, attack: float, release_t60: float) -> np.ndarray:
    """快起、指数落：attack 秒内升到 1，然后按 T60 = release_t60 衰减。"""
    t = time_axis(n)
    a = np.clip(t / max(attack, 1e-4), 0.0, 1.0)
    return (0.5 - 0.5 * np.cos(np.pi * a)) * np.exp(-LN1000 * np.maximum(t - attack, 0.0) / release_t60)


def sweep_sine(dur: float, f0: float, f1: float, curve: float = 1.0, phase: float = 0.0) -> np.ndarray:
    """从 f0 滑到 f1 的正弦（curve > 1 先慢后快）。"""
    n = samples(dur)
    x = (np.arange(n) / max(n - 1, 1)) ** curve
    f = f0 * (f1 / f0) ** x
    return np.sin(TWO_PI * np.cumsum(f) / SR + phase)


def noise_sweep(dur: float, rng: np.random.Generator, f0: float, f1: float, q: float = 1.5,
                steps: int = 24) -> np.ndarray:
    """中心频率从 f0 扫到 f1 的带通噪声：「呼——」的风声、刀风、转场的呼啸都用它。

    分段带通再交叉淡化，而不是逐样本时变滤波：几十段就足够连贯，且只用标准的 sosfilt。
    """
    n = samples(dur)
    noise = rng.standard_normal(n + samples(0.05))
    out = np.zeros(n)
    seg = n / steps
    win_len = int(seg * 2)
    window = np.hanning(win_len)
    for k in range(steps + 1):
        c = f0 * (f1 / f0) ** (k / steps)
        lo = max(30.0, c / (1.0 + 0.5 / q))
        hi = min(0.45 * SR, c * (1.0 + 0.5 / q))
        start = int(k * seg - seg)
        a = max(start, 0)
        b = min(start + win_len, n)
        if b <= a:
            continue
        band = bandpass(noise[a:b + samples(0.05)], lo, hi)[:b - a]
        out[a:b] += band * window[a - start:b - start]
    return out / max(float(np.max(np.abs(out))), 1e-9)


def stereo(x: np.ndarray, pan: float = 0.0) -> np.ndarray:
    return pan_mono(x, pan) * math.sqrt(2.0)


def widen(x: np.ndarray, rng: np.random.Generator, amount: float = 0.5) -> np.ndarray:
    """把单声道做宽：左右各过一个 Schroeder 全通（同一延迟、系数反号）。

    全通的幅频响应严格等于 1——每个声道的能量与音色都和原声一样，只是相位被打散，
    听起来是「开」而不是「偏」；两侧延迟相同，也不会因为哈斯效应把声像拉向一边。
    曾经的做法是「原声 + 延迟几毫秒的副本」，那是梳状滤波：对钟、铃这种音高分明的声音，
    某个分音在左边恰好相长、在右边恰好相消，蓄劲一档就因此左右差出将近 10 dB。
    """
    # 系数封顶 0.4：单个全通相对纯延迟的相位偏移不超过 ±2·asin(g) ≈ ±47°，两侧相差不超过约 94°，
    # 左右就不会在某些分音上反相——反相的声音一折成单声道就互相抵消（金系法术曾因此掉了 3.8 dB）。
    g = 0.4 * float(np.clip(amount, 0.0, 1.0))
    d = max(1, samples(rng.uniform(0.0021, 0.0037)))
    out = []
    for sign in (1.0, -1.0):
        b = np.zeros(d + 1)
        a = np.zeros(d + 1)
        b[0], b[d] = -sign * g, 1.0
        a[0], a[d] = 1.0, -sign * g
        out.append(signal.lfilter(b, a, x))
    return np.stack(out)


def place(buf: np.ndarray, sig: np.ndarray, at_s: float, gain: float = 1.0) -> None:
    """把 sig 叠进 buf（单声道或立体声皆可，自动广播）。累加器，原地修改 buf。

    sig 比剩下的空间长时（钟、磬的余音动辄五六秒）不许硬切——硬切就是一声「咔」。
    在截断点前用 80 ms（最多放入长度的三成）的升余弦收掉。
    """
    start = samples(at_s)
    sig = np.atleast_2d(sig) if buf.ndim == 2 else sig
    n = min(sig.shape[-1], buf.shape[-1] - start)
    if n <= 0:
        return
    part = gain * sig[..., :n]
    if n < sig.shape[-1]:
        f = max(1, min(samples(0.08), int(n * 0.3)))
        part = part.copy()
        part[..., n - f:] *= raised_cosine(f)[::-1]
    buf[..., start:start + n] += part


def pluck(voice_name: str, midi: float, dur: float, vel: float, rng: np.random.Generator,
          orn: str = "") -> np.ndarray:
    """单个拨弦音（含琴身共鸣），界面音与短曲都用它，音色与 BGM 里的同一把琴一致。"""
    import plucked
    voice = plucked.VOICES[voice_name]
    note = Note(beat=0.0, beats=1.0, midi=midi, vel=vel, orn=orn)
    x = plucked.render_note(voice, note, dur, rng)
    return apply_sos(x, eq_sos(voice.body)) if voice.body else x


def reverb(x: np.ndarray, room: str, seed_name: str, wet: float, tail_s: float = 1.0) -> np.ndarray:
    """给一段音效加混响（非循环），输出比输入长 tail_s 秒，留给尾巴。"""
    y = np.atleast_2d(x)
    if y.shape[0] == 1:
        y = np.repeat(y, 2, axis=0)
    y = np.concatenate([y, np.zeros((2, samples(tail_s)))], axis=1)
    w = convolve_ir(y, make_ir(ROOMS[room], seed_of("sfx-room", seed_name)), circular=False)
    return y + wet * w[:, :y.shape[1]]


@dataclass(frozen=True)
class Part:
    """短曲里的一个声部（与 mixer.Track 同义，但不循环）。"""
    name: str
    inst: str
    notes: tuple
    level: float = 0.0
    pan: float = 0.0
    send: float = 0.25
    human: tuple = (6.0, 0.05)
    params: dict | None = None


def render_ensemble(parts, bpm: float, length_s: float, room: str, seed_name: str,
                    reverb_gain: float = 1.0, meter: float = 4.0) -> np.ndarray:
    """把几条声部渲染成一段不循环的立体声（胜利曲、章节卡之类）。配平方式与 BGM 相同。"""
    import percussion
    import plucked
    import sustained
    spb = SR * 60.0 / bpm
    n = samples(length_s)
    mix = np.zeros((2, n))
    bus = np.zeros((2, n))
    for part in parts:
        rng = np.random.default_rng(seed_of(seed_name, part.name))
        notes = part.notes
        if part.human != (0.0, 0.0):
            notes = humanize(notes, np.random.default_rng(seed_of(seed_name, part.name, "h")), meter,
                             part.human[0], part.human[1])
        params = part.params or {}
        kind, _, sub = part.inst.partition(":")
        if kind in ("perc", "bell"):
            mono = percussion.render_track(sub, notes, spb, n, rng, params, circular=False)
        elif part.inst in plucked.VOICES:
            mono = plucked.render_track(part.inst, notes, spb, n, rng, params, circular=False)
        else:
            mono = sustained.render_track(part.inst, notes, spb, n, rng, params, circular=False)
        stem = pan_mono(mono, part.pan)
        loud = integrated_lufs(stem)
        if not np.isfinite(loud):
            raise ValueError(f"{seed_name}/{part.name}：声部是静音的")
        stem *= db_to_amp(-24.0 + part.level - loud)
        mix += stem
        bus += stem * part.send
    wet = convolve_ir(bus, make_ir(ROOMS[room], seed_of("sfx-room", seed_name)), circular=False)
    # 短曲有长度上限（胜利曲 ≤ 5 秒），混响尾巴到头时还没散完：末 0.35 秒缓缓收掉，别一刀切。
    return fade(mix + reverb_gain * wet[:, :n], 0.0, 0.35)


def shards(rng: np.random.Generator, dur: float, count: int, lo: float, hi: float,
           decay: float = 0.18) -> np.ndarray:
    """碎裂的「碴碴」声：许多极短的高频小叮当，越往后越稀、越弱，左右随机散开。"""
    n = samples(dur)
    out = np.zeros((2, n))
    for _ in range(count):
        at = float(rng.exponential(decay))
        if at >= dur - 0.08:
            continue
        m = samples(rng.uniform(0.012, 0.07))
        t = time_axis(m)
        f = rng.uniform(lo, hi)
        tink = np.zeros(m)
        for ratio, amp in ((1.0, 1.0), (2.37, 0.5), (3.93, 0.25)):
            if f * ratio < 0.45 * SR:
                tink += amp * np.sin(TWO_PI * f * ratio * t + rng.uniform(0, TWO_PI))
        tink *= np.exp(-LN1000 * t / (m / SR)) * rng.uniform(0.3, 1.0) * math.exp(-at / (2 * decay))
        place(out, pan_mono(tink, rng.uniform(-0.9, 0.9)), at)
    return out
