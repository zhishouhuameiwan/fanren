# -*- coding: utf-8 -*-
"""混音与母带：把一首曲子的各声部渲染成分轨、配平、加混响/回声、压到目标响度。

**配平靠响度而不是靠耳朵**：每条声部写一个「相对响度」level（LU），渲染出分轨后按
BS.1770 量它自己的积分响度，再补增益到 REF + level。这样主旋律 0、伴奏 -8、环境 -20
这种相对关系是被**量出来并落实**的，而不是凭感觉填一个 dB 增益、再祈祷各乐器的
合成电平恰好差不多——后者换一个音区、多几个音，平衡就全变了。

**循环**：所有声部都按 circular=True 渲染（尾巴绕回开头），混响/回声做循环卷积，
限幅器按周期信号算释放。所以整首曲子首尾相接处与曲中任何一处没有区别。
"""
from __future__ import annotations

from dataclasses import dataclass, field

import numpy as np

import ambience
import percussion
import plucked
import sustained
from dsp import (ROOMS, SR, convolve_ir, db_to_amp, echo_ir, eq, highpass, integrated_lufs,
                 limiter, make_ir, pan_mono, sample_peak_db, soft_clip, true_peak_db)
from score import humanize, seed_of

REF_LUFS = -24.0          # level = 0 的声部被配到这个响度（之后整首再统一抬到目标）
BGM_TARGET_LUFS = -16.0
BGM_CEILING_DB = -1.5     # 真峰值上限（dBTP）；留 0.5 dB 给 Vorbis 编码过冲，解码后仍 ≤ -1


@dataclass(frozen=True)
class Track:
    name: str
    inst: str                       # 乐器：plucked/sustained 的音色名，或 perc:<鼓>、bell:<钟>、amb:<环境>
    notes: tuple = ()
    level: float = 0.0              # 相对响度（LU）
    pan: float = 0.0
    send: float = 0.25              # 混响发送
    echo: float = 0.0               # 回声发送（曲子要有 echo 设置才生效）
    human: tuple = (8.0, 0.06)      # (起拍抖动 ms, 力度抖动)
    fx: tuple = ()                  # 声部 EQ
    drive: float = 0.0              # 软削波失真
    params: dict = field(default_factory=dict)


@dataclass(frozen=True)
class Song:
    id: str
    title: str                      # 用途（哪些地图/场合）
    mood: str                       # 情绪
    mode: str                       # 调式
    bpm: float
    meter: float                    # 每小节拍数
    bars: int
    room: str
    tracks: tuple
    instruments: str
    form: str                       # 曲式
    echo: tuple | None = None       # (延迟拍数, 反馈, 跳数, 阻尼 Hz)
    reverb_gain: float = 1.0

    @property
    def spb(self) -> float:
        """每拍样本数（浮点；音符起点四舍五入到样本）。"""
        return SR * 60.0 / self.bpm

    @property
    def length(self) -> int:
        return int(round(self.bars * self.meter * self.spb))


def render_stem(song: Song, track: Track) -> np.ndarray:
    """渲染一条声部，返回 (2, L)，尚未配平。"""
    length = song.length
    rng = np.random.default_rng(seed_of(song.id, track.name))
    notes = track.notes
    if notes and track.human != (0.0, 0.0):
        hrng = np.random.default_rng(seed_of(song.id, track.name, "human"))
        notes = humanize(notes, hrng, song.meter, track.human[0], track.human[1])
    kind, _, sub = track.inst.partition(":")
    if kind == "amb":
        return ambience.render_track(sub, length, rng, track.params)
    if kind in ("perc", "bell"):
        mono = percussion.render_track(sub, notes, song.spb, length, rng, track.params)
    elif track.inst in plucked.VOICES:
        mono = plucked.render_track(track.inst, notes, song.spb, length, rng, track.params)
    elif track.inst in sustained.VOICES:
        mono = sustained.render_track(track.inst, notes, song.spb, length, rng, track.params)
    else:
        raise ValueError(f"{song.id}/{track.name}：未知乐器 {track.inst!r}")
    if track.fx:
        mono = eq(mono, track.fx, circular=True)
    if track.drive > 0:
        peak = float(np.max(np.abs(mono)))
        if peak > 0:
            mono = soft_clip(mono / peak, track.drive) * peak
    return pan_mono(mono, track.pan)


def finalize(x: np.ndarray, target_lufs: float, ceiling_db: float, circular: bool) -> tuple[np.ndarray, dict]:
    """响度归一 + 真峰值限幅，交替几轮直到两者都满足（限幅会让响度掉一点，再抬回来）。"""
    y = x
    gr_max = 0.0
    for _ in range(5):
        lufs = integrated_lufs(y, circular=circular)
        y = y * db_to_amp(target_lufs - lufs)
        peak = true_peak_db(y, circular=circular)
        if peak <= ceiling_db:
            break
        gr_max = max(gr_max, peak - ceiling_db)
        y = limiter(y, ceiling_db - 0.1, circular=circular, true_peak=True)
    if true_peak_db(y, circular=circular) > ceiling_db:
        y = limiter(y, ceiling_db - 0.2, circular=circular, true_peak=True)
    info = {
        "lufs": integrated_lufs(y, circular=circular),
        "peak_db": sample_peak_db(y),
        "true_peak_db": true_peak_db(y, circular=circular),
        "limit_db": gr_max,
    }
    return y, info


def render_song(song: Song, report: list | None = None) -> tuple[np.ndarray, dict]:
    """渲染整首曲子，返回 ((2, L) float64, 信息)。report 非 None 时追加每条声部的配平记录。"""
    length = song.length
    mix = np.zeros((2, length))
    rev_bus = np.zeros((2, length))
    echo_bus = np.zeros((2, length))
    for track in song.tracks:
        stem = render_stem(song, track)
        measured = integrated_lufs(stem, circular=True)
        if not np.isfinite(measured):
            raise ValueError(f"{song.id}/{track.name}：声部是静音的")
        gain = db_to_amp(REF_LUFS + track.level - measured)
        stem = stem * gain
        mix += stem
        if track.send > 0:
            rev_bus += stem * track.send
        if track.echo > 0:
            echo_bus += stem * track.echo
        if report is not None:
            report.append((track.name, track.inst, measured, track.level))
    if song.echo is not None and np.any(echo_bus):
        beats, feedback, taps, damp = song.echo
        wet_echo = convolve_ir(echo_bus, echo_ir(beats * 60.0 / song.bpm, feedback, taps, damp), True)
        mix += wet_echo
        rev_bus += 0.5 * wet_echo
    ir = make_ir(ROOMS[song.room], seed_of(song.id, "room"))
    mix += song.reverb_gain * convolve_ir(rev_bus, ir, circular=True)
    mix = highpass(mix, 28.0, order=2, circular=True)
    return finalize(mix, BGM_TARGET_LUFS, BGM_CEILING_DB, circular=True)
