# -*- coding: utf-8 -*-
"""第 6 章地图生成器：重跑即重建下列六张图与六个 tileset，并给嘉元城南城补一道东门。

    python tools/mapgen/genmaps_ch06.py
    python tools/mapgen/genmaps_ch06.py --check   # 只比对，不写盘；有漂移则退出码 1
    python tools/validate.py                      # 自检

    （`genmaps.py --check` 会把本章这六张连同南城一并盯上，build.bat 里仍只有那一条。）

产出（全部写进 maps/，不经手 scripts/ 与 data/）：

    maps/ch06_tainan_cun.tmj     40x30  太南山脚小村（节点 1、2）
    maps/ch06_tainan_gu.tmj      48x36  太南谷（节点 3–6、8、10a、11）
    maps/ch06_sanxiu_lou.tmj     32x24  散修小楼（节点 7、9、10b）
    maps/ch06_shanqiu.tmj        40x30  百里荒丘（节点 12）
    maps/ch06_huangfenggu.tmj    48x36  黄枫谷外门（节点 13–17）
    maps/ch06_baiyaoyuan.tmj     32x24  百药园（节点 18）
    maps/tilesets/terrain_{tainan_cun,tainan_gu,sanxiu_lou,shanqiu,huangfenggu,baiyaoyuan}.tsj

    另就地修补 maps/ch05_nancheng.tmj（见第五节）。

施工图：docs/ch06-design.md。挂点的 mode / once / guard_flag / set_flag 一律照它 3.1 那张表，
这份文件只回答「摆在哪一格」。

--------------------------------------------------------------------------
一、连通与闸门（施工图第 4 节）

    ch05_nancheng ──(ch05.done)──▶ ch06_tainan_cun ──teleport(2)──▶ ch06_tainan_gu
    ch06_tainan_gu ◀──(ch06.ruhuo)──▶ ch06_sanxiu_lou
    ch06_tainan_gu ──teleport(11)──▶ ch06_shanqiu ──teleport(12 胜)──▶ ch06_huangfenggu
    ch06_huangfenggu ◀──(ch06.huinuo)──▶ ch06_baiyaoyuan

    三道闸门各指一张不同的图，deny_text_key 按目标图命名（规则 20）：
    ch06.block.tainan_cun / ch06.block.sanxiu_lou / ch06.block.baiyaoyuan。
    山脚没有回南城的门、谷没有回山脚的门、山丘没有任何门、黄枫谷没有回山丘的门：
    四处都只靠剧情 teleport 进，规则 18 把 teleport() 算作一条边。
    迎宾楼的「禁法」只在旁白里说，不做门（施工图第 4 节末段的取舍）。

二、每一处挂战斗的格子前面都有存档点（施工图第 4 节）

    ① 叶家寻衅  太南谷：facility_cunji 在楼阁与广场之间，摊位区入口在它南边
    ② 山丘袭杀  百里荒丘：facility_cunji 就在落点旁
    ③ 吴风切磋  黄枫谷：facility_cunji 在石屋群，传功阁在它北边

三、踏入型卡在咽喉上，一格只挂一个对象

    enter
      trigger_chudu      山脚：出生点东边一格（林中小径两格宽，出生就踩上）
      trigger_cunkou     山脚：林子与村子之间那道土坡上唯一的豁口
      trigger_qingyan    谷口两格宽的雾道，落点往北第一格
      trigger_sanhui     同一条雾道，靠北三格（11 散会时从广场往南走必踩）
      trigger_ruhuo      雾道北口
      trigger_lingshi    摊位区（「回」字外圈）唯一的口子
      trigger_ye_xunxin  口子里头一格（5b 换完飞行符出摊位区必踩）
      trigger_shuangshou 小楼门前通广场的路上
      trigger_canpian    同一条路靠广场那头
      trigger_xisha      荒丘凹地唯一的口子
      trigger_ce_linggen 迎宾楼房间的门（落点往北两格）
      trigger_jinzhi     百药园西口两山之间的夹道
    interact 全部贴在一件东西上：界石、门、桌、床、蒲团、柜台、卷宗架；
    只有 trigger_maiping 在药田角上那一格翻土上（人站在田埂上朝它按）。

四、剧情传送的落点（脚本照这里写，改了要一起改）

    GU_GUKOU        (23, 33)  太南谷谷口雾道底（scripts/ch06/guaipo.lua）
    SHANQIU_LUODIAN (4, 25)   荒丘西南的落脚处（scripts/ch06/sanhui.lua）
    HFG_YINGBIN     (8, 30)   黄枫谷迎宾楼的厅里（scripts/ch06/xisha.lua）

五、南城东门：就地修补，不重建

    南城的地形归 genmaps_ch05.py 所有。本文件只把东墙 (47,17)(47,18) 两格开成路，追加一道
    portal_to_tainan_cun（require_flag=ch05.done），其余原样。南陵街 y 16..20 本来就铺到 x=46，
    出门往东走、进山脚林子也朝东（规则 29）。幂等。
    genmaps.py 的 check() 在收齐各章产出之后回头调用 patch_nancheng()。

六、瓦片 gid 约定沿用 genmaps.py：1 主地表 2 道路 3 特殊地表 4 front 5 building 主体
    6/7 overlay 8 building 次体（家具、摊架、门扇、界石）。
"""
import json
import os
import random
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import genmaps  # noqa: E402  MAPS 要运行时取
from genmaps import (  # noqa: E402
    Grid,
    build_map,
    check_maps,
    dump_map,
    facility,
    npc,
    portal,
    spawn,
    trigger,
    write_tileset,
)
from genmaps_ch05 import border, eaves_row, encounter, floor, road, scatter, tilled, tree  # noqa: E402

