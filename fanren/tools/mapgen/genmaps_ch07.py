# -*- coding: utf-8 -*-
"""第 7 章地图生成器：重跑即重建下列九张图与九个 tileset，并给第 6 章的黄枫谷、百药园就地补对象。

    python tools/mapgen/genmaps_ch07.py
    python tools/mapgen/genmaps_ch07.py --check   # 只比对，不写盘；有漂移则退出码 1
    python tools/validate.py                      # 自检

    （`genmaps.py --check` 会把本章这九张连同两处修补一并盯上，build.bat 里仍只有那一条。）

产出（全部写进 maps/，不经手 scripts/ 与 data/）：

    maps/ch07_yuelu_dian.tmj        48x36  岳麓殿（节点 4、5、35a）
    maps/ch07_dihuo.tmj             32x24  地火屋（节点 35b、35c、36）
    maps/ch07_fangshi.tmj           48x30  黄枫谷坊市（节点 11、12）
    maps/ch07_shandong.tmj          32x24  太岳山脉外围·山洞（节点 13、14）
    maps/ch07_jindi_wai.tmj         48x36  禁地外：荒山与黄土坡（节点 19、20、32b）
    maps/ch07_jindi_waiwei.tmj      48x36  禁地外围（节点 21、22）
    maps/ch07_jindi_zhongxin.tmj    48x36  禁地中心区外（节点 23、24、25）
    maps/ch07_huanxingshan.tmj      48x40  环形山（节点 26–29、31 落点、32a；支线 Z2）
    maps/ch07_dixia_zhaoze.tmj      36x28  地下沼泽（节点 30、31）
    maps/tilesets/terrain_{yuelu_dian,dihuo,fangshi,shandong,jindi_wai,jindi_waiwei,jindi_zhongxin,huanxingshan,dixia_zhaoze}.tsj

    另就地修补 maps/ch06_huangfenggu.tmj 与 maps/ch06_baiyaoyuan.tmj（见第五节）。

施工图：docs/ch07-design.md。挂点的 mode / once / guard_flag / set_flag 一律照它 3.1 那张表，
这份文件只回答「摆在哪一格」。

--------------------------------------------------------------------------
一、连通与闸门（施工图第 4 节）

    ch06_baiyaoyuan ◀──▶ ch06_huangfenggu ──(ch07.chuling)──▶ ch07_fangshi ──teleport(12)──▶ ch07_shandong
    ch07_shandong ──teleport(14)──▶ ch06_baiyaoyuan
    ch06_huangfenggu ──(ch07.danbao)──▶ ch07_yuelu_dian ──(ch07.dihuo)──▶ ch07_dihuo ──teleport(36)──▶ ch06_baiyaoyuan
    ch06_huangfenggu ──teleport(18)──▶ ch07_jindi_wai[荒山] ──teleport(19b)──▶ [黄土坡] ──teleport(20)──▶ ch07_jindi_waiwei
    ch07_jindi_waiwei ──(ch07.yixiantian)──▶ ch07_jindi_zhongxin ──(ch07.yueyang)──▶ ch07_huanxingshan
    ch07_huanxingshan ──teleport(29)──▶ ch07_dixia_zhaoze ──teleport(31)──▶ ch07_huanxingshan[破洞]
    ch07_huanxingshan ──teleport(32a)──▶ ch07_jindi_wai[出口] ──teleport(32b)──▶ ch06_baiyaoyuan

    五道闸门各指一张不同的图，deny_text_key 按目标图命名（规则 20）：ch07.block.fangshi / yuelu_dian / dihuo /
    jindi_zhongxin / huanxingshan。坊市、岳麓殿、地火屋各有回程的门（不设闸）；禁地四张图只进不出。

    出入口按「往哪边走进门，落点就面朝哪边」摆（规则 29）：黄枫谷通坊市的门在南缘（往下走进去，坊市落点在北口
    面朝下）；通岳麓殿的门在北缘（往上走进去，岳麓殿落点在大厅南端的传送阵面朝上）；岳麓殿通地火屋的五彩石门
    在北缘（往上走进去，地火屋落点在坞石通道底面朝上）；回程三道门反过来。外围通中心区、中心区通环形山都在北缘。

二、每一处挂战斗的格子前面都有存档点（施工图第 4 节）

    ① 山洞  facility_cunji 在洞里    ② 外围  在落点    ③ 中心区外  在树林入口
    ④ 环形山  facility_cunji_dian 在小石殿前那条上山路上    ⑤ 沼泽  在石阶出口

三、剧情传送的落点（脚本照这里写，改了要一起改）

    SHANDONG_DONGNEI   (8, 9)    山洞洞内（scripts/ch07/fangshi_chu.lua）
    BAIYAOYUAN_LUKOU   (24, 10)  百药园茅屋前那条路（fanhui.lua、chukou.lua、zhuji.lua）
    JINDI_WAI_HUANGSHAN (5, 4)   禁地外·荒山的石坳（jihe.lua）
    JINDI_WAI_POXIA    (23, 25)  禁地外·黄土坡（liedui.lua）
    JINDI_WAI_CHUKOU   (23, 34)  禁地外·出口通道底（xiashan.lua）
    WAIWEI_LUODIAN     (5, 31)   禁地外围的落点（pojin.lua）
    ZHAOZE_SHIJIE      (3, 2)    地下沼泽的石阶口（qingshidian.lua）
    HXS_POKONG         (38, 35)  环形山地面破洞旁（jiaoshi.lua）

四、踏入型卡在咽喉上，一格只挂一个对象（施工图 3.1 末段）

    enter  —— trigger_yuelu_ru（传送阵口）、trigger_shimen（石门前）、trigger_murong（黄枫谷东口咽喉）、
              trigger_fangshi_ru / trigger_fangshi_chu（坊市南北口）、trigger_ma_songyao（百药园门内）、
              trigger_jihe（大殿门前）、trigger_xiang（荒山石坳口）、trigger_pojin / trigger_chukou（破禁通道两头）、
              trigger_wulongtan（潭边树丛的缺口）、trigger_yixiantian（一线天北口）、trigger_tongmen（铜门内）、
              trigger_yueyang（上山小路口）、trigger_kuitan / trigger_youdi（紫猴花洞拐角与石厅口）、
              trigger_xiaoshidian（小石殿前的豁口）、trigger_guanzhan（黑土堆后）、trigger_xiashan（破洞外）、
              trigger_xieshili（茅屋门内）、trigger_shijiu（十九号门内）
    interact —— 贴在一件东西上：桌、柜台、案、篓、树、石、门、蒲团、人（吴风、丑汉身上那一格：他们没挂脚本，
              规则 17 不算遮蔽）；只有 trigger_charen（洞道中段）、trigger_liangshi（崖下两具尸首）、
              trigger_yuanjiao（园角田边）、trigger_jiaoshi（蛟尸旁）、trigger_yujian_hantan（寒潭边）在可走格上。

五、黄枫谷、百药园：就地修补，不重建（施工图第 17 节第 25 条）

    两张图的地形与第 6 章的对象归 genmaps_ch06.py 所有。本文件只加第 7 章的对象、开两道门、摆几件家具
    （报名的案、拆包的桌、读方的书桌、药篓、园角两槽灵田），第 6 章已有的对象一个不动。幂等。
    genmaps_ch06.build_all() 与 genmaps.py 的 check() 在拿到第 6 章那一份之后回头调用 patch_huangfenggu() /
    patch_baiyaoyuan()——单跑第 6 章的生成器也不会把第 7 章从图上抹掉（第 5 章给南城补东门的同一个做法）。

六、瓦片 gid 约定沿用 genmaps.py：1 主地表 2 道路 3 特殊地表（挡路=水面，可走=翻土） 4 front 5 building 主体
    6/7 overlay 8 building 次体（家具、摊架、门扇、界石）。
"""
import os
import random
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import genmaps  # noqa: E402  MAPS 要运行时取
from genmaps import (  # noqa: E402
    Grid,
    build_map,
    check_maps,
    chest,
    dump_map,
    facility,
    foe,
    gate,
    gather,
    npc,
    portal,
    spawn,
    trigger,
    write_tileset,
)
from genmaps_ch05 import border, eaves_row, floor, road, scatter, tilled, tree, water  # noqa: E402

CHAPTER = 7
PREFIX = "ch07_"

SHANDONG_DONGNEI = (8, 9)
BAIYAOYUAN_LUKOU = (24, 10)
JINDI_WAI_HUANGSHAN = (5, 4)
JINDI_WAI_POXIA = (23, 25)
JINDI_WAI_CHUKOU = (23, 34)
WAIWEI_LUODIAN = (5, 31)
ZHAOZE_SHIJIE = (3, 2)
HXS_POKONG = (38, 35)

# 伙伴改版样板（docs/ch07-companions-design.md）：地底石窟里破顶的那一处。
# 岩层最薄处在 SHIKU_YANCENG（实体格，挂 trigger_poding）；人站在它正下方 SHIKU_DING_XIA 按确认。
# 破顶打赢，poding.lua 把人送回再下一格 SHIKU_TUIBU（碎石塌落、退开一步），再往上一迈踩 trigger_wenming。
# 打输：poding.lua 送到打坐点旁 SHIKU_DAZUO_PANG（施工图 5.1「破顶打输了从这里重来」）。
SHIKU_YANCENG = (21, 2)
SHIKU_DING_XIA = (21, 3)
SHIKU_TUIBU = (21, 4)
SHIKU_DAZUO_PANG = (24, 15)

# 伙伴的 id（插话的 require_party 只收伙伴 id，契约 2.4）
NANGONG = "nangong_wan"
CHEN = "chen_qiaoqian"

