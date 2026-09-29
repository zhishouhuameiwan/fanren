# -*- coding: utf-8 -*-
"""第 5 章地图生成器：重跑即重建下列六张图与六个 tileset，并给韩家村补一处村东新口。

    python tools/mapgen/genmaps_ch05.py
    python tools/mapgen/genmaps_ch05.py --check   # 只比对，不写盘；有漂移则退出码 1
    python tools/validate.py                      # 自检

    （`genmaps.py --check` 会把本章这六张连同韩家村一并盯上，build.bat 里仍只有那一条。）

产出（全部写进 maps/，不经手 scripts/ 与 data/）：

    maps/ch05_dukou.tmj            40x30  岚州渡口（节点 1：东去路上、上船）
    maps/ch05_xicheng.tmj          48x36  嘉元城西城（码头 · 黑水巷 · 仓房 · 潇湘院）
    maps/ch05_nancheng.tmj         48x36  嘉元城南城（南陵街 · 墨府大门 · 香家酒楼 · 药铺）
    maps/ch05_kezhan.tmj           32x24  汇源客栈（上房 · 后院 · 大堂）
    maps/ch05_mofu.tmj             48x36  墨府（前院 · 马厩院 · 月洞门 · 后园 · 小楼 · 后宅）
    maps/ch05_dubashanzhuang.tmj   40x30  独霸山庄（庄外林子 · 围墙 · 后园赏月亭）
    maps/tilesets/terrain_{dukou,xicheng,nancheng,kezhan,mofu,dubashanzhuang}.tsj

    另就地修补 maps/ch01_hanjiacun.tmj（见第五节）。

施工图：docs/ch05-design.md。挂点的 mode / once / guard_flag / set_flag 一律照它 3.1 那张表，
不在这里另作主张；这份文件只回答「摆在哪一格」。

--------------------------------------------------------------------------
一、连通与闸门（施工图第 4 节）

    ch01_hanjiacun ──(ch04.done)──▶ ch05_dukou ──teleport(1b)──▶ ch05_xicheng
    ch05_xicheng ──(ch05.zhuishao)──▶ ch05_nancheng ◀──▶ ch05_kezhan
    ch05_nancheng ──▶ ch05_xicheng（不设闸）
    ch05_nancheng ──(ch05.dengmen)──▶ ch05_mofu（正门）；ch05_mofu ──▶ ch05_nancheng（不设闸）
    ch05_nancheng ──teleport(6a，夜)──▶ ch05_mofu 后园
    ch05_mofu ──teleport(12b)──▶ ch05_dubashanzhuang ──teleport(12d 胜)──▶ ch05_mofu 前院

    三道闸门各指一张不同的图，deny_text_key 按目标图命名（规则 20）：
    ch05.block.dukou / ch05.block.nancheng / ch05.block.mofu。**不再加第四道。**

    渡口没有通往西城的传送点、山庄没有任何出口：两处都只靠剧情 teleport 进出，
    规则 18 把 teleport() 算作一条边，所以这不是断链。

二、每一处挂战斗的格子前面都有存档点（施工图第 17 节第 12 条）

    ① 狼      渡口：facility_cunji 在出生点与峡口之间
    ②③④ 西城 ：facility_cunji 在潇湘院门外的街上，码头一下来就走得到，黑水巷口在它后头
    ⑤ 夺帮    客栈：大堂里的 facility_cunji
    ⑥⑦⑧ 墨府 ：前院 facility_cunji，在马厩院与月洞门之前
    ⑨⑩ 山庄  ：林子里的 facility_cunji
    潇湘院条件战同上（西城那一个）。

三、踏入型卡在咽喉上，一格只挂一个 trigger

    enter（全章八处，每一处独占自己的格子，且都在一条走不开的路上）
      trigger_dongqu_lu    渡口峡口（出生点到江边唯一的两格口子）
      trigger_matou        西城码头栈桥最里那一格（下船必踩）
      trigger_heishuixiang 黑水巷巷口（码头进城唯一的两格口子）
      trigger_zhuishao     往南城去的巷口（西城闸门之前三格）
      trigger_jianmianli   小楼通后宅那段夹道靠小楼的一格
      trigger_huayuan      同一段夹道靠后宅的一格（与上一格前后相邻、不同格）
      trigger_yange        月洞门（前院与后园之间唯一的口子）
      trigger_zhuwu        马厩院院门（马厩院唯一的出口）

    interact（其余，全部贴在一件东西上：墙、门、窗、桌、床、船）

    小楼一处有 6b、7a、9、11a、12e 五场戏，分在五个格子上：西墙（偷听）、东窗（扔戒指）、
    楼门三格（对质 · 交易 · 换玉）。

四、墨府的夜：月洞门里站着暗哨

    6a 把人从后巷送进后园时，他还没有登门（ch05.dengmen 未置）。这时候他若能一路走到前院、
    出正门回南城，就再也进不来了——正门的闸门要 ch05.dengmen，而 6a 已经烧掉。
    这不是规则 19 查得出的死锁（它只报必然的），却是玩家一定撞得上的那一种。

    所以月洞门里站着一个暗哨（npc_anshao，role mofu_huyuan）：visible_flag=ch05.yeru、
    hidden_flag=ch05.dengmen，只在「翻进来了、还没登门」那一段在场。在场的 NPC 占格
    （WorldScene::tryStep 走 visibleNpcAt），月洞门一格宽，夜里就过不去；
    登门之后他不在了，路也就通了。校验器照引擎口径不把带旗标的 NPC 当墙，规则 13 照样
    从正门的出生点推得到后园里的每一样东西。
    这一处与原著对得上：ch108 他一路绕过的正是府里的暗哨。

五、韩家村那处村东新口：就地修补，不重建

    韩家村的地形归 genmaps.py 所有，本文件照第 2、3、4 章的路子只改需要改的那几格、
    只追加那两个对象，其余原样写回；幂等，补过的图再补一次不变。

    改动一共八格：村东篱笆 (39,20)(39,21) 开口，(36..38,20..21) 铺路。第 4 章那处车道口在
    (39,14)(39,15)，巷墙砌在 (36..38,13) 与 (36..38,16)，两处互不相碰；村东老树（树干 (34,17)）
    与第 4 章章末的 trigger_dongqu (35,17) 一格没动——第 4 章终局就站在那棵树底下，
    往南走三格、往东走四格就是这道新口。

    genmaps.py 的 build_all() 在第 4 章那一次修补之后回头调用本文件的 patch_hanjiacun()，
    顺序要紧：对象表的先后与 maps/ 里的一致，--check 才不报假漂移。

六、瓦片 gid 约定（沿用 genmaps.py 那一套）

    1 主地表  2 道路/木板/石板  3 特殊地表（水面挡路 / 药圃翻土可踩）  4 front 遮挡
    5 building 主体（墙体/屋身/山石）  6 overlay 花草  7 overlay 碎石
    8 building 次体（家具/栏杆/竹棚/床/案）
    水面固定写法：ground=3、collision=1、building=0（「水面不画墙，只挡路」，MapArtTests 钉着）。

不变量：building 有实体瓦片处 collision 必须非 0，一律走 Grid.solid_rect。
"""
import json
import os
import random
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import genmaps  # noqa: E402  MAPS 要运行时取：它会被重定向到副本上
from genmaps import (  # noqa: E402
    Grid,
    build_map,
    check_maps,
    dump_map,
    facility,
    npc,
    obj,
    portal,
    spawn,
    trigger,
    write_tileset,
)