CHAPTER = 6
PREFIX = "ch06_"

GU_GUKOU = (23, 33)
SHANQIU_LUODIAN = (4, 25)
HFG_YINGBIN = (8, 30)


def map_props(map_id, bgm, region, outdoor=True):
    return [
        ("map_id", map_id),
        # 规则 21：display_name_key 由地图 id 推出来。
        ("display_name_key", "ch06.map.%s.name" % map_id[len(PREFIX):]),
        ("region", region),           # 施工图第 4 节：山脚、谷、小楼、山丘在岚州；黄枫谷、百药园在建州
        ("bgm", bgm),
        ("chapter", CHAPTER),
        ("outdoor", outdoor),
        ("can_leave_edge", False),
    ]


def edge_fronts(g, rnd, chance=0.3):
    """挡路的实体边上压一层 front（坡檐、树影），只动 front，不影响通行。"""
    for y in range(g.h):
        for x in range(g.w):
            if not g.walkable(x, y) and g.get("ground", x, y) != 3 and any(
                    g.walkable(x + dx, y + dy) for dx, dy in ((0, 1), (0, -1), (1, 0), (-1, 0))):
                if rnd.random() < chance:
                    g.set("front", x, y, 4)


# ---------------------------------------------------------------------------
# 地图 1：太南山脚小村 40x30 —— 节点 1、2
#
# 西半是林子，一条两格宽的小径从西边进来（南城东门出来就落在这里），穿过林子东缘那道
# 土坡上唯一的豁口进村；村子北头是一道常年罩着白雾的坡，坡脚立一块界石——怪坡。
#
#   1a trigger_chudu   小径上、出生点东边一格（enter + once）：一睁眼就是除毒最后一天
#   1b trigger_cunkou  土坡豁口（enter + once，guard ch06.chudu）：刚进村就撞见万小山
#   2  trigger_guaipo  界石（interact + once，guard ch06.wan_met）：人站在界石南边朝北按
#   存档点在村口。
# ---------------------------------------------------------------------------
def make_tainan_cun():
    W, H = 40, 30
    g = Grid(W, H)
    rnd = random.Random(601)
    g.fill("ground", 1)
    border(g)

    # 林子东缘的土坡：两格厚，豁口在 y 20..21
    g.solid_rect(14, 1, 15, 28, 5)
    # 北头怪坡：整条坡是挡路的山体，坡脚一块界石
    g.solid_rect(16, 1, 38, 5, 5)

    # 林中小径（西口到豁口）、村里的横街与往北的坡道
    road(g, 1, 20, 38, 21)
    road(g, 24, 6, 25, 28)
    g.solid_rect(24, 5, 25, 5, 8)        # 界石（两格宽）

    # 林子：树干挡一格，树冠压 front；小径两侧与出生点那棵巨树
    tree(g, 3, 17)                       # 出生点旁那棵巨树（1a 在它底下醒来）
    for tx, ty in ((6, 3), (10, 5), (3, 8), (8, 10), (12, 13), (5, 13), (10, 17),
                   (7, 24), (3, 26), (11, 25), (12, 8), (2, 4), (9, 27), (6, 16)):
        tree(g, tx, ty)

    # 村里四户人家（门都朝着横街或坡道）
    g.solid_rect(18, 11, 22, 14, 5)      # 借住的老农家
    eaves_row(g, 18, 22, 10)
    g.solid_rect(28, 11, 32, 14, 5)
    eaves_row(g, 28, 32, 10)
    g.solid_rect(31, 23, 35, 26, 5)
    eaves_row(g, 31, 35, 22)
    g.solid_rect(17, 24, 21, 27, 5)
    eaves_row(g, 17, 21, 23)
    g.solid_rect(27, 24, 27, 24, 8)      # 井台
    for tx, ty in ((35, 9), (37, 17), (37, 27)):
        tree(g, tx, ty)

    edge_fronts(g, rnd)
    scatter(g, rnd, 130, 1, 1, W - 2, H - 2)

    objects = [
        spawn("spawn_from_nancheng", 1, 20, "right", default=True),

        # ---- 节点 1a：除毒（出生点东边一格，出生即踩）----
        trigger("trigger_chudu", 2, 20, "ch06/chudu.lua", "enter", True, h=2,
                set_flag="ch06.chudu"),
        # ---- 节点 1b：村口遇万小山（土坡豁口）----
        trigger("trigger_cunkou", 14, 20, "ch06/cunkou.lua", "enter", True, h=2,
                guard_flag="ch06.chudu", set_flag="ch06.wan_met"),
        # ---- 节点 2：怪坡前（界石）----
        trigger("trigger_guaipo", 24, 5, "ch06/guaipo.lua", "interact", True, w=2,
                guard_flag="ch06.wan_met", set_flag="ch06.rugu"),

        facility("facility_cunji", 17, 18, "save"),

        # 村民一人一个 role（规则 30）；借住的老农说得上话，另两个只在路边
        npc("npc_laonong", 21, 16, "tainan_laonong", "down", "ch06/laonong.lua"),
        npc("npc_tiaoshui_fu", 30, 17, "tainan_tiaoshui_fu", "left"),
        npc("npc_cunkou_haizi", 23, 22, "tainan_cunkou_haizi", "up"),
        # 万小山：1b 撞见之后站在村口；2 跟他进了谷，这张图上就没有他了
        npc("npc_wan_xiaoshan", 20, 19, "wan_xiaoshan", "left", "ch06/wan.lua",
            visible_flag="ch06.wan_met", hidden_flag="ch06.rugu"),
    ]
    return build_map("ch06_tainan_cun", W, H, "terrain_tainan_cun",
                     map_props("ch06_tainan_cun", "bgm_mountain_path", "lanzhou"), g, objects)


