# -*- coding: utf-8 -*-
"""第 11 章七张图（docs/ch11-design.md 第 4 节；契约 docs/interfaces-p3-ch11.md 第 7 节）。

    python tools/mapgen/genmaps_ch11.py           # 重写七张 tmj 与七个 tileset
    python tools/mapgen/genmaps_ch11.py --check   # 只比对

build_all 是纯函数、不导入别章生成器、不用随机数；genmaps.py 的 check() 在第 10 章之后
接进来。本章不 patch 任何旧图。

七张图与出入口（规则 29「出门往哪走，进门就朝哪」，落点在括号里）：

    ch11_fangshi      56x44  天星城坊市：南缘城门（spawn chengmen，默认）、中段坊市街、北缘楼阁；
                             北口 portal_shangshan → ch11_dongfu(men)，require ch11.zufu；
                             街西青竹小轩门 portal_xiaoxuan → ch11_xiaoxuan(dian)，require ch11.kaidian；
                             东北角茶馆雅间：同图一对门 portal_chaguan(进，require ch11.lianer) /
                             portal_chaguan_chu(出)，落点 yajian / chaguan_men——规则 13 要求雅间里的
                             NPC 从城门走得到，所以进门那一格与雅间在地形上连着，闸门靠 portal 的 require_flag。
    ch11_dongfu       34x26  圣山第三十九层洞府：南口 portal_xiashan → ch11_fangshi(shanjiao)；
                             北口 portal_xingkong → ch11_xingkong(dian)，require ch11.nichang。
    ch11_xingkong     24x18  星空殿：南口 portal_xiashan_50 → ch11_dongfu(beimen)。
    ch11_ningcui_jie  30x20  凝翠岛小街：东口 portal_chuhai → ch11_honghu(xi)，require ch11.ningcui；
                             石屋传送阵 trigger_huicheng_chuansong 是脚本 teleport。
    ch11_honghu       36x30  红瑚岛：西口 portal_huijie → ch11_ningcui_jie(dong)。
    ch11_yinsha_dao   28x22  隐煞门礁岛：无门，只进不出（n21 teleport 进、n24 teleport 出）。
    ch11_xiaoxuan     20x14  青竹小轩：南口 portal_chudian → ch11_fangshi(xiaoxuan_men)。

脚本 teleport 只落 default spawn，所以每张图把脚本落点那个 spawn 标 default：
chengmen / men / dian / shiwu / xi / haishang / dian。

施工偏差（集成时并进施工图第 18 节）：
    - 同一格叠两个触发器引擎按 objectAt 只取第一个（WorldScene.cpp），validate 规则 16 也报，
      所以红瑚岛 trigger_huicheng 挪到登岸格旁一格（interact 面朝海）、礁岛 n22/n23/n24
      三连改成登岛小径上连续三格、炼器台 trigger_lianjian 挪到台前一格（施工图 3.1 的 O-7 已许）。
    - deny_text_key 按规则 20 用目标图命名（ch11.block.<目标图短名>），不用施工图写的
      ch11.fangshi.shangshan_deny / ch11.dongfu.xingkong_deny（契约第 7 节「按规则 20 成对」）。
    - trigger_chujian 加 guard_flag ch11.liugen：剑只能在炼器台（require ch11.liugen）炼出，
      没有 guard 它会在每次踏过密室门口时空跑一次脚本。
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import genmaps
from genmaps import Grid, build_map, check_maps, dump_map, facility, npc, portal, spawn, trigger, write_tileset

REGION = "luanxinghai_zhongbu"


def base(w, h, sea=False):
    g = Grid(w, h)
    g.fill("ground", 3 if sea else 1)
    if sea:
        g.fill("collision", 1)
    else:
        g.solid_rect(0, 0, w - 1, 0)
        g.solid_rect(0, h - 1, w - 1, h - 1)
        g.solid_rect(0, 0, 0, h - 1)
        g.solid_rect(w - 1, 0, w - 1, h - 1)
    return g


def cave(w, h):
    """整张实心，再用 floor 一间间凿出来（洞府、星空殿、青竹小轩）。"""
    g = Grid(w, h)
    g.fill("ground", 1)
    g.solid_rect(0, 0, w - 1, h - 1)
    return g


def floor(g, x0, y0, x1, y1, gid=1):
    g.open_rect(x0, y0, x1, y1)
    g.rect("ground", x0, y0, x1, y1, gid)


def shop_block(g, x0, y0, x1, y1):
    """临街的实心铺面：屋身 + 北侧一行檐口遮挡；门口的人与门另摆。"""
    g.solid_rect(x0, y0, x1, y1)
    g.rect("front", x0, y0 - 1, x1, y0 - 1, 4)


def room(g, x0, y0, x1, y1, door):
    g.solid_rect(x0, y0, x1, y1)
    floor(g, x0 + 1, y0 + 1, x1 - 1, y1 - 1)
    floor(g, *door, *door, 2)
    g.rect("front", x0, y0 - 1, x1, y0 - 1, 4)


def event(g, objects, short, xy, guard, done, mode="interact", once=True, width=1, name=None):
    x, y = xy
    if mode == "interact":
        g.solid_rect(x, y, x + width - 1, y, 8)
    objects.append(trigger(name or "trigger_" + short, x, y, "ch11/" + short + ".lua",
                           mode, once=once, w=width, guard_flag=guard, set_flag=done))


def person(objects, role, xy, visible, hidden=None, name=None, script=None):
    objects.append(npc(name or "npc_" + role, *xy, role, "down",
                       script=("ch11/" + script) if script else None,
                       visible_flag=visible, hidden_flag=hidden))


def save(objects, name, xy):
    objects.append(facility(name, *xy, "save"))


def finish(short, bgm, g, objects, outdoor=True):
    mid = "ch11_" + short
    props = [("map_id", mid), ("display_name_key", "ch11.map." + short + ".name"),
             ("region", REGION), ("bgm", bgm), ("chapter", 11),
             ("outdoor", outdoor), ("can_leave_edge", False)]
    return build_map(mid, g.w, g.h, "terrain_ch11_" + short, props, g, objects)


# ---------------------------------------------------------------------------
# 天星城坊市 56x44：南缘城门、中段坊市街、北缘山脚楼阁、东北角茶馆雅间
# ---------------------------------------------------------------------------
def make_fangshi():
    g = base(56, 44)
    g.solid_rect(1, 1, 15, 8)                   # 西北角圣山山脚岩体
    g.solid_rect(31, 1, 40, 2)                  # 北缘岩体
    floor(g, 26, 1, 30, 42, 2)                  # 主街：城门到北口
    floor(g, 28, 0, 28, 0, 2)                   # 北口
    floor(g, 16, 11, 41, 12, 2)                 # 北缘横街
    floor(g, 10, 20, 45, 21, 2)                 # 坊市横街
    floor(g, 10, 30, 45, 31, 2)                 # 南段横街
    g.solid_rect(1, 35, 54, 35)                 # 城墙
    floor(g, 26, 35, 30, 35, 2)                 # 城门洞
    g.solid_rect(23, 34, 25, 36, 8)             # 门楼石栏
    g.solid_rect(31, 34, 33, 36, 8)
    room(g, 17, 3, 24, 9, (21, 9))              # 山脚楼阁（星宫执事）
    shop_block(g, 17, 14, 23, 17)               # 白门阁
    shop_block(g, 33, 14, 39, 17)               # 丰乐拍卖行
    shop_block(g, 17, 23, 23, 26)               # 李氏杂货 → 青竹小轩
    floor(g, 20, 26, 20, 26, 2)                 # 店门（portal）
    floor(g, 20, 27, 20, 29, 2)
    shop_block(g, 33, 23, 39, 26)               # 储物袋铺
    g.solid_rect(42, 3, 53, 11)                 # 茶馆
    g.rect("front", 42, 2, 53, 2, 4)
    floor(g, 43, 4, 52, 10)                     # 雅间（zone wood_floor）
    floor(g, 42, 7, 42, 7, 2)                   # 茶馆门（portal_chaguan）
    g.rect("overlay", 44, 37, 52, 40, 6)        # 城门外花草
    g.rect("overlay", 4, 37, 12, 40, 6)
    objects = [spawn("chengmen", 28, 38, "up", True), spawn("shanjiao", 28, 2, "down"),
               spawn("xiaoxuan_men", 20, 29, "down"), spawn("yajian", 44, 8, "right"),
               spawn("chaguan_men", 40, 10, "left")]
    save(objects, "facility_cunji_fangshi", (32, 33))
    objects.append(facility("facility_zahuo", 33, 31, "shop", ref_id="ch11_fangshi_zahuo",
                            require_flag="ch11.nichang"))
    objects.append(facility("facility_xuanshang", 24, 32, "board"))
    event(g, objects, "chudao", (26, 35), "ch10.done", "ch11.chudao", "enter", width=5)
    person(objects, "baiyi_xiushi", (28, 34), "ch11.chudao", "ch11.zhihuan", script="zhihuan.lua")
    person(objects, "lu_er", (29, 32), "ch11.zhihuan", "ch11.anju", script="luer.lua")
    person(objects, "xinggong_zhishi", (20, 6), "ch11.luer", "ch11.zufu", script="zufu.lua")
    person(objects, "baimen_zhanggui", (20, 18), "ch11.kuilei", "ch11.done", script="ziliao.lua")
    person(objects, "fengle_menmian", (36, 18), "ch11.kuilei", "ch11.done")
    person(objects, "chuwu_zhanggui", (36, 27), "ch11.kuilei", "ch11.huicheng")
    person(objects, "lian_er", (29, 23), "ch11.zhaolan", "ch11.lianer", script="lianer.lua")
    # 范夫人 n21 起赴礁岛（同 role 的 npc_fan_furen_2），NpcPresenceShipped 不许一人两地：雅间对象到 ch11.zhuzhan 退场；
    # n25 她只作说话人（zhanglao.lua 的 talk 走 role 名牌）。
    person(objects, "fan_furen", (48, 6), "ch11.lianer", "ch11.zhuzhan", script="chaguan.lua")
    person(objects, "zhao_zhanglao", (50, 6), "ch11.lianer", "ch11.chaguan")
    person(objects, "zi_ling_zhenrong", (48, 8), "ch11.yaoshi", "ch11.zhanglao",
           name="npc_zi_ling", script="zhanglao.lua")
    person(objects, "zhuo_ruting", (50, 8), "ch11.yaoshi", "ch11.zhanglao")
    person(objects, "li_dianzhu", (21, 27), "ch11.jinlei", "ch11.kaidian", script="kaidian.lua")
    objects.append(portal("portal_shangshan", 28, 0, "ch11_dongfu", "men",
                          require_flag="ch11.zufu", deny_text_key="ch11.block.dongfu"))
    objects.append(portal("portal_xiaoxuan", 20, 26, "ch11_xiaoxuan", "dian",
                          require_flag="ch11.kaidian", deny_text_key="ch11.block.xiaoxuan"))
    objects.append(portal("portal_chaguan", 42, 7, "ch11_fangshi", "yajian",
                          require_flag="ch11.lianer", deny_text_key="ch11.block.fangshi"))
    objects.append(portal("portal_chaguan_chu", 43, 7, "ch11_fangshi", "chaguan_men"))
    return finish("fangshi", "bgm_town", g, objects)


# ---------------------------------------------------------------------------
# 圣山第三十九层洞府 34x26：门外小径、门禁制、门厅、密室、丹房、静室、虫室、药园
# ---------------------------------------------------------------------------
def make_dongfu():
    g = cave(34, 26)
    floor(g, 11, 22, 22, 24)                    # 门外小径（金青、结丹邻居站这里）
    floor(g, 16, 25, 16, 25, 2)                 # 南口
    floor(g, 16, 19, 16, 21, 2)                 # 门禁制 → 门内：n18 / spawn men / n05
    floor(g, 15, 20, 15, 20)                    # 存档点壁龛（紧挨 spawn men）
    floor(g, 12, 12, 21, 18)                    # 门厅
    floor(g, 9, 15, 11, 15, 2)                  # 密室门外甬道（n17、n30b）
    floor(g, 3, 12, 8, 17)                      # 密室
    floor(g, 8, 18, 8, 18)                      # 炼器台（只从 (8,17) 够得着）
    floor(g, 16, 10, 17, 11, 2)                 # 丹房甬道
    floor(g, 13, 5, 20, 9)                      # 丹房
    floor(g, 21, 7, 23, 7, 2)                   # 静室甬道
    floor(g, 24, 5, 27, 8)                      # 静室
    floor(g, 26, 1, 26, 4, 2)                   # 北口甬道
    floor(g, 26, 0, 26, 0, 2)                   # 北口
    floor(g, 22, 14, 23, 14, 2)                 # 虫室甬道
    floor(g, 24, 12, 30, 16)                    # 虫室
    floor(g, 22, 18, 23, 18, 2)                 # 药园甬道
    floor(g, 24, 17, 31, 23)                    # 药园
    g.solid_rect(30, 21, 31, 23)                # 药园一角的珊瑚礁岩体（n26 培竹在它旁边）
    g.rect("ground", 25, 18, 27, 19, 3)         # 六槽灵田（zone tilled）
    g.rect("overlay", 24, 20, 29, 20, 6)        # 药园花草
    objects = [spawn("men", 16, 20, "up", True), spawn("beimen", 26, 2, "down")]
    save(objects, "facility_cunji_39", (15, 20))
    objects.extend([
        facility("facility_jingshi_39", 25, 5, "meditate", effectiveness=125, require_flag="ch11.anju"),
        facility("facility_danfang_39", 14, 6, "alchemy", grade=3, require_flag="ch11.anju"),
        facility("facility_lianqi_39", 8, 18, "forge", grade=3, require_flag="ch11.liugen"),
        facility("facility_yaoyuan_39", 26, 19, "field", ref_id="field_tianxing_39",
                 slots=6, require_flag="ch11.anju")])
    event(g, objects, "zhaolan", (16, 21), "ch11.chuguan", "ch11.zhaolan", "enter")
    event(g, objects, "anju", (16, 19), "ch11.zufu", "ch11.anju", "enter")
    event(g, objects, "zhuzhan", (17, 20), "ch11.chaguan", "ch11.zhuzhan")
    event(g, objects, "kuilei", (4, 13), "ch11.anju", "ch11.kuilei")
    event(g, objects, "biguan", (4, 16), "ch11.sangong", "ch11.jiedan")
    event(g, objects, "lianjian", (8, 17), "ch11.liugen", "ch11.lianjian_qian", "enter")
    event(g, objects, "chuguan", (11, 15), "ch11.jiedan", "ch11.chuguan", "enter")
    event(g, objects, "chujian", (10, 15), "ch11.liugen", "ch11.lianjian", "enter")
    event(g, objects, "liandan", (19, 6), "ch11.huicheng", "ch11.sangong")
    event(g, objects, "nichang", (29, 18), "ch11.ziliao", "ch11.nichang")
    event(g, objects, "peizhu", (29, 22), "ch11.zhanglao", None, once=False)
    person(objects, "jin_qing", (14, 23), "ch11.lianjian", "ch11.done", script="zhuzhong.lua")
    person(objects, "jiedan_linju_jia", (19, 23), "ch11.zhaolan", "ch11.done", name="npc_jiedan_linju_a")
    person(objects, "jiedan_linju_yi", (20, 23), "ch11.zhaolan", "ch11.done", name="npc_jiedan_linju_b",
           script="linju_z2.lua")
    objects.append(portal("portal_xiashan", 16, 25, "ch11_fangshi", "shanjiao"))
    objects.append(portal("portal_xingkong", 26, 0, "ch11_xingkong", "dian",
                          require_flag="ch11.nichang", deny_text_key="ch11.block.xingkong"))
    return finish("dongfu", "bgm_valley", g, objects, outdoor=False)


# ---------------------------------------------------------------------------
# 星空殿（第五十层）24x18：大理石殿、成排传送阵
# ---------------------------------------------------------------------------
def make_xingkong():
    g = cave(24, 18)
    floor(g, 3, 3, 20, 14)
    floor(g, 11, 15, 12, 16, 2)                 # 南口甬道
    floor(g, 12, 17, 12, 17, 2)                 # 南口
    floor(g, 11, 6, 12, 14, 2)                  # 殿中石板道
    g.rect("overlay", 5, 4, 19, 5, 7)           # 成排传送阵的阵纹
    for x, y in ((5, 8), (18, 8), (5, 12), (18, 12)):
        g.solid_rect(x, y, x, y, 8)             # 石柱
    objects = [spawn("dian", 12, 13, "up", True)]
    save(objects, "facility_cunji_50", (15, 13))
    person(objects, "xingkong_xiushi", (12, 6), "ch11.nichang", "ch11.done", script="xingkong.lua")
    person(objects, "yaoyan_furen", (7, 9), "ch11.nichang", "ch11.chuansong")
    person(objects, "tongxing_xiushi", (9, 9), "ch11.nichang", "ch11.chuansong")
    person(objects, "zaohong_laozhe", (16, 10), "ch11.nichang", "ch11.chuansong")
    objects.append(portal("portal_xiashan_50", 12, 17, "ch11_dongfu", "beimen"))
    return finish("xingkong", "bgm_valley", g, objects, outdoor=False)


# ---------------------------------------------------------------------------
# 凝翠岛小街 30x20：西端传送石屋、一条街四间铺、东端海岸
# ---------------------------------------------------------------------------
def make_ningcui_jie():
    g = base(30, 20, True)
    floor(g, 2, 3, 27, 16)                      # 岛
    floor(g, 28, 3, 28, 9)                      # 街北第四间铺面（25..28）的东墙脚下补一列岸：水面 gid 3 上不画 building（MapArtTests）
    floor(g, 4, 10, 28, 11, 2)                  # 小街
    floor(g, 28, 10, 29, 11, 2)                 # 东口栈桥
    room(g, 3, 6, 8, 11, (6, 11))               # 传送石屋
    g.rect("overlay", 4, 8, 6, 9, 7)            # 石屋里的阵纹
    g.solid_rect(4, 12, 4, 14, 8)               # 石屋前院的石栏：出院只能走 (8, 12)
    g.solid_rect(5, 14, 8, 14, 8)
    g.solid_rect(8, 13, 8, 13, 8)
    for x0 in (10, 15, 20, 25):                 # 街北四间铺面
        shop_block(g, x0, 5, x0 + 3, 8)
    g.rect("overlay", 10, 14, 26, 15, 6)        # 街南的海边草
    objects = [spawn("shiwu", 6, 12, "up", True), spawn("dong", 27, 11, "left")]
    save(objects, "facility_cunji_nc", (24, 13))
    objects.append(facility("facility_zahuo_nc", 13, 9, "shop", ref_id="ch11_ningcui_zahuo"))
    objects.append(facility("facility_shougou", 19, 9, "shop", ref_id="ch11_ningcui_shougou"))
    event(g, objects, "huicheng_chuansong", (5, 8), "ch11.chuansong", None, "enter", once=False)
    event(g, objects, "ningcui", (8, 12), "ch11.chuansong", "ch11.ningcui", "enter")
    person(objects, "ningcui_shouzhen", (7, 9), "ch11.chuansong", "ch11.done")
    person(objects, "zahuo_nc", (11, 9), "ch11.chuansong", "ch11.huicheng")
    person(objects, "fan_xiaozi", (17, 9), "ch11.chuansong", "ch11.huicheng", script="shougou_dingdan.lua")
    person(objects, "hao_yuantian", (22, 12), "ch11.ningcui", "ch11.huicheng")
    objects.append(portal("portal_chuhai", 29, 11, "ch11_honghu", "xi",
                          require_flag="ch11.ningcui", deny_text_key="ch11.block.honghu"))
    return finish("ningcui_jie", "bgm_river", g, objects)


# ---------------------------------------------------------------------------
# 红瑚岛 36x30：环形珊瑚礁、三处必经路、环心孤礁
# ---------------------------------------------------------------------------
def make_honghu():
    g = base(36, 30, True)
    floor(g, 5, 4, 31, 25)                      # 环形礁
    g.rect("ground", 10, 10, 25, 19, 3)         # 礁湖
    g.rect("collision", 10, 10, 25, 19, 1)
    floor(g, 10, 14, 15, 15, 2)                 # 西侧礁道（必经路一）
    floor(g, 16, 13, 19, 16)                    # 环心孤礁
    floor(g, 1, 14, 4, 14, 2)                   # 登岸格
    floor(g, 0, 14, 0, 14, 2)                   # 西口
    for rect in ((12, 5, 14, 7), (27, 20, 29, 23), (8, 21, 10, 23), (22, 6, 23, 7), (6, 9, 7, 10)):
        g.solid_rect(*rect)                     # 礁石
    g.rect("overlay", 16, 13, 19, 16, 7)        # 环心碎珊瑚
    g.rect("overlay", 5, 24, 31, 25, 7)
    objects = [spawn("xi", 3, 14, "right", True)]
    save(objects, "facility_cunji_hh", (5, 13))
    event(g, objects, "buzhen", (4, 14), "ch11.ningcui", "ch11.buzhen", "enter")
    floor(g, 2, 13, 2, 13, 2)                   # 收手礁石脚下先铺礁道：水面（gid 3）上不画 building（MapArtTests）
    event(g, objects, "huicheng", (2, 13), "ch11.buzhen", "ch11.huicheng")
    event(g, objects, "youyao", (18, 14), "ch11.buzhen", None, once=False)
    objects.append(portal("portal_huijie", 0, 14, "ch11_ningcui_jie", "dong"))
    return finish("honghu", "bgm_cliff", g, objects)


# ---------------------------------------------------------------------------
# 隐煞门礁岛 28x22：礁石小岛、残阵；无门，登岛小径上 n22 → n23 → n24 连续三格
# ---------------------------------------------------------------------------
def make_yinsha_dao():
    g = base(28, 22, True)
    floor(g, 6, 3, 21, 15)                      # 岛
    floor(g, 13, 16, 13, 19, 2)                 # 登岛小径（海上礁石）
    for rect in ((7, 4, 8, 5), (18, 4, 20, 5), (6, 12, 7, 14), (19, 13, 21, 15)):
        g.solid_rect(*rect)                     # 礁石
    g.rect("overlay", 10, 7, 16, 11, 7)         # 残阵
    g.rect("overlay", 12, 8, 14, 10, 0)
    objects = [spawn("haishang", 13, 19, "up", True)]
    event(g, objects, "guiju", (13, 18), "ch11.zhuzhan", "ch11.guiju", "enter")
    event(g, objects, "wuchou", (13, 17), "ch11.guiju", "ch11.jiyin", "enter")
    event(g, objects, "daoge", (13, 16), "ch11.jiyin", "ch11.yaoshi", "enter")
    person(objects, "zi_ling", (11, 12), "ch11.zhuzhan", "ch11.yaoshi")
    for role, xy in (("fan_furen", (9, 13)), ("zhuo_ruting", (10, 14)), ("zhao_zhanglao", (16, 13))):
        person(objects, role, xy, "ch11.zhuzhan", "ch11.yaoshi", name="npc_" + role + "_2")
    for role, xy in (("fu_zhanglao", (17, 14)), ("meng_daoyou", (8, 11)), ("chihuo_laoguai", (18, 11))):
        person(objects, role, xy, "ch11.zhuzhan", "ch11.yaoshi")
    person(objects, "yinsha_menzhu", (13, 6), "ch11.guiju", "ch11.yaoshi")
    person(objects, "wu_chou", (13, 9), "ch11.guiju", "ch11.jiyin")
    person(objects, "jiyin_fushen", (13, 9), "ch11.jiyin", "ch11.yaoshi")
    return finish("yinsha_dao", "bgm_wild", g, objects)


# ---------------------------------------------------------------------------
# 青竹小轩 20x14：铺面、柜台、后宅
# ---------------------------------------------------------------------------
def make_xiaoxuan():
    g = cave(20, 14)
    floor(g, 2, 2, 17, 11)                      # 后宅（y 2..6）、柜台（y 7）、铺面（y 8..11）
    floor(g, 9, 12, 9, 12, 2)                   # 店门甬道
    floor(g, 9, 13, 9, 13, 2)                   # 南口
    g.solid_rect(7, 7, 12, 7, 8)                # 柜台
    g.solid_rect(2, 2, 5, 2, 8)                 # 后宅货架
    g.solid_rect(14, 2, 17, 2, 8)
    objects = [spawn("dian", 9, 10, "up", True)]
    save(objects, "facility_cunji_xx", (16, 10))
    event(g, objects, "jiesuan", (8, 7), "ch11.kaidian", None, once=False)
    event(g, objects, "guitai", (11, 7), "ch11.kaidian", None, once=False)
    person(objects, "he_xianshi", (4, 9), "ch11.kaidian", "ch11.liugen", name="npc_he_xianshi_2")
    person(objects, "linju_dianzhu", (14, 9), "ch11.kaidian", "ch11.liugen")
    objects.append(portal("portal_chudian", 9, 13, "ch11_fangshi", "xiaoxuan_men"))
    return finish("xiaoxuan", "bgm_town", g, objects, outdoor=False)


BUILDERS = (make_fangshi, make_dongfu, make_xingkong, make_ningcui_jie,
            make_honghu, make_yinsha_dao, make_xiaoxuan)


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
