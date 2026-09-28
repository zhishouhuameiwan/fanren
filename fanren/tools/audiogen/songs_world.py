# -*- coding: utf-8 -*-
"""野外与市井的曲子：韩家村、城镇、山道、崖顶、渡口、野外、客栈。

每首一个函数，返回 mixer.Song。写法约定：
  - 旋律用简谱（score.parse），一段一个常量，结构一眼可见（A / A' / B / A''）。
  - 伴奏用 score.arpeggio / bassline / chords 按「和声根音表」生成——根音表逐小节
    对着旋律选：旋律落在强拍上的骨干音，要么是根音，要么是根音上方的四、五度。
    五声音乐不走西洋功能和声（没有属七、没有导音解决），和声只是「垫一个空五度」。
  - 声部的 level 是相对响度（见 mixer.py 文件头）：主旋律 0，副旋律 -4 ~ -7，
    伴奏 -7 ~ -10，低音 -6 ~ -8，打击 -8 ~ -15，环境 -18 ~ -26。
  - 曲尾都回到曲首能接上的位置（落在调式主音或它的五度上），循环时像一首曲子
    自然地「再来一遍」，而不是结束了又重新开始。
"""
from __future__ import annotations

from mixer import Song, Track
from score import (Key, arpeggio, bassline, chords, fold, hits, penta_shift, scale_vel, seq, strip,
                   transpose)

# ============================================================== 韩家村：闲适的田园（D 徵，♩=84）

_VILLAGE_A = """
| 5. 6_ 1' 2'_ 1'_ | 6_ 5_ 6_ 1'_ 5~ - | (6)3. 5_ 6 5_ 3_ | 2_ 3_ 5_ 3_ 2~ - |
| 1'. 2'_ 3'^ 2'_ 1'_ | 6_ 1'_ 2'_ 3'_ /2' - | 5'_ 3'_ 2'_ 1'_ 6 1'_ 6_ | 5~ - - - |
"""
_VILLAGE_A2 = """
| 5. 6_ 1' 2'_ 1'_ | 6_ 5_ 6_ 1'_ 5~ - | 3. 5_ 6 1'_ 6_ | 5_ 6_ 5_ 3_ 2~ - |
| 3 5_ 6_ 1' 6_ 5_ | 3_ 5_ 2_ 3_ 1~ - | 2_ 3_ 5_ 6_ 3 2_ 3_ | 5~ - - - |
"""
_VILLAGE_B = """
| 6* - 1'_ 6_ 5_ 3_ | 6~ - 5 3_ 2_ | 1_ 2_ 3_ 5_ 6. 5_ | /3* - - - |
| 2 3_ 5_ 6 5_ 3_ | 2_ 1_ 6,_ 1_ 2~ - | 3_ 5_ 3_ 2_ 1 6,_ 1_ | 2* - - - |
"""
_VILLAGE_COUNTER = """
| 2 - - - | 3 - 5 - | 6 - 5 3 | 2 - - - |
| 3 - 2 - | 1 - - - | 2 - 3 5 | 5 - - - |
"""
_VILLAGE_ROOTS_A = "5 1 6 2 1 2 1 5"
_VILLAGE_ROOTS_A2 = "5 1 6 2 6 1 2 5"
_VILLAGE_ROOTS_B = "6 6 1 6 2 2 1 2"


