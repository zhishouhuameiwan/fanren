# -*- coding: utf-8 -*-
"""宗门与室内的曲子：七玄门山门、药谷、居所、宗门堂舍、暗道、墨府。

写法约定同 songs_world.py。这几首大多慢、疏、空间大，靠三样东西撑住「不空」：
  - 长音垫底（笙、大胡的空五度），但压得很低，只是让房间里有「气」；
  - 古琴、古筝的余韵本来就长，音与音之间让它响着；
  - 钟、磬放在段落的第一拍，给结构一个可以听见的路标。
"""
from __future__ import annotations

from mixer import Song, Track
from score import (Key, arpeggio, bassline, chords, fold, hits, on_beats, penta_shift, scale_vel,
                   seq, strip, transpose)

# ============================================================== 七玄门：宗门肃穆（F 宫，♩=66）

_QXM_A = """
| 1' - 6 5 | 6~ - 1' 2' | 3' 2' 1' 6 | 5~ - - - |
| 6 - 1' 6 | 5 3 2 3 | 5~ - 3 2 | 1~ - - - |
"""
_QXM_B = """
| 5 6_ 1'_ 2' 1' | 6_ 5_ 3_ 5_ 6~ - | 1' 2'_ 3'_ 5' 3' | 2'~ - - - |
| 3'_ 2'_ 1'_ 6_ 5 6 | 1' 6_ 5_ 3 5 | 6_ 5_ 3_ 2_ 1 2_ 3_ | 5~ - - - |
"""
_QXM_C = """
| 1' - 6 5 | 6~ - 1' 2' | 3' 2' 1' 6 | 5 6 1' 2' | 3' 2' 1' 6_ 5_ | 1'~ - - - |
"""
_QXM_ROOTS = "1 6 1 5 6 5 2 1 | 1 6 1 2 6 1 2 5 | 1 6 1 5 6 1"


def qixuanmen() -> Song:
    key = Key.of("F4")
    m = 4
    qin = seq(key, m, _QXM_A, 8, _QXM_C, octave=-1, where="qxm.qin")
    xiao = seq(key, m, _QXM_B, _QXM_C, start_bar=8, where="qxm.xiao")
    bells = on_beats(seq(key, m, _QXM_A, 8, _QXM_C, octave=-1, where="qxm.bells"), m, (0.0, 2.0))
    pad = fold(chords(key, m, _QXM_ROOTS, (0, 3, 5), octave=-1, vel=0.42), 55.0)
    bass = chords(key, m, _QXM_ROOTS, (0,), octave=-2, vel=0.5)
    drum = (hits(m, "x... .... .... ....", 16, repeat=15, midi=55.0, vel=0.5)
            + hits(m, "x... .... ..x. rrrr", 16, start_bar=15, midi=55.0, vel=0.6)
            + hits(m, "X... .... .... ....", 16, start_bar=16, repeat=6, midi=55.0, vel=0.6))
    chant = hits(m, "x... x... x... x...", 16, start_bar=8, repeat=8, vel=0.4)
    qing = tuple(n for b in (0, 8, 16) for n in hits(m, "x...", 4, start_bar=b, midi=65.0, vel=0.55))
    tracks = (
        Track("qin", "guqin", qin, level=0.0, pan=-0.12, send=0.35, human=(12.0, 0.06)),
        Track("xiao", "xiao", xiao, level=-0.5, pan=0.12, send=0.4, human=(10.0, 0.06)),
        Track("bells", "bell:bianzhong", bells, level=-6.0, pan=0.3, send=0.5, human=(6.0, 0.05)),
        Track("sheng", "sheng", pad, level=-11.0, pan=0.18, send=0.45, human=(0.0, 0.0)),
        Track("dahu", "dahu", bass, level=-10.0, pan=-0.05, send=0.3, human=(0.0, 0.0)),
        Track("tanggu", "perc:tanggu", drum, level=-11.0, pan=0.0, send=0.35, human=(5.0, 0.05)),
        Track("muyu", "perc:muyu", chant, level=-19.0, pan=-0.35, send=0.3, human=(5.0, 0.05)),
        Track("qing", "bell:qing", qing, level=-13.0, pan=0.4, send=0.5, human=(0.0, 0.0)),
    )
    return Song("bgm_qixuanmen", "七玄门山门", "肃穆、庄重、山门高远", "F 宫调式（宫 = F）", 66, m, 22,
                "temple", tracks,
                "古琴主奏、编钟（敲骨干音）、箫（B 段）、笙的空五度和声、大胡、堂鼓（每小节一下，段落前滚奏）、"
                "木鱼（B 段诵经般的慢拍）、磬",
                "A 8 + B 8 + A'' 6（合头：前三句与 A 相同，末两句扬起再落回宫音）")