CHAPTER = 5
PREFIX = "ch05_"


def map_props(map_id, bgm, outdoor=True):
    return [
        ("map_id", map_id),
        # 规则 21：display_name_key 由地图 id 推出来。
        ("display_name_key", "ch05.map.%s.name" % map_id[len(PREFIX):]),
        ("region", "lanzhou"),        # 施工图第 4 节：本章六张图全在岚州
        ("bgm", bgm),
        ("chapter", CHAPTER),
        ("outdoor", outdoor),
        # can_leave_edge 一律 false：引擎里没有任何地方消费它（第 4 章生成器同一条理由）。
        ("can_leave_edge", False),
    ]


# ---------------------------------------------------------------------------
# 网格小工具（与 genmaps_ch04 同形；那边的没有导出，这里各留一份）
# ---------------------------------------------------------------------------
def border(g, gid=5):
    """四面围一圈实体。开口由调用方随后挖出来。"""
    g.solid_rect(0, 0, g.w - 1, 0, gid)
    g.solid_rect(0, g.h - 1, g.w - 1, g.h - 1, gid)
    g.solid_rect(0, 0, 0, g.h - 1, gid)
    g.solid_rect(g.w - 1, 0, g.w - 1, g.h - 1, gid)


def road(g, x0, y0, x1, y1, gid=2):
    """铺一条路：清掉实体 + 地表换成道路。"""
    g.open_rect(x0, y0, x1, y1)
    g.rect("ground", x0, y0, x1, y1, gid)


def encounter(name, x, y, w, h, table_id, steps_min, steps_max, require_flag=None):
    """野外遭遇区（map_spec 4.5；docs/interfaces-octo-encounters.md 第 3 节）。与第 3 章生成器同形。"""
    kw = dict(table_id=table_id, steps_min=steps_min, steps_max=steps_max)
    if require_flag:
        kw["require_flag"] = require_flag
    return obj(name, "encounter", x, y, w, h, **kw)


def floor(g, x0, y0, x1, y1, gid=1):
    """清出一片可走的地面（不是路）。"""
    g.open_rect(x0, y0, x1, y1)
    g.rect("ground", x0, y0, x1, y1, gid)


def water(g, x0, y0, x1, y1):
    """水面：ground 3 + 挡路，building 不画（MapArtTests 那条判据的前提）。"""
    g.rect("ground", x0, y0, x1, y1, 3)
    g.rect("building", x0, y0, x1, y1, 0)
    g.rect("collision", x0, y0, x1, y1, 1)
    g.rect("front", x0, y0, x1, y1, 0)
    g.rect("overlay", x0, y0, x1, y1, 0)


def tilled(g, x0, y0, x1, y1):
    """药圃翻土：ground 3，可踩。"""
    g.open_rect(x0, y0, x1, y1)
    g.rect("ground", x0, y0, x1, y1, 3)
    g.rect("overlay", x0, y0, x1, y1, 6)


def eaves_row(g, x0, x1, y):
    """屋檐压在墙外一行（front，不挡路）。"""
    g.rect("front", x0, y, x1, y, 4)


def tree(g, x, y):
    """一棵树：树干占一格，树冠三格见方压在 front 层（不挡路）。"""
    g.rect("front", x - 1, y - 1, x + 1, y + 1, 4)
    g.solid_rect(x, y, x, y, 5)


def scatter(g, rnd, count, x0, y0, x1, y1, grass=0.7):
    """在可通行的主地表上撒花草与碎石，只动 overlay，不影响通行。"""
    for _ in range(count):
        x = rnd.randrange(x0, x1 + 1)
        y = rnd.randrange(y0, y1 + 1)
        if g.walkable(x, y) and g.get("overlay", x, y) == 0 and g.get("ground", x, y) == 1:
            g.set("overlay", x, y, 6 if rnd.random() < grass else 7)


