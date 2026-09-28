# -*- coding: utf-8 -*-
"""弹拨乐器：Karplus-Strong 拨弦（古筝、琵琶、古琴、大阮、扬琴）。

**做法**：Karplus-Strong 本质是一个 LTI 系统（延迟线 + 损耗滤波 + 分数延迟全通），
所以每个音高只需要算一次「单位冲激响应」（下文叫弦 IR），缓存起来；每个音符再用
各自随机的拨弦激励去卷它。这比逐样本跑环路快两个数量级，而音色完全等价。

  - 分数延迟用一阶全通而不是线性插值：线性插值会给每个音高附加一个不同的低通，
    同一把古筝上相邻两根弦的亮度会忽明忽暗；全通幅频是平的。
  - 环路按「块长 = 整数延迟 M」分块向量化：块内每个样本只依赖 M 个样本之前的值，
    而那些值在上一块里已经算完；全通的逐样本递推交给 lfilter 并在块间传状态。
  - 揉弦、按滑、滑音都靠「变速重读」已渲染好的音（dsp.warp）：弦上一按，所有泛音
    一起升高，恰好就是时间轴的伸缩。
  - 摇指 / 轮指：同一根弦上的多次拨弦 = 同一个弦 IR 卷一串激励（叠加原理）。
"""
from __future__ import annotations

import math
from dataclasses import dataclass
from functools import lru_cache

import numpy as np
from scipy import signal

from dsp import (SR, apply_sos, eq_sos, exp_decay, lowpass, midi_to_hz, mix_into,
                 raised_cosine, samples, time_axis, warp, warp_input_length)
from score import Note


@dataclass(frozen=True)
class PluckVoice:
    name: str
    t60_low: float        # 低音（65 Hz）处基频的衰减时间（秒）
    t60_high: float       # 高音（1 kHz）处
    loss: float           # 环路单零点损耗系数 S：越小越亮、泛音留得越久
    hardness: float       # 激励里噪声（指甲/义甲）所占比例
    position: float       # 拨弦点（占弦长的比例）：越靠近端点越亮
    ring: float           # 记谱时值之后任其自然延音的时间（秒）
    damp: float           # 止音的释放时间（秒）
    click: float          # 起音「咔」的一下（甲片擦弦）的电平
    body: tuple = ()      # 琴身共鸣（EQ 描述，见 dsp.eq_sos）
    courses: int = 1      # 每音几根弦（扬琴 2 根，轻微失谐出「亮晶晶」的拍频）
    detune_cents: float = 0.0
    vib_rate: float = 5.4
    vib_depth: float = 20.0   # 「~」揉弦的深度（音分）
    trem_rate: float = 14.0   # 「*」摇指/轮指的频率（Hz）
    trem_pattern: tuple = (1.0,)
    bend_time: float = 0.14   # 上滑音用时（秒）
    level: float = 1.0


VOICES: dict[str, PluckVoice] = {
    # 古筝：钢丝尼龙弦 + 义甲，亮、延音长，「嘣」的一下之后是长长的余韵。
    "guzheng": PluckVoice("guzheng", 7.5, 2.4, 0.16, 0.45, 0.13, 0.9, 0.25, 0.12,
                          body=(("hp", 55.0), ("peak", 210.0, 3.0, 1.0), ("peak", 1100.0, -2.0, 1.2),
                                ("peak", 3300.0, 3.0, 1.4), ("lp", 11500.0)),
                          vib_depth=22.0, trem_rate=13.5),
    # 琵琶：出音脆、衰减快，中频突出；轮指五指力度不一，是它「颗粒感」的来源。
    "pipa": PluckVoice("pipa", 3.6, 1.2, 0.2, 0.6, 0.1, 0.12, 0.07, 0.22,
                       body=(("hp", 85.0), ("peak", 380.0, 3.0, 1.2), ("peak", 2100.0, 3.5, 1.3),
                             ("lp", 10500.0)),
                       vib_depth=26.0, trem_rate=17.0, trem_pattern=(0.82, 0.95, 1.0, 0.9, 0.78)),
    # 古琴：丝弦、指肉拨弦，低沉温润，泛音是它的灵魂；走手音（按住滑动）很慢。
    "guqin": PluckVoice("guqin", 9.0, 3.2, 0.42, 0.12, 0.17, 1.6, 0.45, 0.03,
                        body=(("hp", 40.0), ("peak", 115.0, 4.0, 1.0), ("peak", 260.0, 2.0, 1.1),
                              ("peak", 1800.0, -3.0, 1.0), ("lp", 5200.0)),
                        vib_depth=18.0, bend_time=0.32),
    # 大阮：圆润的弹拨低音，托住和声。
    "daruan": PluckVoice("daruan", 4.5, 1.8, 0.36, 0.25, 0.2, 0.25, 0.12, 0.05,
                         body=(("hp", 38.0), ("peak", 140.0, 3.0, 1.0), ("peak", 700.0, 1.5, 1.0),
                               ("lp", 4200.0))),
    # 扬琴：竹槌击弦，每音两根弦，略失谐，金属感的「叮」。
    "yangqin": PluckVoice("yangqin", 4.8, 1.7, 0.2, 0.7, 0.12, 0.5, 0.3, 0.1,
                          body=(("hp", 75.0), ("peak", 2600.0, 2.0, 1.0), ("lp", 12000.0)),
                          courses=2, detune_cents=3.5, trem_rate=12.0),
}


