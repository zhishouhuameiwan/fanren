# -*- coding: utf-8 -*-
"""第 1 章地图生成器：重跑即重建下列六张图与四个 tileset。

    python tools/mapgen/genmaps.py
    python tools/mapgen/genmaps.py --check   # 只比对，不写盘；有漂移则退出码 1
    python tools/validate.py --maps          # 自检，应为 VALIDATE_OK

**本文件是 maps/ch01_*.tmj 的唯一真源，两边必须一字不差。**
曾经不是：防跳过用的 require_flag / deny_text_key / guard_flag 是 genmaps.py
产出之后手工补进 tmj 的，生成器这边一直没有，于是「重跑一次」= 把第 1 章的
9 处闸门连同第 2 章的入口一起抹掉，而且不报任何错（validate.py 只查引用的旗标
已登记，不查闸门存在）。闸门现在写在下面六张图的对象表里，`--check` 用来盯住
它们不再走散——改地图请改这里再重跑，不要直接编辑 tmj。

神手谷北墙那处通往第 2 章的谷口不属于本文件：它是 genmaps_ch02.py 的
patch_shenshougu() 就地修补出来的（那边有原委）。build_all() 会在最后把它
原样补回去，所以单跑本脚本也不会把第 2 章从第 1 章割断。

产出（全部写进 maps/，不经手 scripts/ 与 data/）：

    maps/ch01_hanjiacun.tmj      40x30  韩家村（P2 已有，本脚本重建地表并补全对象）
    maps/ch01_qingniuzhen.tmj    48x36  青牛镇
    maps/ch01_caixiashan.tmj     40x30  彩霞山道
    maps/ch01_qixuanmen.tmj      24x18  七玄门（P2 已有，只换对象层，图块层原样保留）
    maps/ch01_liangu_ya.tmj      32x40  炼骨崖（纵向狭长）
    maps/ch01_shenshougu.tmj     40x30  神手谷
    maps/tilesets/terrain_{caixiashan,qingniuzhen,liangu_ya,shenshougu}.tsj

地图连通链路（每对相邻双向；依据 docs/ch01-design.md 与 docs/lore/势力与地理.md）：

    韩家村 → 青牛镇 → 彩霞山道 → 七玄门 → 炼骨崖 → 神手谷

    彩霞山是七玄门总门所在的山（ch3），青牛镇是山下受其控制的镇子（ch2），
    炼骨崖在门内、考核那天从竹林小路走过去（ch4）——所以炼骨崖排在七玄门之后。

    这一串**一路向北**：每一处都从上一张图的北口出去、落在下一张图的南口，
    进门面朝北（map_spec 规则 29：出门往哪走，进门就朝哪）。曾经有两处掉了头：
    韩家村村口开在南墙、却落在青牛镇南门；炼骨崖从崖顶北出、却落在神手谷北口。
    落点离回程门一两格，多按一下方向键就在两张图之间来回弹。

瓦片 gid 约定（四个 tileset 统一编号，美术补 PNG 时按位对上即可；
PNG 尚未产出，validate.py 与 TileMapLoader 都只查 .tsj 存在，不查 PNG）。

    **渲染侧的另一半在 src/game/MapArt.cpp 的 tileRole / TilePalette。**
    这两半必须一起改：它们分家过一次，渲染器对着另一套语义还用 `gid % 8`
    兜底，于是 gid 8（实心挡路）画成了草地绿、gid 4（遮挡）画成不透明水蓝盖住
    人物，两边各自都自洽，没有任何工具报警。现在由 tests/MapArtTests.cpp 拿全部
    18 张真地图逐层对账，改错一侧就在门禁上红。

    1  主地表（草地 / 土地 / 石地 / 木地板）
    2  道路（土路 / 石板路 / 石阶）
    3  特殊地表（水面 / 药圃翻土 / 碎石崖面）
    4  front 遮挡层（树冠 / 屋檐 / 崖檐），绘制在角色之上
    5  building 主体（墙体 / 屋身 / 岩壁）
    6  overlay 装饰 A（花草）
    7  overlay 装饰 B（碎石 / 裂纹）
    8  building 次体（篱笆 / 摊架 / 石栏 / 石柱）

不变量：building 有实体瓦片处 collision 必须非 0，由 Grid.solid_rect 一手包办，
不要绕开它单独写 building 层（map_spec 第 7 节规则 12）。
"""
import json
import os
import random
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
MAPS = os.path.join(ROOT, "maps")
TSETS = os.path.join(MAPS, "tilesets")

TILE = 32


# ---------------------------------------------------------------------------
# 网格
# ---------------------------------------------------------------------------
class Grid:
    def __init__(self, w, h):
        self.w, self.h = w, h
        self.ground = [0] * (w * h)
        self.overlay = [0] * (w * h)
        self.building = [0] * (w * h)
        self.front = [0] * (w * h)
        self.collision = [0] * (w * h)

    def idx(self, x, y):
        return y * self.w + x

    def fill(self, layer, value):
        setattr(self, layer, [value] * (self.w * self.h))

    def rect(self, layer, x0, y0, x1, y1, value):
        buf = getattr(self, layer)
        for y in range(max(0, y0), min(self.h - 1, y1) + 1):
            for x in range(max(0, x0), min(self.w - 1, x1) + 1):
                buf[y * self.w + x] = value

    def set(self, layer, x, y, value):
        if 0 <= x < self.w and 0 <= y < self.h:
            getattr(self, layer)[y * self.w + x] = value

    def get(self, layer, x, y):
        return getattr(self, layer)[y * self.w + x]

    def walkable(self, x, y):
        return 0 <= x < self.w and 0 <= y < self.h and self.collision[y * self.w + x] == 0

    def solid_rect(self, x0, y0, x1, y1, gid=5):
        """实体块：building + collision 同步（对应 map_spec 规则 12）。"""
        self.rect("building", x0, y0, x1, y1, gid)
        self.rect("collision", x0, y0, x1, y1, 1)

    def open_rect(self, x0, y0, x1, y1):
        self.rect("building", x0, y0, x1, y1, 0)
        self.rect("collision", x0, y0, x1, y1, 0)

    def bfs(self, sx, sy):
        seen = {(sx, sy)}
        q = [(sx, sy)]
        while q:
            x, y = q.pop()
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                n = (x + dx, y + dy)
                if n in seen or not self.walkable(*n):
                    continue
                seen.add(n)
                q.append(n)
        return seen


