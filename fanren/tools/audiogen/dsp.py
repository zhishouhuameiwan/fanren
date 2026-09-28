# -*- coding: utf-8 -*-
"""音频生成器的底层 DSP：滤波、包络、插值、放置、混响、回声、限幅、响度。

为什么全部自己写、不引 pyloudnorm / pedalboard 之类的现成库：
工具链只保证有 numpy + scipy + soundfile。多一个依赖就多一处「换台机器跑不起来」，
而这里要的都是教科书级的东西（RBJ 双二阶、BS.1770 K 加权、卷积混响），自己写反而好查。

**循环（circular）约定**：BGM 是无缝循环的，一首曲子在内存里就是长度为 L 的一个周期。
凡是带记忆的处理（IIR 滤波、混响、限幅器的释放）都按「周期信号的稳态响应」来算——
把信号前面接上它自己的尾巴先跑一段热身，再取正式那一段。这样循环点处滤波器的状态与
曲子中间任何一处一样，不会凭空多出一个「从静止启动」的瞬态，接缝也就不会咔哒一声。
卷积类处理（混响、回声）则直接做线性卷积，再把伸出 L 的尾巴折回开头——
混响尾巴绕回开头，正是「无缝循环」要的样子。

**确定性**：本文件不持有任何全局随机状态，要随机数的函数一律显式收 `rng` 或 `seed`。
"""
from __future__ import annotations

import math
from dataclasses import dataclass

import numpy as np
from scipy import signal
from scipy.ndimage import minimum_filter1d

SR = 44100
TWO_PI = 2.0 * math.pi
LN1000 = math.log(1000.0)  # 60 dB 衰减对应的自然对数：exp(-LN1000 * t / T60)


def db_to_amp(db: float) -> float:
    return 10.0 ** (db / 20.0)


def amp_to_db(amp: float, floor: float = 1e-12) -> float:
    return 20.0 * math.log10(max(float(amp), floor))


def samples(seconds: float) -> int:
    return int(round(seconds * SR))


def midi_to_hz(midi: float) -> float:
    return 440.0 * 2.0 ** ((midi - 69.0) / 12.0)


# ============================================================== 滤波

def _sos_row(b: list[float], a: list[float]) -> np.ndarray:
    a0 = a[0]
    return np.array([[b[0] / a0, b[1] / a0, b[2] / a0, 1.0, a[1] / a0, a[2] / a0]])


def rbj_peaking(f0: float, gain_db: float, q: float) -> np.ndarray:
    """RBJ cookbook 峰值均衡。乐器「琴身共鸣」都用它堆出来。"""
    amp = 10.0 ** (gain_db / 40.0)
    w0 = TWO_PI * f0 / SR
    alpha = math.sin(w0) / (2.0 * q)
    cw = math.cos(w0)
    return _sos_row([1 + alpha * amp, -2 * cw, 1 - alpha * amp],
                    [1 + alpha / amp, -2 * cw, 1 - alpha / amp])


def rbj_shelf(kind: str, f0: float, gain_db: float, q: float = 0.7071) -> np.ndarray:
    amp = 10.0 ** (gain_db / 40.0)
    w0 = TWO_PI * f0 / SR
    alpha = math.sin(w0) / (2.0 * q)
    cw = math.cos(w0)
    sa = 2.0 * math.sqrt(amp) * alpha
    if kind == "low":
        b = [amp * ((amp + 1) - (amp - 1) * cw + sa), 2 * amp * ((amp - 1) - (amp + 1) * cw),
             amp * ((amp + 1) - (amp - 1) * cw - sa)]
        a = [(amp + 1) + (amp - 1) * cw + sa, -2 * ((amp - 1) + (amp + 1) * cw),
             (amp + 1) + (amp - 1) * cw - sa]
    elif kind == "high":
        b = [amp * ((amp + 1) + (amp - 1) * cw + sa), -2 * amp * ((amp - 1) + (amp + 1) * cw),
             amp * ((amp + 1) + (amp - 1) * cw - sa)]
        a = [(amp + 1) - (amp - 1) * cw + sa, 2 * ((amp - 1) - (amp + 1) * cw),
             (amp + 1) - (amp - 1) * cw - sa]
    else:
        raise ValueError(f"未知的搁架类型：{kind}")
    return _sos_row(b, a)