# 明雷的巡逻路线（2026-11-20 用户试玩：「敌人不动的确很呆」；docs/ch07-playtest-foes.md 第 4 节逐只说明）。
# 从明雷所在那一格走到这里列的格、再原路走回去；只列自己那一格是原地张望。历练副本与原图同一只、同一条路。
# 选路的规矩：只走可走格，横平竖直、四五格长；不横在主线必经的窄口与出入口上（门禁规则 36 按「整条路连同
# 上下左右一格都当墙」量过目标链）；整条路连同旁边一格都不挨传送点、踏入触发区（规则 32，引擎也不让它站上去）。
PATROL_ZHAOZE_CHAN = [(31, 20)]          # 地下沼泽东岸那一溜：(31,17)↕(31,20)，北边窄口与南边石阶都不碰
PATROL_WAIWEI_CHAN = [(45, 10)]          # 禁地外围乌龙潭北岸：(42,10)↔(45,10)，潭边缺口 (38,12) 在它西边四格外
PATROL_WAIWEI_XI = [(3, 22)]             # 外围西边那一大片的西缘：(3,18)↕(3,22)，主路走中间与东边
PATROL_ZHONGXIN_LIN = [(8, 30)]          # 中心区外树林西南角：(8,26)↕(8,30)，主路在东边的落点与铜门之间
PATROL_ZHONGXIN_YUAN = [(34, 4)]         # 铜门北边那一大片的北缘：(30,4)↔(34,4)，主路贴着第 8–13 行去上山小路
PATROL_HXS_ZHONG = [(20, 17)]            # 环形山中央盆地西侧：(20,20)↕(20,17)，盆地十几格宽，主路从东边过
PATROL_HXS_DONG = [(44, 29)]             # 东坡石屋南边那一溜：(44,24)↕(44,29)，青石殿与破洞都从南边那片空地去
PATROL_HXS_NAN = [(32, 33)]              # 山脚空地：(28,33)↔(32,33)，落点 (24,38) 往北那条路在它西边
PATROL_SHENCHU_CHAN = [(16, 21)]         # 沼泽深处西南：(12,21)↔(16,21)，往东口的路还能从第 15–20 行东边绕
PATROL_SHENCHU_SHU = [(29, 4)]           # 北边矿脉东侧：(25,4)↔(29,4)，背对矿脉走开时就是去采的时候
PATROL_SHENCHU_SHE = [(36, 17)]          # 东边那一溜：(36,14)↕(36,17)，不挨岔道口的插话（35,9）
PATROL_SHENCHU_E = [(33, 26)]            # 精英碧水鳄守着水湾里的宝箱与矿脉：原地转身张望


def map_props(map_id, bgm, outdoor=True):
    return [
        ("map_id", map_id),
        # 规则 21：display_name_key 由地图 id 推出来。
        ("display_name_key", "ch07.map.%s.name" % map_id[len(PREFIX):]),
        ("region", "jianzhou"),        # 施工图第 4 节：一律建州（禁地在建州以北，ch174）
        ("bgm", bgm),
        ("chapter", CHAPTER),
        ("outdoor", outdoor),
        ("can_leave_edge", False),
    ]


def solid_all(g):
    g.fill("building", 5)
    g.fill("collision", 1)


def rock(g, x0, y0, x1, y1):
    g.solid_rect(x0, y0, x1, y1, 5)


def thing(g, x0, y0, x1=None, y1=None):
    """家具、摊架、界石：building 8 + 挡路。"""
    g.solid_rect(x0, y0, x0 if x1 is None else x1, y0 if y1 is None else y1, 8)


def edge_fronts(g, rnd, chance=0.3):
    """挡路的实体边上压一层 front（坡檐、树影），只动 front，不影响通行。"""
    for y in range(g.h):
        for x in range(g.w):
            if not g.walkable(x, y) and g.get("ground", x, y) != 3 and any(
                    g.walkable(x + dx, y + dy) for dx, dy in ((0, 1), (0, -1), (1, 0), (-1, 0))):
                if rnd.random() < chance:
                    g.set("front", x, y, 4)


# ---------------------------------------------------------------------------
# 地图 1：岳麓殿 48x36 —— 节点 4、5、35a
#
# 巫钧山腹里一座钟乳洞改的大厅。传送阵在大厅南端一个小间里（落点，4a），往北是大厅；
# 大厅三条通道：北「器」道（到头是一扇关着的门，装饰）、西「丹」道（许老的屋，柜台 5a；
# 楼梯上去是藏室，书桌 4b）、东边那条没标记的通道（北上到石门前的小广场：五彩石门 portal_to_dihuo、
# 门前一格 5b、丑汉守在石屋门口 35a）。
# ---------------------------------------------------------------------------
def make_yuelu_dian():
    W, H = 48, 36
    g = Grid(W, H)
    rnd = random.Random(701)
    g.fill("ground", 1)
    solid_all(g)

    floor(g, 15, 22, 32, 30)                 # 大厅
    for x, y in ((17, 24), (30, 24), (17, 28), (30, 28), (21, 26), (26, 26)):
        rock(g, x, y, x, y)                  # 钟乳石柱
    road(g, 23, 31, 24, 35)                  # 传送阵小间，南缘是回黄枫谷的阵口
    road(g, 23, 14, 24, 21)                  # 「器」道
    thing(g, 23, 13, 24, 13)                 # 器房的门，关着
    road(g, 8, 25, 14, 26)                   # 「丹」道
    road(g, 7, 19, 8, 24)
    floor(g, 3, 12, 13, 18)                  # 许老的屋
    thing(g, 5, 15, 10, 15)                  # 柜台
    thing(g, 12, 13, 12, 14)                 # 靠墙一排丹鼎
    road(g, 3, 9, 4, 11)                     # 楼梯
    floor(g, 3, 2, 13, 8)                    # 藏室
    thing(g, 6, 2, 12, 2)                    # 书架
    thing(g, 3, 5, 3, 7)                     # 另一排书架
    thing(g, 8, 5, 9, 5)                     # 书桌
    road(g, 33, 25, 38, 26)                  # 没标记的通道
    road(g, 37, 8, 38, 24)
    floor(g, 34, 1, 45, 7)                   # 石门前的小广场
    road(g, 39, 0, 40, 0)                    # 五彩石门
    rock(g, 42, 1, 45, 3)                    # 丑汉的石屋
    for x in (35, 44):
        thing(g, x, 6, x, 6)                 # 门前两只石墩

    objects = [
        spawn("spawn_from_huangfenggu", 23, 33, "up", default=True),
        spawn("spawn_from_dihuo", 39, 2, "down"),
        portal("portal_to_huangfenggu", 23, 35, "ch06_huangfenggu", "spawn_from_yuelu", w=2),
        portal("portal_to_dihuo", 39, 0, "ch07_dihuo", "spawn_from_yuelu", w=2,
               require_flag="ch07.dihuo", deny_text_key="ch07.block.dihuo"),

        # ---- 4a：传送阵出口 ----
        trigger("trigger_yuelu_ru", 23, 32, "ch07/yuelu_ru.lua", "enter", True, w=2,
                guard_flag="ch07.danbao", set_flag="ch07.yuelu"),
        # ---- 4b：藏室书桌 ----
        trigger("trigger_cangshi", 8, 5, "ch07/cangshi.lua", "interact", True, w=2,
                guard_flag="ch07.yuelu", set_flag="ch07.cangshi"),
        # ---- 5a：许老柜台 ----
        trigger("trigger_dihuo_wen", 8, 15, "ch07/dihuo_wen.lua", "interact", True,
                guard_flag="ch07.cangshi", set_flag="ch07.yinsi"),
        # ---- 5b：石门前一格 ----
        trigger("trigger_shimen", 39, 1, "ch07/shimen.lua", "enter", True, w=2,
                guard_flag="ch07.yinsi", set_flag="ch07.chouhan"),
        # ---- 35a：丑汉面前一格（施工图 3.1：同一格只挂一个对象）----
        trigger("trigger_chouhan", 43, 5, "ch07/chouhan.lua", "interact", True,
                guard_flag="ch07.sannian", set_flag="ch07.dihuo"),

        facility("facility_cunji", 16, 23, "save"),

        npc("npc_xu_lao", 8, 14, "xu_lao", "down"),
        npc("npc_chou_han", 43, 4, "chou_han", "down"),
    ]
    edge_fronts(g, rnd, 0.35)
    return build_map("ch07_yuelu_dian", W, H, "terrain_yuelu_dian",
                     map_props("ch07_yuelu_dian", "bgm_indoor", outdoor=False), g, objects)


# ---------------------------------------------------------------------------
# 地图 2：地火屋 32x24 —— 节点 35b、35c、36
#
# 坞石通道从南缘进来（落点），北上一座圆厅，四周石门一圈（装饰）；东北角那一扇通十九号：
# 门内第一格 35b、圆墩（地火，facility_dihuo grade 4）、墩旁石凳 35c、墙角蒲团 36。
# 打坐点不放：此图的日子只由脚本拨（施工图第 4 节）。
# ---------------------------------------------------------------------------
def make_dihuo():
    W, H = 32, 24
    g = Grid(W, H)
    rnd = random.Random(702)
    g.fill("ground", 1)
    solid_all(g)

    road(g, 15, 17, 16, 23)                  # 坞石通道
    floor(g, 6, 7, 25, 16)                   # 圆厅
    for x0, y0 in ((6, 7), (24, 7), (6, 15), (24, 15)):
        rock(g, x0, y0, x0 + 1, y0 + 1)      # 切掉四角，看着圆一些
    for x in (9, 12, 15, 18):
        thing(g, x, 6, x, 6)                 # 北墙上的一排石门
    for y in (10, 13):
        thing(g, 5, y, 5, y)
        thing(g, 26, y, 26, y)
    floor(g, 18, 1, 28, 4)                   # 十九号
    road(g, 22, 5, 22, 6)                    # 十九号的门
    thing(g, 24, 2, 24, 2)                   # 圆墩旁的石凳
    thing(g, 28, 1, 28, 1)                   # 墙角的蒲团

    objects = [
        spawn("spawn_from_yuelu", 15, 21, "up", default=True),
        portal("portal_to_yuelu_dian", 15, 23, "ch07_yuelu_dian", "spawn_from_dihuo", w=2),

        # ---- 35b：十九号门内第一格 ----
        trigger("trigger_shijiu", 22, 4, "ch07/shijiu.lua", "enter", True,
                guard_flag="ch07.dihuo", set_flag="ch07.feidan"),
        # ---- 35c：圆墩旁的石凳 ----
        trigger("trigger_banian", 24, 2, "ch07/banian.lua", "interact", True,
                guard_flag="ch07.feidan", set_flag="ch07.chengdan"),
        # ---- 36：墙角蒲团 ----
        trigger("trigger_zhuji", 28, 1, "ch07/zhuji.lua", "interact", True,
                guard_flag="ch07.chengdan", set_flag="ch07.done"),

        facility("facility_dihuo", 20, 1, "alchemy", w=2, h=2, grade=4, require_flag="ch07.feidan"),
        facility("facility_cunji", 9, 9, "save"),
    ]
    edge_fronts(g, rnd, 0.3)
    return build_map("ch07_dihuo", W, H, "terrain_dihuo",
                     map_props("ch07_dihuo", "bgm_cave", outdoor=False), g, objects)


