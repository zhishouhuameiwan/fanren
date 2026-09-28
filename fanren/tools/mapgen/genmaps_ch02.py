# -*- coding: utf-8 -*-
"""第 2 章地图生成器：重跑即重建下列四张图与四个 tileset，并给神手谷补一处北口。

    python tools/mapgen/genmaps_ch02.py
    python tools/mapgen/genmaps_ch02.py --check   # 只比对，不写盘；有漂移则退出码 1
    python tools/validate.py --maps               # 自检

    （`genmaps.py --check` 会把本章这四张也一并盯上，所以 build.bat 里只有那一条。）

产出（全部写进 maps/，不经手 scripts/ 与 data/text/）：

    maps/ch02_yaopu.tmj          40x30  神手谷药圃（本章主场兼枢纽）
    maps/ch02_jusuo.tmj          24x18  谷中居所（一屋一院）
    maps/ch02_yabi.tmj           32x36  谷中崖壁（纵向山路）
    maps/ch02_wairentang.tmj     48x36  七玄门外刃堂
    maps/tilesets/terrain_{yaopu,jusuo,yabi,wairentang}.tsj

    另就地修补 maps/ch01_shenshougu.tmj：北面开一处谷口通向 ch02_yaopu。
    这一处是「修补」不是「重建」：神手谷的地形归 genmaps.py 所有，本脚本只改
    它需要改的那几格与追加那两个对象，其余原样写回。谷口本身归本脚本所有，
    genmaps.py 的 build_all() 会在重建完神手谷之后回头调用 patch_shenshougu()
    把它补回去 —— 两边单跑都不会把第 2 章从第 1 章割断。

    （曾经会：第 1 章六张图的 require_flag / deny_text_key 一度只存在于 tmj 里，
    genmaps.py 那边没有，重跑就是一次静默的数据损毁。闸门现已回到 genmaps.py
    的对象表，由 `python tools/mapgen/genmaps.py --check` 盯住。）

--------------------------------------------------------------------------
一、空间关系（先想清楚再摆，第 1 章在这上面栽过一次）

    神手谷是七玄门内墨大夫的医馆兼居所（docs/lore/势力与地理.md：越国镜州
    彩霞山，七玄门内部设施），是一条谷，不是独立山头。谷里自南向北：

        谷口（ch01_shenshougu，第 1 章已有，南出接炼骨崖 → 七玄门）
          │  北
          ▼
        药圃（ch02_yaopu，南口进）──西──▶ 居所（ch02_jusuo，谷西坡上那一屋一院）
          │  └────────东──▶ 崖壁（ch02_yabi，谷东壁，西口进，山路盘上去，尽头是望谷的崖顶）
          │
          └──谷北小道（出谷后门）──▶ 七玄门外刃堂（ch02_wairentang，谷外，走半日）

    每一处都「出门往哪走，进门就朝哪」（map_spec 规则 29）：神手谷北口出去落在药圃
    南口、面朝北；药圃东口出去落在崖壁西口、面朝东。曾经不是：神手谷与炼骨崖两头都在
    北边（掉头，按住「上」来回弹），药圃东口出去却落在崖脚南口、面朝北（拐了 90°）。
    于是神手谷的谷口挪到了它的南墙，药圃通神手谷的口子从北墙挪到了南墙，
    崖壁的入口从崖脚南口挪到了西口。

    崖壁只跟药圃相通，是一条上去又下来的路：崖顶的view是爬上去的由头，
    拾瓶就在这条路的中段（崖脚到崖顶最短 74 步，拾瓶带在第 35 步；西口进来到崖脚
    另有十来步）——玩家是冲着崖顶去的，瓶子是半道上踢到的，与原著「山路上意外拾得」一致。
    外刃堂在谷外，门开在药圃的**北**边：谷的正口在南，往北是出谷的后路，
    不会出现「谷里冒出个宗门院子」的怪事。

二、防跳过

    段与段之间靠出口的 require_flag + deny_text_key 拦（第 1 章的教训：引擎
    支持这两个属性，六张图一处没用）：

        出口               require_flag             拦的是
        神手谷 → 药圃      ch01.done                第 1 章没打完就进第 2 章
        药圃 → 居所        ch02.renyao_done         没上过入谷第一课就去打坐
        药圃 → 外刃堂      ch02.dazuo_done          没开始修炼就去卖药（节点 3）
        药圃 → 崖壁        ch02.duan2_start         段一就跑去把瓶子捡了

    段内的先后次序不靠地图拦，靠脚本自己：scripts/ch02/ 下每个节点脚本开头都
    先查前置旗标，不满足就说一句「还不到时候」再 return（如 ch02.dazuo.gate、
    ch02.caiyao.gate_sit）。这比地图上的 guard_flag 好：被拦住的玩家听得到
    自己还差什么，而不是按了没反应。

三、once 怎么定（这一条是和编剧的接口，写错会造成死局）

    once=true 的触发器**必须**写 set_flag，且必须指向脚本自己在末尾置的那个完成
    旗标（renyao_done / shiping_done / chousui_jian）。引擎判「这一幕演过没有」
    看的就是这个旗标：脚本半路 return 根本走不到那一行，于是触发器留着，玩家
    条件满足后走回来还能再触发。作者不用记别的规矩——照常在末尾置完成旗标即可。
    漏写 set_flag 的 once 会退化成可重复触发（响亮，不是死局），
    tools/validate.py 的 once 契约检查会在门禁上拦住。

    这条曾经反过来：引擎在**跑脚本之前**就把记号打上（markTriggered 先于
    startEvent），于是任何会中途 return 的脚本，一被踩到就把 once 永久烧掉。
    当时只能靠「凡会提前 return 的脚本一律不设 once」这条口头约定绕开，
    而口头约定必然失效。引擎已改，约定作废，本节按新口径写。

    段内的先后次序仍然不靠地图拦、靠脚本自己查前置（见上一节），
    所以本章多数触发是 once=false；三处 once=true 的是不可重演的剧情节点：

      · trigger_renyao   —— 入谷第一课，set_flag=ch02.renyao_done
      · trigger_shiping  —— 山路拾瓶，按设计只能有这一回，set_flag=ch02.shiping_done
      · trigger_chousui  —— 抽髓丸之祸，set_flag=ch02.chousui_jian；
                            它前置不满足时是**静默** return（连话都不说），
                            所以仍挂 guard_flag=ch02.duan3_start 把它挡在段三之前
                            ——那是为了别让玩家撞上一个一声不吭的格子，
                            不再是为了防止被空烧

四、瓦片 gid 约定（沿用 genmaps.py 那一套，四个 tileset 统一编号）

    1 主地表  2 道路  3 特殊地表（翻土/碎石/石阶）  4 front 遮挡
    5 building 主体（墙体/岩壁）  6 overlay 花草  7 overlay 碎石
    8 building 次体（篱笆/摊架/家具）

不变量：building 有实体瓦片处 collision 必须非 0，一律走 Grid.solid_rect。
"""
import json
import os
import random
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

