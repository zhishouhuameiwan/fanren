# -*- coding: utf-8 -*-
"""乐谱层：简谱小语言、五声调式、声部拼接、伴奏型、鼓谱、人性化。

为什么自己造一个简谱小语言：曲子要「真的作曲」，十八首主旋律若写成 Python 元组，
一眼看不出旋律走向，也没法对着小节线查拍子；简谱是中国乐人最熟的记法，
「5. 6_ 1' 2'_ 1'_ |」读起来就是一句旋律。解析器**逐小节核对时值**，
写错一拍当场报错（带曲名与小节号），不会悄悄错位半首曲子。

记法（空白分隔的记号）：
  1–7        简谱音级（1 = 宫音）。0 = 休止。前缀 # / b 升降半音。
  ' ,        后缀：高 / 低八度，可叠（1'' = 高两个八度）。
  _          后缀：时值减半（5_ = 八分，5__ = 十六分）。
  .          后缀：附点（×1.5）。
  -          后缀或独立记号：延长一拍（5-- = 三拍；独立的 - 延长前一个音或休止）。
  (x)        前缀：倚音，x 是音级（可带八度记号），在主音之前一瞬奏出。
  /          前缀：上滑音——从下方相邻的五声音级滑入。
  后缀装饰：~ 揉弦/颤音   * 摇指/轮指（快速重复）  > 重音   ! 顿音（短）
            ^ 打音（瞬间点到上方相邻音级再回来）  v 下滑音（尾部滑落）
            s 从前一个音滑过来（二胡的滑音、古筝的按滑）  o 泛音
  |          小节线：核对这一小节的拍数。相邻两条小节线之间没有音符（每行首尾都写「|」，
             换行处就是「| |」）视为同一条小节线，不算一个空小节。
  ;          换气/断句（吹奏、拉弦类乐器在此处重新起音）。
  @p @mf …   力度（ppp pp p mp mf f ff），或 @0.65 这样的数值。

「拍」是曲子的计数单位：4/4 拍的曲子一拍 = 四分音符；6/8 的渡口曲按八分音符计拍。
"""
from __future__ import annotations

import math
import re
import zlib
from dataclasses import dataclass, replace

import numpy as np

_NOTE_BASE = {"C": 0, "D": 2, "E": 4, "F": 5, "G": 7, "A": 9, "B": 11}
_DEGREE_SEMI = {1: 0, 2: 2, 3: 4, 4: 5, 5: 7, 6: 9, 7: 11}
PENTA = (1, 2, 3, 5, 6)
_DYN = {"ppp": 0.25, "pp": 0.34, "p": 0.46, "mp": 0.58, "mf": 0.7, "f": 0.83, "ff": 0.95}


def pitch(name: str) -> int:
    """「D4」「F#3」「Bb2」→ MIDI 音高（C4 = 60）。"""
    m = re.fullmatch(r"([A-G])([#b]?)(-?\d)", name)
    if m is None:
        raise ValueError(f"认不出的音名：{name!r}")
    acc = {"": 0, "#": 1, "b": -1}[m.group(2)]
    return 12 * (int(m.group(3)) + 1) + _NOTE_BASE[m.group(1)] + acc


def seed_of(*parts: str) -> int:
    """稳定的种子。不用内置 hash()：它对字符串是逐进程随机化的，两次运行结果会不同。"""
    return zlib.crc32("/".join(parts).encode("utf-8"))