def village() -> Song:
    key = Key.of("G4")
    m = 4
    melody = seq(key, m, _VILLAGE_A, _VILLAGE_A2, 8, _VILLAGE_A, where="village.dizi")
    lead_b = seq(key, m, _VILLAGE_B, start_bar=16, where="village.zheng_b")
    zheng_double = transpose(strip(seq(key, m, _VILLAGE_A, start_bar=24, where="village.zheng_a"),
                                   "~^("), -12)
    counter = seq(key, m, _VILLAGE_COUNTER, start_bar=8, octave=-1, vel=0.5, where="village.zhonghu")
    all_roots = " ".join([_VILLAGE_ROOTS_A, _VILLAGE_ROOTS_A2, _VILLAGE_ROOTS_B, _VILLAGE_ROOTS_A])
    flow = (0, 3, 5, 7, 8, 7, 5, 3)
    acc = (arpeggio(key, m, " ".join([_VILLAGE_ROOTS_A, _VILLAGE_ROOTS_A2]), flow, -2, vel=0.5)
           + scale_vel(arpeggio(key, m, _VILLAGE_ROOTS_B, (0, None, 5, None, 3, None, 5, None), -2,
                                vel=0.5, start_bar=16), 0.8)
           + arpeggio(key, m, _VILLAGE_ROOTS_A, flow, -2, vel=0.55, start_bar=24))
    bass = bassline(key, m, all_roots, ((0, 2, 0), (2, 2, 3)), octave=-2, vel=0.62)
    bells = hits(m, "x... .... .... ....", 16, repeat=8, midi=93.0, vel=0.55)
    pad = chords(key, m, "6 6 1 1 2 2 1 2", (0, 2, 3), octave=-1, vel=0.45, start_bar=16)
    wood = hits(m, ".... x... .... x...", 16, start_bar=24, repeat=8, vel=0.5)
    tracks = (
        Track("dizi", "dizi", melody, level=0.0, pan=0.08, send=0.22),
        Track("zheng_lead", "guzheng", lead_b, level=-0.5, pan=-0.1, send=0.25),
        Track("zheng_double", "guzheng", zheng_double, level=-6.0, pan=-0.2, send=0.25),
        Track("sheng", "sheng", pad, level=-12.0, pan=0.2, send=0.35, human=(4.0, 0.03)),
        Track("zhonghu", "zhonghu", counter, level=-7.0, pan=0.3, send=0.3),
        Track("zheng_acc", "guzheng", acc, level=-8.0, pan=-0.25, send=0.28, human=(6.0, 0.08)),
        Track("bass", "daruan", bass, level=-7.5, pan=0.05, send=0.12),
        Track("bells", "bell:pengling", bells, level=-17.0, pan=0.45, send=0.4),
        Track("muyu", "perc:muyu", wood, level=-15.0, pan=-0.35, send=0.18),
        Track("birds", "amb:birds", level=-26.0, send=0.3, params={"per_minute": 4.0}),
    )
    return Song("bgm_village", "韩家村", "闲适、田园、晨光", "D 徵调式（宫 = G）", 84, m, 32, "open",
                tracks, "笛子主奏、古筝（流水型伴奏 + B 段主奏，长音摇指）、中胡副旋律、笙、大阮、碰铃、"
                "木鱼、远处鸟鸣",
                "A A' B A''（各 8 小节）。A 是起承转合四个两小节乐句，徵音收束；B 转到羽音色彩，"
                "由古筝接过旋律")


# ============================================================== 城镇：热闹的市集（D 宫，♩=116）

_TOWN_A = """
| 5 5_^ 6_ 1' 6_ 1'_ | (6)5 - 3_ 5_ 6_ 5_ | 2 2_^ 3_ 5 3_ 5_ | (3)2 - 1_ 2_ 3_ 5_ |
| 6 6_^ 1'_ 2' 1'_ 6_ | 5_ 6_ 1'_ 2'_ 3'^ 2' | 1'_ 2'_ 1'_ 6_ 5 6_ 5_ | 3_ 5_ 2_ 3_ 1~ - |
"""
_TOWN_A2 = """
| 5 5_^ 6_ 1' 6_ 1'_ | (6)5 - 3_ 5_ 6_ 5_ | 2 2_^ 3_ 5 3_ 5_ | (3)2 - 1_ 2_ 3_ 5_ |
| 6_ 5_ 6_ 1'_ 2' 3'_ 2'_ | 1' 6_ 1'_ 5 - | 6_ 1'_ 6_ 5_ 3_ 5_ 2_ 3_ | 1 - - 0 |
"""
_TOWN_B_PIPA = """
| 5'* - 3'_ 2'_ 1'_ 2'_ | 3'* - 2'_ 1'_ 6_ 1'_ | 2' 1'_ 6_ 5 6_ 1'_ | 5* - - - |
"""
_TOWN_B_DIZI = """
| 3_ 5_ 3_ 2_ 1 2_ 3_ | 5 6_^ 5_ 3 - | 2_ 3_ 5_ 6_ 5_ 3_ 2_ 3_ | 5~ - 0 0 |
"""
_TOWN_COUNTER = """
| 1 - 2 - | 3 - 5 - | 6 - 5 3 | 2 - - - |
| 2 - 3 5 | 6 - 5 - | 3 - 2 - | 1 - - - |
"""
_TOWN_ROOTS_A = "5 1 2 5 6 1 1 5/1"
_TOWN_ROOTS_A2 = "5 1 2 5 2 1 6/5 1"
_TOWN_ROOTS_B = "1 6 5 5 1 6 2 5"


