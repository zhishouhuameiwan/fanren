"""C9 independent maps. Shared patches require explicit frozen bindings.

Default generation writes only four new maps and their tilesets.
No C8 module is imported and no old map is read or written by this entry point.
"""
from __future__ import annotations

import argparse
import copy
import json
from pathlib import Path

import genmaps
from genmaps import Grid, build_map, facility, npc, spawn, trigger

ROOT = Path(__file__).resolve().parents[2]
LANDINGS = {
    "ridge": ("ch09_huangshan", 6, 5),
    "foot": ("ch09_huangshan", 29, 26),
    "ruins": ("ch09_yuanwu", 8, 32),
    "peak": ("ch09_yuanwu", 43, 9),
    "hut": ("ch09_wumingshan", 7, 22),
    "forest": ("ch09_milin", 6, 33),
    "hollow": ("ch09_milin", 37, 12),
}

PATCH_BINDINGS = {
    "ch06_baiyaoyuan": {"trigger_zhongsheng": (9, 11)},
    "ch06_huangfenggu": {"trigger_laozu": (27, 9)},
    "ch08_dongfu": {"trigger_fengfu": (29, 17)},
    "ch08_tianxing_fangshi": {
        "trigger_xulao_jiu": (11, 11), "trigger_xingchenge": (24, 11),
        "trigger_houyuan_jiu": (11, 17), "trigger_goucai": (8, 19),
        "npc_tianxing_shouwei": (24, 34),
    },
    "ch08_lingkuang": {
        "spawn_ch09_dongku": (39, 34),
        "trigger_zhenwei_jiu_jin": (31, 27),
        "trigger_zhenwei_jiu_mu": (44, 27),
        "trigger_zhenwei_jiu_shui": (31, 36),
        "trigger_zhenwei_jiu_huo": (44, 36),
        "trigger_zhenyan_jiu": (30, 25), "trigger_xiuzhen": (39, 33),
        "trigger_wangong": (41, 33), "trigger_qidong": (40, 32),
        "npc_qu_hun_dongku": (42, 33), "trigger_suidao_hui": (28, 25),
    },
    "ch08_jinmacheng": {},
}


def base(w, h):
    g = Grid(w, h)
    g.fill("ground", 1)
    g.solid_rect(0, 0, w - 1, 0)
    g.solid_rect(0, h - 1, w - 1, h - 1)
    g.solid_rect(0, 0, 0, h - 1)
    g.solid_rect(w - 1, 0, w - 1, h - 1)
    return g


def path(g, points):
    for a, b in zip(points, points[1:]):
        x0, y0 = a
        x1, y1 = b
        if x0 != x1 and y0 != y1:
            raise ValueError("paths must follow grid axes")
        g.rect("ground", min(x0, x1), min(y0, y1),
               max(x0, x1), max(y0, y1), 2)


def trees(g, points):
    for x, y in points:
        g.solid_rect(x, y, x, y)
        g.rect("front", x - 1, y - 2, x + 1, y, 4)


def hook(name, x, y, script, guard, done, mode="interact", once=True):
    return trigger(name, x, y, "ch09/" + script + ".lua", mode,
                   once=once, guard_flag=guard, set_flag=done)


def props(short, bgm, region):
    return [("map_id", "ch09_" + short),
            ("display_name_key", "ch09.map." + short + ".name"),
            ("region", region), ("chapter", 9), ("outdoor", True),
            ("can_leave_edge", False), ("bgm", bgm)]


