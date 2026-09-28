# -*- coding: utf-8 -*-
"""持续音乐器：笛子、箫（吹）、二胡、中胡、大胡（拉）、笙（簧）。

与弹拨乐器不同，吹拉乐器的一句旋律是**一口气、一弓下来**的连续发声：音与音之间是
滑过去、吐一下音、换一下弓，而不是各自独立地响。所以这里以「乐句」为单位渲染：
先按音符表画出逐点的音高曲线与音量曲线（控制率约 1.4 kHz），再一次性合成整句。

  - 音高曲线：音符阶梯 + 连接处的滑音（默认很短；「s」是二胡式的长滑）+ 装饰
    （倚音、上滑、打音、下滑）+ 延迟起振的颤音 + 几音分的随机漂移（人不是机器）。
  - 音量曲线：起音、吐音/换弓处的小凹陷、长音的渐强渐弱、句尾释放。
  - 笛子 / 箫：谐波加法合成 + 气声噪音；笛子另加「笛膜」的嗡鸣（软削波后取高频）。
  - 二胡 / 大胡：polyBLEP 锯齿波（弓弦的亥姆霍兹运动本就是锯齿）+ 琴筒共鸣峰 + 弓噪。
  - 笙：簧片音色的锯齿低通 + 共振峰，和弦里每个音单独成声、微微失谐。
"""
from __future__ import annotations

from dataclasses import dataclass, replace

import numpy as np

from dsp import (SR, TWO_PI, apply_sos, bandpass, eq_sos, mix_into, raised_cosine, samples,
                 soft_clip)
from score import Note

_CR = 32  # 控制曲线的步长（样本）：约 1.4 kHz，足够画颤音与 15 ms 的滑音


@dataclass(frozen=True)
class WindVoice:
    name: str
    source: str               # "additive"（吹）/ "saw"（拉、簧）
    harmonics: tuple = ()     # 加法合成的各次谐波幅度
    tilt: float = 0.5         # 力度对高次谐波的提亮程度
    breath: float = 0.0       # 持续气声
    breath_band: tuple = (1500.0, 7000.0)
    attack: float = 0.05
    release: float = 0.12
    glide: float = 0.02       # 音与音之间默认的滑音时间
    slide: float = 0.14       # 「s」长滑音
    vib_rate: float = 5.5
    vib_auto: float = 12.0    # 长音自动颤音深度（音分）
    vib_strong: float = 30.0  # 「~」颤音深度
    vib_delay: float = 0.18
    dip: float = 0.3          # 吐音/换弓处的音量凹陷
    buzz: float = 0.0         # 笛膜嗡鸣
    bow_noise: float = 0.0
    body: tuple = ()
    poly: bool = False        # 和弦乐器：每个音单独成声
    detune: float = 0.0       # 和弦内部的随机失谐（音分）
    tremolo: float = 0.0      # 音量上的慢颤动（笙的气息）
    drift: float = 3.0        # 随机音高漂移（音分）
    level: float = 1.0