def butter(kind: str, freq, order: int = 2) -> np.ndarray:
    return signal.butter(order, freq, btype=kind, fs=SR, output="sos")


def eq_sos(bands) -> np.ndarray:
    """把一串均衡描述拼成一个 SOS 级联。

    描述是元组，第一项是种类：
      ("hp", f[, order])  ("lp", f[, order])  ("bp", lo, hi[, order])
      ("peak", f, dB, Q)  ("lowshelf", f, dB)  ("highshelf", f, dB)
    """
    rows = []
    for band in bands:
        kind = band[0]
        if kind == "hp":
            rows.append(butter("highpass", band[1], band[2] if len(band) > 2 else 2))
        elif kind == "lp":
            rows.append(butter("lowpass", band[1], band[2] if len(band) > 2 else 2))
        elif kind == "bp":
            rows.append(butter("bandpass", [band[1], band[2]], band[3] if len(band) > 3 else 2))
        elif kind == "peak":
            rows.append(rbj_peaking(band[1], band[2], band[3]))
        elif kind == "lowshelf":
            rows.append(rbj_shelf("low", band[1], band[2]))
        elif kind == "highshelf":
            rows.append(rbj_shelf("high", band[1], band[2]))
        else:
            raise ValueError(f"未知的均衡段：{band!r}")
    if not rows:
        return np.array([[1.0, 0.0, 0.0, 1.0, 0.0, 0.0]])
    return np.vstack(rows)


def apply_sos(x: np.ndarray, sos: np.ndarray, circular: bool = False,
              warm_s: float = 0.6) -> np.ndarray:
    """沿最后一维滤波。circular=True 时按周期信号的稳态响应算（见文件头）。"""
    if circular:
        warm = min(x.shape[-1], samples(warm_s))
        ext = np.concatenate([x[..., -warm:], x], axis=-1)
        return signal.sosfilt(sos, ext, axis=-1)[..., warm:]
    return signal.sosfilt(sos, x, axis=-1)


def eq(x: np.ndarray, bands, circular: bool = False) -> np.ndarray:
    if not bands:
        return x
    return apply_sos(x, eq_sos(bands), circular=circular)


def lowpass(x: np.ndarray, fc: float, order: int = 2, circular: bool = False) -> np.ndarray:
    return apply_sos(x, butter("lowpass", min(fc, SR * 0.45), order), circular)


def highpass(x: np.ndarray, fc: float, order: int = 2, circular: bool = False) -> np.ndarray:
    return apply_sos(x, butter("highpass", fc, order), circular)


def bandpass(x: np.ndarray, lo: float, hi: float, order: int = 2,
             circular: bool = False) -> np.ndarray:
    return apply_sos(x, butter("bandpass", [lo, min(hi, SR * 0.45)], order), circular)


# ============================================================== 包络与插值

def time_axis(n: int) -> np.ndarray:
    return np.arange(n, dtype=np.float64) / SR


def exp_decay(n: int, t60: float) -> np.ndarray:
    return np.exp(-LN1000 * time_axis(n) / max(t60, 1e-4))


def raised_cosine(n: int) -> np.ndarray:
    """0→1 的升余弦，长度 n。淡入淡出都用它：比线性斜坡少一处导数突变，不发「噗」声。"""
    if n <= 0:
        return np.zeros(0)
    return 0.5 - 0.5 * np.cos(np.pi * (np.arange(n) + 0.5) / n)


def fade(x: np.ndarray, fade_in_s: float = 0.0, fade_out_s: float = 0.0) -> np.ndarray:
    y = np.array(x, dtype=np.float64, copy=True)
    n = y.shape[-1]
    fi = min(n, samples(fade_in_s))
    fo = min(n, samples(fade_out_s))
    if fi > 0:
        y[..., :fi] *= raised_cosine(fi)
    if fo > 0:
        y[..., n - fo:] *= raised_cosine(fo)[::-1]
    return y


