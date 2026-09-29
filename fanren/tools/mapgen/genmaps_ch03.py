# -*- coding: utf-8 -*-
"""第 3 章地图生成器：重跑即重建下列四张图与四个 tileset，并给神手谷补两处口子。

    python tools/mapgen/genmaps_ch03.py
    python tools/mapgen/genmaps_ch03.py --check   # 只比对，不写盘；有漂移则退出码 1
    python tools/validate.py --maps               # 自检

    （`genmaps.py --check` 会把本章这四张也一并盯上，所以 build.bat 里只有那一条。）

产出（全部写进 maps/，不经手 scripts/ 与 data/）：

    maps/ch03_mishi.tmj      24x18  神手谷密室（摊牌、尸虫丸、解药）
    maps/ch03_cangshu.tmj    32x24  藏书处（偷秘籍；明路短而险、暗路长而稳）
    maps/ch03_andao.tmj      40x32  谷中暗道（纵向；云翅鸟线索与第二场战斗）
    maps/ch03_guwai.tmj      40x30  谷外（第一场仗的战场；章末离谷）
    maps/tilesets/terrain_{mishi,cangshu,andao,guwai}.tsj

    另就地修补 maps/ch01_shenshougu.tmj（见第四节，那里有原委）。

--------------------------------------------------------------------------
一、空间关系（先想清楚再摆 —— 第 1 章在这上面栽过一次：山路摆在了村与镇之间，
    而宗门正门就在那座山上）

    神手谷是七玄门内墨大夫的医馆兼居所，一条南北向的谷，谷口朝南。
    第 1、2 章已经定死的部分：

                                              ┌──北东─▶ 外刃堂 ch02_wairentang
        神手谷 ch01_shenshougu ── 北 ──▶ 药圃 ch02_yaopu ──西──▶ 居所 ch02_jusuo
        （堂屋 · 偏屋 · 打坐台）              └──东──▶ 崖壁 ch02_yabi
          │  谷口
          ▼ 南
        炼骨崖（ch01_liangu_ya，从崖顶往北翻过来）

    （2026-09-27 前谷口朝北、药圃在谷南：炼骨崖崖顶北出却落在谷北口，按住「上」
    就来回弹。两头对调之后「出门往哪走，进门就朝哪」，见 map_spec 规则 29。）

    本章往这张骨架上加两处，一处朝里、一处朝外：

        · 朝里：墨大夫堂屋（ch01_shenshougu 的 x5..11 / y5..9）南墙原本是一整面，
          本章在 (8,9) 开一处门洞，进去是**堂屋底下的密室**（ch03_mishi）。
          摊牌、尸虫丸、解药三节都在这间屋子里 —— 它就在师父屋子底下，
          玩家这四年天天从门口走过，这一点是本章要的那种恶心劲。
          密室往东一道石门进**藏书处**（ch03_cangshu，同在堂屋地基之下），
          往西一道暗门进**暗道**（ch03_andao，落在暗道东口）。
          进门是往北走（门洞开在堂屋南墙），所以密室的石阶甬道从它的南墙上来。

        · 朝外：谷西壁 y13..14 处本就是谷中横径的西端，往外凿穿是一道豁口，
          出去便是谷外的荒坡（ch03_guwai）。荒坡在谷之西、彩霞山的外坡上，
          既不在宗门院墙里，也不是山下的青牛镇 —— 野狼在这儿说得通，
          而炼骨崖（门内考核场）那边说不通。

        · 暗道是墨居仁自己凿的一条退路：自密室向西、再向南向下穿山，
          出口就在荒坡北侧崖根的一道石缝里。所以它连的是「密室 ↔ 谷外」，
          与设计文档第 6 节一致。这也正是章末离谷要走它的理由 ——
          不经谷口、不经宗门。

    连通与闸门一览（括号里是 require_flag）：

        ch01_shenshougu ──(ch02.done)──▶ ch03_guwai ──(ch03.andao_zhan)──▶ ch03_andao
                │                              ▲                               │
                │                              └───────────(无闸门)────────────┘
                └──(ch03.mo_gui_gu)──▶ ch03_mishi ──(ch03.shichong_wan)──▶ ch03_cangshu
                                             └─────(ch03.beidu_done)─────▶ ch03_andao

    反向一律不设闸门：进得来就出得去，免得把玩家关在里面。

    ch03.beidu_done 这一道要多看一眼：钥匙**不在密室里**，在第 2 章的药圃
    （ch02_yaopu 的 trigger_beidu，由 genmaps_ch02.make_yaopu() 摆）。
    玩家偷完秘籍要从藏书处折回密室、上神手谷、北上药圃挖那一丛土菇花，
    再原路回密室才推得开暗道那道门。三跳都没有闸门拦（谷北口那道
    portal_to_ch02_yaopu 只要 ch01.done），顺序合法，规则 19 静态就推得出来。

二、防跳过与「谁置这个旗标」

    本章五道闸门用的旗标，全部由本文件某个触发器的 set_flag 兑现，
    没有一个悬在尚未写出的脚本上 —— 规则 19 是静态推演的，
    闸门的钥匙若只存在于将来的某个脚本里，那道门在门禁眼里就是永远打不开的：

        ch02.done          scripts/ch02/suanzhang.lua（第 2 章章末，已有）
        ch03.elang_done    ch03_guwai  trigger_elang
        ch03.mo_gui_gu     ch01_shenshougu trigger_mogui（见第四节）
        ch03.shichong_wan  ch03_mishi  trigger_shichong
        ch03.beidu_done    ch02_yaopu  trigger_beidu（genmaps_ch02.make_yaopu）
        ch03.andao_zhan    ch03_andao  trigger_andao_zhan

    段内的先后次序不靠地图拦，靠脚本自己在开头查前置再 return —— 这是第 2 章
    定下的口径，好处是被拦住的玩家听得见自己还差什么，而不是按了没反应。
    所以本章只有 trigger_mogui 一处挂 guard_flag（它是踏入型，压在谷中往药圃去的
    主径上，不挂 guard 会让玩家在第 2 章就踩到一格一声不吭的地面）。

三、once 与 set_flag（map_spec 4.4，2026-09-20 改过的那一条）

    once=true 的触发器一律写 set_flag，指向脚本在**演完之后**置的那个完成旗标。
    引擎据此判断这一幕演过没有，不再预写任何内部记号；脚本半路 return 走不到
    那一行，于是触发器留着可重演。本章十处剧情触发都是这个写法。
    唯一的 once=false 是藏书处的 trigger_mingdao（走明路被撞见的风险），
    它按设计要能反复触发，所以**不写** set_flag（写了会被规则 15 判错）。

四、神手谷那两处口子：就地修补，不重建

    神手谷的地形归 genmaps.py 所有，第 2 章已经用 patch_shenshougu() 在它北墙
    开过一处谷口。本章照同一个路子，只改需要改的那几格、只追加那几个对象，
    其余原样写回；幂等，补过的图再补一次不变。

    genmaps.py 的 build_all() 会在第 2 章那一处补丁之后回头调用本文件的
    patch_shenshougu()，所以单跑 genmaps.py 也不会把第 3 章割断。
    —— 这一条不是假想：第 1 章的闸门曾经只存在于 tmj 里，重跑一次就是一次
    静默的数据损毁；第 2 章的入口也险些被同一个缺口抹掉。

    第 2 章那两张图（居所 ch02_jusuo、药圃 ch02_yaopu）是 genmaps_ch02.py
    **从零重建**的，所以本章落在那两张图上的挂点一律摆在**那个文件**里，
    不从这里塞 —— 往它产出的 tmj 里补对象会让 `genmaps_ch02.py --check`
    当场报漂移。现已这样摆着的有四处：居所的 trigger_yingdui（节点 5）、
    trigger_duoshe（节点 10）、trigger_chujue（节点 11），药圃的
    trigger_beidu（节点 7）。神手谷不同 —— 第 2 章那边的 patch_shenshougu()
    是**读盘再补**的，所以本章往它上面再补一层，两边的 --check 都仍然
    一字不差。

五、瓦片 gid 约定（沿用 genmaps.py 那一套，四个 tileset 统一编号）

    1 主地表  2 道路/石阶  3 特殊地表（碎石/翻土）  4 front 遮挡
    5 building 主体（墙体/岩壁）  6 overlay 花草  7 overlay 碎石
    8 building 次体（家具/书架/栏杆/落石）

不变量：building 有实体瓦片处 collision 必须非 0，一律走 Grid.solid_rect。
"""
import json
import os
import random
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

