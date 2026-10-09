# -*- coding: utf-8 -*-
"""Independent Chapter 10 maps. build_all is pure and imports no C8/9 generator."""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import genmaps
from genmaps import Grid, build_map, check_maps, dump_map, facility, npc, portal, spawn, trigger, write_tileset


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


def floor(g, x0, y0, x1, y1, gid=1):
    g.open_rect(x0, y0, x1, y1)
    g.rect("ground", x0, y0, x1, y1, gid)


def room(g, x0, y0, x1, y1, door):
    g.solid_rect(x0, y0, x1, y1)
    floor(g, x0 + 1, y0 + 1, x1 - 1, y1 - 1)
    floor(g, *door, *door, 2)
    g.rect("front", x0, y0 - 1, x1, y0 - 1, 4)


def event(g, objects, short, xy, guard, done, mode="interact", once=True, width=1):
    x, y = xy
    if mode == "interact":
        g.solid_rect(x, y, x, y, 8)
    objects.append(trigger("trigger_" + short, x, y, "ch10/" + short + ".lua",
                           mode, once=once, w=width, guard_flag=guard, set_flag=done))


def person(objects, role, xy, visible, hidden=None, name=None, script=None):
    objects.append(npc(name or "npc_" + role, *xy, role, "down", script=script,
                       visible_flag=visible, hidden_flag=hidden))


def save(objects, xy, name="facility_cunji"):
    objects.append(facility(name, *xy, "save"))


def finish(short, bgm, g, objects):
    mid = "ch10_" + short
    props = [("map_id", mid), ("display_name_key", "ch10.map." + short + ".name"),
             ("region", "luanxinghai_xinan"), ("bgm", bgm), ("chapter", 10),
             ("outdoor", True), ("can_leave_edge", False)]
    return build_map(mid, g.w, g.h, "terrain_ch10_" + short, props, g, objects)


def make_gudao():
    g = base(24, 20, True)
    floor(g, 3, 2, 20, 8)
    g.solid_rect(3, 10, 20, 18)
    g.rect("ground", 3, 10, 20, 18, 2)
    floor(g, 7, 13, 17, 17)
    floor(g, 8, 8, 9, 14, 2)
    floor(g, 8, 8, 15, 9, 2)
    g.rect("overlay", 11, 14, 13, 15, 7)
    for x, y in ((5, 4), (18, 6)):
        g.solid_rect(x, y, x, y)
        g.rect("front", x - 1, y - 2, x + 1, y, 4)
    objects = [spawn("start", 12, 16, "up", True)]
    save(objects, (15, 16))
    person(objects, "qu_hun", (14, 15), "ch09.done", "ch10.likai")
    event(g, objects, "huizhen", (12, 15), "ch09.done", "ch10.huizhen")
    event(g, objects, "jushi", (8, 8), "ch10.huizhen", "ch10.jushi", "enter")
    event(g, objects, "wanghai", (12, 3), "ch10.jushi", "ch10.likai")
    return finish("gudao", "bgm_cliff", g, objects)


def make_haichuan():
    g = base(40, 24, True)
    floor(g, 6, 3, 33, 21)
    floor(g, 16, 1, 24, 4)
    floor(g, 34, 10, 37, 11, 2)
    room(g, 7, 12, 16, 20, (16, 17))
    room(g, 19, 12, 32, 20, (24, 12))
    g.solid_rect(24, 15, 27, 15, 8)
    objects = [spawn("chuantou", 20, 4, "down", True),
               spawn("dating", 21, 17, "up")]
    save(objects, (21, 6))
    for role, xy in (("gu_dongzhu", (24, 7)), ("wang_changqing", (17, 16)),
                     ("chuangong_jia", (10, 6)), ("chuangong_yi", (29, 8))):
        person(objects, role, xy, "ch10.guyu", "ch10.kaoan",
               script="ch10/xuehua.lua" if role == "wang_changqing" else None)
    person(objects, "qu_hun", (15, 10), "ch10.likai", "ch10.kaoan")
    for role, xy in zip(("luo_zheng", "feng_sanniang", "mao_daoyou", "xue_daoyou",
                         "qing_suanzi", "yan_daoyou", "dou_daoyou"),
                        ((28, 18), (23, 18), (28, 14), (30, 14), (26, 18), (30, 18), (21, 14))):
        person(objects, role, xy, "ch10.jueding", "ch10.chuhai")
    event(g, objects, "chuantou", (20, 5), "ch10.likai", "ch10.guyu")
    event(g, objects, "yetan", (10, 17), "ch10.guyu", "ch10.yetan")
    event(g, objects, "kaoan", (33, 10), "ch10.yetan", "ch10.kaoan")
    event(g, objects, "chuanting", (25, 16), "ch10.jueding", "ch10.chuhai")
    return finish("haichuan", "bgm_river", g, objects)