# 对象小工具连同闸门参数一并住在 genmaps.py 里，本章原样沿用 —— 曾经这里各备
# 过一版带闸门的 portal/trigger/npc，因为那边的不带；那份重复现已并回去。
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


def map_props(map_id, bgm, outdoor=True):
    return [
        ("map_id", map_id),
        ("display_name_key", "ch02.map.%s.name" % map_id[len("ch02_"):]),
        ("region", "qingzhou"),       # 与第 1 章六张图一致
        ("bgm", bgm),
        ("chapter", 2),
        ("outdoor", outdoor),
        # can_leave_edge 一律 false：TileMapLoader 只校验它的存在与类型，并不写回
        # TileMap，引擎里没有任何地方消费它（src/io/TileMapLoader.cpp 的注释已自陈
        # 这一点）。写 true 只会得到一个不生效的属性，所以不写。
        ("can_leave_edge", False),
    ]


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


def road(g, x0, y0, x1, y1):
    """铺一条路：清掉实体 + 地表换成道路。"""
    g.open_rect(x0, y0, x1, y1)
    g.rect("ground", x0, y0, x1, y1, 2)


def tree(g, x, y):
    """一棵树：树干占一格，树冠三格见方压在 front 层（不挡路）。"""
    g.rect("front", x - 1, y - 1, x + 1, y + 1, 4)
    g.solid_rect(x, y, x, y, 5)


def scatter(g, rnd, count, x0, y0, x1, y1, grass=0.7):
    """在可通行的空地上撒花草与碎石，只动 overlay，不影响通行。"""
    for _ in range(count):
        x = rnd.randrange(x0, x1 + 1)
        y = rnd.randrange(y0, y1 + 1)
        if g.walkable(x, y) and g.get("overlay", x, y) == 0 and g.get("ground", x, y) != 2:
            g.set("overlay", x, y, 6 if rnd.random() < grass else 7)