# ---------------------------------------------------------------------------
# 对象
# ---------------------------------------------------------------------------
def prop(name, value):
    if isinstance(value, bool):
        t = "bool"
    elif isinstance(value, int):
        t = "int"
    else:
        t = "string"
    return {"name": name, "type": t, "value": value}


def obj(name, otype, x, y, w=1, h=1, **props):
    order = ["id", "default", "facing", "target_map", "target_spawn", "transition",
             "role_id", "script", "wander", "mode", "once", "kind", "ref_id", "slots",
             "require_flag", "deny_text_key", "guard_flag", "set_flag",
             "visible_flag", "hidden_flag",
             "table_id", "steps_min", "steps_max"]
    keys = [k for k in order if k in props] + [k for k in props if k not in order]
    return {
        "id": 0,  # 写盘前统一重编号
        "name": name,
        "type": otype,
        "x": x * TILE, "y": y * TILE,
        "width": w * TILE, "height": h * TILE,
        "rotation": 0,
        "visible": True,
        "properties": [prop(k, props[k]) for k in keys],
    }


def spawn(name, x, y, facing, default=False):
    return obj(name, "spawn", x, y, id=name, default=default, facing=facing)


def portal(name, x, y, target_map, target_spawn, w=1, h=1,
           require_flag=None, deny_text_key=None):
    kw = dict(target_map=target_map, target_spawn=target_spawn, transition="fade")
    if require_flag:
        kw["require_flag"] = require_flag
        # 拦住玩家却不说明还差什么，等于没拦：WorldScene::tryStep 被 require_flag
        # 拦下时播的就是这一条，缺了它玩家只会觉得按键没反应。
        kw["deny_text_key"] = deny_text_key
    return obj(name, "portal", x, y, w, h, **kw)


def npc(name, x, y, role_id, facing, script=None, wander=False,
        visible_flag=None, hidden_flag=None):
    kw = dict(role_id=role_id, facing=facing, wander=wander)
    if script:
        kw["script"] = script
    if visible_flag:
        kw["visible_flag"] = visible_flag
    if hidden_flag:
        kw["hidden_flag"] = hidden_flag
    return obj(name, "npc", x, y, **kw)


def trigger(name, x, y, script, mode, once=True, w=1, h=1,
            guard_flag=None, set_flag=None):
    kw = dict(script=script, mode=mode, once=once)
    if guard_flag:
        kw["guard_flag"] = guard_flag
    if set_flag:
        kw["set_flag"] = set_flag
    return obj(name, "trigger", x, y, w, h, **kw)


def facility(name, x, y, kind, w=1, h=1, **extra):
    return obj(name, "facility", x, y, w, h, kind=kind, **extra)


# ---------------------------------------------------------------------------
# 写盘
# ---------------------------------------------------------------------------
def render_map(payload):
    """按既有 .tmj 的排版渲染成文本：图块数据压成一行，其余 2 空格缩进。

    会就地改写 payload 的图块层（data 换成占位串），调用方用完即弃。
    """
    markers = {}
    for i, layer in enumerate(payload["layers"]):
        if layer.get("type") == "tilelayer":
            token = "@@DATA%d@@" % i
            markers[token] = layer["data"]
            layer["data"] = token
    text = json.dumps(payload, ensure_ascii=False, indent=2)
    for token, data in markers.items():
        one = "[\n        " + ",".join(str(v) for v in data) + "\n      ]"
        text = text.replace('"%s"' % token, one)
    return text + "\n"


def dump_map(path, payload):
    with open(path, "wb") as f:
        f.write(render_map(payload).encode("utf-8"))


def tilelayer(name, w, h, data):
    return {"name": name, "type": "tilelayer", "width": w, "height": h,
            "x": 0, "y": 0, "visible": True, "opacity": 1, "data": data}


def build_map(map_id, w, h, tileset, props, grid, objects):
    for i, o in enumerate(objects, 1):
        o["id"] = i
    layers = [
        tilelayer("ground", w, h, grid.ground),
        tilelayer("overlay", w, h, grid.overlay),
        tilelayer("building", w, h, grid.building),
        tilelayer("front", w, h, grid.front),
        tilelayer("collision", w, h, grid.collision),
        {"name": "objects", "type": "objectgroup", "x": 0, "y": 0,
         "visible": True, "opacity": 1, "objects": objects},
    ]
    return {
        "type": "map",
        "version": "1.10",
        "tiledversion": "1.10.2",
        "orientation": "orthogonal",
        "renderorder": "right-down",
        "width": w,
        "height": h,
        "tilewidth": TILE,
        "tileheight": TILE,
        "infinite": False,
        "nextlayerid": 7,
        "nextobjectid": len(objects) + 1,
        "tilesets": [{"firstgid": 1, "source": "tilesets/%s.tsj" % tileset}],
        "properties": [prop(k, v) for k, v in props],
        "layers": layers,
    }


def map_props(map_id, bgm, outdoor=True):
    return [
        ("map_id", map_id),
        ("display_name_key", "ch01.map.%s.name" % map_id[len("ch01_"):]),
        ("region", "qingzhou"),
        ("bgm", bgm),
        ("chapter", 1),
        ("outdoor", outdoor),
        ("can_leave_edge", False),
    ]


def write_tileset(name):
    path = os.path.join(TSETS, name + ".tsj")
    payload = {
        "columns": 8,
        "image": name + ".png",
        "imageheight": 256,
        "imagewidth": 256,
        "margin": 0,
        "name": name,
        "spacing": 0,
        "tilecount": 64,
        "tileheight": 32,
        "tilewidth": 32,
        "type": "tileset",
        "version": "1.10",
        "tiledversion": "1.10.2",
    }
    with open(path, "wb") as f:
        f.write((json.dumps(payload, ensure_ascii=False, indent=2) + "\n").encode("utf-8"))