def huangshan():
    g = base(40, 32)
    # Broken escarpments leave a continuous route between ridge and foothill.
    for x, y, w in [(11, 7, 5), (25, 5, 8), (4, 17, 7), (19, 21, 5)]:
        g.solid_rect(x, y, x + w, y + 1)
        g.rect("front", x, y - 1, x + w, y - 1, 4)
    path(g, [(6, 5), (6, 12), (17, 12), (17, 26), (29, 26)])
    g.solid_rect(31, 26, 32, 27, 8)
    trees(g, [(4, 10), (9, 25), (34, 16), (25, 30), (35, 23)])
    objects = [
        spawn("spawn_entry", 6, 5, "down", True),
        spawn("spawn_foot", 29, 26, "right"),
        facility("facility_cunji", 5, 5, "save"),
        facility("facility_yangxi", 5, 6, "meditate"),
        facility("facility_yangxi_foot", 28, 26, "meditate"),
        hook("trigger_fuji", 6, 6, "fuji", "ch09.fengfu", "ch09.tuwei"),
        hook("trigger_tiaoxi", 30, 26, "tiaoxi", "ch09.tuwei", "ch09.tiaoxi"),
    ]
    return build_map("ch09_huangshan", 40, 32, "terrain_huangshan",
                     props("huangshan", "bgm_cliff", "yue_border"), g, objects)


def yuanwu():
    g = base(56, 40)
    # The old house is visibly broken: isolated remnants, never a complete room.
    for box in [(10, 25, 13, 25), (10, 26, 10, 27), (17, 25, 18, 27),
                (12, 29, 14, 29), (18, 29, 19, 30)]:
        g.solid_rect(*box)
    for x, y in [(11, 28), (16, 26), (20, 29), (13, 31)]:
        g.set("overlay", x, y, 7)
    g.solid_rect(33, 5, 37, 7)
    # Four small stone pavilions and a temple, all approached from open ground.
    for x, y in [(42, 12), (34, 17), (44, 20), (48, 14)]:
        g.solid_rect(x, y, x + 1, y, 8)
        g.rect("front", x, y - 1, x + 1, y - 1, 4)
    path(g, [(8, 32), (25, 32), (25, 9), (43, 9), (43, 11)])
    trees(g, [(5, 12), (9, 18), (18, 8), (21, 20), (31, 30),
              (36, 24), (46, 28), (51, 22), (48, 8), (9, 35)])
    objects = [
        spawn("spawn_entry", 8, 32, "up", True),
        spawn("spawn_peak", 43, 9, "down"),
        facility("facility_cunji", 7, 32, "save"),
        facility("facility_yangxi", 7, 33, "meditate"),
        facility("facility_cunji_peak", 42, 9, "save"),
        hook("trigger_feixu", 15, 28, "feixu", "ch09.tiaoxi", "ch09.fujia"),
        hook("trigger_shiting", 42, 13, "shiting", "ch09.xiufu", "ch09.baichi"),
        hook("trigger_fengjiao", 51, 6, "fengjiao", "ch09.baichi", "ch09.qulu"),
    ]
    for suffix, x, y in [("a", 34, 18), ("b", 44, 21), ("c", 48, 15)]:
        objects.append(npc("npc_baichi_sanxiu_" + suffix, x, y,
                           "baichi_sanxiu_" + suffix, "down",
                           visible_flag="ch09.xiufu", hidden_flag="ch09.qulu"))
    return build_map("ch09_yuanwu", 56, 40, "terrain_yuanwu",
                     props("yuanwu", "bgm_wild", "yuanwu"), g, objects)


def wumingshan():
    g = base(32, 28)
    g.solid_rect(13, 6, 18, 9)
    g.rect("front", 13, 5, 18, 5, 4)
    g.solid_rect(22, 8, 25, 10)
    g.rect("front", 22, 7, 25, 7, 4)
    path(g, [(7, 22), (7, 12), (15, 12), (15, 10)])
    trees(g, [(4, 7), (10, 7), (24, 18), (20, 24), (13, 22),
              (4, 18), (28, 24)])
    objects = [
        spawn("spawn_entry", 7, 22, "up", True),
        facility("facility_cunji", 6, 22, "save"),
        hook("trigger_zhuwu", 15, 10, "zhuwu", "ch09.fujia", "ch09.chengnuo"),
        hook("trigger_zhuwu_er", 16, 10, "zhuwu_er", "ch09.feidao", "ch09.xiufu"),
        npc("npc_xin_ruyin_jiu", 18, 10, "xin_ruyin", "down",
            visible_flag="ch09.fujia"),
    ]
    return build_map("ch09_wumingshan", 32, 28, "terrain_wumingshan",
                     props("wumingshan", "bgm_mountain_path", "yuanwu"), g, objects)