# ---------------------------------------------------------------------------
# 地图 1：神手谷药圃 40x30 —— 本章主场兼枢纽，四个方向各通一处
#
# 谷底一块平地。西北角用篱笆围出药圃本体（八畦翻土 + 灵田），只开东面一道栅门；
# 东北是晾药棚，棚下一张药案、一盘石碾；南半边是野地、水潭与兔笼。
# 四道口子：南中回神手谷北口、北东出谷去外刃堂、西去居所、东去崖壁。
#
# 南口内侧收成两格宽的门洞（x18-19 / y28，两侧各砌一格墙）：节点 1 是踏入型触发，
# 摆在门洞里侧那一排，只有卡在咽喉上才靠得住。第 1 章吃过「触发器能绕开」的亏。
# （口子原在北墙，里侧是一段 y1-4 的夹道；口子挪到南墙之后那段夹道拆了，
# 墙拆成空地而不是把空地砌成墙——旧存档可能正站在那几格上。）
#
# 各处挂点按 scripts/ch02/*.lua 头部自陈的位置摆：
#   renyao  → 进场夹道（enter, once）   caiyao → 晾药棚旁
#   zhitong → 晾药棚石碾旁              shiyao → 空地（兔笼）
#   cuishu  → 东头那畦                  guanshi / modaifu → npc
#   ch03/beidu → 灵田东侧那道畦背（第 3 章节点 7，见下面那段注释）
# ---------------------------------------------------------------------------
def make_yaopu():
    W, H = 40, 30
    g = Grid(W, H)
    rnd = random.Random(606)
    g.fill("ground", 1)
    border(g)

    # 四处门洞
    gate(g, 18, H - 1, 19, H - 1)   # 南中 → 神手谷北口（ch01_shenshougu）
    gate(g, 33, 0, 34, 0)       # 北东 → 出谷小道（ch02_wairentang）
    gate(g, 0, 13, 0, 14)       # 西   → 居所（ch02_jusuo）
    gate(g, 39, 21, 39, 22)     # 东   → 崖壁（ch02_yabi）

    # 谷中小径
    road(g, 18, 5, 19, H - 2)   # 主径：自南口一路向北，过横径到灵田栅门
    road(g, 1, 13, 38, 14)      # 横径：贯通东西，接西门与东侧下行道
    road(g, 33, 1, 34, 13)      # 出谷小道：自北东门下来接横径
    road(g, 37, 14, 38, 22)     # 东侧下行道：接崖壁门

    # 南口门洞：两侧各砌一格墙，出门洞必过节点 1 的触发区
    g.solid_rect(17, H - 2, 17, H - 2, 5)
    g.solid_rect(20, H - 2, 20, H - 2, 5)

    # 药圃本体：篱笆围起 x2..16 / y2..12，东面开一道栅门
    g.solid_rect(2, 2, 16, 2, 8)
    g.solid_rect(2, 12, 16, 12, 8)
    g.solid_rect(2, 2, 2, 12, 8)
    g.solid_rect(16, 2, 16, 12, 8)
    gate(g, 16, 7, 16, 8)                 # 栅门（东头畦口）
    road(g, 17, 7, 17, 8)                 # 栅门到主径的短接

    # 八畦翻土（可踩，只是地表不同）；灵田设施摆在当中那道田埂上
    for y0, y1 in ((3, 5), (7, 9), (11, 11)):
        g.rect("ground", 3, y0, 15, y1, 3)
        g.rect("overlay", 3, y0, 15, y1, 6)

    # 晾药棚：谷东北的敞棚。棚檐压在 front 层，棚前一张药案、一盘石碾
    g.solid_rect(23, 4, 30, 7, 5)
    g.rect("front", 23, 3, 30, 3, 4)
    g.solid_rect(27, 8, 28, 8, 8)         # 石碾（节点 8 配药就在这儿）

    # 兔笼：南头空地上，段五试药用的那几只
    g.solid_rect(21, 17, 22, 17, 8)

    # 水潭：不画墙，只挡路（与第 1 章神手谷同一处理）
    g.rect("ground", 3, 23, 9, 27, 3)
    g.rect("collision", 3, 23, 9, 27, 1)

    # 谷中草木
    for tx, ty in ((24, 20), (30, 17), (13, 22), (34, 27), (25, 27), (7, 17), (28, 24), (16, 26)):
        tree(g, tx, ty)
    scatter(g, rnd, 150, 1, 1, W - 2, H - 2)

    objects = [
        # ---- 出入口 ----
        spawn("spawn_from_shenshougu", 18, H - 2, "up", default=True),
        spawn("spawn_from_jusuo", 1, 13, "right"),
        spawn("spawn_from_yabi", 38, 21, "left"),
        spawn("spawn_from_wairentang", 33, 2, "down"),

        portal("portal_to_shenshougu", 18, H - 1, "ch01_shenshougu", "spawn_from_ch02_yaopu", w=2),
        portal("portal_to_jusuo", 0, 13, "ch02_jusuo", "spawn_from_yaopu", h=2,
               require_flag="ch02.renyao_done", deny_text_key="ch02.block.jusuo"),
        portal("portal_to_wairentang", 33, 0, "ch02_wairentang", "spawn_from_yaopu", w=2,
               require_flag="ch02.dazuo_done", deny_text_key="ch02.block.wairentang"),
        portal("portal_to_yabi", 39, 21, "ch02_yabi", "spawn_from_yaopu", h=2,
               require_flag="ch02.duan2_start", deny_text_key="ch02.block.yabi"),

        # ---- 节点 1：入谷第一课（段一进场；南口门洞里侧那一排，出门洞必踩） ----
        # set_flag 指向 renyao.lua 末尾置的那个完成旗标——once 就是靠它兑现的。
        trigger("trigger_renyao", 18, H - 3, "ch02/renyao.lua", "enter", True, w=2,
                set_flag="ch02.renyao_done"),

        # ---- 灵田 ----
        facility("facility_lingtian", 8, 6, "field", w=2, h=2,
                 ref_id="shenshougu_yaopu", slots=8),
        facility("facility_cunji", 20, 15, "save"),

        # ---- 节点 3 上半：采第一株药（晾药棚旁） ----
        trigger("trigger_caiyao", 23, 8, "ch02/caiyao.lua", "interact", False, w=2),

        # ---- 节点 8：配止痛药（晾药棚下的石碾旁，本章唯一的三选一） ----
        trigger("trigger_zhitong", 27, 8, "ch02/zhitong.lua", "interact", False, w=2),

        # ---- 节点 10 上半：试药兔死、绿液误洒（南头空地的兔笼） ----
        trigger("trigger_shiyao", 21, 17, "ch02/shiyao.lua", "interact", False, w=2),

        # ---- 节点 10 下半 + 节点 11 上半：发现催熟、攒满一瓶浇同一株（东头那畦） ----
        trigger("trigger_cuishu", 14, 4, "ch02/cuishu.lua", "interact", False, w=2, h=2),

        # ---- 第 3 章节点 7：备毒（灵田东侧那道畦背） ----
        # 这一节从头到尾是药圃的戏：卯时畦头、第七畦的畦背、管事按畦记的账、
        # 东头两畦的土色（ch03.beidu.ledger / opt_one / opt_two / two1 / two2）。
        # 它从前挂在 ch03_mishi 里紧挨着 portal_to_andao 的那两格——机制上说得通
        # （ch03.beidu_done 正是那道门的 require_flag），可那是一间石屋，屋里
        # 既没有畦也没有管事。裁决是挪地图不改文案：设计第 5 节节点 7 写的就是
        # 「药圃/密室」，而这一整节是药圃。
        #
        # 摆在 (10,6)-(11,6)：灵田（facility_lingtian，八畦，(8,6) 起两格见方）
        # 紧东边那道田埂，正是「第七畦的畦背」，与 opt_two 说的东头那两畦
        # （trigger_cuishu 那一带）隔着几格，两处说的不是同一块地。
        #
        # 不挂 guard_flag，与同样落在第 2 章图上的 trigger_yingdui /
        # trigger_chujue 一个口径：交互型触发要玩家自己走过去按确认，撞不上；
        # 前置由 beidu.lua 开头查 ch03.miji_done 再 return 来拦，被拦住的玩家
        # 听得见自己还差什么。踏入型的那一处（trigger_duoshe）才需要地图来拦。
        trigger("trigger_beidu", 10, 6, "ch03/beidu.lua", "interact", True, w=2,
                set_flag="ch03.beidu_done"),

        # ---- 人 ----
        # 管事有两副面孔，按章分。同一个 npc 对象只挂得住一个 script，
        # 所以第 3 章那副另起一个对象，**摆在同一格**：占格本来就照占（隐身
        # 的 NPC 也是一堵墙），同格就不会凭空多出一道看不见的墙；
        # 规则 17 的同格遮蔽对带旗标的 NPC 本就豁免，这正是它留出来的写法。
        npc("npc_yaopu_guanshi", 17, 9, "yaopu_guanshi", "right", "ch02/guanshi.lua",
            hidden_flag="ch03.mo_gui_gu"),
        npc("npc_yaopu_guanshi_ch03", 17, 9, "yaopu_guanshi", "right", "ch03/guanshi.lua",
            visible_flag="ch03.mo_gui_gu"),
        # 段五一开场师父就不再搭话了（modaifu.lua 开头查 ch02.xiangqi_ping 就静默
        # return），所以这里干脆让他不在场，免得玩家对着一个一声不吭的人按半天。
        # 第 1 章授完口诀（ch01.done）他才从神手谷堂前挪到这儿：同一个人一时只在一处。
        npc("npc_mo_daifu", 12, 17, "mo_daifu", "up", "ch02/modaifu.lua",
            visible_flag="ch01.done", hidden_flag="ch02.xiangqi_ping"),
    ]
    return build_map("ch02_yaopu", W, H, "terrain_yaopu",
                     map_props("ch02_yaopu", "bgm_valley"), g, objects)