# ---------------------------------------------------------------------------
# 地图 2：太南谷 48x36 —— 节点 3、4、5、6、8、10a、11
#
# 三面靠山。南边谷口是一条两格宽的雾道（通音符开出来的那条小路），怪坡那边 teleport
# 进来落在雾道底。雾道北口往北一条石板大道直通楼阁；大道西边是「回」字形的摊位区，
# 外圈一圈摊架只在东面开一道口子；大道东边是散修小楼，门朝南。
#
#   3  trigger_qingyan    雾道里落点往北第一格（enter + once，guard ch06.rugu）
#   11 trigger_sanhui     雾道靠北（enter + once，guard ch06.shengxianling）：散会后往南出谷必踩
#   4  trigger_ruhuo      雾道北口（enter + once，guard ch06.qingyan）
#   5a trigger_lingshi    摊位区口子（enter + once，guard ch06.ruhuo）
#   5b npc_caomao_qingnian 草帽青年的摊（npc 脚本自己判 ch06.lingshi / ch06.feixingfu）
#   6  trigger_ye_xunxin  口子里头一格（enter + once，guard ch06.feixingfu）
#   8a trigger_shuangshou 小楼门前的路（enter + once，guard ch06.yishi）
#   8b npc_maifu_shaonv   卖符少女的摊（npc 脚本自己判 ch06.shuangshou / ch06.jinzhubi）
#   10a trigger_canpian   同一条路靠广场那头（enter + once，guard ch06.jiuceng）
#   小楼的门：portal_to_sanxiu_lou，require_flag=ch06.ruhuo。
# ---------------------------------------------------------------------------
def make_tainan_gu():
    W, H = 48, 36
    g = Grid(W, H)
    rnd = random.Random(602)
    g.fill("ground", 1)
    border(g)

    # 三面靠山
    g.solid_rect(1, 1, 46, 2, 5)
    g.solid_rect(1, 1, 3, 34, 5)
    g.solid_rect(44, 1, 46, 34, 5)
    # 谷口：南边整片是雾里的山脚，只留两格宽的雾道
    g.solid_rect(4, 26, 22, 34, 5)
    g.solid_rect(25, 26, 43, 34, 5)
    road(g, 23, 26, 24, 34)

    # 楼阁（北），楼前一道石坪
    g.solid_rect(16, 3, 31, 8, 5)
    road(g, 16, 9, 31, 10)
    # 石板大道：雾道北口一直通到楼阁
    road(g, 23, 11, 24, 25)

    # 「回」字摊位区：外圈摊架一圈，只在东面 (20,18)(20,19) 开口；里圈一块摊架
    g.rect("ground", 5, 12, 20, 25, 2)
    g.solid_rect(5, 12, 20, 12, 8)
    g.solid_rect(5, 25, 20, 25, 8)
    g.solid_rect(5, 12, 5, 25, 8)
    g.solid_rect(20, 12, 20, 25, 8)
    g.open_rect(20, 18, 20, 19)
    g.solid_rect(10, 17, 15, 20, 8)
    road(g, 21, 18, 22, 19)              # 大道到口子

    # 散修小楼（东），门朝南 (38,18)(39,18)
    g.solid_rect(35, 12, 42, 18, 5)
    eaves_row(g, 35, 42, 11)
    g.open_rect(38, 18, 39, 18)
    g.rect("ground", 38, 18, 39, 18, 2)
    road(g, 25, 19, 39, 20)              # 小楼门前到大道

    # 白石灯盏：大道两侧四盏
    for lx, ly in ((22, 13), (25, 13), (22, 23), (25, 23)):
        g.solid_rect(lx, ly, lx, ly, 8)

    for tx, ty in ((8, 5), (12, 8), (33, 5), (40, 8), (29, 25), (38, 24), (6, 9), (42, 22)):
        tree(g, tx, ty)

    edge_fronts(g, rnd)
    scatter(g, rnd, 120, 4, 3, 43, 25)

    objects = [
        spawn("spawn_gukou", GU_GUKOU[0], GU_GUKOU[1], "up", default=True),
        spawn("spawn_from_sanxiu_lou", 38, 19, "down"),
        portal("portal_to_sanxiu_lou", 38, 18, "ch06_sanxiu_lou", "spawn_from_tainan_gu", w=2,
               require_flag="ch06.ruhuo", deny_text_key="ch06.block.sanxiu_lou"),

        # ---- 节点 3：青颜真人（雾道里第一格）----
        trigger("trigger_qingyan", 23, 32, "ch06/qingyan.lua", "enter", True, w=2,
                guard_flag="ch06.rugu", set_flag="ch06.qingyan"),
        # ---- 节点 11：散会（雾道靠北，往南出谷必踩）----
        trigger("trigger_sanhui", 23, 29, "ch06/sanhui.lua", "enter", True, w=2,
                guard_flag="ch06.shengxianling", set_flag="ch06.chugu"),
        # ---- 节点 4：入伙（雾道北口）----
        trigger("trigger_ruhuo", 23, 26, "ch06/ruhuo.lua", "enter", True, w=2,
                guard_flag="ch06.qingyan", set_flag="ch06.ruhuo"),
        # ---- 节点 5a：灵石与灵符（摊位区唯一的口子）----
        trigger("trigger_lingshi", 20, 18, "ch06/lingshi.lua", "enter", True, h=2,
                guard_flag="ch06.ruhuo", set_flag="ch06.lingshi"),
        # ---- 节点 6：叶家寻衅（口子里头一格）----
        trigger("trigger_ye_xunxin", 19, 18, "ch06/xunxin.lua", "enter", True, h=2,
                guard_flag="ch06.feixingfu", set_flag="ch06.xunxin"),
        # ---- 节点 8a：双首鹜（小楼门前的路）----
        trigger("trigger_shuangshou", 31, 19, "ch06/shuangshou.lua", "enter", True, h=2,
                guard_flag="ch06.yishi", set_flag="ch06.shuangshou"),
        # ---- 节点 10a：法宝残片（同一条路靠广场那头）----
        trigger("trigger_canpian", 27, 19, "ch06/canpian.lua", "enter", True, h=2,
                guard_flag="ch06.jiuceng", set_flag="ch06.canpian"),

        facility("facility_cunji", 26, 11, "save"),
        # 坊市（施工图第 7 节）：摊位区南排当中那两格摊架
        facility("facility_fangshi", 12, 25, "shop", w=2, ref_id="ch06_tainan_fangshi"),

        npc("npc_qingyan_zhenren", 21, 10, "qingyan_zhenren", "down", "ch06/qingyan_npc.lua"),
        # ---- 节点 5b：草帽青年（换完收摊）----
        npc("npc_caomao_qingnian", 7, 13, "caomao_qingnian", "down", "ch06/caomao.lua",
            hidden_flag="ch06.feixingfu"),
        # ---- 节点 8b：卖符少女（换完不撤场；出谷后撤——第 7 章施工图要她在第 6 章之后离开这张图，ch06.chugu）----
        npc("npc_maifu_shaonv", 13, 16, "maifu_shaonv", "down", "ch06/shaonv.lua",
            hidden_flag="ch06.chugu"),
        npc("npc_dansha_tanzhu", 17, 24, "dansha_tanzhu", "up"),
        npc("npc_yejia_tanzhu", 6, 22, "yejia_tanzhu", "right"),
        # 赴会的人：一人一个 role（规则 30）
        npc("npc_sanxiu_a", 29, 14, "tainan_sanxiu_shaonian", "left"),
        npc("npc_sanxiu_b", 33, 23, "tainan_sanxiu_beijian", "up"),
        npc("npc_sanxiu_c", 9, 9, "tainan_sanxiu_shusheng", "right"),
        npc("npc_sanxiu_d", 38, 7, "tainan_sanxiu_nvxiu", "down"),
    ]
    return build_map("ch06_tainan_gu", W, H, "terrain_tainan_gu",
                     map_props("ch06_tainan_gu", "bgm_valley", "lanzhou"), g, objects)