# ---------------------------------------------------------------------------
# 地图 1：岚州渡口 40x30 —— 节点 1
#
# 韩家村东口出来走了两个月，到岚州界上这处渡口。西边一条土路进来，穿过两道土坡夹出的
# 峡口，峡口以东是江边一片空地、船家的草棚和一道伸进江里的栈桥。
#
#   1a trigger_dongqu_lu  峡口两格（enter + once）。出生点到渡口唯一的路，狼扑营、毒发作都是
#                         找上他的，所以踩上去就响。
#   1b trigger_shangchuan 栈桥尽头拴着的那条船（interact + once，guard ch05.kaipian）。
#                         船本身在水面格上，人站在栈桥头面朝东按。
#   存档点在出生点与峡口之间：①号仗之前。
# ---------------------------------------------------------------------------
def make_dukou():
    W, H = 40, 30
    g = Grid(W, H)
    rnd = random.Random(501)
    g.fill("ground", 1)
    border(g)

    # 两道土坡：北坡与南坡，中间夹出一条东西向的谷地（y 11..18）
    g.solid_rect(1, 1, 30, 10, 5)
    g.solid_rect(1, 19, 30, 28, 5)
    # 峡口：x 11..15 这一段谷地收成两格（y 14..15）
    g.solid_rect(11, 11, 15, 13, 5)
    g.solid_rect(11, 16, 15, 18, 5)
    # 峡口以东，坡脚往后退，让出江边的空地
    floor(g, 16, 5, 31, 24)

    # 江：x 32..38 整条是水
    water(g, 32, 1, 38, 28)

    # 土路：西口进来一直通到江边
    road(g, 1, 14, 31, 15)
    road(g, 0, 14, 0, 15)            # 西口（回韩家村）
    # 栈桥：伸进江里四格
    road(g, 32, 14, 35, 15)

    # 船家的草棚与江边几棵树
    g.solid_rect(22, 7, 25, 9, 5)
    eaves_row(g, 22, 25, 6)
    for tx, ty in ((18, 20), (28, 21), (19, 7), (29, 10), (6, 12), (8, 17)):
        tree(g, tx, ty)

    # 坡面压一层 front，谷地撒草
    for y in range(H):
        for x in range(W):
            if not g.walkable(x, y) and g.get("ground", x, y) != 3 and any(
                    g.walkable(x + dx, y + dy) for dx, dy in ((0, 1), (0, -1), (1, 0), (-1, 0))):
                if rnd.random() < 0.3:
                    g.set("front", x, y, 4)
    scatter(g, rnd, 110, 1, 1, W - 2, H - 2)

    objects = [
        spawn("spawn_from_hanjiacun", 1, 14, "right", default=True),
        portal("portal_to_hanjiacun", 0, 14, "ch01_hanjiacun", "spawn_from_ch05_dukou", h=2),

        facility("facility_cunji", 5, 12, "save"),

        # ---- 节点 1a：东去路上（峡口，踩上去就响）----
        trigger("trigger_dongqu_lu", 13, 14, "ch05/dongqu_lu.lua", "enter", True, h=2,
                set_flag="ch05.kaipian"),

        # ---- 节点 1b：上船（栈桥尽头的船）----
        trigger("trigger_shangchuan", 36, 14, "ch05/shangchuan.lua", "interact", True, h=2,
                guard_flag="ch05.kaipian", set_flag="ch05.shangchuan"),
        # 船家站在草棚门口，不挡栈桥
        npc("npc_chuanjia", 24, 10, "chuanjia", "down", "ch05/chuanjia.lua"),

        # ---- 野外遭遇：峡口以东的河滩（1a 之后才开；峡口以西那段谷地不放：①号仗得是头一仗）----
        # 从峡口径直走到栈桥十几步，够不着 steps_min：只赶路的人碰不上，往滩上溜达的才碰得上。
        encounter("encounter_hetan", 16, 5, 16, 20, "encounter_ch05_dukou", 20, 45,
                  require_flag="ch05.kaipian"),
    ]
    return build_map("ch05_dukou", W, H, "terrain_dukou",
                     map_props("ch05_dukou", "bgm_river"), g, objects)


# ---------------------------------------------------------------------------
# 地图 2：嘉元城西城 48x36 —— 节点 2、3、4b、10b
#
# 东边是运河，一处破码头：两座竹棚、一道栈桥。码头往北是仓房，往南是潇湘院那条街；
# 往西只有一条路——黑水巷，七拐八绕穿过一片黑压压的民居，最后在西南角出来，
# 一条巷子直通南城那道门。
#
#   2  trigger_matou         栈桥最里那一格（enter + once）。1b 的 teleport 落在栈桥外头，
#                            下船往岸上走必踩。
#   3a trigger_heishuixiang  黑水巷巷口（enter + once，guard ch05.matou）
#   3b trigger_zhuishao      往南城去的巷口（enter + once，guard ch05.shoufu）
#   4b trigger_jieren        仓房的门（interact + once，guard ch05.qingbao）
#   10b trigger_xiaoxiangyuan 潇湘院的门（interact + once，guard ch05.dingji）
#   存档点在潇湘院门外：码头下来就走得到，②③④与潇湘院那一仗都在它后头。
# ---------------------------------------------------------------------------
def make_xicheng():
    W, H = 48, 36
    g = Grid(W, H)
    rnd = random.Random(502)
    g.fill("ground", 1)
    border(g)

    # 整个西城先铺满民居，再挖街
    g.solid_rect(1, 1, 31, 34, 5)

    # 运河与码头
    water(g, 42, 1, 46, 34)
    floor(g, 32, 1, 41, 34)              # 码头这一片先整个清出来
    g.solid_rect(32, 1, 41, 2, 5)        # 北边一排货栈的后墙
    road(g, 33, 11, 41, 25)              # 码头木板
    road(g, 42, 17, 44, 18)              # 栈桥（伸进水里三格）
    g.solid_rect(35, 12, 37, 13, 8)      # 竹棚（北）
    g.solid_rect(35, 23, 37, 24, 8)      # 竹棚（南）

    # 仓房：码头后头，门朝南
    g.solid_rect(33, 3, 40, 8, 5)
    eaves_row(g, 33, 40, 9)
    road(g, 33, 9, 41, 10)

    # 潇湘院：码头南边，门朝北；门前一条街，西头封死（不许绕过黑水巷）
    g.solid_rect(31, 28, 40, 33, 5)
    eaves_row(g, 31, 40, 27)
    road(g, 28, 26, 41, 27)
    g.solid_rect(32, 34, 41, 34, 5)
    g.solid_rect(41, 28, 41, 33, 8)      # 院墙边的栏杆，把码头南角收住
    g.solid_rect(28, 28, 30, 34, 5)

    # 黑水巷：巷口两格（x 30..32，y 18..19），往西、往北、再往西南七拐八绕
    road(g, 26, 18, 32, 19)              # 巷口
    road(g, 26, 8, 27, 19)               # 往北
    road(g, 16, 8, 27, 9)                # 往西
    road(g, 16, 8, 17, 24)               # 往南
    road(g, 18, 22, 22, 22)              # 一条死巷（ch102 他就是被领进这样一条巷子）
    road(g, 6, 23, 17, 24)               # 往西
    road(g, 6, 23, 7, 29)                # 往南
    road(g, 6, 28, 9, 29)
    road(g, 8, 28, 9, 34)                # 往南城去的那条巷子
    road(g, 8, 35, 9, 35)                # 西城南门（通南城）
    # 巷子里几处小岔，只为不让它像一条管子
    road(g, 12, 11, 15, 11)
    road(g, 20, 13, 25, 13)
    road(g, 10, 17, 15, 17)

    # 屋檐压在巷子上
    for y in range(1, H - 1):
        for x in range(1, 31):
            if g.walkable(x, y) and not g.walkable(x, y - 1) and g.get("building", x, y - 1) == 5:
                if rnd.random() < 0.5:
                    g.set("front", x, y, 4)
    # 巷子里的污水与碎石
    for y in range(1, H - 1):
        for x in range(1, 32):
            if g.walkable(x, y) and rnd.random() < 0.18:
                g.set("overlay", x, y, 7)
    scatter(g, rnd, 60, 32, 3, 41, 33, grass=0.2)

    objects = [
        # 1b 的 teleport 落点，也是这张图的默认入口
        spawn("spawn_from_dukou", 44, 17, "left", default=True),
        spawn("spawn_from_nancheng", 8, 33, "up"),
        portal("portal_to_nancheng", 8, 35, "ch05_nancheng", "spawn_from_xicheng", w=2,
               require_flag="ch05.zhuishao", deny_text_key="ch05.block.nancheng"),

        # ---- 节点 2：码头（栈桥最里那一格）----
        trigger("trigger_matou", 42, 17, "ch05/matou.lua", "enter", True, h=2,
                set_flag="ch05.matou"),
        # ---- 节点 3a：黑水巷（巷口）----
        trigger("trigger_heishuixiang", 30, 18, "ch05/heishui.lua", "enter", True, h=2,
                guard_flag="ch05.matou", set_flag="ch05.shoufu"),
        # ---- 节点 3b：码头盯梢（往南城去的巷口）----
        trigger("trigger_zhuishao", 8, 31, "ch05/zhuishao.lua", "enter", True, w=2,
                guard_flag="ch05.shoufu", set_flag="ch05.zhuishao"),
        # ---- 节点 4b：铁拳会截人（仓房的门）----
        trigger("trigger_jieren", 36, 8, "ch05/jieren.lua", "interact", True, w=2,
                guard_flag="ch05.qingbao", set_flag="ch05.jieren"),
        # ---- 节点 10b：潇湘院（院门）----
        trigger("trigger_xiaoxiangyuan", 35, 28, "ch05/xiaoxiang.lua", "interact", True, w=2,
                guard_flag="ch05.dingji", set_flag="ch05.xiaoxiang"),

        facility("facility_cunji", 30, 26, "save"),
    ]
    return build_map("ch05_xicheng", W, H, "terrain_xicheng",
                     map_props("ch05_xicheng", "bgm_town"), g, objects)


