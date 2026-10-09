"""Chapter 8 maps and additive patches for five existing maps.

The chapter-5/6 entry points reapply these patches after their own construction.
All new maps are fully connected; plot teleports save walking, not topology.
"""
from __future__ import annotations

import copy
import os
import random
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import genmaps
from genmaps import Grid, build_map, check_maps, dump_map, facility, npc, obj, portal, prop, spawn, trigger, write_tileset

MAP_SPECS = [
  [
    "dongfu",
    48,
    40,
    "jianzhou",
    "bgm_mountain_path",
    "洞府"
  ],
  [
    "tianxing_fangshi",
    48,
    36,
    "yuanwu",
    "bgm_town",
    "天星宗坊市"
  ],
  [
    "yanlingbao",
    56,
    40,
    "linzhou",
    "bgm_town",
    "燕翎堡"
  ],
  [
    "lingkuang",
    48,
    40,
    "yue_border",
    "bgm_cliff",
    "灵矿"
  ],
  [
    "jinguyuan",
    48,
    36,
    "yue_border",
    "bgm_wild",
    "金鼓原大营"
  ],
  [
    "jinmacheng",
    48,
    36,
    "yuanwu",
    "bgm_town",
    "金马城"
  ],
  [
    "yuejing",
    64,
    48,
    "yuejing",
    "bgm_town",
    "越京"
  ],
  [
    "jiayuan_shanlin",
    48,
    36,
    "lanzhou",
    "bgm_night",
    "嘉元城外山林"
  ]
]
NODES = [
  [
    "ch06_baiyaoyuan",
    "trigger_shixiong",
    "shixiong.lua",
    [
      18,
      8
    ],
    "interact",
    "ch07.done",
    "ch08.shixiong",
    True
  ],
  [
    "ch06_huangfenggu",
    "trigger_dengji",
    "dengji.lua",
    [
      26,
      8
    ],
    "interact",
    "ch08.shixiong",
    "ch08.dengji",
    True
  ],
  [
    "ch08_dongfu",
    "trigger_shanfeng",
    "shanfeng.lua",
    [
      35,
      17
    ],
    "interact",
    "ch08.dengji",
    "ch08.lingquan",
    True
  ],
  [
    "ch08_dongfu",
    "trigger_kaifu",
    "kaifu.lua",
    [
      20,
      16
    ],
    "interact",
    "ch08.lingquan",
    "ch08.kaifu",
    True
  ],
  [
    "ch08_dongfu",
    "trigger_chucang",
    "chucang.lua",
    [
      10,
      8
    ],
    "interact",
    "ch08.kaifu",
    "ch08.qiannian",
    True
  ],
  [
    "ch08_tianxing_fangshi",
    "trigger_tianxing_ru",
    "tianxing_ru.lua",
    [
      24,
      32
    ],
    "enter",
    "ch08.qiannian",
    "ch08.fangshi",
    True
  ],
  [
    "ch08_tianxing_fangshi",
    "trigger_xulao",
    "xulao.lua",
    [
      10,
      11
    ],
    "interact",
    "ch08.fangshi",
    "ch08.xulao",
    True
  ],
  [
    "ch08_tianxing_fangshi",
    "trigger_midian",
    "midian.lua",
    [
      35,
      23
    ],
    "interact",
    "ch08.xulao",
    "ch08.midian",
    True
  ],
  [
    "ch08_tianxing_fangshi",
    "trigger_qiyunxiao",
    "qiyunxiao.lua",
    [
      40,
      29
    ],
    "interact",
    "ch08.midian",
    "ch08.qiyunxiao",
    True
  ],
  [
    "ch08_tianxing_fangshi",
    "trigger_quqi",
    "quqi.lua",
    [
      10,
      17
    ],
    "interact",
    "ch08.qiyunxiao",
    "ch08.quqi",
    True
  ],
  [
    "ch08_dongfu",
    "trigger_zhenyan",
    "zhenyan.lua",
    [
      19,
      29
    ],
    "interact",
    "ch08.quqi",
    "ch08.buzhen1",
    True
  ],
  [
    "ch08_dongfu",
    "trigger_shichuang",
    "shichuang.lua",
    [
      7,
      16
    ],
    "interact",
    "ch08.buzhen1",
    "ch08.yexi",
    True
  ],
  [
    "ch08_dongfu",
    "trigger_dating",
    "dating.lua",
    [
      24,
      8
    ],
    "interact",
    "ch08.yexi",
    "ch08.gufang",
    True
  ],
  [
    "ch08_dongfu",
    "trigger_lvbo",
    "lvbo.lua",
    [
      37,
      27
    ],
    "interact",
    "ch08.gufang",
    "ch08.lvbo",
    True
  ],
  [
    "ch08_dongfu",
    "trigger_shuan",
    "shuan.lua",
    [
      11,
      16
    ],
    "interact",
    "ch08.lvbo",
    "ch08.shuye",
    True
  ],
  [
    "ch08_dongfu",
    "trigger_biguan",
    "biguan.lua",
    [
      23,
      17
    ],
    "interact",
    "ch08.shuye",
    "ch08.biguan",
    True
  ],
  [
    "ch08_dongfu",
    "trigger_chuanyin",
    "chuanyin.lua",
    [
      39,
      27
    ],
    "interact",
    "ch08.biguan",
    "ch08.hongfu",
    True
  ],
  [
    "ch08_yanlingbao",
    "trigger_biwuchang",
    "biwuchang.lua",
    [
      27,
      33
    ],
    "enter",
    "ch08.hongfu",
    "ch08.biwu",
    True
  ],
  [
    "ch08_yanlingbao",
    "trigger_caihuan",
    "caihuan.lua",
    [
      19,
      27
    ],
    "interact",
    "ch08.biwu",
    "ch08.caihuan",
    True
  ],
  [
    "ch08_yanlingbao",
    "trigger_tianheju",
    "tianheju.lua",
    [
      35,
      26
    ],
    "interact",
    "ch08.caihuan",
    "ch08.tianheju",
    True
  ],
  [
    "ch08_yanlingbao",
    "trigger_xifeng",
    "xifeng.lua",
    [
      8,
      5
    ],
    "enter",
    "ch08.tianheju",
    "ch08.tuoshen",
    True
  ],
  [
    "ch08_lingkuang",
    "trigger_yaodong",
    "yaodong.lua",
    [
      8,
      33
    ],
    "enter",
    "ch08.tuoshen",
    "ch08.lingkuang",
    True
  ],
  [
    "ch08_lingkuang",
    "trigger_jingshi",
    "jingshi.lua",
    [
      14,
      23
    ],
    "interact",
    "ch08.lingkuang",
    "ch08.kuilei",
    True
  ],
  [
    "ch08_lingkuang",
    "trigger_zhenbian",
    "zhenbian.lua",
    [
      15,
      15
    ],
    "interact",
    "ch08.kuilei",
    "ch08.shouzhen",
    True
  ],
  [
    "ch08_lingkuang",
    "trigger_judong",
    "judong.lua",
    [
      33,
      30
    ],
    "enter",
    "ch08.shouzhen",
    "ch08.zhanzhu",
    True
  ],
  [
    "ch08_lingkuang",
    "trigger_chuansongzhen",
    "chuansongzhen.lua",
    [
      40,
      33
    ],
    "interact",
    "ch08.zhanzhu",
    "ch08.nuoyi",
    True
  ],
  [
    "ch08_dongfu",
    "trigger_quanyan",
    "quanyan.lua",
    [
      25,
      17
    ],
    "interact",
    "ch08.nuoyi",
    "ch08.zhongqi",
    True
  ],
  [
    "ch08_dongfu",
    "trigger_feifu",
    "feifu.lua",
    [
      35,
      24
    ],
    "enter",
    "ch08.zhongqi",
    "ch08.zhaoji",
    True
  ],
  [
    "ch08_jinguyuan",
    "trigger_daoying",
    "daoying.lua",
    [
      24,
      31
    ],
    "enter",
    "ch08.zhaoji",
    "ch08.yinian",
    True
  ],
  [
    "ch08_jinguyuan",
    "trigger_jiaoyisuo",
    "jiaoyisuo.lua",
    [
      9,
      15
    ],
    "interact",
    "ch08.yinian",
    "ch08.qiaoqian",
    True
  ],
  [
    "ch08_jinguyuan",
    "trigger_chuzhen",
    "chuzhen.lua",
    [
      23,
      28
    ],
    "interact",
    "ch08.qiaoqian",
    "ch08.daji",
    True
  ],
  [
    "ch08_jinguyuan",
    "trigger_lizhu",
    "lizhu.lua",
    [
      34,
      8
    ],
    "enter",
    "ch08.daji",
    "ch08.nangong",
    True
  ],
  [
    "ch08_jinmacheng",
    "trigger_chaguan",
    "chaguan.lua",
    [
      29,
      25
    ],
    "interact",
    "ch08.nangong",
    "ch08.jinma",
    True
  ],
  [
    "ch08_jinmacheng",
    "trigger_biyun",
    "biyun.lua",
    [
      9,
      9
    ],
    "enter",
    "ch08.jinma",
    "ch08.jiuren",
    True
  ],
  [
    "ch08_jinmacheng",
    "trigger_xin_zhuwu",
    "xin_zhuwu.lua",
    [
      10,
      16
    ],
    "interact",
    "ch08.jiuren",
    "ch08.yueding",
    True
  ],
  [
    "ch08_yuejing",
    "trigger_qinmen",
    "qinmen.lua",
    [
      46,
      37
    ],
    "interact",
    "ch08.yueding",
    "ch08.qinzhai",
    True
  ],
  [
    "ch08_yuejing",
    "trigger_yefang",
    "yefang.lua",
    [
      48,
      29
    ],
    "interact",
    "ch08.qinzhai",
    "ch08.fengwu",
    True
  ],
  [
    "ch08_yuejing",
    "trigger_jiulou_yj",
    "jiulou_yj.lua",
    [
      54,
      23
    ],
    "interact",
    "ch08.fengwu",
    "ch08.qb_mengshan",
    True
  ],
  [
    "ch08_yuejing",
    "trigger_wangfu",
    "wangfu.lua",
    [
      21,
      34
    ],
    "interact",
    "ch08.qb_mengshan",
    "ch08.qb_shouyan",
    True
  ],
  [
    "ch08_yuejing",
    "trigger_qiuling",
    "qiuling.lua",
    [
      59,
      17
    ],
    "enter",
    "ch08.qb_shouyan",
    "ch08.qb_xuezhou",
    True
  ],
  [
    "ch08_yuejing",
    "trigger_qingyinyuan",
    "qingyinyuan.lua",
    [
      9,
      22
    ],
    "interact",
    "ch08.qb_xuezhou",
    "ch08.qb_wumei",
    True
  ],
  [
    "ch08_yuejing",
    "trigger_huangmiao",
    "huangmiao.lua",
    [
      50,
      10
    ],
    "enter",
    "ch08.qb_wumei",
    "ch08.qb_huangmiao",
    True
  ],
  [
    "ch08_yuejing",
    "trigger_qingbao",
    "qingbao.lua",
    [
      47,
      31
    ],
    "interact",
    "ch08.fengwu",
    "ch08.qb_yuanbing",
    False
  ],
  [
    "ch08_yuejing",
    "trigger_zhulin_zhenyan",
    "zhulin_zhenyan.lua",
    [
      14,
      11
    ],
    "interact",
    "ch08.qb_yuanbing",
    "ch08.buzhen2",
    True
  ],
  [
    "ch08_yuejing",
    "trigger_huayuan_yj",
    "huayuan_yj.lua",
    [
      54,
      33
    ],
    "enter",
    "ch08.buzhen2",
    "ch08.huayuan",
    True
  ],
  [
    "ch08_yuejing",
    "trigger_chufa",
    "chufa.lua",
    [
      45,
      33
    ],
    "interact",
    "ch08.huayuan",
    "ch08.jingong",
    True
  ],
  [
    "ch08_yuejing",
    "trigger_lenggong",
    "lenggong.lua",
    [
      29,
      11
    ],
    "enter",
    "ch08.jingong",
    "ch08.huanggong",
    True
  ],
  [
    "ch08_yuejing",
    "trigger_zhulinxin",
    "zhulinxin.lua",
    [
      19,
      12
    ],
    "enter",
    "ch08.huanggong",
    "ch08.yuehuang",
    True
  ],
  [
    "ch08_yuejing",
    "trigger_shadan",
    "shadan.lua",
    [
      49,
      31
    ],
    "interact",
    "ch08.yuehuang",
    "ch08.junling",
    True
  ],
  [
    "ch05_nancheng",
    "trigger_xiangjia",
    "xiangjia.lua",
    [
      20,
      21
    ],
    "interact",
    "ch08.junling",
    "ch08.xiangjia",
    True
  ],
  [
    "ch05_nancheng",
    "trigger_sipingbang",
    "sipingbang.lua",
    [
      34,
      19
    ],
    "interact",
    "ch08.xiangjia",
    "ch08.sunergou",
    True
  ],
  [
    "ch05_mofu",
    "trigger_pianyuan",
    "pianyuan.lua",
    [
      36,
      8
    ],
    "enter",
    "ch08.sunergou",
    "ch08.lifu",
    True
  ],
  [
    "ch08_jiayuan_shanlin",
    "trigger_luanshi",
    "luanshi.lua",
    [
      36,
      6
    ],
    "enter",
    "ch08.lifu",
    "ch08.bigong",
    True
  ],
  [
    "ch08_jiayuan_shanlin",
    "trigger_shandong",
    "shandong.lua",
    [
      19,
      10
    ],
    "enter",
    "ch08.bigong",
    "ch08.yaolang",
    True
  ],
  [
    "ch08_jiayuan_shanlin",
    "trigger_shanding",
    "shanding.lua",
    [
      38,
      7
    ],
    "enter",
    "ch08.yaolang",
    "ch08.quhun",
    True
  ],
  [
    "ch08_jiayuan_shanlin",
    "trigger_milin_zhenyan",
    "milin_zhenyan.lua",
    [
      14,
      25
    ],
    "interact",
    "ch08.quhun",
    "ch08.buzhen3",
    True
  ],
  [
    "ch08_jiayuan_shanlin",
    "trigger_yindong",
    "yindong.lua",
    [
      16,
      14
    ],
    "interact",
    "ch08.buzhen3",
    "ch08.shalang",
    True
  ],
  [
    "ch08_dongfu",
    "trigger_huifu",
    "huifu.lua",
    [
      27,
      17
    ],
    "interact",
    "ch08.shalang",
    "ch08.huifu",
    True
  ],
  [
    "ch06_baiyaoyuan",
    "trigger_jinglong",
    "jinglong.lua",
    [
      7,
      11
    ],
    "enter",
    "ch08.huifu",
    "ch08.done",
    True
  ]
]