VOICES: dict[str, WindVoice] = {
    # 笛子：亮、穿透，笛膜给出特有的「沙沙」嗡鸣；颤音靠气息，快而浅。
    "dizi": WindVoice("dizi", "additive", (1.0, 0.5, 0.38, 0.2, 0.14, 0.09, 0.06, 0.04, 0.03, 0.02),
                      tilt=0.7,
                      breath=0.035, breath_band=(1800.0, 8000.0), attack=0.035, release=0.09,
                      glide=0.018, slide=0.12, vib_rate=5.6, vib_auto=16.0, vib_strong=34.0,
                      vib_delay=0.2, dip=0.35, buzz=0.2,
                      body=(("hp", 250.0), ("peak", 3600.0, 2.5, 1.5), ("lp", 12000.0))),
    # 箫：竖吹，暗、气多，像夜里的叹息。
    "xiao": WindVoice("xiao", "additive", (1.0, 0.2, 0.12, 0.045, 0.02), tilt=0.4, breath=0.08,
                      breath_band=(900.0, 5000.0), attack=0.07, release=0.16, glide=0.03,
                      slide=0.16, vib_rate=5.0, vib_auto=12.0, vib_strong=24.0, vib_delay=0.25,
                      dip=0.25, body=(("hp", 160.0), ("lp", 6000.0))),
    # 二胡：揉弦深、滑音多，琴筒蟒皮给出鼻音般的共鸣峰。
    "erhu": WindVoice("erhu", "saw", tilt=0.6, attack=0.06, release=0.12, glide=0.045, slide=0.15,
                      vib_rate=6.2, vib_auto=22.0, vib_strong=42.0, vib_delay=0.14, dip=0.22,
                      bow_noise=0.03,
                      body=(("hp", 190.0), ("peak", 820.0, 5.0, 1.3), ("peak", 1650.0, -3.0, 2.0),
                            ("peak", 2900.0, 3.0, 2.0), ("lp", 6000.0, 4))),
    # 中胡：二胡的中音兄弟（空弦低五度，最低 G3），音色更厚、鼻音更少——低于 D4 的副旋律交给它。
    "zhonghu": WindVoice("zhonghu", "saw", tilt=0.5, attack=0.07, release=0.14, glide=0.05, slide=0.16,
                         vib_rate=5.8, vib_auto=18.0, vib_strong=34.0, vib_delay=0.16, dip=0.2,
                         bow_noise=0.025,
                         body=(("hp", 120.0), ("peak", 560.0, 4.5, 1.2), ("peak", 1250.0, -2.0, 2.0),
                               ("peak", 2200.0, 2.5, 2.0), ("lp", 5000.0, 4))),
    # 大胡（低音胡琴）：垫低音长音，暗、厚。
    "dahu": WindVoice("dahu", "saw", tilt=0.4, attack=0.14, release=0.25, glide=0.06, slide=0.2,
                      vib_rate=5.2, vib_auto=10.0, vib_strong=20.0, vib_delay=0.3, dip=0.15,
                      bow_noise=0.015,
                      body=(("hp", 45.0), ("peak", 260.0, 4.0, 1.0), ("peak", 700.0, 2.0, 1.4),
                            ("lp", 2600.0))),
    # 笙：簧片、和弦，庙堂与仪式感的来源。
    "sheng": WindVoice("sheng", "saw", tilt=0.3, attack=0.16, release=0.3, glide=0.04,
                       vib_auto=0.0, vib_strong=6.0, dip=0.1, poly=True, detune=4.0, tremolo=0.035,
                       drift=1.5,
                       body=(("hp", 150.0), ("peak", 1250.0, 5.0, 1.2), ("peak", 2600.0, 2.0, 1.5),
                             ("lp", 3200.0, 4))),
}


def _smoothstep(x: np.ndarray) -> np.ndarray:
    x = np.clip(x, 0.0, 1.0)
    return x * x * (3.0 - 2.0 * x)


def _polyblep_saw(phase_cycles: np.ndarray, dt: np.ndarray) -> np.ndarray:
    """带限锯齿波：朴素锯齿在跳变处加 polyBLEP 修正，高音也不刺耳地混叠。"""
    t = phase_cycles - np.floor(phase_cycles)
    saw = 2.0 * t - 1.0
    dt = np.clip(dt, 1e-6, 0.5)
    m1 = t < dt
    x = t[m1] / dt[m1]
    saw[m1] -= x + x - x * x - 1.0
    m2 = t > 1.0 - dt
    x = (t[m2] - 1.0) / dt[m2]
    saw[m2] -= x * x + x + x + 1.0
    return saw


def _phrases(notes, spb: float, voice: WindVoice) -> list[list[tuple[Note, int, int]]]:
    """按换气、休止与时间空隙把音符切成乐句。元素是 (音符, 起点样本, 终点样本)。"""
    ordered = sorted(notes, key=lambda x: (x.beat, x.shift))
    timed = []
    for i, n in enumerate(ordered):
        start = int(round(n.beat * spb + n.shift * SR))
        length = n.beats * spb * (0.5 if "!" in n.orn else 1.0)
        if i + 1 < len(ordered) and ordered[i + 1].breath and not voice.poly:
            # 换气要真的有一口气的空：句末音让出 0.12 秒（短音最多让出四分之一）。
            length -= min(samples(0.12), 0.25 * length)
        timed.append((n, start, start + max(samples(0.04), int(round(length)))))
    if voice.poly:
        return [[item] for item in timed]
    out: list[list[tuple[Note, int, int]]] = []
    cur: list[tuple[Note, int, int]] = []
    for item in timed:
        n, s, _ = item
        gap = s - cur[-1][2] if cur else 0
        if cur and (n.breath or gap > samples(0.08) or "!" in cur[-1][0].orn):
            out.append(cur)
            cur = []
        cur.append(item)
    if cur:
        out.append(cur)
    return out