# ---------------------------------------------------------------------------
# 地图 3：嘉元城南城 48x36 —— 节点 5、6a
#
# 南陵街东西横贯。街北是墨府的外墙（墨府里头是另一张图），正中一道黑漆大门；
# 墨府背后另有一条后巷，从西头一条窄弄堂绕进去。街南一排铺面：药铺、香家酒楼；
# 汇源客栈在街北东头那排门面里。
#
#   5  trigger_jiulou  香家酒楼的门（interact + once，guard ch05.jieren）
#   6a trigger_yeru    墨府后巷的墙根（interact + once，guard ch05.jiulou）
#   墨府正门：portal_to_mofu，require_flag=ch05.dengmen。门口四个护院分站两边，不挡门。
#
# 出入口一律「出门往哪走，进门就朝哪」（map_spec 规则 29）：
#   · 西城在北：西城南口往下走出来，落在这里西头那条窄弄堂的北口、面朝南，顺弄堂下到大街
#     （ch05.zhuishao.inn「往南走了一刻钟，街面宽了，灯多了」）。这道口子曾开在西墙
#     （面朝东），出门往南、进门朝东，拐了 90°。
#   · 客栈门开在街北那排朝南的门面上，往北走进门、落在客栈大堂南门口、面朝北。
#     它曾开在街南铺面的北沿：往南进门却落在客栈南门口、面朝北，回街的门就在脚下两格，
#     按住「下」便在两张图之间来回弹。
# ---------------------------------------------------------------------------
def make_nancheng():
    W, H = 48, 36
    g = Grid(W, H)
    rnd = random.Random(503)
    g.fill("ground", 1)
    border(g)

    # 南陵街
    road(g, 1, 16, 46, 20)

    # 墨府外墙（街北），门前一片石板
    g.solid_rect(14, 3, 33, 14, 5)
    road(g, 20, 15, 27, 15)
    road(g, 23, 14, 24, 14)              # 墨府大门的门洞
    # 墨府后巷：西头一条窄弄堂绕进去；弄堂北口就是通西城的城门
    road(g, 12, 1, 35, 2)
    road(g, 12, 1, 13, 15)
    road(g, 12, 0, 13, 0)                # 北口（回西城）
    g.solid_rect(1, 1, 11, 15, 5)
    g.solid_rect(34, 3, 46, 15, 5)
    g.solid_rect(36, 1, 46, 2, 5)
    road(g, 41, 15, 41, 15)              # 汇源客栈的门（街北东头那排门面，门朝南）
    eaves_row(g, 38, 44, 16)             # 客栈的檐子
    for x in range(14, 34):
        if rnd.random() < 0.4:
            g.set("front", x, 2, 4)      # 后巷里伸出来的树枝

    # 街南一排铺面（门都开在北墙上）
    g.solid_rect(1, 21, 46, 34, 5)
    g.solid_rect(8, 21, 13, 27, 8)       # 药铺的柜台一溜
    eaves_row(g, 8, 13, 20)
    eaves_row(g, 18, 25, 20)             # 香家酒楼的檐子
    eaves_row(g, 32, 39, 20)             # 街南东头那几间铺面的檐子
    # 铺面背后一条窄街，两条弄堂从南陵街穿过去——南城不是一堵墙
    road(g, 2, 29, 45, 30)
    floor(g, 2, 31, 45, 33)             # 街南是人家的后院，几棵树、一口井
    road(g, 29, 21, 30, 28)
    road(g, 3, 21, 4, 28)
    for tx, ty in ((8, 32), (20, 32), (36, 32), (44, 33)):
        tree(g, tx, ty)
    g.solid_rect(24, 31, 25, 32, 8)      # 井台

    for tx, ty in ((4, 17), (43, 19), (30, 16)):
        g.rect("front", tx - 1, ty - 1, tx + 1, ty - 1, 4)
    scatter(g, rnd, 60, 1, 16, 46, 20, grass=0.2)

    objects = [
        spawn("spawn_from_xicheng", 12, 1, "down", default=True),
        spawn("spawn_from_mofu", 23, 15, "down"),
        spawn("spawn_from_kezhan", 41, 16, "down"),

        portal("portal_to_xicheng", 12, 0, "ch05_xicheng", "spawn_from_nancheng", w=2),
        portal("portal_to_mofu", 23, 14, "ch05_mofu", "spawn_from_nancheng", w=2,
               require_flag="ch05.dengmen", deny_text_key="ch05.block.mofu"),
        portal("portal_to_kezhan", 41, 15, "ch05_kezhan", "spawn_from_nancheng"),

        # ---- 节点 5：香家酒楼（酒楼的门）----
        trigger("trigger_jiulou", 21, 21, "ch05/jiulou.lua", "interact", True, w=2,
                guard_flag="ch05.jieren", set_flag="ch05.jiulou"),
        # ---- 节点 6a：夜入墨府（后巷的墙根）----
        trigger("trigger_yeru", 24, 3, "ch05/yeru.lua", "interact", True, w=2,
                guard_flag="ch05.jiulou", set_flag="ch05.yeru"),

        # 南城药铺（施工图第 9 节）
        facility("facility_yaopu", 10, 21, "shop", w=2, ref_id="ch05_nancheng_yaopu"),

        # 墨府门口的护院（ch106：八个劲装大汉分站两侧；这里摆四个，文案里说八个）
        npc("npc_huyuan_a", 20, 15, "mofu_huyuan", "down"),
        npc("npc_huyuan_b", 21, 15, "mofu_huyuan", "down"),
        npc("npc_huyuan_c", 26, 15, "mofu_huyuan", "down"),
        npc("npc_huyuan_d", 27, 15, "mofu_huyuan", "down"),
        # 街坊：一个说得上话，一个只管走。走的那个是过路的汉子（他的打探文案
        # ch05.path.luren_dating.text 就这么称呼他），不再借街坊的 role——
        # 从前两个名牌都是「南城街坊」（规则 30）。
        npc("npc_jiefang", 30, 19, "jiayuan_jiefang", "up", "ch05/jiefang.lua"),
        npc("npc_luren", 40, 17, "jiayuan_guolu_hanzi", "left", wander=True),
    ]
    return build_map("ch05_nancheng", W, H, "terrain_nancheng",
                     map_props("ch05_nancheng", "bgm_town"), g, objects)