def room(g, x0, y0, x1, y1):
    g.solid_rect(x0, y0, x1, y1)
    g.open_rect(x0 + 1, y0 + 1, x1 - 1, y1 - 1)
    g.rect("ground", x0 + 1, y0 + 1, x1 - 1, y1 - 1, 2)


def clear(g, x0, y0, x1, y1, ground=2):
    g.open_rect(x0, y0, x1, y1)
    g.rect("ground", x0, y0, x1, y1, ground)


def layout(short, w, h):
    g = Grid(w, h)
    g.fill("ground", 1)
    g.solid_rect(0, 0, w - 1, 0)
    g.solid_rect(0, h - 1, w - 1, h - 1)
    g.solid_rect(0, 0, 0, h - 1)
    g.solid_rect(w - 1, 0, w - 1, h - 1)
    if short == "dongfu":
        g.solid_rect(2, 2, 34, 21)
        for r in ((4, 4, 14, 11), (18, 4, 31, 11), (4, 14, 14, 19), (18, 14, 30, 20)):
            clear(g, *r)
        for r in ((14, 8, 18, 8), (22, 11, 22, 14), (14, 17, 18, 17), (30, 17, 35, 17)):
            clear(g, *r)
        clear(g, 24, 0, 24, 3)
        clear(g, 24, 3, 37, 3)
        clear(g, 37, 3, 37, 24)
        clear(g, 35, 24, 47, 24)
        g.rect("ground", 5, 26, 12, 32, 3)
        g.rect("ground", 19, 18, 20, 19, 3)
    elif short == "tianxing_fangshi":
        for r in ((4, 4, 14, 10), (20, 4, 28, 10), (34, 4, 44, 10), (32, 24, 46, 33)):
            room(g, *r)
        clear(g, 10, 10, 10, 12)
        clear(g, 24, 10, 24, 35)
        clear(g, 34, 23, 36, 25)
        g.rect("ground", 4, 18, 44, 20, 2)
    elif short == "yanlingbao":
        for r in ((4, 17, 14, 24), (17, 20, 23, 26), (32, 18, 40, 25), (42, 18, 50, 25)):
            room(g, *r)
        clear(g, 20, 26, 20, 28)
        clear(g, 35, 25, 35, 28)
        g.rect("ground", 26, 3, 29, 38, 2)
        g.rect("ground", 7, 5, 9, 32, 2)
        g.rect("ground", 8, 31, 28, 33, 2)
        g.solid_rect(12, 3, 18, 9)
    elif short == "lingkuang":
        g.fill("building", 5)
        g.fill("collision", 1)
        for r in ((2, 3, 23, 36), (28, 2, 45, 14), (29, 24, 45, 37)):
            clear(g, *r, ground=1)
        clear(g, 23, 9, 28, 10)
        clear(g, 23, 25, 29, 25)
        room(g, 10, 19, 18, 24)
        clear(g, 14, 23, 14, 25)
    elif short == "jinguyuan":
        for r in ((4, 6, 14, 14), (30, 4, 39, 10), (15, 6, 24, 12)):
            room(g, *r)
        clear(g, 9, 14, 9, 17)
        clear(g, 34, 8, 34, 12)
        clear(g, 19, 12, 19, 14)
        g.rect("ground", 22, 12, 26, 34, 2)
        for x0, x1 in ((2, 21), (27, 45)):
            g.solid_rect(x0, 27, x1, 27, 8)
    elif short == "jinmacheng":
        for r in ((25, 18, 35, 24), (4, 14, 10, 20), (16, 16, 23, 23)):
            room(g, *r)
        clear(g, 29, 24, 29, 27)
        clear(g, 10, 16, 12, 16)
        clear(g, 20, 23, 20, 25)
        g.rect("ground", 23, 24, 25, 33, 2)
        g.rect("ground", 8, 8, 10, 13, 2)
    elif short == "yuejing":
        room(g, 3, 2, 37, 18)
        for x, y in ((5, 6), (5, 14), (13, 5), (14, 15), (25, 5), (34, 6), (33, 14), (27, 15)):
            g.solid_rect(x - 1, y - 1, x + 1, y + 1)
            g.rect("front", x - 1, y - 2, x + 1, y, 4)
        clear(g, 22, 18, 22, 20)
        # The palace has exactly one entrance, blocked by a flag-controlled guard.
        for r in ((40, 26, 60, 38), (16, 35, 30, 43), (3, 25, 14, 32), (45, 5, 55, 9)):
            room(g, *r)
        clear(g, 45, 37, 47, 40)
        clear(g, 21, 34, 21, 36)
        clear(g, 9, 31, 9, 34)
        clear(g, 50, 9, 50, 11)
        g.rect("ground", 22, 20, 39, 22, 2)
        g.rect("ground", 37, 20, 39, 46, 2)
        g.rect("ground", 40, 39, 62, 41, 2)
    elif short == "jiayuan_shanlin":
        room(g, 15, 3, 25, 13)
        for x, y in ((4, 5), (4, 14), (6, 19), (30, 29), (40, 29), (41, 5), (39, 24), (31, 24)):
            g.solid_rect(x - 1, y - 1, x + 1, y + 1)
            g.rect("front", x - 1, y - 2, x + 1, y, 4)
        clear(g, 20, 12, 20, 16)
        g.solid_rect(29, 12, 41, 21)
        clear(g, 35, 18, 35, 21)
        g.rect("ground", 34, 4, 39, 11, 2)
        g.rect("ground", 23, 22, 25, 33, 2)
    rnd = random.Random(800 + next(i for i, spec in enumerate(MAP_SPECS) if spec[0] == short))
    for _ in range(70):
        x, y = rnd.randrange(1, w - 1), rnd.randrange(1, h - 1)
        if g.get("ground", x, y) == 1 and g.walkable(x, y):
            g.set("overlay", x, y, 6 if rnd.random() < .7 else 7)
    return g