def _t60_for(voice: PluckVoice, f0: float) -> float:
    """按音高在 t60_low 与 t60_high 之间对数插值：高音弦天生衰减快。"""
    x = (math.log2(max(f0, 20.0)) - math.log2(65.0)) / (math.log2(1000.0) - math.log2(65.0))
    x = min(max(x, -0.5), 1.5)
    return voice.t60_low * (voice.t60_high / voice.t60_low) ** x


@lru_cache(maxsize=256)
def ks_ir(f0: float, t60: float, loss: float, n: int) -> np.ndarray:
    """Karplus-Strong 弦的单位冲激响应（只读数组）。见文件头的做法说明。"""
    period = SR / f0
    m = int(math.floor(period - loss - 0.5))
    if m < 2:
        raise ValueError(f"音太高（{f0:.1f} Hz），环路延迟不足两个样本")
    d = period - loss - m
    c = (1.0 - d) / (1.0 + d)
    g = 10.0 ** (-3.0 / (f0 * t60))
    pad = m + 2
    y = np.zeros(pad + n)
    y[pad] = 1.0
    b_ap = np.array([c, 1.0])
    a_ap = np.array([1.0, c])
    zi = np.zeros(1)
    p = pad
    end = pad + n
    one_minus = 1.0 - loss
    while p < end:
        q = min(p + m, end)
        u = y[p - m:q - m]
        u1 = y[p - m - 1:q - m - 1]
        w, zi = signal.lfilter(b_ap, a_ap, one_minus * u + loss * u1, zi=zi)
        y[p:q] += g * w
        p = q
    out = y[pad:]
    out.flags.writeable = False
    return out


def _excitation(voice: PluckVoice, f0: float, vel: float, rng: np.random.Generator,
                harmonic: bool) -> np.ndarray:
    """一个周期长的拨弦激励：三角形初始位移（弦）+ 按力度变亮的噪声（甲片）。"""
    n = max(8, int(round(SR / f0)))
    if harmonic:
        # 泛音：轻点节点，几乎只剩纯音——一个周期的正弦就是最干净的激励。
        k = np.arange(n) / n
        return np.sin(2.0 * math.pi * k) + 0.03 * rng.standard_normal(n)
    beta = min(0.45, max(0.04, voice.position * rng.uniform(0.85, 1.15)))
    k = np.arange(n) / n
    tri = np.where(k < beta, k / beta, (1.0 - k) / (1.0 - beta))
    tri -= tri.mean()
    tri /= max(float(np.std(tri)), 1e-9)
    noise = rng.standard_normal(n)
    cutoff = min(0.45 * SR, f0 * (4.0 + 26.0 * vel) * (1.0 + voice.hardness))
    noise = lowpass(noise, cutoff)
    shift = max(1, int(round(beta * n)))
    noise = noise - 0.9 * np.roll(noise, shift)  # 与三角形同一个拨弦点：同一处的谐波被挖掉
    noise -= noise.mean()
    noise /= max(float(np.std(noise)), 1e-9)
    h = voice.hardness * (0.6 + 0.6 * vel)
    return (1.0 - h) * tri + h * noise


def _smoothstep(x: np.ndarray) -> np.ndarray:
    x = np.clip(x, 0.0, 1.0)
    return x * x * (3.0 - 2.0 * x)


def _pitch_ratio(note: Note, voice: PluckVoice, n: int, dur_s: float,
                 rng: np.random.Generator) -> np.ndarray | None:
    """装饰音对应的逐样本播放速率曲线；没有任何变调装饰就返回 None（省一次重读）。"""
    orn = note.orn
    wants = note.slide_from is not None or any(c in orn for c in "~v^")
    if not wants:
        return None
    t = time_axis(n)
    cents = np.zeros(n)
    if note.slide_from is not None:
        span = voice.bend_time * (2.2 if "s" in orn else 1.0)
        cents += (note.slide_from - note.midi) * 100.0 * (1.0 - _smoothstep(t / span))
    if "^" in orn and note.up is not None:
        # 打音：按弦瞬间顶到上方邻音再放回。
        bump = _smoothstep((t - 0.04) / 0.035) * (1.0 - _smoothstep((t - 0.1) / 0.05))
        cents += (note.up - note.midi) * 100.0 * bump
    if "v" in orn and note.down is not None:
        t0 = max(0.08, dur_s * 0.55)
        cents += (note.down - note.midi) * 100.0 * _smoothstep((t - t0) / max(0.12, dur_s * 0.4))
    if "~" in orn:
        onset = _smoothstep((t - 0.12) / 0.3)
        rate = voice.vib_rate * rng.uniform(0.92, 1.08)
        # 古筝/古琴的揉弦是「按下去再松开」：音高只往上走，所以用 (1 - cos)/2 而不是正弦。
        cents += voice.vib_depth * onset * 0.5 * (1.0 - np.cos(2.0 * math.pi * rate * t))
    return 2.0 ** (cents / 1200.0)