# ---------------------------------------------------------------------------
# 地图 4：汇源客栈 32x24（室内）—— 节点 4a、10a、10c、11c，支线 Z1、Z2，内视
#
#   上房（西北）：床（10c 睡觉）、书案两格（4a 摊遗书 · Z1 誊抄）、八仙桌两格
#                （10a 定计 · 11c 安排）、蒲团两格（左格内视、右格打坐）
#   后院（东北）：药炉（alchemy，1 品）、一截木桩（Z2 练符）
#   大堂（南）  ：存档点、告示板，南墙正中是门
#
# 内视（trigger_neishi）是全章唯一 once=false 的挂点：无 guard、不写 set_flag（规则 15）。
# 它与打坐处同是那张蒲团——蒲团两格宽，左格压着内视、右格是打坐的 facility。内视那一格
# 按下去永远是内视，打坐那一格照旧是打坐（规则 17：设施只要有一格不被压住就按得出来）。
# 它不与 10c 的床同格（施工图第 17 节第 8 条）。
# ---------------------------------------------------------------------------
def make_kezhan():
    W, H = 32, 24
    g = Grid(W, H)
    g.fill("ground", 1)
    border(g)

    # 隔墙：上房 | 后院，走廊 y 11..12，大堂 y 13..22
    g.solid_rect(1, 10, 30, 10, 5)
    g.solid_rect(15, 1, 16, 9, 5)
    g.solid_rect(1, 13, 30, 13, 5)
    road(g, 1, 11, 30, 12)               # 走廊
    road(g, 7, 10, 8, 10)                # 上房的门
    road(g, 22, 10, 23, 10)              # 后院的门
    road(g, 14, 13, 17, 13)              # 大堂的门

    # 上房
    g.solid_rect(2, 2, 3, 2, 8)          # 床（两格）
    g.solid_rect(8, 2, 9, 2, 8)          # 书案（两格）
    g.solid_rect(6, 5, 7, 6, 8)          # 八仙桌（两格见方）
    g.solid_rect(11, 7, 12, 7, 8)        # 蒲团（两格）

    # 后院：碎石地、药炉、木桩。**不铺 gid 3**：gid 3 上压着实体（药炉、木桩）会被渲染器读成
    # 「水面上砌了墙」（tests/MapArtTests.cpp 那条水与翻土的判据），院子用主地表加碎石就够了。
    g.rect("overlay", 17, 1, 30, 9, 7)
    g.solid_rect(20, 3, 21, 4, 8)        # 药炉（两格见方）
    g.solid_rect(27, 5, 27, 5, 8)        # 木桩

    # 大堂：柜台与几张桌子
    g.rect("ground", 1, 14, 30, 22, 2)
    g.solid_rect(3, 15, 8, 15, 8)        # 柜台
    for tx, ty in ((20, 16), (25, 16), (20, 20), (25, 20)):
        g.solid_rect(tx, ty, tx + 1, ty, 8)
    road(g, 15, 23, 16, 23)              # 客栈大门

    objects = [
        spawn("spawn_from_nancheng", 15, 21, "up", default=True),
        portal("portal_to_nancheng", 15, 23, "ch05_nancheng", "spawn_from_kezhan", w=2),

        # ---- 上房 ----
        # 节点 4a：书案左格，把遗书摊开
        trigger("trigger_qingbao", 8, 2, "ch05/qingbao.lua", "interact", True,
                guard_flag="ch05.zhuishao", set_flag="ch05.qingbao"),
        # 支线 Z1：书案右格，誊抄手札
        trigger("trigger_chaoxie", 9, 2, "ch05/chaoxie.lua", "interact", True,
                guard_flag="ch05.fengwu_qiu", set_flag="ch05.fengwu_chao"),
        # 节点 10a：八仙桌上两格
        trigger("trigger_dingji", 6, 5, "ch05/dingji.lua", "interact", True, w=2,
                guard_flag="ch05.duizhi", set_flag="ch05.dingji"),
        # 节点 11c：八仙桌下两格
        trigger("trigger_anpai", 6, 6, "ch05/anpai.lua", "interact", True, w=2,
                guard_flag="ch05.yange", set_flag="ch05.anpai"),
        # 节点 10c：床
        trigger("trigger_shuijiao", 2, 2, "ch05/shuijiao.lua", "interact", True, w=2,
                guard_flag="ch05.xiaoxiang", set_flag="ch05.duobang"),
        # 内视：蒲团左格（可重复，不写 set_flag）
        trigger("trigger_neishi", 11, 7, "ch05/neishi.lua", "interact", False),
        facility("facility_dazuo", 11, 7, "meditate", w=2),

        # ---- 后院 ----
        facility("facility_yaolu", 20, 3, "alchemy", w=2, h=2, grade=1),
        # 支线 Z2：木桩
        trigger("trigger_lian_jianfu", 27, 5, "ch05/lian_jianfu.lua", "interact", True,
                guard_flag="ch05.jianfu_qiu", set_flag="ch05.lian_jianfu"),

        # ---- 大堂 ----
        facility("facility_cunji", 27, 14, "save"),
        facility("facility_gaoshi", 11, 14, "board", w=2),
    ]
    return build_map("ch05_kezhan", W, H, "terrain_kezhan",
                     map_props("ch05_kezhan", "bgm_inn", outdoor=False), g, objects)