def town() -> Song:
    key = Key.of("D5")
    m = 4
    melody = seq(key, m, 4, _TOWN_A, _TOWN_A2, 4, _TOWN_B_DIZI, 4, _TOWN_A, where="town.dizi")
    pipa_b = seq(key, m, _TOWN_B_PIPA, start_bar=20, octave=-1, where="town.pipa_b")
    pipa_double = strip(seq(key, m, _TOWN_A, start_bar=32, octave=-1, where="town.pipa_a"), "~^(")
    counter = seq(key, m, _TOWN_COUNTER, start_bar=12, octave=-1, vel=0.52, where="town.erhu")
    roots_main = " ".join(["1 5 1 5", _TOWN_ROOTS_A, _TOWN_ROOTS_A2, _TOWN_ROOTS_B])
    bounce = (0, 5, 3, 5, 0, 5, 3, 5)
    yq = (arpeggio(key, m, roots_main, bounce, -1, vel=0.5)
          + seq(key, m, "| 5* - - - | 5* - - - | 5* - - - | 5* - 0 0 |", start_bar=28, octave=-1,
                vel=0.42, where="town.yq_break")
          + arpeggio(key, m, _TOWN_ROOTS_A, bounce, -1, vel=0.55, start_bar=32))
    bass = bassline(key, m, " ".join([roots_main, "5 5 5 5", _TOWN_ROOTS_A]),
                    ((0, 1, 0), (1, 1, 3), (2, 1, 0), (3, 1, 3)), octave=-3, vel=0.62, orn="!")
    clap = (hits(m, ".... x... .... x...", 16, start_bar=4, repeat=16, vel=0.55)
            + hits(m, ".... x... .... x...", 16, start_bar=32, repeat=8, vel=0.6))
    drum = (hits(m, "........ ....x.xx", 16, start_bar=3, vel=0.55)
            + hits(m, "x...x.x.x...x.xx", 16, start_bar=32, repeat=8, vel=0.5))
    gong_small = hits(m, "x...............", 16, start_bar=0, vel=0.7) + tuple(
        n for b in (4, 12, 20, 32) for n in hits(m, "x...............", 16, start_bar=b, vel=0.75))
    # 第 29–32 小节：锣鼓经过门（仓 才 仓 才 / 仓 令 才 仓……）。
    brk_daluo = hits(m, "x....... x....... x...x... x.x.x...", 8, start_bar=28)
    brk_bo = hits(m, "..x...x. ..x.x... ..x...x. .x.x.x.x", 8, start_bar=28, vel=0.8)
    brk_luo = hits(m, "....x... ......x. ....x... ........", 8, start_bar=28, vel=0.85)
    brk_gu = hits(m, "x.x.x.x. x.x.xxx. x.x.x.x. rrrrxxxx", 8, start_bar=28, vel=0.75)
    brk_tang = hits(m, "x...x... x...x... x...x... x.x.x.x.", 8, start_bar=28, vel=0.8)
    tracks = (
        Track("dizi", "dizi", melody, level=0.0, pan=0.06, send=0.18),
        Track("pipa_b", "pipa", pipa_b, level=-1.0, pan=-0.15, send=0.2),
        Track("pipa_double", "pipa", pipa_double, level=-6.0, pan=-0.22, send=0.2),
        Track("erhu", "erhu", counter, level=-7.0, pan=0.3, send=0.25),
        Track("yangqin", "yangqin", yq, level=-8.0, pan=-0.3, send=0.2, human=(5.0, 0.07)),
        Track("bass", "daruan", bass, level=-7.0, pan=0.05, send=0.1, human=(5.0, 0.05)),
        Track("paiban", "perc:paiban", clap, level=-15.0, pan=0.35, send=0.15, human=(4.0, 0.08)),
        Track("bangu", "perc:bangu", drum + brk_gu, level=-14.0, pan=-0.2, send=0.15,
              human=(4.0, 0.08)),
        Track("xiaoluo", "perc:xiaoluo", gong_small + brk_luo, level=-14.0, pan=0.25, send=0.25,
              human=(3.0, 0.05)),
        Track("daluo", "perc:daluo", brk_daluo, level=-10.0, pan=-0.05, send=0.3, human=(3.0, 0.05)),
        Track("bo", "perc:bo", brk_bo, level=-12.0, pan=0.15, send=0.25, human=(3.0, 0.05)),
        Track("tanggu", "perc:tanggu", brk_tang, level=-10.0, pan=0.0, send=0.2, human=(3.0, 0.05)),
    )
    return Song("bgm_town", "青牛镇、山下镇、嘉元城（南城、西城）", "热闹、市井、人来人往", "D 宫调式（宫 = D）",
                116, m, 40, "open", tracks,
                "梆笛主奏（打音、倚音）、琵琶（B 段主奏 + 轮指）、二胡副旋律、扬琴跳音伴奏、大阮、"
                "拍板、板鼓、小锣；第 29–32 小节是大锣、铙钹、小锣、板鼓、堂鼓的锣鼓经过门",
                "前奏 4 + A 8 + A' 8 + B 8（琵琶问、笛子答）+ 锣鼓过门 4 + A'' 8")


