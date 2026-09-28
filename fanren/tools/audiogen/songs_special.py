# -*- coding: utf-8 -*-
"""特殊场合的曲子：标题、普通战斗、首领战、识海吞噬战、夜探潜入。

这五首不挂在地图属性上，由标题画面、战斗场景、剧情脚本按 id 点播（id 见 docs/audio.md）。

  - 标题：循环点就是全曲最隆重的一刻——末两小节大鼓滚奏渐强，正好滚进曲首那一记大锣。
  - 战斗：堂鼓 + 琵琶固定音型打底，二胡唱主旋律，C 段拆掉旋律只剩鼓与轮指层层推高。
  - 首领：低音在羽音与它上方的半音之间来回（五声之外的那一个音，就是压迫感），抄锣开段。
  - 识海：倒放的古筝（每个音「吸」进拍点）、延迟回声、削波失真的二胡、失谐的笙、心跳。
  - 夜探：稀疏的顿音拨弦 + 皮下心跳般的闷鼓，大段留白。
"""
from __future__ import annotations

from dataclasses import replace

from mixer import Song, Track
from score import (Key, Note, arpeggio, bassline, chords, fold, hits, only_bars, penta_shift, seq,
                   strip, strum, transpose)

# ============================================================== 标题：大气（A 宫，♩=72）

_TITLE_A = """
| 1 5 1'~ - | 2'_ 1'_ 6_ 5_ 6~ - | 5 3_ 5_ 6 1'_ 6_ | 5~ - - - |
| 1 5 1'~ - | 2'_ 3'_ 5'_ 3'_ /2'~ - | 1'_ 2'_ 1'_ 6_ 5 6 | 1'~ - - - |
"""
_TITLE_COUNTER = """
| 3' - - - | 2' - 1' - | 2' - 3' - | 2' - - - | 3' - 5' - | 3' - 2' - | 1' - 2' - | 3' - - - |
"""
_TITLE_B = """
| 3'~ - 5' 3'_ 2'_ | 1'~ - 6 5 | 6_ 1'_ 2'_ 3'_ 2' 1'_ 6_ | 1'~ - - - |
"""
_TITLE_ROOTS = "1 1 | 1 6 1 5 1 2 6/5 1 | 1 6 1 5 1 2 6/5 1 | 6 1 2/5 1 | 5 5"


def _gliss(key: Key, beat: float, start_deg: int, octave: int, count: int, span_beats: float,
           vel: float) -> tuple[Note, ...]:
    """古筝刮奏：沿五声音阶一口气扫上去，力度先弱后强。"""
    out = []
    for k in range(count):
        midi = float(key.penta(start_deg, octave, k))
        out.append(Note(beat=beat + span_beats * k / count, beats=1.5, midi=midi,
                        vel=vel * (0.45 + 0.55 * k / max(count - 1, 1))))
    return tuple(out)


