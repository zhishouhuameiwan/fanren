# -*- coding: utf-8 -*-
"""第 4 章地图生成器：重跑即重建下列四张图与四个 tileset，并给韩家村补一处村东口。

    python tools/mapgen/genmaps_ch04.py
    python tools/mapgen/genmaps_ch04.py --check   # 只比对，不写盘；有漂移则退出码 1
    python tools/validate.py                      # 自检

    （`genmaps.py --check` 会把本章这四张连同韩家村一并盯上，build.bat 里仍只有那一条。）

产出（全部写进 maps/，不经手 scripts/ 与 data/）：

    maps/ch04_luorifeng.tmj     40x30  落日峰（本章主场：丹房 · 崖台打坐处 · 石坪习法处）
    maps/ch04_getang.tmj        48x36  七玄门各堂（门派日常；节点 1 与节点 8）
    maps/ch04_yanwuchang.tmj    48x40  演武场（切磋 · 多波次攻防战 · 战利品）
    maps/ch04_shanxiazhen.tmj   40x30  山下镇（药市；贾天龙那一节）
    maps/tilesets/terrain_{luorifeng,getang,yanwuchang,shanxiazhen}.tsj

    另就地修补 maps/ch01_hanjiacun.tmj（见第五节，那里有原委）。

--------------------------------------------------------------------------
一、空间关系（先想清楚再摆 —— 第 1 章在这上面栽过一次：山路摆在了村与镇之间，
    而宗门正门就在那座山上）

    前三章已经定死的骨架，一格都不动：

        韩家村 ──南下──▶ 青牛镇 ──上山──▶ 彩霞山道 ──▶ 七玄门正门
                                                          │
                                                       炼骨崖 ──▶ 神手谷 ──▶ 第 2、3 章那几张

    彩霞山是七玄门总门所在的山，**青牛镇在它南麓**、是主道上那个受门派控制的镇子
    （`docs/lore/势力与地理.md` 第 88-90 行）。本章往这张骨架的**另一侧**加四张：

        · **落日峰**是七玄门的后峰。原著里它是七玄门之乱的决战处（人间篇校对
          第 199 行「炼骨崖 / 落日峰 … 落日峰是七玄门之乱的决战处」）。
          韩立以「韩神医」的身份留下，门中给他的正是这么一座偏峰 ——
          一个没有根基的外人分不到腹地，这一点本身就是节点 1 的底色。
          峰顶只有一条路，自各堂北上；四面是崖。

        · **七玄门各堂**是宗门腹地：掌门殿居中偏北，内刃堂在西、执事堂在东，
          南面一道二门隔出外院。**第 2 章的外刃堂就在西墙之外、同一片院墙里，
          但不是这张图**（`ch02_wairentang` 由 genmaps_ch02.py 从零重建，
          往它的 tmj 里塞对象会让那边的 --check 当场报漂移）。两张之间不设传送点：
          外刃堂那张图只有一个 spawn（`spawn_from_yaopu`，在它的南门口），
          从各堂进去会落在一个「从药圃上来」的落点上，那是假的。

        · **演武场**在各堂东门之外，是门内校场。切磋与门派攻防战都在这儿，
          所以它是四张里最大的一张（48x40），中间留出一整片空场给多波次战斗。

        · **山下镇在彩霞山北麓**，是七玄门控制的十几个小城镇里的另一个
          （势力与地理表：青牛镇是「十几个小城镇之一」，不是唯一一个）。
          **它不是青牛镇**：青牛镇在南麓主道上、韩家村南下就是；山下镇在北麓、
          自各堂南门下山才到。镇上有药市 —— 韩神医的药材从这儿来，
          节点 3 开炉之后玩家会反复走这一趟，节点 7 贾天龙就堵在这条路上。

    连通与闸门一览（括号里是 require_flag，反向一律不设闸门）：

        ch01_hanjiacun ──(ch03.done)──▶ ch04_shanxiazhen ──▶ ch04_getang
              ▲                                                  │
              └──────────────(无闸门，原路返回)──────────────────┘
                                                                 ├──(ch04.liuxia)────▶ ch04_luorifeng
                                                                 └──(ch04.yufeng_xue)▶ ch04_yanwuchang

    **为什么本章的入口挂在韩家村而不是神手谷那一侧**：本批的文件白名单只准
    就地修补韩家村一张旧图（maps/ch01_* 与 ch02_* / ch03_* 一律不碰）。
    韩家村村东原本是一面到头的篱笆，这里开一处车道口，往东北绕过山脚到山下镇。
    走法上这条路比主道绕，但它正是节点 11 回村要走的那一条 —— 他是**悄悄走的**，
    留的是一封信而不是一场送别，不从宗门正门、不经青牛镇，这条后路与那一节的
    分量对得上。反过来，章首从山下镇北上各堂也走得通，规则 18 因此不必等脚本。

二、防跳过与「谁置这个旗标」

    本章三道 portal 闸门与七处 guard_flag 用的旗标，全部由本文件某个触发器的
    set_flag 兑现，没有一个悬在尚未写出的脚本上 —— 规则 19 是静态推演的，
    闸门的钥匙若只存在于将来的某个脚本里，那道门在门禁眼里就是永远打不开的：

        ch03.done           ch03_guwai trigger_ligu（第 3 章章末，已有）
        ch04.liuxia         ch04_getang      trigger_liuxia      （节点 1）
        ch04.huodan_xue     ch04_luorifeng   trigger_huodan      （节点 2）
        ch04.kailu          ch04_luorifeng   trigger_kailu       （节点 3）
        ch04.yufeng_xue     ch04_luorifeng   trigger_yufeng      （节点 4）
        ch04.fawu_bingyong  ch04_yanwuchang  trigger_qiecuo      （节点 5）
        ch04.kaizhan        ch04_yanwuchang  trigger_kaizhan     （节点 6）
        ch04.jia_tianlong   ch04_shanxiazhen trigger_jia_tianlong（节点 7）
        ch04.jinguang_jian  ch04_getang      trigger_jinguang    （节点 8）
        ch04.gongfang_zhan  ch04_yanwuchang  trigger_gongfang    （节点 9）
        ch04.pai_dedao      ch04_yanwuchang  trigger_zhanlipin   （节点 10）
        ch04.hui_cun        ch01_hanjiacun   trigger_huicun      （节点 11 上）
        ch04.xin_neirong    ch01_hanjiacun   trigger_xinbie      （节点 11 中）
        ch04.done           ch01_hanjiacun   trigger_dongqu      （节点 11 下）

    另两个旗标（ch04.bushu 节点 6 的三选一、ch04.jianfu_dedao 节点 10 的剑符）
    由脚本自己置，不作闸门，规则 19 不问它们。

三、踏入型与交互型怎么分（前三章踩出来的）

    **踏入型（enter）卡在咽喉上，交互型（interact）离开必经之路。**
    一处 enter 摆在敞开的过道上就靠不住，一处 interact 摆在必经之路上则等于
    把「主动」摊薄成「路过」。本章的分法：

      enter（六处，每一处都是真咽喉，且各自独占一格，见下）
        trigger_liuxia       各堂二门（外院进内院唯一的两格口子）
        trigger_huodan       落日峰峰道口（全峰唯一入口，四面是崖）
        trigger_kaizhan      演武场西辕门甬道末（甬道两侧砌墙，八格深）
        trigger_gongfang     演武场西辕门甬道口外第一格（与上一处**不同格**）
        trigger_jia_tianlong 山下镇坊门（北坊下市集唯一的两格口子）
        trigger_jinguang     掌门殿门洞（进殿唯一的两格口子）
        trigger_huicun       韩家村村东巷口（新开的那条车道唯一的出口）

      interact（六处，全部离开主路）
        trigger_kailu        丹房里的丹炉（与设施同格，见第四节）
        trigger_yufeng       崖台（石栏之外，一条支路的尽头）
        trigger_qiecuo       演武台台面正中（台阶上去才够得着）
        trigger_zhanlipin    校场东头焦土（空场里一处地标，不在任何路上）
        trigger_xinbie       韩家屋南的院子
        trigger_dongqu       村东老树下

    **两个 enter 触发绝不同格。** 引擎的 tryStep 走 `objectAt(target, "trigger")`，
    它不分 mode 只取对象表里第一个命中的：第一个若因 guard 未满足而不 ready，
    这一步就什么也不发生，排在后面的那个**永远轮不到**。校验器规则 17 只报
    「无 guard 且非 once」的那一类遮蔽，带 guard 的两个叠在一格上它一个字都不说。
    所以演武场那两处（节点 6 与节点 9）是 (8,20) 与 (9,20) 两格前后相邻，
    玩家自西门出甬道时先踩前一格、再踩后一格，两节都靠得住。

四、丹炉：facility 与 interact 触发**故意**同格

    节点 3「开炉」要教炼丹，而炼丹面板的入口是 `facility kind=alchemy`。
    两者摆在同一格上是规范里写明的合法写法（map_spec 规则 17 末段）：
    `WorldScene::interact` 依次问 npc → interact 触发 → facility，而 once=true
    的触发器**烧掉之后就让位**。于是同一个丹炉：

        节点 3 之前按它 → trigger_kailu 应声，演「开炉」这一节；
        节点 3 之后按它 → 触发器烧掉了，facility 应声，炼丹面板打开。

    这比「给 facility 挂 require_flag」好：facility 的 require_flag 没有配套的
    deny 文案（规则 20 只管 portal），拦住了玩家一个字也听不见，正是本项目
    明令禁止的那种「按键没反应」。现在拦它的是一段有台词的戏。

    炉鼎品阶写在 `facility.grade`（本章唯一一处），0-5，缺省 1。
    `docs/interfaces-p3-ch04.md` §3.3.1 说明它直接进 `rules::successChance`，
    每品阶 5 点。map_spec §4.6 的属性表尚未收录这一行（主控会补）；
    validate.py 不查未知属性，先按 grade 写。落日峰这一座是门中拨给韩神医的
    旧炉，写 grade=1 —— 与引擎侧 `kDefaultCraftToolGrade` 同值，
    是「有炉，但不是什么好炉」。

五、韩家村那处村东口：就地修补，不重建

    韩家村的地形归 genmaps.py 所有（make_hanjiacun），本文件照第 2、3 章给
    神手谷开口子的路子，只改需要改的那几格、只追加那几个对象，其余原样写回；
    幂等，补过的图再补一次不变。

    改动一共十四格：村东篱笆 (39,14)(39,15) 开成车道口，(36..38,14..15) 铺路，
    (36..38,13) 与 (36..38,16) 砌成巷墙。砌这两道墙是为了把巷口收成两格
    ——(35,14)(35,15) 于是成了真咽喉，节点 11 的 trigger_huicun 才踩得响。
    不砌墙的话村里是一片敞开的空地，一处 enter 摆在那儿就是第 3 章那种
    「墙角的戏摆在敞开过道上」。

    genmaps.py 的 build_all() 会在 make_hanjiacun() 之后回头调用本文件的
    patch_hanjiacun()，所以单跑 genmaps.py 也不会把第 4 章割断。
    —— 这一条不是假想：第 1 章的闸门曾经只存在于 tmj 里，重跑一次就是一次
    静默的数据损毁。

六、瓦片 gid 约定（沿用 genmaps.py 那一套，四个 tileset 统一编号）

    1 主地表  2 道路/石阶/台面  3 特殊地表（焦土/翻土/碎石）  4 front 遮挡
    5 building 主体（墙体/岩壁/院墙）  6 overlay 花草  7 overlay 碎石
    8 building 次体（家具/石栏/兵器架/摊架/篱笆）

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

CHAPTER = 4
PREFIX = "ch04_"


def map_props(map_id, bgm, outdoor=True):
    return [
        ("map_id", map_id),
        ("display_name_key", "ch04.map.%s.name" % map_id[len(PREFIX):]),
        ("region", "qingzhou"),       # 与第 1-3 章一致：越国镜州彩霞山
        ("bgm", bgm),
        ("chapter", CHAPTER),
        ("outdoor", outdoor),
        # can_leave_edge 一律 false：TileMapLoader 只校验它的存在与类型，
        # 引擎里没有任何地方消费它，写 true 只会得到一个不生效的属性。
        ("can_leave_edge", False),
    ]


def battle_anchor(name, x, y, w, h, terrain, ally_zone, enemy_zone):
    """战斗地形锚点（map_spec 4.7）。引擎目前不消费它，校验器查三个必填属性。

    摆它是为了让战斗场景取地形与初始站位时有据可依，而不是让战斗数据里
    再抄一遍坐标 —— 抄两遍的东西迟早会对不上。
    """
    return obj(name, "battle_anchor", x, y, w, h,
               terrain=terrain, ally_zone=ally_zone, enemy_zone=enemy_zone)


# ---------------------------------------------------------------------------
# 网格小工具（与 genmaps_ch02/ch03 同形；那两边的没有导出，这里各留一份）
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


def hall(g, x0, y0, x1, y1, door):
    """一座堂：实心砌起来再掏空，门洞由 door=(dx0,dy0,dx1,dy1) 指定。

    屋檐压在北墙外一行的 front 层（不挡路，角色从檐下走过时被遮住）。
    """
    g.solid_rect(x0, y0, x1, y1, 5)
    g.open_rect(x0 + 1, y0 + 1, x1 - 1, y1 - 1)
    g.rect("ground", x0 + 1, y0 + 1, x1 - 1, y1 - 1, 1)
    gate(g, *door)
    g.rect("front", x0, y0 - 1, x1, y0 - 1, 4)


def tree(g, x, y):
    """一棵树：树干占一格，树冠三格见方压在 front 层（不挡路）。"""
    g.rect("front", x - 1, y - 1, x + 1, y + 1, 4)
    g.solid_rect(x, y, x, y, 5)


def eaves(g, rnd, density=0.35):
    """给紧挨着可走格的岩壁压一层 front，山崖与室内用。"""
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
# 地图 1：落日峰 40x30 —— 本章主场
#
# 一座四面是崖的偏峰，只有南面一条峰道通下去（各堂）。峰顶分三处，
# 正好对上设计文档第 4 节要的三样：
#
#   · 石坪（南，紧挨峰道口）—— 习法处。几根石桩当靶子。
#   · 丹房（西）—— 炼丹炉在里头，外加药案与一处存档点。
#   · 崖台（东，石栏之外）—— 打坐处；御风决在这儿练，身法要临着崖才练得出。
#
# 挂点（脚本由编剧写，本图只按设计文档第 3 节的节点表引用）：
#   节点 2 习火弹术 → trigger_huodan（峰道口，enter + once）
#                     全峰唯一入口，踏进来必响 —— 它是本章第一个必过的教学，
#                     漏掉玩家就推不动节点 3，所以卡在咽喉上而不是摆成交互。
#   节点 3 开炉     → trigger_kailu（与丹炉同格，interact + once，见第四节）
#   节点 4 习御风决 → trigger_yufeng（崖台，interact + once）
# ---------------------------------------------------------------------------
def make_luorifeng():
    W, H = 40, 30
    g = Grid(W, H)
    rnd = random.Random(401)
    g.fill("ground", 1)

    # 四面崖壁，两格厚 —— 一座峰该有的样子，也顺手把「只有一条路」写死在地形里
    g.solid_rect(0, 0, W - 1, 1, 5)
    g.solid_rect(0, H - 2, W - 1, H - 1, 5)
    g.solid_rect(0, 0, 1, H - 1, 5)
    g.solid_rect(W - 2, 0, W - 1, H - 1, 5)

    # 峰道：南崖上唯一的两格口子
    road(g, 19, H - 2, 20, H - 1)

    # 石坪（习法处）：峰道口一进来就是
    g.rect("ground", 13, 20, 27, 27, 2)
    for px, py in ((14, 22), (16, 25), (24, 22), (26, 25)):
        g.solid_rect(px, py, px, py, 8)      # 石桩

    # 丹房（西）：一间十格见方的石屋，门开在东墙
    hall(g, 5, 7, 14, 16, (14, 11, 14, 12))
    g.solid_rect(10, 9, 11, 10, 8)           # 药案
    g.rect("overlay", 6, 8, 13, 15, 7)

    # 崖台（东）：石栏留一处两格的豁口，台面铺石板
    g.solid_rect(33, 5, 33, 8, 8)
    g.solid_rect(33, 11, 33, 14, 8)
    g.rect("ground", 34, 5, 37, 14, 2)

    # 峰上零星几棵松
    for tx, ty in ((7, 22), (10, 25), (30, 20), (34, 24), (29, 4), (17, 5)):
        tree(g, tx, ty)

    eaves(g, rnd, 0.28)
    scatter(g, rnd, 150, 2, 2, W - 3, H - 3)

    objects = [
        # ---- 出入口（全峰唯一一条路）----
        spawn("spawn_from_getang", 19, 28, "up", default=True),
        portal("portal_to_getang", 19, 29, "ch04_getang", "spawn_from_luorifeng", w=2),

        # ---- 节点 2：习火弹术（峰道口，踏进峰顶必响）----
        trigger("trigger_huodan", 19, 27, "ch04/huodan.lua", "enter", True, w=2,
                set_flag="ch04.huodan_xue"),

        # ---- 节点 3：开炉（与丹炉同格 —— 烧掉之后丹炉才应声，见第四节）----
        # 顺序不靠地图拦，靠 kailu.lua 开头查 ch04.huodan_xue 再 return：
        # 被拦住的玩家听得见自己还差什么，而 once 因为走不到末尾那行而留着，
        # 火弹术学会之后再按一次照样演得出来（map_spec 4.4 那条改动的用意）。
        trigger("trigger_kailu", 7, 9, "ch04/kailu.lua", "interact", True, w=2, h=2,
                set_flag="ch04.kailu"),
        facility("facility_danlu", 7, 9, "alchemy", w=2, h=2, grade=1),

        # ---- 节点 4：习御风决（崖台）----
        trigger("trigger_yufeng", 35, 12, "ch04/yufeng.lua", "interact", True,
                set_flag="ch04.yufeng_xue"),

        facility("facility_dazuo", 35, 7, "meditate"),
        facility("facility_cunji", 12, 14, "save"),
    ]
    return build_map("ch04_luorifeng", W, H, "terrain_luorifeng",
                     map_props("ch04_luorifeng", "bgm_cliff"), g, objects)


# ---------------------------------------------------------------------------
# 地图 2：七玄门各堂 48x36 —— 门派日常
#
# 一座规整的宗门院子，南山门 → 外院 → 二门 → 内院 → 掌门殿，一条甬道到底。
# 二门那两格是外院进内院唯一的口子，节点 1 就卡在那儿：玩家从山下镇上来，
# 一脚踏进二门，门中人就把「韩神医」这个身份摆到他面前。
#
#   北排三堂：内刃堂（西）· 掌门殿（中）· 执事堂（东）
#   中排两屋：客堂（西）· 库房（东）
#   南排厢房两列，外加外院
#   北口（西北角）上落日峰；东口出演武场
#
# 外刃堂（ch02_wairentang）就在西墙之外，同一片院墙里 —— 但不设传送点，
# 原委见本文件第一节。
#
# 挂点：
#   节点 1 以韩神医的身份留下 → trigger_liuxia（二门，enter + once）
#   节点 8 金光上人与王绝楚   → trigger_jinguang（掌门殿门洞，enter + once）
#                               戏在殿里演，触发就摆在殿门上 —— 第 3 章出过
#                               「戏在 A 图触发器在 B 图」，这里不重犯。
# ---------------------------------------------------------------------------
def make_getang():
    W, H = 48, 36
    g = Grid(W, H)
    rnd = random.Random(402)
    g.fill("ground", 1)
    border(g)

    # 北排三堂（门一律朝南，开在自家南墙上）
    hall(g, 6, 2, 17, 11, (11, 11, 12, 11))     # 内刃堂
    hall(g, 19, 2, 30, 12, (23, 12, 24, 12))    # 掌门殿
    hall(g, 32, 2, 43, 11, (37, 11, 38, 11))    # 执事堂

    # 中排两屋
    hall(g, 6, 16, 17, 19, (11, 19, 12, 19))    # 客堂
    hall(g, 31, 16, 42, 19, (36, 19, 37, 19))   # 库房

    # 南排厢房（门朝北，开在自家北墙上）
    hall(g, 6, 27, 17, 31, (11, 27, 12, 27))
    hall(g, 31, 27, 42, 31, (36, 27, 37, 27))

    # 二门墙：把外院与内院隔开。那两格口子是全图唯一的咽喉，节点 1 卡在上面。
    g.solid_rect(1, 32, 46, 32, 5)

    # 街巷（砌完堂再铺，免得把墙铺穿）
    road(g, 2, 13, 45, 15)        # 北横街
    road(g, 2, 20, 46, 21)        # 中横街
    road(g, 2, 25, 45, 26)        # 南横街
    road(g, 2, 1, 3, 31)          # 西巷
    road(g, 44, 1, 45, 31)        # 东巷
    road(g, 23, 13, 24, 34)       # 主甬道（顺手在二门墙上开出那两格）

    # 三处门：南山门、北口（上落日峰）、东口（出演武场）
    gate(g, 23, 35, 24, 35)
    gate(g, 2, 0, 3, 0)
    gate(g, 47, 20, 47, 21)

    # 院里的老树与石灯
    for tx, ty in ((20, 17), (28, 17), (20, 29), (28, 29), (36, 23), (12, 23)):
        tree(g, tx, ty)

    scatter(g, rnd, 220, 1, 1, W - 2, H - 2)

    objects = [
        # ---- 出入口 ----
        spawn("spawn_from_shanxiazhen", 23, 34, "up", default=True),
        spawn("spawn_from_luorifeng", 2, 1, "down"),
        spawn("spawn_from_yanwuchang", 46, 20, "left"),

        portal("portal_to_shanxiazhen", 23, 35, "ch04_shanxiazhen",
               "spawn_from_getang", w=2),
        # 落日峰是门中拨给「韩神医」的住处 —— 没应下这个身份就上不去。
        portal("portal_to_luorifeng", 2, 0, "ch04_luorifeng", "spawn_from_getang", w=2,
               require_flag="ch04.liuxia", deny_text_key="ch04.block.luorifeng"),
        # 演武场要等两门法术都到手：节点 5 的切磋教的是法术与刀怎么配合，
        # 只会一门的时候那一节没有可演的东西。
        portal("portal_to_yanwuchang", 47, 20, "ch04_yanwuchang", "spawn_from_getang", h=2,
               require_flag="ch04.yufeng_xue", deny_text_key="ch04.block.yanwuchang"),

        # ---- 节点 1：以韩神医的身份留下（二门，踏进内院必响）----
        trigger("trigger_liuxia", 23, 32, "ch04/liuxia.lua", "enter", True, w=2,
                set_flag="ch04.liuxia"),

        # ---- 节点 8：金光上人与王绝楚（掌门殿门洞）----
        trigger("trigger_jinguang", 23, 12, "ch04/jinguang.lua", "enter", True, w=2,
                guard_flag="ch04.jia_tianlong", set_flag="ch04.jinguang_jian"),

        # ---- 常驻 ----
        # 王绝楚坐在殿上。role_id 尚待主控建档（设计文档第 5 节），
        # data/roles/wang_juechu.json 建好之前 validate 会在这一处报缺角色。
        npc("npc_wang_juechu", 24, 4, "wang_juechu", "down", "ch04/wang_juechu.lua"),
        # 厉飞雨这一章要独当一面，别把他写成陪衬 —— 他站在内院当中，
        # 不在任何一条路上，玩家愿意找他说话才说得上。
        # 第 3 章末（ch03.done）起才在这儿：此前他在外刃堂（ch02_wairentang 那两个
        # npc_li_feiyu*），同一个人一时只在一处。
        npc("npc_li_feiyu", 20, 23, "li_feiyu", "down", "ch04/feiyu.lua",
            visible_flag="ch03.done"),

        facility("facility_cunji", 27, 23, "save"),
    ]
    return build_map("ch04_getang", W, H, "terrain_getang",
                     map_props("ch04_getang", "bgm_sect"), g, objects)


# ---------------------------------------------------------------------------
# 地图 3：演武场 48x40 —— 切磋与多波次攻防战
#
# 四张里最大的一张，理由只有一个：门派攻防战是多波次的
# （docs/interfaces-p3-ch04.md §2：打完一波下一波进场，中途不回血不重置），
# 战场小了，第二波压上来时根本没有地方站。所以校场中段 x4..43 / y14..35
# 整整一片四十乘二十二格全留空，battle_anchor_gongfang 就框在那里。
#
#   西辕门：一条八格深、两格宽的甬道，两侧砌墙 —— 全图唯一的入口。
#           节点 6 与节点 9 两处 enter 触发前后相邻各占一格（见第三节，
#           两个 enter 绝不同格）。
#   演武台：北面一座石栏围起来的台子，只有南面一处台阶上得去。节点 5 在台面正中。
#   焦土  ：校场东头。节点 10 的战利品落在那儿 —— 那是金光上人倒下的地方。
#
# 挂点：
#   节点 5 法武并用 → trigger_qiecuo（演武台台面，interact + once）
#                     这一打是他自己要打的（原著 ch75：法武并用是他自己摸索
#                     出来的，不是谁教的），所以摆成主动的交互，不摆在路上。
#   节点 6 开战     → trigger_kaizhan（甬道末格，enter + once）
#   节点 9 攻防战   → trigger_gongfang（甬道口外第一格，enter + once）
#   节点 10 战利品  → trigger_zhanlipin（焦土，interact + once）
# ---------------------------------------------------------------------------
def make_yanwuchang():
    W, H = 48, 40
    g = Grid(W, H)
    rnd = random.Random(403)
    g.fill("ground", 1)
    border(g)

    # 西辕门甬道：两侧砌墙，八格深两格宽
    g.solid_rect(1, 19, 8, 19, 5)
    g.solid_rect(1, 22, 8, 22, 5)
    road(g, 1, 20, 8, 21)
    gate(g, 0, 20, 0, 21)

    # 演武台：石栏围一圈，南面留两格台阶
    g.solid_rect(18, 4, 29, 4, 8)
    g.solid_rect(18, 11, 29, 11, 8)
    g.solid_rect(18, 5, 18, 10, 8)
    g.solid_rect(29, 5, 29, 10, 8)
    gate(g, 23, 11, 24, 11)
    g.rect("ground", 19, 5, 28, 10, 2)

    # 东辕门：敌人压上来的那一面。只做样子，不设传送点（墙照旧不通）
    g.rect("building", 47, 18, 47, 23, 8)

    # 兵器架与箭靶
    g.solid_rect(4, 34, 12, 34, 8)
    g.solid_rect(35, 34, 43, 34, 8)
    for bx, by in ((14, 16), (33, 16), (14, 33), (33, 33)):
        g.solid_rect(bx, by, bx, by, 8)

    # 校场东头那片焦土 —— 节点 10 的地标
    g.rect("ground", 33, 22, 39, 27, 3)
    g.rect("overlay", 33, 22, 39, 27, 7)

    scatter(g, rnd, 200, 1, 1, W - 2, H - 2, grass=0.5)

    objects = [
        # ---- 出入口 ----
        spawn("spawn_from_getang", 1, 20, "right", default=True),
        portal("portal_to_getang", 0, 20, "ch04_getang", "spawn_from_yanwuchang", h=2),

        # ---- 节点 5：法武并用（演武台台面正中）----
        trigger("trigger_qiecuo", 23, 7, "ch04/qiecuo.lua", "interact", True, w=2,
                set_flag="ch04.fawu_bingyong"),
        battle_anchor("battle_anchor_qiecuo", 18, 4, 12, 8, "battlefield",
                      ally_zone="20,9,3,2", enemy_zone="25,5,3,2"),

        # ---- 节点 6：野狼帮来犯，开战（甬道末格）----
        # 切磋完从台上回西门，出甬道之前必踩这一格 —— 号角是在他背后响的。
        trigger("trigger_kaizhan", 8, 20, "ch04/kaizhan.lua", "enter", True, h=2,
                guard_flag="ch04.fawu_bingyong", set_flag="ch04.kaizhan"),

        # ---- 节点 9：门派攻防战（甬道口外第一格；与上一处不同格）----
        trigger("trigger_gongfang", 9, 20, "ch04/gongfang.lua", "enter", True, h=2,
                guard_flag="ch04.jinguang_jian", set_flag="ch04.gongfang_zhan"),
        battle_anchor("battle_anchor_gongfang", 4, 14, 40, 22, "battlefield",
                      ally_zone="6,22,4,4", enemy_zone="36,22,4,4"),

        # ---- 节点 10：战利品（焦土。剑符与那块牌子都在这儿）----
        # 设计文档 1.1：那块牌子在本章不得有名字、不得有任何用途暗示。
        # 地图这一侧能做的只有一件事 —— 不给它单独的地标、不给它单独的挂点，
        # 它和剑符一起躺在同一堆战利品里，由同一个脚本一次交代完。
        trigger("trigger_zhanlipin", 36, 24, "ch04/zhanlipin.lua", "interact", True, w=2,
                guard_flag="ch04.gongfang_zhan", set_flag="ch04.pai_dedao"),

        facility("facility_cunji", 3, 17, "save"),
    ]
    return build_map("ch04_yanwuchang", W, H, "terrain_yanwuchang",
                     map_props("ch04_yanwuchang", "bgm_sect"), g, objects)


# ---------------------------------------------------------------------------
# 地图 4：山下镇 40x30 —— 药市与贾天龙那一节
#
# 彩霞山北麓的镇子（不是南麓主道上的青牛镇，原委见第一节）。
# 一道坊墙把它分成两截：
#
#   北坊：北门坡道下来，一条主街，街两侧两爿铺子。西门通韩家村。
#   市集：坊墙以南一整片空场，药市摆在这儿 —— 韩神医的药材从这儿来。
#
# 坊门那两格是北坊下市集唯一的口子，节点 7 就堵在那儿：
# 他是下来买药材的，堵他的人算准了他必走这一趟。
#
# 挂点：
#   节点 7 贾天龙现身 → trigger_jia_tianlong（坊门，enter + once）
# ---------------------------------------------------------------------------
def make_shanxiazhen():
    W, H = 40, 30
    g = Grid(W, H)
    rnd = random.Random(404)
    g.fill("ground", 1)
    border(g)

    # 北门坡道：两侧砌墙，五格深两格宽
    g.solid_rect(18, 1, 18, 5, 5)
    g.solid_rect(21, 1, 21, 5, 5)
    road(g, 19, 1, 20, 5)
    gate(g, 19, 0, 20, 0)

    # 街两侧的铺子
    hall(g, 3, 3, 10, 12, (10, 7, 10, 8))      # 西铺（门朝东）
    hall(g, 28, 3, 35, 12, (28, 7, 28, 8))     # 东铺（门朝西）

    # 坊墙：把北坊与市集隔开
    g.solid_rect(1, 16, 38, 16, 5)

    # 街道（砌完墙再铺）
    road(g, 19, 6, 20, 15)        # 主街
    road(g, 1, 14, 38, 15)        # 横街
    gate(g, 0, 14, 0, 15)         # 西门（往韩家村）
    road(g, 19, 16, 20, 18)       # 坊门连着市集

    # 市集：一整片石板空场，摊架分列两侧
    g.rect("ground", 6, 18, 33, 27, 2)
    g.solid_rect(10, 25, 12, 25, 8)
    g.solid_rect(25, 25, 27, 25, 8)
    g.solid_rect(22, 21, 24, 21, 8)

    for tx, ty in ((5, 21), (34, 21), (13, 9), (26, 9)):
        tree(g, tx, ty)

    scatter(g, rnd, 160, 1, 1, W - 2, H - 2, grass=0.4)

    objects = [
        # ---- 出入口 ----
        spawn("spawn_from_hanjiacun", 1, 14, "right", default=True),
        spawn("spawn_from_getang", 19, 2, "down"),

        portal("portal_to_hanjiacun", 0, 14, "ch01_hanjiacun",
               "spawn_from_ch04_shanxiazhen", h=2),
        portal("portal_to_getang", 19, 0, "ch04_getang", "spawn_from_shanxiazhen", w=2),

        # ---- 节点 7：贾天龙现身（坊门，下市集必踩）----
        trigger("trigger_jia_tianlong", 19, 17, "ch04/jia_tianlong.lua", "enter", True, w=2,
                guard_flag="ch04.kaizhan", set_flag="ch04.jia_tianlong"),
        battle_anchor("battle_anchor_jia", 8, 18, 24, 10, "field",
                      ally_zone="18,25,4,2", enemy_zone="18,19,4,2"),

        # ---- 药市：节点 3 开炉之后的经营循环要走这一趟 ----
        facility("facility_yaoshi", 12, 21, "shop", w=2, h=2,
                 ref_id="shop_village_general"),
        # 第 1 章青牛镇那个走镇的卖药郎（scripts/ch04/yaofan.lua 首部）。
        # 青牛镇那一个第 1 章末就走了，这一个第 4 章（ch03.done 之后）才到：同一个人一时只在一处。
        npc("npc_zhen_min_yaofan", 14, 22, "zhen_min_yaofan", "left", "ch04/yaofan.lua",
            visible_flag="ch03.done"),
    ]
    return build_map("ch04_shanxiazhen", W, H, "terrain_shanxiazhen",
                     map_props("ch04_shanxiazhen", "bgm_town"), g, objects)


# ---------------------------------------------------------------------------
# 韩家村：村东那处车道口 —— 就地修补，不重建（原委见本文件第五节）
# ---------------------------------------------------------------------------
def patch_hanjiacun(payload=None):
    """给韩家村补上村东的巷口与节点 11 的三处挂点。幂等，补过的图再补一次不变。

    payload 为 None 时读 maps/ch01_hanjiacun.tmj（本脚本自己跑的路径）；
    genmaps.py 会把 make_hanjiacun() 手上那一份传进来，免得读到旧盘上的图。
    追加顺序必须稳定（落点 → 传送点 → 三处触发），否则两边的对象表排序不同，
    `--check` 会报一堆「只在 maps/ 里有」的假漂移。
    """
    if payload is None:
        path = os.path.join(genmaps.MAPS, "ch01_hanjiacun.tmj")
        payload = json.loads(open(path, "rb").read().decode("utf-8"))
    W = payload["width"]
    layers = {l["name"]: l for l in payload["layers"]}

    def put(layer, x, y, value):
        layers[layer]["data"][y * W + x] = value

    # 一、村东篱笆开一处车道口，往东北绕过山脚去山下镇。
    for y in (14, 15):
        put("building", W - 1, y, 0)
        put("collision", W - 1, y, 0)
        for x in (36, 37, 38, 39):
            put("ground", x, y, 2)
            put("overlay", x, y, 0)     # 原本撒在这儿的花草，铺了路就没有了

    # 二、巷墙。砌这两道是为了把巷口收成两格 —— (35,14)(35,15) 于是成了真咽喉，
    # 节点 11 的 trigger_huicun 才踩得响。不砌墙的话村东是一片敞开的空地，
    # 一处踏入型触发摆在那儿谁也保证不了它会响（第 3 章那条教训）。
    for x in (36, 37, 38):
        for y in (13, 16):
            put("building", x, y, 8)
            put("collision", x, y, 1)
            put("overlay", x, y, 0)

    objs = layers["objects"]["objects"]
    names = {o["name"] for o in objs}

    if "spawn_from_ch04_shanxiazhen" not in names:
        objs.append(spawn("spawn_from_ch04_shanxiazhen", 38, 14, "left"))
    if "portal_to_ch04_shanxiazhen" not in names:
        # 这条路第 3 章走完才通：第 4 章之前村东就是一面到头的篱笆。
        objs.append(portal("portal_to_ch04_shanxiazhen", 39, 14, "ch04_shanxiazhen",
                           "spawn_from_hanjiacun", h=2,
                           require_flag="ch03.done",
                           deny_text_key="ch04.block.shanxiazhen"))
    # 节点 11 上：回村。巷口那两格是新开这条路唯一的出口，回到村里必踩。
    if "trigger_huicun" not in names:
        objs.append(trigger("trigger_huicun", 35, 14, "ch04/huicun.lua", "enter", True, h=2,
                            guard_flag="ch04.pai_dedao", set_flag="ch04.hui_cun"))
    # 节点 11 中：留药与信别厉飞雨（二选一：信里写什么）。
    # 摆在韩家屋南的院子里 —— 这一节是他自己坐下来写的，不该是走着走着撞上的，
    # 所以是交互型，而且离开村里那几条路。
    if "trigger_xinbie" not in names:
        objs.append(trigger("trigger_xinbie", 12, 14, "ch04/xinbie.lua", "interact", True, w=2,
                            guard_flag="ch04.hui_cun", set_flag="ch04.xin_neirong"))
    # 节点 11 下：东去（章末）。村东老树底下 —— 同样是主动的戏，
    # 而且与村口那处「母亲送别」（第 1 章 trigger_cunkou_bie，在北墙村口）分得开。
    if "trigger_dongqu" not in names:
        objs.append(trigger("trigger_dongqu", 35, 17, "ch04/dongqu.lua", "interact", True,
                            guard_flag="ch04.xin_neirong", set_flag="ch04.done"))

    for i, o in enumerate(objs, 1):
        o["id"] = i
    payload["nextobjectid"] = len(objs) + 1
    return payload


# ---------------------------------------------------------------------------
def build_all():
    """本章四张图的完整产出，外加韩家村那处就地修补的村东口。"""
    return {
        "ch04_luorifeng": make_luorifeng(),
        "ch04_getang": make_getang(),
        "ch04_yanwuchang": make_yanwuchang(),
        "ch04_shanxiazhen": make_shanxiazhen(),
        "ch01_hanjiacun": patch_hanjiacun(),
    }


TILESETS = ("terrain_luorifeng", "terrain_getang",
            "terrain_yanwuchang", "terrain_shanxiazhen")


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