# ============================================================== 山道：行旅（G 商，♩=96）

_PATH_A = """
| 2~ - 3_ 5_ 6 | 5_ 3_ 2_ 3_ 5~ - | 6 1'_ 6_ 5 3_ 5_ | 2~ - - - |
| 2 - 3_ 5_ 6 | 1'_ 6_ 1'_ 2'_ /3'~ - | 2'_ 1'_ 6_ 5_ 6 5_ 3_ | 2~ - - - |
"""
_PATH_A2 = """
| 2~ - 3_ 5_ 6 | 5_ 3_ 2_ 3_ 5~ - | 6 1'_ 6_ 5 3_ 5_ | 2~ - - - |
| 5 - 6_ 1'_ 2' | 3'_ 2'_ 1'_ 6_ 5~ - | 6_ 5_ 3_ 5_ 2s 1_ 6,_ | 2~ - - - |
"""
_PATH_B = """
| 6 1' 2' 3' | 5'~ - 3'_ 2'_ 1'_ 2'_ | 3' - 2' 1'_ 6_ | 5 - 6 - |
| 1'~ - 6_ 5_ 3_ 5_ | 6 - 5_ 3_ 2_ 3_ | 5_ 6_ 5_ 3_ 2 1_ 2_ | 3~ - - - |
"""
_PATH_ROOTS_A = "2 5 6 2 2 1 5 2"
_PATH_ROOTS_A2 = "2 5 6 2 5 1 6 2"
_PATH_ROOTS_B = "6 1 6 5/6 1 6 5 6"


def mountain_path() -> Song:
    key = Key.of("F4")
    m = 4
    dizi = seq(key, m, _PATH_A, 8, _PATH_B, _PATH_A, where="path.dizi")
    erhu = (seq(key, m, _PATH_A2, start_bar=8, where="path.erhu")
            + scale_vel(penta_shift(seq(key, m, _PATH_A, start_bar=24, where="path.erhu2"), key, -2),
                        0.8))
    roots = " ".join([_PATH_ROOTS_A, _PATH_ROOTS_A2, _PATH_ROOTS_B, _PATH_ROOTS_A])
    walk = (0, 3, 5, 6, 5, 3, 5, 3)
    zheng = arpeggio(key, m, roots, walk, -2, vel=0.5)
    bass = bassline(key, m, roots, ((0, 1.5, 0), (1.5, 0.5, 0), (2, 2, 3)), octave=-2, vel=0.62)
    steps = hits(m, ".... x... .... x...", 16, repeat=32, vel=0.45)
    drum = hits(m, "x....... ........", 16, repeat=32, vel=0.5)
    bells = hits(m, "x... .... .... ....", 16, start_bar=16, repeat=2, midi=86.0, vel=0.5)
    pad = fold(chords(key, m, _PATH_ROOTS_B, (0, 3, 5), octave=-1, vel=0.42, start_bar=16), 55.0)
    tracks = (
        Track("dizi", "dizi", dizi, level=0.0, pan=0.1, send=0.25),
        Track("erhu", "erhu", erhu, level=-2.5, pan=-0.12, send=0.25),
        Track("zheng", "guzheng", zheng, level=-8.0, pan=-0.3, send=0.28, human=(6.0, 0.07)),
        Track("bass", "daruan", bass, level=-7.0, pan=0.05, send=0.12),
        Track("sheng", "sheng", pad, level=-13.0, pan=0.25, send=0.35, human=(4.0, 0.03)),
        Track("muyu", "perc:muyu", steps, level=-17.0, pan=0.3, send=0.2, human=(5.0, 0.1)),
        Track("tanggu", "perc:tanggu", drum, level=-15.0, pan=-0.05, send=0.25, human=(4.0, 0.08)),
        Track("bells", "bell:bianzhong", bells, level=-16.0, pan=0.35, send=0.5),
        Track("wind", "amb:wind", level=-25.0, send=0.1, params={"center": 480.0}),
    )
    return Song("bgm_mountain_path", "彩霞山道、谷外", "行旅、山风、步履不停", "G 商调式（宫 = F）", 96, m, 32,
                "valley", tracks,
                "曲笛主奏、二胡（A' 接旋律、A'' 支声平行四度）、古筝行进型分解和弦、大阮、笙、木鱼（步点）、"
                "堂鼓、编钟、山风",
                "A A' B A''（各 8 小节）。A 与 A' 同头异尾（换头合尾的反用：同起不同落），"
                "B 登高转入羽音，末句落角音，再一步回到商音开头")