# ---------------------------------------------------------------------------
# 地图 2：谷中居所 24x18 —— 一屋一院
#
# 东门进来是院子，院西那道墙上一处门洞通屋里。屋里两张床（韩立与张铁同住一屋，
# 第 1 章已定下的改编）、一领蒲团、一张桌、墙角一只衣箱。
#
# 五处挂点按 scripts/ch02/*.lua 头部自陈的位置摆：
#   dazuo / ceng2   → 打坐处旁（蒲团东西两侧各一处）
#   zaping          → 桌边
#   kaigai          → 院中空地（水缸边；第八日瓶盖是在院子里开的）
#   cangping/ceng3  → 床边（床头一处、床前一处）
# 张铁的闲聊也挂在这张图（zhangtie.lua 自陈 role_id=zhang_tie、在 ch02_jusuo）。
# ---------------------------------------------------------------------------
def make_jusuo():
    W, H = 24, 18
    g = Grid(W, H)
    rnd = random.Random(707)
    g.fill("ground", 1)
    border(g)

    gate(g, 23, 8, 23, 9)                 # 院门 → 药圃

    # 屋墙：x=13 那道墙把屋（西）与院（东）分开，只开一处屋门
    g.solid_rect(13, 1, 13, 16, 5)
    g.open_rect(13, 8, 13, 9)

    # ---- 屋里（西，x1..12）----
    g.solid_rect(2, 2, 4, 4, 5)           # 韩立的床
    g.rect("front", 2, 1, 4, 1, 4)        # 帐幔
    g.solid_rect(9, 2, 11, 4, 5)          # 张铁的床
    g.rect("front", 9, 1, 11, 1, 4)
    g.solid_rect(6, 13, 8, 14, 8)         # 桌
    g.solid_rect(1, 11, 2, 12, 8)         # 衣箱
    g.rect("ground", 1, 8, 12, 9, 2)      # 屋里的过道

    # ---- 院里（东，x14..22）----
    g.rect("ground", 14, 8, 22, 9, 2)     # 院中石板路，直通屋门
    g.solid_rect(17, 6, 18, 7, 8)         # 水缸
    g.solid_rect(20, 12, 21, 13, 8)       # 晒药架
    g.rect("overlay", 14, 10, 22, 16, 6)  # 院中草

    scatter(g, rnd, 40, 1, 1, W - 2, H - 2, grass=0.3)

    objects = [
        spawn("spawn_from_yaopu", 22, 8, "left", default=True),
        portal("portal_to_yaopu", 23, 8, "ch02_yaopu", "spawn_from_jusuo", h=2),

        # 打坐处与它两侧的两处挂点（段一第一次打坐、段四口诀第二层）
        facility("facility_dazuo", 6, 6, "meditate", w=2, h=2),
        trigger("trigger_dazuo", 8, 6, "ch02/dazuo.lua", "interact", False, h=2),
        trigger("trigger_ceng2", 5, 6, "ch02/ceng2.lua", "interact", False, h=2),

        # 节点 5 上半：砸不开、泡不开（桌边）
        trigger("trigger_zaping", 6, 12, "ch02/zaping.lua", "interact", False, w=3),
        # 节点 5 下半：第八日瓶盖自开（院中水缸边）
        trigger("trigger_kaigai", 17, 8, "ch02/kaigai.lua", "interact", False, w=2),

        # 节点 6：藏起来（床头）；段五口诀第三层、翻皮袋想起那只瓶子（床前）
        trigger("trigger_cangping", 5, 3, "ch02/cangping.lua", "interact", False),
        trigger("trigger_ceng3", 2, 5, "ch02/ceng3.lua", "interact", False, w=3),

        # 第 2 章张铁与他比邻而居（ch02.liao_zhangtie 的登记原话），此前他在炼骨崖、
        # 此后他已被炼成曲魂（第 3 章），所以只在第 2 章在场。原著 ch15 交代他在四年里的
        # 第二年、练成象甲功第三层时就留书出走了；本作让他待满这一章是第 2 章既定的改编
        # （路径行动 zhangtie_dating2 到 ch02.done）。
        npc("npc_zhang_tie", 10, 6, "zhang_tie", "down", "ch02/zhangtie.lua",
            visible_flag="ch01.done", hidden_flag="ch02.done"),

        # 第 3 章借用本图的三处挂点。
        #
        # 为什么落在第 2 章的图上：设计文档把节点 5（怎么应对墨居仁）、
        # 10（夺舍·识海之战）、11（身醒敌亡与处决余子童）都放在韩立自己屋里，
        # 而这三场戏本来就该发生在他睡觉的地方——夺舍是趁他睡着动手的。
        # 另起一张「第 3 章的居所」会让玩家在同一个谷里见到两间一样的屋子。
        #
        # 为什么写在这里而不是 genmaps_ch03.py：make_jusuo() 是**整张重建**的，
        # 第 3 章那边再往 tmj 里塞对象会让本文件的 --check 当场报漂移。
        # 神手谷是另一回事——那边是读盘再补，所以第 3 章能安全地再叠一层。
        #
        # 三条都是 once=true 且写了 set_flag：按 map_spec 4.4 的新口径，
        # 「用过了」看的是脚本自己在演完之后置的那个旗标，所以脚本开头
        # 「还不到时候」地提前 return 不会把触发器烧掉，第 2 章的玩家踩到也无妨。
        #
        # ---------------------------------------------------------------
        # 两种杀的操作感，全章的题眼。**这两行的 mode 不是随手填的。**
        # ---------------------------------------------------------------
        # 大纲注解与设计第 0.3 节把本章的道德分量全押在这一处对比上：
        #
        #   节点 10 夺舍  —— 墨居仁趁韩立**睡着**动手。玩家不该按下任何一个键，
        #                    所以是 enter：走到床边那一排就发生，没有「要不要开始」
        #                    这个问题。duoshe.lua 一次 choice() 也没有，同一个道理。
        #   节点 11 处决  —— 韩立醒着，自己走到墙角、自己按确认。所以是 interact：
        #                    玩家必须专门走过去、面朝它、按下那一下，
        #                    ch03.chujue.after「这一回是他自己走过去、自己叫的名字、
        #                    自己按下的拇指」说的就是这一下。
        #
        # 这两行曾经**恰好装反**（duoshe=interact / chujue=enter），
        # 两个脚本的首部却一直写着正确的那一半，于是最要害的设计论证建立在了
        # 一个不成立的前提上；更糟的是 Ch03SliceTests 按装反后的样子驱动，
        # 把错的那一半钉成了绿灯。改这两个字符串的同时要改测试的驱动方式，
        # 并由 Ch03TriggerModeTests 直接钉住这两个属性本身。
        trigger("trigger_yingdui", 2, 6, "ch03/yingdui.lua", "interact", True, w=3,
                set_flag="ch03.yingdui_xuan"),
        trigger("trigger_duoshe", 9, 5, "ch03/duoshe.lua", "enter", True, w=3,
                set_flag="ch03.shihai_done"),
        # 处决那一幕在**屋子西北角**，不在过道上。
        #
        # 它从前摆在 (14,8)——东西两半之间那道一格宽的门洞，四周一堵墙也没有。
        # 两处不对：
        #   · 文案写的是「墙角有个东西在往阴影里缩，缩得很急，像是怕他看见」。
        #     缩成一团的东西不会躲在天天有人过的门洞里。
        #   · 更要紧的是戏理。处决是本章唯一一件**玩家主动做**的杀
        #     （另一件是他睡着时发生的），所以它该让玩家特意走过去，
        #     而不是在必经之路上撞见。摆在咽喉上等于把「主动」摊薄成「路过」。
        #
        # 墙角就是一格，所以不再占两格高。
        trigger("trigger_chujue", 1, 1, "ch03/chujue.lua", "interact", True,
                set_flag="ch03.yuzitong_chujue"),
    ]
    return build_map("ch02_jusuo", W, H, "terrain_jusuo",
                     map_props("ch02_jusuo", "bgm_indoor", outdoor=False), g, objects)