# ---------------------------------------------------------------------------
# 地图 1：韩家村 40x30（已有，补全）
# ---------------------------------------------------------------------------
def make_hanjiacun():
    W, H = 40, 30
    g = Grid(W, H)
    rnd = random.Random(101)
    g.fill("ground", 1)

    # 村墙（保留原图的外圈碰撞），村口在北墙 x=19,20 开口。
    #
    # 村口曾开在南墙，而青牛镇那头是「南门下接韩家村」：出村往下走，进镇落在镇南门、
    # 面朝北，回村的门就在脚下两格——多按一下「下」就在两张图之间来回弹。
    # 青牛镇 → 彩霞山道 → 七玄门一路向北，所以挪的是这一头：出村往北走，进镇从南门进。
    g.solid_rect(0, 0, W - 1, 0)
    g.solid_rect(0, H - 1, W - 1, H - 1)
    g.solid_rect(0, 0, 0, H - 1)
    g.solid_rect(W - 1, 0, W - 1, H - 1)
    g.rect("building", 0, 0, W - 1, 0, 8)      # 篱笆
    g.rect("building", 0, H - 1, W - 1, H - 1, 8)
    g.rect("building", 0, 0, 0, H - 1, 8)
    g.rect("building", W - 1, 0, W - 1, H - 1, 8)
    g.open_rect(19, 0, 20, 0)                  # 村口

    # 村道：横街 y7-8，主道 x19-20 北通村口、南到韩家门前的岔道口
    g.rect("ground", 6, 7, 33, 8, 2)
    g.rect("ground", 19, 0, 20, 21, 2)
    g.rect("ground", 10, 19, 20, 20, 2)        # 通往韩家院子的岔道

    # 村口那棵老槐树：话别就在它底下（ch01.cunkou.dawn「村口那棵老槐树还是黑的」、
    # ch01.cunkou.depart「娘还站在老槐树底下」）。村口在南墙那些年图上没有这棵树。
    # 树干在 (16,3)：娘站在它东边 (18,2)，烘出来的槐树冠比三格宽，再往东一格就把她整个盖住了。
    g.rect("front", 15, 1, 17, 3, 4)
    g.solid_rect(16, 3, 16, 3, 5)

    # 韩家（原图已有的 6x4 屋子，位置不动）
    g.solid_rect(10, 10, 15, 13)
    g.rect("front", 10, 9, 15, 9, 4)           # 屋檐

    # 另加两处村屋，让村子不再是空盒子
    g.solid_rect(24, 10, 29, 13)
    g.rect("front", 24, 9, 29, 9, 4)
    g.solid_rect(9, 22, 14, 25)
    g.rect("front", 9, 21, 14, 21, 4)
    g.solid_rect(26, 22, 31, 25)
    g.rect("front", 26, 21, 31, 21, 4)

    # 村中老树（原图 front 的 3x3 树冠保留，补上树干碰撞）
    g.rect("front", 2, 2, 4, 4, 4)
    g.solid_rect(3, 4, 3, 4, 5)
    for tx, ty in ((34, 16), (6, 15), (33, 5)):
        g.rect("front", tx - 1, ty - 1, tx + 1, ty + 1, 4)
        g.solid_rect(tx, ty + 1, tx, ty + 1, 5)

    # 菜畦与草丛
    g.rect("ground", 3, 10, 7, 14, 3)
    for _ in range(70):
        x, y = rnd.randrange(1, W - 1), rnd.randrange(1, H - 1)
        if g.walkable(x, y) and g.get("ground", x, y) == 1:
            g.set("overlay", x, y, 6 if rnd.random() < 0.7 else 7)

    objects = [
        spawn("spawn_main", 20, 20, "up", default=True),
        # 从青牛镇南门往下走回来，落在村口里侧、面朝村里。
        spawn("spawn_cunkou", 20, 2, "down"),
        # 三叔本人就是节点 1 的入口，不另挂触发器：同格的 trigger 永远轮不到
        # （WorldScene::interact 先问 npc，命中就 startEvent 并 return），
        # 而 sanshu.lua 结尾自己会 flag.set("ch01.sanshu_met")。
        # 原先那个 trigger_sanshu_recruit 是死对象，见 docs/ch01-review.md 建议-5。
        #
        # 三叔与娘各有两处站位，按文案走：
        #   · 谈完之前在村里——三叔来家里提这件事，娘在屋外（hanmu.lua 只说一句「三叔等你」）；
        #   · 谈完（ch01.sanshu_met）三叔当晚就回去了（ch01.cunkou.mu_echo_yes「三叔昨儿回去」），
        #     天亮两人都在村口老槐树下等他（ch01.cunkou.sanshu_wait / depart），
        #     找他们谁说话都是话别（cunkou_bie.lua），话别（ch01.muqin_bie）之后都不再露面。
        # 从前两人一直站在村里：谈完再找三叔会把提议与二选一整段重演；没见三叔先找娘
        # 会直接演完话别、出村的门就开了；第 4 章回村（按设计韩立躲在树后没走出去，
        # scripts/ch04/huicun.lua 首部）走过去一按，第 1 章的话别又演一遍。
        # 同格互补旗标那种写法在这里不适用——人换了地方；两处各带一半旗标，同一时刻只有一处在场。
        npc("npc_han_sanshu", 20, 18, "han_sanshu", "down", "ch01/sanshu.lua",
            hidden_flag="ch01.sanshu_met"),
        npc("npc_han_mu", 18, 21, "han_mu", "right", "ch01/hanmu.lua",
            hidden_flag="ch01.sanshu_met"),
        npc("npc_han_mu_cunkou", 18, 2, "han_mu", "right", "ch01/cunkou_bie.lua",
            visible_flag="ch01.sanshu_met", hidden_flag="ch01.muqin_bie"),
        npc("npc_han_sanshu_cunkou", 21, 2, "han_sanshu", "left", "ch01/cunkou_bie.lua",
            visible_flag="ch01.sanshu_met", hidden_flag="ch01.muqin_bie"),
        # once 触发器一律要写 set_flag，且必须指到脚本自己在末尾置的那个完成旗标：
        # 引擎就是拿它判断「这一幕演过没有」的（WorldScene::triggerReady）。写别的
        # 旗标、或者不写，这个 once 要么提前作废要么根本不生效。
        #
        # 摆在村口门洞里侧那一排：门洞两侧是篱笆，门只能从这两格往上踩进去，出村必踩。
        trigger("trigger_cunkou_bie", 19, 1, "ch01/cunkou_bie.lua", "enter", True, w=2,
                guard_flag="ch01.sanshu_met", set_flag="ch01.muqin_bie"),
        portal("portal_to_qingniuzhen", 19, 0, "ch01_qingniuzhen", "spawn_from_hanjiacun", w=2,
               require_flag="ch01.muqin_bie", deny_text_key="ch01.block.qingniuzhen"),
    ]
    return build_map("ch01_hanjiacun", W, H, "terrain_hanjiacun",
                     map_props("ch01_hanjiacun", "bgm_village"), g, objects)