def node_objects(map_id, g=None):
    out = []
    for owner, name, script, at, mode, guard, done, once in NODES:
        if owner != map_id:
            continue
        kw = {"guard_flag": guard}
        if once:
            kw["set_flag"] = done
        out.append(trigger(name, *at, "ch08/" + script, mode, once, **kw))
        if g is not None:
            if mode == "enter":
                clear(g, *at, *at)
            else:
                g.solid_rect(*at, *at, 8)
    return out


def make_map(spec):
    short, w, h, region, bgm, _name = spec
    map_id = "ch08_" + short
    g = layout(short, w, h)
    objects = node_objects(map_id, g)
    def add_npc(name, x, y, role, shown=None, hidden=None):
        objects.append(npc("npc_" + name, x, y, role, "down", visible_flag=shown, hidden_flag=hidden))
    def add_fac(name, x, y, kind, **props):
        if g.get("ground", x, y) == 3:
            g.set("ground", x, y, 2)
        g.solid_rect(x, y, x, y, 8)
        objects.append(facility(name, x, y, kind, **props))
    def add_trigger(name, x, y, guard, done=None, once=False):
        if g.get("ground", x, y) == 3:
            g.set("ground", x, y, 2)
        g.solid_rect(x, y, x, y, 8)
        objects.append(trigger("trigger_" + name, x, y, "ch08/" + name + ".lua",
                               "interact", once, guard_flag=guard, set_flag=done))
    starts = {"dongfu": (45, 24, "left"), "tianxing_fangshi": (24, 33, "up"),
              "yanlingbao": (28, 37, "up"), "lingkuang": (8, 34, "up"),
              "jinguyuan": (24, 32, "up"), "jinmacheng": (24, 32, "up"),
              "yuejing": (58, 40, "left"), "jiayuan_shanlin": (24, 32, "up")}
    x, y, facing = starts[short]
    objects.insert(0, spawn("spawn_entry", x, y, facing, default=True))
    save_at = {"dongfu": (39, 23), "tianxing_fangshi": (27, 32),
               "yanlingbao": (30, 35), "lingkuang": (8, 31),
               "jinguyuan": (27, 20), "jinmacheng": (27, 31),
               "yuejing": (44, 35), "jiayuan_shanlin": (26, 31)}[short]
    add_fac("facility_cunji", *save_at, "save")
    # No countdown in this chapter: real meditation supplies a retreat after legal consumption.
    if short != "dongfu":
        add_fac("facility_yangxi", save_at[0] + 1, save_at[1], "meditate")
    if short == "dongfu":
        objects += [portal("portal_to_huangfenggu", 47, 24, "ch06_huangfenggu", "spawn_from_dongfu"),
                    portal("portal_to_tianxing_fangshi", 24, 0, "ch08_tianxing_fangshi", "spawn_entry",
                           require_flag="ch08.qiannian", deny_text_key="ch08.block.tianxing_fangshi"),
                    spawn("spawn_from_tianxing", 24, 2, "down")]
        add_npc("shuangtong_shu", 34, 17, "shuangtong_shu", "ch08.dengji", "ch08.lingquan")
        add_npc("qu_hun", 28, 19, "qu_hun", "ch08.huifu")
        add_fac("facility_yaoyuan", 8, 28, "field", ref_id="field_dongfu", slots=4, require_flag="ch08.kaifu")
        add_fac("facility_neiyuan", 6, 6, "field", ref_id="field_dongfu_nei", slots=2, require_flag="ch08.biguan")
        add_fac("facility_lingquan", 21, 17, "meditate", effectiveness=125, require_flag="ch08.kaifu")
        add_fac("facility_danfang", 7, 8, "alchemy", grade=3, require_flag="ch08.kaifu")
        add_fac("facility_fuzhuo", 8, 6, "talisman", grade=2, require_flag="ch08.biguan")
        add_trigger("beiyao", 12, 28, "ch08.gufang")
        for i, (name, at) in enumerate(zip(("zhenwei_jin", "zhenwei_mu", "zhenwei_shui", "zhenwei_huo"),
                                         ((8, 24), (30, 24), (8, 34), (30, 34)))):
            add_trigger(name, *at, "ch08.quqi")
    elif short == "tianxing_fangshi":
        objects.append(portal("portal_to_dongfu", 24, 35, "ch08_dongfu", "spawn_from_tianxing"))
        add_npc("xulao_qipu", 12, 12, "xulao_qipu")
        add_npc("wang_ziling", 13, 17, "wang_ziling", "ch08.xulao", "ch08.qiyunxiao")
        add_fac("facility_fangshi_pu", 7, 19, "shop", ref_id="ch08_tianxing_fangshi")
        add_fac("facility_yaopu", 13, 19, "shop", ref_id="ch08_tianxing_yaopu", require_flag="ch08.gufang")
    elif short == "yanlingbao":
        add_npc("yanjia_dizi", 39, 28, "yanjia_dizi")
        add_fac("facility_yanling_pu", 40, 26, "shop", ref_id="ch08_yanlingbao")
        add_fac("facility_cunji_xifeng", 11, 31, "save")
    elif short == "lingkuang":
        add_npc("xuan_le", 12, 15, "xuan_le", "ch08.lingkuang", "ch08.shouzhen")
        add_npc("lv_tianmeng", 22, 19, "lv_tianmeng", "ch08.kuilei", "ch08.shouzhen")
        add_npc("zhong_wu", 20, 19, "zhong_wu", "ch08.kuilei", "ch08.shouzhen")
        add_npc("yu_xing", 25, 25, "yu_xing", hidden="ch08.shouzhen")
        objects.append(obj("encounter_huangyuan", "encounter", 29, 3, 15, 10,
                           table_id="encounter_ch08_lingkuang", steps_min=20, steps_max=30,
                           require_flag="ch08.lingkuang"))
        add_fac("facility_cunji_dong", 35, 34, "save")
        add_fac("facility_yangxi_dong", 36, 34, "meditate")
    elif short == "jinguyuan":
        add_npc("chen_pangzi", 11, 17, "chen_pangzi", "ch08.yinian")
        add_npc("song_meng", 18, 13, "song_meng", "ch08.yinian", "ch08.nangong")
        add_npc("huangfeng_zhanshi", 20, 17, "huangfeng_zhanshi")
        add_fac("facility_jiaoyisuo", 9, 17, "shop", ref_id="ch08_jinguyuan_jiaoyisuo", require_flag="ch08.yinian")
        add_trigger("zhanbao", 24, 18, "ch08.yinian")
        add_trigger("liesha", 26, 28, "ch08.yinian")
    elif short == "jinmacheng":
        add_npc("qi_yunxiao", 12, 20, "qi_yunxiao", "ch08.jinma")
        add_npc("xin_ruyin", 12, 16, "xin_ruyin", "ch08.jiuren")
    elif short == "yuejing":
        add_npc("gongmen_shiwei", 22, 18, "gongmen_shiwei", hidden="ch08.qb_yuanbing")
        add_npc("qin_yan", 50, 34, "qin_yan", "ch08.qinzhai")
        add_npc("mo_fengwu", 53, 30, "mo_fengwu", "ch08.qinzhai", "ch08.junling")
        add_npc("chen_qiaoqian", 55, 30, "chen_qiaoqian", "ch08.qb_yuanbing", "ch08.jingong")
        add_npc("song_meng", 56, 32, "song_meng", "ch08.qb_yuanbing", "ch08.jingong")
        for i, at in enumerate(((9, 7), (20, 7), (9, 15), (20, 15)), 1):
            add_trigger("zhulin_" + str(i), *at, "ch08.qb_yuanbing")
        add_trigger("houmen", 31, 38, "ch08.xiao_qiu", "ch08.xiao_jiaoyi", True)
        add_trigger("xiqu", 9, 31, "ch08.xiao_jiaoyi", "ch08.xiao_tuijian", True)
    else:
        add_fac("facility_cunji_milin", 10, 27, "save")
        add_fac("facility_yangxi_milin", 11, 27, "meditate")
        for i, at in enumerate(((8, 22), (20, 22), (8, 29), (20, 29)), 1):
            add_trigger("milin_" + str(i), *at, "ch08.quhun")
    props = [("map_id", map_id), ("display_name_key", "ch08.map." + short + ".name"),
             ("region", region), ("bgm", bgm), ("chapter", 8),
             ("outdoor", True), ("can_leave_edge", False)]
    return build_map(map_id, w, h, "terrain_" + short, props, g, objects)