# 对象小工具连同闸门参数一并住在 genmaps.py 里，本章原样沿用。
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

CHAPTER = 3
PREFIX = "ch03_"


def map_props(map_id, bgm, outdoor=True):
    return [
        ("map_id", map_id),
        ("display_name_key", "ch03.map.%s.name" % map_id[len(PREFIX):]),
        ("region", "qingzhou"),       # 与第 1、2 章一致：越国镜州彩霞山
        ("bgm", bgm),
        ("chapter", CHAPTER),
        ("outdoor", outdoor),
        # can_leave_edge 一律 false：TileMapLoader 只校验它的存在与类型，
        # 引擎里没有任何地方消费它，写 true 只会得到一个不生效的属性。
        ("can_leave_edge", False),
    ]


def encounter(name, x, y, w, h, table_id, steps_min, steps_max, require_flag=None):
    """野外遭遇区（map_spec 4.5；docs/interfaces-octo-encounters.md 第 3 节）。

    区定疏密（步数区间），表（data/encounters/）定遇上谁。require_flag 是本作补的可选属性：
    区在那面旗置起来之前不数步、横幅也不画危险度——谷外要等教学那一仗打完才有野兽。
    """
    kw = dict(table_id=table_id, steps_min=steps_min, steps_max=steps_max)
    if require_flag:
        kw["require_flag"] = require_flag
    return obj(name, "encounter", x, y, w, h, **kw)