# ---------------------------------------------------------------------------
# 地图 2：彩霞山道 40x30（新建）
# ---------------------------------------------------------------------------
def make_caixiashan():
    W, H = 40, 30
    g = Grid(W, H)
    rnd = random.Random(202)
    g.fill("ground", 1)
    g.fill("building", 5)          # 整张先当作山体
    g.fill("collision", 1)

    # 之字形山路：越往上越高，四段折返
    segs = [(3, 22, 6, 29), (3, 22, 20, 25), (17, 12, 20, 25),
            (17, 12, 34, 15), (31, 1, 34, 15)]
    for x0, y0, x1, y1 in segs:
        g.open_rect(x0, y0, x1, y1)
        g.rect("ground", x0, y0, x1, y1, 2)
    g.open_rect(33, 0, 34, 0)      # 东北隘口，上七玄门总门
    g.rect("ground", 33, 0, 34, 0, 2)
    # 山口只留两格：portal 之外的边缘一律封死，免得玩家贴着图边走到地图外沿
    g.solid_rect(3, 29, 3, 29)
    g.solid_rect(6, 29, 6, 29)
    g.open_rect(4, 29, 5, 29)      # 西南山口，下山回青牛镇北门
    g.rect("ground", 4, 29, 5, 29, 2)

    # 路肩：靠崖一侧铺碎石，另一侧留土面，让线性山路有方向感
    for y in range(H):
        for x in range(W):
            if g.walkable(x, y) and (not g.walkable(x - 1, y) or not g.walkable(x + 1, y)):
                if rnd.random() < 0.5:
                    g.set("overlay", x, y, 7)

    # 崖檐与松枝压在路两侧的山体上
    for y in range(H):
        for x in range(W):
            if not g.walkable(x, y) and any(g.walkable(x + dx, y + dy)
                                            for dx, dy in ((0, 1), (0, -1), (1, 0), (-1, 0))):
                if rnd.random() < 0.28:
                    g.set("front", x, y, 4)

    objects = [
        # 山路自西南的青牛镇起，一路折返向上，出东北隘口即七玄门总门
        spawn("spawn_from_qingniuzhen", 4, 27, "up", default=True),
        spawn("spawn_from_qixuanmen", 33, 2, "down"),
        portal("portal_to_qingniuzhen", 4, 29, "ch01_qingniuzhen", "spawn_from_caixiashan", w=2),
        portal("portal_to_qixuanmen", 33, 0, "ch01_qixuanmen", "spawn_main", w=2,
               require_flag="ch01.zhangtie_met", deny_text_key="ch01.block.qixuanmen"),
        trigger("trigger_yu_zhangtie", 17, 18, "ch01/shandao_zhangtie.lua", "enter", True, w=4,
                set_flag="ch01.zhangtie_met"),
        # 张铁在这条山道上待到考核为止（路径行动 zhangtie_dating 的时段），
        # 此后他在炼骨崖（那边的 npc_zhang_tie 从 ch01.climb_done 起在场）。
        # 两处都不挂旗标时，第 2 章以后同一个张铁同时站在三张图上。
        npc("npc_zhang_tie", 18, 16, "zhang_tie", "down", "ch01/shandao_zhangtie.lua",
            hidden_flag="ch01.climb_done"),
    ]
    return build_map("ch01_caixiashan", W, H, "terrain_caixiashan",
                     map_props("ch01_caixiashan", "bgm_mountain_path"), g, objects)