def title() -> Song:
    key = Key.of("A4")
    m = 4
    dizi = seq(key, m, 2, _TITLE_A, _TITLE_COUNTER, _TITLE_B, where="title.dizi")
    erhu = (seq(key, m, _TITLE_A, start_bar=10, where="title.erhu")
            + penta_shift(seq(key, m, _TITLE_B, start_bar=18, where="title.erhu_b"), key, -2))
    erhu2 = transpose(erhu, 0.0)  # 第二把二胡：同谱，靠各自的种子与人性化拉开，成「一组」的厚度
    dahu = transpose(strip(seq(key, m, _TITLE_A, start_bar=10, where="title.dahu"), "~/"), -12)
    zheng = (_gliss(key, 0.0, 1, -2, 16, 1.0, 0.85)
             + arpeggio(key, m, "1 6 1 5 1 2 6/5 1 | 1 6 1 5 1 2 6/5 1", (0, 3, 5, 7, 8, 7, 5, 3), -2,
                        vel=0.52, start_bar=2)
             + _gliss(key, 18 * m, 5, -2, 16, 1.0, 0.9)
             + arpeggio(key, m, "6 1 2/5 1", (0, 3, 5, 8, 10, 8, 5, 3), -2, vel=0.6, start_bar=18))
    bass = chords(key, m, _TITLE_ROOTS, (0,), octave=-2, vel=0.55)
    pad = chords(key, m, "1 6 1 5 1 2 6/5 1 | 1 6 1 5 1 2 6/5 1 | 6 1 2/5 1", (0, 3, 5), octave=-1,
                 vel=0.42, start_bar=2)
    # 引子与过门单独一层厚和声：这两处只有锣、刮奏、滚奏，没有旋律，不托住就是一个坑。
    swell = (chords(key, m, "1 1", (0, 3, 5, 7), octave=-1, vel=0.55)
             + chords(key, m, "5 5", (0, 3, 5, 7), octave=-1, vel=0.55, start_bar=22))
    pluck_bass = bassline(key, m, "1 6 1 5 1 2 6/5 1 | 1 6 1 5 1 2 6/5 1 | 6 1 2/5 1", ((0, 2, 0), (2, 2, 3)),
                          octave=-2, vel=0.6, start_bar=2)
    gong = hits(m, "x...", 4, start_bar=0, vel=0.95) + hits(m, "x...", 4, start_bar=18, vel=0.85)
    drum = (hits(m, "........ ..rrXXXX", 16, start_bar=1, vel=0.7)
            + hits(m, "x... .... .... ....", 16, start_bar=2, repeat=8, vel=0.55)
            + hits(m, "x... .... x... ....", 16, start_bar=10, repeat=7, vel=0.65)
            + hits(m, "x... .... x.x. rrrr", 16, start_bar=17, vel=0.75)
            + hits(m, "X... x.x. X... x.x.", 16, start_bar=18, repeat=4, vel=0.85)
            + hits(m, "rrrr rrrr rrrr rrrr", 16, start_bar=22, vel=0.62)
            + hits(m, "rrrr rrrr rrrr rrXX", 16, start_bar=23, vel=0.95))
    bells = tuple(n for b in (2, 6, 10, 14, 18) for n in
                  hits(m, "x...", 4, start_bar=b, midi=57.0, vel=0.6)
                  + hits(m, "x...", 4, start_bar=b, midi=64.0, vel=0.45))
    cym = hits(m, "x...", 4, start_bar=18, vel=0.85)
    tracks = (
        Track("dizi", "dizi", dizi, level=0.0, pan=0.08, send=0.3),
        Track("erhu", "erhu", erhu, level=-2.5, pan=-0.22, send=0.32),
        Track("erhu2", "erhu", erhu2, level=-4.5, pan=0.3, send=0.35, params={"transpose_cents": 7.0}),
        Track("dahu", "dahu", dahu, level=-7.0, pan=-0.05, send=0.3),
        Track("zheng", "guzheng", zheng, level=-6.0, pan=-0.3, send=0.3, human=(6.0, 0.06)),
        Track("bass", "dahu", bass, level=-10.0, pan=0.0, send=0.25, human=(0.0, 0.0)),
        Track("pluck_bass", "daruan", pluck_bass, level=-9.0, pan=0.05, send=0.15),
        Track("sheng", "sheng", pad, level=-12.0, pan=0.2, send=0.4, human=(0.0, 0.0)),
        Track("swell", "sheng", swell, level=-4.0, pan=-0.1, send=0.45, human=(0.0, 0.0)),
        Track("daluo", "perc:daluo", gong, level=-4.0, pan=0.0, send=0.35, human=(0.0, 0.0)),
        Track("tanggu", "perc:tanggu", drum, level=-8.0, pan=-0.05, send=0.3, human=(4.0, 0.06)),
        Track("bells", "bell:bianzhong", bells, level=-11.0, pan=0.35, send=0.5, human=(4.0, 0.04)),
        Track("bo", "perc:bo", cym, level=-12.0, pan=0.25, send=0.35, human=(0.0, 0.0)),
    )
    return Song("bgm_title", "标题画面", "大气、开阔、踏上仙途", "A 宫调式（宫 = A）", 72, m, 24, "hall",
                tracks,
                "梆笛主奏、二胡齐奏（两把，A' 段接旋律）、大胡低八度、古筝刮奏与分解和弦、大阮、笙、大锣、"
                "堂鼓（段落滚奏）、编钟、铙钹",
                "引子 2（大锣 + 古筝刮奏）+ A 8（笛）+ A' 8（二胡齐奏，笛吹高声部）+ 高潮 4 + 过门 2；"
                "过门的大鼓滚奏渐强，直接滚回曲首的大锣")