# ---------------------------------------------------------------------------
# 网格小工具（与 genmaps_ch02.py 同形；那边的没有导出，这里各留一份）
# ---------------------------------------------------------------------------
def border(g, gid=5):
    """四面围一圈实体。开口由调用方随后挖出来。"""
    g.solid_rect(0, 0, g.w - 1, 0, gid)
    g.solid_rect(0, g.h - 1, g.w - 1, g.h - 1, gid)
    g.solid_rect(0, 0, 0, g.h - 1, gid)
    g.solid_rect(g.w - 1, 0, g.w - 1, g.h - 1, gid)


def gate(g, x0, y0, x1, y1):
    """在墙上挖一处门洞并铺路。"""
    g.open_rect(x0, y0, x1, y1)
    g.rect("ground", x0, y0, x1, y1, 2)


def road(g, x0, y0, x1, y1, gid=2):
    """铺一条路：清掉实体 + 地表换成道路。"""
    g.open_rect(x0, y0, x1, y1)
    g.rect("ground", x0, y0, x1, y1, gid)


def tree(g, x, y):
    """一棵树：树干占一格，树冠三格见方压在 front 层（不挡路）。"""
    g.rect("front", x - 1, y - 1, x + 1, y + 1, 4)
    g.solid_rect(x, y, x, y, 5)


def eaves(g, rnd, density=0.35):
    """给紧挨着可走格的岩壁压一层 front，室内与洞道用，做出「头顶压着石头」的观感。"""
    for y in range(g.h):
        for x in range(g.w):
            if g.walkable(x, y):
                continue
            if not any(g.walkable(x + dx, y + dy)
                       for dx, dy in ((0, 1), (0, -1), (1, 0), (-1, 0))):
                continue
            if rnd.random() < density:
                g.set("front", x, y, 4)


def scatter(g, rnd, count, x0, y0, x1, y1, grass=0.7):
    """在可通行的空地上撒花草与碎石，只动 overlay，不影响通行。"""
    for _ in range(count):
        x = rnd.randrange(x0, x1 + 1)
        y = rnd.randrange(y0, y1 + 1)
        if g.walkable(x, y) and g.get("overlay", x, y) == 0 and g.get("ground", x, y) != 2:
            g.set("overlay", x, y, 6 if rnd.random() < grass else 7)