# ---------------------------------------------------------------------------
# 地图 3：青牛镇 48x36（新建）
# ---------------------------------------------------------------------------
def make_qingniuzhen():
    W, H = 48, 36
    g = Grid(W, H)
    rnd = random.Random(303)
    g.fill("ground", 1)

    # 镇墙
    g.solid_rect(0, 0, W - 1, 0)
    g.solid_rect(0, H - 1, W - 1, H - 1)
    g.solid_rect(0, 0, 0, H - 1)
    g.solid_rect(W - 1, 0, W - 1, H - 1)

    # 街巷骨架：内环 + 一条纵贯主街 + 三条横街
    def street(x0, y0, x1, y1):
        g.open_rect(x0, y0, x1, y1)
        g.rect("ground", x0, y0, x1, y1, 2)

    # 先把内部铺满民居，再挖街
    g.solid_rect(1, 1, W - 2, H - 2)
    street(1, 1, W - 2, 2)
    street(1, H - 3, W - 2, H - 2)
    street(1, 1, 2, H - 2)
    street(W - 3, 1, W - 2, H - 2)
    street(22, 1, 25, H - 2)            # 主街
    street(1, 8, W - 2, 9)
    street(1, 17, W - 2, 18)
    street(1, 26, W - 2, 27)

    # 民居之间的小巷，让镇子密而不闷
    for x in (7, 13, 19, 31, 37, 43):
        street(x, 3, x, H - 4)
    # 屋身重铺一遍（上面挖街时被清掉的屋檐要补回）
    for y in range(1, H - 1):
        for x in range(1, W - 1):
            if not g.walkable(x, y):
                g.set("building", x, y, 5)

    # 报名处广场（镇东大院）。台词一律叫它「镇东那片空场」（ch01.zhenshang.crowd、ch01.block.caixiashan）；
    # 「镇口」只指南北两道镇门（天不亮在镇口等齐、从镇口动身）。
    street(32, 19, 42, 25)
    g.rect("ground", 32, 19, 42, 25, 1)
    g.rect("overlay", 32, 19, 42, 25, 7)
    # 报名的两张长案（ch01.zhenshang.crowd「支着两张长桌」、kg_register「桌后坐着……面前摊着册子」）。
    # 考官站在两案之间的后头 (37,21)，正前方 (37,22) 留一格过道，从正面、两侧都找得到他说话。
    # 案子各 2 格：3 格长的会被 tools/artgen 当成「篱笆线」，两案之间那格过道就被画成一道门。
    # 美术种类在 data/visual/maps.json 里指成 desk（案上纸、砚、笔），案子底下的地面也在那里指成广场石
    #（推断只给能走的格铺石板，不指的话案子四周露一圈底色草地）。
    g.solid_rect(35, 22, 36, 22, 8)
    g.solid_rect(38, 22, 39, 22, 8)

    # 主街两侧摊架：交替占一格，把街挤窄，人多的感觉靠空间挤出来
    for y in range(3, H - 3):
        if y % 4 == 0:
            g.solid_rect(22, y, 22, y, 8)
        elif y % 4 == 2:
            g.solid_rect(25, y, 25, y, 8)
    # 南北镇门必须留空
    g.open_rect(23, 0, 24, 0)
    g.open_rect(23, H - 1, 24, H - 1)
    g.rect("ground", 23, 0, 24, 0, 2)
    g.rect("ground", 23, H - 1, 24, H - 1, 2)
    g.open_rect(22, 1, 25, 3)
    g.open_rect(22, H - 4, 25, H - 2)

    # 屋檐压在街上
    for y in range(1, H - 1):
        for x in range(1, W - 1):
            if g.walkable(x, y) and not g.walkable(x, y - 1) and g.get("building", x, y - 1) == 5:
                if rnd.random() < 0.55:
                    g.set("front", x, y, 4)
    for _ in range(90):
        x, y = rnd.randrange(1, W - 1), rnd.randrange(1, H - 1)
        if g.walkable(x, y) and g.get("overlay", x, y) == 0:
            g.set("overlay", x, y, 7 if rnd.random() < 0.6 else 6)

    objects = [
        # 南门下接韩家村（来路），北门上接彩霞山道（往七玄门）
        spawn("spawn_from_hanjiacun", 23, 33, "up", default=True),
        spawn("spawn_from_caixiashan", 23, 2, "down"),
        portal("portal_to_hanjiacun", 23, 35, "ch01_hanjiacun", "spawn_cunkou", w=2),
        portal("portal_to_caixiashan", 23, 0, "ch01_caixiashan", "spawn_from_qingniuzhen", w=2,
               require_flag="ch01.baoming_done", deny_text_key="ch01.block.caixiashan"),

        # 报名处：七玄门在镇上设的收徒大院。考官收完名册就随人上山：次日天不亮他领着这一拨人走了，
        # 晌午山道遇张铁（ch01.zhangtie_met）之后这里不再有人坐着（路径行动 kaoguan_dating 的时段）。
        # 从前挂到考完（ch01.climb_done），次日走回镇上找他，他还会说「明日天不亮在镇口等齐」。
        facility("facility_baoming_chu", 36, 19, "board", w=2, h=2, ref_id="board_qixuan_zhaotu"),
        npc("npc_qixuan_kaoguan", 37, 21, "qixuan_kaoguan", "down", "ch01/qingniuzhen_baoming.lua",
            hidden_flag="ch01.zhangtie_met"),
        trigger("trigger_baoming", 35, 25, "ch01/qingniuzhen_baoming.lua", "enter", True, w=4,
                set_flag="ch01.baoming_done"),

        # 三个可闲聊 NPC：教玩家按键与人说话。他们说的全是收徒那几日的事
        # （卖药的「每年这几日他都来」），第 1 章一过就散了——第 4 章那个卖药郎
        # 是同一个走镇行商，在山下镇（ch04_shanxiazhen 的 npc_zhen_min_yaofan）。
        npc("npc_zhen_min_jiuke", 15, 8, "zhen_min_jiuke", "down", "ch01/zhenmin_jiuke.lua",
            hidden_flag="ch01.done"),
        npc("npc_zhen_min_yaofan", 10, 9, "zhen_min_yaofan", "down", "ch01/zhenmin_yaofan.lua",
            hidden_flag="ch01.done"),
        npc("npc_zhen_min_jiaofu", 24, 28, "zhen_min_jiaofu", "up", "ch01/zhenmin_jiaofu.lua",
            hidden_flag="ch01.done"),

        # 春香酒楼（三叔的店，原著 ch2）
        facility("facility_chunxiang_lou", 16, 8, "shop", w=2, ref_id="shop_village_general"),

        # 街上的人：不挂脚本，只为把镇子填满，常住镇上、各章都在。
        # 每人一个自己的 role：从前这六个借用上面三个打探 NPC 的 role，
        # 名牌上于是有三个「卖药郎」、三个「脚夫」、三个「镇上酒客」（规则 30）。
        npc("npc_lu_ren_a", 23, 12, "qingniu_laohan", "left", wander=True),
        npc("npc_lu_ren_b", 24, 21, "qingniu_furen", "up", wander=True),
        npc("npc_lu_ren_c", 23, 30, "qingniu_chebashi", "right", wander=True),
        npc("npc_lu_ren_d", 40, 23, "qingniu_chatan_huoji", "left", wander=True),
        npc("npc_lu_ren_e", 34, 24, "qingniu_haitong", "right", wander=True),
        npc("npc_lu_ren_f", 8, 18, "qingniu_modaojiang", "down", wander=True),
    ]
    return build_map("ch01_qingniuzhen", W, H, "terrain_qingniuzhen",
                     map_props("ch01_qingniuzhen", "bgm_town"), g, objects)


