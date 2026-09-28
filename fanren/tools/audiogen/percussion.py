# -*- coding: utf-8 -*-
"""打击乐：堂鼓、板鼓、拍板、木鱼、小锣、大锣、抄锣、钹、编钟、碰铃、磬、心跳鼓。

全部是「模态合成」：一件打击乐 = 若干个指数衰减的正弦（它的振动模态）+ 一小段敲击噪声。
各乐器的差别在模态的频率比、衰减、以及有没有音高滑动：

  - 京剧的小锣「台」一敲音高往上走，大锣「哐」往下走——这是戏曲锣鼓最抓耳的一点，
    用整体的变速曲线（glide）做出来。
  - 鼓皮受力大时张力高，刚敲下去音高偏高、随后回落：堂鼓的「咚」就靠这一下。
  - 编钟有「一钟双音」，模态里放一个小三度的第二基音；每个模态再配一个相差零点几赫兹的
    孪生模态，产生青铜钟特有的缓慢「嗡——嗡——」拍频。

每次击打都从一个很小的变体库里取（按力度分档 × 4 个变体，确定性生成、缓存复用）：
既省时间，又不会像同一个采样反复触发那样「机关枪」。
"""
from __future__ import annotations

from functools import lru_cache

import numpy as np

from dsp import (LN1000, SR, TWO_PI, apply_sos, bandpass, eq_sos, highpass, lowpass, midi_to_hz,
                 mix_into, raised_cosine, samples, time_axis)
from score import seed_of


def _modal(freqs, amps, t60s, dur: float, rng: np.random.Generator, glide=None,
           beat_hz: float = 0.0, attack_s: float = 0.0) -> np.ndarray:
    """模态合成：Σ a·exp(-t/τ)·sin(2π f ∫glide dt + φ)。glide 为 None 时音高不动。"""
    n = samples(dur)
    t = time_axis(n)
    warp_t = t if glide is None else np.cumsum(glide(t)) / SR
    out = np.zeros(n)
    for f, a, t60 in zip(freqs, amps, t60s):
        if f >= 0.45 * SR or a == 0.0:
            continue
        env = np.exp(-LN1000 * t / t60)
        if attack_s > 0:
            env *= 1.0 - np.exp(-t / attack_s)
        out += a * env * np.sin(TWO_PI * f * warp_t + rng.uniform(0.0, TWO_PI))
        if beat_hz > 0:
            f2 = f + beat_hz * rng.uniform(0.5, 1.5)
            out += 0.6 * a * env * np.sin(TWO_PI * f2 * warp_t + rng.uniform(0.0, TWO_PI))
    return out


def _burst(dur: float, rng: np.random.Generator, lo: float, hi: float, decay: float) -> np.ndarray:
    n = samples(dur)
    x = rng.standard_normal(n) * np.exp(-time_axis(n) / decay)
    if lo > 0 and hi < 0.45 * SR:
        return bandpass(x, lo, hi)
    if lo > 0:
        return highpass(x, lo)
    return lowpass(x, hi)


def _finish(x: np.ndarray, fade_s: float = 0.01) -> np.ndarray:
    """统一收尾：1 ms 升余弦起音（不许从非零处硬切进来），尾部淡出，峰值归一。"""
    x = x.copy()
    a = samples(0.001)
    x[:a] *= raised_cosine(a)
    f = min(x.size, samples(fade_s))
    x[x.size - f:] *= raised_cosine(f)[::-1]
    peak = float(np.max(np.abs(x)))
    return x / peak if peak > 0 else x


# ---------------------------------------------------------------- 各件乐器（力度 0–1，音高系数 1 = 标准）