# ============================================================== 普通战斗：急促（A 羽，♩=150）

_BATTLE_A = """
| 6> 6_ 1'_ 2' 1'_ 6_ | 5_ 6_ 5_ 3_ 2 3 | 6> 6_ 1'_ 2'_ 3'_ 2'_ 1'_ | 6~ - - 0 |
| /3'> 3'_ 2'_ 1' 2'_ 3'_ | 5's 3'_ 2'_ 1' 6 | 1'_ 6_ 5_ 3_ 5_ 6_ 1'_ 2'_ | 6~ - - 0 |
"""
_BATTLE_B = """
| 3' - 5'_ 3'_ 2'_ 3'_ | 6'~ - 5' 3' | 2'_ 3'_ 2'_ 1'_ 6 1' | 2'~ - - - |
| 1' - 2'_ 1'_ 6_ 1'_ | 5~ - 6 1' | 2'_ 1'_ 6_ 5_ 3_ 5_ 6_ 1'_ | 6~ - - - |
"""
_BATTLE_B_ERHU = """
| 6 - - - | 1' - - - | 2' - 1' - | 6 - - - | 3' - - - | 2' - 1' - | 6 - 5 - | 6 - - - |
"""
_BATTLE_C_ERHU = """
| 6~ - - - | 1's - - - | 2's - - - | 3's - 5's - | 6'~ - - - | 5's - - - | 3's - 2's - | 3'~ - - - |
"""
_BATTLE_C_PIPA = """
| 6* - 6* - | 1'* - 1'* - | 2'* - 2'* - | 3'* - 5'* - | 6'* - 6'* - | 5'* - 5'* - | 3'* - 2'* - | 3'* - - - |
"""
_BATTLE_ROOTS_A = "6 5 6 6 1 2 5 6"
_BATTLE_ROOTS_B = "6 1 2 5 6 1 2/5 6"
_BATTLE_ROOTS_C = "6 1 2 2 6 5 2 3"


