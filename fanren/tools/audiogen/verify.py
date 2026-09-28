# -*- coding: utf-8 -*-
"""生成物自检：逐文件解码回来量时长、采样率、声道、峰值、响度、直流、NaN、循环接缝。

量的是**游戏真正会读的那个 .ogg 解码结果**，不是生成时内存里的 PCM——编码器可能让峰值
过冲、可能在首尾引入瑕疵，只有解码结果能说明问题。

循环接缝的三道检查（BGM）——只查「坏循环」的特征，不把曲首的强拍误当成毛病：
  1. 台阶：末样本→首样本的差，不许大于接缝两侧各 20 ms 内最大相邻样本差的 1.5 倍。
     真正的断口是一个孤立的台阶；起音再陡，前后几毫秒里也有同量级的台阶。
  2. 折角：跨接缝的二阶差分（波形的「折角」），同样与两侧 20 ms 内的最大值比，≤ 1.5 倍。
     断口即使恰好数值接近（台阶小），斜率也会突变，咔哒声就藏在这里。
  3. 掉电平：接缝前 50 ms 比接缝后 50 ms 响了多少（只算「掉」，不算「涨」）——
     混响尾巴没绕回来、或者曲尾戛然而止，都表现为跨过接缝音量突然掉下去；
     曲首一记锣、一个强拍是「涨」，那是音乐，不是毛病。门槛是全曲相邻 50 ms 掉幅的 99 百分位，下限 3 dB。
音效（不循环）改查：首样本从静音起、尾 10 ms 已衰减到 -50 dBFS 以下（没有被硬切）。
"""
from __future__ import annotations

import math
import os
from dataclasses import dataclass

import numpy as np
import soundfile as sf

from dsp import (SR, amp_to_db, integrated_lufs, momentary_max_lufs, sample_peak_db, samples,
                 true_peak_db)


@dataclass(frozen=True)
class Spec:
    kind: str                 # "bgm" / "sfx"
    min_s: float
    max_s: float
    loop: bool
    lufs_range: tuple | None = None


def _seam(x: np.ndarray) -> dict:
    w = samples(0.02)
    around = np.concatenate([x[:, -w:], x[:, :w]], axis=1)  # 接缝落在第 w-1 与第 w 个样本之间
    d1 = np.abs(np.diff(around, axis=1))
    jump = float(np.max(d1[:, w - 1]))
    local1 = float(np.max(np.delete(d1, w - 1, axis=1)))
    d2 = np.abs(np.diff(around, n=2, axis=1))  # d2[k] 用到 k, k+1, k+2；跨接缝的是 k = w-2 与 w-1
    bend = float(np.max(d2[:, w - 2:w]))
    local2 = float(np.max(np.delete(d2, [w - 2, w - 1], axis=1)))
    blk = samples(0.05)
    nb = x.shape[1] // blk
    power = np.mean(x[:, :nb * blk].reshape(x.shape[0], nb, blk) ** 2, axis=(0, 2))
    levels = 10.0 * np.log10(power + 1e-12)
    drops = np.maximum(0.0, levels[:-1] - levels[1:])
    head = 10.0 * math.log10(float(np.mean(x[:, :blk] ** 2)) + 1e-12)
    tail = 10.0 * math.log10(float(np.mean(x[:, -blk:] ** 2)) + 1e-12)
    return {
        "jump_ratio": jump / max(local1, 1e-12),
        "bend_ratio": bend / max(local2, 1e-12),
        "drop": max(0.0, tail - head),
        "drop_limit": max(3.0, float(np.percentile(drops, 99.0))),
    }


def analyze(path: str, spec: Spec) -> dict:
    x, sr = sf.read(path, dtype="float64", always_2d=True)
    x = x.T
    res = {"path": path, "sr": sr, "channels": x.shape[0], "dur": x.shape[1] / sr}
    res["nan"] = int(np.sum(~np.isfinite(x)))
    x = np.nan_to_num(x)
    res["peak"] = sample_peak_db(x)
    res["tpeak"] = true_peak_db(x, circular=spec.loop)
    res["rms"] = amp_to_db(math.sqrt(float(np.mean(x ** 2))))
    res["dc"] = float(np.max(np.abs(np.mean(x, axis=1))))
    if spec.loop:
        res["lufs"] = integrated_lufs(x, circular=True)
        res.update(_seam(x))
    else:
        res["lufs"] = momentary_max_lufs(x)
        res["head"] = float(np.max(np.abs(x[:, 0])))
        res["tail"] = amp_to_db(math.sqrt(float(np.mean(x[:, -samples(0.01):] ** 2))))
        res["attack_ms"] = 1000.0 * int(np.argmax(np.max(np.abs(x), axis=0))) / sr
    problems = []
    if sr != SR:
        problems.append(f"采样率 {sr}")
    if x.shape[0] != 2:
        # 音效也统一立体声：混着单声道，哪天引擎给音效加了声像，单声道那几条就会表现不一致。
        problems.append("应为立体声")
    if not spec.min_s <= res["dur"] <= spec.max_s:
        problems.append(f"时长 {res['dur']:.2f}s 不在 [{spec.min_s}, {spec.max_s}]")
    if res["nan"]:
        problems.append("含 NaN/Inf")
    if res["peak"] > -1.0:
        problems.append(f"峰值 {res['peak']:.2f} dBFS > -1")
    if res["dc"] > (0.002 if spec.loop else 0.005):
        problems.append(f"直流 {res['dc']:.4f}")
    if spec.lufs_range and not spec.lufs_range[0] <= res["lufs"] <= spec.lufs_range[1]:
        problems.append(f"响度 {res['lufs']:.1f} 不在 {spec.lufs_range}")
    if res["tpeak"] > -1.0:
        problems.append(f"真峰值 {res['tpeak']:.2f} dBTP > -1")
    if spec.loop:
        if res["jump_ratio"] > 1.5:
            problems.append(f"接缝台阶 {res['jump_ratio']:.2f}×")
        if res["bend_ratio"] > 1.5:
            problems.append(f"接缝折角 {res['bend_ratio']:.2f}×")
        if res["drop"] > res["drop_limit"]:
            problems.append(f"跨接缝掉 {res['drop']:.1f} dB")
    else:
        if res["head"] > 1e-3:
            problems.append(f"首样本 {res['head']:.4f} 非静音")
        if res["tail"] > -50.0:
            problems.append(f"尾部 {res['tail']:.1f} dBFS 未收干净")
    res["problems"] = problems
    return res


