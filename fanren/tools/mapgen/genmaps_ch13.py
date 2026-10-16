# -*- coding: utf-8 -*-
"""第 13 章四张图（docs/ch13-design.md 第 4 节、3.1 节；契约 docs/interfaces-p3-ch13.md 第 2、4 节）。

    python tools/mapgen/genmaps_ch13.py           # 重写四张 tmj 与四个 tileset
    python tools/mapgen/genmaps_ch13.py --check   # 只比对

build_all 是纯函数、不导入别章生成器、不用随机数；genmaps.py 的 check() 在第 12 章之后
接进来。本章不 patch 任何旧图，四张图之间也没有一扇门（跨图全是脚本 teleport）。

    ch13_nanming   48x36  南明岛：西南礁 haishang（默认，第 12 章末行 teleport 的落点，格上无对象）
                          → 北面一行横贯礁路的 trigger_wenxun → 港口小城（原料铺）→ 码头
                          → 港后丘陵坡口 trigger_qiuling → 客栈一角（打坐）→ 东出海口 trigger_chugang
                          → 出海口外礁头（凌玉灵）。
    ch13_tianxing  56x44  天星城（戒严），南 → 北：南端码头 chengwai（默认）→ 城门前横贯带 trigger_rucheng
                          → 城门 → 圣山脚坊市门 → 第四层宅院 → 第三十九层洞府（旧灵田、静室）
                          → 第四十九层石阶 trigger_lanlu → 第五十层外廊 trigger_xingkong → 星空殿传送大厅。
    ch13_waihai    72x56  外星海一图三区：北区黑石城（祭坛石屋 shiwu 默认）、西南秘市荒岛、东南小岛，
                          以中部礁路相连；礁路进北区的那一行是 trigger_feixu。
    ch13_wuzhong   64x48  雾中岛：东岸登岸礁 andeng（默认）→ trigger_lifu；山脉凹谷洞府（甬道外侧
                          trigger_chuguan、内侧 trigger_dizhen，静室丹炉药园与六扇门都在内侧带之内）；
                          东岸码头（入口带 trigger_lidao、尽头 trigger_qiyuan_hang）；西海珊瑚礁。

脚本 teleport 的落点（照抄进脚本与 tests/ObjectiveTests.cpp 的 kScriptTransfers；落点格不放任何对象）：
    lingquan.lua / yinjian.lua / chuansong.lua / chucheng.lua / hui_wuzhong.lua 只传图 id，
        落 haishang / chengwai / shiwu / andeng（default）。
    lidao.lua       → teleport("ch13_waihai", 14, 46)   秘市岸（MISHI_SHORE）
    qiyuan_hang.lua → teleport("ch13_waihai", 7, 12)    黑石城西门外礁（QIYUAN_REEF），
        在 trigger_hui_wuzhong (3, 12) 与西门带 trigger_chucheng (10, 11..13) 之间，两边各隔三格以上。

施工偏差见 C:/Users/htx-lyh/Documents/FanrenChapterWorktrees/notes/ch13/deviations-A.md。
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import genmaps
from genmaps import Grid, build_map, check_maps, dump_map, facility, npc, spawn, trigger, write_tileset

REGION = "luanxinghai_nixing"
CHAPTER = 13

# 带坐标的两处脚本落点。改这里就要同步改 scripts/ch13/lidao.lua、qiyuan_hang.lua 与 kScriptTransfers。
MISHI_SHORE = (14, 46)
QIYUAN_REEF = (7, 12)


def sea_base(w, h):
    """整张海面（gid 3 + 碰撞，不画 building），再用 floor 堆出陆地。"""
    g = Grid(w, h)
    g.fill("ground", 3)
    g.fill("collision", 1)
    return g


def floor(g, x0, y0, x1, y1, gid=1):
    g.open_rect(x0, y0, x1, y1)
    g.rect("ground", x0, y0, x1, y1, gid)


def rock(g, x0, y0, x1, y1, gid=5):
    """实心块。先把地表垫成实地：海面（gid 3）上画了 building，水与墙的分工就乱了（MapArtTests）。"""
    g.rect("ground", x0, y0, x1, y1, 1)
    g.solid_rect(x0, y0, x1, y1, gid)


def pool(g, x0, y0, x1, y1):
    """水面：gid 3 + 碰撞，不画 building。"""
    g.open_rect(x0, y0, x1, y1)
    g.rect("ground", x0, y0, x1, y1, 3)
    g.rect("collision", x0, y0, x1, y1, 1)


def house(g, x0, y0, x1, y1):
    """屋身实心 + 北侧一行檐口遮挡。"""
    rock(g, x0, y0, x1, y1)
    g.rect("front", x0, y0 - 1, x1, y0 - 1, 4)


def hook(g, objects, name, script, xy, mode, guard, done, once=True, w=1, h=1):
    """挂点。interact 那一格画成石栏 / 门（实心，站旁边按）；enter 留空地。"""
    x, y = xy
    if mode == "interact":
        rock(g, x, y, x + w - 1, y + h - 1, 8)
    objects.append(trigger(name, x, y, "ch13/" + script + ".lua", mode, once=once, w=w, h=h,
                           guard_flag=guard, set_flag=done))


def person(objects, name, role, xy, visible, hidden=None, script=None, facing="down"):
    objects.append(npc(name, *xy, role, facing,
                       script=("ch13/" + script + ".lua") if script else None,
                       visible_flag=visible, hidden_flag=hidden))


def save(objects, short, xy):
    objects.append(facility("facility_cunji_" + short, *xy, "save"))


def finish(short, bgm, g, objects, outdoor):
    mid = "ch13_" + short
    props = [("map_id", mid), ("display_name_key", "ch13.map." + short + ".name"),
             ("region", REGION), ("bgm", bgm), ("chapter", CHAPTER),
             ("outdoor", outdoor), ("can_leave_edge", False)]
    return build_map(mid, g.w, g.h, "terrain_ch13_" + short, props, g, objects)


# ---------------------------------------------------------------------------
# 南明岛 48x36：西南礁、横贯礁路、港口小城、码头、港后丘陵、客栈一角、东出海口、礁头
# ---------------------------------------------------------------------------
def make_nanming():
    g = sea_base(48, 36)
    floor(g, 3, 29, 9, 33)                      # 西南礁盘（haishang 在这里）
    g.rect("overlay", 3, 32, 9, 33, 7)          # 礁上碎石
    floor(g, 5, 19, 7, 28, 2)                   # 横贯礁路：礁盘北口直上小城（trigger_wenxun 横贯 y=28）
    floor(g, 3, 8, 41, 18)                      # 主岛：小城、码头、客栈
    g.rect("ground", 3, 13, 41, 14, 2)          # 横街
    g.rect("ground", 5, 15, 7, 18, 2)           # 礁路进城
    house(g, 4, 9, 9, 11)                       # 原料铺
    house(g, 12, 9, 17, 11)                     # 民居
    house(g, 20, 9, 24, 10)                     # 码头货栈
    g.rect("ground", 19, 17, 34, 18, 2)         # 码头栈道
    floor(g, 21, 19, 22, 24, 2)                 # 两道栈桥
    floor(g, 29, 19, 30, 24, 2)
    g.rect("overlay", 26, 17, 27, 18, 7)        # 码头上的缆桩与碎石
    house(g, 35, 9, 41, 11)                     # 港口客栈
    g.rect("overlay", 36, 16, 40, 18, 6)        # 客栈后院几丛草
    rock(g, 10, 7, 36, 7)                       # 港后丘陵的坡坎：主岛与丘陵之间只有一处坡口
    floor(g, 25, 7, 27, 7, 2)                   # 坡口（trigger_qiuling 横贯这三格）
    floor(g, 10, 2, 36, 6)                      # 港后丘陵
    g.rect("overlay", 11, 2, 22, 4, 6)
    g.rect("overlay", 29, 3, 35, 5, 6)
    for x0, y0 in ((13, 5), (17, 3), (31, 2), (34, 4)):
        rock(g, x0, y0, x0, y0)                 # 坡上几株树
        g.set("front", x0, y0 - 1, 4)
    floor(g, 42, 13, 42, 15, 2)                 # 东出海口（一格宽三格长，trigger_chugang 竖着横贯）
    floor(g, 43, 11, 46, 17)                    # 出海口外礁头
    g.rect("overlay", 43, 11, 46, 12, 7)
    g.rect("overlay", 44, 16, 46, 17, 7)
    objects = [spawn("haishang", 6, 29, "up", True)]
    save(objects, "nanming", (5, 29))
    objects.append(facility("facility_dazuo_nanming", 40, 12, "meditate"))
    hook(g, objects, "trigger_wenxun", "wenxun", (5, 28), "enter", "ch12.done", "ch13.wenxun", w=3)
    hook(g, objects, "trigger_qiuling", "gongdao", (25, 7), "enter", "ch13.caigou", "ch13.gongdao", w=3)
    hook(g, objects, "trigger_chugang", "zhuibing", (42, 13), "enter", "ch13.panzha", "ch13.zhuibing", h=3)
    person(objects, "npc_yuanliao_zhanggui", "nanming_zhanggui", (7, 12), "ch13.wenxun", script="caigou")
    person(objects, "npc_sanxian_laozhe", "sanxian_laozhe", (20, 16), "ch13.wenxun", "ch13.gongdao")
    person(objects, "npc_cang_yunlong", "cang_yunlong", (25, 16), "ch13.gongdao", "ch13.panzha",
           script="panzha")
    person(objects, "npc_liu_hufa", "liu_hufa", (27, 16), "ch13.gongdao", "ch13.panzha")
    person(objects, "npc_xin_ming", "xin_ming", (23, 16), "ch13.gongdao", "ch13.panzha")
    person(objects, "npc_ling_yuling", "ling_yuling", (45, 14), "ch13.zhuibing", "ch13.yinjian",
           script="yinjian", facing="left")
    return finish("nanming", "bgm_river", g, objects, True)


# ---------------------------------------------------------------------------
# 天星城（戒严）56x44，南 → 北：码头、城门、城里空街、圣山脚坊市门、第四层宅院、
# 第三十九层洞府、第四十九层石阶、第五十层外廊、星空殿传送大厅
# ---------------------------------------------------------------------------
def make_tianxing():
    g = sea_base(56, 44)
    floor(g, 19, 36, 35, 40, 2)                 # 南端码头（chengwai 在这里）
    floor(g, 21, 41, 22, 42, 2)                 # 栈桥
    floor(g, 32, 41, 33, 42, 2)
    g.rect("overlay", 19, 39, 24, 40, 7)
    floor(g, 26, 35, 28, 35, 2)                 # 码头北口（trigger_rucheng 横贯这三格）
    floor(g, 23, 33, 31, 34)                    # 城门前空地（两位护法）
    g.rect("ground", 26, 33, 28, 34, 2)
    floor(g, 1, 1, 54, 32)                      # 城与圣山
    rock(g, 1, 1, 54, 21)                       # 圣山山体，下面一层层凿出来
    rock(g, 1, 32, 54, 32)                      # 南城墙
    rock(g, 1, 22, 1, 31)
    rock(g, 54, 22, 54, 31)
    floor(g, 26, 32, 28, 32, 2)                 # 城门
    g.rect("ground", 26, 22, 28, 31, 2)         # 城中直街
    g.rect("ground", 2, 27, 53, 27, 2)          # 横街
    rock(g, 6, 22, 16, 24)                      # 圣山脚坊市（trigger_fangshi_bimen 在它南墙上）
    for rect in ((19, 23, 24, 25), (31, 23, 36, 25), (40, 23, 46, 25), (48, 23, 52, 25),
                 (3, 29, 9, 30), (12, 29, 20, 30), (33, 29, 40, 30), (44, 29, 52, 30)):
        house(g, *rect)                         # 闭户的人家（戒严，空街）
    g.rect("overlay", 2, 31, 10, 31, 7)
    floor(g, 26, 20, 28, 21, 2)                 # 上山石阶
    floor(g, 22, 16, 35, 19)                    # 第四层
    rock(g, 36, 16, 43, 19)                     # 第四层宅院（院墙西面一格是 trigger_zhaiyuan）
    g.rect("front", 36, 15, 43, 15, 4)
    g.rect("overlay", 23, 18, 30, 19, 6)
    floor(g, 23, 15, 24, 15, 2)                 # 第四层 → 第三十九层
    floor(g, 6, 10, 26, 14)                     # 第三十九层
    rock(g, 8, 10, 13, 12)                      # 洞府（门 trigger_dongfu39 在南墙上）
    g.rect("ground", 15, 10, 18, 11, 3)         # 六槽灵田（翻土，可走）
    rock(g, 20, 10, 22, 11)                     # 静室
    g.rect("overlay", 7, 13, 25, 14, 6)         # 院中花草
    floor(g, 24, 7, 26, 9, 2)                   # 第四十九层石阶（trigger_lanlu 横贯 y=9）
    floor(g, 24, 5, 26, 6)                      # 第五十层外廊（trigger_xingkong 横贯 y=5）
    g.rect("overlay", 24, 6, 26, 6, 7)
    floor(g, 14, 1, 38, 4)                      # 星空殿传送大厅
    g.rect("overlay", 23, 1, 29, 2, 7)          # 传送阵阵纹
    objects = [spawn("chengwai", 27, 36, "up", True)]
    save(objects, "tianxing", (26, 36))
    objects.extend([
        facility("facility_yaoyuan_39", 16, 11, "field", ref_id="field_tianxing_39",
                 slots=6, require_flag="ch13.rucheng"),
        facility("facility_jingshi_39", 21, 12, "meditate", effectiveness=125, require_flag="ch13.rucheng")])
    hook(g, objects, "trigger_rucheng", "rucheng", (26, 35), "enter", "ch13.yinjian", "ch13.rucheng", w=3)
    hook(g, objects, "trigger_fangshi_bimen", "fangshi_bimen", (11, 24), "interact", None, None, once=False)
    hook(g, objects, "trigger_zhaiyuan", "qianru", (36, 18), "interact", "ch13.tanzhen", "ch13.qianru")
    hook(g, objects, "trigger_dongfu39", "qifu", (10, 12), "interact", "ch13.rucheng", "ch13.qifu")
    hook(g, objects, "trigger_lanlu", "yirong", (24, 9), "enter", "ch13.qianru", "ch13.yirong", w=3)
    hook(g, objects, "trigger_xingkong", "tanzhen", (24, 5), "enter", "ch13.qifu", "ch13.tanzhen", w=3)
    person(objects, "npc_zhao_hufa", "zhao_hufa", (24, 33), "ch13.yinjian", "ch13.chuansong")
    person(objects, "npc_yu_hufa", "yu_hufa", (30, 33), "ch13.yinjian", "ch13.chuansong")
    person(objects, "npc_gu_laozhe", "gu_laozhe", (30, 2), "ch13.yirong", "ch13.chuansong",
           script="chuansong", facing="left")
    person(objects, "npc_qian_xiushi", "qian_xiushi", (32, 3), "ch13.yirong", "ch13.chuansong", facing="left")
    person(objects, "npc_yi_jing_tx", "yi_jing", (20, 3), "ch13.yirong", "ch13.chuansong", facing="right")
    person(objects, "npc_zhang_xiushi", "zhang_xiushi", (18, 2), "ch13.yirong", "ch13.chuansong",
           facing="right")
    return finish("tianxing", "bgm_town", g, objects, True)


# ---------------------------------------------------------------------------
# 外星海 72x56：北区黑石城（祭坛石屋、明珠轩、寸金阁、告示墙、西门与西门外礁头），
# 中部礁路，西南秘市荒岛，东南小岛
# ---------------------------------------------------------------------------
def make_waihai():
    g = sea_base(72, 56)
    # 北区黑石城：城墙一圈，西门、南门两处口子
    floor(g, 10, 1, 50, 21)
    rock(g, 10, 1, 50, 1)
    rock(g, 10, 21, 50, 21)
    rock(g, 10, 1, 10, 21)
    rock(g, 50, 1, 50, 21)
    floor(g, 10, 11, 10, 13, 2)                 # 西门（trigger_chucheng 竖着横贯这三格）
    floor(g, 29, 21, 31, 21, 2)                 # 南门
    g.rect("ground", 11, 11, 49, 13, 2)         # 东西大街
    g.rect("ground", 29, 14, 31, 20, 2)         # 南门直街
    rock(g, 28, 2, 40, 10)                      # 祭坛高台（石屋凿在台顶）
    floor(g, 31, 3, 37, 7)                      # 祭坛石屋（shiwu 在屋内）
    g.rect("overlay", 31, 3, 37, 3, 7)
    floor(g, 34, 8, 34, 8, 2)                   # 石屋门
    floor(g, 33, 9, 35, 10, 2)                  # 门外台阶（trigger_shiwu 横贯 y=9）
    house(g, 13, 3, 21, 8)                      # 明珠轩
    house(g, 42, 3, 48, 8)                      # 寸金阁
    for rect in ((13, 16, 20, 19), (35, 16, 41, 19), (43, 16, 48, 19)):
        house(g, *rect)
    g.rect("overlay", 22, 17, 27, 19, 7)
    floor(g, 4, 11, 9, 13, 2)                   # 西门外礁头
    floor(g, 3, 12, 3, 12, 2)                   # 礁头尖（trigger_hui_wuzhong）
    g.rect("overlay", 4, 11, 6, 11, 7)
    g.rect("overlay", 4, 13, 6, 13, 7)
    # 中部礁路：南门外一条主干，往西南、东南各分一支
    floor(g, 29, 22, 31, 29, 2)                 # 主干（trigger_feixu 横贯 y=22）
    floor(g, 14, 27, 28, 29, 2)                 # 西支
    floor(g, 14, 30, 16, 37, 2)
    floor(g, 32, 27, 56, 29, 2)                 # 东支
    floor(g, 54, 30, 56, 36, 2)                 # 东支末段（trigger_jiaoshi 横贯 y=36）
    for x0 in (20, 26, 38, 46):
        g.rect("overlay", x0, 27, x0 + 1, 29, 7)
    # 西南秘市荒岛：西岸秘市岸、乱石堆、东侧绿山小洞府
    floor(g, 13, 38, 31, 52)
    g.rect("overlay", 13, 44, 16, 50, 7)        # 秘市岸的碎石滩
    rock(g, 20, 40, 24, 42)                     # 乱石堆（石壁 trigger_mishi 在南面）
    g.rect("overlay", 19, 43, 25, 43, 7)
    rock(g, 27, 44, 31, 49)                     # 绿山
    g.rect("front", 27, 43, 31, 43, 4)
    rock(g, 27, 47, 27, 47, 8)                  # 小洞府门（摆设）
    g.rect("overlay", 18, 48, 25, 51, 6)
    # 东南小岛：外礁、半山、巨石后洞口
    floor(g, 46, 37, 66, 52)
    rock(g, 50, 40, 64, 48)                     # 小岛山体
    floor(g, 55, 43, 60, 46)                    # 半山平台
    floor(g, 57, 47, 58, 48, 2)                 # 上半山的石径
    rock(g, 57, 44, 58, 45)                     # 巨石
    g.rect("overlay", 46, 49, 66, 52, 7)
    g.rect("overlay", 47, 38, 53, 39, 6)
    objects = [spawn("shiwu", 34, 7, "down", True)]
    save(objects, "qiyuan", (33, 7))
    save(objects, "mishi", (MISHI_SHORE[0] - 1, MISHI_SHORE[1]))
    hook(g, objects, "trigger_shiwu", "daoqiyuan", (33, 9), "enter", "ch13.chuansong", "ch13.daoqiyuan", w=3)
    hook(g, objects, "trigger_xuanshang", "xuanshang", (25, 15), "interact", None, None, once=False)
    hook(g, objects, "trigger_chucheng", "chucheng", (10, 11), "enter", "ch13.cunjin", "ch13.chucheng", h=3)
    hook(g, objects, "trigger_hui_wuzhong", "hui_wuzhong", (3, 12), "enter", "ch13.lifu", None, once=False)
    hook(g, objects, "trigger_feixu", "feixu", (29, 22), "enter", "ch13.shouchao", None, once=False, w=3)
    hook(g, objects, "trigger_mishi", "mishi", (22, 42), "interact", "ch13.lidao", "ch13.jiaohuan")
    hook(g, objects, "trigger_jiaoshi", "jianshi", (54, 36), "enter", "ch13.wenshi", "ch13.jianshi", w=3)
    hook(g, objects, "trigger_dongkou", "banyao", (61, 44), "interact", "ch13.jianshi", "ch13.done")
    for name, role, xy, script in (("npc_yi_jing", "yi_jing", (31, 11), None),
                                   ("npc_xu_yun", "xu_yun", (37, 11), None),
                                   ("npc_ming_furen", "ming_furen", (16, 9), "mingzhu"),
                                   ("npc_li_xun", "li_xun", (43, 9), "cunjin"),
                                   ("npc_gu_yu", "gu_yu", (46, 9), None)):
        person(objects, name, role, xy, "ch13.chuansong", "ch13.shouchao", script=script)
    person(objects, "npc_paimaishi", "jingbao_paimaishi", (19, 9), "ch13.mingzhu", "ch13.shouchao")
    person(objects, "npc_huang_mingli", "huang_mingli", (25, 43), "ch13.lidao", "ch13.jiaohuan",
           facing="left")
    person(objects, "npc_wen_siyue", "wen_siyue", (26, 47), "ch13.jiaohuan", "ch13.wenshi",
           script="wenshi", facing="left")
    return finish("waihai", "bgm_river", g, objects, True)


# ---------------------------------------------------------------------------
# 雾中岛 64x48：东南登岸礁、东岸与码头、山脉凹谷洞府（甬道两条带、六扇门、静室丹炉药园）、
# 林边光幕、西海珊瑚礁
# ---------------------------------------------------------------------------
def make_wuzhong():
    g = sea_base(64, 48)
    floor(g, 17, 2, 54, 44)                     # 雾中岛
    rock(g, 17, 2, 46, 21)                      # 山脉
    floor(g, 20, 4, 43, 12)                     # 凹谷洞府（六扇门开在北壁与东壁上）
    g.rect("ground", 21, 8, 26, 11, 3)          # 药园（翻土，可走）
    g.rect("overlay", 34, 9, 40, 12, 6)
    g.rect("overlay", 20, 4, 43, 4, 7)          # 石室门前一行碎石
    floor(g, 30, 13, 32, 21, 2)                 # 洞府甬道（内侧 trigger_dizhen y=15、外侧 trigger_chuguan y=19）
    for x0, y0 in ((19, 23), (22, 26), (44, 23), (47, 26), (40, 30), (24, 31)):
        rock(g, x0, y0, x0 + 1, y0 + 1)         # 谷外散树
        g.rect("front", x0, y0 - 1, x0 + 1, y0 - 1, 4)
    rock(g, 37, 23, 41, 26)                     # 林边：光幕外那片林子
    g.rect("front", 37, 22, 41, 22, 4)
    g.rect("overlay", 33, 22, 36, 26, 6)
    rock(g, 19, 37, 52, 43)                     # 岛南密林
    g.rect("front", 19, 36, 52, 36, 4)
    g.rect("overlay", 47, 2, 54, 21, 7)         # 东岸礁滩
    floor(g, 55, 15, 60, 17, 2)                 # 码头（入口一行 trigger_lidao）
    floor(g, 61, 16, 61, 16, 2)                 # 码头尽头（trigger_qiyuan_hang）
    floor(g, 55, 33, 57, 35, 2)                 # 东南礁道
    floor(g, 58, 33, 60, 35, 2)
    floor(g, 58, 36, 60, 36, 2)                 # 登岸礁北口（trigger_lifu 横贯这三格）
    floor(g, 57, 37, 61, 41)                    # 东岸登岸礁（andeng 在这里）
    g.rect("overlay", 57, 40, 61, 41, 7)
    floor(g, 12, 30, 16, 32, 2)                 # 西海礁路（trigger_buzhen 竖着横贯 x=12）
    floor(g, 2, 22, 11, 42)                     # 西海珊瑚礁
    g.rect("overlay", 2, 22, 11, 42, 7)
    for rect in ((3, 23, 5, 25), (8, 24, 10, 26), (2, 34, 3, 37), (9, 36, 11, 38)):
        pool(g, *rect)                          # 礁间水潭
    for x0, y0 in ((4, 29), (8, 29), (4, 33), (8, 33)):
        g.rect("overlay", x0, y0, x0, y0, 6)    # 珊瑚礁四阵的阵脚
    objects = [spawn("andeng", 59, 37, "up", True)]
    save(objects, "wz", (58, 37))
    objects.extend([
        facility("facility_jingshi_wz", 42, 10, "meditate", effectiveness=150, require_flag="ch13.lifu"),
        facility("facility_danlu_wz", 40, 5, "alchemy", grade=4, require_flag="ch13.lifu"),
        facility("facility_yaoyuan_wz", 23, 9, "field", ref_id="field_wuzhong", slots=6,
                 require_flag="ch13.lifu")])
    hook(g, objects, "trigger_lifu", "lifu", (58, 36), "enter", "ch13.chucheng", "ch13.lifu", w=3)
    for name, script, x, guard, done in (
            ("trigger_chongshi", "chongshi", 22, "ch13.lifu", "ch13.chongshi"),
            ("trigger_shichuang", "xuanyin", 26, "ch13.chongshi", "ch13.xuanyin"),
            ("trigger_lianjianshi", "chonglian", 30, "ch13.xuanyin", "ch13.chonglian"),
            ("trigger_biguanshi", "wusezhu", 34, "ch13.dujie", "ch13.wusezhu"),
            ("trigger_danfang", "liandan", 38, "ch13.chongmo", "ch13.liandan")):
        hook(g, objects, name, script, (x, 3), "interact", guard, done)
    hook(g, objects, "trigger_jingshimen", "biguan", (44, 8), "interact", "ch13.liandan", "ch13.biguan")
    hook(g, objects, "trigger_yanghun", "yanghun", (27, 10), "interact", None, None, once=False)
    hook(g, objects, "trigger_dizhen", "dujie", (30, 15), "enter", "ch13.chonglian", "ch13.dujie", w=3)
    hook(g, objects, "trigger_chuguan", "chuguan", (30, 19), "enter", "ch13.biguan", "ch13.chuguan", w=3)
    hook(g, objects, "trigger_buzhen", "buzhen", (12, 30), "enter", "ch13.wusezhu", "ch13.buzhen", h=3)
    hook(g, objects, "trigger_youyao", "youyao", (6, 31), "interact", "ch13.buzhen", None, once=False)
    hook(g, objects, "trigger_shoushou", "chongmo", (3, 40), "interact", "ch13.buzhen", "ch13.chongmo")
    hook(g, objects, "trigger_lidao", "lidao", (55, 15), "enter", "ch13.gongsun", "ch13.lidao", h=3)
    hook(g, objects, "trigger_qiyuan_hang", "qiyuan_hang", (61, 16), "enter", "ch13.lifu", None, once=False)
    person(objects, "npc_gongsun_xing", "gongsun_xing", (35, 24), "ch13.chuguan", "ch13.gongsun",
           script="gongsun", facing="left")
    person(objects, "npc_sun_shishu", "sun_shishu", (35, 26), "ch13.chuguan", "ch13.gongsun", facing="left")
    return finish("wuzhong", "bgm_valley", g, objects, True)


BUILDERS = (make_nanming, make_tianxing, make_waihai, make_wuzhong)


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