# ============================================================== 崖顶：苍茫（B 羽，♩=60）

_CLIFF_A = """
| 6~ - - 1'_ 2'_ | 3'~ - 2'_ 1'_ 6 | 5 - 3 5_ 6_ | 6~ - - - |
| 3'~ - 5'_ 3'_ 2' | 1'_ 2'_ 1'_ 6_ 5 - | 3 5 6 1'_ 2'_ | 6~ - - - |
"""
_CLIFF_B_QIN = """
| 6,~ - 3s - | 2 - 1 6,_ 1_ | 2 - 3 5 | 6o - - - |
"""
_CLIFF_B_XIAO = """
| 6'~ - 5'_ 3'_ 2'_ 3'_ | 5'~ - 3' 2'_ 1'_ | 6 - 5 3_ 5_ | 6~ - - - |
"""
_CLIFF_CODA = """
| 3'~ - 2'_ 1'_ 6 | 5_ 3_ 2_ 3_ 5 - | 3 - 2 1_ 2_ | 6~ - - - |
"""
_CLIFF_ROOTS = "6 6 5 6 6 2 1 6 | 6 2 5 6 6 1 5 6 | 6 5 2 6"


def cliff() -> Song:
    key = Key.of("D4")
    m = 4
    xiao = seq(key, m, _CLIFF_A, 4, _CLIFF_B_XIAO, _CLIFF_CODA, where="cliff.xiao")
    qin_melody = seq(key, m, _CLIFF_B_QIN, start_bar=8, where="cliff.qin")
    qin_bass = arpeggio(key, m, _CLIFF_ROOTS, (0, None, None, None), -2, vel=0.55, beats=4.0)
    qin_harm = (seq(key, m, "| 0 0 6'o - | 0 0 0 0 | 0 0 0 5'o | 0 0 0 0 |", start_bar=0,
                    octave=-1, vel=0.45, where="cliff.harm1")
                + seq(key, m, "| 0 0 3'o - | 0 0 0 0 | 0 0 2'o - | 0 0 0 0 |", start_bar=16,
                      octave=-1, vel=0.45, where="cliff.harm2"))
    drone_root = chords(key, m, _CLIFF_ROOTS, (0,), octave=-2, vel=0.5)
    drone_fifth = fold(chords(key, m, _CLIFF_ROOTS, (3,), octave=-2, vel=0.4), 55.0)
    gong = hits(m, "x...", 4, start_bar=0, vel=0.35) + hits(m, "x...", 4, start_bar=8, vel=0.3)
    bells = (hits(m, "..x.", 4, start_bar=4, midi=71.0, vel=0.4)
             + hits(m, "x...", 4, start_bar=12, midi=66.0, vel=0.35)
             + hits(m, "..x.", 4, start_bar=17, midi=71.0, vel=0.35))
    tracks = (
        Track("xiao", "xiao", xiao, level=0.0, pan=0.05, send=0.4, human=(14.0, 0.07)),
        Track("qin", "guqin", qin_melody, level=-1.5, pan=-0.15, send=0.35, human=(18.0, 0.06)),
        Track("qin_bass", "guqin", qin_bass + qin_harm, level=-7.0, pan=-0.2, send=0.35,
              human=(12.0, 0.05)),
        Track("dahu", "dahu", drone_root, level=-10.0, pan=0.0, send=0.3, human=(0.0, 0.0)),
        Track("sheng", "sheng", drone_fifth, level=-14.0, pan=0.2, send=0.45, human=(0.0, 0.0)),
        Track("daluo", "perc:daluo", gong, level=-13.0, pan=-0.1, send=0.5, human=(0.0, 0.0),
              params={"eq": (("lp", 3000.0),)}),
        Track("bells", "bell:bianzhong", bells, level=-15.0, pan=0.4, send=0.55),
        Track("wind", "amb:wind", level=-19.0, send=0.05, params={"center": 560.0}),
    )
    return Song("bgm_cliff", "炼骨崖、崖壁、落日峰", "苍茫、孤高、风声", "B 羽调式（宫 = D）", 60, m, 20,
                "valley", tracks,
                "箫主奏、古琴（B 段主奏、低音与泛音）、大胡与笙的空五度长音、大锣（轻）、编钟（远）、崖顶长风",
                "A 8 + B 8（古琴问、箫答）+ 尾声 4；尾声落回羽音，接回 A 的第一个音")