def render_note(voice: PluckVoice, note: Note, dur_s: float, rng: np.random.Generator) -> np.ndarray:
    """渲染一个拨弦音（单声道，从起音开始）。"""
    f0 = midi_to_hz(note.midi)
    harmonic = "o" in note.orn
    t60 = _t60_for(voice, f0) * (1.3 if harmonic else 1.0)
    loss = 0.47 if harmonic else voice.loss
    stacc = "!" in note.orn
    ring_s = dur_s * 0.45 + 0.02 if stacc else dur_s + voice.ring
    damp = 0.04 if stacc else voice.damp
    total_s = min(ring_s + damp * 3.0, t60 * 1.05)
    n_out = max(samples(0.03), samples(total_s))
    ratio = _pitch_ratio(note, voice, n_out, dur_s, rng)
    n_in = n_out if ratio is None else warp_input_length(ratio)
    n_ir = samples(t60 * 1.05)
    ir_full = ks_ir(round(f0, 4), round(t60, 3), loss, n_ir)

    vel = note.vel
    train = np.zeros(n_in)
    if "*" in note.orn:
        # 摇指/轮指：同一根弦上一串拨弦，力度按指序起伏，时间带一点抖动。
        count = max(2, int(dur_s * voice.trem_rate))
        for k in range(count):
            pos = samples(k / voice.trem_rate * rng.uniform(0.94, 1.06))
            if pos >= n_in:
                break
            amp = voice.trem_pattern[k % len(voice.trem_pattern)] * rng.uniform(0.9, 1.08)
            exc = _excitation(voice, f0, vel * 0.85, rng, harmonic) * amp
            mix_into(train, exc[:n_in - pos], pos, circular=False)
    else:
        exc = _excitation(voice, f0, vel, rng, harmonic)
        train[:min(exc.size, n_in)] = exc[:n_in]

    ir = ir_full[:n_in] if n_in <= ir_full.size else np.concatenate([ir_full, np.zeros(n_in - ir_full.size)])
    x = signal.oaconvolve(train, ir)[:n_in]
    if voice.courses > 1 and not harmonic:
        f2 = f0 * 2.0 ** (voice.detune_cents / 1200.0)
        ir2 = ks_ir(round(f2, 4), round(t60 * 0.95, 3), loss, n_ir)
        ir2 = ir2[:n_in] if n_in <= ir2.size else np.concatenate([ir2, np.zeros(n_in - ir2.size)])
        x = x + 0.85 * signal.oaconvolve(train, ir2)[:n_in]

    head = x[:samples(0.08)]
    peak = float(np.max(np.abs(head))) if head.size else 1.0
    x = x / max(peak, 1e-9)
    if ratio is not None:
        x = warp(x, ratio)
    x = x[:n_out]

    if voice.click > 0 and not harmonic:
        m = samples(0.004)
        click = rng.standard_normal(m) * np.exp(-np.arange(m) / (m / 4.0))
        click = apply_sos(click, eq_sos((("hp", 2500.0),)))
        x[:m] += voice.click * vel * click

    env = np.ones(n_out)
    r0 = min(n_out, samples(ring_s))
    if r0 < n_out:
        env[r0:] = exp_decay(n_out - r0, damp)
    env[:samples(0.0015)] *= raised_cosine(samples(0.0015))
    tail = min(n_out, samples(0.006))
    env[n_out - tail:] *= raised_cosine(tail)[::-1]
    gain = voice.level * (vel ** 1.4) * (0.55 if harmonic else 1.0)
    return x * env * gain


def render_track(voice_name: str, notes, spb: float, length: int, rng: np.random.Generator,
                 params: dict, circular: bool = True) -> np.ndarray:
    """把一整条弹拨声部渲染成单声道分轨（长度 length，circular 时尾巴绕回开头）。

    params:
      reverse  True：每个音倒放，且让倒放的末端落在记谱的起拍上（识海曲的「吸气」声）
      legato   时值系数（缺省 1.0）
    """
    voice = VOICES[voice_name]
    buf = np.zeros(length)
    reverse = bool(params.get("reverse", False))
    legato = float(params.get("legato", 1.0))
    for note in notes:
        dur_s = note.beats * spb / SR * legato
        start = int(round(note.beat * spb + note.shift * SR))
        if note.grace is not None:
            g = Note(beat=note.beat, beats=0.1, midi=note.grace, vel=note.vel * 0.8)
            gx = render_note(voice, g, 0.07, rng)
            gx = gx[:samples(0.25)] * np.linspace(1.0, 0.0, min(gx.size, samples(0.25)))
            mix_into(buf, gx, start - samples(0.065), circular)
        x = render_note(voice, note, dur_s, rng)
        if reverse:
            x = x[::-1]
            mix_into(buf, x, start - x.size, circular)
        else:
            mix_into(buf, x, start, circular)
    if voice.body:
        buf = apply_sos(buf, eq_sos(voice.body), circular=circular)
    return buf