# ---------------------------------------------------------------------------
# 地图 3：谷中崖壁 32x36 —— 纵向山路，拾瓶处在半途
#
# 谷东的崖壁，一条盘上去的路：九段折返，自崖脚（南）到崖顶（北），越往上越窄。
# 崖顶望得见整条谷，是爬上去的由头；拾瓶带横跨第二上行段的整幅路面，
# 实测崖脚到崖顶最短 74 步、拾瓶带在第 35 步——玩家是冲着崖顶去的，
# 瓶子是半道上踢到的，与原著「山路上意外拾得、不是任何人所赠」一致。
#
# 这张图一个 NPC、一处设施都不放，出口只有崖脚西侧一处：这一段就该是他一个人。
#
# 入口在西：崖壁在药圃东边，从药圃东口往东走出来，落在这张图的西口、面朝东。
# 入口曾开在崖脚南口（面朝北）——出门往东、进门朝北，衔接拐了 90°（map_spec 规则 29）。
# ---------------------------------------------------------------------------
def make_yabi():
    W, H = 32, 36
    g = Grid(W, H)
    rnd = random.Random(808)
    g.fill("ground", 1)
    g.fill("building", 5)
    g.fill("collision", 1)

    # 自下而上九段：横道与上行段交替
    segs = [
        (13, 30, 18, 35, 1),   # 崖脚
        (4, 28, 18, 29, 2),    # 第一横道（西行）
        (4, 22, 9, 27, 3),     # 第一上行
        (4, 20, 26, 21, 2),    # 第二横道（东行，最长）
        (23, 14, 27, 19, 3),   # 第二上行 —— 拾瓶带压在它的下半截，正是全程一半
        (6, 12, 27, 13, 2),    # 第三横道（西行）
        (6, 6, 11, 11, 3),     # 第三上行
        (6, 4, 24, 5, 2),      # 第四横道（东行）
        (19, 1, 24, 3, 1),     # 崖顶（望谷）
    ]
    for x0, y0, x1, y1, gid in segs:
        g.open_rect(x0, y0, x1, y1)
        g.rect("ground", x0, y0, x1, y1, gid)

    # 崖脚南沿封死；西侧凿一条两格宽的小径通谷，只在西口开门
    g.solid_rect(13, 35, 18, 35, 5)
    road(g, 1, 32, 12, 33)
    gate(g, 0, 32, 0, 33)

    # 崖檐压在头顶：越往上越密
    for y in range(H):
        for x in range(W):
            if g.walkable(x, y):
                continue
            if not any(g.walkable(x + dx, y + dy) for dx, dy in ((0, 1), (0, -1), (1, 0), (-1, 0))):
                continue
            if rnd.random() < max(0.14, 0.52 - y * 0.010):
                g.set("front", x, y, 4)
    # 碎石：上行段最碎
    for y in range(H):
        for x in range(W):
            if not g.walkable(x, y):
                continue
            if g.get("ground", x, y) == 3 and rnd.random() < 0.38:
                g.set("overlay", x, y, 7)
            elif rnd.random() < 0.10:
                g.set("overlay", x, y, 6)

    objects = [
        spawn("spawn_from_yaopu", 1, 32, "right", default=True),
        portal("portal_to_yaopu", 0, 32, "ch02_yaopu", "spawn_from_yabi", h=2),

        # 节点 4：山路拾瓶。横跨第二上行段整幅路面（宽 5），上行必踩、绕不开。
        # 按设计这一幕只能有一回，故 once=true；兑现它的是 shiping.lua 末尾那句
        # flag.set("ch02.shiping_done")——瓶子没到手，这一格就还留着。
        trigger("trigger_shiping", 23, 18, "ch02/shiping.lua", "enter", True, w=5, h=2,
                set_flag="ch02.shiping_done"),
    ]
    return build_map("ch02_yabi", W, H, "terrain_yabi",
                     map_props("ch02_yabi", "bgm_cliff"), g, objects)