def attack_env(n: int, attack_s: float) -> np.ndarray:
    env = np.ones(n)
    a = min(n, max(1, samples(attack_s)))
    env[:a] = raised_cosine(a)
    return env


def hermite_read(x: np.ndarray, pos: np.ndarray) -> np.ndarray:
    """四点三次 Hermite 插值读表。越界处读作 0。

    变调（按音、揉弦、滑音）都是「按变速读回已渲染好的音」，线性插值会把高频抹掉
    一截，拨弦的亮度就没了；三次 Hermite 在这种近似整速的读法下几乎不损高频。
    """
    n = x.shape[-1]
    xp = np.concatenate([[0.0], x, [0.0, 0.0, 0.0]])
    p = pos + 1.0
    idx = np.floor(p).astype(np.int64)
    valid = (idx >= 1) & (idx <= n)
    idx = np.clip(idx, 1, n + 1)
    t = p - idx
    y0 = xp[idx - 1]
    y1 = xp[idx]
    y2 = xp[idx + 1]
    y3 = xp[idx + 2]
    c1 = 0.5 * (y2 - y0)
    c2 = y0 - 2.5 * y1 + 2.0 * y2 - 0.5 * y3
    c3 = 0.5 * (y3 - y0) + 1.5 * (y1 - y2)
    out = ((c3 * t + c2) * t + c1) * t + y1
    out[~valid] = 0.0
    return out


def warp(x: np.ndarray, ratio: np.ndarray) -> np.ndarray:
    """按逐样本的播放速率 ratio 重读 x（ratio=1 原速，>1 变高）。输出长度 = len(ratio)。"""
    pos = np.concatenate([[0.0], np.cumsum(ratio[:-1], dtype=np.float64)])
    return hermite_read(x, pos)


def warp_input_length(ratio: np.ndarray) -> int:
    """warp 需要的输入长度：读指针走到哪就得渲染到哪。"""
    return int(math.ceil(float(np.sum(ratio)))) + 4


# ============================================================== 放置与声像

def mix_into(buf: np.ndarray, sig: np.ndarray, start: int, circular: bool) -> None:
    """把 sig 叠加进 buf 的 start 处。circular=True 时越过末尾的部分绕回开头。

    这是本工具唯一**原地修改**的函数：它是混音总线的累加器，每首曲子要叠上千个音，
    每次都复制一份几百万样本的总线是纯浪费。调用方持有 buf，语义清楚。
    """
    length = buf.shape[-1]
    n = sig.shape[-1]
    if n == 0:
        return
    if circular:
        start %= length
        pos = 0
        while pos < n:
            s = (start + pos) % length
            m = min(n - pos, length - s)
            buf[..., s:s + m] += sig[..., pos:pos + m]
            pos += m
        return
    s0 = max(start, 0)
    e = min(start + n, length)
    if e > s0:
        buf[..., s0:e] += sig[..., s0 - start:e - start]


def pan_mono(x: np.ndarray, pan: float) -> np.ndarray:
    """等功率声像。pan ∈ [-1, 1]，0 为正中。"""
    theta = (float(np.clip(pan, -1.0, 1.0)) + 1.0) * math.pi / 4.0
    return np.stack([x * math.cos(theta), x * math.sin(theta)])


# ============================================================== 周期噪声

def spectral_noise(rng: np.random.Generator, n: int, weight) -> np.ndarray:
    """频域合成的噪声：给每个频点一个高斯复数，再乘权重 weight(freqs)。

    用逆 FFT 生成的噪声**天然以 n 为周期**——这正是循环 BGM 里环境声（流水、风、
    识海里的低语）要的：首尾接得上，不用再做交叉淡化。
    """
    freqs = np.fft.rfftfreq(n, 1.0 / SR)
    spec = (rng.standard_normal(freqs.size) + 1j * rng.standard_normal(freqs.size))
    spec *= weight(freqs)
    spec[0] = 0.0
    out = np.fft.irfft(spec, n)
    std = float(np.std(out))
    return out / std if std > 0 else out