# ---------------------------------------------------------------------------
# 地图 3：黄枫谷坊市 48x30 —— 节点 11、12
#
# 一条南北街。北口一座门楼（落点，11a 在门楼里第一格；回黄枫谷的门在北缘）；北段临时摊位，
# 西边收药摊（facility_yaotan ＋ 摊主）；南段两排铺面（装饰），东边最气派的一座是万宝楼（门 11b、伙计）；
# 南口（12）是一处门洞，走进去就出坊市。
# ---------------------------------------------------------------------------
def make_fangshi():
    W, H = 48, 30
    g = Grid(W, H)
    rnd = random.Random(703)
    g.fill("ground", 1)
    border(g)

    rock(g, 18, 1, 22, 3)                    # 北口门楼
    rock(g, 25, 1, 29, 3)
    road(g, 23, 0, 24, 3)
    road(g, 21, 4, 26, 27)                   # 南北街
    road(g, 23, 28, 24, 28)                  # 南口门洞
    rock(g, 21, 28, 22, 28)
    rock(g, 25, 28, 26, 28)
    # 北段摊位：街两侧一溜摊架
    for y in (5, 8, 11):
        thing(g, 17, y, 18, y)
        thing(g, 29, y, 30, y)
    thing(g, 19, 7, 19, 7)                   # 收药摊的柜
    for x0, y0 in ((6, 5), (10, 9), (36, 6), (40, 10), (8, 12)):
        thing(g, x0, y0, x0 + 1, y0)
    # 南段铺面
    rock(g, 2, 15, 8, 20)
    rock(g, 10, 15, 19, 20)
    rock(g, 2, 22, 11, 27)
    rock(g, 13, 22, 19, 27)
    rock(g, 28, 15, 37, 22)                  # 万宝楼
    thing(g, 28, 19, 28, 20)                 # 万宝楼的门
    rock(g, 39, 15, 45, 20)
    rock(g, 28, 24, 45, 27)
    for x0, x1, y in ((2, 8, 14), (10, 19, 14), (28, 37, 14), (39, 45, 14), (2, 11, 21), (13, 19, 21),
                      (28, 45, 23)):
        eaves_row(g, x0, x1, y)
    for tx, ty in ((4, 3), (13, 3), (34, 3), (43, 3), (44, 12)):
        tree(g, tx, ty)
    scatter(g, rnd, 60, 2, 4, 45, 13)

    objects = [
        spawn("spawn_from_huangfenggu", 23, 1, "down", default=True),
        portal("portal_to_huangfenggu", 23, 0, "ch06_huangfenggu", "spawn_from_fangshi", w=2),

        # ---- 11a：北口门楼里第一格 ----
        trigger("trigger_fangshi_ru", 23, 2, "ch07/fangshi_ru.lua", "enter", True, w=2,
                guard_flag="ch07.chuling", set_flag="ch07.fangshi"),
        # ---- 11b：万宝楼的门 ----
        trigger("trigger_wanbaolou", 28, 19, "ch07/wanbaolou.lua", "interact", True, h=2,
                guard_flag="ch07.fangshi", set_flag="ch07.wanbaolou"),
        # ---- 12：南口门洞 ----
        trigger("trigger_fangshi_chu", 23, 28, "ch07/fangshi_chu.lua", "enter", True, w=2,
                guard_flag="ch07.wanbaolou", set_flag="ch07.likai"),

        facility("facility_yaotan", 19, 7, "shop", ref_id="ch07_fangshi_yaotan"),
        facility("facility_cunji", 26, 4, "save"),

        npc("npc_fangshi_tanzhu", 19, 6, "fangshi_tanzhu", "right"),
        npc("npc_yuanwu_xiushi", 29, 9, "yuanwu_xiushi", "left"),
        npc("npc_fangshi_zhishi", 20, 12, "fangshi_zhishi", "right"),
        npc("npc_wanbaolou_shicong", 27, 21, "wanbaolou_shicong", "left"),
    ]
    return build_map("ch07_fangshi", W, H, "terrain_fangshi",
                     map_props("ch07_fangshi", "bgm_town"), g, objects)


# ---------------------------------------------------------------------------
# 地图 4：太岳山脉外围·山洞 32x24 —— 节点 13、14
#
# 西半是山，半山腰一个乱石挡着的洞（洞内落点、石壁 13、存档点）；东半洞外一片空地（14）。
# ---------------------------------------------------------------------------
def make_shandong():
    W, H = 32, 24
    g = Grid(W, H)
    rnd = random.Random(704)
    g.fill("ground", 1)
    border(g)

    rock(g, 1, 1, 14, 22)                    # 山体
    floor(g, 4, 6, 11, 12)                   # 洞里
    road(g, 12, 9, 14, 10)                   # 洞口
    rock(g, 15, 7, 16, 8)                    # 挡着洞口的乱石
    rock(g, 15, 11, 16, 12)
    thing(g, 20, 10, 20, 10)                 # 空地上一块大石
    for tx, ty in ((19, 3), (24, 5), (28, 3), (27, 14), (22, 18), (18, 20), (29, 20)):
        tree(g, tx, ty)
    edge_fronts(g, rnd, 0.35)
    scatter(g, rnd, 40, 17, 2, 30, 22)

    objects = [
        spawn("spawn_dongnei", SHANDONG_DONGNEI[0], SHANDONG_DONGNEI[1], "down", default=True),

        # ---- 13：洞里石壁 ----
        trigger("trigger_yeyu", 7, 5, "ch07/yeyu.lua", "interact", True,
                guard_flag="ch07.likai", set_flag="ch07.yeyu"),
        # ---- 14：洞外空地那块大石 ----
        trigger("trigger_fanhui", 20, 10, "ch07/fanhui.lua", "interact", True,
                guard_flag="ch07.yeyu", set_flag="ch07.fanhui"),

        facility("facility_cunji", 10, 11, "save"),
    ]
    return build_map("ch07_shandong", W, H, "terrain_shandong",
                     map_props("ch07_shandong", "bgm_night"), g, objects)


# ---------------------------------------------------------------------------
# 地图 5：禁地外 48x36 —— 节点 19、20、32b
#
# 北半荒山：西北一个石坳（落点，坳口 19a），东边一片空地（队列 19b）。
# 一道横着的石梁把南北隔开，只在东边留一道长坡。
# 南半黄土坡：一道「风刃之墙」横在南边，墙上打出的通道两头各一格（20 入、32b 出）。
#
# 在场（施工图第 5 节；规则 30「前一段的 hidden 恰是后一段的 visible」）：
#   荒山   visible ch07.jihe  / hidden ch07.qipai   黄枫谷一行（他派还没到）
#   黄土坡 visible ch07.qipai / hidden ch07.pojin   进去之后就没再出来的（向之礼要最后一个爬出来，不先站在出口）
#   坡与出口 visible ch07.qipai / hidden ch07.chujindi   结丹们与活着出来的：破禁时站在坡上，五天后还站在那里
#   出口   visible ch07.pojin / hidden ch07.chujindi   穹前辈、钟吾、菡云芝
# 掩月宗的弟子与南宫婉在他落地时还没出来，出口不摆（施工图 16.2 之外的偏差，记第 18 节）。
# ---------------------------------------------------------------------------
def make_jindi_wai():
    W, H = 48, 36
    g = Grid(W, H)
    rnd = random.Random(705)
    g.fill("ground", 1)
    border(g)

    rock(g, 1, 1, 11, 9)                     # 石坳
    floor(g, 3, 2, 8, 6)
    road(g, 9, 4, 11, 5)
    for x0, y0, x1, y1 in ((16, 2, 18, 3), (34, 2, 36, 4), (41, 8, 44, 10), (14, 11, 16, 13), (38, 13, 40, 14)):
        rock(g, x0, y0, x1, y1)
    thing(g, 26, 6, 26, 6)                   # 队列前那根旗杆
    rock(g, 1, 16, 46, 19)                   # 石梁
    road(g, 30, 16, 31, 19)                  # 长坡
    rock(g, 1, 29, 22, 30)                   # 风刃之墙（烘成一道土墙）
    rock(g, 25, 29, 46, 30)
    road(g, 23, 29, 24, 34)                  # 墙上打出的通道
    rock(g, 1, 31, 22, 34)                   # 墙那一头是禁地：只剩通道
    rock(g, 25, 31, 46, 34)
    for x0, y0 in ((6, 23), (14, 21), (38, 22), (43, 25)):
        rock(g, x0, y0, x0 + 1, y0)          # 黄土坡上的石堆
    edge_fronts(g, rnd, 0.25)
    scatter(g, rnd, 50, 12, 2, 45, 15, grass=0.4)

    objects = [
        spawn("spawn_huangshan", JINDI_WAI_HUANGSHAN[0], JINDI_WAI_HUANGSHAN[1], "right", default=True),

        # ---- 19a：石坳口 ----
        trigger("trigger_xiang", 10, 4, "ch07/xiang.lua", "enter", True, h=2,
                guard_flag="ch07.jihe", set_flag="ch07.xiang"),
        # ---- 19b：队列前的旗杆 ----
        trigger("trigger_liedui", 26, 6, "ch07/liedui.lua", "interact", True,
                guard_flag="ch07.xiang", set_flag="ch07.qipai"),
        # ---- 20：通道口 ----
        trigger("trigger_pojin", 23, 29, "ch07/pojin.lua", "enter", True, w=2,
                guard_flag="ch07.qipai", set_flag="ch07.pojin"),
        # ---- 32b：出口第一格 ----
        trigger("trigger_chukou", 23, 33, "ch07/chukou.lua", "enter", True, w=2,
                guard_flag="ch07.xiashan", set_flag="ch07.chujindi"),

        facility("facility_cunji", 4, 6, "save"),
    ]

    def crowd(stage, rows, visible, hidden):
        for role_id, x, y, facing in rows:
            objects.append(npc("npc_%s_%s" % (role_id, stage), x, y, role_id, facing,
                               visible_flag=visible, hidden_flag=hidden))

    # 荒山：黄枫谷一行
    crowd("huangshan", (
        ("li_huayuan", 27, 4, "down"),
        ("chen_shimei", 22, 8, "down"),
        ("xiang_zhili", 14, 6, "left"),
        ("xiang_shaonian", 14, 7, "left"),
        ("huangshan_shijie", 23, 8, "down"),
        ("huangfeng_shilian", 28, 8, "down"),
    ), "ch07.jihe", "ch07.qipai")
    # 黄土坡：进去以后没有站到出口的
    crowd("poxia", (
        ("xiang_zhili", 19, 23, "down"),
        ("xiang_shaonian", 20, 23, "down"),
        ("huangshan_shijie", 18, 25, "right"),
        ("luosai_huzi", 33, 26, "left"),
        ("yanyue_dizi", 36, 21, "down"),
    ), "ch07.qipai", "ch07.pojin")
    # 坡与出口：结丹们，与五天后活着出来的
    crowd("chukou", (
        ("li_huayuan", 21, 26, "down"),
        ("fuyunzi", 27, 26, "down"),
        ("nichang_xianzi", 29, 24, "down"),
        ("jujianmen_gaoren", 16, 26, "right"),
        ("chen_shimei", 19, 27, "right"),
        ("chen_dage", 18, 27, "right"),
        ("huangfeng_shilian", 17, 24, "right"),
        ("qingxumen_daoshi", 30, 27, "left"),
        ("jujianmen_dizi", 12, 25, "right"),
        ("lingshoushan_dizi", 35, 27, "left"),
        ("tianquebao_dizi", 40, 26, "left"),
        ("huadaowu_dizi", 9, 26, "right"),
    ), "ch07.qipai", "ch07.chujindi")
    # 出口：守在外头等的、先出来的
    crowd("chukou", (
        ("qiong_qianbei", 26, 23, "down"),
        ("zhong_wu", 37, 27, "left"),
        ("han_yunzhi", 36, 25, "left"),
    ), "ch07.pojin", "ch07.chujindi")
    return build_map("ch07_jindi_wai", W, H, "terrain_jindi_wai",
                     map_props("ch07_jindi_wai", "bgm_cliff"), g, objects)