def battle() -> Song:
    key = Key.of("C4")
    m = 4
    erhu = seq(key, m, 4, _BATTLE_A, _BATTLE_A, _BATTLE_B_ERHU, _BATTLE_C_ERHU, _BATTLE_A,
               where="battle.erhu")
    # 笛子：A' 后半（第 17–20 小节）高八度叠上去，B 段主奏，A'' 全段高八度。
    dizi = (only_bars(strip(seq(key, m, _BATTLE_A, start_bar=12, octave=1, where="battle.dizi_a"), ">"),
                      m, 16, 20)
            + seq(key, m, _BATTLE_B, start_bar=20, octave=1, where="battle.dizi_b")
            + strip(seq(key, m, _BATTLE_A, start_bar=36, octave=1, where="battle.dizi_a2"), ">"))
    pipa_c = seq(key, m, _BATTLE_C_PIPA, start_bar=28, octave=-1, where="battle.pipa_c")
    roots_drive = " ".join(["6 6 6 6", _BATTLE_ROOTS_A, _BATTLE_ROOTS_A, _BATTLE_ROOTS_B])
    riff = (0, 0, 3, 0, 2, 0, 4, 3)
    ostinato = (arpeggio(key, m, roots_drive, riff, -1, vel=0.62, accent=0.25, orn="!")
                + arpeggio(key, m, " ".join([_BATTLE_ROOTS_A, "6 6 6 6"]), riff, -1, vel=0.62,
                           accent=0.25, orn="!", start_bar=36))
    roots_all = " ".join([roots_drive, _BATTLE_ROOTS_C, _BATTLE_ROOTS_A, "6 6 6 6"])
    drive = ((0, .5, 0), (.5, .5, 0), (1, .5, 3), (1.5, .5, 0), (2, .5, 0), (2.5, .5, 0), (3, .5, 2),
             (3.5, .5, 3))
    bass = fold(bassline(key, m, roots_all, drive, octave=-2, vel=0.66, orn="!"), 38.0)
    groove = "X..x..x.X.x...x."
    fill = "X.x.X.x.rrrrXXXX"

    def section(start: int, pattern: str, bars: int = 8) -> tuple:
        return (hits(m, pattern, 16, start_bar=start, repeat=bars - 1, vel=0.85)
                + hits(m, fill, 16, start_bar=start + bars - 1, vel=0.9))

    drum = (section(0, groove, 4) + section(4, groove) + section(12, groove)
            + section(20, "X...x.x.X...x.xx")
            + hits(m, "X.......X.......", 16, start_bar=28, repeat=2, vel=0.9)
            + hits(m, "X...X...X...X...", 16, start_bar=30, repeat=2, vel=0.85)
            + hits(m, "X.X.X.X.X.X.X.X.", 16, start_bar=32, repeat=2, vel=0.85)
            + hits(m, "XgXgXgXgXgXgXgXg", 16, start_bar=34, vel=0.85)
            + hits(m, "rrrrrrrrrrrrXXXX", 16, start_bar=35, vel=0.9)
            + section(36, groove) + section(44, groove, 4))
    back = tuple(n for b in list(range(0, 28)) + list(range(36, 48))
                 for n in hits(m, ".... x... .... x...", 16, start_bar=b, vel=0.6))
    crash = tuple(n for b in (4, 12, 20, 36) for n in hits(m, "x...", 4, start_bar=b, vel=0.9))
    choke = hits(m, "x...", 4, start_bar=28, repeat=8, vel=0.8, orn="!")
    tag = tuple(n for b in (7, 15, 27, 43) for n in hits(m, "..............x.", 16, start_bar=b, vel=0.75))
    boom = tuple(n for b in (20, 28, 36) for n in hits(m, "x...", 4, start_bar=b, vel=0.85))
    drone = chords(key, m, roots_all, (0,), octave=-2, vel=0.45)
    tracks = (
        Track("erhu", "erhu", erhu, level=0.0, pan=0.05, send=0.2, human=(5.0, 0.05)),
        Track("dizi", "dizi", dizi, level=-1.5, pan=0.18, send=0.22, human=(5.0, 0.05)),
        Track("pipa_c", "pipa", pipa_c, level=-2.0, pan=-0.15, send=0.2, human=(4.0, 0.05)),
        Track("ostinato", "pipa", ostinato, level=-7.0, pan=-0.3, send=0.12, human=(3.0, 0.07)),
        Track("bass", "daruan", bass, level=-7.0, pan=0.0, send=0.08, human=(3.0, 0.05)),
        Track("dahu", "dahu", drone, level=-13.0, pan=0.1, send=0.2, human=(0.0, 0.0)),
        Track("tanggu", "perc:tanggu", drum, level=-4.5, pan=0.0, send=0.15, human=(3.0, 0.07)),
        Track("bangu", "perc:bangu", back, level=-13.0, pan=-0.25, send=0.12, human=(3.0, 0.07)),
        Track("bo", "perc:bo", crash + choke, level=-12.0, pan=0.25, send=0.2, human=(2.0, 0.04)),
        Track("xiaoluo", "perc:xiaoluo", tag, level=-14.0, pan=0.3, send=0.25, human=(2.0, 0.04)),
        Track("daluo", "perc:daluo", boom, level=-10.0, pan=-0.1, send=0.3, human=(0.0, 0.0)),
    )
    return Song("bgm_battle", "普通战斗", "急促、紧逼、刀光", "A 羽调式（宫 = C）", 150, m, 48, "plate",
                tracks,
                "二胡主奏（重音、滑音）、梆笛（A' 后半与 A'' 高八度、B 段主奏）、琵琶顿音固定音型与 C 段轮指、"
                "大阮八分音符低音、大胡、堂鼓、板鼓、铙钹、小锣、大锣",
                "前奏 4 + A 8 + A' 8 + B 8 + C 8（拆掉旋律，鼓点从疏到密层层推高）+ A'' 8 + 尾奏 4（接回前奏）")


# ============================================================== 首领战：更重（D 羽，♩=126）