# ---------------------------------------------------------------------------
# 地图 1：神手谷密室 24x18 —— 摊牌、尸虫丸、解药
#
# 压迫感是这张图唯一的设计目标，做法有三条，都在尺寸上：
#   · 24x18 的图里只有 16x8 的主室 + 两个两格见方的壁龛真正能走，其余全是石头；
#   · 三个出口全是一格半宽的门洞，进来的那条还是一条三格深的石阶甬道；
#   · 甬道里口横着节点 3 的踏入触发 —— 走完最后一级台阶就摊牌，退不回去。
#
# 甬道在南墙：神手谷那道门是往北走进去的（堂屋南墙的门洞），进来就该落在屋子南头、
# 面朝北（map_spec 规则 29）。甬道曾开在北墙：往北进门却落在北头、面朝南，
# 回谷的门就在脚下两格，按住「上」便在两张图之间来回弹。北墙那段旧甬道留成
# 一处壁龛（砌死的话，存档正落在那几格上的人会被关在石头里）。
#
# 挂点（脚本由编剧写，本图只按设计文档第 5 节的节点表引用）：
#   节点 3 摊牌     → trigger_tanpai（甬道底口，enter + once）
#   节点 4 尸虫丸   → trigger_shichong（石案侧，interact + once）
#   节点 9 解药     → trigger_jieyao（药柜前，interact + once）
#   墨居仁本人      → npc_mo_daifu（石案后。role_id 先用已有的 mo_daifu，
#                     data/roles/mo_juren.json 建好之后改这一处即可）
# ---------------------------------------------------------------------------
def make_mishi():
    W, H = 24, 18
    g = Grid(W, H)
    rnd = random.Random(301)
    g.fill("ground", 1)
    g.fill("building", 5)
    g.fill("collision", 1)

    # 石阶甬道：自堂屋地面下来，两格宽，从南墙进屋
    road(g, 11, 14, 12, H - 1)
    # 北墙那段旧甬道：只剩一处壁龛，不再通外头。加宽成三格：两格宽、三面是墙的死胡同
    # 尽头会被美术生成器认成「墙上的门龛」画出门框（mapinfer.find_gaps 的 notch，宽 ≤2），
    # 像一道打不开的门；三格宽就只是一处凹进去的空当。
    road(g, 10, 1, 12, 5, gid=1)

    # 主室
    road(g, 4, 6, 19, 13, gid=1)

    # 东龛（往藏书处的石门）与西龛（往暗道的暗门）
    road(g, 20, 8, 21, 9)
    road(g, 2, 11, 3, 12)

    # 家具。北壁那一排分列东西两头，当中 x11..12 空着 —— 那是旧甬道留下的壁龛口；
    # 南壁那一排（石榻、丹炉）同样让开 x11..12，那是如今甬道的落口，
    # 摆任何东西都会把这间屋子跟唯一的入口切开（初稿的石案压在旧落口上，
    # 规则 13 当场报了七条「从出生点无法到达」）。
    g.solid_rect(5, 6, 8, 6, 8)           # 石案
    g.solid_rect(16, 6, 19, 6, 8)         # 药柜（解药在这儿）
    g.solid_rect(17, 12, 19, 13, 8)       # 石榻
    g.solid_rect(5, 12, 6, 13, 8)         # 丹炉
    for px, py in ((7, 9), (7, 11), (16, 9), (16, 11)):
        g.solid_rect(px, py, px, py, 8)   # 石柱

    g.rect("overlay", 4, 6, 19, 13, 7)    # 满地碎石
    eaves(g, rnd, 0.42)

    objects = [
        # ---- 出入口 ----
        spawn("spawn_from_shenshougu", 11, H - 2, "up", default=True),
        spawn("spawn_from_cangshu", 19, 8, "left"),
        spawn("spawn_from_andao", 4, 11, "right"),

        portal("portal_to_shenshougu", 11, H - 1, "ch01_shenshougu",
               "spawn_from_ch03_mishi", w=2),
        # 服下尸虫丸、成了他的人之后，藏书处才对韩立开着 —— 偷秘籍偷的正是
        # 这份「他以为拿捏住了」的松懈（节点 6 的前提）。
        portal("portal_to_cangshu", 21, 8, "ch03_cangshu", "spawn_from_mishi", h=2,
               require_flag="ch03.shichong_wan", deny_text_key="ch03.block.cangshu"),
        # 暗道是备毒那一夜才摸到的：节点 7 在先，节点 8 在后。
        portal("portal_to_andao", 2, 11, "ch03_andao", "spawn_from_mishi", h=2,
               require_flag="ch03.beidu_done", deny_text_key="ch03.block.andao"),

        # ---- 节点 3：摊牌（甬道里口，两格全占，退不回去） ----
        trigger("trigger_tanpai", 11, 14, "ch03/tanpai.lua", "enter", True, w=2,
                set_flag="ch03.tanpai_done"),

        # ---- 墨居仁 ----
        # 摊牌前后必须是同一个人，所以这里不换 role，只换他站的地方：
        # 第 2 章他在药圃的田埂上，这一章他站在屋子正中、面朝石阶口（南）——
        # 玩家一路下来，他是一直等在那儿的。
        # 他归谷（ch03.mo_gui_gu）之前不在这儿：密室的门也是那时才开，
        # 而同一个人不该在药圃、神手谷堂前与密室同时站着。
        # 章末高潮把他埋了之后他就不该还在这儿拨算盘。ch03.mo_siwang 的登记原话
        # 就是「墨居仁已暴毙」，而 ch03.ligu.clean 明写「师父埋在药圃东头那棵老槐
        # 底下」——没有这一条 hidden_flag，玩家演完节点 11 从 ch03_andao 折回来
        # （那道门没有任何 require_flag，一步就到），按一下确认键，mojuren.lua 会
        # 走到第三条分支播 ch03.npc.mojuren_again：一个正在数东西、数到一半会重来
        # 的活人。本批其它 NPC（管事、厉飞雨）都做了按章切换，独独漏了这一个，
        # 而漏掉的恰好是全章唯一一个会死的人。
        #
        # 只隐不补：他的尸首归节点 12 在 ch03_andao 搜（tiezhe.lua / quhun_rudui.lua），
        # 这间密室在他死后本来就该是空的。若日后要让玩家在这儿看得见什么，
        # 照管事那一对的写法**在同一格**再摆一个带互补 visible_flag="ch03.mo_siwang"
        # 的对象即可——同格两个各带互补旗标是合法写法（map_spec 规则 17），
        # 不在场的那个既不占格也不抢话。
        npc("npc_mo_daifu", 11, 9, "mo_daifu", "down", "ch03/mojuren.lua",
            visible_flag="ch03.mo_gui_gu", hidden_flag="ch03.mo_siwang"),

        # ---- 节点 4：尸虫丸（他手边；与 npc 不同格，免得压住其中一个） ----
        trigger("trigger_shichong", 13, 9, "ch03/shichong.lua", "interact", True,
                set_flag="ch03.shichong_wan"),

        # ---- 节点 9：解药（药柜前一排） ----
        # set_flag 用的是那个二选一旗标本身：用与不用都会把它置成 1 或 2，
        # 非零即「这一幕演过了」，本章没有另一个 jieyao_done 可用。
        trigger("trigger_jieyao", 16, 7, "ch03/jieyao.lua", "interact", True, w=3,
                set_flag="ch03.jieyao_xuan"),

        facility("facility_cunji", 7, 7, "save"),
    ]
    return build_map("ch03_mishi", W, H, "terrain_mishi",
                     map_props("ch03_mishi", "bgm_indoor", outdoor=False), g, objects)