# ---------------------------------------------------------------------------
# 地图 6：禁地外围 48x36 —— 节点 21、22
#
# 当中一大块峭壁，只夹出一条两格宽的小路（一线天）北通中心区；北口 22，过了口子才到得了北缘那道门。
# 西南是落点；东北是乌龙潭（潭边一道矮树丛只留一个缺口，21a）；西北一道山崖，崖下茅草地（21b）。
# 禁地里没有能聊天的人，也没有回头的门（施工图第 17 节第 21 条）。
# ---------------------------------------------------------------------------
def make_jindi_waiwei():
    W, H = 48, 36
    g = Grid(W, H)
    rnd = random.Random(706)
    g.fill("ground", 1)
    border(g)

    rock(g, 14, 10, 33, 26)                  # 峭壁
    road(g, 22, 10, 23, 26)                  # 一线天
    road(g, 22, 0, 23, 0)                    # 北缘通中心区的口
    rock(g, 14, 1, 14, 9)                    # 北面那块只从一线天进得去
    rock(g, 33, 1, 33, 9)
    water(g, 37, 3, 44, 8)                   # 乌龙潭
    rock(g, 34, 12, 37, 12)                  # 潭边的矮树丛
    rock(g, 40, 12, 46, 12)
    rock(g, 2, 2, 12, 5)                     # 山崖
    for tx, ty in ((4, 14), (9, 17), (6, 21), (11, 24), (38, 17), (44, 20), (36, 23), (42, 26),
                   (29, 31), (36, 30), (15, 31), (9, 29), (45, 32), (18, 5), (29, 4)):
        tree(g, tx, ty)
    edge_fronts(g, rnd, 0.3)
    scatter(g, rnd, 70, 1, 6, 12, 34, grass=0.5)
    scatter(g, rnd, 60, 15, 27, 46, 34, grass=0.4)
    g.rect("overlay", 2, 7, 5, 11, 6)        # 茅草地

    objects = [
        spawn("spawn_luodian", WAIWEI_LUODIAN[0], WAIWEI_LUODIAN[1], "up", default=True),
        portal("portal_to_jindi_zhongxin", 22, 0, "ch07_jindi_zhongxin", "spawn_from_waiwei", w=2,
               require_flag="ch07.yixiantian", deny_text_key="ch07.block.jindi_zhongxin"),

        # ---- 21a：潭边矮树丛的缺口 ----
        trigger("trigger_wulongtan", 38, 12, "ch07/wulongtan.lua", "enter", True, w=2,
                guard_flag="ch07.pojin", set_flag="ch07.wulongtan"),
        # ---- 21b：崖下两具尸首 ----
        trigger("trigger_liangshi", 7, 6, "ch07/liangshi.lua", "interact", True,
                guard_flag="ch07.wulongtan", set_flag="ch07.sixian"),
        # ---- 22：一线天北口 ----
        trigger("trigger_yixiantian", 22, 10, "ch07/yixiantian.lua", "enter", True, w=2,
                guard_flag="ch07.sixian", set_flag="ch07.yixiantian"),

        facility("facility_cunji", 4, 30, "save"),
    ] + explore_waiwei("ch07_jindi_waiwei")
    return build_map("ch07_jindi_waiwei", W, H, "terrain_jindi_waiwei",
                     map_props("ch07_jindi_waiwei", "bgm_wild"), g, objects)


# ---------------------------------------------------------------------------
# 地图 7：禁地中心区外 48x36 —— 节点 23、24、25
#
# 南半树林（落点、林边大树 23 与树冠打坐点）；一道石墙横过全图，青铜门是唯一的口（门内第一格 24）；
# 北半第一层花园（树洞打坐点）；东北一条上山小路，路口 25，尽头北缘通环形山。
# ---------------------------------------------------------------------------
def make_jindi_zhongxin():
    W, H = 48, 36
    g = Grid(W, H)
    rnd = random.Random(707)
    g.fill("ground", 1)
    border(g)

    rock(g, 1, 14, 22, 14)                   # 石墙
    rock(g, 25, 14, 46, 14)
    road(g, 23, 13, 24, 15)                  # 青铜门
    rock(g, 30, 24, 31, 25)                  # 林边那棵大树
    g.rect("front", 29, 22, 32, 25, 4)
    rock(g, 8, 5, 9, 6)                      # 花园里那棵空心老树
    g.rect("front", 7, 4, 10, 6, 4)
    rock(g, 39, 1, 39, 7)                    # 上山小路两侧的山石
    rock(g, 42, 1, 42, 7)
    road(g, 40, 0, 41, 7)
    road(g, 22, 16, 23, 34)                  # 林间小路
    for tx, ty in ((4, 18), (8, 21), (13, 17), (17, 24), (5, 28), (11, 31), (15, 28), (27, 18),
                   (36, 19), (41, 23), (38, 29), (44, 31), (33, 32), (27, 30), (44, 17), (19, 32)):
        tree(g, tx, ty)
    for tx, ty in ((15, 4), (28, 6), (33, 10), (18, 10), (5, 11)):
        tree(g, tx, ty)
    edge_fronts(g, rnd, 0.25)
    scatter(g, rnd, 90, 1, 1, 38, 12, grass=0.9)
    scatter(g, rnd, 60, 1, 16, 46, 34, grass=0.6)

    objects = [
        spawn("spawn_from_waiwei", 22, 34, "up", default=True),
        portal("portal_to_huanxingshan", 40, 0, "ch07_huanxingshan", "spawn_from_zhongxin", w=2,
               require_flag="ch07.yueyang", deny_text_key="ch07.block.huanxingshan"),

        # ---- 23：林边大树 ----
        trigger("trigger_shulin", 30, 25, "ch07/shulin.lua", "interact", True,
                guard_flag="ch07.yixiantian", set_flag="ch07.fengyue"),
        # ---- 24：铜门内第一格 ----
        trigger("trigger_tongmen", 23, 13, "ch07/tongmen.lua", "enter", True, w=2,
                guard_flag="ch07.fengyue", set_flag="ch07.zhongwu"),
        # ---- 25：上山小路口 ----
        trigger("trigger_yueyang", 40, 7, "ch07/yueyang.lua", "enter", True, w=2,
                guard_flag="ch07.zhongwu", set_flag="ch07.yueyang"),

        facility("facility_dazuo_shuguan", 31, 25, "meditate"),
        facility("facility_dazuo_shudong", 8, 6, "meditate"),
        facility("facility_cunji", 20, 33, "save"),
    ] + explore_zhongxin("ch07_jindi_zhongxin")
    return build_map("ch07_jindi_zhongxin", W, H, "terrain_jindi_zhongxin",
                     map_props("ch07_jindi_zhongxin", "bgm_wild"), g, objects)


# ---------------------------------------------------------------------------
# 地图 8：环形山 48x40 —— 节点 26–29、31 落点、32a；支线 Z2
#
# 南缘进来是上山小路（落点、存档点、遭遇区）。西边一大块山体里是紫猴花洞：洞口朝东，横洞道西行，
# 转北一条竖洞道（中段 26b），拐角（26a），石厅口（26c）。洞外岔路石（27）。
# 北边山顶：小石殿围在一圈山石里，只留一道豁口（28），豁口外一个存档点；西北山顶石洞（打坐点）；
# 北坡寒潭（Z2b）；东坡石屋（Z2a）。东南一个小盆地，青石殿（29）；殿旁地面破洞（31 落点、32a）。
# ---------------------------------------------------------------------------
def make_huanxingshan():
    W, H = 48, 40
    g = Grid(W, H)
    rnd = random.Random(708)
    g.fill("ground", 1)
    border(g)

    rock(g, 1, 10, 16, 34)                   # 西边山体
    road(g, 8, 30, 16, 31)                   # 紫猴花洞：横洞道（洞口朝东）
    road(g, 8, 17, 9, 29)                    # 竖洞道
    floor(g, 3, 11, 12, 16)                  # 石厅
    thing(g, 3, 11, 6, 11)                   # 厅里那面紫色石壁
    rock(g, 18, 1, 29, 1)                    # 小石殿四周的山石（北沿贴着图边，只留南边一道豁口）
    rock(g, 18, 2, 18, 10)
    rock(g, 29, 2, 29, 10)
    rock(g, 18, 11, 22, 11)
    rock(g, 25, 11, 29, 11)
    rock(g, 21, 3, 26, 5)                    # 小石殿
    road(g, 23, 11, 24, 16)                  # 豁口外那条上山路
    rock(g, 10, 2, 15, 7)                    # 山顶石洞
    floor(g, 12, 4, 13, 7)
    water(g, 31, 3, 36, 5)                   # 北坡寒潭
    rock(g, 40, 18, 44, 21)                  # 东坡石屋
    rock(g, 33, 26, 41, 27)                  # 小盆地的石沿
    rock(g, 33, 28, 33, 31)
    rock(g, 42, 26, 42, 31)
    rock(g, 35, 28, 40, 31)                  # 青石殿
    rock(g, 37, 34, 39, 34)                  # 地面破洞一圈
    rock(g, 37, 36, 39, 36)
    rock(g, 37, 35, 37, 35)
    thing(g, 19, 28, 19, 28)                 # 岔路石
    for tx, ty in ((21, 20), (27, 23), (31, 18), (36, 12), (44, 10), (46, 24), (20, 34), (29, 35),
                   (44, 35), (26, 29), (22, 14), (33, 14)):
        tree(g, tx, ty)
    edge_fronts(g, rnd, 0.3)
    scatter(g, rnd, 80, 17, 12, 46, 38, grass=0.6)

    objects = [
        spawn("spawn_from_zhongxin", 24, 38, "up", default=True),

        # ---- 26a / 26b / 26c：紫猴花洞 ----
        trigger("trigger_kuitan", 8, 18, "ch07/kuitan.lua", "enter", True, w=2,
                guard_flag="ch07.yueyang", set_flag="ch07.kuitan"),
        trigger("trigger_charen", 8, 24, "ch07/charen.lua", "interact", True, w=2,
                guard_flag="ch07.kuitan", set_flag="ch07.mairen"),
        trigger("trigger_youdi", 8, 16, "ch07/youdi.lua", "enter", True, w=2,
                guard_flag="ch07.mairen", set_flag="ch07.zihouhua"),
        # ---- 27：岔路石 ----
        trigger("trigger_sichu", 19, 28, "ch07/sichu.lua", "interact", True,
                guard_flag="ch07.zihouhua", set_flag="ch07.sichu"),
        # ---- 28：小石殿前的豁口 ----
        trigger("trigger_xiaoshidian", 23, 11, "ch07/xiaoshidian.lua", "enter", True, w=2,
                guard_flag="ch07.sichu", set_flag="ch07.wuyou"),
        # ---- 29：青石殿的门 ----
        trigger("trigger_qingshidian", 37, 31, "ch07/qingshidian.lua", "interact", True,
                guard_flag="ch07.wuyou", set_flag="ch07.didao"),
        # ---- 32a：破洞外一格 ----
        trigger("trigger_xiashan", 39, 35, "ch07/xiashan.lua", "enter", True,
                guard_flag="ch07.nangong", set_flag="ch07.xiashan"),
        # ---- 支线 Z2：玉简上的两处 ----
        trigger("trigger_yujian_dongpo", 42, 21, "ch07/yujian_dongpo.lua", "interact", True,
                guard_flag="ch07.yujian_qiu", set_flag="ch07.yujian_a"),
        trigger("trigger_yujian_hantan", 33, 6, "ch07/yujian_hantan.lua", "interact", True,
                guard_flag="ch07.yujian_qiu", set_flag="ch07.yujian_b"),

        facility("facility_dazuo_shidong", 12, 4, "meditate"),
        facility("facility_cunji", 26, 37, "save"),
        facility("facility_cunji_dian", 25, 14, "save"),
        # 伙伴改版样板 5.2：原先这里是一块暗雷遭遇区（encounter_shanlu，表 encounter_ch07_huanxingshan），
        # 撤掉，换成三只明雷，编成沿用那张表的三份（be07_tuishan_shou / tiebi_yuan / huoyan_shu）。
    ] + explore_huanxingshan("ch07_huanxingshan")
    return build_map("ch07_huanxingshan", W, H, "terrain_huanxingshan",
                     map_props("ch07_huanxingshan", "bgm_mountain_path"), g, objects)