# ---------------------------------------------------------------------------
# 地图 5：墨府 48x36 —— 节点 6b、7a、7b、8、9、11a、11b、12a、12b、12e
#
#   南：正门（通南城）→ 前院（存档点）
#       前院西头是马厩院：马厩、封死多年的地窖口，院门一格宽两格高（x 11），
#       12a 在马厩门上（interact），12b 在院门上（enter）——从马厩出来必过院门。
#   中：一道两层的院墙（y 22..23）把前院与后头隔开，只在 x 24 开一格月洞门。
#       11b（燕歌讨教）在月洞门外格（enter）；夜里暗哨站在里格（见文件首部第四节）。
#   北：后园（西，花木与墨凤舞的药圃）→ 一条一格宽的竹径 → 小楼（中）→ 后宅（东北）
#       小楼五场戏在五格上：西墙 6b、东窗 7a、楼门三格 9 / 11a / 12e。
#       小楼通后宅厢房是一段一格宽的夹道 (42,7)..(42,9)：靠小楼的 (42,9) 是 7b，
#       靠厢房的 (42,8) 是 8。7b 把人送到厢房的床前，第二天出门必踩 8。
# ---------------------------------------------------------------------------
MOFU_BACK_GARDEN = (3, 3)      # 6a 翻墙落地的那一格（后园西北角）
MOFU_BED_FRONT = (40, 3)       # 7b 宿厢房，脚本把人放在床前
MOFU_FRONT_YARD = (23, 30)     # 12d 得手之后回城，落在前院