def tanggu(vel: float, rng: np.random.Generator, pitch: float = 1.0, rim: bool = False) -> np.ndarray:
    """堂鼓：低沉的「咚」。rim=True 是敲鼓边，短而脆。"""
    f0 = 68.0 * pitch * (2.3 if rim else 1.0)
    ratios = (1.0, 1.594, 2.136, 2.296, 2.653, 2.918, 3.156)
    amps = (1.0, 0.55, 0.38, 0.3, 0.2, 0.14, 0.1) if not rim else (0.6, 0.7, 0.6, 0.5, 0.45, 0.4, 0.3)
    base = (0.75 if not rim else 0.18) * (0.7 + 0.5 * vel)
    t60s = tuple(base / (1.0 + 0.45 * i) for i in range(len(ratios)))
    bend = 0.18 + 0.3 * vel

    def glide(t):
        return 1.0 + bend * np.exp(-t / 0.03)

    body = _modal([f0 * r for r in ratios], amps, t60s, base * 1.3 + 0.05, rng, glide)
    stick = _burst(0.02, rng, 0.0, 2500.0 if not rim else 7000.0, 0.004)
    stick *= (0.35 if not rim else 0.9) * (0.5 + vel)
    x = body.copy()
    x[:stick.size] += stick * float(np.max(np.abs(body)))
    return _finish(x, 0.03)


def bangu(vel: float, rng: np.random.Generator, pitch: float = 1.0) -> np.ndarray:
    """板鼓：京剧的指挥，高而干的「嗒」。"""
    f0 = 430.0 * pitch
    body = _modal([f0, f0 * 1.52, f0 * 2.21, f0 * 2.93], [1.0, 0.5, 0.35, 0.2],
                  [0.11, 0.07, 0.05, 0.035], 0.18, rng)
    crack = _burst(0.012, rng, 1500.0, 0.5 * SR, 0.002) * (0.6 + 0.6 * vel)
    x = body.copy()
    x[:crack.size] += crack
    return _finish(x, 0.02)


def paiban(vel: float, rng: np.random.Generator, pitch: float = 1.0) -> np.ndarray:
    """拍板：两片硬木一合，「啪」——两声极近的撞击。"""
    n = samples(0.12)
    x = np.zeros(n)
    for delay, g in ((0.0, 1.0), (0.007 * rng.uniform(0.8, 1.2), 0.7)):
        hit = _modal([1900.0 * pitch, 2750.0 * pitch, 4100.0 * pitch], [1.0, 0.6, 0.4],
                     [0.05, 0.035, 0.025], 0.09, rng)
        hit[:samples(0.006)] += _burst(0.006, rng, 1500.0, 9000.0, 0.0015) * 1.5
        mix_into(x, g * hit, samples(delay), circular=False)
    return _finish(x, 0.02)


def muyu(vel: float, rng: np.random.Generator, pitch: float = 1.0) -> np.ndarray:
    """木鱼：空心木头的「笃」。"""
    f0 = 720.0 * pitch
    x = _modal([f0, f0 * 1.87, f0 * 2.93, f0 * 4.1], [1.0, 0.35, 0.2, 0.08],
               [0.11, 0.06, 0.04, 0.03], 0.16, rng)
    x[:samples(0.003)] += _burst(0.003, rng, 2000.0, 0.5 * SR, 0.0008) * (0.4 + 0.4 * vel)
    return _finish(x, 0.02)


def xiaoluo(vel: float, rng: np.random.Generator, pitch: float = 1.0) -> np.ndarray:
    """小锣：「台」——敲下去音高往上翘两个半音左右。"""
    f0 = 610.0 * pitch
    ratios = (1.0, 1.47, 1.93, 2.36, 2.98, 3.61, 4.22, 4.87, 5.6, 6.3)
    amps = (1.0, 0.7, 0.55, 0.45, 0.35, 0.28, 0.2, 0.15, 0.1, 0.08)
    t60s = tuple(1.4 / (1.0 + 0.18 * i) for i in range(len(ratios)))

    def glide(t):
        return 1.0 + 0.13 * (1.0 - np.exp(-t / 0.12))

    x = _modal([f0 * r for r in ratios], amps, t60s, 1.6, rng, glide, beat_hz=1.3)
    x[:samples(0.01)] += _burst(0.01, rng, 3000.0, 0.5 * SR, 0.002) * 2.0
    return _finish(x, 0.05)