def patch(payload, map_id, extras=(), hidden=None):
    payload = copy.deepcopy(payload)
    objects = next(layer["objects"] for layer in payload["layers"] if layer["type"] == "objectgroup")
    existing = {o["name"] for o in objects}
    for record in node_objects(map_id) + list(extras):
        if record["name"] not in existing:
            objects.append(record)
            existing.add(record["name"])
    for record in objects:
        if hidden and record["name"] in hidden:
            values = {p["name"]: p for p in record.get("properties", [])}
            if "hidden_flag" in values:
                assert values["hidden_flag"]["value"] == hidden[record["name"]]
            else:
                record.setdefault("properties", []).append(prop("hidden_flag", hidden[record["name"]]))
    for i, record in enumerate(objects, 1):
        record["id"] = i
    payload["nextobjectid"] = len(objects) + 1
    return payload


def patch_huangfenggu(payload):
    return patch(payload, "ch06_huangfenggu", (
        portal("portal_to_dongfu", 1, 20, "ch08_dongfu", "spawn_entry",
               require_flag="ch08.dengji", deny_text_key="ch08.block.dongfu"),
        spawn("spawn_from_dongfu", 3, 24, "right")),
        {"npc_lin_shidi": "ch08.midian"})


def patch_baiyaoyuan(payload):
    return patch(payload, "ch06_baiyaoyuan")