def make_kuixing():
    g = base(64, 48)
    g.rect("ground", 48, 1, 62, 47, 3)
    g.rect("ground", 48, 47, 62, 47, 2)
    g.rect("collision", 48, 1, 62, 47, 1)
    floor(g, 34, 3, 60, 35)
    floor(g, 48, 36, 57, 45, 2)
    floor(g, 12, 22, 51, 24, 2)
    floor(g, 39, 22, 41, 39, 2)
    room(g, 49, 35, 57, 39, (53, 39))
    room(g, 42, 25, 47, 30, (43, 30))
    g.solid_rect(49, 24, 55, 30, 8)
    floor(g, 50, 25, 54, 29)
    floor(g, 49, 28, 49, 28, 2)
    floor(g, 47, 28, 47, 28, 2)
    floor(g, 52, 30, 52, 30, 2)
    floor(g, 46, 31, 55, 32, 3)
    for xy in ((39, 27), (40, 28), (38, 29)):
        g.solid_rect(*xy, *xy)
        g.rect("front", xy[0] - 1, xy[1] - 2, xy[0] + 1, xy[1], 4)
    room(g, 43, 4, 58, 16, (48, 16))
    floor(g, 46, 9, 55, 11, 2)
    g.solid_rect(44, 4, 57, 4, 8)
    room(g, 7, 13, 19, 20, (14, 20))
    floor(g, 13, 21, 15, 23, 2)
    floor(g, 30, 1, 32, 23, 2)
    objects = [spawn("gangkou", 52, 40, "up", True), spawn("gujia", 47, 28, "left"),
               spawn("muwu", 43, 26, "down"), spawn("dongshi", 48, 17, "up"),
               spawn("dengxian", 14, 20, "up"), spawn("xilu", 31, 3, "down")]
    save(objects, (50, 42))
    save(objects, (41, 18), "facility_dongshi_cunji")
    objects.append(facility("facility_muwu_putuan", 44, 28, "meditate"))
    person(objects, "gangkou_xiushi", (55, 37), "ch10.kaoan")
    person(objects, "gangkou_xiaofan", (49, 41), "ch10.kaoan", "ch10.xuandi")
    person(objects, "qu_hun", (51, 39), "ch10.kaoan", "ch10.dengji", "npc_qu_hun_gang")
    person(objects, "qu_hun", (45, 26), "ch10.dengji", "ch10.xuandi", "npc_qu_hun_muwu")
    person(objects, "gu_dongzhu", (50, 28), "ch10.dengji", "ch10.chudao")
    person(objects, "gu_dongzhu_lao", (50, 29), "ch10.chudao", "ch10.chuhai")
    person(objects, "gu_kai", (51, 29), "ch10.chudao", "ch10.chuhai")
    person(objects, "wang_changqing", (48, 27), "ch10.dengji", "ch10.xuandi")
    person(objects, "gujia_zhuangding", (48, 31), "ch10.dengji", "ch10.xuandi")
    person(objects, "wen_qiang", (46, 13), "ch10.muwu", "ch10.leitai")
    person(objects, "dongshi_luren", (41, 17), "ch10.muwu", "ch10.leitai")
    person(objects, "dengxiange_zhishi", (16, 16), "ch10.dengxian")
    for name, xy, guard, done in (
        ("dengji", (53, 37), "kaoan", "dengji"),
        ("muwu", (48, 28), "dengji", "muwu"),
        ("gongdian", (47, 16), "muwu", "chouqian"),
        ("leitai", (50, 8), "chouqian", "leitai"),
        ("dengxian", (43, 25), "leitai", "dengxian"),
        ("xuandi", (14, 17), "dengxian", "xuandi"),
        ("gujia", (49, 31), "chudao", "gujia")):
        event(g, objects, name, xy, "ch10." + guard, "ch10." + done)
    g.rect("ground", 49, 31, 49, 31, 2)
    objects.append(portal("portal_to_tiandujie", 31, 0, "ch10_tiandujie", "xilu",
                          require_flag="ch10.chudao", deny_text_key="ch10.block.tiandujie"))
    floor(g, 31, 0, 31, 0, 2)
    return finish("kuixing", "bgm_town", g, objects)