# ---------------------------------------------------------------------------
# 地图 3：散修小楼 32x24（室内）—— 节点 7、9a、9b、10b
#
#   一楼（y 13..22）：大厅、八仙桌、苦桑的角落、存档点；南墙正中是门
#   楼梯（x 28..29，y 12）
#   二楼（y 1..11）：走廊 y 9..11；议事屋的门 (6,8)(7,8)；韩立的房间门 (22,8)
#     房里：制符桌 (16,2)(17,2)——左格 9a、右格制符设施；蒲团 (21,2)(22,2)——左格 9b、
#     右格打坐设施；床 (27,2)(28,2)——10b。一格一个对象（规则 17）。
# ---------------------------------------------------------------------------
def make_sanxiu_lou():
    W, H = 32, 24
    g = Grid(W, H)
    g.fill("ground", 1)
    border(g)

    # 一楼二楼之间的隔墙，楼梯口在东头
    g.solid_rect(1, 12, 30, 12, 5)
    road(g, 28, 12, 29, 12)
    # 二楼：房间与走廊之间的墙；议事屋与韩立房间之间的墙
    g.solid_rect(1, 8, 30, 8, 5)
    g.solid_rect(13, 1, 13, 7, 5)
    g.solid_rect(6, 8, 7, 8, 8)          # 议事屋的门扇（关着，推门是 7）
    road(g, 22, 8, 22, 8)                # 韩立房间的门
    g.rect("ground", 1, 9, 30, 11, 2)    # 走廊铺木板

    # 议事屋里一张长案（门关着，看不见也不挡谁）
    g.solid_rect(4, 3, 9, 4, 8)
    # 韩立的房间
    g.solid_rect(16, 2, 17, 2, 8)        # 制符桌
    g.solid_rect(21, 2, 22, 2, 8)        # 蒲团
    g.solid_rect(27, 2, 28, 2, 8)        # 床

    # 一楼
    g.solid_rect(7, 16, 8, 17, 8)        # 八仙桌
    g.solid_rect(20, 14, 21, 14, 8)      # 另一张方桌
    g.solid_rect(2, 13, 3, 13, 8)        # 苦桑念经的供案
    road(g, 15, 23, 16, 23)              # 大门

    objects = [
        spawn("spawn_from_tainan_gu", 15, 21, "up", default=True),
        portal("portal_to_tainan_gu", 15, 23, "ch06_tainan_gu", "spawn_from_sanxiu_lou", w=2),

        # ---- 节点 7：议事（二楼议事屋的门）----
        trigger("trigger_yishi", 6, 8, "ch06/yishi.lua", "interact", True, w=2,
                guard_flag="ch06.xunxin", set_flag="ch06.yishi"),
        # ---- 节点 9a：制符（制符桌左格）----
        trigger("trigger_zhifu", 16, 2, "ch06/zhifu.lua", "interact", True,
                guard_flag="ch06.jinzhubi", set_flag="ch06.zhifu"),
        facility("facility_zhifu", 17, 2, "talisman", grade=2, require_flag="ch06.zhifu"),
        # ---- 节点 9b：苦修（蒲团左格）----
        trigger("trigger_kuxiu", 21, 2, "ch06/kuxiu.lua", "interact", True,
                guard_flag="ch06.zhifu", set_flag="ch06.jiuceng"),
        facility("facility_dazuo", 22, 2, "meditate"),
        # ---- 节点 10b：青溪笔录（床）----
        trigger("trigger_bilu", 27, 2, "ch06/bilu.lua", "interact", True, w=2,
                guard_flag="ch06.canpian", set_flag="ch06.shengxianling"),

        facility("facility_cunji", 26, 21, "save"),

        # 散修团伙（施工图第 5 节：NPC 只摆在小楼）。出场的时刻照脚本（复验 N-L1、N-L7，裁决 16.5）：
        # ① 之前一伙人都在广场上，楼里只有苦桑；① 那天傍晚青纹在二楼议事屋门口等人，其余的已在屋里（门关着）；
        # 议事之后众人才在一楼走动。
        npc("npc_kusang", 4, 14, "kusang", "down", "ch06/kusang.lua"),
        npc("npc_qingwen_daoshi", 9, 10, "qingwen_daoshi", "down", "ch06/qingwen.lua", visible_flag="ch06.xunxin"),
        npc("npc_hu_pinggu", 9, 18, "hu_pinggu", "up", visible_flag="ch06.yishi"),
        npc("npc_xiong_dali", 10, 18, "xiong_dali", "up", visible_flag="ch06.yishi"),
        npc("npc_hei_mu", 20, 15, "hei_mu", "up", visible_flag="ch06.yishi"),
        npc("npc_hei_jin", 21, 15, "hei_jin", "up", visible_flag="ch06.yishi"),
        npc("npc_honglian", 24, 19, "honglian_sanren", "left", visible_flag="ch06.yishi"),
        # 议事那一天才露面的两个
        npc("npc_wu_jiuzhi", 18, 20, "wu_jiuzhi", "up", visible_flag="ch06.yishi"),
        npc("npc_huang_xiaotian", 27, 15, "huang_xiaotian", "left", visible_flag="ch06.yishi"),
    ]
    return build_map("ch06_sanxiu_lou", W, H, "terrain_sanxiu_lou",
                     map_props("ch06_sanxiu_lou", "bgm_inn", "lanzhou", outdoor=False), g, objects)