# ============================================================== 药谷：幽静（B 角，♩=72）

_VALLEY_A = """
| 3~ - 5_ 6_ 5_ 3_ | 2 - 3 - | 5 6_ 1'_ 6 5_ 3_ | 3~ - - - |
| 6 - 1'_ 2'_ 1'_ 6_ | 5~ - 6 3 | 2_ 3_ 5_ 6_ 5 3_ 2_ | 3~ - - - |
"""
_VALLEY_B = """
| 3'o - 2'_ 3'_ 5'o | 3' - 2' 1'_ 6_ | 5 - 6_ 1'_ 2' | 3'* - - - |
| 1' - 6_ 5_ 3 | 5 - 3 2_ 1_ | 2 - 3 5_ 6_ | 3* - - - |
"""
_VALLEY_B_XIAO = """
| 3 - - - | 6 - 5 - | 3 - - - | 2 - 3 - | 1' - - - | 6 - 5 - | 3 - 2 - | 3 - - - |
"""
_VALLEY_A2 = """
| 3~ - 5_ 6_ 5_ 3_ | 2 - 3 - | 5 6_ 1'_ 6 5_ 3_ | 3~ - - - |
| 6 - 1'_ 2'_ 3' | 2'_ 1'_ 6_ 5_ 3 - | 2_ 3_ 5_ 6_ 5 3_ 2_ | 3~ - - - |
"""
_VALLEY_ROOTS = "6 5 1 6 2 5 5 6 | 6 1 5 6 1 5 2 6 | 6 5 1 6 2 5 5 6"


def valley() -> Song:
    key = Key.of("G4")
    m = 4
    xiao = seq(key, m, _VALLEY_A, _VALLEY_B_XIAO, _VALLEY_A2, where="valley.xiao")
    # B 段古筝是主奏，A' 的古筝只是低八度重叠：分两轨各自配平。合在一轨时，量出来的响度
    # 被重叠段主导，B 段的泛音旋律（泛音本来就轻）会整段沉下去。
    zheng_b = seq(key, m, _VALLEY_B, start_bar=8, where="valley.zheng")
    zheng_double = transpose(strip(seq(key, m, _VALLEY_A2, start_bar=16, where="valley.zheng2"), "~"), -12)
    shimmer = arpeggio(key, m, _VALLEY_ROOTS, (0, 3, 5, 8), -2, vel=0.4, beats=2.0)
    qin = arpeggio(key, m, _VALLEY_ROOTS, (0, None, None, None), -2, vel=0.5, beats=4.0)
    bells = hits(m, ".... .... x... .... | .... .... .... .... | .... .... .... .... | "
                    ".... .... .... ....", 16, repeat=6, midi=96.0, vel=0.4)
    tracks = (
        Track("xiao", "xiao", xiao, level=0.0, pan=0.1, send=0.38, human=(12.0, 0.06)),
        Track("zheng_b", "guzheng", zheng_b, level=0.0, pan=-0.1, send=0.34, human=(10.0, 0.06)),
        Track("zheng_double", "guzheng", zheng_double, level=-6.0, pan=-0.2, send=0.3, human=(10.0, 0.06)),
        Track("shimmer", "guzheng", shimmer, level=-9.5, pan=-0.3, send=0.35, human=(8.0, 0.08)),
        Track("qin", "guqin", qin, level=-9.0, pan=0.05, send=0.3, human=(10.0, 0.05)),
        Track("bells", "bell:pengling", bells, level=-19.0, pan=0.45, send=0.5),
        Track("water", "amb:stream", level=-21.0, send=0.1),
        Track("birds", "amb:birds", level=-25.0, send=0.35, params={"per_minute": 6.0}),
    )
    return Song("bgm_valley", "神手谷、药圃", "幽静、清润、草木与溪声", "B 角调式（宫 = G）", 72, m, 24, "valley",
                tracks,
                "箫主奏、古筝（B 段主奏，泛音与摇指）、古筝泛音型伴奏、古琴低音、碰铃、溪水、鸟鸣",
                "A 8 + B 8（古筝奏旋律，泛音与摇指交替，箫退为长音）+ A' 8；三段都落在角音上——角调式少见，"
                "不落宫也不落羽，那种「悬着」的感觉正合药谷的清幽")