@dataclass(frozen=True)
class Key:
    """一个五声调式的「宫」。调式主音（羽、徵……）由旋律自己落在哪个音上决定。"""

    gong: int

    @staticmethod
    def of(name: str) -> "Key":
        return Key(pitch(name))

    def midi(self, degree: int, octave: int = 0, acc: int = 0) -> int:
        return self.gong + _DEGREE_SEMI[degree] + 12 * octave + acc

    def penta(self, degree: int, octave: int, steps: int) -> int:
        """从 (音级, 八度) 出发在五声音阶上走 steps 级，返回 MIDI。偏音按最近的五声音处理。"""
        if degree not in PENTA:
            degree = {4: 3, 7: 6}[degree]
        idx = PENTA.index(degree) + steps
        return self.midi(PENTA[idx % 5], octave + idx // 5)

    def neighbor(self, midi: float, direction: int) -> float:
        """五声音阶上紧挨着的上/下一个音（direction = +1 / -1）。"""
        rel = int(round(midi)) - self.gong
        cands = [self.gong + _DEGREE_SEMI[d] + 12 * o for o in range(rel // 12 - 2, rel // 12 + 3)
                 for d in PENTA]
        if direction > 0:
            return float(min(c for c in cands if c > midi + 0.5))
        return float(max(c for c in cands if c < midi - 0.5))


@dataclass(frozen=True)
class Note:
    beat: float                 # 起拍（从曲首算，单位：拍）
    beats: float                # 记谱时值（拍）
    midi: float                 # 音高（MIDI，可带小数）
    vel: float = 0.7            # 力度 0–1
    orn: str = ""               # 装饰记号（见文件头）
    grace: float | None = None  # 倚音音高
    slide_from: float | None = None  # 滑入的起点音高（/ 与 s）
    up: float | None = None     # 五声上邻音（打音用）
    down: float | None = None   # 五声下邻音（下滑音用）
    breath: bool = False        # 该音之前换气
    shift: float = 0.0          # 人性化的时间偏移（秒）


_TOKEN = re.compile(
    r"(?:\((?P<gacc>[#b]?)(?P<gdeg>[1-7])(?P<goct>[',]*)\))?"
    r"(?P<scoop>/)?(?P<acc>[#b])?(?P<deg>[0-7])(?P<oct>[',]*)"
    r"(?P<dur>[_.\-]*)(?P<orn>[~*>!^vso]*)"
)


def _octaves(marks: str) -> int:
    return marks.count("'") - marks.count(",")


def _duration(marks: str) -> float:
    return (0.5 ** marks.count("_")) * (1.5 ** marks.count(".")) + marks.count("-")


def parse(text: str, key: Key, meter: float, start: float = 0.0, vel: float | None = None,
          octave: int = 0, where: str = "") -> tuple[list[Note], float]:
    """解析一段简谱，返回 (音符表, 结束拍)。逐小节核对拍数，错了抛 ValueError。"""
    notes: list[Note] = []
    t = start
    bar_start = start
    bar_no = 1
    cur_vel = 0.7 if vel is None else vel
    breath = False
    last_is_note = False
    prev_midi: float | None = None
    for tok in text.split():
        if tok == "|":
            if t - bar_start > 1e-6:
                if abs((t - bar_start) - meter) > 1e-6:
                    raise ValueError(f"{where} 第 {bar_no} 小节是 {t - bar_start:g} 拍，应为 {meter:g} 拍")
                bar_no += 1
            bar_start = t
            continue
        if tok == "-":
            if last_is_note:
                notes[-1] = replace(notes[-1], beats=notes[-1].beats + 1.0)
            t += 1.0
            continue
        if tok == ";":
            breath = True
            continue
        if tok.startswith("@"):
            word = tok[1:]
            cur_vel = _DYN[word] if word in _DYN else float(word)
            continue
        m = _TOKEN.fullmatch(tok)
        if m is None:
            raise ValueError(f"{where} 第 {bar_no} 小节认不出的记号：{tok!r}")
        dur = _duration(m.group("dur"))
        deg = int(m.group("deg"))
        if deg == 0:
            t += dur
            last_is_note = False
            breath = True  # 休止之后必然重新起音
            continue
        acc = {"": 0, "#": 1, "b": -1}[m.group("acc") or ""]
        midi = float(key.midi(deg, octave + _octaves(m.group("oct")), acc))
        orn = m.group("orn")
        grace = None
        if m.group("gdeg"):
            gacc = {"": 0, "#": 1, "b": -1}[m.group("gacc") or ""]
            grace = float(key.midi(int(m.group("gdeg")), octave + _octaves(m.group("goct")), gacc))
        slide_from = None
        if m.group("scoop"):
            slide_from = key.neighbor(midi, -1)
        elif "s" in orn and prev_midi is not None:
            slide_from = prev_midi
        v = cur_vel * (1.18 if ">" in orn else 1.0)
        notes.append(Note(beat=t, beats=dur, midi=midi, vel=min(v, 1.0), orn=orn, grace=grace,
                          slide_from=slide_from, up=key.neighbor(midi, 1),
                          down=key.neighbor(midi, -1), breath=breath))
        breath = False
        last_is_note = True
        prev_midi = midi
        t += dur
    if t - bar_start > 1e-6 and abs((t - bar_start) - meter) > 1e-6:
        raise ValueError(f"{where} 末小节是 {t - bar_start:g} 拍，应为 {meter:g} 拍")
    return notes, t


def seq(key: Key, meter: float, *parts, start_bar: float = 0, octave: int = 0,
        vel: float | None = None, where: str = "") -> tuple[Note, ...]:
    """把若干段简谱首尾相接。整数/浮点参数表示「空若干小节」。"""
    t = start_bar * meter
    out: list[Note] = []
    for i, part in enumerate(parts):
        if isinstance(part, (int, float)):
            t += part * meter
            continue
        notes, t = parse(part, key, meter, t, vel=vel, octave=octave, where=f"{where}#{i}")
        out.extend(notes)
    return tuple(out)


# ============================================================== 变换（一律返回新元组）

def transpose(notes, semitones: float) -> tuple[Note, ...]:
    def mv(v):
        return None if v is None else v + semitones
    return tuple(replace(n, midi=n.midi + semitones, grace=mv(n.grace),
                         slide_from=mv(n.slide_from), up=mv(n.up), down=mv(n.down))
                 for n in notes)


def penta_shift(notes, key: Key, steps: int) -> tuple[Note, ...]:
    """把旋律在五声音阶上平移 steps 级：民乐合奏里「支声」式的平行三、四度声部。

    偏音（4、7、升降音）原样保留；装饰跟着新音高重算（倚音去掉，免得两个声部倚音打架）。
    """
    semi_to_deg = {v: k for k, v in _DEGREE_SEMI.items()}
    out: list[Note] = []
    for n in notes:
        rel = int(round(n.midi)) - key.gong
        octv, semi = divmod(rel, 12)
        deg = semi_to_deg.get(semi)
        if deg not in PENTA:
            out.append(replace(n, grace=None))
            continue
        midi = float(key.penta(deg, octv, steps)) + (n.midi - round(n.midi))
        slide = None
        if n.slide_from is not None:
            slide = out[-1].midi if ("s" in n.orn and out) else key.neighbor(midi, -1)
        out.append(replace(n, midi=midi, grace=None, slide_from=slide,
                           up=key.neighbor(midi, 1), down=key.neighbor(midi, -1)))
    return tuple(out)


def fold(notes, lo: float, hi: float = 127.0) -> tuple[Note, ...]:
    """把超出 [lo, hi] 的音按八度折回乐器音域（伴奏、低音、和声垫用；主旋律别用，会改轮廓）。

    真乐手遇到够不着的音就是这么做的：低音大阮没有 C2，就弹高八度的 C3。
    """
    out = []
    for n in notes:
        # 在所有八度移法里挑：落在窗内的优先；窗比一个八度还窄、怎么移都落不进时，取离窗最近的那个。
        shifts = [12.0 * k for k in range(-10, 11)]
        inside = [s for s in shifts if lo <= n.midi + s <= hi]
        if inside:
            shift = min(inside, key=abs)
        else:
            shift = min(shifts, key=lambda s: min(abs(n.midi + s - lo), abs(n.midi + s - hi)))
        out.append(n if shift == 0.0 else transpose((n,), shift)[0])
    return tuple(out)


def on_beats(notes, meter: float, beats=(0.0,)) -> tuple[Note, ...]:
    """只留落在小节内指定拍位上的音（编钟只敲骨干音之类）。"""
    return tuple(n for n in notes if any(abs((n.beat % meter) - b) < 1e-6 for b in beats))


def shift_beats(notes, beats: float) -> tuple[Note, ...]:
    return tuple(replace(n, beat=n.beat + beats) for n in notes)


def scale_vel(notes, factor: float) -> tuple[Note, ...]:
    return tuple(replace(n, vel=min(1.0, n.vel * factor)) for n in notes)


def only_bars(notes, meter: float, first_bar: int, last_bar: int) -> tuple[Note, ...]:
    """取 [first_bar, last_bar) 小节里起拍的音（小节从 0 数）。"""
    lo = first_bar * meter - 1e-6
    hi = last_bar * meter - 1e-6
    return tuple(n for n in notes if lo <= n.beat < hi)


def strip(notes, marks: str) -> tuple[Note, ...]:
    """去掉某些装饰（同一条旋律给不同乐器时，古筝不需要二胡的揉弦记号之类）。

    marks 里可以有任意后缀装饰字符，外加：s 去掉「从前音滑来」，/ 去掉上滑音，( 去掉倚音。
    """
    out = []
    for n in notes:
        orn = "".join(c for c in n.orn if c not in marks)
        slide = n.slide_from
        if slide is not None:
            from_prev = "s" in n.orn
            if (from_prev and "s" in marks) or (not from_prev and "/" in marks):
                slide = None
        grace = None if "(" in marks else n.grace
        out.append(replace(n, orn=orn, slide_from=slide, grace=grace))
    return tuple(out)


# ============================================================== 伴奏型

def roots(spec: str) -> list[list[int]]:
    """和声根音表：「5 1 6 2 | 1 5/1」→ 每小节一个或几个根音（用 / 把一小节等分）。"""
    bars = []
    for tok in spec.split():
        if tok == "|":
            continue
        bars.append([int(d) for d in tok.split("/")])
    return bars


def arpeggio(key: Key, meter: float, root_spec: str, steps, base_octave: int = 0,
             vel: float = 0.55, start_bar: float = 0, accent: float = 0.12,
             orn_first: str = "", beats: float | None = None, orn: str = "") -> tuple[Note, ...]:
    """分解和弦：每小节按 steps（相对根音的五声级数，None 为空拍）均分铺开。

    一小节里若换了根音（「5/1」），型不重起、只换根——古筝的流水型本来就是这样一路滚下去的。
    beats：每个音的时值（缺省 = 均分的格长）；给大了就是「让弦响着」。
    orn：加在每个音上的记号（固定音型常用「!」顿音）；orn_first 只加在每小节第一个音上。
    """
    out: list[Note] = []
    bars = roots(root_spec)
    n = len(steps)
    step_len = meter / n
    for b, bar_roots in enumerate(bars):
        for i, s in enumerate(steps):
            if s is None:
                continue
            root = bar_roots[min(len(bar_roots) - 1, int(i * len(bar_roots) / n))]
            midi = float(key.penta(root, base_octave, s))
            v = vel * (1.0 + accent if i == 0 else 1.0)
            out.append(Note(beat=(start_bar + b) * meter + i * step_len,
                            beats=beats if beats is not None else step_len, midi=midi,
                            vel=min(v, 1.0), orn=orn + (orn_first if i == 0 else ""),
                            up=key.neighbor(midi, 1), down=key.neighbor(midi, -1)))
    return tuple(out)


def bassline(key: Key, meter: float, root_spec: str, pattern, octave: int = -1,
             vel: float = 0.7, start_bar: float = 0, orn: str = "") -> tuple[Note, ...]:
    """低音：pattern 是 (小节内起拍, 时值, 相对根音的五声级数) 的列表。"""
    out: list[Note] = []
    for b, bar_roots in enumerate(roots(root_spec)):
        for off, dur, step in pattern:
            root = bar_roots[min(len(bar_roots) - 1, int(off * len(bar_roots) / meter))]
            midi = float(key.penta(root, octave, step))
            out.append(Note(beat=(start_bar + b) * meter + off, beats=dur, midi=midi,
                            vel=vel * (1.1 if off == 0 else 1.0), orn=orn,
                            up=key.neighbor(midi, 1), down=key.neighbor(midi, -1)))
    return tuple(out)


def chords(key: Key, meter: float, root_spec: str, voicing, octave: int = 0,
           vel: float = 0.5, start_bar: float = 0, hold: float = 1.0) -> tuple[Note, ...]:
    """持续和弦（笙、低音胡的长音垫底）：每个根音段落铺一组五声音级。"""
    out: list[Note] = []
    for b, bar_roots in enumerate(roots(root_spec)):
        seg = meter / len(bar_roots)
        for j, root in enumerate(bar_roots):
            for s in voicing:
                midi = float(key.penta(root, octave, s))
                out.append(Note(beat=(start_bar + b) * meter + j * seg, beats=seg * hold,
                                midi=midi, vel=vel))
    return tuple(out)


_HIT_VEL = {"X": 1.0, "x": 0.8, "o": 0.58, "g": 0.36}


def hits(meter: float, pattern: str, per_bar: int, start_bar: float = 0, repeat: int = 1,
         midi: float = 60.0, vel: float = 1.0, orn: str = "") -> tuple[Note, ...]:
    """鼓谱：一个字符一格（每小节 per_bar 格）。空格与 | 只为好读，会被忽略。

      X 强  x 中  o 弱  g 鬼音  . 空
      r 滚奏（一格里连击三下，渐强）   f 装饰音（前面贴一个很轻的倚击）
    """
    cells = [c for c in pattern if c not in " |"]
    if len(cells) % per_bar:
        raise ValueError(f"鼓谱长度 {len(cells)} 不是每小节 {per_bar} 格的整数倍：{pattern!r}")
    step = meter / per_bar
    out: list[Note] = []
    span = len(cells) // per_bar
    for r in range(repeat):
        base = (start_bar + r * span) * meter
        for i, c in enumerate(cells):
            t = base + i * step
            if c in _HIT_VEL:
                out.append(Note(beat=t, beats=step, midi=midi, vel=_HIT_VEL[c] * vel, orn=orn))
            elif c == "r":
                for k in range(3):
                    out.append(Note(beat=t + k * step / 3, beats=step / 3, midi=midi,
                                    vel=(0.45 + 0.2 * k) * vel, orn=orn))
            elif c == "f":
                out.append(Note(beat=t - step * 0.18, beats=step * 0.18, midi=midi,
                                vel=0.3 * vel, orn=orn))
                out.append(Note(beat=t, beats=step, midi=midi, vel=0.85 * vel, orn=orn))
            elif c != ".":
                raise ValueError(f"鼓谱里认不出的字符 {c!r}")
    return tuple(out)


def strum(key: Key, beat: float, degrees, octave: int = 0, vel: float = 0.8,
          spread_s: float = 0.018, beats: float = 1.0, up: bool = False) -> tuple[Note, ...]:
    """扫弦：一组音在几十毫秒里依次拨出（琵琶的扫、古筝的抹托连用）。"""
    midis = [float(key.midi(d, octave + o)) for d, o in degrees]
    order = midis[::-1] if up else midis
    return tuple(Note(beat=beat, beats=beats, midi=m, vel=vel * (1.0 - 0.06 * i), shift=i * spread_s)
                 for i, m in enumerate(order))


# ============================================================== 人性化

def humanize(notes, rng: np.random.Generator, meter: float, time_ms: float = 8.0,
             vel_sd: float = 0.06, phrase_bars: float = 4.0, arch: float = 0.12) -> tuple[Note, ...]:
    """给音符加上微小的时值与力度偏差（固定种子），再叠一层「乐句弧线」与强弱拍。

    偏差按音符在表中的顺序逐个抽取，所以同一首曲子每次生成都一样；而循环播放时，
    同一个音在每一遍里的偏差也一样——这正是循环接缝无痕的前提之一。
    """
    out = []
    lim_t = 2.5 * time_ms
    lim_v = 2.5 * vel_sd
    for n in notes:
        dt = float(np.clip(rng.normal(0.0, time_ms), -lim_t, lim_t)) / 1000.0 if time_ms > 0 else 0.0
        dv = float(np.clip(rng.normal(0.0, vel_sd), -lim_v, lim_v)) if vel_sd > 0 else 0.0
        pos = n.beat % meter
        metric = 1.06 if pos < 1e-6 else (0.95 if abs(pos - round(pos)) > 1e-6 else 1.0)
        phase = (n.beat / (phrase_bars * meter)) % 1.0
        shape = 1.0 - arch / 2.0 + arch * math.sin(math.pi * phase)
        v = float(np.clip(n.vel * (1.0 + dv) * metric * shape, 0.05, 1.0))
        out.append(replace(n, vel=v, shift=n.shift + dt))
    return tuple(out)