_BOSS_A = """
| 6~ - - 5_ 6_ | 1's - 6 5 | 3~ - 2_ 3_ 5 | 6 - b7s 6s |
| 1'~ - 2' 3' | 2'_ 1'_ 6_ 5_ 3 5 | 6~ - 5_ 3_ 2_ 3_ | 6,~ - - - |
"""
_BOSS_B = """
| 3'~ - 5'_ 3'_ 2' | 1' - 6 b7s | 6~ - 5_ 3_ 2_ 3_ | 6~ - - - |
| 1'~ - 2'_ 1'_ 6 | 5 - b7s 6s | 1'_ 6_ 5_ 3_ 2 3 | 6~ - - - |
"""
_BOSS_B_ERHU = """
| 6, - - - | 6, - b7,s - | 6, - - - | 6, - 1s - | 6, - - - | 1 - b7,s - | 6, - - - | 6, - - - |
"""
_BOSS_C_ERHU = """
| 6,~ - - - | b7,s - - - | 6,s - - - | 1s - 2 - | 3~ - - - | 5s - - - | 6s - b7s - | 6s - - - |
"""
_BOSS_RIFF = "| 6,_ 6,_ 6,_ b7,_ 6,_ 6,_ 1_ 6,_ |"


def boss() -> Song:
    key = Key.of("F4")
    m = 4
    erhu = seq(key, m, 4, _BOSS_A, _BOSS_A, _BOSS_B_ERHU, _BOSS_C_ERHU, _BOSS_A, where="boss.erhu")
    dizi = seq(key, m, _BOSS_B, start_bar=20, where="boss.dizi")
    dahu_line = transpose(strip(seq(key, m, 12, _BOSS_A, 16, _BOSS_A, where="boss.dahu"), "~"), -12)
    riff_bars = [_BOSS_RIFF] * 28 + ["| 6, - - - |"] * 8 + [_BOSS_RIFF] * 8
    riff = seq(key, m, " ".join(riff_bars), octave=-2, where="boss.riff")
    riff = tuple(n if n.beats > 1 else replace(n, orn="!") for n in riff)
    stabs: list = []
    for b in list(range(4, 20)) + list(range(36, 44)):
        stabs.extend(strum(key, b * m, [(6, -1), (3, 0), (6, 0)], vel=0.85, beats=0.5))
        stabs.extend(strum(key, b * m + 2.5, [(6, -1), (3, 0), (6, 0)], vel=0.7, beats=0.5, up=True))
    # 首领的堂鼓调到约 60 Hz（midi 58 ≈ 0.89 倍标准音高）：再低就只剩耳机里的一团轰鸣，
    # 笔记本喇叭根本放不出来，还白白吃掉峰值余量。
    heavy = "X..X..X.X.X.X..."
    fill = "X.X.X.X.rrrrXXXX"

    def section(start: int, pattern: str, bars: int = 8) -> tuple:
        return (hits(m, pattern, 16, start_bar=start, repeat=bars - 1, midi=58.0, vel=0.9)
                + hits(m, fill, 16, start_bar=start + bars - 1, midi=58.0, vel=0.9))

    drum = (section(0, "X.....X.X.......", 4) + section(4, heavy) + section(12, heavy)
            + section(20, "X...X...X...X.X.")
            + hits(m, "X...............", 16, start_bar=28, repeat=2, midi=58.0, vel=0.9)
            + hits(m, "X.......X.......", 16, start_bar=30, repeat=2, midi=58.0, vel=0.9)
            + hits(m, "X...X...X...X...", 16, start_bar=32, repeat=2, midi=58.0, vel=0.9)
            + hits(m, "X.X.X.X.X.X.X.X.", 16, start_bar=34, midi=58.0, vel=0.9)
            + hits(m, "rrrrrrrrrrrrXXXX", 16, start_bar=35, midi=58.0, vel=0.95)
            + section(36, heavy))
    tam = tuple(n for b in (0, 20, 28, 36) for n in hits(m, "x...", 4, start_bar=b, midi=57.0, vel=0.9))
    crash = tuple(n for b in (4, 12, 36) for n in hits(m, "x...", 4, start_bar=b, vel=0.9))
    choke = hits(m, "x...", 4, start_bar=28, repeat=8, vel=0.75, orn="!")
    cluster = tuple(n for b in (12, 28) for n in hits(m, "x...", 4, start_bar=b, midi=50.0, vel=0.6)
                    + hits(m, "x...", 4, start_bar=b, midi=51.0, vel=0.5))
    drone = chords(key, m, " ".join(["6"] * 44), (0,), octave=-3, vel=0.5)
    tracks = (
        Track("erhu", "erhu", erhu, level=0.0, pan=0.05, send=0.22, human=(5.0, 0.05)),
        Track("dizi", "dizi", dizi, level=-1.0, pan=0.15, send=0.25, human=(5.0, 0.05)),
        Track("dahu_line", "dahu", dahu_line, level=-6.0, pan=-0.1, send=0.25, human=(5.0, 0.05)),
        Track("riff", "daruan", riff, level=-5.5, pan=-0.05, send=0.1, human=(3.0, 0.05)),
        Track("stabs", "pipa", tuple(stabs), level=-8.0, pan=-0.3, send=0.18, human=(2.0, 0.05)),
        Track("drone", "dahu", drone, level=-12.0, pan=0.0, send=0.2, human=(0.0, 0.0)),
        Track("tanggu", "perc:tanggu", drum, level=-4.5, pan=0.0, send=0.18, human=(3.0, 0.06)),
        Track("tamtam", "perc:tamtam", tam, level=-7.5, pan=0.0, send=0.35, human=(0.0, 0.0)),
        Track("bo", "perc:bo", crash + choke, level=-12.0, pan=0.25, send=0.22, human=(2.0, 0.04)),
        Track("bells", "bell:bianzhong", cluster, level=-12.0, pan=-0.3, send=0.45, human=(0.0, 0.0)),
    )
    return Song("bgm_boss", "首领战", "沉重、压迫、生死一线", "D 羽调式（宫 = F，低音加 ♭E 半音）", 126, m, 44,
                "plate", tracks,
                "二胡主奏（半音滑奏）、梆笛（B 段）、大胡低八度、大阮「羽—♭E—羽」的半音固定音型、琵琶扫弦重音、"
                "低音堂鼓、抄锣（开段）、铙钹、编钟低音小二度",
                "前奏 4 + A 8 + A' 8 + B 8 + C 8（只剩低音、鼓与二胡半音爬升）+ A'' 8")