# ============================================================== 室内：清寂（D 羽，♩=63）

_INDOOR_A = """
| 6~ - 1' 2'_ 1'_ | 6s - 5 3 | 5 6_ 5_ 3 2_ 3_ | 6,~ - - - |
| 3 - 5 6_ 1'_ | 2's - 1' 6_ 5_ | 3_ 5_ 6_ 1'_ 6 5 | 6~ - - - |
"""
_INDOOR_B_HARM = """
| 6'o - 5'o 3'_o 2'_o | 3'o - - - | 2'o 1'_o 6_o 5o 6o | 3o - - - |
"""
_INDOOR_B_XIAO = """
| 5 - 6 1'_ 2'_ | 3' - 2' 1' | 6 5_ 3_ 2 3_ 5_ | 6~ - - - |
"""
_INDOOR_CODA = """
| 1' - 6 5 | 3 - 2 3_ 5_ | 6~ - - - | - - - - |
"""
_INDOOR_ROOTS = "6 1 2 6 6 5 1 6 | 6 6 2 6 5 1 6 6 | 1 2 6 6"


def indoor() -> Song:
    key = Key.of("F3")
    m = 4
    qin = seq(key, m, _INDOOR_A, _INDOOR_B_HARM, 4, _INDOOR_CODA, where="indoor.qin")
    xiao = seq(key, m, _INDOOR_B_XIAO, start_bar=12, octave=1, where="indoor.xiao")
    low = arpeggio(key, m, _INDOOR_ROOTS, (0, None, None, None), -1, vel=0.42, beats=4.0)
    pad = fold(chords(key, m, "6 6 2 6 5 1 6 6", (0, 3), octave=0, vel=0.35, start_bar=8), 55.0)
    qing = tuple(n for b in (0, 8, 16) for n in hits(m, "x...", 4, start_bar=b, midi=62.0, vel=0.45))
    tracks = (
        Track("qin", "guqin", qin, level=0.0, pan=-0.05, send=0.3, human=(16.0, 0.07)),
        Track("xiao", "xiao", xiao, level=-2.0, pan=0.25, send=0.45, human=(12.0, 0.05)),
        Track("low", "guqin", low, level=-8.0, pan=-0.15, send=0.3, human=(10.0, 0.05)),
        Track("sheng", "sheng", pad, level=-16.0, pan=0.2, send=0.5, human=(0.0, 0.0)),
        Track("qing", "bell:qing", qing, level=-12.0, pan=0.35, send=0.45, human=(0.0, 0.0)),
    )
    return Song("bgm_indoor", "居所、藏书处、密室", "清寂、独处、灯下", "D 羽调式（宫 = F，古琴正调）", 63, m, 20,
                "room", tracks,
                "古琴独奏（走手音、泛音段）、箫（B 段后半）、古琴低音、极轻的笙、磬",
                "A 8 + B 8（泛音四小节 + 箫四小节）+ 尾声 4；尾声最后一音让它自己响完，"
                "余韵绕回曲首")


# ============================================================== 宗门堂舍：外刃堂、阁堂、演武场（D 商，♩=92）

_SECT_A = """
| 2.> 3_ 5 3_ 2_ | 1_ 2_ 3_ 5_ 2* - | 5.> 6_ 1' 6_ 5_ | 3_ 5_ 6_ 5_ 3* - |
| 2.> 3_ 5 6_ 1'_ | 2'_ 1'_ 6_ 5_ 3* - | 5_ 3_ 2_ 1_ 6,_ 1_ 2_ 3_ | 2* - - - |
"""
_SECT_B = """
| 5~ - 6_ 5_ 3_ 5_ | 6~ - 1' 2' | 3'_ 2'_ 1'_ 2'_ 3' 5'_ 3'_ | 2'~ - - - |
| 1'~ - 2'_ 1'_ 6_ 1'_ | 5~ - 6 3 | 2_ 3_ 5_ 6_ 5_ 3_ 2_ 3_ | 2~ - - - |
"""
_SECT_ROOTS_A = "2 5 1 6 2 6 5 2"
_SECT_ROOTS_B = "1 6 1 5 6 1 5 2"