# ---------------------------------------------------------------------------
# 地图 4：炼骨崖 32x40（新建，纵向狭长）
#
# 崖底接七玄门（门内的竹林小路走过来），崖顶翻过去就是神手谷。玩家在崖底考核、
# 在崖底放榜、在崖底被墨大夫点名，然后沿着刚才比试的这条崖路一步步走上去进谷：
# 攀爬在玩法上是抽象的三选一，在地形上是一条实走的路，两者共用同一段崖壁。
# ---------------------------------------------------------------------------
def make_liangu_ya():
    W, H = 32, 40
    g = Grid(W, H)
    rnd = random.Random(404)
    g.fill("ground", 1)
    g.fill("building", 5)
    g.fill("collision", 1)

    # 自下而上：崖底广场 → 缓坡 → 歇脚台 → 险径 → 回望台 → 绝壁 → 崖顶
    # 通道宽度 24 → 12 → 7 → 3，越往上越窄，攀爬的压迫感全靠这条收束线。
    tiers = [
        ("plaza", 4, 33, 27, 38, 1),     # 崖底广场
        ("slope", 10, 26, 21, 32, 3),    # 第一段：缓坡，宽 12
        ("ledgeA", 8, 24, 23, 25, 1),    # 歇脚台
        ("path", 13, 16, 19, 23, 3),     # 第二段：险径，宽 7
        ("ledgeB", 11, 14, 21, 15, 1),   # 回望台
        ("cliff", 15, 6, 17, 13, 3),     # 第三段：绝壁，宽 3
        ("top", 6, 1, 25, 5, 1),         # 崖顶
    ]
    for _name, x0, y0, x1, y1, gid in tiers:
        g.open_rect(x0, y0, x1, y1)
        g.rect("ground", x0, y0, x1, y1, gid)

    g.open_rect(15, 39, 16, 39)          # 崖底南口，回七玄门
    g.rect("ground", 15, 39, 16, 39, 2)
    g.open_rect(15, 0, 16, 0)            # 崖顶北口，翻过去进神手谷（落在谷南口）
    g.rect("ground", 15, 0, 16, 0, 2)

    # 石阶：每段路中央铺出一条踏步，视觉上把三段串成一条向上的线
    for y in range(26, 33):
        g.rect("ground", 15, y, 16, y, 2)
    for y in range(16, 24):
        g.rect("ground", 15, y, 16, y, 2)
    for y in range(6, 14):
        g.rect("ground", 15, y, 16, y, 2)
    g.rect("ground", 15, 24, 16, 25, 2)
    g.rect("ground", 15, 14, 16, 15, 2)
    g.rect("ground", 15, 1, 16, 5, 2)
    g.rect("ground", 15, 33, 16, 38, 2)

    # 崖檐：越往上，压在头顶的岩体越多
    for y in range(H):
        for x in range(W):
            if not g.walkable(x, y) and any(g.walkable(x + dx, y + dy)
                                            for dx, dy in ((0, 1), (0, -1), (1, 0), (-1, 0))):
                density = 0.5 - y * 0.009      # 崖顶附近最密
                if rnd.random() < max(0.12, density):
                    g.set("front", x, y, 4)
    # 碎石：段位越高越碎
    for y in range(H):
        for x in range(W):
            if g.walkable(x, y) and g.get("ground", x, y) == 3 and rnd.random() < 0.35:
                g.set("overlay", x, y, 7)
            elif g.walkable(x, y) and rnd.random() < 0.08:
                g.set("overlay", x, y, 6)

    objects = [
        spawn("spawn_from_qixuanmen", 15, 37, "up", default=True),
        spawn("spawn_from_shenshougu", 15, 2, "down"),
        portal("portal_to_qixuanmen", 15, 39, "ch01_qixuanmen", "spawn_from_liangu_ya", w=2),
        portal("portal_to_shenshougu", 15, 0, "ch01_shenshougu", "spawn_from_liangu_ya", w=2,
               require_flag="ch01.fangbang_done", deny_text_key="ch01.block.shenshougu"),

        # 考核：崖壁下起攀。墨大夫点完名（ch01.shoutu_done）这一场就散了；
        # 考完之后再找他说话只得一句（lianguya_kaohe.lua 开头那道判断），不会把四轮重考一遍。
        npc("npc_qixuan_kaoguan", 13, 34, "qixuan_kaoguan", "down", "ch01/lianguya_kaohe.lua",
            hidden_flag="ch01.shoutu_done"),
        trigger("trigger_kaohe_start", 14, 32, "ch01/lianguya_kaohe.lua", "interact", True, w=4,
                set_flag="ch01.climb_done"),
        # 张铁考完才在崖底（路径行动 zhangtie_dating2 的时段）——考核之前他还在彩霞山道上，
        # 两处同时在场就是一人两地；随墨大夫进谷（ch01.shoutu_done）之后离开。
        npc("npc_zhang_tie", 11, 35, "zhang_tie", "right",
            visible_flag="ch01.climb_done", hidden_flag="ch01.shoutu_done"),

        # 放榜与收徒：都在崖底，未达顶的那份落差要留在原地
        facility("facility_bangwen", 20, 33, "board", w=2, h=2, ref_id="board_liangu_ya_bangwen"),
        trigger("trigger_fangbang", 19, 35, "ch01/fangbang.lua", "enter", True, w=2,
                guard_flag="ch01.climb_done", set_flag="ch01.fangbang_done"),
        # 墨大夫放榜之后才从石阶上下来（ch01.modaifu.appear），点完名就上去了（modaifu.close）。
        # 从前他从开局就站在这儿：考核之前找他说话会先被收徒，第 3 章他死后也还站着。
        npc("npc_mo_daifu", 22, 35, "mo_daifu", "left", "ch01/modaifu_shoutu.lua",
            visible_flag="ch01.fangbang_done", hidden_flag="ch01.shoutu_done"),
    ]
    return build_map("ch01_liangu_ya", W, H, "terrain_liangu_ya",
                     map_props("ch01_liangu_ya", "bgm_cliff"), g, objects)