# ============================================================== 识海吞噬战：诡谲（E 羽，♩=112）

_SOUL_A = """
| 6~ - - 5_ 3_ | 5s - 3 2 | 3~ - 2_ 1_ 6,_ 1_ | 2s - - - |
| 3~ - 5 6 | b7s - 6 5 | 3_ 5_ 6_ 1'_ 7s 6 | 6~ - - - |
"""
_SOUL_B = """
| 3' - 2' 6 | 1' - 6 5 | 3 - 5 6 | 6 - - - |
| 3' - 5' 3' | 2' - 1' 6 | 5 - 3 2 | 3 - - - |
"""
_SOUL_B_XIAO = """
| 6 - - - | 5 - - - | 3 - - - | 6 - - - | 3' - - - | 2' - - - | 5 - - - | 3 - - - |
"""
_SOUL_C = """
| 6,s - 6s - | 3's - 6,s - | 1's - #4s - | 3s - - - | 6,s - 3's - | 2's - b7s - | 6s - #4s - | 6~ - - - |
"""


def soulsea() -> Song:
    key = Key.of("G4")
    m = 4
    erhu = seq(key, m, _SOUL_A, 8, _SOUL_A, _SOUL_C, _SOUL_A, where="soul.erhu")
    rev = (seq(key, m, _SOUL_B, start_bar=8, where="soul.rev")
           + transpose(strip(seq(key, m, _SOUL_A, start_bar=16, where="soul.rev2"), "~s"), -12)
           + seq(key, m, _SOUL_B, start_bar=32, where="soul.rev3"))
    xiao = seq(key, m, _SOUL_B_XIAO, start_bar=8, where="soul.xiao")
    # C 段的三对四：附点八分一个音、三个音一轮，与 4/4 的强拍错开，十二拍才重合一次。
    poly = tuple(Note(beat=24 * m + k * 0.75, beats=0.5, midi=float(key.midi((6, 1, 3)[k % 3], (-1, 0, 0)[k % 3])),
                      vel=0.55 + 0.15 * ((k % 4) == 0), orn="!")
                 for k in range(int(8 * m / 0.75)))
    heart = hits(m, "xg.. xg.. xg.. xg..", 16, repeat=40, midi=60.0, vel=0.7)
    pulse = hits(m, "x.x. x.x. x.x. x.x.", 16, repeat=40, midi=55.0, vel=0.45)
    drone_a = chords(key, m, " ".join(["6"] * 40), (0,), octave=-1, vel=0.45)
    bells = tuple(n for b in (0, 16, 32) for n in hits(m, "x...", 4, start_bar=b, midi=64.0, vel=0.6)
                  + hits(m, "x...", 4, start_bar=b, midi=65.0, vel=0.55))
    tracks = (
        Track("erhu", "erhu", erhu, level=0.0, pan=0.05, send=0.3, echo=0.28, drive=2.6,
              human=(8.0, 0.05)),
        Track("rev_zheng", "guzheng", rev, level=-3.0, pan=-0.2, send=0.4, echo=0.4,
              params={"reverse": True}, human=(6.0, 0.06)),
        Track("xiao", "xiao", xiao, level=-5.0, pan=0.3, send=0.5, echo=0.45, human=(10.0, 0.05)),
        Track("poly", "pipa", poly, level=-6.0, pan=-0.35, send=0.3, echo=0.3, human=(2.0, 0.05)),
        Track("sheng_hi", "sheng", drone_a, level=-14.0, pan=-0.4, send=0.5, human=(0.0, 0.0),
              params={"transpose_cents": 24.0}),
        Track("sheng_lo", "sheng", drone_a, level=-14.0, pan=0.4, send=0.5, human=(0.0, 0.0),
              params={"transpose_cents": -24.0}),
        Track("dahu", "dahu", transpose(drone_a, -24), level=-11.0, pan=0.0, send=0.3, human=(0.0, 0.0)),
        Track("heart", "perc:heartbeat", heart, level=-9.0, pan=0.0, send=0.15, human=(2.0, 0.03)),
        Track("pulse", "perc:tanggu", pulse, level=-12.0, pan=0.0, send=0.2, human=(3.0, 0.06),
              params={"eq": (("lp", 900.0),)}),
        Track("bells", "bell:bianzhong", bells, level=-11.0, pan=-0.2, send=0.6, echo=0.3,
              human=(0.0, 0.0)),
        Track("whisper", "amb:whisper", level=-19.0, send=0.3),
    )
    return Song("bgm_soulsea", "识海吞噬战", "诡谲、失真、意识被撕扯", "E 羽调式（宫 = G，间用 ♭7、#4）", 112, m, 40,
                "abyss", tracks,
                "失真二胡主奏、倒放古筝（每个音「吸」进拍点）、箫（长音 + 回声）、C 段三对四的琵琶复节奏、"
                "左右相差 ±24 音分的两层笙（拍频「晃」）、大胡、心跳鼓、闷鼓、编钟小二度、识海低语",
                "A 8 + B 8（倒放古筝）+ A' 8 + C 8（大跨度滑奏 + 复节奏）+ A'' 8；全曲走附点八分的乒乓回声",
                echo=(0.75, 0.5, 6, 3000.0))