def make_xiaohuan():
    g = base(56, 44, True)
    floor(g, 3, 3, 46, 38)
    floor(g, 47, 34, 51, 40, 2)
    g.solid_rect(5, 5, 23, 22)
    g.solid_rect(18, 20, 24, 30)
    floor(g, 8, 8, 20, 19)
    floor(g, 19, 20, 21, 28, 2)
    room(g, 22, 25, 31, 33, (27, 33))
    floor(g, 20, 27, 24, 29, 2)
    floor(g, 9, 15, 12, 17, 1)
    g.solid_rect(14, 8, 14, 12)
    floor(g, 14, 10, 14, 10, 2)
    room(g, 38, 24, 45, 29, (41, 29))
    floor(g, 43, 29, 43, 29, 2)
    floor(g, 43, 30, 49, 36, 2)
    objects = [spawn("matou", 49, 36, "left", True), spawn("zhenzhong", 43, 29, "up"),
               spawn("end", 50, 39, "down")]
    save(objects, (47, 35))
    person(objects, "hei_gui", (48, 34), "ch10.xuandi", "ch10.chuguan", "npc_hei_gui_matou")
    person(objects, "hei_gui", (40, 27), "ch10.chuguan", "ch10.chudao", "npc_hei_gui_zhen")
    person(objects, "xiaohuan_zhenzhang", (42, 27), "ch10.xuandi", "ch10.huashen")
    person(objects, "xiaohuan_zhenmin", (40, 25), "ch10.chuguan", "ch10.chudao")
    person(objects, "qu_hun", (18, 11), "ch10.xuandi", "ch10.huashen")
    person(objects, "qu_hun_huashen", (19, 12), "ch10.huashen", "ch10.chuguan")
    objects.extend([
        facility("facility_yaoyuan_xh", 10, 16, "field", ref_id="field_xiaohuan",
                 slots=4, require_flag="ch10.dingce"),
        facility("facility_jingshi", 10, 10, "meditate", require_flag="ch10.dingce"),
        facility("facility_danfang_xh", 18, 17, "alchemy", grade=3, require_flag="ch10.zhuji")])
    for name, xy, guard, done in (
        ("matou", (48, 36), "xuandi", "zhenzhang"),
        ("kaifu", (26, 33), "zhenzhang", "kaifu"),
        ("zhenyan", (25, 24), "kaifu", "buzhen"),
        ("lifu", (12, 16), "buzhen", "dingce"),
        ("zhuji", (11, 11), "dingce", "zhuji"),
        ("dayan", (18, 12), "zhuji", "huashen"),
        ("sanzhuan", (12, 11), "huashen", "chuguan"),
        ("chudao", (50, 35), "chuguan", "chudao"),
        ("shijin", (18, 9), "shadan", "shijin")):
        event(g, objects, name, xy, "ch10." + guard, "ch10." + done)
    for direction, xy in (("dong", (33, 15)), ("nan", (26, 35)),
                           ("xi", (4, 23)), ("bei", (26, 5))):
        event(g, objects, "zhenwei_" + direction, xy, "ch10.kaifu", None, once=False)
    event(g, objects, "zhifadui", (47, 38), "ch10.shijin", "ch10.done", "enter", width=5)
    return finish("xiaohuan", "bgm_valley", g, objects)


