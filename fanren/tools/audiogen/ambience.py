# -*- coding: utf-8 -*-
"""环境声层：流水、风、滴水、虫鸣、鸟鸣、识海低语。

它们垫在旋律底下，音量很低，作用是「交代场景」：渡口有水声，崖顶有风，暗道里滴水。
全部以曲长 L 为周期生成——噪声用频域合成（dsp.spectral_noise，天然周期），
起伏用周期的慢变随机曲线，零星的事件（水滴、鸟叫）按模 L 摆放——所以循环接缝处
环境声也是连续的，不会每一遍到结尾都「断一下风」。

每个函数返回 (2, L) 立体声。
"""
from __future__ import annotations

import math

import numpy as np

from dsp import LN1000, SR, TWO_PI, mix_into, pan_mono, samples, smooth_random, spectral_noise, time_axis


def _pos(x: np.ndarray) -> np.ndarray:
    """把 0 均值的起伏曲线变成 (0, 1] 的包络。"""
    return 1.0 / (1.0 + np.exp(-x))


def stream(length: int, rng: np.random.Generator) -> np.ndarray:
    """流水：宽带的「哗哗」底 + 无数小气泡的「咕嘟」。"""
    out = np.zeros((2, length))
    for ch in range(2):
        base = spectral_noise(rng, length, lambda f: np.where(f > 80.0, (np.maximum(f, 1.0) / 600.0) ** -0.6,
                                                              0.0) * np.exp(-f / 9000.0))
        swell = _pos(1.2 * smooth_random(rng, length, 0.35))
        out[ch] = base * (0.35 + 0.65 * swell)
    count = int(length / SR * 14)
    for _ in range(count):
        dur = rng.uniform(0.015, 0.05)
        n = samples(dur)
        t = time_axis(n)
        f0 = rng.uniform(500.0, 1800.0)
        # 气泡破裂的音高是往上滑的（Minnaert 共振随气泡变小而升高）。
        f = f0 * (1.0 + 1.6 * t / dur)
        blip = np.sin(TWO_PI * np.cumsum(f) / SR) * np.exp(-LN1000 * t / dur)
        blip *= rng.uniform(0.6, 2.2)
        mix_into(out, pan_mono(blip, rng.uniform(-0.8, 0.8)), int(rng.integers(0, length)), True)
    return out / max(float(np.std(out)), 1e-9)


def wind(length: int, rng: np.random.Generator, center: float = 520.0) -> np.ndarray:
    """风：带通噪声，中心频率与音量都在慢慢飘——「呜——」地一阵一阵。"""
    out = np.zeros((2, length))
    for ch in range(2):
        low = spectral_noise(rng, length, lambda f: np.exp(-0.5 * ((np.log(np.maximum(f, 1.0))
                                                                     - math.log(center)) / 0.55) ** 2))
        high = spectral_noise(rng, length, lambda f: np.exp(-0.5 * ((np.log(np.maximum(f, 1.0))
                                                                      - math.log(center * 2.6)) / 0.4) ** 2))
        gust = _pos(1.6 * smooth_random(rng, length, 0.12))
        whistle = _pos(2.0 * smooth_random(rng, length, 0.2) - 1.0)
        out[ch] = low * (0.25 + 0.75 * gust) + 0.5 * high * gust * whistle
    return out / max(float(np.std(out)), 1e-9)


def drips(length: int, rng: np.random.Generator, per_minute: float = 26.0) -> np.ndarray:
    """暗道滴水：「嘀——」一声往上滑的短音，零零星星，左右不定。"""
    out = np.zeros((2, length))
    count = max(1, int(length / SR / 60.0 * per_minute))
    for _ in range(count):
        dur = rng.uniform(0.05, 0.11)
        n = samples(dur)
        t = time_axis(n)
        f0 = rng.uniform(1100.0, 2600.0)
        f = f0 * (1.0 + 0.9 * (t / dur) ** 0.7)
        drop = np.sin(TWO_PI * np.cumsum(f) / SR) * np.exp(-LN1000 * t / dur)
        drop[:samples(0.002)] *= np.linspace(0.0, 1.0, samples(0.002))
        mix_into(out, pan_mono(drop * rng.uniform(0.4, 1.0), rng.uniform(-0.9, 0.9)),
                 int(rng.integers(0, length)), True)
    return out / max(float(np.max(np.abs(out))), 1e-9)


def crickets(length: int, rng: np.random.Generator) -> np.ndarray:
    """夏夜虫鸣：两三只蟋蟀，各自一串「唧唧唧」，隔一会儿叫一轮。"""
    out = np.zeros((2, length))
    for bug in range(3):
        f0 = rng.uniform(4200.0, 5200.0)
        pan = (-0.7, 0.6, 0.1)[bug]
        pulse_n = samples(0.018)
        t = time_axis(pulse_n)
        pulse = np.sin(TWO_PI * f0 * t) * np.sin(np.pi * t / t[-1]) ** 2
        pos = int(rng.integers(0, samples(1.0)))
        while pos < length:
            chirps = int(rng.integers(3, 6))
            for k in range(chirps):
                mix_into(out, pan_mono(pulse * rng.uniform(0.6, 1.0), pan), pos + k * samples(0.032),
                         True)
            pos += samples(rng.uniform(0.7, 1.6))
    return out / max(float(np.max(np.abs(out))), 1e-9)


def birds(length: int, rng: np.random.Generator, per_minute: float = 5.0) -> np.ndarray:
    """远处的鸟：偶尔两三声上下滑的啁啾。药谷用。"""
    out = np.zeros((2, length))
    count = max(1, int(length / SR / 60.0 * per_minute))
    for _ in range(count):
        start = int(rng.integers(0, length))
        pan = rng.uniform(-0.8, 0.8)
        for k in range(int(rng.integers(2, 4))):
            dur = rng.uniform(0.06, 0.13)
            n = samples(dur)
            t = time_axis(n)
            f0 = rng.uniform(2800.0, 4200.0)
            f = f0 * (1.0 + 0.35 * np.sin(np.pi * t / dur) * (1 if k % 2 == 0 else -1))
            chirp = np.sin(TWO_PI * np.cumsum(f) / SR) * np.sin(np.pi * t / dur) ** 1.5
            mix_into(out, pan_mono(chirp * rng.uniform(0.5, 1.0), pan), start + k * samples(0.16), True)
    return out / max(float(np.max(np.abs(out))), 1e-9)


def whisper(length: int, rng: np.random.Generator) -> np.ndarray:
    """识海里的低语：像人声的带通噪声，几个「共振峰」在慢慢游走，左右各说各的。"""
    out = np.zeros((2, length))
    for ch in range(2):
        voice = np.zeros(length)
        for center in (700.0, 1500.0, 2800.0):
            band = spectral_noise(rng, length, lambda f, c=center: np.exp(-0.5 * ((f - c) / (0.18 * c)) ** 2))
            voice += band * _pos(2.5 * smooth_random(rng, length, 0.6) - 1.5)
        out[ch] = voice
    return out / max(float(np.std(out)), 1e-9)


KINDS = {
    "stream": stream,
    "wind": wind,
    "drips": drips,
    "crickets": crickets,
    "birds": birds,
    "whisper": whisper,
}


def render_track(kind: str, length: int, rng: np.random.Generator, params: dict) -> np.ndarray:
    fn = KINDS[kind]
    kwargs = {k: v for k, v in params.items() if k in ("center", "per_minute")}
    return fn(length, rng, **kwargs)