# ---------------------------------------------------------------------------
# 地图 4：七玄门外刃堂 48x36 —— 谷外的宗门院子
#
# 南门进来是前院，一道二门隔开后院；后院当中是演武场，场北是外刃堂正堂，
# 西厢是药材铺（收药处就在铺前那方空地上），东厢是弟子住处。
#
# 二门那道墙砌两行厚，门洞成了一段两格深的夹道——编剧把抽髓丸之祸写在
# 「后院」的踏入触发上（chousui.lua），这条夹道是进后院的唯一通路，压在这儿
# 才绕不开。它又是本章唯一「前置不满足就静默 return」的脚本，所以必须由地图
# 挂 guard_flag 把它挡在段三之前，否则会被空烧掉。
#
# 四处挂点按 scripts/ch02/*.lua 头部自陈的位置摆：
#   maiyao / suanzhang → 收药处（柜台两侧各一处）
#   chousui            → 后院（二门夹道，enter + once + guard）
#   renqing            → 台阶处（正堂前的石阶）
# ---------------------------------------------------------------------------
def make_wairentang():
    W, H = 48, 36
    g = Grid(W, H)
    rnd = random.Random(909)
    g.fill("ground", 1)
    border(g)

    gate(g, 23, 35, 24, 35)               # 南门 → 出谷小道回药圃

    # 院中道路
    road(g, 23, 12, 24, 34)               # 主道：南门直上演武场
    road(g, 1, 11, 46, 12)                # 前横道（正堂之南）
    road(g, 1, 25, 46, 26)                # 后横道（演武场之南）

    # 二门：墙砌两行厚，门洞是一段两格深的夹道
    g.solid_rect(1, 27, 46, 28, 5)
    gate(g, 23, 27, 24, 28)

    # 正堂：横在院北，堂前一排石阶
    g.solid_rect(14, 3, 33, 9, 5)
    g.rect("front", 14, 2, 33, 2, 4)
    g.rect("ground", 21, 10, 26, 10, 3)

    # 西厢药材铺 + 铺前空地（矮栅三面围，只留南面一道口，收药处摆在当中）
    g.solid_rect(4, 15, 11, 20, 5)
    g.rect("front", 4, 14, 11, 14, 4)
    g.solid_rect(3, 21, 3, 24, 8)
    g.solid_rect(12, 21, 12, 24, 8)
    g.rect("ground", 4, 21, 11, 24, 3)

    # 东厢弟子住处
    g.solid_rect(36, 15, 43, 20, 5)
    g.rect("front", 36, 14, 43, 14, 4)

    # 演武场：沙地，兵器架靠边
    g.rect("overlay", 14, 15, 33, 24, 7)
    for bx in (15, 17, 32):
        g.solid_rect(bx, 15, bx, 15, 8)
    for bx in (15, 32):
        g.solid_rect(bx, 24, bx, 24, 8)

    # 院中零星草木（避开道路与演武场）
    for tx, ty in ((7, 31), (40, 31), (20, 31), (28, 31), (45, 22), (2, 8), (45, 8)):
        tree(g, tx, ty)
    scatter(g, rnd, 120, 1, 1, W - 2, H - 2, grass=0.4)

    objects = [
        spawn("spawn_from_yaopu", 23, 33, "up", default=True),
        portal("portal_to_yaopu", 23, 35, "ch02_yaopu", "spawn_from_wairentang", w=2),

        # ---- 收药处：柜台一座，药商站东侧，两处挂点分列南、西 ----
        facility("facility_yaoshang", 6, 21, "shop", w=2, h=2,
                 ref_id="ch02_wairentang_yaoshang"),
        npc("npc_wairentang_yaoshang", 9, 21, "wairentang_yaoshang", "left",
            "ch02/yaoshang.lua"),
        # 节点 3 下半：第一次卖药，六块灵石（段一的段末）
        trigger("trigger_maiyao", 9, 22, "ch02/maiyao.lua", "interact", False),
        # 节点 11：算账 —— 同一株药这回值多少（章末）
        trigger("trigger_suanzhang", 4, 21, "ch02/suanzhang.lua", "interact", False, h=2),

        # ---- 节点 7：抽髓丸之祸（段三进场，二门夹道，进后院必经） ----
        trigger("trigger_chousui", 23, 27, "ch02/chousui.lua", "enter", True, w=2, h=2,
                guard_flag="ch02.duan3_start", set_flag="ch02.chousui_jian"),

        # ---- 节点 9：同伴还人情（段四的段末，正堂前的石阶） ----
        trigger("trigger_renqing", 23, 10, "ch02/renqing.lua", "interact", False, w=2),

        # ---- 同门往来 ----
        npc("npc_tongmen_luchun", 18, 17, "tongmen_luchun", "down", "ch02/luchun.lua"),
        npc("npc_tongmen_maliu", 29, 17, "tongmen_maliu", "down", "ch02/maliu.lua"),
        # 厉飞雨的闲聊要人情结下之后才有（lifeiyu.lua 在那之前静默 return），
        # 所以人情结下之前他干脆不在场。摆在演武场空处，免得隐身时挡住通路
        # ——引擎的 visible_flag 只影响绘制与对话，占格是照占的。
        npc("npc_li_feiyu", 20, 22, "li_feiyu", "right", "ch02/lifeiyu.lua",
            visible_flag="ch02.renqing_jiexia", hidden_flag="ch03.mo_gui_gu"),
        # 第 3 章的那一次：韩立这一章唯一一次对朋友说假话。同格、互补旗标，
        # 与上面管事那一对同理。npcVisible 先看 visible_flag 再看 hidden_flag，
        # 所以这一位仍旧要等人情那条线先走到。
        # 第 3 章末（ch03.done）他就不在这儿了：第 4 章他在七玄门各堂
        # （ch04_getang 的 npc_li_feiyu，从 ch03.done 起在场），同一个人一时只在一处。
        npc("npc_li_feiyu_ch03", 20, 22, "li_feiyu", "right", "ch03/lifeiyu.lua",
            visible_flag="ch03.mo_gui_gu", hidden_flag="ch03.done"),
    ]
    return build_map("ch02_wairentang", W, H, "terrain_wairentang",
                     map_props("ch02_wairentang", "bgm_sect"), g, objects)