def make_tiandujie():
    g = base(48, 36)
    floor(g, 22, 24, 26, 35, 2)
    floor(g, 0, 30, 24, 32, 2)
    floor(g, 2, 30, 2, 35, 2)
    floor(g, 8, 18, 39, 21, 2)
    floor(g, 22, 3, 25, 24, 2)
    for rect, door in (((7, 6, 16, 12), (12, 12)), ((19, 5, 28, 11), (24, 11)),
                       ((31, 6, 40, 12), (35, 12)), ((7, 13, 16, 17), (12, 17)),
                       ((19, 12, 28, 17), (24, 17)), ((31, 13, 40, 17), (35, 17)),
                       ((6, 23, 15, 29), (12, 23)), ((31, 23, 41, 30), (35, 23))):
        room(g, *rect, door)
    objects = [spawn("chengwai", 24, 32, "up", True), spawn("xilu", 2, 33, "up")]
    save(objects, (25, 30))
    person(objects, "tiandujie_sanxiu", (20, 28), "ch10.chudao", "ch10.jueding")
    objects.append(facility("facility_tiandujie_pu", 10, 25, "shop", ref_id="ch10_tiandujie_pu"))
    for name, xy, guard, done in (("yunmeng", (24, 24), "chudao", "shuangjiao"),
        ("baishuilou", (24, 16), "shuangjiao", "liulian"),
        ("danyaopu", (13, 25), "liulian", "danfang"),
        ("kezhan", (36, 27), "danfang", "jueding")):
        event(g, objects, name, xy, "ch10." + guard, "ch10." + done)
    objects.append(portal("portal_to_kuixing", 2, 35, "ch10_kuixing", "xilu",
                          require_flag="ch10.chudao", deny_text_key="ch10.block.kuixing"))
    return finish("tiandujie", "bgm_town", g, objects)


def make_jinhai():
    g = base(48, 36, True)
    floor(g, 9, 5, 39, 29)
    floor(g, 40, 24, 44, 26, 2)
    floor(g, 19, 30, 22, 33, 2)
    g.rect("ground", 16, 12, 27, 21, 2)
    for x, y in ((12, 7), (34, 9), (29, 25)):
        g.solid_rect(x, y, x + 1, y + 1)
    objects = [spawn("anbian", 38, 28, "up", True), spawn("zhendi", 20, 16, "right")]
    save(objects, (36, 27))
    for role, xy in zip(("feng_sanniang", "mao_daoyou", "xue_daoyou",
                         "qing_suanzi", "yan_daoyou", "dou_daoyou"),
                        ((34, 23), (36, 23), (38, 23), (32, 24), (34, 25), (36, 25))):
        person(objects, role, xy, "ch10.chuhai", "ch10.taoli")
    for name, xy, guard, done in (("huangdao", (38, 27), "chuhai", "yingli"),
        ("jiaoshi", (22, 16), "yingli", "xinshen"),
        ("bishui_yan", (23, 16), "xinshen", "bishui_cheng"),
        ("zhenmen", (21, 32), "bishui_cheng", "taoli"),
        ("ruzhen", (25, 20), "taoli", "zhangu")):
        event(g, objects, name, xy, "ch10." + guard, "ch10." + done)
    for direction, xy in (("dong", (28, 16)), ("nan", (22, 22)),
                           ("xi", (15, 16)), ("bei", (22, 10))):
        event(g, objects, "bishui_" + direction, xy, "ch10.xinshen", None, once=False)
    return finish("jinhai", "bgm_wild", g, objects)


def make_haiyuandao():
    g = base(32, 24)
    g.solid_rect(5, 3, 22, 14)
    floor(g, 8, 6, 19, 11)
    floor(g, 12, 12, 14, 18, 2)
    g.solid_rect(13, 6, 13, 10)
    floor(g, 13, 9, 13, 9, 2)
    room(g, 24, 16, 29, 20, (26, 20))
    g.rect("ground", 9, 7, 10, 9, 3)
    objects = [spawn("huangshan", 8, 18, "right", True)]
    save(objects, (9, 18))
    person(objects, "haiyuan_sanxiu", (25, 21), "ch10.zhangu", "ch10.shadan")
    event(g, objects, "linshi", (13, 12), "ch10.zhangu", "ch10.shadan")
    return finish("haiyuandao", "bgm_cliff", g, objects)


BUILDERS = (make_gudao, make_haichuan, make_kuixing, make_xiaohuan,
            make_tiandujie, make_jinhai, make_haiyuandao)


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