def daluo(vel: float, rng: np.random.Generator, pitch: float = 1.0) -> np.ndarray:
    """大锣：「哐」——低沉、往下沉的音高，带一片噪声的「呼」。"""
    f0 = 175.0 * pitch
    ratios = (1.0, 1.41, 1.87, 2.3, 2.76, 3.3, 3.9, 4.6, 5.3, 6.2, 7.3, 8.4)
    amps = (1.0, 0.8, 0.7, 0.6, 0.5, 0.42, 0.35, 0.28, 0.22, 0.18, 0.14, 0.1)
    t60s = tuple(3.4 / (1.0 + 0.16 * i) for i in range(len(ratios)))

    def glide(t):
        return 1.0 - 0.085 * (1.0 - np.exp(-t / 0.3))

    x = _modal([f0 * r for r in ratios], amps, t60s, 3.6, rng, glide, beat_hz=0.9)
    wash = _burst(1.2, rng, 200.0, 2500.0, 0.3)
    x[:wash.size] += wash * float(np.max(np.abs(x))) / max(float(np.max(np.abs(wash))), 1e-9) * 0.3
    x[:samples(0.015)] += _burst(0.015, rng, 1000.0, 0.5 * SR, 0.003) * 1.5
    return _finish(x, 0.1)


def tamtam(vel: float, rng: np.random.Generator, pitch: float = 1.0) -> np.ndarray:
    """抄锣（风锣）：首领战的大锣。敲下去先是一闷，高频随后才「开花」涌上来。"""
    count = 44
    freqs = np.sort(rng.uniform(55.0, 3200.0, count)) * pitch
    amps = 1.0 / (1.0 + freqs / 400.0)
    t60s = 6.0 / (1.0 + freqs / 1500.0)
    n = samples(6.5)
    t = time_axis(n)
    x = np.zeros(n)
    for f, a, t60 in zip(freqs, amps, t60s):
        bloom = 0.05 + 0.5 * (f / 3200.0)  # 越高的模态「开花」越慢
        env = (1.0 - np.exp(-t / bloom)) * np.exp(-LN1000 * t / t60)
        x += a * env * np.sin(TWO_PI * f * t + rng.uniform(0.0, TWO_PI))
    thump = _modal([48.0 * pitch, 71.0 * pitch], [1.0, 0.6], [0.8, 0.5], 1.0, rng)
    x[:thump.size] += thump * float(np.max(np.abs(x))) * 0.8
    x[:samples(0.02)] += _burst(0.02, rng, 500.0, 6000.0, 0.005) * float(np.max(np.abs(x)))
    return _finish(x, 0.3)


def bo(vel: float, rng: np.random.Generator, pitch: float = 1.0, choke: bool = False) -> np.ndarray:
    """钹（铙钹）：「锵」。choke=True 是捂钹，一声就收。"""
    dur = 0.22 if choke else 1.5
    count = 60
    freqs = np.sort(rng.uniform(400.0, 11000.0, count)) * pitch
    amps = 0.6 + 0.4 * rng.random(count)
    t60s = (0.18 if choke else 1.3) / (1.0 + freqs / 9000.0)
    x = _modal(freqs, amps, t60s, dur, rng)
    noise = _burst(dur, rng, 2500.0, 0.5 * SR, 0.18 if not choke else 0.04)
    x = 0.6 * x / max(float(np.max(np.abs(x))), 1e-9) + noise / max(float(np.max(np.abs(noise))), 1e-9)
    return _finish(x, 0.04 if choke else 0.2)