# ---------------------------------------------------------------------------
# 地图 9：地下沼泽 36x28 —— 节点 30、31；伙伴改版样板 4.1–4.3
#
# 西北石阶口（落点、存档点），下了石阶夹在两堆黑土之间（黑土堆后 30）；当中一大片沼泽（水面），
# 沼泽当中一座白玉小亭（装饰）；西岸蛟尸旁（31，样板里改成「困在地下」那一步）；东岸边上长着一片奇花灵草（装饰）。
# 样板 5.1：加两个出入口接沼泽深处——东壁一道（往右走出去）、南壁一道（往下走出去），都要 ch07.kundi 才开
# （困在地下、两人商定去找火灵石之后）。探索层 5.2：一个宝箱、一只明雷。
# ---------------------------------------------------------------------------
def zhaoze_terrain():
    W, H = 36, 28
    g = Grid(W, H)
    rnd = random.Random(709)
    g.fill("ground", 1)
    solid_all(g)

    road(g, 2, 1, 4, 3)                      # 石阶
    floor(g, 2, 4, 33, 26)                   # 洞底
    rock(g, 5, 4, 7, 6)                      # 黑土堆
    rock(g, 1, 7, 2, 8)
    water(g, 10, 8, 27, 21)                  # 沼泽
    g.rect("ground", 17, 13, 19, 15, 1)      # 白玉亭所在的小岛
    thing(g, 17, 13, 19, 15)
    for x0, y0 in ((30, 5), (32, 12), (29, 22), (5, 22), (14, 24)):
        rock(g, x0, y0, x0 + 1, y0 + 1)
    g.rect("overlay", 29, 8, 32, 20, 6)      # 东岸的奇花灵草
    edge_fronts(g, rnd, 0.4)
    # 样板 5.1：两个出口（在 edge_fronts 之后挖，前面那一步的随机数一个不变）
    floor(g, 34, 24, 35, 25)                 # 东壁的岩缝
    floor(g, 16, 27, 17, 27)                 # 南壁的岩缝
    return g, W, H


ZHAOZE_DONG = (34, 24)       # 从沼泽深处回来、落在东壁岩缝里（面朝左）
ZHAOZE_NAN = (16, 26)        # 从沼泽深处回来、落在南壁岩缝口（面朝上）


def zhaoze_links(target, require=None, deny=None):
    return [
        spawn("spawn_from_shenchu_dong", ZHAOZE_DONG[0], ZHAOZE_DONG[1], "left"),
        spawn("spawn_from_shenchu_nan", ZHAOZE_NAN[0], ZHAOZE_NAN[1], "up"),
        portal("portal_to_shenchu_dong", 35, 24, target, "spawn_from_zhaoze_dong", h=2,
               require_flag=require, deny_text_key=deny),
        portal("portal_to_shenchu_nan", 16, 27, target, "spawn_from_zhaoze_nan", w=2,
               require_flag=require, deny_text_key=deny),
    ]


def make_dixia_zhaoze():
    g, W, H = zhaoze_terrain()
    mid = "ch07_dixia_zhaoze"
    objects = [
        spawn("spawn_shijie", ZHAOZE_SHIJIE[0], ZHAOZE_SHIJIE[1], "down", default=True),

        # ---- 30：黑土堆后 ----
        trigger("trigger_guanzhan", 3, 7, "ch07/guanzhan.lua", "enter", True, w=2,
                guard_flag="ch07.didao", set_flag="ch07.mojiao"),
        # ---- 31 / 样板 4.2 末：蛟尸旁（毒囊、醒来、金箱灵药）----
        trigger("trigger_jiaoshi", 9, 15, "ch07/jiaoshi.lua", "interact", True,
                guard_flag="ch07.mojiao", set_flag="ch07.xinglai"),
        # ---- 样板 4.3「困在地下」：石阶口最上面那一格的岩壁（人站在 (3,1) 面朝上），回头去看封死的通道 ----
        trigger("trigger_fengkou", 3, 0, "ch07/fengkou.lua", "interact", True,
                guard_flag="ch07.xinglai", set_flag="ch07.kundi"),

        facility("facility_cunji", 2, 4, "save"),
    ] + zhaoze_links("ch07_zhaoze_shenchu", "ch07.kundi", "ch07.block.zhaoze_shenchu") + [
        # ---- 样板 5.2 探索层：东岸一个宝箱、一只明雷（寒冰蟾，地下是改编）----
        # 宝箱内容：第 7 章试玩返修（docs/ch07-playtest-loot.md 第 4 节，用户 2026-11-20「找东西没啥回报感」）。
        chest("chest_dongan", 32, 4, mid, "chest_dongan",
              [("pill_huiqi_dan", 1), ("material_hanyan_cao", 2), ("material_lingshi", 15)]),
        foe("foe_chan", 31, 17, mid, "foe_chan", "be07_zhaoze_chan", "hanbing_chan", "left", patrol=PATROL_ZHAOZE_CHAN),
    ]
    return build_map(mid, W, H, "terrain_dixia_zhaoze",
                     map_props(mid, "bgm_cave", outdoor=False), g, objects)


# ---------------------------------------------------------------------------
# 样板 5.2：禁地原有三张图的探索层（宝箱、采集点、明雷；主线这几张图上不放机关）
# ---------------------------------------------------------------------------
# 宝箱内容：第 7 章试玩返修（docs/ch07-playtest-loot.md 第 4 节，用户 2026-11-20「找东西没啥回报感」）——普通的箱子
# 至少一样不是金疮药；藏得深的（潭背、东坡角上、残禁后）、精英守着的（鳄湾）放符箓、中阶灵石与成堆的灵石。
def explore_waiwei(mid):
    return [
        chest("chest_yaxia", 12, 6, mid, "chest_yaxia",       # 崖下遗落的储物袋
              [("talisman_huoqiu_fu", 1), ("material_lingshi", 15), ("pill_jinchuang_yao", 1)]),
        chest("chest_tanbei", 46, 1, mid, "chest_tanbei",     # 潭背的角上，藏得深
              [("pill_huiqi_dan", 1), ("talisman_tulao_fu", 1), ("material_lingshi", 25)]),
        gather("gather_hanyan", 36, 6, mid, "gather_hanyan", "material_hanyan_cao", 30),    # 寒烟草，乌龙潭边（ch180）
        foe("foe_chan", 42, 10, mid, "foe_chan", "be07_hanbing_chan", "hanbing_chan", "down", patrol=PATROL_WAIWEI_CHAN),
        foe("foe_e", 3, 18, mid, "foe_e", "be07_cai_e", "cai_e", "right", patrol=PATROL_WAIWEI_XI),
    ]


def explore_zhongxin(mid):
    return [
        chest("chest_linbian", 1, 33, mid, "chest_linbian", [("pill_yangjing_dan", 1), ("material_lingshi", 10)]),
        chest("chest_huayuan", 2, 1, mid, "chest_huayuan",
              [("material_huolong_cao", 2), ("pill_huiqi_dan", 1), ("material_lingshi", 15)]),
        gather("gather_huolong", 45, 24, mid, "gather_huolong", "material_huolong_cao", 30),   # 火龙草（ch191）
        foe("foe_she_lin", 8, 26, mid, "foe_she_lin", "be07_fei_she", "fei_she", "right", patrol=PATROL_ZHONGXIN_LIN),
        foe("foe_she_yuan", 30, 4, mid, "foe_she_yuan", "be07_fei_she", "fei_she", "down", patrol=PATROL_ZHONGXIN_YUAN),
    ]


def explore_huanxingshan(mid):
    return [
        chest("chest_shanjiao", 1, 37, mid, "chest_shanjiao", [("pill_yangjing_dan", 1), ("talisman_huoqiu_fu", 2)]),
        chest("chest_beipo", 46, 1, mid, "chest_beipo",
              [("material_huolong_cao", 1), ("material_lingshi", 20), ("pill_jinchuang_yao", 1)]),
        chest("chest_dongpo", 46, 17, mid, "chest_dongpo",    # 东坡尽头的角上
              [("pill_huiqi_dan", 1), ("material_lingshi_zhong", 1)]),
        gather("gather_huolong", 3, 12, mid, "gather_huolong", "material_huolong_cao", 30),
        foe("foe_tuishan", 20, 20, mid, "foe_tuishan", "be07_tuishan_shou", "tuishan_shou", "down",
            patrol=PATROL_HXS_ZHONG),
        foe("foe_tiebi", 44, 24, mid, "foe_tiebi", "be07_tiebi_yuan", "tiebi_yuan", "left", patrol=PATROL_HXS_DONG),
        foe("foe_huoyan", 28, 33, mid, "foe_huoyan", "be07_huoyan_shu", "huoyan_shu", "left", patrol=PATROL_HXS_NAN),
    ]