# ---------------------------------------------------------------------------
# 地图 2：藏书处 32x24 —— 偷秘籍
#
# 「被发现的风险感」不靠脚本喊，靠两条路的长短差：
#
#   明路：门厅 →(x5 那道窄口)→ 阅览堂五排书架之间 → 内阁正门 → 书匣
#         最短 30 步左右，但**必经 trigger_mingdao**（once=false，可反复撞见）。
#   暗路：门厅 → 西廊北上 → 北廊东行 → 东廊南下 → 从内阁背后进去
#         最短 55 步上下，一个触发都不碰。
#
#   两条都到得了同一只书匣，差的只是「快」与「稳」—— 取舍摆在地上，
#   不需要弹一个「你要走哪条」的选单。窄口 x5 那三格是明路的咽喉，
#   暗路从门厅北端的西廊上去，绕得开它。
#
# 挂点：节点 6 偷秘籍 → trigger_miji（内阁书匣，interact + once）
# ---------------------------------------------------------------------------
def make_cangshu():
    W, H = 32, 24
    g = Grid(W, H)
    rnd = random.Random(302)
    g.fill("ground", 1)
    g.fill("building", 5)
    g.fill("collision", 1)

    # 门厅（自密室的石门进来）
    road(g, 0, 11, 4, 13)

    # 明路：门厅 → 阅览堂的窄口
    road(g, 5, 11, 5, 13)

    # 阅览堂：五排书架，前后两条横巷（y=9 与 y=16）
    road(g, 6, 9, 23, 16, gid=1)
    for sx in (8, 11, 14, 17, 20):
        g.solid_rect(sx, 10, sx, 15, 8)
        g.rect("front", sx, 9, sx, 9, 4)   # 架顶压在人头上，走横巷时压过去

    # 暗路：西廊北上 → 北廊东行 → 东廊南下
    road(g, 2, 3, 3, 11)
    road(g, 2, 2, 28, 3)
    road(g, 27, 3, 28, 10)

    # 内阁：阅览堂的正门在西，东廊从背后的高窗下来
    road(g, 26, 11, 30, 15, gid=1)
    road(g, 24, 12, 25, 13)
    g.solid_rect(29, 12, 29, 13, 8)        # 书匣（秘籍在里头）

    g.rect("overlay", 6, 9, 23, 16, 6)
    eaves(g, rnd, 0.30)
    scatter(g, rnd, 60, 1, 1, W - 2, H - 2, grass=0.2)

    objects = [
        spawn("spawn_from_mishi", 2, 12, "right", default=True),
        portal("portal_to_mishi", 0, 12, "ch03_mishi", "spawn_from_cangshu", h=2),

        # ---- 明路的代价：撞见的风险 ----
        # once=false，所以**不写** set_flag（map_spec 4.4：once=false 留着
        # set_flag 会被判错，那是个引擎根本不去置的摆设）。
        # 这一格反复踩得响，正是「每走一次明路就赌一次」。
        trigger("trigger_mingdao", 5, 11, "ch03/cangshu_mingdao.lua", "enter", False, h=3),

        # ---- 节点 6：偷秘籍（内阁书匣） ----
        trigger("trigger_miji", 29, 12, "ch03/miji.lua", "interact", True, h=2,
                set_flag="ch03.miji_done"),
    ]
    return build_map("ch03_cangshu", W, H, "terrain_cangshu",
                     map_props("ch03_cangshu", "bgm_indoor", outdoor=False), g, objects)