def smooth_random(rng: np.random.Generator, n: int, max_hz: float) -> np.ndarray:
    """0 均值、单位方差、以 n 为周期的慢变随机曲线（给风声的起伏、音高的漂移用）。"""
    return spectral_noise(rng, n, lambda f: (f > 0) & (f <= max_hz))


# ============================================================== 混响

@dataclass(frozen=True)
class Room:
    """程序生成混响 IR 的参数。

    为什么用卷积混响而不是 Freeverb：Freeverb 的梳状滤波器在 numpy 里只能逐样本或逐块
    循环，一首八十秒的曲子要跑很久；而生成的 IR 一次 FFT 卷积就完事，尾巴还更密、更平滑。
    IR 本身是「分频段指数衰减的高斯噪声 + 稀疏早期反射 + 可选离散回声」，
    高频衰减得比低频快，这是真实房间「越到尾巴越暗」的听感来源。
    """

    rt60: float
    predelay_ms: float
    early_ms: float
    early_taps: int
    early_gain: float
    low_ratio: float
    high_ratio: float
    width: float = 1.0
    echoes: tuple[tuple[float, float], ...] = ()


ROOMS: dict[str, Room] = {
    # 小室：居所、藏书处、密室。短、亮度中等，早期反射明显（墙近）。
    "room": Room(0.95, 6.0, 28.0, 12, 0.55, 1.1, 0.55, 0.9),
    # 户外开阔：村、镇、渡口。很短的尾巴，只给一点「空气」。
    "open": Room(1.25, 14.0, 55.0, 8, 0.32, 1.0, 0.5, 0.85),
    # 堂：外刃堂、客栈、墨府。
    "hall": Room(2.3, 22.0, 75.0, 14, 0.4, 1.15, 0.5, 1.0),
    # 山门、宗庙：长而暗。
    "temple": Room(3.6, 32.0, 110.0, 16, 0.42, 1.3, 0.42, 1.0),
    # 山谷：中等尾巴 + 两三声山谷回声。
    # 回声增益按「比同一时刻的混响尾巴高出 3–4 dB」取，低了就被尾巴吞掉，听不出是回声。
    "valley": Room(2.1, 20.0, 70.0, 8, 0.3, 1.0, 0.5, 0.85,
                   echoes=((210.0, 0.38), (455.0, 0.22), (720.0, 0.12))),
    # 暗道：暗、湿，早期反射密（石壁近），带短回声。
    "cave": Room(3.3, 10.0, 60.0, 18, 0.6, 1.45, 0.33, 1.0,
                 echoes=((105.0, 0.30), (260.0, 0.18))),
    # 战斗：短而亮的「板式」混响，不拖泥带水。
    "plate": Room(1.45, 8.0, 24.0, 12, 0.35, 0.9, 0.7, 1.0),
    # 识海：巨大、不真实。
    "abyss": Room(5.8, 45.0, 160.0, 12, 0.3, 1.2, 0.65, 1.0,
                  echoes=((350.0, 0.35), (760.0, 0.22))),
}