# ---------------------------------------------------------------------------
# 地图 10：沼泽深处 44x32 —— 伙伴改版样板 4.3、5.1
#
# 浅水、枯木、发光的菌。西口、北口接地下沼泽，东口一条岩缝通地底石窟。
# 主线：两处火灵石矿脉（采集点）采一处，到东口岩缝里把火灵石交给她（trigger_huolingshi），岩缝那头的门才开。
# 北边一处矿脉在主路边上；东南一条岔道里精英碧水鳄守着一个宝箱和另一处矿脉。
# 西南、东北两条岔道上各一处插话（只在她在队时演），三只明雷，一处打坐点。
# ---------------------------------------------------------------------------
SHENCHU_XI = (1, 5)          # 从地下沼泽东口来，落在西口（面朝右）
SHENCHU_BEI = (10, 1)        # 从地下沼泽南口来，落在北口（面朝下）
SHENCHU_DONG = (42, 22)      # 从地底石窟回来，落在东口岩缝里（面朝左）


def shenchu_terrain():
    W, H = 44, 32
    g = Grid(W, H)
    rnd = random.Random(7010)
    g.fill("ground", 1)
    solid_all(g)

    floor(g, 1, 3, 39, 28)                   # 洞底
    floor(g, 0, 5, 0, 6)                     # 西口
    floor(g, 10, 0, 11, 2)                   # 北口
    floor(g, 40, 22, 43, 23)                 # 东口岩缝
    water(g, 13, 9, 20, 15)                  # 浅水（挡路）
    water(g, 24, 15, 31, 20)
    water(g, 4, 18, 8, 21)
    rock(g, 32, 10, 34, 10)                  # 东北岔道：一道石棱只留 (35,10) 一个口
    rock(g, 36, 10, 39, 10)
    rock(g, 31, 3, 31, 9)
    rock(g, 9, 23, 9, 25)                    # 西南岔道：一道石棱只留 (9,26) 一个口
    rock(g, 9, 27, 9, 28)
    rock(g, 1, 23, 8, 23)
    rock(g, 29, 23, 29, 25)                  # 东南岔道：碧水鳄的水湾，口在 (29,26)
    rock(g, 29, 27, 29, 28)
    rock(g, 30, 23, 39, 23)
    for x0, y0 in ((6, 9), (24, 6), (36, 18), (12, 27), (22, 22), (18, 4)):
        thing(g, x0, y0)                     # 枯木
    water(g, 35, 26, 36, 27)                 # 鳄鱼背后那一小汪水
    g.rect("overlay", 32, 4, 39, 9, 6)       # 东北岔道里一片发光的菌
    g.rect("overlay", 2, 24, 8, 28, 7)
    edge_fronts(g, rnd, 0.4)
    scatter(g, rnd, 60, 1, 3, 39, 28, grass=0.5)
    return g, W, H


def shenchu_links():
    return [
        spawn("spawn_from_zhaoze_dong", SHENCHU_XI[0], SHENCHU_XI[1], "right", default=True),
        spawn("spawn_from_zhaoze_nan", SHENCHU_BEI[0], SHENCHU_BEI[1], "down"),
        spawn("spawn_from_shiku", SHENCHU_DONG[0], SHENCHU_DONG[1], "left"),
    ]


def make_zhaoze_shenchu():
    g, W, H = shenchu_terrain()
    mid = "ch07_zhaoze_shenchu"
    objects = shenchu_links() + [
        portal("portal_to_zhaoze_dong", 0, 5, "ch07_dixia_zhaoze", "spawn_from_shenchu_dong", h=2),
        portal("portal_to_zhaoze_nan", 10, 0, "ch07_dixia_zhaoze", "spawn_from_shenchu_nan", w=2),
        portal("portal_to_shiku", 43, 22, "ch07_dixia_shiku", "spawn_from_shenchu", h=2,
               require_flag="ch07.huolingshi", deny_text_key="ch07.block.dixia_shiku"),

        # ---- 样板 4.3：把火灵石交给她（东口岩缝里，过门之前）----
        trigger("trigger_huolingshi", 40, 22, "ch07/huolingshi.lua", "enter", True, h=2,
                guard_flag="ch07.kundi", set_flag="ch07.huolingshi"),
        # ---- 插话（4.6：都摆在岔路上）----
        trigger("trigger_chahua_jun", 35, 9, "ch07/chahua_jun.lua", "enter", True,
                set_flag="ch07.chahua_jun", require_party=NANGONG),
        trigger("trigger_chahua_kumu", 8, 26, "ch07/chahua_kumu.lua", "enter", True,
                set_flag="ch07.chahua_kumu", require_party=NANGONG),

        gather("gather_huolingshi_bei", 22, 3, mid, "gather_huolingshi_bei", "material_huolingshi", 30),
        gather("gather_huolingshi_wan", 39, 24, mid, "gather_huolingshi_wan", "material_huolingshi", 30),
        # 鳄湾的箱子碧水鳄守着（精英）：第 7 章试玩返修起放成堆的灵石、中阶灵石与符（docs/ch07-playtest-loot.md 第 4 节）。
        chest("chest_ewan", 38, 27, mid, "chest_ewan",
              [("material_lingshi", 50), ("material_lingshi_zhong", 2), ("talisman_tulao_fu", 1),
               ("material_huolingshi", 2)]),
        foe("foe_chan", 12, 21, mid, "foe_chan", "be07_zhaoze_chan", "hanbing_chan", "up", patrol=PATROL_SHENCHU_CHAN),
        foe("foe_shu", 25, 4, mid, "foe_shu", "be07_shenchu_shu", "huoyan_shu", "left", patrol=PATROL_SHENCHU_SHU),
        foe("foe_she", 36, 14, mid, "foe_she", "be07_shenchu_she", "fei_she", "left", patrol=PATROL_SHENCHU_SHE),
        foe("foe_bishui_e", 33, 26, mid, "foe_bishui_e", "be07_bishui_e", "bishui_e", "left", elite=True,
            patrol=PATROL_SHENCHU_E),

        facility("facility_dazuo", 20, 26, "meditate"),
    ]
    return build_map(mid, W, H, "terrain_zhaoze_shenchu",
                     map_props(mid, "bgm_cave", outdoor=False), g, objects)


# ---------------------------------------------------------------------------
# 地图 11：地底石窟 36x28 —— 伙伴改版样板 4.3–4.5、5.1
#
# 残破的石室。西口接沼泽深处；入口大厅往东一条甬道进中央大厅；大厅北边一条甬道一直走到岩层最薄处（破顶，4.4）。
# 西北一间残破石室（插话、一个宝箱），东边一道残禁（她的场地技开，后面一间小室里一个宝箱），
# 东北一条死胡同（插话），大厅里一处打坐点（破顶打输从这里重来）。
# ---------------------------------------------------------------------------
SHIKU_XI = (1, 22)           # 从沼泽深处来，落在西口（面朝右）


def shiku_terrain():
    W, H = 36, 28
    g = Grid(W, H)
    rnd = random.Random(7011)
    g.fill("ground", 1)
    solid_all(g)

    floor(g, 0, 22, 0, 23)                   # 西口
    floor(g, 1, 18, 10, 25)                  # 入口大厅
    floor(g, 11, 21, 16, 22)                 # 东去的甬道
    floor(g, 17, 14, 27, 25)                 # 中央大厅
    floor(g, 2, 4, 9, 12)                    # 西北残破石室
    floor(g, 5, 13, 6, 17)                   # 石室与入口大厅之间的甬道
    floor(g, 21, 4, 22, 13)                  # 北去的甬道
    floor(g, SHIKU_DING_XIA[0], SHIKU_DING_XIA[1], SHIKU_DING_XIA[0], SHIKU_DING_XIA[1])   # 岩层最薄处正下方
    floor(g, 28, 21, 28, 21)                 # 残禁那一格
    floor(g, 29, 18, 34, 24)                 # 残禁后面的小室
    floor(g, 28, 14, 33, 15)                 # 东北的死胡同
    for x0, y0 in ((3, 5), (8, 11), (19, 16), (25, 23), (2, 19), (33, 23)):
        thing(g, x0, y0)                     # 塌下来的石块、断柱
    g.rect("overlay", 2, 4, 9, 12, 7)
    edge_fronts(g, rnd, 0.4)
    scatter(g, rnd, 40, 1, 4, 34, 25, grass=0.2)
    return g, W, H


def make_dixia_shiku():
    g, W, H = shiku_terrain()
    mid = "ch07_dixia_shiku"
    objects = [
        spawn("spawn_from_shenchu", SHIKU_XI[0], SHIKU_XI[1], "right", default=True),
        portal("portal_to_shenchu", 0, 22, "ch07_zhaoze_shenchu", "spawn_from_shiku", h=2),

        # ---- 样板 4.4：岩层最薄处（实体格，人站在正下方按确认）----
        trigger("trigger_poding", SHIKU_YANCENG[0], SHIKU_YANCENG[1], "ch07/poding.lua", "interact", True,
                guard_flag="ch07.huolingshi", set_flag="ch07.poding"),
        # ---- 样板 4.5：破开之后爬出去、问名（岩层正下方那一格，踏入）----
        trigger("trigger_wenming", SHIKU_DING_XIA[0], SHIKU_DING_XIA[1], "ch07/wenming.lua", "enter", True,
                guard_flag="ch07.poding", set_flag="ch07.nangong"),
        # ---- 插话 ----
        trigger("trigger_chahua_shishi", 5, 13, "ch07/chahua_shishi.lua", "enter", True, w=2,
                set_flag="ch07.chahua_shishi", require_party=NANGONG),
        trigger("trigger_chahua_yanbi", 29, 14, "ch07/chahua_yanbi.lua", "enter", True, h=2,
                set_flag="ch07.chahua_yanbi", require_party=NANGONG),

        # ---- 样板 5.1：一道残禁（她的场地技能开），后面一个宝箱；另一个宝箱在残破石室里 ----
        gate("gate_canjin", 28, 21, mid, "gate_canjin", "break_seal", hint="ch07.gate.canjin.hint"),
        # 宝箱内容：第 7 章试玩返修（docs/ch07-playtest-loot.md 第 4 节）——残禁后那一个藏得最深。
        chest("chest_xiaoshi", 33, 20, mid, "chest_xiaoshi",
              [("material_lingshi", 40), ("material_lingshi_zhong", 2), ("talisman_huoqiu_fu", 2),
               ("pill_yangjing_dan", 1)]),
        chest("chest_shishi", 2, 4, mid, "chest_shishi",
              [("pill_yangjing_dan", 1), ("material_lingshi", 15), ("pill_jinchuang_yao", 1)]),

        facility("facility_dazuo", SHIKU_DAZUO_PANG[0] + 1, SHIKU_DAZUO_PANG[1], "meditate"),
    ]
    return build_map(mid, W, H, "terrain_dixia_shiku",
                     map_props(mid, "bgm_cave", outdoor=False), g, objects)