# ============================================================== 渡口：流水（C 徵，6/8，♪=150）

_RIVER_A = """
| 5--~ 6 1' 6 | 5-- 3--~ | 2 3 5 6 5 3 | 2-----~ |
| 3-- 5 6 1' | /2'-- 1' 6 5 | 6 1' 6 5 3 2 | 5-----~ |
"""
_RIVER_A2 = """
| 5-- 6 1' 6 | 5-- 3-- | 2 3 5 6 5 3 | 2----- |
| 1'-- 2' 3' 2' | 1'-- 6 5 3 | 2 3 1 2 6, 1 | 5,----- |
"""
_RIVER_B = """
| 6--~ 1' 2' 3' | 5'--~ 3' 2' 1' | 6-- 5 6 1' | 6-----~ |
| 3'-- 2' 1' 6 | 5-- 6 1' 2' | 3' 2' 1' 6 5 3 | 2-----~ |
"""
_RIVER_ROOTS_A = "5 1 2 2 6 5 6/2 5"
_RIVER_ROOTS_A2 = "5 1 2 2 1 6 2 5"
_RIVER_ROOTS_B = "6 1 6 6 1 5 1/6 2"


def river() -> Song:
    key = Key.of("F4")
    m = 6  # 6/8 拍，按八分音符计拍
    dizi = seq(key, m, _RIVER_A, 8, _RIVER_B, _RIVER_A, where="river.dizi")
    zheng_lead = strip(seq(key, m, _RIVER_A2, start_bar=8, where="river.zheng"), "~")
    counter = scale_vel(seq(key, m, "| 3----- | 2----- | 1----- | 2----- | 3----- | 5----- | 6----- | 2----- |",
                            start_bar=8, vel=0.45, where="river.xiao"), 1.0)
    roots = " ".join([_RIVER_ROOTS_A, _RIVER_ROOTS_A2, _RIVER_ROOTS_B, _RIVER_ROOTS_A])
    flow = (0, 3, 5, 7, 5, 3)
    zheng_acc = arpeggio(key, m, roots, flow, -2, vel=0.48, beats=2.0)
    bass = bassline(key, m, roots, ((0, 3, 0), (3, 3, 3)), octave=-2, vel=0.6)
    bells = hits(m, "..x... ......", 6, start_bar=16, repeat=4, midi=91.0, vel=0.4)
    tracks = (
        Track("dizi", "dizi", dizi, level=0.0, pan=0.1, send=0.3, human=(9.0, 0.06)),
        Track("zheng_lead", "guzheng", zheng_lead, level=-0.5, pan=-0.1, send=0.3),
        Track("xiao", "xiao", counter, level=-8.0, pan=0.3, send=0.4, human=(10.0, 0.05)),
        Track("zheng_acc", "guzheng", zheng_acc, level=-8.5, pan=-0.3, send=0.3, human=(6.0, 0.07)),
        Track("bass", "daruan", bass, level=-8.0, pan=0.05, send=0.15),
        Track("bells", "bell:pengling", bells, level=-19.0, pan=0.45, send=0.5),
        Track("water", "amb:stream", level=-18.5, send=0.08),
    )
    return Song("bgm_river", "渡口", "流水、摆渡、船歌的摇晃", "C 徵调式（宫 = F）", 150, m, 32, "open",
                tracks,
                "曲笛主奏、古筝（A' 段主奏 + 6/8 流水型）、箫的长音、大阮、碰铃、河水（流水底噪 + 气泡）",
                "A A' B A''（各 8 小节，6/8 拍）。每句都是「长—短短短」的摇橹节奏，A' 的尾句沉到低音徵")


# ============================================================== 野外：独霸山庄外的紧张（E 羽，♩=104）