# ---------------------------------------------------------------------------
# 地图 5：神手谷 40x30（新建）
# ---------------------------------------------------------------------------
def make_shenshougu():
    W, H = 40, 30
    g = Grid(W, H)
    rnd = random.Random(505)
    g.fill("ground", 1)

    # 谷壁
    g.solid_rect(0, 0, W - 1, 0)
    g.solid_rect(0, H - 1, W - 1, H - 1)
    g.solid_rect(0, 0, 1, H - 1)
    g.solid_rect(W - 2, 0, W - 1, H - 1)
    # 谷口在南墙，通炼骨崖：从崖顶往北翻过来，进谷面朝北。
    # 谷口曾开在北墙——崖顶往北出、落在谷北口面朝南，回崖的门就在脚下两格，
    # 按住「上」便在两张图之间来回弹。北墙现在归第 2 章（谷北口通药圃，genmaps_ch02.py）。
    # 顺带对上了文案：「谷口进来那块平石」（ch01.shenshougu.sit_hint）就是南口东边那片打坐石坪。
    g.open_rect(19, H - 1, 20, H - 1)
    g.rect("ground", 19, H - 1, 20, H - 1, 2)

    # 谷中小径
    g.rect("ground", 19, 1, 20, H - 1, 2)
    g.rect("ground", 4, 13, 35, 14, 2)
    g.rect("ground", 24, 4, 25, 13, 2)
    g.rect("ground", 30, 14, 31, 25, 2)

    # 居所：墨大夫的堂屋与韩立的偏屋
    g.solid_rect(5, 5, 11, 9)
    g.rect("front", 5, 4, 11, 4, 4)
    g.solid_rect(14, 6, 17, 9)
    g.rect("front", 14, 5, 17, 5, 4)
    g.solid_rect(5, 18, 9, 21)
    g.rect("front", 5, 17, 9, 17, 4)

    # 药圃：四畦翻土，中间留田埂
    for x0, y0 in ((26, 4), (32, 4), (26, 9), (32, 9)):
        g.rect("ground", x0, y0, x0 + 4, y0 + 2, 3)
        g.rect("overlay", x0, y0, x0 + 4, y0 + 2, 6)

    # 水潭
    g.rect("ground", 4, 22, 9, 26, 3)
    g.solid_rect(4, 22, 9, 26, 0)
    g.rect("building", 4, 22, 9, 26, 0)   # 水面不画墙，只挡路

    # 打坐台：谷东南的石坪
    g.rect("ground", 29, 19, 35, 25, 1)
    g.rect("overlay", 29, 19, 35, 25, 7)
    g.solid_rect(29, 19, 29, 19, 8)
    g.solid_rect(35, 19, 35, 19, 8)
    g.solid_rect(29, 25, 29, 25, 8)
    g.solid_rect(35, 25, 35, 25, 8)

    # 谷中草木
    for tx, ty in ((13, 20), (22, 22), (37, 17), (12, 25), (34, 16)):
        g.rect("front", tx - 1, ty - 1, tx + 1, ty + 1, 4)
        g.solid_rect(tx, ty, tx, ty, 5)
    for _ in range(110):
        x, y = rnd.randrange(2, W - 2), rnd.randrange(1, H - 1)
        if g.walkable(x, y) and g.get("ground", x, y) == 1 and g.get("overlay", x, y) == 0:
            g.set("overlay", x, y, 6 if rnd.random() < 0.75 else 7)

    objects = [
        spawn("spawn_from_liangu_ya", 19, 28, "up", default=True),
        portal("portal_to_liangu_ya", 19, 29, "ch01_liangu_ya", "spawn_from_shenshougu", w=2),

        trigger("trigger_ru_gu_anding", 19, 25, "ch01/shenshougu_anding.lua", "enter", True, w=2,
                guard_flag="ch01.shoutu_done", set_flag="ch01.shenshougu_arrived"),
        # 他领人进谷、安顿下来之后才在堂前（路径行动 modaifu_dating 的时段），授完口诀
        # （ch01.done，与 koujue_received 同在 shenshougu_koujue.lua 末尾）就不在这儿了：
        # 第 2 章他在药圃（ch02_yaopu），第 3 章在密室。不挂旗标时他从开局到死后
        # 一直站在这里，而且一按就把「授口诀」整段重演、重设 ch01.xiulian_taidu。
        npc("npc_mo_daifu", 12, 10, "mo_daifu", "down", "ch01/shenshougu_koujue.lua",
            visible_flag="ch01.shenshougu_arrived", hidden_flag="ch01.done"),

        # 与第 2 章的药圃是同一片地，故用同一个 id。两个 id 会在 GameState 里
        # 变成两块互不相干的田，玩家第 1 章种下的药到第 2 章就从眼前消失了。
        # 这里仍报 6 槽，第 2 章 field.unlock(..., 8) 把它补到 8 槽——
        # FieldUnlock 只补足不重建，也从不缩小，所以回头走进本图不会把田缩回去。
        facility("facility_yaopu", 28, 7, "field", w=2, h=2, ref_id="shenshougu_yaopu", slots=6),
        facility("facility_dazuo", 32, 22, "meditate", w=2, h=2),
        trigger("trigger_dazuo_jiaoxue", 31, 26, "ch01/shenshougu_dazuo.lua", "enter", True, w=4,
                guard_flag="ch01.shenshougu_arrived", set_flag="ch01.dazuo_done"),
        facility("facility_cunji", 17, 13, "save"),
    ]
    return build_map("ch01_shenshougu", W, H, "terrain_shenshougu",
                     map_props("ch01_shenshougu", "bgm_valley"), g, objects)


# ---------------------------------------------------------------------------
# 地图 6：七玄门 24x18（已有，本章只路过，只补进出 portal）
# ---------------------------------------------------------------------------
def patch_qixuanmen():
    path = os.path.join(MAPS, "ch01_qixuanmen.tmj")
    payload = json.loads(open(path, "rb").read().decode("utf-8"))
    layers = {l["name"]: l for l in payload["layers"]}
    objs = layers["objects"]["objects"]

    keep = [o for o in objs if o["name"] == "spawn_main"]
    # spawn_main 现在是从彩霞山道上山进门的落点，朝向改为面朝门内（原为 down，
    # 那是 P2 桩子的朝向，会让玩家一进门就背对宗门）。
    for prop_entry in keep[0]["properties"]:
        if prop_entry["name"] == "facing":
            prop_entry["value"] = "up"
    keep.append(spawn("spawn_from_liangu_ya", 4, 3, "down"))
    keep.append(portal("portal_to_caixiashan", 11, 16, "ch01_caixiashan",
                       "spawn_from_qixuanmen", w=2))
    keep.append(portal("portal_to_liangu_ya", 3, 1, "ch01_liangu_ya",
                       "spawn_from_qixuanmen", w=2))
    for i, o in enumerate(keep, 1):
        o["id"] = i
    layers["objects"]["objects"] = keep
    payload["nextobjectid"] = len(keep) + 1
    return payload