# ---------------------------------------------------------------------------
# 第 1 章神手谷补一处北口 —— 就地修补，不重建
# ---------------------------------------------------------------------------
def patch_shenshougu(payload=None):
    """给神手谷北墙开一处通向 ch02_yaopu 的谷口。幂等，补过的图再补一次不变。

    payload 为 None 时读 maps/ch01_shenshougu.tmj（本脚本自己跑的路径）；
    genmaps.py 重建完神手谷之后会把刚建好的那一份传进来，免得读到旧盘上的图。

    这处口子曾开在南墙：那时神手谷的正口（通炼骨崖）在北墙。两处对调是因为炼骨崖
    从崖顶往北出、却落在谷北口，按住「上」就来回弹（map_spec 规则 29）；
    正口挪到南墙之后，往药圃去是往北走、落在药圃南口，也顺了。
    """
    if payload is None:
        path = os.path.join(genmaps.MAPS, "ch01_shenshougu.tmj")
        payload = json.loads(open(path, "rb").read().decode("utf-8"))
    W = payload["width"]
    layers = {l["name"]: l for l in payload["layers"]}

    def put(layer, x, y, value):
        layers[layer]["data"][y * W + x] = value

    # 北墙开谷口：谷中小径原本铺到 y=1，往上接一格到墙外。
    for x in (19, 20):
        put("ground", x, 0, 2)
        put("building", x, 0, 0)
        put("collision", x, 0, 0)

    objs = layers["objects"]["objects"]
    names = {o["name"] for o in objs}
    if "spawn_from_ch02_yaopu" not in names:
        objs.append(spawn("spawn_from_ch02_yaopu", 19, 1, "down"))
    if "portal_to_ch02_yaopu" not in names:
        objs.append(portal("portal_to_ch02_yaopu", 19, 0, "ch02_yaopu",
                           "spawn_from_shenshougu", w=2,
                           require_flag="ch01.done",
                           deny_text_key="ch02.block.yaopu"))
    for i, o in enumerate(objs, 1):
        o["id"] = i
    payload["nextobjectid"] = len(objs) + 1
    return payload


# ---------------------------------------------------------------------------
def build_all():
    """本章四张图的完整产出，外加神手谷那处就地修补的南口。"""
    return {
        "ch02_yaopu": make_yaopu(),
        "ch02_jusuo": make_jusuo(),
        "ch02_yabi": make_yabi(),
        "ch02_wairentang": make_wairentang(),
        "ch01_shenshougu": patch_shenshougu(),
    }


def main(argv=None):
    argv = list(sys.argv[1:] if argv is None else argv)
    if "--check" in argv:
        return check_maps(build_all())

    for name in ("terrain_yaopu", "terrain_jusuo", "terrain_yabi", "terrain_wairentang"):
        write_tileset(name)

    for map_id, payload in build_all().items():
        dump_map(os.path.join(genmaps.MAPS, map_id + ".tmj"), payload)
        print("wrote", map_id)
    return 0


if __name__ == "__main__":
    sys.exit(main())