# ---------------------------------------------------------------------------
# 地图 3：谷中暗道 40x32 —— 纵向，密室在东北头，谷外在南头
#
# 一条自上而下的穿山道，九段折返换成「窄 — 阔 — 窄」三拍：
#
#   东口（密室）→ 北廊 → 斜道 → 竖道一 →┬→ 岔室（云翅鸟；余子童的线索在这里浮出）
#                                         └→ 竖道二 → 石室（第二场战斗）→ 竖道三 → 南口（谷外）
#
# 通密室的口子在东：密室那道暗门开在它的西墙，从暗门往西走出来，就落在这里的东口、
# 面朝西（map_spec 规则 29）。口子曾开在北头（面朝南），出门往西、进门朝南，拐了 90°。
# 北头那段旧竖道留成死胡同（砌死的话，存档正落在那几格上的人会被关在石头里）。
#
# 石室是全章唯一一处宽敞地方（17x8），战斗要地形，落石与石棺就摆在里头。
# 岔室是条死路：进去只为看那只云翅鸟，看完原路退出来 —— 死路在这里是对的，
# 玩家会记住「那边关着什么东西」。
#
# 挂点：
#   节点 8 上半 云翅鸟   → trigger_yunchi（岔室鸟架前，interact + once）
#   节点 8 下半 暗道一战 → trigger_andao_zhan（石室北口，enter + once）
#   节点 12 曲魂入队     → trigger_quhun（石室西壁的石棺旁，interact + once）
#   节点 12 三大铁则     → trigger_tiezhe（南口内侧，interact + once）
# ---------------------------------------------------------------------------
def make_andao():
    W, H = 40, 32
    g = Grid(W, H)
    rnd = random.Random(303)
    g.fill("ground", 1)
    g.fill("building", 5)
    g.fill("collision", 1)

    road(g, 19, 1, 21, 4)                  # 旧北口：如今是一截死胡同，加宽成三格免得被画成门龛（同密室）
    road(g, 21, 5, W - 2, 6)               # 北廊：自东口（密室的暗门）进来，往西接斜道
    gate(g, W - 1, 5, W - 1, 6)            # 东口
    road(g, 12, 5, 20, 6, gid=3)           # 斜道（西行）
    road(g, 12, 6, 13, 11)                 # 竖道一
    road(g, 13, 11, 24, 12)                # 岔道口（东行）
    road(g, 24, 8, 29, 13, gid=1)          # 岔室（死路，云翅鸟）
    road(g, 13, 12, 14, 17, gid=3)         # 竖道二
    road(g, 8, 17, 24, 24, gid=1)          # 石室（战场）
    road(g, 19, 24, 20, 29, gid=3)         # 竖道三
    road(g, 19, 29, 20, 31)                # 南口：出谷外

    # 岔室里的鸟架与石笼
    g.solid_rect(26, 9, 27, 9, 8)
    g.solid_rect(28, 10, 28, 11, 8)

    # 石室里的地形：石棺一具、落石两处
    g.solid_rect(10, 19, 11, 19, 8)
    g.solid_rect(16, 21, 16, 21, 8)
    g.solid_rect(21, 20, 21, 20, 8)

    g.rect("overlay", 8, 17, 24, 24, 7)
    eaves(g, rnd, 0.46)

    objects = [
        spawn("spawn_from_mishi", W - 2, 5, "left", default=True),
        spawn("spawn_from_guwai", 19, 29, "up"),

        portal("portal_to_mishi", W - 1, 5, "ch03_mishi", "spawn_from_andao", h=2),
        portal("portal_to_guwai", 19, 31, "ch03_guwai", "spawn_from_andao", w=2),

        # ---- 节点 8 上半：云翅鸟（岔室，死路尽头） ----
        trigger("trigger_yunchi", 26, 10, "ch03/yunchi.lua", "interact", True, w=2,
                set_flag="ch03.yuzitong_lu"),

        # ---- 节点 8 下半：暗道一战（石室北口，下来必踩） ----
        trigger("trigger_andao_zhan", 13, 17, "ch03/andao_zhan.lua", "enter", True, w=2,
                set_flag="ch03.andao_zhan"),

        facility("facility_cunji", 9, 23, "save"),

        # ---- 节点 12：曲魂入队 ----
        trigger("trigger_quhun", 10, 20, "ch03/quhun_rudui.lua", "interact", True, w=2,
                set_flag="ch03.quhun_rudui"),
        # ---- 节点 12：三大铁则（临出口那一段路上说完） ----
        trigger("trigger_tiezhe", 19, 27, "ch03/tiezhe.lua", "interact", True, w=2,
                set_flag="ch03.tiezhe_zhi"),
    ]
    return build_map("ch03_andao", W, H, "terrain_andao",
                     map_props("ch03_andao", "bgm_cave", outdoor=False), g, objects)