def milin():
    g = base(48, 40)
    for x, y in [(12, 28), (12, 18), (25, 24), (31, 8), (40, 20),
                 (7, 9), (23, 5), (36, 31)]:
        g.solid_rect(x, y, x + 2, y + 1)
        g.rect("front", x - 1, y - 1, x + 3, y, 4)
    # A tree hollow is represented by a dark trunk with an open approach.
    trees(g, [(37, 10)])
    path(g, [(6, 33), (18, 33), (18, 15), (37, 15), (37, 12)])
    trees(g, [(4, 20), (10, 23), (15, 12), (22, 31), (26, 17),
              (29, 35), (33, 23), (40, 29), (43, 8), (7, 36),
              (20, 8), (30, 17), (44, 35)])
    objects = [
        spawn("spawn_entry", 6, 33, "right", True),
        spawn("spawn_hollow", 37, 12, "up"),
        facility("facility_cunji", 5, 33, "save"),
        facility("facility_yangxi", 5, 32, "meditate"),
        hook("trigger_niyin", 10, 33, "niyin", "ch09.xiuzhen", "ch09.niyin",
             mode="enter"),
        hook("trigger_jiuren", 19, 15, "jiuren", "ch09.niyin", "ch09.jiuren"),
        hook("trigger_shudong", 37, 11, "shudong", "ch09.jiuren", "ch09.dieluo"),
        hook("trigger_huicheng", 38, 12, "huicheng", "ch09.dieluo", "ch09.huicheng"),
    ]
    # The entry trigger spans the path; the rest of the forest remains connected.
    objects[4]["width"] = 3 * genmaps.TILE
    return build_map("ch09_milin", 48, 40, "terrain_milin",
                     props("milin", "bgm_night", "yue_border"), g, objects)


def build_all():
    return {m["properties"][0]["value"]: m for m in
            (huangshan(), yuanwu(), wumingshan(), milin())}


def _patch(payload, bindings, additions, hidden=()):
    """Pure patch design. Coordinates must be supplied from frozen old maps."""
    result = copy.deepcopy(payload)
    objects = next(layer["objects"] for layer in result["layers"]
                   if layer["type"] == "objectgroup")
    by_name = {o["name"]: o for o in objects}
    for name, factory in additions:
        x, y = bindings[name]
        item = factory(x, y)
        if name in by_name:
            item["id"] = by_name[name]["id"]
            objects[objects.index(by_name[name])] = item
        else:
            objects.append(item)
        by_name[name] = item
    for name, flag in hidden:
        item = by_name[name]
        p = next((p for p in item["properties"] if p["name"] == "hidden_flag"), None)
        if p is not None and p["value"] != flag:
            raise ValueError("existing hidden flag conflicts: " + name)
        if p is None:
            item["properties"].append(genmaps.prop("hidden_flag", flag))
    for i, item in enumerate(objects, 1):
        item["id"] = i
    result["nextobjectid"] = len(objects) + 1
    return result


def patch_baiyaoyuan(payload, bindings):
    return _patch(payload, bindings, [
        ("trigger_zhongsheng", lambda x, y: hook("trigger_zhongsheng", x, y,
         "zhongsheng", "ch08.done", "ch09.zhongsheng"))])


def patch_huangfenggu(payload, bindings):
    return _patch(payload, bindings, [
        ("trigger_laozu", lambda x, y: hook("trigger_laozu", x, y,
         "laozu", "ch09.zhongsheng", "ch09.laozu"))])


def patch_dongfu(payload, bindings):
    return _patch(payload, bindings, [
        ("trigger_fengfu", lambda x, y: hook("trigger_fengfu", x, y,
         "fengfu", "ch09.laozu", "ch09.fengfu"))],
        [("npc_qu_hun", "ch09.fengfu")])