def patch_nancheng(payload):
    return patch(payload, "ch05_nancheng", (
        spawn("spawn_from_tainan", 45, 17, "left"),),
        {"npc_huyuan_" + s: "ch08.junling" for s in "abcd"})


def patch_mofu(payload):
    return patch(payload, "ch05_mofu")


def patch_tainan_cun(payload):
    return patch(payload, "ch06_tainan_cun", (
        portal("portal_to_nancheng", 1, 22, "ch05_nancheng", "spawn_from_tainan"),))


PATCHES = {"ch06_huangfenggu": patch_huangfenggu, "ch06_baiyaoyuan": patch_baiyaoyuan,
           "ch05_nancheng": patch_nancheng, "ch05_mofu": patch_mofu, "ch06_tainan_cun": patch_tainan_cun}


def build_all():
    import genmaps_ch05
    import genmaps_ch06
    built = {"ch08_" + spec[0]: make_map(spec) for spec in MAP_SPECS}
    old = genmaps_ch06.build_all()
    old["ch05_mofu"] = genmaps_ch05.make_mofu()
    for map_id, apply in PATCHES.items():
        built[map_id] = apply(old[map_id])
    return built


def main(argv=None):
    argv = list(sys.argv[1:] if argv is None else argv)
    built = build_all()
    if "--check" in argv:
        return check_maps(built)
    for short, *_ in MAP_SPECS:
        write_tileset("terrain_" + short)
    for map_id, payload in built.items():
        dump_map(os.path.join(genmaps.MAPS, map_id + ".tmj"), payload)
        print("wrote", map_id)
    return 0


if __name__ == "__main__":
    sys.exit(main())