def sect() -> Song:
    key = Key.of("C4")
    m = 4
    pipa = seq(key, m, _SECT_A, _SECT_A, 8, _SECT_A, where="sect.pipa")
    pipa_b = arpeggio(key, m, _SECT_ROOTS_B, (0, None, 3, None), -1, vel=0.45, start_bar=16, beats=2.0)
    dizi = strip(seq(key, m, 8, _SECT_A, 8, _SECT_A, octave=1, where="sect.dizi"), "*>")
    erhu = (seq(key, m, _SECT_B, start_bar=16, where="sect.erhu")
            + scale_vel(penta_shift(strip(seq(key, m, _SECT_A, start_bar=24, where="sect.erhu2"), "*>"),
                                    key, 2), 0.75))
    roots = " ".join([_SECT_ROOTS_A, _SECT_ROOTS_A, _SECT_ROOTS_B, _SECT_ROOTS_A])
    pad = fold(chords(key, m, roots, (0, 3, 5), octave=-1, vel=0.4), 55.0)
    bass = bassline(key, m, roots, ((0, 1.5, 0), (1.5, 0.5, 0), (2, 1, 3), (3, 1, 0)), octave=-1,
                    vel=0.62)
    march = "X...x.x.X...x..."
    fill = "X.x.X.x.rrrrXXXX"
    drum = (hits(m, march, 16, repeat=7) + hits(m, fill, 16, start_bar=7)
            + hits(m, march, 16, start_bar=8, repeat=7) + hits(m, fill, 16, start_bar=15)
            + hits(m, "X.......X.x.....", 16, start_bar=16, repeat=7, vel=0.9)
            + hits(m, fill, 16, start_bar=23)
            + hits(m, march, 16, start_bar=24, repeat=7) + hits(m, fill, 16, start_bar=31, vel=0.85))
    back = hits(m, ".... x... .... x...", 16, repeat=32, vel=0.5)
    luo = tuple(n for b in (0, 8, 16, 24) for n in hits(m, "x...", 4, start_bar=b, vel=0.75))
    cym = tuple(n for b in (16, 24) for n in hits(m, "x...", 4, start_bar=b, vel=0.8))
    tracks = (
        Track("pipa", "pipa", pipa, level=0.0, pan=-0.08, send=0.2),
        Track("dizi", "dizi", dizi, level=-3.0, pan=0.15, send=0.22),
        Track("erhu", "erhu", erhu, level=-1.0, pan=0.12, send=0.25),
        Track("pipa_b", "pipa", pipa_b, level=-9.0, pan=-0.25, send=0.2),
        Track("sheng", "sheng", pad, level=-13.0, pan=0.25, send=0.4, human=(0.0, 0.0)),
        Track("bass", "daruan", bass, level=-7.5, pan=0.0, send=0.12, human=(5.0, 0.05)),
        Track("tanggu", "perc:tanggu", drum, level=-8.0, pan=0.0, send=0.25, human=(4.0, 0.07)),
        Track("bangu", "perc:bangu", back, level=-16.0, pan=-0.3, send=0.2, human=(4.0, 0.08)),
        Track("xiaoluo", "perc:xiaoluo", luo, level=-14.0, pan=0.3, send=0.3, human=(3.0, 0.05)),
        Track("bo", "perc:bo", cym, level=-14.0, pan=0.2, send=0.3, human=(0.0, 0.0)),
    )
    return Song("bgm_sect", "外刃堂、阁堂、演武场", "宗门、操练、整肃中带锐气", "D 商调式（宫 = C）", 92, m, 32,
                "hall", tracks,
                "琵琶主奏（附点、重音、长音轮指）、梆笛（A' 与 A'' 高八度重叠）、二胡（B 段主奏，A'' 上方三度支声）、"
                "笙、大阮进行曲低音、堂鼓进行曲节奏与加花、板鼓、小锣、铙钹",
                "A A' B A''（各 8 小节）；A 两句都以附点「嗒——嗒」起头，是操练的口令感")


# ============================================================== 暗道：阴冷（A 羽，♩=54）