# ============================================================== 夜探 / 潜入：压低（E 羽，♩=84）

_NIGHT_PLUCK_A = """
| 6,! 0 6,! 0 | 3! 0 0 2! | 1! 0 6,! 0 | 0 0 0 0 |
| 6,! 0 6,! 0 | 5! 0 0 3! | 2! 0 1! 0 | 6,! 0 0 0 |
"""
_NIGHT_XIAO = """
| 6~ - - - | 5_ 3_ 5_ 6_ 3 - | 2 - 3 5_ 3_ | 6,~ - - - |
| 6~ - - 1'_ 6_ | 5 - 3 - | 2_ 3_ 2_ 1_ 6, - | 6,~ - - - |
"""
_NIGHT_PLUCK_B = """
| 6,_! 0_ 6,_! 0_ 3_! 0_ 2_! 0_ | 1_! 0_ 6,_! 0_ 0 0 | 6,_! 0_ 6,_! 0_ 5_! 0_ 3_! 0_ | 2_! 0_ 1_! 0_ 0 6,! |
| 6,_! 0_ 6,_! 0_ 3_! 0_ 2_! 0_ | 1_! 0_ 6,_! 0_ 0 0 | 6,_! 0_ 6,_! 0_ 5_! 0_ 6_! 0_ | 5_! 0_ 3_! 0_ 2! 0 |
"""
_NIGHT_ERHU_B = """
| 6~ - - - | 5s - 6s - | 3~ - - - | 2s - 3s - | 6~ - - - | 1's - 6s - | 5~ - 3s - | 6~ - - - |
"""
_NIGHT_CODA = """
| 6,! 0 0 0 | 0 0 6,! 0 | 0 0 0 0 | 0 0 0 0 |
"""