# ---------------------------------------------------------------------------
# 地图 4：百里荒丘 40x30 —— 节点 12
#
# 御风决一口气奔出百余里，落在荒丘西南。当中一处凹地，四面土埂，只在南边留一道口子。
#   12 trigger_xisha  凹地口子（enter + once，guard ch06.chugu）
#   野外遭遇在东北那片坡上（施工图 8.4），不挡去凹地的路。
# ---------------------------------------------------------------------------
def make_shanqiu():
    W, H = 40, 30
    g = Grid(W, H)
    rnd = random.Random(604)
    g.fill("ground", 1)
    border(g)

    # 几道土丘
    g.solid_rect(1, 1, 10, 6, 5)
    g.solid_rect(30, 20, 38, 28, 5)
    g.solid_rect(12, 24, 18, 28, 5)
    g.solid_rect(1, 12, 5, 16, 5)
    # 凹地：四面土埂，口子在南 (20,18)(21,18)
    g.solid_rect(17, 10, 24, 18, 5)
    floor(g, 18, 11, 23, 17)
    g.open_rect(20, 18, 21, 18)
    g.rect("ground", 20, 18, 21, 18, 2)

    for tx, ty in ((8, 9), (14, 4), (27, 6), (33, 3), (36, 12), (28, 15), (9, 21), (24, 23),
                   (4, 19), (13, 16), (35, 17)):
        tree(g, tx, ty)
    road(g, 3, 24, 6, 26, gid=1)         # 落脚那一小块空地

    edge_fronts(g, rnd)
    scatter(g, rnd, 150, 1, 1, W - 2, H - 2, grass=0.5)

    objects = [
        spawn("spawn_luodian", SHANQIU_LUODIAN[0], SHANQIU_LUODIAN[1], "up", default=True),
        facility("facility_cunji", 6, 24, "save"),

        # ---- 节点 12：袭杀（凹地口子）----
        trigger("trigger_xisha", 20, 18, "ch06/xisha.lua", "enter", True, w=2,
                guard_flag="ch06.chugu", set_flag="ch06.xisha"),

        encounter("encounter_dongpo", 26, 3, 12, 10, "encounter_ch06_shanqiu", 20, 45,
                  require_flag="ch06.chugu"),
    ]
    return build_map("ch06_shanqiu", W, H, "terrain_shanqiu",
                     map_props("ch06_shanqiu", "bgm_wild", "lanzhou"), g, objects)