def make_mofu():
    W, H = 48, 36
    g = Grid(W, H)
    rnd = random.Random(505)
    g.fill("ground", 1)
    border(g)

    # ---- 前院 ----
    road(g, 23, 24, 24, 34)              # 正门进来的甬道
    road(g, 23, 35, 24, 35)              # 正门
    g.rect("ground", 13, 26, 35, 32, 2)  # 前院的石板
    for tx, ty in ((16, 33), (31, 33), (37, 27), (43, 30)):
        tree(g, tx, ty)

    # ---- 马厩院（x 1..10）----
    g.solid_rect(11, 24, 11, 34, 5)      # 马厩院东墙
    g.open_rect(11, 29, 11, 30)          # 院门（一格宽两格高）
    g.rect("ground", 11, 29, 11, 30, 2)
    g.solid_rect(2, 25, 8, 27, 5)        # 马厩
    eaves_row(g, 2, 8, 28)
    g.rect("ground", 9, 25, 10, 26, 3)   # 地窖口一带翻着碎砖（可踩）
    g.rect("overlay", 9, 25, 10, 26, 7)
    g.solid_rect(2, 33, 4, 33, 8)        # 草料垛
    g.set("building", 0, 31, 8)          # 侧门（只是一扇门的样子，外头是另一条巷子）

    # ---- 中间那道两层院墙，只留月洞门 ----
    g.solid_rect(1, 22, 46, 23, 5)
    g.open_rect(24, 22, 24, 23)
    g.rect("ground", 24, 22, 24, 23, 2)

    # ---- 中庭（y 14..21）：假山、花木 ----
    g.solid_rect(8, 16, 11, 18, 5)       # 假山
    g.solid_rect(35, 15, 38, 17, 5)
    for tx, ty in ((18, 19), (30, 19), (44, 18), (4, 20)):
        tree(g, tx, ty)
    road(g, 24, 11, 24, 21)              # 小楼门前到月洞门的石板路

    # ---- 后园（西北，x 1..20，y 1..13）----
    g.solid_rect(21, 1, 22, 13, 5)       # 后园东边的竹篱（两格厚）
    g.open_rect(21, 7, 22, 7)            # 竹径：一格宽
    g.rect("ground", 21, 7, 22, 7, 2)
    g.set("front", 21, 6, 4)
    g.set("front", 22, 8, 4)
    tilled(g, 6, 5, 11, 6)               # 墨凤舞的药圃（两畦）
    tilled(g, 6, 9, 11, 10)
    for tx, ty in ((15, 3), (17, 10), (3, 11), (13, 12)):
        tree(g, tx, ty)
    g.solid_rect(1, 14, 20, 14, 5)       # 后园南墙（与中庭之间）
    g.open_rect(14, 14, 15, 14)          # 后园南门
    g.rect("ground", 14, 14, 15, 14, 2)

    # ---- 小楼（x 26..33，y 5..9）----
    g.solid_rect(26, 5, 33, 9, 5)
    eaves_row(g, 26, 33, 4)
    road(g, 25, 10, 42, 11)              # 楼前的廊子，往东一直通到后宅夹道口

    # ---- 后宅厢房（东北）：一间屋，南墙三格厚，只由一段一格宽的夹道进出 ----
    g.solid_rect(37, 1, 46, 9, 5)
    floor(g, 38, 2, 45, 6)               # 厢房里头
    road(g, 42, 7, 42, 9)                # 夹道：(42,9) 靠小楼、(42,8) 靠厢房、(42,7) 是房门
    g.solid_rect(39, 2, 40, 2, 8)        # 床（两格）
    g.solid_rect(44, 2, 45, 2, 8)        # 衣柜
    eaves_row(g, 37, 46, 10)

    scatter(g, rnd, 140, 1, 1, W - 2, H - 2, grass=0.75)

    objects = [
        spawn("spawn_from_nancheng", 23, 33, "up", default=True),
        portal("portal_to_nancheng", 23, 35, "ch05_nancheng", "spawn_from_mofu", w=2),

        facility("facility_cunji", 14, 27, "save"),

        # ---- 节点 6b：偷听（小楼西墙）----
        trigger("trigger_toutin", 26, 7, "ch05/toutin.lua", "interact", True,
                guard_flag="ch05.yeru", set_flag="ch05.toutin"),
        # ---- 节点 7a：登门（小楼东窗）----
        trigger("trigger_dengmen", 33, 7, "ch05/dengmen.lua", "interact", True,
                guard_flag="ch05.toutin", set_flag="ch05.dengmen"),
        # ---- 节点 7b：见面礼（夹道靠小楼那一格）----
        trigger("trigger_jianmianli", 42, 9, "ch05/jianmian.lua", "enter", True,
                guard_flag="ch05.dengmen", set_flag="ch05.jianmianli"),
        # ---- 节点 8：花园（夹道靠后宅那一格）----
        trigger("trigger_huayuan", 42, 8, "ch05/huayuan.lua", "enter", True,
                guard_flag="ch05.jianmianli", set_flag="ch05.huayuan"),
        # ---- 节点 9：孝服（楼门左格）----
        trigger("trigger_duizhi", 28, 9, "ch05/duizhi.lua", "interact", True,
                guard_flag="ch05.huayuan", set_flag="ch05.duizhi"),
        # ---- 节点 11a：交易（楼门中格）----
        trigger("trigger_jiaoyi", 30, 9, "ch05/jiaoyi.lua", "interact", True,
                guard_flag="ch05.duobang", set_flag="ch05.jiaoyi"),
        # ---- 节点 12e：换玉（楼门右格）。章末 ----
        trigger("trigger_huanyu", 32, 9, "ch05/huanyu.lua", "interact", True,
                guard_flag="ch05.cisha", set_flag="ch05.done"),
        # ---- 节点 11b：燕歌讨教（月洞门外格）----
        trigger("trigger_yange", 24, 23, "ch05/yange.lua", "enter", True,
                guard_flag="ch05.jiaoyi", set_flag="ch05.yange"),
        # ---- 节点 12a：马厩（马厩的门）----
        trigger("trigger_majiu", 5, 27, "ch05/majiu.lua", "interact", True,
                guard_flag="ch05.anpai", set_flag="ch05.shigui"),
        # ---- 节点 12b：吴剑鸣（马厩院院门）----
        trigger("trigger_zhuwu", 11, 29, "ch05/zhuwu.lua", "enter", True, h=2,
                guard_flag="ch05.shigui", set_flag="ch05.chuzheng"),

        # 暗哨：只在「翻进来了、还没登门」那一夜站在月洞门里格（文件首部第四节）
        npc("npc_anshao", 24, 22, "mofu_huyuan", "up",
            visible_flag="ch05.yeru", hidden_flag="ch05.dengmen"),
        # 墨凤舞在药圃里（支线 Z1 交付）。登门之后才在，走了之后不在
        npc("npc_mo_fengwu", 8, 7, "mo_fengwu", "down", "ch05/fengwu.lua",
            visible_flag="ch05.dengmen", hidden_flag="ch05.done"),
        # 燕歌在中庭闲站：花园那一面之后才在，讨教之后不再出来（原著 ch115 之后他再没出现）
        npc("npc_yan_ge", 30, 16, "yan_ge", "left", "ch05/yange_npc.lua",
            visible_flag="ch05.huayuan", hidden_flag="ch05.yange"),
    ]
    return build_map("ch05_mofu", W, H, "terrain_mofu",
                     map_props("ch05_mofu", "bgm_manor"), g, objects)


# ---------------------------------------------------------------------------
# 地图 6：独霸山庄 40x30 —— 节点 12c、12d，支线 Z2′
#
# 西南一片林子（庄外林子），东北是山庄的围墙。围墙西面有一处塌了半截的水口，
# 钻进去是山庄的后园，后园里一座赏月亭；后园与山庄里头之间另有一道内墙，
# 所以从水口进来只到得了后园。
#
#   12c trigger_tancha          围墙西墙根（interact + once，guard ch05.chuzheng）
#   12d trigger_shangyue        赏月亭（interact + once，guard ch05.tancha）
#   Z2′ trigger_lian_jianfu_lin 林子里一截老树桩（interact + once，guard ch05.jianfu_qiu），
#                               与客栈后院那一处挂同一个脚本（施工图 8.4）
#   存档点在林子里。
# ---------------------------------------------------------------------------
SHANZHUANG_LINZI = (4, 25)     # 12b 骑马十日之后落脚的林子（也是默认入口）