def bell(midi: float, vel: float, rng: np.random.Generator, kind: str = "bianzhong") -> np.ndarray:
    """钟、铃、磬。kind：bianzhong（编钟）/ pengling（碰铃）/ qing（磬）/ chime（清亮小铃）。"""
    f0 = midi_to_hz(midi)
    if kind == "bianzhong":
        ratios = (1.0, 1.19, 2.01, 2.43, 2.97, 3.98, 5.2, 6.4)
        amps = (1.0, 0.45, 0.5, 0.35, 0.3, 0.2, 0.12, 0.08)
        t60s = (5.0, 3.6, 3.0, 2.2, 1.8, 1.2, 0.8, 0.5)
        dur, beat, strike = 5.5, 0.7, 0.35
    elif kind == "pengling":
        ratios = (1.0, 2.76, 5.4, 8.93)
        amps = (1.0, 0.5, 0.25, 0.12)
        t60s = (2.2, 1.3, 0.7, 0.4)
        dur, beat, strike = 2.4, 2.5, 0.2
    elif kind == "qing":
        ratios = (1.0, 2.71, 5.1, 8.2)
        amps = (1.0, 0.4, 0.18, 0.08)
        t60s = (6.5, 3.5, 2.0, 1.0)
        dur, beat, strike = 6.8, 0.5, 0.12
    elif kind == "chime":
        ratios = (1.0, 2.0, 3.01, 4.2)
        amps = (1.0, 0.3, 0.15, 0.06)
        t60s = (1.6, 0.9, 0.5, 0.3)
        dur, beat, strike = 1.8, 1.8, 0.12
    else:
        raise ValueError(f"未知的钟铃种类：{kind}")
    x = _modal([f0 * r for r in ratios], amps, t60s, dur, rng, beat_hz=beat)
    hit = _burst(0.008, rng, 800.0, 0.5 * SR, 0.002) * strike * (0.5 + vel)
    x[:hit.size] += hit
    return _finish(x, 0.08)


def heartbeat(vel: float, rng: np.random.Generator, pitch: float = 1.0) -> np.ndarray:
    """心跳鼓：闷在皮下的「咚——咚」，只剩 200 Hz 以下。"""
    x = tanggu(vel, rng, 0.62 * pitch)
    return _finish(lowpass(x, 190.0, order=4), 0.05)


_KITS = {
    "tanggu": lambda v, r, p, o: tanggu(v, r, p, rim="!" in o),
    "bangu": lambda v, r, p, o: bangu(v, r, p),
    "paiban": lambda v, r, p, o: paiban(v, r, p),
    "muyu": lambda v, r, p, o: muyu(v, r, p),
    "xiaoluo": lambda v, r, p, o: xiaoluo(v, r, p),
    "daluo": lambda v, r, p, o: daluo(v, r, p),
    "tamtam": lambda v, r, p, o: tamtam(v, r, p),
    "bo": lambda v, r, p, o: bo(v, r, p, choke="!" in o),
    "heartbeat": lambda v, r, p, o: heartbeat(v, r, p),
}
_BELLS = ("bianzhong", "pengling", "qing", "chime")


@lru_cache(maxsize=512)
def sample(kit: str, midi: float, vel_bucket: int, variant: int, orn: str) -> np.ndarray:
    """变体库：同样的 (乐器, 音高, 力度档, 变体号, 记号) 永远得到同一段采样（只读）。"""
    rng = np.random.default_rng(seed_of("perc", kit, f"{midi:.2f}", str(vel_bucket), str(variant), orn))
    vel = (vel_bucket + 0.5) / 8.0
    if kit in _BELLS:
        x = bell(midi, vel, rng, kit)
    else:
        pitch = 2.0 ** ((midi - 60.0) / 12.0)
        x = _KITS[kit](vel, rng, pitch, orn)
    x.flags.writeable = False
    return x


def render_track(kit: str, notes, spb: float, length: int, rng: np.random.Generator,
                 params: dict, circular: bool = True) -> np.ndarray:
    """一条打击乐分轨。音符的 midi 对打击乐是「音高系数」（60 = 标准音高），对钟铃是实际音高。"""
    buf = np.zeros(length)
    variants = int(params.get("variants", 4))
    for note in notes:
        bucket = min(7, max(0, int(note.vel * 8.0)))
        x = sample(kit, round(float(note.midi), 2), bucket, int(rng.integers(0, variants)), note.orn)
        gain = (0.3 + 0.7 * note.vel) ** 1.5
        start = int(round(note.beat * spb + note.shift * SR))
        mix_into(buf, x * gain, start, circular)
    bands = params.get("eq")
    if bands:
        buf = apply_sos(buf, eq_sos(bands), circular=circular)
    return buf
