# -*- coding: utf-8 -*-
"""写 OGG Vorbis，并按「解码回来的峰值」兜底。

Vorbis 是有损编码，解码后的峰值会比送进去的 PCM 高一点（瞬态越尖越明显，
拨弦与锣鼓常见 +0.3 ~ +0.8 dB）。游戏里放的是解码结果，所以峰值要以解码为准：
编一遍、解一遍、量一遍（样本峰值与真峰值都量），超了 -1 dBFS 就把 PCM 整体压低超出的量再编，最多三轮。
压低是乘一个常数，不改变任何音色，也不破坏循环。

OGG 容器里有随机的流序列号，所以同一 PCM 两次编码字节不同——这不影响确定性的承诺：
确定的是 PCM（见 audiogen.py 的 --determinism），不是文件字节。
"""
from __future__ import annotations

import hashlib
import os

import numpy as np
import soundfile as sf

from dsp import SR, amp_to_db, db_to_amp, true_peak_db

PEAK_LIMIT_DB = -1.0
_MARGIN_DB = 0.08


def pcm_hash(x: np.ndarray) -> str:
    """最终 PCM（float32，声道交织）的 sha256。确定性自检比的就是它。"""
    data = np.ascontiguousarray(np.atleast_2d(x).T.astype(np.float32))
    return hashlib.sha256(data.tobytes()).hexdigest()


def _write_chunked(path: str, data: np.ndarray, quality: float) -> None:
    """分块写。libsndfile 1.2.2 的 Vorbis 编码器一次吃下几十秒的数据会直接崩掉进程
    （实测 90 秒立体声一次 sf.write 即崩，无异常可捕获）；按 8192 帧一块喂就没事，结果与一次写等价。
    compression_level = 1 - 质量：libsndfile 把它映射为 vorbis_encode_init_vbr 的质量参数。"""
    with sf.SoundFile(path, "w", SR, data.shape[1], format="OGG", subtype="VORBIS",
                      compression_level=1.0 - quality) as f:
        for i in range(0, data.shape[0], 8192):
            f.write(data[i:i + 8192])


def write_ogg(path: str, x: np.ndarray, quality: float, circular: bool = False) -> dict:
    """x 形状 (声道, 样本)。返回 {'bytes', 'decoded_peak_db', 'trim_db', 'pcm_sha256'}。

    circular：循环的 BGM 量真峰值时首尾按周期接上，不在两端补零（补零会在边缘凭空造出过冲）。
    """
    os.makedirs(os.path.dirname(path), exist_ok=True)
    pcm = np.atleast_2d(np.asarray(x, dtype=np.float64))
    trim_total = 0.0
    decoded_peak = 0.0
    for _ in range(4):
        data = np.ascontiguousarray(pcm.T.astype(np.float32))
        _write_chunked(path, data, quality)
        decoded, sr = sf.read(path, dtype="float32", always_2d=True)
        if sr != SR or decoded.shape[0] != data.shape[0]:
            raise RuntimeError(f"{path}：解码回来的长度/采样率不对（{decoded.shape[0]} / {sr}）")
        # 样本峰值与真峰值（4 倍过采样）取大者：声卡重建出来的是连续波形，样本之间也会冒尖。
        decoded_peak = max(amp_to_db(float(np.max(np.abs(decoded)))), true_peak_db(decoded.T, circular=circular))
        if decoded_peak <= PEAK_LIMIT_DB:
            break
        trim = PEAK_LIMIT_DB - decoded_peak - _MARGIN_DB
        pcm = pcm * db_to_amp(trim)
        trim_total += trim
    else:
        raise RuntimeError(f"{path}：三轮压低后解码峰值仍为 {decoded_peak:.2f} dBFS")
    return {
        "bytes": os.path.getsize(path),
        "decoded_peak_db": decoded_peak,
        "trim_db": trim_total,
        "pcm_sha256": pcm_hash(pcm),
    }