# ---------------------------------------------------------------------------
# 历练「禁地旧境」的六张图（样板施工图第 7 节、契约 3.12）
#
# 禁地四张原图与两张新图的副本：底图（瓦片、碰撞）与原图逐格相同，对象另是一套——
# 原图上的存档点、打坐点、剧情触发器、闸门一律不带过来；六张图之间用不设闸的门连成一串；
# 每张图都有一处 kind=lilian 的设施当出口（面朝它按确认就出去）；明雷、宝箱带 chance（每趟不同）；
# 两道机关：残禁（南宫婉开）、幻阵（陈师妹开）。历练图上的机关不写 opens（门禁规则 39）。
# foe.battle 写编成 id 的前半（bl07_*），引擎按韩立的境界拼 _t1/_t2/_t3。
# ---------------------------------------------------------------------------
LILIAN_SITE = "jindi"


def lilian_props(map_id, bgm, outdoor=True):
    return [
        ("map_id", map_id),
        ("display_name_key", "lilian.map.%s.name" % map_id[len("lilian_"):]),
        ("region", "jianzhou"),
        ("bgm", bgm),
        ("chapter", CHAPTER),
        ("outdoor", outdoor),
        ("can_leave_edge", False),
    ]


def lilian_copy(original, map_id, bgm, outdoor, objects, openings=()):
    """拿原图的产出换掉图名属性与对象层：底图与原图逐格相同，只多挖开 openings 那几格（图边上的门）。"""
    payload = original
    for x, y in openings:
        _open(payload, x, y, ground=1)
    for i, o in enumerate(objects, 1):
        o["id"] = i
    for layer in payload["layers"]:
        if layer.get("type") == "objectgroup":
            layer["objects"] = objects
    payload["nextobjectid"] = len(objects) + 1
    payload["properties"] = [genmaps.prop(k, v) for k, v in lilian_props(map_id, bgm, outdoor)]
    return payload


def exit_facility(x, y):
    return facility("facility_lilian_exit", x, y, "lilian", site=LILIAN_SITE)


def make_lilian_waiwei():
    mid = "lilian_jindi_waiwei"
    objects = [
        spawn("spawn_rukou", WAIWEI_LUODIAN[0], WAIWEI_LUODIAN[1], "up", default=True),
        spawn("spawn_from_zhongxin", 22, 1, "down"),
        portal("portal_to_zhongxin", 22, 0, "lilian_jindi_zhongxin", "spawn_from_waiwei", w=2),
        exit_facility(4, 30),
        # ---- 入口前两格：两人头一回开口（内容审查 2026-11-18 第 6 条：神游这件事要在玩家看得见的文字里落地）----
        trigger("trigger_lilian_chen", 5, 30, "ch07/lilian_chen.lua", "enter", True,
                set_flag="ch07.lilian_chen", require_party=CHEN),
        trigger("trigger_lilian_nangong", 5, 29, "ch07/lilian_nangong.lua", "enter", True,
                set_flag="ch07.lilian_nangong", require_party=NANGONG),
        # ---- 幻阵：陈师妹开（样板第 7 节「带对的人才开得了」）。罩住乌龙潭边矮树丛的缺口，潭边那一片只从这里进 ----
        gate("gate_huanzhen", 38, 12, mid, "gate_huanzhen", "see_through_illusion", w=2,
             hint="lilian.gate.huanzhen.hint"),
        # 历练的宝箱每趟重抽，加码比正史收着些（docs/ch07-playtest-loot.md 第 4 节）：灵石略多，深处的多一张火球符。
        chest("chest_yaxia", 12, 6, mid, "chest_yaxia", [("material_lingshi", 8), ("material_hanyan_cao", 1)],
              chance=60),
        chest("chest_tanbei", 46, 1, mid, "chest_tanbei", [("pill_huiqi_dan", 1), ("talisman_huoqiu_fu", 1)],
              chance=50),
        gather("gather_hanyan", 36, 6, mid, "gather_hanyan", "material_hanyan_cao", 1),
        foe("foe_chan", 42, 10, mid, "foe_chan", "bl07_hanbing_chan", "hanbing_chan", "down", chance=70,
            patrol=PATROL_WAIWEI_CHAN),
        foe("foe_she", 3, 18, mid, "foe_she", "bl07_fei_she", "fei_she", "right", chance=70, patrol=PATROL_WAIWEI_XI),
        foe("foe_shu", 30, 30, mid, "foe_shu", "bl07_huoyan_shu", "huoyan_shu", "left", chance=50,
            patrol=[(34, 30)]),
    ]
    return lilian_copy(make_jindi_waiwei(), mid, "bgm_wild", True, objects)


def make_lilian_zhongxin():
    mid = "lilian_jindi_zhongxin"
    objects = [
        spawn("spawn_from_waiwei", 22, 34, "up", default=True),
        spawn("spawn_from_huanxingshan", 40, 1, "down"),
        portal("portal_to_waiwei", 22, 35, "lilian_jindi_waiwei", "spawn_from_zhongxin", w=2),
        portal("portal_to_huanxingshan", 40, 0, "lilian_jindi_huanxingshan", "spawn_from_zhongxin", w=2),
        exit_facility(20, 33),
        chest("chest_linbian", 1, 33, mid, "chest_linbian", [("pill_jinchuang_yao", 1), ("material_lingshi", 6)],
              chance=60),
        chest("chest_huayuan", 2, 1, mid, "chest_huayuan", [("material_lingshi", 10), ("material_huolong_cao", 1)]),
        gather("gather_huolong", 45, 24, mid, "gather_huolong", "material_huolong_cao", 1),
        foe("foe_she_lin", 8, 26, mid, "foe_she_lin", "bl07_fei_she", "fei_she", "right", chance=70,
            patrol=PATROL_ZHONGXIN_LIN),
        foe("foe_she_yuan", 30, 4, mid, "foe_she_yuan", "bl07_fei_she", "fei_she", "down", chance=70,
            patrol=PATROL_ZHONGXIN_YUAN),
    ]
    return lilian_copy(make_jindi_zhongxin(), mid, "bgm_wild", True, objects, openings=((22, 35), (23, 35)))


def make_lilian_huanxingshan():
    mid = "lilian_jindi_huanxingshan"
    objects = [
        spawn("spawn_from_zhongxin", 24, 38, "up", default=True),
        spawn("spawn_from_zhaoze", 39, 35, "up"),
        portal("portal_to_zhongxin", 24, 39, "lilian_jindi_zhongxin", "spawn_from_huanxingshan"),
        portal("portal_to_zhaoze", 38, 35, "lilian_jindi_zhaoze", "spawn_from_huanxingshan"),
        exit_facility(26, 37),
        chest("chest_shanjiao", 1, 37, mid, "chest_shanjiao", [("pill_yangjing_dan", 1), ("material_lingshi", 6)],
              chance=50),
        chest("chest_beipo", 46, 1, mid, "chest_beipo", [("material_lingshi", 8), ("material_huolong_cao", 1)],
              chance=60),
        gather("gather_huolong", 3, 12, mid, "gather_huolong", "material_huolong_cao", 1),
        foe("foe_shu", 20, 20, mid, "foe_shu", "bl07_huoyan_shu", "huoyan_shu", "down", chance=70,
            patrol=PATROL_HXS_ZHONG),
        foe("foe_chan", 44, 24, mid, "foe_chan", "bl07_hanbing_chan", "hanbing_chan", "left", chance=70,
            patrol=PATROL_HXS_DONG),
        foe("foe_she", 28, 33, mid, "foe_she", "bl07_fei_she", "fei_she", "left", chance=60, patrol=PATROL_HXS_NAN),
    ]
    return lilian_copy(make_huanxingshan(), mid, "bgm_mountain_path", True, objects, openings=((24, 39),))


def make_lilian_zhaoze():
    g, W, H = zhaoze_terrain()
    mid = "lilian_jindi_zhaoze"
    base = build_map(mid, W, H, "terrain_dixia_zhaoze", map_props("ch07_dixia_zhaoze", "bgm_cave", False), g, [])
    objects = [
        spawn("spawn_from_huanxingshan", ZHAOZE_SHIJIE[0], ZHAOZE_SHIJIE[1], "left", default=True),
        portal("portal_to_huanxingshan", 2, 1, "lilian_jindi_huanxingshan", "spawn_from_zhaoze", w=3),
        exit_facility(2, 4),
    ] + zhaoze_links("lilian_jindi_shenchu") + [
        chest("chest_dongan", 32, 4, mid, "chest_dongan", [("material_lingshi", 8), ("pill_huiqi_dan", 1)],
              chance=60),
        foe("foe_chan", 31, 17, mid, "foe_chan", "bl07_hanbing_chan", "hanbing_chan", "left", chance=70,
            patrol=PATROL_ZHAOZE_CHAN),
    ]
    return lilian_copy(base, mid, "bgm_cave", False, objects)


def make_lilian_shenchu():
    g, W, H = shenchu_terrain()
    mid = "lilian_jindi_shenchu"
    base = build_map(mid, W, H, "terrain_zhaoze_shenchu", map_props("ch07_zhaoze_shenchu", "bgm_cave", False), g, [])
    objects = shenchu_links() + [
        portal("portal_to_zhaoze_dong", 0, 5, "lilian_jindi_zhaoze", "spawn_from_shenchu_dong", h=2),
        portal("portal_to_zhaoze_nan", 10, 0, "lilian_jindi_zhaoze", "spawn_from_shenchu_nan", w=2),
        portal("portal_to_shiku", 43, 22, "lilian_jindi_shiku", "spawn_from_shenchu", h=2),
        exit_facility(20, 26),
        gather("gather_huolingshi_bei", 22, 3, mid, "gather_huolingshi_bei", "material_huolingshi", 1),
        chest("chest_ewan", 38, 27, mid, "chest_ewan",
              [("material_lingshi", 25), ("material_huolingshi", 1), ("talisman_huoqiu_fu", 1)]),
        foe("foe_shu", 25, 4, mid, "foe_shu", "bl07_huoyan_shu", "huoyan_shu", "left", chance=70,
            patrol=PATROL_SHENCHU_SHU),
        foe("foe_she", 36, 14, mid, "foe_she", "bl07_fei_she", "fei_she", "left", chance=70, patrol=PATROL_SHENCHU_SHE),
        foe("foe_bishui_e", 33, 26, mid, "foe_bishui_e", "bl07_bishui_e", "bishui_e", "left", elite=True,
            chance=60, patrol=PATROL_SHENCHU_E),
    ]
    return lilian_copy(base, mid, "bgm_cave", False, objects)