# ---------------------------------------------------------------------------
# 地图 5：黄枫谷外门 48x36 —— 节点 13–17
#
#   西南 迎宾楼：厅 (3..13, 29..33)、房间 (3..13, 23..27)，隔墙上的门 (8,28)；东墙门 (14,30)(14,31)
#     13 trigger_ce_linggen  房门那一格（enter + once，guard ch06.xisha）：落点往北两格
#     14 trigger_ye_maidan   房里的床 (4,23)(5,23)（interact + once，guard ch06.linggen）
#   北   议事大殿：殿门 (23,8)(24,8)——15 trigger_dadian（interact + once，guard ch06.rangdan）
#   东北 传功阁：阁前的案 (38,9)(39,9)——16b trigger_wufeng（interact + once，guard ch06.chuwudai）
#   东   石屋群、林师弟屋；屋前的案 (38,28)(39,28)——16a trigger_lin_lingqu（guard ch06.rumen）
#   西   百机堂：柜台 (4..9, 12)——17a trigger_zawu 在 (6,12)；内殿卷宗架 (4,3)(5,3)——17b
#   东口 portal_to_baiyaoyuan (47,26)(47,27)，require_flag=ch06.huinuo
# ---------------------------------------------------------------------------
def make_huangfenggu():
    W, H = 48, 36
    g = Grid(W, H)
    rnd = random.Random(605)
    g.fill("ground", 1)
    border(g)

    # 路网
    road(g, 14, 19, 46, 20)              # 东西大路
    road(g, 23, 9, 24, 33)               # 南北大路（殿前到南头）
    road(g, 15, 30, 22, 31)              # 迎宾楼门口到大路
    road(g, 25, 26, 47, 27)              # 往百药园的东口
    road(g, 14, 14, 22, 15)              # 百机堂门口到大路

    # 迎宾楼
    g.solid_rect(2, 22, 14, 34, 5)
    floor(g, 3, 23, 13, 33)
    g.solid_rect(3, 28, 13, 28, 5)
    g.open_rect(8, 28, 8, 28)
    g.open_rect(14, 30, 14, 31)
    g.rect("ground", 14, 30, 14, 31, 2)
    g.solid_rect(4, 23, 5, 23, 8)        # 床
    g.solid_rect(10, 24, 11, 24, 8)      # 小案
    eaves_row(g, 2, 14, 21)

    # 议事大殿
    g.solid_rect(17, 2, 30, 8, 5)
    g.solid_rect(23, 8, 24, 8, 8)        # 殿门
    road(g, 17, 9, 30, 10)               # 殿前石坪

    # 传功阁
    g.solid_rect(34, 2, 44, 7, 5)
    g.solid_rect(38, 9, 39, 9, 8)        # 阁前的案
    road(g, 34, 8, 44, 8)

    # 石屋群（新弟子的屋子，十年一轮，眼下空着）
    for hx in (34, 37, 40, 43):
        g.solid_rect(hx, 14, hx + 1, 15, 5)
        g.solid_rect(hx, 22, hx + 1, 23, 5)
    # 林师弟那间大屋与屋前的案
    g.solid_rect(36, 29, 41, 32, 5)
    g.solid_rect(38, 28, 39, 28, 8)

    # 百机堂：外堂（柜台）与内殿（卷宗）
    g.solid_rect(1, 1, 13, 17, 5)
    floor(g, 2, 3, 12, 16)
    g.solid_rect(2, 9, 12, 9, 5)
    g.open_rect(7, 9, 7, 9)
    g.solid_rect(4, 12, 9, 12, 8)        # 柜台
    g.solid_rect(4, 3, 5, 3, 8)          # 卷宗架（17b）
    g.solid_rect(8, 3, 10, 3, 8)         # 另一排架子
    g.open_rect(13, 14, 13, 15)
    g.rect("ground", 13, 14, 13, 15, 2)

    for tx, ty in ((32, 11), (46 - 1, 11), (28, 14), (18, 24), (29, 32), (44, 33), (16, 12),
                   (31, 24), (20, 34 - 1)):
        tree(g, tx, ty)

    edge_fronts(g, rnd, chance=0.15)
    scatter(g, rnd, 90, 15, 11, 46, 34, grass=0.8)

    objects = [
        spawn("spawn_yingbin", HFG_YINGBIN[0], HFG_YINGBIN[1], "up", default=True),
        spawn("spawn_from_baiyaoyuan", 45, 26, "left"),
        portal("portal_to_baiyaoyuan", 47, 26, "ch06_baiyaoyuan", "spawn_from_huangfenggu", h=2,
               require_flag="ch06.huinuo", deny_text_key="ch06.block.baiyaoyuan"),

        # ---- 节点 13：测灵根（迎宾楼房门）----
        trigger("trigger_ce_linggen", 8, 28, "ch06/linggen.lua", "enter", True,
                guard_flag="ch06.xisha", set_flag="ch06.linggen"),
        # ---- 节点 14：叶师叔买丹（房里的床）----
        trigger("trigger_ye_maidan", 4, 23, "ch06/maidan.lua", "interact", True, w=2,
                guard_flag="ch06.linggen", set_flag="ch06.rangdan"),
        # ---- 节点 15：掌门殿（殿门）----
        trigger("trigger_dadian", 23, 8, "ch06/dadian.lua", "interact", True, w=2,
                guard_flag="ch06.rangdan", set_flag="ch06.rumen"),
        # ---- 节点 16a：领装备（林师弟屋前的案）----
        trigger("trigger_lin_lingqu", 38, 28, "ch06/lingqu.lua", "interact", True, w=2,
                guard_flag="ch06.rumen", set_flag="ch06.chuwudai"),
        # ---- 节点 16b：传功阁（阁前的案）----
        trigger("trigger_wufeng", 38, 9, "ch06/wufeng.lua", "interact", True, w=2,
                guard_flag="ch06.chuwudai", set_flag="ch06.wufeng"),
        # ---- 节点 17a：百机堂（柜台）----
        trigger("trigger_zawu", 6, 12, "ch06/zawu.lua", "interact", True,
                guard_flag="ch06.wufeng", set_flag="ch06.zawu"),
        # ---- 节点 17b：内殿卷宗（卷宗架）----
        trigger("trigger_juanzong", 4, 3, "ch06/juanzong.lua", "interact", True, w=2,
                guard_flag="ch06.zawu", set_flag="ch06.huinuo"),

        facility("facility_cunji", 32, 21, "save"),

        npc("npc_shoudian_dizi", 21, 9, "shoudian_dizi", "down"),
        # 王师叔：14-16b 的脚本里他全程陪着韩立，把人送回石屋群之后才站到大殿前（校对 LOW-7，施工图 16.4）
        npc("npc_wang_shishu", 26, 12, "wang_shishu", "left", "ch06/wang_npc.lua",
            visible_flag="ch06.wufeng"),
        npc("npc_lin_shidi", 40, 28, "lin_shidi", "up"),
        npc("npc_wu_feng", 40, 9, "wu_feng", "left"),
        npc("npc_yu_zhishi", 6, 11, "yu_zhishi", "down"),
        npc("npc_ye_shishu", 9, 5, "ye_shishu", "left", "ch06/ye_npc.lua",
            visible_flag="ch06.rangdan"),
        npc("npc_waimen_dizi", 36, 21, "huangfenggu_waimen_dizi", "left",
            visible_flag="ch06.chuwudai"),
    ]
    return build_map("ch06_huangfenggu", W, H, "terrain_huangfenggu",
                     map_props("ch06_huangfenggu", "bgm_sect", "jianzhou"), g, objects)