def _curves(phrase, voice: WindVoice, rng: np.random.Generator, origin: int, n_ctrl: int):
    """画一句的音高（MIDI）与音量控制曲线，控制率 SR/_CR。"""
    tc = (np.arange(n_ctrl) * _CR + origin).astype(np.float64)
    pitch = np.full(n_ctrl, phrase[0][0].midi, dtype=np.float64)
    amp = np.zeros(n_ctrl)
    vib_depth = np.zeros(n_ctrl)

    def span(a: float, b: float) -> slice:
        i = int(np.searchsorted(tc, a))
        j = int(np.searchsorted(tc, b))
        return slice(max(i, 0), min(j, n_ctrl))

    def ramp(a: float, b: float) -> tuple[slice, np.ndarray]:
        s = span(a, b)
        return s, _smoothstep((tc[s] - a) / max(b - a, 1.0))

    for k, (note, s0, e0) in enumerate(phrase):
        nxt = phrase[k + 1][1] if k + 1 < len(phrase) else e0
        seg = span(s0, max(nxt, e0))
        pitch[seg] = note.midi
        level = voice.level * (0.25 + 0.75 * note.vel) ** 1.3
        # 句内音与音首尾相接（人性化可能留下几毫秒的缝），音量铺到下一个音的起点，不断气。
        amp[span(s0, e0 if "!" in note.orn else max(e0, nxt))] = level
        if k > 0:
            prev = phrase[k - 1][0]
            g = samples(voice.slide if "s" in note.orn else voice.glide)
            gs, w = ramp(s0 - g * 0.5, s0 + g * 0.5) if "s" not in note.orn else ramp(s0, s0 + g)
            pitch[gs] = prev.midi + (note.midi - prev.midi) * w
            # 吐音 / 换弓：新音开头音量浅浅一凹；同音反复时凹得更深，否则听不出是两个音。
            depth = voice.dip * (2.0 if abs(prev.midi - note.midi) < 0.5 else 1.0)
            if "s" not in note.orn:
                ds = span(s0 - samples(0.012), s0 + samples(0.03))
                x = (tc[ds] - (s0 - samples(0.012))) / samples(0.042)
                amp[ds] *= 1.0 - min(depth, 0.85) * np.sin(np.pi * np.clip(x, 0, 1))
        if note.slide_from is not None and "s" not in note.orn:
            us, w = ramp(s0, s0 + samples(voice.slide))
            pitch[us] = note.slide_from + (note.midi - note.slide_from) * w
        if note.grace is not None:
            gsl = span(s0 - samples(0.07), s0)
            pitch[gsl] = note.grace
        if "^" in note.orn and note.up is not None:
            ms = span(s0 + samples(0.03), s0 + samples(0.075))
            pitch[ms] = note.up
        dur_s = (e0 - s0) / SR
        if "v" in note.orn and note.down is not None:
            fs, w = ramp(s0 + (e0 - s0) * 0.6, e0)
            pitch[fs] = note.midi + (note.down - note.midi) * w
            amp[fs] *= 1.0 - 0.5 * w
        depth = voice.vib_strong if "~" in note.orn else (voice.vib_auto if dur_s > 0.42 else 0.0)
        if depth > 0:
            vs, w = ramp(s0 + samples(voice.vib_delay), s0 + samples(voice.vib_delay + 0.25))
            vib_depth[vs] = np.maximum(vib_depth[vs], depth * w)
            vt = span(s0 + samples(voice.vib_delay + 0.25), e0)
            vib_depth[vt] = np.maximum(vib_depth[vt], depth)
        if dur_s > 0.9:
            # 长音的渐强渐弱：吹拉乐器的长音从不是一条直线。
            ls = span(s0, e0)
            x = (tc[ls] - s0) / max(e0 - s0, 1)
            amp[ls] *= 0.9 + 0.2 * np.sin(np.pi * x) ** 0.8

    first, last = phrase[0][1], phrase[-1][2]
    if phrase[0][0].grace is not None:
        # 句首带倚音：起音要提前、倚音那一段也要有音量，否则倚音落在静音里，
        # 主音起点还会从 0 一步跳到满音量（咔哒一声）。
        first -= samples(0.07)
        amp[span(first, phrase[0][1])] = amp[min(n_ctrl - 1, int(np.searchsorted(tc, phrase[0][1])))]
    a_s, w = ramp(first - samples(0.005), first + samples(voice.attack))
    amp[a_s] *= w
    r_s, w = ramp(last, last + samples(voice.release))
    amp[r_s] = amp[max(r_s.start - 1, 0)] * (1.0 - w) if r_s.stop > r_s.start else amp[r_s]
    amp[span(last + samples(voice.release), tc[-1] + 1)] = 0.0
    k = max(1, samples(0.008) // _CR)
    amp = np.convolve(amp, np.ones(2 * k + 1) / (2 * k + 1), mode="same")

    rate = voice.vib_rate * rng.uniform(0.94, 1.06)
    t_ctrl = (tc - origin) / SR
    vib = vib_depth * np.sin(TWO_PI * rate * t_ctrl + rng.uniform(0, TWO_PI))
    win = np.hanning(max(3, samples(0.5) // _CR))
    drift = np.convolve(rng.standard_normal(n_ctrl), win / win.sum(), mode="same")
    drift *= voice.drift / max(float(np.std(drift)), 1e-9)
    return pitch + (vib + drift) / 100.0, amp


def render_phrase(voice: WindVoice, phrase, rng: np.random.Generator) -> tuple[np.ndarray, int]:
    """渲染一句，返回 (单声道信号, 起点样本)。"""
    origin = phrase[0][1] - samples(0.09)
    end = phrase[-1][2] + samples(voice.release + 0.06)
    n = end - origin
    n_ctrl = n // _CR + 2
    pitch_c, amp_c = _curves(phrase, voice, rng, origin, n_ctrl)
    if voice.poly and voice.detune > 0:
        pitch_c = pitch_c + rng.uniform(-voice.detune, voice.detune) / 100.0
    xc = np.arange(n_ctrl) * _CR
    xs = np.arange(n)
    pitch = np.interp(xs, xc, pitch_c)
    amp = np.interp(xs, xc, amp_c)
    freq = 440.0 * 2.0 ** ((pitch - 69.0) / 12.0)
    cycles = np.cumsum(freq) / SR + rng.uniform(0.0, 1.0)
    phase = TWO_PI * cycles
    bright = np.clip(amp / max(voice.level, 1e-9), 0.0, 1.0)

    if voice.source == "additive":
        sig = np.zeros(n)
        for k, a in enumerate(voice.harmonics, start=1):
            limit = freq * k < 16000.0
            gain = a * (0.35 + 0.65 * bright) ** (voice.tilt * (k - 1))
            sig += np.where(limit, gain * np.sin(k * phase + 0.3 * k), 0.0)
        if voice.buzz > 0:
            # 笛膜：把主音适度削波，得到的奇次谐波只留 2.5–7 kHz 那一段——那就是「嗡」。
            fuzz = soft_clip(sig * 1.6, 3.0) - sig * 0.5
            sig = sig + voice.buzz * bandpass(fuzz, 2500.0, 7000.0)
    else:
        saw = _polyblep_saw(cycles, freq / SR)
        mix = 0.55 + 0.45 * bright * voice.tilt / 0.6
        sig = mix * saw + (1.0 - mix) * np.sin(phase)
    sig *= amp
    if voice.tremolo > 0:
        sig *= 1.0 + voice.tremolo * np.sin(TWO_PI * 4.3 * xs / SR + rng.uniform(0, TWO_PI))

    if voice.breath > 0:
        lo, hi = voice.breath_band
        noise = bandpass(rng.standard_normal(n), lo, hi)
        noise /= max(float(np.std(noise)), 1e-9)
        onset = np.zeros(n)
        for _, s0, _ in phrase:
            a = s0 - origin
            m = min(n - a, samples(0.08))
            if m > 0:
                onset[a:a + m] += np.exp(-np.arange(m) / samples(0.02))
        sig += noise * (voice.breath * amp ** 0.8 + 0.35 * voice.breath * onset * amp.max())
    if voice.bow_noise > 0:
        noise = bandpass(rng.standard_normal(n), 2500.0, 9000.0)
        noise /= max(float(np.std(noise)), 1e-9)
        sig += noise * voice.bow_noise * amp
    edge = min(n // 2, samples(0.004))
    sig[:edge] *= raised_cosine(edge)
    sig[n - edge:] *= raised_cosine(edge)[::-1]
    return sig, origin


def render_track(voice_name: str, notes, spb: float, length: int, rng: np.random.Generator,
                 params: dict, circular: bool = True) -> np.ndarray:
    """把一整条吹拉声部渲染成单声道分轨。params 目前只认 transpose_cents（整轨微调）。"""
    voice = VOICES[voice_name]
    buf = np.zeros(length)
    cents = float(params.get("transpose_cents", 0.0))
    for phrase in _phrases(notes, spb, voice):
        if cents:
            phrase = [(replace(n, midi=n.midi + cents / 100.0), s, e) for n, s, e in phrase]
        sig, origin = render_phrase(voice, phrase, rng)
        mix_into(buf, sig, origin, circular)
    if voice.body:
        buf = apply_sos(buf, eq_sos(voice.body), circular=circular)
    return buf