def make_ir(room: Room, seed: int) -> np.ndarray:
    """按 Room 生成立体声 IR，形状 (2, n)。同一 seed 逐样本相同。"""
    rng = np.random.default_rng(seed)
    pre = room.predelay_ms / 1000.0
    n = samples(room.rt60 * 1.5 + pre + 0.25)
    t = time_axis(n)
    since = np.maximum(t - pre, 0.0)
    bands = (
        (butter("lowpass", 300.0, 2), room.low_ratio),
        (butter("bandpass", [300.0, 1500.0], 2), 1.0),
        (butter("bandpass", [1500.0, 5000.0], 2), 0.5 * (1.0 + room.high_ratio)),
        (butter("highpass", 5000.0, 2), room.high_ratio),
    )
    onset = np.clip(since / 0.03, 0.0, 1.0)
    onset = 0.5 - 0.5 * np.cos(np.pi * onset)
    tails = np.zeros((2, n))
    for ch in range(2):
        tail = np.zeros(n)
        for sos, ratio in bands:
            band = signal.sosfilt(sos, rng.standard_normal(n))
            tail += band * np.exp(-LN1000 * since / (room.rt60 * ratio))
        tails[ch] = tail * onset
    mid = 0.5 * (tails[0] + tails[1])
    side = 0.5 * (tails[0] - tails[1]) * room.width
    tails = np.stack([mid + side, mid - side])
    energy = np.sqrt(np.sum(tails ** 2, axis=1, keepdims=True))
    tails /= np.maximum(energy, 1e-12)

    ir = tails
    smear = np.hanning(9)
    smear /= smear.sum()
    for ch in range(2):
        taps = np.sort(rng.uniform(0.25 * pre, pre + room.early_ms / 1000.0, room.early_taps))
        for k, tap in enumerate(taps):
            pos = samples(tap)
            if pos + smear.size >= n:
                continue
            gain = room.early_gain * rng.uniform(0.35, 1.0) * (1.0 - 0.6 * k / room.early_taps)
            gain *= 1.0 if rng.random() < 0.5 else -1.0
            ir[ch, pos:pos + smear.size] += gain * smear
    for k, (delay_ms, gain) in enumerate(room.echoes):
        # 离散回声：一小团低通过的噪声，而不是一个脉冲——山壁把声音「糊」回来的样子。
        pos = samples(delay_ms / 1000.0)
        width = samples(0.03)
        if pos + width >= n:
            continue
        burst = lowpass(rng.standard_normal(width), 2500.0) * np.hanning(width)
        burst *= gain / max(float(np.sqrt(np.sum(burst ** 2))), 1e-12)
        ch = k % 2
        ir[ch, pos:pos + width] += burst
        ir[1 - ch, pos + samples(0.004):pos + samples(0.004) + width] += 0.6 * burst
    return ir


def convolve_ir(x: np.ndarray, ir: np.ndarray, circular: bool) -> np.ndarray:
    """立体声进、立体声出的卷积混响。circular=True 时尾巴折回开头，输出长度不变。

    左右输入各自以 7:3 混到两侧 IR：单个声部即使摆在最左，混响也会从两边回来——
    空间是包着人的，不是贴在一侧墙上。
    """
    x = np.atleast_2d(x)
    if x.shape[0] == 1:
        x = np.repeat(x, 2, axis=0)
    feed = np.stack([0.7 * x[0] + 0.3 * x[1], 0.3 * x[0] + 0.7 * x[1]])
    y = signal.oaconvolve(feed, ir, axes=-1)
    length = x.shape[-1]
    if not circular:
        return y
    out = y[:, :length].copy()
    tail = y[:, length:]
    pos = 0
    while pos < tail.shape[1]:
        m = min(length, tail.shape[1] - pos)
        out[:, :m] += tail[:, pos:pos + m]
        pos += m
    return out


def echo_ir(delay_s: float, feedback: float, taps: int, damp_hz: float,
            pingpong: bool = True) -> np.ndarray:
    """反馈延迟（回声）等效成的有限长 IR。

    每一跳都更暗一点（高斯核逐跳变宽），模拟带阻尼的磁带回声。写成 FIR 再卷积，
    就不必逐样本跑反馈回路，循环时也能与混响用同一套「尾巴折回」的办法。
    """
    n = samples(delay_s * taps) + samples(0.05)
    ir = np.zeros((2, n))
    for k in range(1, taps + 1):
        pos = samples(delay_s * k)
        sigma = max(0.5, k * SR / (TWO_PI * damp_hz))
        half = int(min(4 * sigma, samples(0.02)))
        kern = np.exp(-0.5 * (np.arange(-half, half + 1) / sigma) ** 2)
        kern /= kern.sum()
        gain = feedback ** (k - 1)
        lo = pos - half
        hi = pos + half + 1
        if lo < 0 or hi > n:
            continue
        if pingpong:
            ir[(k - 1) % 2, lo:hi] += gain * kern
        else:
            ir[:, lo:hi] += gain * kern
    return ir


# ============================================================== 失真

def soft_clip(x: np.ndarray, drive: float) -> np.ndarray:
    """tanh 软削波，归一到单位增益附近。drive=1 几乎透明，4 以上明显发毛。"""
    if drive <= 0:
        return x
    return np.tanh(drive * x) / math.tanh(drive)