# ---------------------------------------------------------------------------
# 地图 6：百药园 32x24 —— 节点 18
#
# 两座山丘之间的小盆地，西口一道两格宽的夹道，禁制就罩在夹道上。
#   18a trigger_jinzhi   夹道（enter + once，guard ch06.huinuo）
#   18b trigger_maiping  药田西南角那一格翻土（interact + once，guard ch06.renyao）
#   药田：facility kind=field，ref_id=field_baiyaoyuan，slots=6，require_flag=ch06.done
#   茅屋三间，北边那间有院墙的是他的居室，里头一张蒲团（打坐）。
# ---------------------------------------------------------------------------
def make_baiyaoyuan():
    W, H = 32, 24
    g = Grid(W, H)
    rnd = random.Random(606)
    g.fill("ground", 1)
    border(g)

    # 两座山丘，中间夹出西口
    g.solid_rect(1, 1, 8, 10, 5)
    g.solid_rect(1, 13, 8, 22, 5)
    road(g, 0, 11, 8, 12)
    road(g, 9, 11, 21, 12)
    # 他的居室（有院墙），门朝南 (24,7)
    g.solid_rect(21, 2, 27, 7, 5)
    floor(g, 22, 3, 26, 6)
    g.open_rect(24, 7, 24, 7)
    g.rect("ground", 24, 7, 24, 7, 2)
    g.solid_rect(23, 4, 24, 4, 8)        # 蒲团
    eaves_row(g, 21, 27, 1)
    # 另两间茅屋
    g.solid_rect(11, 2, 14, 5, 5)
    eaves_row(g, 11, 14, 1)
    g.solid_rect(15, 2, 18, 5, 5)
    eaves_row(g, 15, 18, 1)
    road(g, 24, 8, 24, 10)
    # 药田：一方方翻土，中间一块开给灵田
    tilled(g, 11, 14, 20, 19)
    tilled(g, 23, 14, 29, 19)
    for x in range(11, 30):
        if x in (21, 22):
            continue
        if x % 3 == 0:
            g.set("overlay", x, 16, 7)   # 田里的沟槽

    for tx, ty in ((29, 4), (10, 8), (29, 9), (29, 21), (10, 21), (16, 21)):
        tree(g, tx, ty)

    edge_fronts(g, rnd)
    scatter(g, rnd, 60, 9, 6, 30, 12)

    objects = [
        spawn("spawn_from_huangfenggu", 1, 11, "right", default=True),
        portal("portal_to_huangfenggu", 0, 11, "ch06_huangfenggu", "spawn_from_baiyaoyuan", h=2),

        # ---- 节点 18a：禁制（西口夹道）----
        trigger("trigger_jinzhi", 6, 11, "ch06/jinzhi.lua", "enter", True, h=2,
                guard_flag="ch06.huinuo", set_flag="ch06.renyao"),
        # ---- 节点 18b：埋瓶（药田西南角）----
        trigger("trigger_maiping", 11, 19, "ch06/maiping.lua", "interact", True,
                guard_flag="ch06.renyao", set_flag="ch06.done"),

        facility("facility_yaotian", 14, 15, "field", w=3, h=2,
                 ref_id="field_baiyaoyuan", slots=6, require_flag="ch06.done"),
        facility("facility_dazuo", 23, 4, "meditate", w=2),
        facility("facility_cunji", 20, 9, "save"),

        npc("npc_ma_shibo", 17, 8, "ma_shibo", "left", hidden_flag="ch06.renyao"),
    ]
    return build_map("ch06_baiyaoyuan", W, H, "terrain_baiyaoyuan",
                     map_props("ch06_baiyaoyuan", "bgm_valley", "jianzhou"), g, objects)