def make_lilian_shiku():
    g, W, H = shiku_terrain()
    mid = "lilian_jindi_shiku"
    base = build_map(mid, W, H, "terrain_dixia_shiku", map_props("ch07_dixia_shiku", "bgm_cave", False), g, [])
    objects = [
        spawn("spawn_from_shenchu", SHIKU_XI[0], SHIKU_XI[1], "right", default=True),
        portal("portal_to_shenchu", 0, 22, "lilian_jindi_shenchu", "spawn_from_shiku", h=2),
        exit_facility(SHIKU_DAZUO_PANG[0] + 1, SHIKU_DAZUO_PANG[1]),
        # ---- 残禁：南宫婉开 ----
        gate("gate_canjin", 28, 21, mid, "gate_canjin", "break_seal", hint="ch07.gate.canjin.hint"),
        chest("chest_xiaoshi", 33, 20, mid, "chest_xiaoshi",
              [("material_lingshi", 30), ("pill_yangjing_dan", 1), ("talisman_huoqiu_fu", 1)]),
        chest("chest_shishi", 2, 4, mid, "chest_shishi", [("pill_jinchuang_yao", 1), ("material_lingshi", 8)],
              chance=60),
        # 石窟大厅这一只每趟都在（不写 chance）：历练至少有一仗是定了的，用例也不靠抽签。
        foe("foe_chan", 24, 20, mid, "foe_chan", "bl07_hanbing_chan", "hanbing_chan", "up", patrol=[(20, 20)]),
    ]
    return lilian_copy(base, mid, "bgm_cave", False, objects)


# ---------------------------------------------------------------------------
# 修补：黄枫谷、百药园（第五节）
# ---------------------------------------------------------------------------
def _layers(payload):
    return {l["name"]: l for l in payload["layers"]}


def _put(payload, layer, x, y, value):
    W = payload["width"]
    _layers(payload)[layer]["data"][y * W + x] = value


def _open(payload, x, y, ground=2):
    for layer, value in (("building", 0), ("collision", 0), ("front", 0), ("overlay", 0), ("ground", ground)):
        _put(payload, layer, x, y, value)


def _furnish(payload, x, y):
    _put(payload, "building", x, y, 8)
    _put(payload, "collision", x, y, 1)
    _put(payload, "overlay", x, y, 0)


def _append(payload, new_objects):
    objs = _layers(payload)["objects"]["objects"]
    names = {o["name"] for o in objs}
    for o in new_objects:
        if o["name"] not in names:
            objs.append(o)
    for i, o in enumerate(objs, 1):
        o["id"] = i
    payload["nextobjectid"] = len(objs) + 1
    return payload


def patch_huangfenggu(payload):
    """给黄枫谷补上第 7 章：南缘通坊市的门、北缘通岳麓殿的门、传功阁、百机堂、报名的案、东口的山头、大殿门前。

    幂等。payload 是 genmaps_ch06.make_huangfenggu() 的产出（或读盘的那一份）。追加顺序固定，`--check` 才不报假漂移。
    第 6 章已有的对象一个不动（施工图第 17 节第 25 条）。
    """
    for x in (23, 24):
        _open(payload, x, 35)                # 南缘：往坊市
    for x in (32, 33):
        _open(payload, x, 0)                 # 北缘：往岳麓殿（巫钧山）
    _furnish(payload, 25, 12)                # 大殿前王师叔收报名的那张案

    shantou_v, shantou_h = "ch07.chouhan", "ch07.murong"
    return _append(payload, [
        spawn("spawn_from_fangshi", 23, 34, "up"),
        spawn("spawn_from_yuelu", 32, 1, "down"),
        portal("portal_to_fangshi", 23, 35, "ch07_fangshi", "spawn_from_huangfenggu", w=2,
               require_flag="ch07.chuling", deny_text_key="ch07.block.fangshi"),
        portal("portal_to_yuelu_dian", 32, 0, "ch07_yuelu_dian", "spawn_from_huangfenggu", w=2,
               require_flag="ch07.danbao", deny_text_key="ch07.block.yuelu_dian"),

        # ---- 6：东口咽喉（往百药园必经；山头一群人站在路两边）----
        trigger("trigger_murong", 46, 26, "ch07/murong.lua", "enter", True, h=2,
                guard_flag="ch07.chouhan", set_flag="ch07.murong"),
        # ---- 8 / 15：吴风跟前一格（他面前那两格是第 6 章的 trigger_wufeng，这一格挨在他南边；同一格只挂一个对象）----
        trigger("trigger_chuangong", 40, 10, "ch07/chuangong.lua", "interact", False,
                guard_flag="ch07.dufang"),
        # ---- 10：百机堂柜台，于执事面前 ----
        trigger("trigger_chuling", 7, 12, "ch07/chuling.lua", "interact", True,
                guard_flag="ch07.qiannian", set_flag="ch07.chuling"),
        # ---- 16：大殿前王师叔那张案 ----
        trigger("trigger_baoming", 25, 12, "ch07/baoming.lua", "interact", True,
                guard_flag="ch07.fudan_fa", set_flag="ch07.baoming"),
        # ---- 18：大殿门前一格 ----
        trigger("trigger_jihe", 23, 9, "ch07/jihe.lua", "enter", True, w=2,
                guard_flag="ch07.ma_songyao", set_flag="ch07.jihe"),

        npc("npc_murong_xiong", 41, 24, "murong_xiong", "down", visible_flag=shantou_v, hidden_flag=shantou_h),
        npc("npc_murong_di", 42, 24, "murong_di", "down", visible_flag=shantou_v, hidden_flag=shantou_h),
        npc("npc_lu_shixiong", 44, 24, "lu_shixiong", "left", visible_flag=shantou_v, hidden_flag=shantou_h),
        npc("npc_chen_shimei", 45, 24, "chen_shimei", "left", visible_flag=shantou_v, hidden_flag=shantou_h),
        npc("npc_lanyi_nvzi", 44, 29, "lanyi_nvzi", "up", visible_flag=shantou_v, hidden_flag=shantou_h),
        npc("npc_cuai_qingnian", 42, 29, "cuai_qingnian", "up", visible_flag=shantou_v, hidden_flag=shantou_h),
        npc("npc_huangfeng_weiguan", 39, 24, "huangfeng_weiguan", "right", visible_flag=shantou_v,
            hidden_flag=shantou_h),
        npc("npc_li_huayuan", 25, 10, "li_huayuan", "left", visible_flag="ch07.ma_songyao", hidden_flag="ch07.jihe"),
    ])


def patch_baiyaoyuan(payload):
    """给百药园补上第 7 章：居室的桌与书桌、药篓、园角两槽灵田、园门内、茅屋门内。幂等。"""
    _furnish(payload, 22, 3)                 # 居室里放包袱的小桌
    _furnish(payload, 26, 4)                 # 书桌
    _furnish(payload, 26, 5)
    _furnish(payload, 23, 8)                 # 茅屋前的药篓
    for x in (9, 10):
        for y in (19, 20):
            _put(payload, "ground", x, y, 3)  # 园角那一小块翻过的土
            _put(payload, "overlay", x, y, 6)

    return _append(payload, [
        # ---- 1：居室小桌上的包袱 ----
        trigger("trigger_chaibao", 22, 3, "ch07/chaibao.lua", "interact", True,
                guard_flag="ch06.done", set_flag="ch07.chaibao"),
        # ---- 2 / 3 / Z1：药篓 ----
        trigger("trigger_yaolou", 23, 8, "ch07/yaolou.lua", "interact", False,
                guard_flag="ch07.chaibao"),
        # ---- 7：书桌 ----
        trigger("trigger_dufang", 26, 4, "ch07/dufang.lua", "interact", True,
                guard_flag="ch07.murong", set_flag="ch07.dufang"),
        # ---- 9：园角田边 ----
        trigger("trigger_yuanjiao", 10, 20, "ch07/yuanjiao.lua", "interact", True,
                guard_flag="ch07.lianqi", set_flag="ch07.qiannian"),
        # ---- 17：园门内 ----
        trigger("trigger_ma_songyao", 8, 11, "ch07/ma_songyao.lua", "enter", True, h=2,
                guard_flag="ch07.baoming", set_flag="ch07.ma_songyao"),
        # ---- 33：茅屋门内 ----
        trigger("trigger_xieshili", 24, 6, "ch07/xieshili.lua", "enter", True,
                guard_flag="ch07.chujindi", set_flag="ch07.xieshili"),
        # ---- 34：书桌旁 ----
        trigger("trigger_sannian", 26, 5, "ch07/sannian.lua", "interact", True,
                guard_flag="ch07.xieshili", set_flag="ch07.sannian"),

        facility("facility_field_jiao", 9, 19, "field", w=2, ref_id="field_baiyaoyuan_jiao", slots=2,
                 require_flag="ch07.chaibao"),
        # ---- 伙伴改版样板 5.3：历练「禁地旧境」的入口，茅屋里一处专门的设施（现有的打坐点不动）----
        facility("facility_lilian", 25, 3, "lilian", site="jindi", require_flag="ch07.chujindi"),
    ])


def build_all():
    """本章九张图的完整产出，外加黄枫谷、百药园两处就地修补。"""
    import genmaps_ch06  # 延迟导入：它的 build_all 回头调本文件的两个 patch，模块层导入会成环
    return {
        "ch07_yuelu_dian": make_yuelu_dian(),
        "ch07_dihuo": make_dihuo(),
        "ch07_fangshi": make_fangshi(),
        "ch07_shandong": make_shandong(),
        "ch07_jindi_wai": make_jindi_wai(),
        "ch07_jindi_waiwei": make_jindi_waiwei(),
        "ch07_jindi_zhongxin": make_jindi_zhongxin(),
        "ch07_huanxingshan": make_huanxingshan(),
        "ch07_dixia_zhaoze": make_dixia_zhaoze(),
        "ch07_zhaoze_shenchu": make_zhaoze_shenchu(),
        "ch07_dixia_shiku": make_dixia_shiku(),
        "lilian_jindi_waiwei": make_lilian_waiwei(),
        "lilian_jindi_zhongxin": make_lilian_zhongxin(),
        "lilian_jindi_huanxingshan": make_lilian_huanxingshan(),
        "lilian_jindi_zhaoze": make_lilian_zhaoze(),
        "lilian_jindi_shenchu": make_lilian_shenchu(),
        "lilian_jindi_shiku": make_lilian_shiku(),
        "ch06_huangfenggu": patch_huangfenggu(genmaps_ch06.make_huangfenggu()),
        "ch06_baiyaoyuan": patch_baiyaoyuan(genmaps_ch06.make_baiyaoyuan()),
    }


TILESETS = ("terrain_yuelu_dian", "terrain_dihuo", "terrain_fangshi", "terrain_shandong", "terrain_jindi_wai",
            "terrain_jindi_waiwei", "terrain_jindi_zhongxin", "terrain_huanxingshan", "terrain_dixia_zhaoze",
            "terrain_zhaoze_shenchu", "terrain_dixia_shiku")


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
