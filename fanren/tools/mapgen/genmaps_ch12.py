# -*- coding: utf-8 -*-
"""第 12 章十张图（docs/ch12-design.md 第 4 节、3.1 节；契约 docs/interfaces-p3-ch12.md 第 4、6 节）。

    python tools/mapgen/genmaps_ch12.py           # 重写十张 tmj 与十个 tileset
    python tools/mapgen/genmaps_ch12.py --check   # 只比对

build_all 是纯函数、不导入别章生成器、不用随机数；genmaps.py 的 check() 在第 11 章之后
接进来。本章不 patch 任何旧图。

十张图与出入口（规则 29「出门往哪走，进门就朝哪」，落点在括号里）：

    ch12_tushan    48x36  土山荒岛：南岸 andeng（默认，C1 落点）→ 北面三格横贯岸路的 trigger_dengdao
                          → 岸路 → 石屋 → 北坡黄雾 → 光罩 → 地洞口；东岸礁滩（文氏父女）。
                          北坡洞口 portal_dixue → ch12_dixue(taijie)，require ch12.tuizhu，往北出；
                          回程落 dongkou（洞口正南一格，朝下）。
    ch12_dixue     30x24  封灵柱地洞：南端 portal_tushan → ch12_tushan(dongkou)，往南出，不设门闸；
                          台阶脚 taijie（默认、朝上）→ 中部水池 → 北壁封灵柱。
    ch12_dadian    44x40  虚天殿外殿大厅：tongdao（默认）→ trigger_rudian 横贯通道 → 两排巨柱 → 北端传送阵。
    ch12_guiwu     56x48  鬼雾：tupo（默认）→ trigger_tupo 横贯坡道 → 一宽的正路、三处岔口、三条错路
                          → 空地咽喉 trigger_guiwang → 花园入口 → 中央亭。
    ch12_xiaojie   64x40  小空间：西端草原 caoyuan（默认）→ 东南小石山、山下树林边 → 东北峡谷口两碑之间。
    ch12_binghuo   80x28  冰火道·熔岩路：西端光壁 guangbi（默认）→ trigger_guangbi 横贯长道 → 熔岩夹道
                          → 草地 → 沼泽 → 黑沙漠西缘一格宽沙口 → 晶柱 → 东端石殿门内一格。
    ch12_baoguang  36x28  宝光阁：玉桥头 yuqiao（默认）→ 玉桥 → 一层（玉台）→ 简易传送阵前一格。
    ch12_taqian    44x32  内殿塔前：西端石室 shishi（默认）→ 石室出口一格 → 塔前空地 → 一格宽石径 → 塔门。
    ch12_hanlitai  40x36  寒骊台：台阶脚南一格 taijiao（默认）；台下一圈、台背青石；台阶是上台唯一通路。
    ch12_lingquan  24x20  灵泉密室：池边 quanbian（默认）。

全章只有荒岛 ↔ 地洞一对门；其余九处脚本 teleport 只传图 id，落各图 default spawn：
andeng / tongdao / tupo / caoyuan / guangbi / yuqiao / shishi / taijiao / quanbian。

鬼雾三处岔口（走偏脚本 zoupian_a/b/c.lua 的落点，照抄进脚本与 kScriptTransfers 注释）：
    第一岔 (27, 33)：正路往西，错路往东再往北，尽头 trigger_zoupian_a (36, 28)
    第二岔 (18, 24)：正路往东，错路往西再往北，尽头 trigger_zoupian_b (8, 16)
    第三岔 (30, 16)：正路往西，错路往东再往南，尽头 trigger_zoupian_c (44, 26)
三处落点格都不压任何对象。

施工偏差见 C:/Users/htx-lyh/Documents/FanrenChapterWorktrees/notes/ch12/deviations-A.md。
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import genmaps
from genmaps import Grid, build_map, check_maps, dump_map, facility, npc, portal, spawn, trigger, write_tileset

REGION = "luanxinghai_xutian"
CHAPTER = 12

# 鬼雾三处岔口（脚本落点）。改这里就要同步改 scripts/ch12/zoupian_{a,b,c}.lua。
FORK_A = (27, 33)
FORK_B = (18, 24)
FORK_C = (30, 16)


def sea_base(w, h):
    """整张海面（gid 3 + 碰撞，不画 building），再用 floor 堆出陆地。"""
    g = Grid(w, h)
    g.fill("ground", 3)
    g.fill("collision", 1)
    return g


def land_base(w, h):
    """整张实地，四周一圈岩体。"""
    g = Grid(w, h)
    g.fill("ground", 1)
    g.solid_rect(0, 0, w - 1, 0)
    g.solid_rect(0, h - 1, w - 1, h - 1)
    g.solid_rect(0, 0, 0, h - 1)
    g.solid_rect(w - 1, 0, w - 1, h - 1)
    return g


def cave(w, h):
    """整张实心，再用 floor 一处处凿出来（殿内、地洞、密室）。"""
    g = Grid(w, h)
    g.fill("ground", 1)
    g.solid_rect(0, 0, w - 1, h - 1)
    return g


def floor(g, x0, y0, x1, y1, gid=1):
    g.open_rect(x0, y0, x1, y1)
    g.rect("ground", x0, y0, x1, y1, gid)


def pool(g, x0, y0, x1, y1):
    """水面 / 熔岩：gid 3 + 碰撞，不画 building（MapArtTests）。"""
    g.open_rect(x0, y0, x1, y1)
    g.rect("ground", x0, y0, x1, y1, 3)
    g.rect("collision", x0, y0, x1, y1, 1)


def hook(g, objects, name, script, xy, mode, guard, done, once=True, w=1, h=1):
    """挂点。interact 那一格画成石栏 / 石台（实心，站旁边按）；enter 留空地。"""
    x, y = xy
    if mode == "interact":
        g.solid_rect(x, y, x + w - 1, y + h - 1, 8)
    objects.append(trigger(name, x, y, "ch12/" + script + ".lua", mode, once=once, w=w, h=h,
                           guard_flag=guard, set_flag=done))


def person(objects, name, role, xy, visible, hidden=None, script=None, facing="down"):
    objects.append(npc(name, *xy, role, facing,
                       script=("ch12/" + script + ".lua") if script else None,
                       visible_flag=visible, hidden_flag=hidden))


def save(objects, short, xy):
    objects.append(facility("facility_cunji_" + short, *xy, "save"))


def meditate(objects, short, xy):
    objects.append(facility("facility_dazuo_" + short, *xy, "meditate", effectiveness=100))


def finish(short, bgm, g, objects, outdoor):
    mid = "ch12_" + short
    props = [("map_id", mid), ("display_name_key", "ch12.map." + short + ".name"),
             ("region", REGION), ("bgm", bgm), ("chapter", CHAPTER),
             ("outdoor", outdoor), ("can_leave_edge", False)]
    return build_map(mid, g.w, g.h, "terrain_ch12_" + short, props, g, objects)


# ---------------------------------------------------------------------------
# 土山荒岛 48x36：南岸登岛、岸路、石屋、北坡黄雾与光罩、地洞口；东岸礁滩
# ---------------------------------------------------------------------------
def make_tushan():
    g = sea_base(48, 36)
    floor(g, 3, 3, 44, 27)                      # 岛身（碎石枯土，少植被）
    floor(g, 12, 29, 36, 32)                    # 南岸滩
    floor(g, 12, 28, 36, 28)
    g.solid_rect(12, 28, 36, 28)                # 岸坎：滩与岛身之间只有一处岸路口
    floor(g, 22, 28, 26, 28, 2)                 # 岸路口（trigger_dengdao 横贯这五格）
    floor(g, 23, 11, 25, 27, 2)                 # 岸路：岸路口直上北坡
    g.solid_rect(3, 3, 44, 10)                  # 北坡山体
    floor(g, 24, 7, 24, 10, 2)                  # 洞口前一格宽的坡道（光罩在它南口）
    floor(g, 24, 6, 24, 6, 2)                   # 地洞口（portal_dixue，只有南面进得去）
    g.solid_rect(14, 17, 20, 21)                # 石屋
    g.rect("front", 14, 16, 20, 16, 4)          # 石屋檐
    g.rect("overlay", 18, 11, 22, 13, 7)        # 坡上黄雾一带的碎石
    for rect in ((41, 15, 42, 16), (38, 20, 39, 21), (43, 23, 44, 24)):
        g.solid_rect(*rect)                     # 东岸礁石
    g.rect("overlay", 37, 14, 44, 22, 7)        # 东岸礁滩
    g.rect("overlay", 13, 30, 20, 32, 7)        # 滩上碎石
    g.rect("overlay", 28, 30, 35, 31, 7)
    g.rect("overlay", 6, 22, 9, 24, 6)          # 岛上仅有的几丛枯草
    objects = [spawn("andeng", 24, 31, "up", True), spawn("dongkou", 24, 7, "down")]
    save(objects, "tushan", (23, 31))
    hook(g, objects, "trigger_dengdao", "dengdao", (22, 28), "enter", "ch11.done", "ch12.dengdao", w=5)
    hook(g, objects, "trigger_huangwu", "pozhen", (21, 12), "interact", "ch12.dengdao", "ch12.pozhen")
    hook(g, objects, "trigger_guaizhao", "guairen", (24, 10), "enter", "ch12.pozhen", "ch12.tuizhu")
    for name, role, xy in (("npc_jin_qing", "jin_qing", (15, 23)), ("npc_hu_yue", "hu_yue", (17, 23)),
                           ("npc_jian_xiushi", "jian_xiushi", (19, 23)), ("npc_shi_die", "shi_die", (18, 25))):
        person(objects, name, role, xy, "ch12.dengdao", "ch12.tuizhu")
    person(objects, "npc_wen_siyue", "wen_siyue", (40, 18), "ch12.canyuan", "ch12.lidao", script="lidao")
    person(objects, "npc_wen_qiang", "wen_qiang_zhuji", (41, 19), "ch12.canyuan", "ch12.lidao")
    objects.append(portal("portal_dixue", 24, 6, "ch12_dixue", "taijie",
                          require_flag="ch12.tuizhu", deny_text_key="ch12.block.dixue"))
    return finish("tushan", "bgm_wild", g, objects, True)


# ---------------------------------------------------------------------------
# 封灵柱地洞 30x24：南端台阶、中部水池、北壁封灵柱
# ---------------------------------------------------------------------------
def make_dixue():
    g = cave(30, 24)
    floor(g, 4, 5, 25, 20)                      # 洞腹
    floor(g, 15, 21, 15, 23, 2)                 # 台阶（南端出口在 y=23）
    floor(g, 16, 21, 16, 21)                    # 台阶旁壁龛（存档点）
    pool(g, 12, 10, 17, 12)                     # 水池
    g.rect("overlay", 14, 11, 15, 11, 6)        # 池中莲
    g.solid_rect(15, 5, 15, 5, 8)               # 封灵柱
    g.rect("overlay", 13, 5, 17, 6, 7)          # 柱脚碎石
    for rect in ((5, 6, 6, 8), (23, 6, 24, 7), (5, 18, 6, 19), (24, 18, 25, 19)):
        g.solid_rect(*rect)                     # 洞壁凸岩
    objects = [spawn("taijie", 15, 21, "up", True)]
    save(objects, "dixue", (16, 21))
    hook(g, objects, "trigger_shuichi", "qixialian", (14, 13), "interact", "ch12.tuizhu", "ch12.qixialian")
    hook(g, objects, "trigger_shizhu", "duoshe", (15, 6), "enter", "ch12.qixialian", "ch12.duoshe")
    person(objects, "npc_jin_qing", "jin_qing", (10, 15), "ch12.tuizhu", "ch12.duoshe")
    for name, role, xy in (("npc_hu_yue", "hu_yue", (11, 17)), ("npc_jian_xiushi", "jian_xiushi", (19, 15)),
                           ("npc_shi_die", "shi_die", (20, 17))):
        person(objects, name, role, xy, "ch12.tuizhu", "ch12.qixialian")
    person(objects, "npc_shaonian", "xuangu_shaonian", (17, 7), "ch12.duoshe", "ch12.canyuan", script="xiaocha")
    objects.append(portal("portal_tushan", 15, 23, "ch12_tushan", "dongkou"))
    return finish("dixue", "bgm_cave", g, objects, False)


# ---------------------------------------------------------------------------
# 虚天殿外殿大厅 44x40：南端通道、长厅两排巨柱、北端石门后的传送阵
# ---------------------------------------------------------------------------
def make_dadian():
    g = cave(44, 40)
    floor(g, 19, 32, 23, 38, 2)                 # 南端通道（五格宽）
    floor(g, 4, 6, 39, 31)                      # 长厅
    floor(g, 17, 6, 25, 31, 2)                  # 厅中石板道
    for y in (7, 11, 15, 19, 23, 27):           # 两排巨柱（每根 2x2）
        g.solid_rect(13, y, 14, y + 1, 8)
        g.solid_rect(28, y, 29, y + 1, 8)
    floor(g, 21, 5, 21, 5, 2)                   # 北端石门
    floor(g, 18, 2, 24, 4)                      # 门后小室
    g.rect("overlay", 19, 3, 23, 4, 7)          # 传送阵阵纹
    objects = [spawn("tongdao", 21, 37, "up", True)]
    save(objects, "dadian", (20, 37))
    meditate(objects, "dadian", (27, 11))
    hook(g, objects, "trigger_rudian", "rudian", (19, 34), "enter", "ch12.lidao", "ch12.rudian", w=5)
    hook(g, objects, "trigger_qunxiang", "qunxiang", (15, 17), "enter", "ch12.jiumeng", "ch12.qunxiang")
    hook(g, objects, "trigger_chuansongzhen", "chuansong", (21, 2), "interact", "ch12.qunxiang", "ch12.chuansong")
    person(objects, "npc_xuan_gu", "xuan_gu", (8, 10), "ch12.rudian", "ch12.chuansong", script="jiumeng")
    person(objects, "npc_zi_ling", "zi_ling_xutian", (35, 9), "ch12.rudian", "ch12.chuansong")
    person(objects, "npc_miao_zhanglao", "miao_zhanglao", (9, 28), "ch12.rudian", "ch12.chuansong")
    for name, role, xy in (("npc_jiyin", "jiyin_zushi", (13, 9)), ("npc_wu_chou", "wu_chou", (12, 9)),
                           ("npc_qingyi", "qingyi_jushi", (13, 13)), ("npc_wen_furen", "wen_furen", (13, 21)),
                           ("npc_wan_tianming", "wan_tianming", (13, 25)), ("npc_tianwuzi", "tianwuzi", (28, 9)),
                           ("npc_heishou", "heishou_laozhe", (28, 13)), ("npc_man_huzi", "man_huzi", (28, 17)),
                           ("npc_zhifa_ci", "xinggong_zhifa_ci", (28, 21)),
                           ("npc_zhifa_leng", "xinggong_zhifa_leng", (29, 21))):
        person(objects, name, role, xy, "ch12.qunxiang", "ch12.chuansong")
    return finish("dadian", "bgm_indoor", g, objects, False)


# ---------------------------------------------------------------------------
# 鬼雾 56x48：南端土坡；雾里一格宽的正路、三处岔口、三条错路；空地咽喉；北端花园与亭
# ---------------------------------------------------------------------------
def make_guiwu():
    g = cave(56, 48)                            # 浓雾一律实心，只凿出路来
    floor(g, 24, 39, 30, 46)                    # 土坡（白光内）
    g.rect("overlay", 24, 45, 25, 46, 6)
    g.rect("overlay", 29, 45, 30, 46, 6)
    # 正路（gid 2：啼魂兽开出的、雾色发淡的通道）
    floor(g, 27, 34, 27, 38, 2)
    floor(g, 18, 33, 27, 33, 2)                 # 第一岔 (27,33) → 往西
    floor(g, 18, 25, 18, 32, 2)
    floor(g, 18, 24, 30, 24, 2)                 # 第二岔 (18,24) → 往东
    floor(g, 30, 17, 30, 23, 2)
    floor(g, 22, 16, 30, 16, 2)                 # 第三岔 (30,16) → 往西
    floor(g, 22, 15, 22, 15, 2)
    # 错路（gid 1，雾浓、无白骨）
    floor(g, 28, 33, 36, 33)
    floor(g, 36, 28, 36, 32)                    # 尽头 trigger_zoupian_a (36,28)
    floor(g, 8, 24, 17, 24)
    floor(g, 8, 16, 8, 23)                      # 尽头 trigger_zoupian_b (8,16)
    floor(g, 31, 16, 44, 16)
    floor(g, 44, 17, 44, 26)                    # 尽头 trigger_zoupian_c (44,26)
    floor(g, 31, 20, 33, 20)                    # 正路旁死角（玉真人遗骸在 (34,20)）
    for xy in ((27, 36), (26, 33), (18, 29), (19, 24), (30, 20), (29, 16), (22, 15)):
        g.set("overlay", *xy, 7)                # 正路上的白骨堆
    floor(g, 19, 12, 25, 14)                    # 第三岔之后的空地
    g.rect("overlay", 20, 12, 24, 13, 7)
    floor(g, 22, 9, 22, 11, 2)                  # 空地北口：一格宽咽喉（trigger_guiwang 在 (22,11)）
    floor(g, 22, 8, 22, 8, 2)                   # 花园入口
    floor(g, 12, 2, 43, 7)                      # 花园
    g.rect("overlay", 13, 6, 20, 7, 6)
    g.rect("overlay", 33, 6, 42, 7, 6)
    g.rect("front", 27, 3, 29, 3, 4)            # 亭檐
    objects = [spawn("tupo", 27, 45, "up", True)]
    save(objects, "guiwu", (26, 45))
    hook(g, objects, "trigger_tupo", "tupo", (24, 42), "enter", "ch12.chuansong", "ch12.tupo", w=7)
    for name, script, xy in (("trigger_zoupian_a", "zoupian_a", (36, 28)),
                             ("trigger_zoupian_b", "zoupian_b", (8, 16)),
                             ("trigger_zoupian_c", "zoupian_c", (44, 26))):
        hook(g, objects, name, script, xy, "enter", "ch12.tupo", None, once=False)
    hook(g, objects, "trigger_yihai", "yihai", (34, 20), "interact", "ch12.tupo", "ch12.z1")
    hook(g, objects, "trigger_guiwang", "guiwang", (22, 11), "enter", "ch12.tupo", "ch12.guiwang")
    hook(g, objects, "trigger_huayuan", "yuanyao", (22, 8), "enter", "ch12.guiwang", "ch12.yuanyao")
    hook(g, objects, "trigger_tingzi", "xiaojie", (28, 4), "interact", "ch12.yuanyao", "ch12.xiaojie")
    person(objects, "npc_zi_ling", "zi_ling_xutian", (25, 40), "ch12.tupo", "ch12.guiwang")
    person(objects, "npc_ge_li", "ge_li", (29, 40), "ch12.tupo", "ch12.guiwang")
    person(objects, "npc_zi_ling_hy", "zi_ling_xutian", (19, 4), "ch12.guiwang", "ch12.xiaojie")
    for name, role, xy in (("npc_yuan_yao", "yuan_yao_jiedan", (25, 4)), ("npc_xuan_gu", "xuan_gu", (32, 3)),
                           ("npc_jiyin", "jiyin_zushi", (34, 4)), ("npc_wu_chou", "wu_chou", (35, 5)),
                           ("npc_qingyi", "qingyi_jushi", (37, 4)), ("npc_wan_tianming", "wan_tianming", (39, 3)),
                           ("npc_man_huzi", "man_huzi", (40, 5)), ("npc_zhifa_ci", "xinggong_zhifa_ci", (15, 3)),
                           ("npc_zhifa_leng", "xinggong_zhifa_leng", (16, 5))):
        person(objects, name, role, xy, "ch12.yuanyao", "ch12.xiaojie")
    return finish("guiwu", "bgm_night", g, objects, True)


# ---------------------------------------------------------------------------
# 小空间 64x40：西端草原、东南小石山与山下树林、东北峡谷口两碑
# ---------------------------------------------------------------------------
def make_xiaojie():
    g = land_base(64, 40)
    g.solid_rect(48, 1, 62, 8)                  # 东北山体
    floor(g, 55, 6, 55, 8, 2)                   # 峡谷道
    g.solid_rect(54, 9, 54, 9, 8)               # 两块石碑
    g.solid_rect(56, 9, 56, 9, 8)
    floor(g, 55, 9, 55, 9, 2)                   # 两碑之间（trigger_xiagu）
    g.solid_rect(42, 30, 48, 35)                # 东南小石山
    g.solid_rect(30, 33, 39, 37)                # 山下树林
    g.rect("front", 30, 32, 39, 32, 4)          # 林冠
    pool(g, 24, 24, 28, 26)                     # 草原上一汪小泉
    g.rect("overlay", 2, 4, 20, 12, 6)          # 草原
    g.rect("overlay", 2, 26, 18, 36, 6)
    g.rect("overlay", 8, 13, 10, 15, 6)         # 灵药丛（只此一丛可采）
    g.rect("overlay", 14, 16, 15, 17, 6)        # 装饰药丛
    g.rect("overlay", 40, 36, 46, 37, 7)        # 石山脚碎石
    objects = [spawn("caoyuan", 3, 20, "right", True)]
    save(objects, "xiaojie", (3, 19))
    hook(g, objects, "trigger_caiyao", "caiyao", (9, 14), "interact", "ch12.xiaojie", "ch12.caiyao")
    hook(g, objects, "trigger_shishan", "lingshen", (45, 29), "interact", "ch12.xiaojie", "ch12.lingshen")
    hook(g, objects, "trigger_fanlian", "fanlian", (41, 32), "enter", "ch12.lingshen", "ch12.fanlian")
    hook(g, objects, "trigger_xiagu", "xiagu", (55, 9), "enter", "ch12.baishi", "ch12.xiagu")
    person(objects, "npc_xuan_gu", "xuan_gu", (47, 28), "ch12.xiaojie", "ch12.lingshen")
    person(objects, "npc_jiyin", "jiyin_zushi", (40, 28), "ch12.fanlian", "ch12.baishi", script="baishi")
    person(objects, "npc_wu_chou", "wu_chou", (39, 29), "ch12.fanlian", "ch12.baishi")
    person(objects, "npc_qingyi", "qingyi_jushi", (38, 28), "ch12.fanlian", "ch12.baishi")
    for name, role, xy in (("npc_jiyin_xg", "jiyin_zushi", (52, 11)), ("npc_wu_chou_xg", "wu_chou", (51, 12)),
                           ("npc_qingyi_xg", "qingyi_jushi", (53, 12)), ("npc_man_huzi", "man_huzi", (57, 11)),
                           ("npc_wan_tianming", "wan_tianming", (58, 12)), ("npc_tianwuzi", "tianwuzi", (59, 11)),
                           ("npc_heishou", "heishou_laozhe", (60, 12)), ("npc_yuan_yao", "yuan_yao_jiedan", (50, 10))):
        person(objects, name, role, xy, "ch12.baishi", "ch12.xiagu")
    return finish("xiaojie", "bgm_valley", g, objects, True)


# ---------------------------------------------------------------------------
# 冰火道·熔岩路 80x28：西 → 东一条长道（光壁、熔岩夹道、草地、沼泽、黑沙漠、石殿）
# ---------------------------------------------------------------------------
def make_binghuo():
    g = land_base(80, 28)
    g.solid_rect(1, 1, 78, 9)                   # 北侧崖壁
    g.solid_rect(1, 18, 78, 26)                 # 南侧崖壁
    g.rect("overlay", 1, 10, 1, 17, 7)          # 光壁脚下
    pool(g, 9, 10, 24, 11)                      # 熔岩夹道
    pool(g, 9, 16, 24, 17)
    g.rect("overlay", 25, 10, 39, 17, 6)        # 草地
    pool(g, 42, 10, 45, 12)                     # 沼泽里的泥潭
    pool(g, 48, 15, 51, 17)
    g.rect("overlay", 40, 13, 53, 14, 6)
    g.solid_rect(55, 10, 55, 12)                # 黑沙漠西缘的沙丘：只留一格沙口
    g.solid_rect(55, 14, 55, 17)
    g.rect("overlay", 56, 10, 68, 17, 7)        # 黑沙漠
    g.solid_rect(70, 10, 70, 17)                # 石殿西墙
    floor(g, 70, 13, 70, 13, 2)                 # 石殿门
    floor(g, 71, 10, 78, 17)                    # 石殿
    g.rect("front", 70, 9, 78, 9, 4)            # 殿檐
    objects = [spawn("guangbi", 3, 13, "right", True)]
    save(objects, "binghuo", (3, 12))
    meditate(objects, "binghuo", (3, 14))
    hook(g, objects, "trigger_guangbi", "rongyan", (6, 10), "enter", "ch12.xiagu", "ch12.rongyan", h=8)
    hook(g, objects, "trigger_shamo", "tihun", (55, 13), "enter", "ch12.rongyan", "ch12.tihun")
    hook(g, objects, "trigger_jingzhu", "lianjing", (62, 13), "interact", "ch12.tihun", "ch12.lianjing")
    hook(g, objects, "trigger_shidian", "shidian", (71, 13), "enter", "ch12.lianjing", "ch12.shidian")
    for name, role, xy in (("npc_jiyin", "jiyin_zushi", (73, 10)), ("npc_wu_chou", "wu_chou", (74, 10)),
                           ("npc_qingyi", "qingyi_jushi", (75, 10)), ("npc_xuan_gu", "xuan_gu", (77, 11)),
                           ("npc_man_huzi", "man_huzi", (73, 16)), ("npc_wan_tianming", "wan_tianming", (74, 16)),
                           ("npc_tianwuzi", "tianwuzi", (75, 16)), ("npc_heishou", "heishou_laozhe", (76, 16)),
                           ("npc_zhifa_ci", "xinggong_zhifa_ci", (77, 14)),
                           ("npc_zhifa_leng", "xinggong_zhifa_leng", (77, 15)),
                           ("npc_yuan_yao", "yuan_yao_jiedan", (74, 13))):
        person(objects, name, role, xy, "ch12.lianjing", "ch12.shidian")
    return finish("binghuo", "bgm_cliff", g, objects, True)


# ---------------------------------------------------------------------------
# 宝光阁 36x28：南端玉桥头、玉桥与匾额、一层玉台、去二层的简易传送阵
# ---------------------------------------------------------------------------
def make_baoguang():
    g = cave(36, 28)
    floor(g, 15, 24, 21, 26)                    # 玉桥头
    floor(g, 17, 17, 19, 23, 2)                 # 玉桥
    pool(g, 10, 17, 16, 23)                     # 桥下池水
    pool(g, 20, 17, 26, 23)
    floor(g, 6, 5, 29, 16)                      # 一层
    g.rect("front", 16, 16, 20, 16, 4)          # 匾额
    g.solid_rect(18, 7, 18, 7, 8)               # 简易传送阵的阵台
    g.rect("overlay", 17, 5, 19, 6, 7)          # 阵纹
    g.rect("overlay", 9, 8, 13, 11, 7)          # 玉台四周
    objects = [spawn("yuqiao", 18, 25, "up", True)]
    save(objects, "baoguang", (17, 25))
    hook(g, objects, "trigger_yutai", "yutai", (11, 9), "interact", None, None, once=False)
    hook(g, objects, "trigger_yuqiao", "quba", (18, 8), "enter", "ch12.shidian", "ch12.quba")
    return finish("baoguang", "bgm_indoor", g, objects, False)


# ---------------------------------------------------------------------------
# 内殿塔前 44x32：西端石室、石室出口、塔前空地、一格宽石径、五层青石塔门
# ---------------------------------------------------------------------------
def make_taqian():
    g = cave(44, 32)
    floor(g, 2, 12, 9, 19)                      # 石室
    floor(g, 10, 15, 15, 15, 2)                 # 石室出口与短廊（出口一格 trigger_changlang）
    floor(g, 16, 12, 40, 28)                    # 塔前空地
    floor(g, 30, 8, 30, 11, 2)                  # 塔前一格宽石径
    g.solid_rect(26, 2, 34, 7)                  # 五层青石塔
    g.rect("front", 26, 1, 34, 1, 4)            # 塔檐
    g.rect("overlay", 17, 26, 39, 27, 7)
    objects = [spawn("shishi", 5, 15, "right", True)]
    save(objects, "taqian", (5, 14))
    meditate(objects, "taqian", (5, 16))
    hook(g, objects, "trigger_changlang", "huanjing", (10, 15), "enter", "ch12.quba", "ch12.huanjing")
    hook(g, objects, "trigger_taqian", "neidian", (30, 10), "enter", "ch12.huanjing", "ch12.neidian")
    hook(g, objects, "trigger_tamen", "cengjie", (30, 7), "interact", "ch12.neidian", "ch12.cengjie")
    for name, role, xy in (("npc_jiyin_tq", "jiyin_zushi", (24, 13)), ("npc_wu_chou_tq", "wu_chou", (25, 14)),
                           ("npc_qingyi_tq", "qingyi_jushi", (23, 14)), ("npc_man_huzi_tq", "man_huzi", (35, 13)),
                           ("npc_wan_tianming_tq", "wan_tianming", (36, 14)),
                           ("npc_tianwuzi_tq", "tianwuzi", (37, 13)),
                           ("npc_heishou_tq", "heishou_laozhe", (38, 14)),
                           ("npc_xuan_gu_tq", "xuan_gu", (33, 17)),
                           ("npc_yuan_yao_tq", "yuan_yao_jiedan", (27, 17))):
        person(objects, name, role, xy, "ch12.huanjing", "ch12.neidian")
    return finish("taqian", "bgm_cave", g, objects, False)


# ---------------------------------------------------------------------------
# 寒骊台 40x36：台下一圈、台背青石；台阶是上台唯一通路；台上鼎台、洞口、另一角
# ---------------------------------------------------------------------------
def make_hanlitai():
    g = cave(40, 36)
    floor(g, 3, 2, 36, 33)                      # 台下一圈
    g.solid_rect(9, 6, 30, 24)                  # 台身（四面石壁）
    floor(g, 10, 7, 29, 23)                     # 台面
    floor(g, 20, 24, 20, 24, 2)                 # 台阶顶（台壁上唯一的缺口）
    g.rect("ground", 20, 25, 20, 28, 2)         # 台阶
    g.solid_rect(19, 9, 21, 10, 8)              # 祭坛
    g.solid_rect(11, 8, 12, 9)                  # 洞口（岩隙）
    g.rect("overlay", 10, 10, 13, 11, 7)
    g.rect("overlay", 17, 3, 23, 4, 7)          # 台背碎石
    objects = [spawn("taijiao", 20, 29, "up", True)]
    save(objects, "hanlitai", (19, 29))
    meditate(objects, "hanlitai", (21, 29))
    hook(g, objects, "trigger_qingshi", "juanzhou", (20, 4), "interact", "ch12.cengjie", "ch12.juanzhou")
    hook(g, objects, "trigger_taijie", "ruyi", (20, 24), "enter", "ch12.juanzhou", "ch12.ruyi")
    hook(g, objects, "trigger_jitan", "butian", (20, 12), "enter", "ch12.ruyi", "ch12.butian")
    hook(g, objects, "trigger_dongkou", "xuangu", (13, 9), "interact", "ch12.butian", "ch12.xuangu")
    hook(g, objects, "trigger_xiaojian", "canhun", (28, 22), "interact", "ch12.xuangu", "ch12.canhun")
    for name, role, xy in (("npc_jiyin_hl", "jiyin_zushi", (15, 14)), ("npc_qingyi_hl", "qingyi_jushi", (16, 15)),
                           ("npc_man_huzi_hl", "man_huzi", (24, 14)),
                           ("npc_wan_tianming_hl", "wan_tianming", (25, 15)),
                           ("npc_tianwuzi_hl", "tianwuzi", (26, 14)),
                           ("npc_heishou_hl", "heishou_laozhe", (24, 17))):
        person(objects, name, role, xy, "ch12.cengjie", "ch12.butian")
    person(objects, "npc_wu_chou_hl", "wu_chou", (15, 18), "ch12.cengjie", "ch12.xuangu")
    person(objects, "npc_xuan_gu_hl", "xuan_gu", (17, 18), "ch12.cengjie", "ch12.xuangu")
    return finish("hanlitai", "bgm_cliff", g, objects, False)


# ---------------------------------------------------------------------------
# 灵泉密室 24x20：池与池边
# ---------------------------------------------------------------------------
def make_lingquan():
    g = cave(24, 20)
    floor(g, 3, 3, 20, 16)
    pool(g, 9, 6, 14, 10)                       # 灵泉
    g.rect("overlay", 8, 5, 15, 5, 6)
    g.rect("overlay", 4, 14, 8, 15, 7)
    objects = [spawn("quanbian", 12, 11, "up", True)]
    save(objects, "lingquan", (11, 11))
    person(objects, "npc_yuan_yao_lq", "yuan_yao_jiedan", (15, 9), "ch12.canhun", script="lingquan")
    return finish("lingquan", "bgm_cave", g, objects, False)


BUILDERS = (make_tushan, make_dixue, make_dadian, make_guiwu, make_xiaojie,
            make_binghuo, make_baoguang, make_taqian, make_hanlitai, make_lingquan)


def build_all():
    built = {}
    for builder in BUILDERS:
        payload = builder()
        mid = next(p["value"] for p in payload["properties"] if p["name"] == "map_id")
        built[mid] = payload
    return built


def main(argv=None):
    args = sys.argv[1:] if argv is None else argv
    if "--check" in args:
        return check_maps(build_all())
    for mid, payload in build_all().items():
        write_tileset("terrain_" + mid)
        dump_map(os.path.join(genmaps.MAPS, mid + ".tmj"), payload)
        print("wrote", mid)
    return 0


if __name__ == "__main__":
    sys.exit(main())