def night() -> Song:
    key = Key.of("G3")
    m = 4
    pipa = seq(key, m, _NIGHT_PLUCK_A, _NIGHT_PLUCK_A, _NIGHT_PLUCK_B, _NIGHT_CODA, where="night.pipa")
    qin = transpose(seq(key, m, _NIGHT_PLUCK_A, start_bar=8, where="night.qin"), -12)
    xiao = seq(key, m, _NIGHT_XIAO, start_bar=8, octave=1, where="night.xiao")
    erhu = seq(key, m, _NIGHT_ERHU_B, start_bar=16, octave=1, where="night.erhu")
    heart = (hits(m, "xg.. .... xg.. ....", 16, repeat=16, vel=0.7)
             + hits(m, "xg.. xg.. xg.. xg..", 16, start_bar=16, repeat=8, vel=0.75)
             + hits(m, "xg.. .... xg.. ....", 16, start_bar=24, repeat=4, vel=0.65))
    tiptoe = hits(m, "..x. .... ..x. ....", 16, start_bar=8, repeat=8, vel=0.4)
    drone = chords(key, m, " ".join(["6"] * 28), (0,), octave=-1, vel=0.45)
    tracks = (
        Track("pipa", "pipa", pipa, level=0.0, pan=-0.1, send=0.3, human=(7.0, 0.07)),
        Track("qin", "guqin", qin, level=-5.0, pan=-0.2, send=0.3, human=(7.0, 0.06)),
        Track("xiao", "xiao", xiao, level=-1.0, pan=0.2, send=0.4, human=(10.0, 0.05)),
        Track("erhu", "erhu", erhu, level=-1.5, pan=0.15, send=0.35, human=(8.0, 0.05)),
        Track("drone", "dahu", drone, level=-14.0, pan=0.0, send=0.3, human=(0.0, 0.0)),
        Track("heart", "perc:heartbeat", heart, level=-10.0, pan=0.0, send=0.12, human=(2.0, 0.03)),
        Track("tiptoe", "perc:muyu", tiptoe, level=-21.0, pan=0.35, send=0.2, human=(8.0, 0.1),
              params={"eq": (("lp", 3000.0),)}),
        Track("crickets", "amb:crickets", level=-23.0, send=0.25),
    )
    return Song("bgm_night", "夜探、潜入（剧情点播）", "紧张、压低呼吸、夜色", "E 羽调式（宫 = G）", 84, m, 28, "open",
                tracks,
                "琵琶顿音（稀疏拨弦）、古琴低八度重叠、箫（A' 段）、二胡（B 段，高而细的长音与滑音）、"
                "大胡极轻的持续音、心跳鼓（A 段两拍一跳，B 段每拍一跳）、木鱼「踮脚」、虫鸣",
                "A 8 + A' 8（箫进来）+ B 8（拨弦变成八分音符，心跳加快）+ 尾声 4（几乎只剩心跳）")


SONGS = {
    "bgm_title": title,
    "bgm_battle": battle,
    "bgm_boss": boss,
    "bgm_soulsea": soulsea,
    "bgm_night": night,
}