# ============================================================== 响度（ITU-R BS.1770-4）

def _k_weight_sos(fs: int = SR) -> np.ndarray:
    """K 加权的两级双二阶，按 libebur128 的写法在任意采样率上重算。

    为什么不用 RBJ 搁架公式（pyloudnorm 缺省的做法）：那样得到的高搁架在 1–2 kHz 过渡带
    与 BS.1770 公布的 48 kHz 系数差出最多 0.44 dB。libebur128 用 K = tan(πf0/fs) 与经验指数
    Vb = Vh^0.4996667741545416，在 48 kHz 上逐位复现公布的系数（见 _k_weight_selftest）。
    """
    f0 = 1681.974450955533
    gain_db = 3.999843853973347
    q = 0.7071752369554196
    k = math.tan(math.pi * f0 / fs)
    vh = 10.0 ** (gain_db / 20.0)
    vb = vh ** 0.4996667741545416
    a0 = 1.0 + k / q + k * k
    shelf_b = [(vh + vb * k / q + k * k) / a0, 2.0 * (k * k - vh) / a0, (vh - vb * k / q + k * k) / a0]
    shelf_a = [1.0, 2.0 * (k * k - 1.0) / a0, (1.0 - k / q + k * k) / a0]
    f0 = 38.13547087613982
    q = 0.5003270373253953
    k = math.tan(math.pi * f0 / fs)
    a0 = 1.0 + k / q + k * k
    hp_b = [1.0, -2.0, 1.0]
    hp_a = [1.0, 2.0 * (k * k - 1.0) / a0, (1.0 - k / q + k * k) / a0]
    return np.vstack([_sos_row(shelf_b, shelf_a), _sos_row(hp_b, hp_a)])


def _k_weight_selftest() -> float:
    """与 BS.1770-4 表 1、表 2 公布的 48 kHz 系数比，返回最大绝对误差（应 < 1e-12 量级）。"""
    official = np.array([[1.53512485958697, -2.69169618940638, 1.19839281085285,
                          1.0, -1.69065929318241, 0.73248077421585],
                         [1.0, -2.0, 1.0, 1.0, -1.99004745483398, 0.99007225036621]])
    return float(np.max(np.abs(_k_weight_sos(48000) - official)))


_K_SOS = _k_weight_sos()


def _block_powers(x: np.ndarray, circular: bool) -> np.ndarray:
    x = np.atleast_2d(x)
    xk = apply_sos(x, _K_SOS, circular=circular, warm_s=0.5)
    power = np.sum(xk ** 2, axis=0)
    blk = samples(0.4)
    hop = samples(0.1)
    if circular:
        ext = np.concatenate([power, power[:blk]])
        starts = np.arange(0, power.size, hop)
    else:
        ext = power if power.size >= blk else np.concatenate([power, np.zeros(blk - power.size)])
        starts = np.arange(0, ext.size - blk + 1, hop)
    cs = np.concatenate([[0.0], np.cumsum(ext)])
    return (cs[starts + blk] - cs[starts]) / blk


def integrated_lufs(x: np.ndarray, circular: bool = False) -> float:
    z = _block_powers(x, circular)
    lk = -0.691 + 10.0 * np.log10(z + 1e-20)
    gated = z[lk > -70.0]
    if gated.size == 0:
        return float("-inf")
    rel = -0.691 + 10.0 * math.log10(float(np.mean(gated))) - 10.0
    final = z[(lk > -70.0) & (lk > rel)]
    return -0.691 + 10.0 * math.log10(float(np.mean(final)))


def momentary_max_lufs(x: np.ndarray) -> float:
    """最大瞬时响度（400 ms 窗）。短音效用它比积分响度更贴近「听起来多响」。"""
    z = _block_powers(x, circular=False)
    return -0.691 + 10.0 * math.log10(float(np.max(z)) + 1e-20)