# ---------------------------------------------------------------------------
def build_all():
    """六张图的完整产出 —— 与 maps/ch01_*.tmj 应当一字不差。"""
    built = {
        "ch01_hanjiacun": make_hanjiacun(),
        "ch01_caixiashan": make_caixiashan(),
        "ch01_qingniuzhen": make_qingniuzhen(),
        "ch01_liangu_ya": make_liangu_ya(),
        "ch01_shenshougu": make_shenshougu(),
        "ch01_qixuanmen": patch_qixuanmen(),
    }
    # 神手谷北墙那处通往 ch02_yaopu 的谷口归第 2 章的生成器所有（见它的 docstring）。
    # 不在这里补回去，单跑本脚本就会把第 2 章整章从第 1 章割断，而且不报错：
    # validate.py 的连通性检查是逐图做的，没有跨图可达性这一条。
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    import genmaps_ch02  # 延迟导入：模块层导入会与它的 from genmaps import ... 成环

    built["ch01_shenshougu"] = genmaps_ch02.patch_shenshougu(built["ch01_shenshougu"])

    # 第 3 章又在神手谷开了两处口子（谷西豁口 → ch03_guwai、堂屋南墙 → ch03_mishi）
    # 外加节点 2「墨大夫归谷」的踏入触发，同理归 genmaps_ch03.py 所有。
    # 顺序要紧：它补在第 2 章那一处之后，对象表的先后才与 maps/ 里的一致。
    import genmaps_ch03

    built["ch01_shenshougu"] = genmaps_ch03.patch_shenshougu(built["ch01_shenshougu"])

    # 第 4 章在韩家村村东开了一处车道口（往山下镇），外加节点 11 的三处挂点，
    # 同理归 genmaps_ch04.py 所有。不在这里补回去，重跑本脚本就会把第 4 章
    # 整章从游戏里割断 —— 那四张图都只从这一处进得去。
    import genmaps_ch04

    built["ch01_hanjiacun"] = genmaps_ch04.patch_hanjiacun(built["ch01_hanjiacun"])

    # 第 5 章又在韩家村村东开了一道新口（往岚州渡口），同理归 genmaps_ch05.py 所有。
    # 顺序要紧：补在第 4 章那一次之后，对象表的先后才与 maps/ 里的一致。
    # 不补回去，重跑本脚本就会把第 5 章整章割断——渡口只从这一处进得去。
    import genmaps_ch05

    built["ch01_hanjiacun"] = genmaps_ch05.patch_hanjiacun(built["ch01_hanjiacun"])
    return built


def objects_of(payload):
    for layer in payload["layers"]:
        if layer.get("type") == "objectgroup":
            return {o["name"]: o for o in layer["objects"]}
    return {}


def describe_drift(have, want):
    """maps/ 里的这张图与重跑产出差在哪，一行一条。"""
    out = []
    hl = {l["name"]: l for l in have["layers"] if l.get("type") == "tilelayer"}
    wl = {l["name"]: l for l in want["layers"] if l.get("type") == "tilelayer"}
    for name, layer in wl.items():
        a = (hl.get(name) or {}).get("data") or []
        b = layer["data"]
        if a != b:
            n = sum(1 for i, v in enumerate(b) if i >= len(a) or a[i] != v)
            out.append("图层 %s 差 %d 格" % (name, n))
    ho, wo = objects_of(have), objects_of(want)
    for name in sorted(set(ho) | set(wo)):
        if name not in wo:
            out.append("对象 %s 只在 maps/ 里有 —— 重跑会把它抹掉" % name)
            continue
        if name not in ho:
            out.append("对象 %s 只在重跑产出里有 —— maps/ 里缺" % name)
            continue
        pa = {q["name"]: q["value"] for q in ho[name].get("properties", [])}
        pb = {q["name"]: q["value"] for q in wo[name].get("properties", [])}
        for key in sorted(set(pa) | set(pb)):
            if pa.get(key) != pb.get(key):
                out.append("对象 %s 的 %s：maps/=%r 重跑=%r" % (name, key, pa.get(key), pb.get(key)))
        for key in ("x", "y", "width", "height"):
            if ho[name].get(key) != wo[name].get(key):
                out.append("对象 %s 的 %s：maps/=%r 重跑=%r"
                           % (name, key, ho[name].get(key), wo[name].get(key)))
    return out


def check_maps(built):
    """只比对不写盘：生成器与 maps/ 一旦走散就在这里拦住。

    漂移是静默的 —— 抹掉一处闸门不会让任何工具报错，玩家却能从第一段直接
    走到第五段。所以「重跑会不会改动 maps/」本身必须是一条门禁。

    两个生成器共用这一份：各写一遍迟早会出现「这边报漂移那边说没事」。
    """
    drifted = 0
    for map_id, payload in built.items():
        path = os.path.join(MAPS, map_id + ".tmj")
        if not os.path.exists(path):
            print("[漂移] %s：maps/ 里没有这张图" % map_id)
            drifted += 1
            continue
        have_text = open(path, "rb").read().decode("utf-8")
        want_text = render_map(payload)
        if have_text == want_text:
            continue
        drifted += 1
        print("[漂移] %s" % map_id)
        for line in describe_drift(json.loads(have_text), json.loads(want_text)) or ["仅排版不同"]:
            print("        " + line)
    if drifted:
        print()
        print("%d 张图与生成器不一致：重跑生成器会改动它们。" % drifted)
        print("请把差异补进生成器的对象表后再重跑，不要直接编辑 tmj。")
        return 1
    print("MAPGEN_IN_SYNC：%d 张图与生成器一字不差" % len(built))
    return 0


def check():
    """全仓漂移门禁：第 1 章六张 + 第 2 章四张 + 第 3 章四张 + 第 4 章四张 + 第 5 章六张。

    第 2、3 章的图归各自的 genmaps_chNN.py 产出，漂移同样静默、同样会抹掉闸门
    （ch02.renyao_done / ch02.dazuo_done / ch02.duan2_start、
    ch03.shichong_wan / ch03.beidu_done / ch03.andao_zhan 那几道）。一并盯住，
    build.bat 里那一行就不必为每新增一章再加一条 —— 但**新增一章要在这里加一段**，
    否则那一章等于没有第二道闸门。
    """
    built = build_all()
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    import genmaps_ch02
    import genmaps_ch03
    import genmaps_ch04
    import genmaps_ch05

    for chapter in (genmaps_ch02, genmaps_ch03, genmaps_ch04, genmaps_ch05):
        for map_id, payload in chapter.build_all().items():
            # 神手谷三边都产出：以本文件的为准（那是整张重建 + 两层补丁，更严）。
            built.setdefault(map_id, payload)
    return check_maps(built)


def main(argv=None):
    argv = list(sys.argv[1:] if argv is None else argv)
    if "--check" in argv:
        return check()

    for name in ("terrain_caixiashan", "terrain_qingniuzhen",
                 "terrain_liangu_ya", "terrain_shenshougu"):
        write_tileset(name)

    for map_id, payload in build_all().items():
        dump_map(os.path.join(MAPS, map_id + ".tmj"), payload)
        print("wrote", map_id)
    return 0


if __name__ == "__main__":
    sys.exit(main())