_CAVE_A = """
| 6,~ - 1 2 | 3s - 2 1_ 6,_ | 5,s - 6, - | - - - - |
| 3 - 5 6 | 1'~ - 6 5_ 3_ | 2 - 3s - | 6,~ - - - |
"""
_CAVE_B = """
| 6 - - 5_ 3_ | 5 - 3 - | 2 - 3 5_ 3_ | 6 - - - |
| 3' - 2' 1' | 6 - 1' 2' | 3' - 2'_ 1'_ 6 | - - - - |
"""
_CAVE_B_HARM = """
| 0 0 6o - | 0 0 0 0 | 0 0 3o - | 0 0 0 0 | 0 0 2'o - | 0 0 0 0 | 0 0 6o - | 0 0 0 0 |
"""
_CAVE_ROOTS = "6 6 5 6 6 1 2 6 | 6 6 2 6 6 6 6 6 | 6 6"


def cave() -> Song:
    key = Key.of("C4")
    m = 4
    qin = seq(key, m, _CAVE_A, 8, "| 6,~ - - - | - - - - |", octave=-1, where="cave.qin")
    harm = seq(key, m, _CAVE_B_HARM, start_bar=8, octave=-1, where="cave.harm")
    xiao = seq(key, m, _CAVE_B, start_bar=8, where="cave.xiao")
    drone = chords(key, m, _CAVE_ROOTS, (0,), octave=-2, vel=0.5)
    qing = (hits(m, "x...", 4, start_bar=0, midi=69.0, vel=0.35)
            + hits(m, "x...", 4, start_bar=0, midi=70.0, vel=0.22)
            + hits(m, "..x.", 4, start_bar=9, midi=76.0, vel=0.4)
            + hits(m, "x...", 4, start_bar=16, midi=64.0, vel=0.35))
    pulse = hits(m, "x... .... .... .... | .... .... .... ....", 16, repeat=9, midi=50.0, vel=0.45)
    tracks = (
        Track("qin", "guqin", qin, level=-1.0, pan=-0.12, send=0.35, echo=0.2, human=(16.0, 0.06)),
        Track("harm", "guqin", harm, level=-6.0, pan=-0.2, send=0.45, echo=0.3, human=(16.0, 0.06)),
        Track("xiao", "xiao", xiao, level=-1.0, pan=0.15, send=0.4, echo=0.35, human=(14.0, 0.06)),
        Track("dahu", "dahu", drone, level=-9.0, pan=0.0, send=0.35, human=(0.0, 0.0),
              params={"transpose_cents": -6.0}),
        Track("qing", "bell:qing", qing, level=-15.0, pan=0.35, send=0.5, human=(0.0, 0.0)),
        Track("pulse", "perc:tanggu", pulse, level=-16.0, pan=-0.1, send=0.5, human=(0.0, 0.0),
              params={"eq": (("lp", 400.0),)}),
        Track("drips", "amb:drips", level=-16.5, send=0.6, params={"per_minute": 22.0}),
        Track("wind", "amb:wind", level=-22.0, send=0.2, params={"center": 230.0}),
    )
    return Song("bgm_cave", "暗道", "阴冷、潮湿、看不见尽头", "A 羽调式（宫 = C）", 54, m, 18, "cave", tracks,
                "古琴低音区主奏（走手音、低泛音）、箫（B 段，带回声）、大胡极低的持续音（略偏低几音分，发阴）、"
                "磬（开头一记小二度的磬是「冷」的来源）、闷鼓、滴水、洞中气流",
                "A 8 + B 8 + 尾声 2；长音之间大量留白，回声与滴水填在空里",
                echo=(1.5, 0.38, 5, 2200.0))


# ============================================================== 墨府：雅致而不安（D 羽，♩=76）