def patch_nancheng(payload):
    """给南城补上东门。幂等，补过的图再补一次不变。

    payload 是 genmaps_ch05.make_nancheng() 的产出（或读盘的那一份）。追加顺序固定
    （只有这一道传送点），`--check` 才不报假漂移。
    """
    W = payload["width"]
    layers = {l["name"]: l for l in payload["layers"]}

    def put(layer, x, y, value):
        layers[layer]["data"][y * W + x] = value

    for y in (17, 18):
        put("building", W - 1, y, 0)
        put("collision", W - 1, y, 0)
        put("front", W - 1, y, 0)
        put("ground", W - 1, y, 2)
        put("overlay", W - 1, y, 0)

    objs = layers["objects"]["objects"]
    names = {o["name"] for o in objs}
    if "portal_to_tainan_cun" not in names:
        objs.append(portal("portal_to_tainan_cun", W - 1, 17, "ch06_tainan_cun", "spawn_from_nancheng",
                           h=2, require_flag="ch05.done", deny_text_key="ch06.block.tainan_cun"))
    for i, o in enumerate(objs, 1):
        o["id"] = i
    payload["nextobjectid"] = len(objs) + 1
    return payload


def build_all():
    """本章六张图的完整产出，外加南城那处就地修补的东门。"""
    import genmaps_ch05
    return {
        "ch06_tainan_cun": make_tainan_cun(),
        "ch06_tainan_gu": make_tainan_gu(),
        "ch06_sanxiu_lou": make_sanxiu_lou(),
        "ch06_shanqiu": make_shanqiu(),
        "ch06_huangfenggu": make_huangfenggu(),
        "ch06_baiyaoyuan": make_baiyaoyuan(),
        "ch05_nancheng": patch_nancheng(genmaps_ch05.make_nancheng()),
    }


TILESETS = ("terrain_tainan_cun", "terrain_tainan_gu", "terrain_sanxiu_lou",
            "terrain_shanqiu", "terrain_huangfenggu", "terrain_baiyaoyuan")


def main(argv=None):
    argv = list(sys.argv[1:] if argv is None else argv)
    if "--check" in argv:
        return check_maps(build_all())

    for name in TILESETS:
        write_tileset(name)

    for map_id, payload in build_all().items():
        dump_map(os.path.join(genmaps.MAPS, map_id + ".tmj"), payload)
        print("wrote", map_id)
    return 0


if __name__ == "__main__":
    sys.exit(main())