_WILD_A = """
| 6~ - - 5_ 6_ | 1's 6_ 5_ 3~ - | 2_ 3_ 5_ 6_ 5_ 3_ 2_ 3_ | 6,~ - - - |
| 3~ - - 2_ 3_ | 5s 3_ 2_ 1~ - | 6,_ 1_ 2_ 3_ 2 1_ 6,_ | 6,~ - - - |
"""
_WILD_A2 = """
| 6~ - - 5_ 6_ | 1's 6_ 5_ 3~ - | 2_ 3_ 5_ 6_ 5_ 3_ 2_ 3_ | 6,~ - - - |
| 1'~ - - 6_ 1'_ | 2's 1'_ 6_ 5~ - | 3_ 5_ 6_ 1'_ 6 5_ 3_ | 6~ - - - |
"""
_WILD_B = """
| 3'~ - 2'_ 3'_ 5'_ 3'_ | 2' - 1' 6 | 1'~ - 6_ 5_ 3_ 5_ | 6~ - - - |
| 3'~ - 5'_ 3'_ 2'_ 1'_ | 2' - 6 1' | 2'_ 1'_ 6_ 5_ 3 5 | 6~ - - - |
"""
_WILD_ROOTS_A = "6 6 2 6 3 1 2 6"
_WILD_ROOTS_A2 = "6 6 2 6 1 2 6 6"
_WILD_ROOTS_B = "6 2 1 6 6 2 5 6"


def wild() -> Song:
    key = Key.of("G4")
    m = 4
    erhu = seq(key, m, 4, _WILD_A, _WILD_A2, 8, _WILD_A, where="wild.erhu")
    dizi = seq(key, m, _WILD_B, start_bar=20, where="wild.dizi")
    erhu_b = seq(key, m, "| 6 - - - | 2' - - - | 1' - - - | 6 - - - | 6 - - - | 2' - - - | 5 - - - | 6 - - - |",
                 start_bar=20, vel=0.5, where="wild.erhu_b")
    pipa_double = strip(seq(key, m, _WILD_A, start_bar=28, where="wild.pipa"), "~s")
    roots = " ".join(["6 6 6 6", _WILD_ROOTS_A, _WILD_ROOTS_A2, _WILD_ROOTS_B, _WILD_ROOTS_A])
    riff = (0, None, 0, 3, None, 0, 1, None, 0, None, 0, 3, None, 4, 3, 1)
    ostinato = fold(arpeggio(key, m, roots, riff, -2, vel=0.55, accent=0.2, beats=0.4), 45.0)
    drone = chords(key, m, roots, (0,), octave=-2, vel=0.45)
    groove = "X..x..x.X...x.x."
    fill = "X.x.xxxxrrrrXXXX"
    drum = (hits(m, groove, 16, repeat=3, vel=0.8) + hits(m, fill, 16, start_bar=3, vel=0.8)
            + hits(m, groove, 16, start_bar=4, repeat=15, vel=0.8)
            + hits(m, fill, 16, start_bar=19, vel=0.85)
            + hits(m, "X..x..x.X..xx.x.", 16, start_bar=20, repeat=7, vel=0.85)
            + hits(m, fill, 16, start_bar=27, vel=0.9)
            + hits(m, groove, 16, start_bar=28, repeat=7, vel=0.8)
            + hits(m, fill, 16, start_bar=35, vel=0.85))
    clap = hits(m, ".... x... .... x...", 16, start_bar=4, repeat=32, vel=0.5)
    luo = tuple(n for b in (4, 12, 20, 28) for n in hits(m, "x...............", 16, start_bar=b, vel=0.7))
    tracks = (
        Track("erhu", "erhu", erhu, level=0.0, pan=0.05, send=0.22),
        Track("dizi", "dizi", dizi, level=-0.5, pan=0.15, send=0.25),
        Track("erhu_b", "erhu", erhu_b, level=-8.0, pan=-0.2, send=0.25),
        Track("pipa_double", "pipa", pipa_double, level=-6.0, pan=-0.25, send=0.2),
        Track("ostinato", "pipa", ostinato, level=-7.0, pan=-0.3, send=0.15, human=(4.0, 0.08)),
        Track("dahu", "dahu", drone, level=-11.0, pan=0.1, send=0.25, human=(0.0, 0.0)),
        Track("tanggu", "perc:tanggu", drum, level=-8.0, pan=0.0, send=0.2, human=(4.0, 0.08)),
        Track("paiban", "perc:paiban", clap, level=-16.0, pan=0.35, send=0.15, human=(4.0, 0.08)),
        Track("xiaoluo", "perc:xiaoluo", luo, level=-14.0, pan=0.25, send=0.3, human=(3.0, 0.05)),
        Track("wind", "amb:wind", level=-26.0, send=0.05, params={"center": 400.0}),
    )
    return Song("bgm_wild", "独霸山庄外", "野外、紧张、步步为营", "E 羽调式（宫 = G）", 104, m, 36, "valley",
                tracks,
                "二胡主奏（滑音、揉弦）、梆笛（B 段）、琵琶切分固定音型、大胡持续低音、堂鼓切分节奏与加花、"
                "拍板、小锣、野风",
                "前奏 4 + A 8 + A' 8 + B 8 + A'' 8；每段第 8 小节堂鼓加花接下一段")