_MANOR_A = """
| 6~ - 1'_ 6_ 5_ 3_ | 5 6_ 5_ 3 2 | 3~ - 5_ 3_ 2_ 1_ | 2~ - - - |
| 6 - 1'_ 2'_ 3' | 2'_ 1'_ 6_ 5_ 4 3 | 2_ 3_ 5_ 6_ 5 3_ 2_ | 6,~ - - - |
"""
_MANOR_B_XIAO = """
| 3~ - 2_ 3_ 5 | 4~ - 3 2 | 1_ 2_ 1_ 6,_ 1 2 | 6,~ - - - |
"""
_MANOR_B_ERHU = """
| 7, - 1s 2 | 3~ - 2_ 1_ 7,_ 6,_ | 3s - 6,s - | - - - - |
"""
_MANOR_A2_XIAO = """
| 3 - - - | 2 - 4 - | 3 - - - | 2 - 1 - | 6, - - - | 7, - 1 - | 2 - 3 - | 6, - - - |
"""
_MANOR_CODA = """
| 6 - 5 3_ 2_ | 3 - 2 1_ 7,_ | 6,~ - - - | - - - - |
"""
_MANOR_ROOTS = "6 1 6 2 6 1 2 6 | 6 2 1 6 6 6 6 6 | 6 1 6 2 6 1 2 6 | 6 6 6 6"


def manor() -> Song:
    key = Key.of("F4")
    m = 4
    zheng = seq(key, m, _MANOR_A, 8, _MANOR_A, _MANOR_CODA, where="manor.zheng")
    xiao = (seq(key, m, _MANOR_B_XIAO, start_bar=8, where="manor.xiao_b")
            + seq(key, m, _MANOR_A2_XIAO, start_bar=16, vel=0.5, where="manor.xiao_a2"))
    erhu = seq(key, m, _MANOR_B_ERHU, start_bar=12, where="manor.erhu")
    pad = fold(chords(key, m, _MANOR_ROOTS, (0, 3), octave=-1, vel=0.38), 55.0)
    rub = seq(key, m, "| 4 - - - | 4 - - - | 3 - - - | 3 - - - | 7, - - - | 7, - - - | 6, - - - | 6, - - - |",
              start_bar=8, vel=0.35, where="manor.rub")
    pulse = bassline(key, m, _MANOR_ROOTS, ((0, 0.5, 0), (0.75, 0.75, 0)), octave=-2, vel=0.55, orn="!")
    tick = hits(m, "x... .... x... ....", 16, repeat=28, midi=62.0, vel=0.4)
    tock = hits(m, ".... x... .... x...", 16, repeat=28, midi=58.0, vel=0.35)
    cold = tuple(n for b in (3, 11, 19) for n in hits(m, "..x.", 4, start_bar=b, midi=98.0, vel=0.35))
    tracks = (
        Track("zheng", "guzheng", zheng, level=0.0, pan=-0.1, send=0.3, human=(9.0, 0.06)),
        Track("xiao", "xiao", xiao, level=-1.5, pan=0.2, send=0.38, human=(10.0, 0.05)),
        Track("erhu", "erhu", erhu, level=-2.0, pan=0.25, send=0.3, human=(10.0, 0.05)),
        Track("sheng", "sheng", pad, level=-14.0, pan=0.15, send=0.4, human=(0.0, 0.0)),
        Track("rub", "sheng", rub, level=-16.0, pan=-0.25, send=0.45, human=(0.0, 0.0)),
        Track("pulse", "daruan", pulse, level=-10.0, pan=0.0, send=0.15, human=(3.0, 0.04)),
        Track("tick", "perc:muyu", tick + tock, level=-19.0, pan=-0.4, send=0.12, human=(2.0, 0.03),
              params={"eq": (("lp", 4000.0),)}),
        Track("cold", "bell:pengling", cold, level=-19.0, pan=0.45, send=0.5, human=(0.0, 0.0)),
    )
    return Song("bgm_manor", "墨府", "雅致、深宅、暗流涌动", "D 羽调式（宫 = F，间用清角与变宫）", 76, m, 28,
                "hall", tracks,
                "古筝主奏、箫（B 段与 A' 的长音副旋律）、二胡（滑音）、笙（B 段一条贴着半音的长音，是不安的来源）、"
                "大阮「咚—咚」的心跳低音、木鱼一滴一答像更漏、碰铃",
                "A 8 + B 8（箫四小节、二胡四小节）+ A' 8 + 尾声 4；旋律是五声的「雅」，"
                "清角（♭B）与变宫（E）只在转折处出现一两次，是「不安」")


SONGS = {
    "bgm_qixuanmen": qixuanmen,
    "bgm_valley": valley,
    "bgm_indoor": indoor,
    "bgm_sect": sect,
    "bgm_cave": cave,
    "bgm_manor": manor,
}