def make_dubashanzhuang():
    W, H = 40, 30
    g = Grid(W, H)
    rnd = random.Random(506)
    g.fill("ground", 1)
    border(g)

    # ---- 山庄（x 20..38，y 1..20）----
    g.solid_rect(20, 1, 38, 20, 5)       # 先整个砌实
    floor(g, 21, 2, 28, 9)               # 后园
    g.solid_rect(21, 10, 28, 10, 5)      # 后园与山庄里头的内墙
    g.solid_rect(29, 2, 29, 9, 5)
    g.open_rect(20, 6, 20, 6)            # 西墙上塌了半截的水口
    g.rect("ground", 20, 6, 20, 6, 3)
    g.solid_rect(24, 4, 25, 5, 8)        # 赏月亭
    eaves_row(g, 23, 26, 3)
    g.rect("ground", 23, 6, 26, 7, 2)    # 亭前石板
    for x in range(21, 38):
        if rnd.random() < 0.5:
            g.set("front", x, 21, 4)     # 围墙外的墙檐

    # ---- 庄外林子（x 1..18，y 1..28）----
    for tx, ty in ((3, 3), (7, 5), (12, 3), (16, 7), (4, 9), (9, 11), (14, 13),
                   (3, 16), (8, 18), (15, 18), (12, 23), (17, 26), (7, 27),
                   (2, 21), (16, 2), (18, 14), (11, 7)):
        tree(g, tx, ty)
    g.solid_rect(10, 22, 10, 22, 8)      # 老树桩（Z2′）
    road(g, 3, 24, 5, 26, gid=1)         # 落脚那一小块空地
    scatter(g, rnd, 150, 1, 1, W - 2, H - 2, grass=0.8)

    objects = [
        spawn("spawn_linzi", SHANZHUANG_LINZI[0], SHANZHUANG_LINZI[1], "up", default=True),

        facility("facility_cunji", 6, 24, "save"),

        # ---- 节点 12c：刺探与巡庄（西墙根）----
        trigger("trigger_tancha", 20, 12, "ch05/tancha.lua", "interact", True,
                guard_flag="ch05.chuzheng", set_flag="ch05.tancha"),
        # ---- 节点 12d：赏月亭 ----
        trigger("trigger_shangyue", 24, 5, "ch05/shangyue.lua", "interact", True, w=2,
                guard_flag="ch05.tancha", set_flag="ch05.cisha"),
        # ---- 支线 Z2′：林子里的老树桩（与客栈后院同一个脚本）----
        trigger("trigger_lian_jianfu_lin", 10, 22, "ch05/lian_jianfu.lua", "interact", True,
                guard_flag="ch05.jianfu_qiu", set_flag="ch05.lian_jianfu"),

        # ---- 野外遭遇：庄外林子（只有野兽；不用庄丁，第 5 章校对 8.4）----
        # 围墙根那一列（x 19..20）不在区里：伏在墙外刺探的那几步不该被一头野猪打断。
        encounter("encounter_linzi", 1, 1, 18, 28, "encounter_ch05_linzi", 18, 40),
    ]
    return build_map("ch05_dubashanzhuang", W, H, "terrain_dubashanzhuang",
                     map_props("ch05_dubashanzhuang", "bgm_wild"), g, objects)


# ---------------------------------------------------------------------------
# 韩家村：村东新口 —— 就地修补，不重建（原委见本文件第五节）
# ---------------------------------------------------------------------------
def patch_hanjiacun(payload=None):
    """给韩家村补上通往渡口的村东新口。幂等，补过的图再补一次不变。

    payload 为 None 时读 maps/ch01_hanjiacun.tmj（本脚本自己跑的路径）；
    genmaps.py 会把第 4 章修补之后手上那一份传进来。追加顺序固定（落点 → 传送点），
    否则两边对象表排序不同，`--check` 会报一堆假漂移。
    """
    if payload is None:
        path = os.path.join(genmaps.MAPS, "ch01_hanjiacun.tmj")
        payload = json.loads(open(path, "rb").read().decode("utf-8"))
    W = payload["width"]
    layers = {l["name"]: l for l in payload["layers"]}

    def put(layer, x, y, value):
        layers[layer]["data"][y * W + x] = value

    # 村东篱笆开两格，铺一小段路。第 4 章东去的那个人就站在老树底下，
    # 这道口子离他七步远。
    for y in (20, 21):
        put("building", W - 1, y, 0)
        put("collision", W - 1, y, 0)
        for x in (36, 37, 38, 39):
            put("ground", x, y, 2)
            put("overlay", x, y, 0)

    objs = layers["objects"]["objects"]
    names = {o["name"] for o in objs}
    if "spawn_from_ch05_dukou" not in names:
        objs.append(spawn("spawn_from_ch05_dukou", 38, 20, "left"))
    if "portal_to_dukou" not in names:
        # 第 4 章走完才通：在那之前，村东这两格还是篱笆。
        objs.append(portal("portal_to_dukou", 39, 20, "ch05_dukou", "spawn_from_hanjiacun", h=2,
                           require_flag="ch04.done", deny_text_key="ch05.block.dukou"))

    for i, o in enumerate(objs, 1):
        o["id"] = i
    payload["nextobjectid"] = len(objs) + 1
    return payload


# ---------------------------------------------------------------------------
def build_all():
    """本章六张图的完整产出，外加韩家村那处就地修补的村东新口。

    韩家村先过第 4 章那一次修补再过本章这一次：单跑本脚本时读的是 maps/ 里那张
    已经带着第 4 章车道口的图，与 genmaps.py 那边的产出一致。
    """
    # 第 6 章在南城东墙开了一道门（往太南山脚），归 genmaps_ch06.py 所有（它的 patch_nancheng）。
    # 不在这里补回去，单跑本脚本就会把第 6 章整章从游戏里割断——太南山脚只从这一处进得去。
    import genmaps_ch06  # 延迟导入：模块层导入会与它的 import genmaps_ch05 成环

    return {
        "ch05_dukou": make_dukou(),
        "ch05_xicheng": make_xicheng(),
        "ch05_nancheng": genmaps_ch06.patch_nancheng(make_nancheng()),
        "ch05_kezhan": make_kezhan(),
        "ch05_mofu": make_mofu(),
        "ch05_dubashanzhuang": make_dubashanzhuang(),
        "ch01_hanjiacun": patch_hanjiacun(),
    }


TILESETS = ("terrain_dukou", "terrain_xicheng", "terrain_nancheng",
            "terrain_kezhan", "terrain_mofu", "terrain_dubashanzhuang")


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