# ============================================================== 客栈：温暖（F 宫，♩=88）

_INN_A = """
| 1 2_ 3_ 5 3_ 2_ | 1_ 6,_ 1_ 2_ 3* - | 5 6_ 5_ 3 2_ 3_ | 5* - - - |
| 6 5_ 6_ 1' 6_ 5_ | 3_ 5_ 6_ 5_ 3* - | 2_ 3_ 5_ 3_ 2 1_ 6,_ | 1* - - - |
"""
_INN_A_ERHU = """
| 1 2_ 3_ 5 3_ 2_ | 1_ 6,_ 1_ 2_ 3~ - | 5 6_ 5_ 3 2_ 3_ | 5~ - - - |
| 6 5_ 6_ 1' 6_ 5_ | 3_ 5_ 6_ 5_ 3~ - | 2_ 3_ 5_ 3_ 2s 1_ 6,_ | 1~ - - - |
"""
_INN_B = """
| 3 5_ 6_ 1' 2' | 3'_ 2'_ 1'_ 6_ 5* - | 6 1'_ 6_ 5 3_ 5_ | 6* - - - |
| 5 6_ 1'_ 2' 1'_ 6_ | 5_ 6_ 5_ 3_ 2* - | 3_ 5_ 3_ 2_ 1 2_ 3_ | 5* - - - |
"""
_INN_ROOTS_A = "1 6 1 5 6 1 2 1"
_INN_ROOTS_B = "6 1 6 6 5 1 6 5"


def inn() -> Song:
    key = Key.of("F4")
    m = 4
    pipa = seq(key, m, _INN_A, 8, _INN_B, _INN_A, where="inn.pipa")
    erhu = (seq(key, m, _INN_A_ERHU, start_bar=8, where="inn.erhu")
            + seq(key, m, "| 3 - - - | 1' - - - | 6 - - - | 3 - - - | 2 - - - | 5 - - - | 3 - 2 - | 3 - - - |",
                  start_bar=16, vel=0.48, where="inn.erhu_b")
            + scale_vel(penta_shift(strip(seq(key, m, _INN_A, start_bar=24, where="inn.erhu_a"), "*"),
                                    key, 2), 0.78))
    roots = " ".join([_INN_ROOTS_A, _INN_ROOTS_A, _INN_ROOTS_B, _INN_ROOTS_A])
    broken = (0, 3, 5, 3, 7, 5, 3, 5)
    yq = fold(arpeggio(key, m, roots, broken, -2, vel=0.42), 43.0)
    bass = bassline(key, m, roots, ((0, 2, 0), (2, 1.5, 3), (3.5, 0.5, 2)), octave=-2, vel=0.6)
    wood = hits(m, "..x. ...x ..x. ...x", 16, repeat=32, vel=0.4)
    # 拍板每两小节一下，像说书人的醒木，只点句头。
    clap = hits(m, "x... .... .... .... | .... .... .... ....", 16, repeat=16, vel=0.5)
    tracks = (
        Track("pipa", "pipa", pipa, level=0.0, pan=-0.05, send=0.25),
        Track("erhu", "erhu", erhu, level=-1.5, pan=0.2, send=0.28),
        Track("yangqin", "yangqin", yq, level=-9.0, pan=-0.3, send=0.25, human=(6.0, 0.08)),
        Track("bass", "daruan", bass, level=-7.5, pan=0.05, send=0.15),
        Track("muyu", "perc:muyu", wood, level=-18.0, pan=0.3, send=0.2, human=(6.0, 0.1),
              params={"eq": (("lp", 5000.0),)}),
        Track("paiban", "perc:paiban", clap, level=-17.0, pan=-0.35, send=0.25, human=(4.0, 0.06)),
    )
    return Song("bgm_inn", "客栈", "温暖、烟火气、说书人的茶馆", "F 宫调式（宫 = F）", 88, m, 32, "hall",
                tracks,
                "琵琶主奏（长音轮指）、二胡（A' 接旋律，A'' 在上方三度走支声）、扬琴分解和弦、大阮、木鱼、拍板",
                "A A' B A''（各 8 小节）。A 的四句以宫音起、徵音顿、再回宫音收；B 转到羽音与徵音之间")


SONGS = {
    "bgm_village": village,
    "bgm_town": town,
    "bgm_mountain_path": mountain_path,
    "bgm_cliff": cliff,
    "bgm_river": river,
    "bgm_wild": wild,
    "bgm_inn": inn,
}