# ---------------------------------------------------------------------------
# 地图 4：谷外 40x30 —— 第一场仗的战场，也是章末离谷的出口
#
# 谷西豁口出来是一条穿岩夹道（东段，两格宽、十五格长），夹道尽头豁然开阔，
# 便是荒坡。教学第一场就打在这片坡上：
#
#   · 夹道西口横着 trigger_elang（enter + once），走出夹道必踩 —— 第一场仗
#     是本章的开场，不能让玩家绕过去；
#   · 坡上零散树与岩，是战棋时代教学要教的「地形」；横版战斗不取地图上的
#     站位，当年框这片坡的 battle_anchor 已于 2026-09-29 删除。
#
# 北面崖根一道石缝是暗道的南口（ch03.andao_zhan 之后才认得出来）；
# 南面林子里一条山口是章末离谷的去处 —— 它不通向任何地图，是条死路，
# 走到头触发章末，符合「离谷」这件事在本章之内不再回来。
# ---------------------------------------------------------------------------
def make_guwai():
    W, H = 40, 30
    g = Grid(W, H)
    rnd = random.Random(304)
    g.fill("ground", 1)
    border(g)

    # 东段：穿岩夹道。南北两面立起岩壁，只留两格宽的路
    g.solid_rect(24, 1, 38, 13, 5)
    g.solid_rect(24, 16, 38, 28, 5)
    gate(g, 39, 14, 39, 15)                # 东口 → 神手谷谷西豁口
    road(g, 24, 14, 38, 15)

    # 坡上横径：接夹道西口
    road(g, 12, 14, 23, 15)

    # 北面小径 → 暗道南口
    gate(g, 6, 0, 7, 0)
    road(g, 6, 1, 7, 13)
    road(g, 6, 13, 12, 14)

    # 南面林子，中间留一条山口（离谷之路；南端仍是边界，走到头是死路）
    g.solid_rect(1, 26, 18, 28, 5)
    g.solid_rect(21, 26, 38, 28, 5)
    road(g, 19, 20, 20, 28, gid=3)

    # 坡上草木与岩石 —— 教学第一场要教地形，这些就是地形
    for tx, ty in ((3, 4), (9, 6), (15, 5), (21, 3), (16, 10),
                   (4, 18), (9, 19), (15, 19), (22, 21), (3, 23), (12, 24)):
        tree(g, tx, ty)
    for rx, ry in ((11, 16), (13, 21), (18, 18), (7, 22)):
        g.solid_rect(rx, ry, rx, ry, 8)

    scatter(g, rnd, 170, 1, 1, W - 2, H - 2)

    objects = [
        spawn("spawn_from_shenshougu", 38, 14, "left", default=True),
        spawn("spawn_from_andao", 6, 1, "down"),

        portal("portal_to_shenshougu", 39, 14, "ch01_shenshougu",
               "spawn_from_ch03_guwai", h=2),
        # 打过暗道那一仗才认得出崖根这道石缝 —— 反过来（自密室下暗道）不设闸门，
        # 所以这条路第一次一定是从里往外走的。
        portal("portal_to_andao", 6, 0, "ch03_andao", "spawn_from_guwai", w=2,
               require_flag="ch03.andao_zhan", deny_text_key="ch03.block.andao"),

        # ---- 节点 1：谷外遇狼（战棋教学一；出夹道必踩） ----
        trigger("trigger_elang", 23, 14, "ch03/elang.lua", "enter", True, h=2,
                set_flag="ch03.elang_done"),

        # ---- 节点 12：离谷（山口尽头） ----
        trigger("trigger_ligu", 19, 26, "ch03/ligu.lua", "interact", True, w=2,
                set_flag="ch03.done"),

        # ---- 野外遭遇：整片荒坡（夹道以西、林子以北）。教学那一仗打完才有野兽出没 ----
        # 夹道与西口那格 trigger_elang 不在区里：教学第一场必须是剧情那一场。
        encounter("encounter_huangpo", 1, 1, 22, 25, "encounter_ch03_guwai", 18, 40,
                  require_flag="ch03.elang_done"),
    ]
    return build_map("ch03_guwai", W, H, "terrain_guwai",
                     map_props("ch03_guwai", "bgm_mountain_path"), g, objects)