def true_peak_db(x: np.ndarray, circular: bool = False) -> float:
    """4 倍过采样估计的真峰值（dBTP）。"""
    x = np.atleast_2d(x)
    pad = 64
    if circular:
        ext = np.concatenate([x[:, -pad:], x, x[:, :pad]], axis=1)
    else:
        ext = np.concatenate([np.zeros((x.shape[0], pad)), x, np.zeros((x.shape[0], pad))], axis=1)
    up = signal.resample_poly(ext, 4, 1, axis=1)[:, 4 * pad:-4 * pad]
    return amp_to_db(float(np.max(np.abs(up))))


def sample_peak_db(x: np.ndarray) -> float:
    return amp_to_db(float(np.max(np.abs(x))))


# ============================================================== 限幅

def _true_peak_env(x: np.ndarray, circular: bool) -> np.ndarray:
    """逐样本的真峰值估计：4 倍过采样后，每个原样本与其后三个插值点里取最大。"""
    pad = 64
    if circular:
        ext = np.concatenate([x[:, -pad:], x, x[:, :pad]], axis=1)
    else:
        ext = np.concatenate([np.zeros((x.shape[0], pad)), x, np.zeros((x.shape[0], pad))], axis=1)
    up = np.abs(signal.resample_poly(ext, 4, 1, axis=1)[:, 4 * pad:4 * pad + 4 * x.shape[1]])
    return np.max(up.reshape(x.shape[0], x.shape[1], 4), axis=2)


def limiter(x: np.ndarray, ceiling_db: float, attack_ms: float = 1.5,
            release_ms: float = 120.0, circular: bool = False, true_peak: bool = False) -> np.ndarray:
    """前瞻砖墙限幅（样本峰值不超过 ceiling；true_peak=True 时连样本之间的真峰值一起管）。

    做法：逐样本算「不超限需要的增益」target，先做半宽 A 的滑动最小（前瞻 + 保持），
    再做只许慢慢回升的块级释放，最后用半宽 ≤ A 的 Hann 窗平滑。
    因为平滑窗不比最小滤波窗宽，平滑后每一点的增益仍 ≤ 该点的 target——
    所以这是「保证不超」的，而不是「大概不超」的（真峰值模式下增益在两个样本之间的变化
    极小，结果以事后复测为准，见 mixer.finalize）。
    """
    x = np.atleast_2d(np.asarray(x, dtype=np.float64))
    n = x.shape[1]
    ceil = db_to_amp(ceiling_db)
    level = np.abs(x)
    if true_peak:
        level = np.maximum(level, _true_peak_env(x, circular))
    peak = np.max(level, axis=0)
    target = np.minimum(1.0, ceil / np.maximum(peak, 1e-12))
    if float(np.min(target)) >= 1.0:
        return x.copy()
    half = max(2, int(round(attack_ms * SR / 1000.0)))
    mode = "wrap" if circular else "nearest"
    held = minimum_filter1d(target, size=2 * half + 1, mode=mode)

    blk = max(1, half // 2)
    nb = -(-n // blk)
    padded = np.concatenate([held, held[:nb * blk - n]]) if circular else \
        np.concatenate([held, np.ones(nb * blk - n)])
    block_min = padded.reshape(nb, blk).min(axis=1)
    rel = math.exp(-blk / (release_ms * SR / 1000.0))
    seq = np.concatenate([block_min, block_min]) if circular else block_min
    gain = np.empty_like(seq)
    cur = 1.0
    for i, v in enumerate(seq.tolist()):
        cur = 1.0 - (1.0 - cur) * rel
        if v < cur:
            cur = v
        gain[i] = cur
    if circular:
        gain = gain[nb:]
    stepped = np.repeat(gain, blk)[:n]

    # stepped 逐点 ≤ held（块最小只会更小），所以平滑半宽取到 half 仍守得住上限。
    win = np.hanning(2 * half + 3)[1:-1]
    win /= win.sum()
    h = win.size // 2
    if circular:
        ext = np.concatenate([stepped[-h:], stepped, stepped[:h]]) if h > 0 else stepped
    else:
        ext = np.concatenate([np.full(h, stepped[0]), stepped, np.full(h, stepped[-1])]) \
            if h > 0 else stepped
    smooth = np.convolve(ext, win, mode="valid")
    smooth = np.minimum(smooth, target)  # 数值上再夹一次，防浮点误差让个别样本擦边
    return x * smooth