def patch_tianxing_fangshi(payload, bindings):
    additions = []
    for name, script, guard, done in [
        ("trigger_xulao_jiu", "xulao_jiu", "chengnuo", "weituo"),
        ("trigger_xingchenge", "xingchenge", "weituo", "xingchen"),
        ("trigger_houyuan_jiu", "houyuan_jiu", "xingchen", "feidao"),
        ("trigger_goucai", "goucai", "qulu", "goucai")]:
        additions.append((name, lambda x, y, n=name, s=script, g=guard, d=done:
                          hook(n, x, y, s, "ch09." + g, "ch09." + d)))
    additions.append(("npc_tianxing_shouwei", lambda x, y:
                      npc("npc_tianxing_shouwei", x, y, "tianxing_shouwei", "up",
                          script="ch09/shouwei.lua", visible_flag="ch09.fengfu")))
    return _patch(payload, bindings, additions)


def patch_lingkuang(payload, bindings):
    additions = [("spawn_ch09_dongku", lambda x, y:
                  spawn("spawn_ch09_dongku", x, y, "up"))]
    for suffix in ("jin", "mu", "shui", "huo"):
        name = "trigger_zhenwei_jiu_" + suffix
        additions.append((name, lambda x, y, n=name, s=suffix:
                          hook(n, x, y, "zhenwei_" + s, "ch09.goucai", None,
                               once=False)))
    for name, script, guard, done in [
        ("trigger_zhenyan_jiu", "zhenyan_jiu", "goucai", "buzhen"),
        ("trigger_xiuzhen", "xiuzhen", "buzhen", "xiuzhen"),
        ("trigger_wangong", "wangong", "huicheng", "wangong"),
        ("trigger_qidong", "qidong", "wangong", "done")]:
        additions.append((name, lambda x, y, n=name, s=script, g=guard, d=done:
                          hook(n, x, y, s, "ch09." + g, "ch09." + d)))
    additions.extend([
        ("npc_qu_hun_dongku", lambda x, y:
         npc("npc_qu_hun_dongku", x, y, "qu_hun", "up",
             visible_flag="ch09.goucai", hidden_flag="ch09.done")),
        ("trigger_suidao_hui", lambda x, y:
         hook("trigger_suidao_hui", x, y, "suidao_hui", "ch09.goucai", None,
              mode="enter", once=False)),
    ])
    return _patch(payload, bindings, additions)


def patch_jinmacheng(payload, bindings):
    return _patch(payload, bindings, [], [
        ("npc_qi_yunxiao", "ch08.yueding"), ("npc_xin_ruyin", "ch08.yueding")])


PATCHES = {
    "ch06_baiyaoyuan": patch_baiyaoyuan,
    "ch06_huangfenggu": patch_huangfenggu,
    "ch08_dongfu": patch_dongfu,
    "ch08_tianxing_fangshi": patch_tianxing_fangshi,
    "ch08_lingkuang": patch_lingkuang,
    "ch08_jinmacheng": patch_jinmacheng,
}


def apply_shared(map_id, payload):
    return PATCHES[map_id](payload, PATCH_BINDINGS[map_id])


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--shared", action="store_true")
    args = parser.parse_args()
    built = build_all()
    if args.shared:
        for map_id in PATCHES:
            payload = json.loads((ROOT / "maps" / (map_id + ".tmj")).read_text(encoding="utf-8"))
            built[map_id] = apply_shared(map_id, payload)
    if args.check:
        return genmaps.check_maps(built)
    for map_id, payload in built.items():
        if map_id.startswith("ch09_"):
            genmaps.write_tileset("terrain_" + map_id[5:])
        genmaps.dump_map(str(ROOT / "maps" / (map_id + ".tmj")), payload)
    print("CH09_NEW_MAPS_WRITTEN 4; shared patches=" + str(args.shared))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