# ---------------------------------------------------------------------------
# 神手谷：本章的两处口子 —— 就地修补，不重建（原委见本文件第四节）
# ---------------------------------------------------------------------------
def patch_shenshougu(payload=None):
    """给神手谷补上本章的两处口子。幂等，补过的图再补一次不变。

    payload 为 None 时读 maps/ch01_shenshougu.tmj（本脚本自己跑的路径）；
    genmaps.py 会在第 2 章那处补丁之后把手上那一份传进来，免得读到旧盘上的图。
    追加顺序必须稳定（先谷外、后密室、最后那个触发器），否则两边的对象表
    排序不同，`--check` 会报一堆「只在 maps/ 里有」的假漂移。
    """
    if payload is None:
        path = os.path.join(genmaps.MAPS, "ch01_shenshougu.tmj")
        payload = json.loads(open(path, "rb").read().decode("utf-8"))
    W = payload["width"]
    layers = {l["name"]: l for l in payload["layers"]}

    def put(layer, x, y, value):
        layers[layer]["data"][y * W + x] = value

    # 一、谷西豁口：谷中横径原本止于 x=4，往西凿穿两格厚的谷壁通到荒坡。
    for y in (13, 14):
        for x in (0, 1):
            put("building", x, y, 0)
            put("collision", x, y, 0)
        for x in (0, 1, 2, 3):
            put("ground", x, y, 2)

    # 二、堂屋南墙那处门洞：下去便是密室。只开一格 —— 一个人侧身进出的样子，
    # 才像一处瞒了四年的门，而不是一道正门。
    put("ground", 8, 9, 2)
    put("building", 8, 9, 0)
    put("collision", 8, 9, 0)

    objs = layers["objects"]["objects"]
    names = {o["name"] for o in objs}

    if "spawn_from_ch03_guwai" not in names:
        objs.append(spawn("spawn_from_ch03_guwai", 2, 13, "right"))
    if "portal_to_ch03_guwai" not in names:
        objs.append(portal("portal_to_ch03_guwai", 0, 13, "ch03_guwai",
                           "spawn_from_shenshougu", h=2,
                           require_flag="ch02.done",
                           deny_text_key="ch03.block.guwai"))
    if "spawn_from_ch03_mishi" not in names:
        objs.append(spawn("spawn_from_ch03_mishi", 8, 10, "down"))
    if "portal_to_ch03_mishi" not in names:
        objs.append(portal("portal_to_ch03_mishi", 8, 9, "ch03_mishi",
                           "spawn_from_shenshougu",
                           require_flag="ch03.mo_gui_gu",
                           deny_text_key="ch03.block.mishi"))
    # 节点 2「墨大夫归谷」：他是**走进谷里**的。这一格在谷中南北主径的北段，
    # 自谷外回来、往北去药圃与居所必经，他从南边谷口跟进来，咳嗽声正落在韩立背后
    # （ch03.mogui.cough）。设计文档第 5 节把它记在「居所」名下 ——
    # 居所（ch02_jusuo）由 genmaps_ch02.make_jusuo() 从零重建，往它的 tmj 里
    # 塞对象会让 `genmaps_ch02.py --check` 报漂移，而那个文件不在本次白名单里。
    # 这一处不只是权宜：ch03.mo_gui_gu 是密室那道门的钥匙，规则 19 要求它
    # 静态拿得到，挂在一个尚未写出的脚本上等于那道门永远开不了。
    if "trigger_mogui" not in names:
        objs.append(trigger("trigger_mogui", 19, 8, "ch03/mogui.lua", "enter", True, w=2,
                            guard_flag="ch03.elang_done",
                            set_flag="ch03.mo_gui_gu"))

    for i, o in enumerate(objs, 1):
        o["id"] = i
    payload["nextobjectid"] = len(objs) + 1
    return payload


# ---------------------------------------------------------------------------
def build_all():
    """本章四张图的完整产出，外加神手谷那两处就地修补的口子。"""
    return {
        "ch03_mishi": make_mishi(),
        "ch03_cangshu": make_cangshu(),
        "ch03_andao": make_andao(),
        "ch03_guwai": make_guwai(),
        "ch01_shenshougu": patch_shenshougu(),
    }


TILESETS = ("terrain_mishi", "terrain_cangshu", "terrain_andao", "terrain_guwai")


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