def table(rows: list[dict]) -> str:
    """把 analyze 的结果排成一张定宽表（中文列名，给人看）。"""
    head = (f"{'id':<16}{'时长s':>8}{'声道':>5}{'峰值':>8}{'真峰':>8}{'响度':>8}{'RMS':>8}"
            f"{'直流':>9}{'NaN':>5}  {'接缝/首尾':<30}结论")
    lines = [head, "-" * len(head.encode("gbk", "replace"))]
    for r in rows:
        if "jump_ratio" in r:
            seam = (f"台阶{r['jump_ratio']:.2f} 折角{r['bend_ratio']:.2f} "
                    f"掉{r['drop']:.1f}/{r['drop_limit']:.1f}dB")
        else:
            seam = f"首{r['head']:.4f} 尾{r['tail']:.0f}dB 峰@{r['attack_ms']:.0f}ms"
        verdict = "PASS" if not r["problems"] else "FAIL " + "；".join(r["problems"])
        name = os.path.splitext(os.path.basename(r["path"]))[0]
        lines.append(f"{name:<16}{r['dur']:>8.2f}{r['channels']:>5}{r['peak']:>8.2f}{r['tpeak']:>8.2f}"
                     f"{r['lufs']:>8.1f}{r['rms']:>8.1f}{r['dc']:>9.5f}{r['nan']:>5}  {seam:<30}{verdict}")
    return "\n".join(lines)


def plot(paths: list[str], out_png: str, title: str) -> None:
    """每个文件一行：波形、短时响度（3 s 窗）、对数频率的声谱图。给人眼确认「不是噪音、不是死寂」。"""
    import matplotlib

    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    from scipy import signal

    from dsp import _K_SOS

    fig, axes = plt.subplots(len(paths), 3, figsize=(20, 2.4 * len(paths)), squeeze=False,
                             gridspec_kw={"width_ratios": [2, 1.2, 2]})
    for i, path in enumerate(paths):
        x, sr = sf.read(path, dtype="float64", always_2d=True)
        x = x.T
        name = os.path.splitext(os.path.basename(path))[0]
        t = np.arange(x.shape[1]) / sr
        axes[i, 0].plot(t, x[0], lw=0.25)
        axes[i, 0].set_ylim(-1, 1)
        axes[i, 0].set_title(name, fontsize=8, loc="left")
        xk = signal.sosfilt(_K_SOS, x, axis=-1)
        p = np.sum(xk ** 2, axis=0)
        win = int(min(3.0 * sr, max(p.size // 4, 1)))
        hop = max(1, win // 6)
        cs = np.concatenate([[0.0], np.cumsum(p)])
        starts = np.arange(0, max(p.size - win, 1), hop)
        st = -0.691 + 10 * np.log10((cs[starts + win] - cs[starts]) / win + 1e-12)
        axes[i, 1].plot((starts + win / 2) / sr, st, lw=0.8)
        axes[i, 1].axhline(-16.0, color="r", lw=0.5)
        axes[i, 1].set_ylim(-45, -5)
        axes[i, 1].grid(alpha=0.3)
        nper = 4096 if x.shape[1] > 8 * 4096 else 1024
        fr, tt, s = signal.spectrogram(x.mean(axis=0), sr, nperseg=nper, noverlap=nper // 2)
        axes[i, 2].pcolormesh(tt, fr, 10 * np.log10(s + 1e-14), vmin=-110, vmax=-30,
                              shading="auto", cmap="magma")
        axes[i, 2].set_yscale("symlog", linthresh=200)
        axes[i, 2].set_ylim(30, 16000)
    fig.suptitle(title, fontsize=10)
    plt.tight_layout()
    os.makedirs(os.path.dirname(out_png) or ".", exist_ok=True)
    plt.savefig(out_png, dpi=60)
    plt.close(fig)
