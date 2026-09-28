"""meta.json：光源、水面格、粒子与发射点、时辰预设。

光源从结构里推，按时辰取舍（白天点灯笼是浪费运行时的光照预算，也显得假）：

  来源                     day   dusk   night  indoor
  丹炉 / 药炉 / 地火        ✓     ✓      ✓      ✓      火光，橙红，闪
  存档点                    ✓     ✓      ✓      ✓      一点青光
  屋门口的灯笼（镇、府、宗门）  ·     半数    ✓      ·
  窗（夜里窗纸透光）          ·     三成    七成    ·
  石灯                      ·     ✓      ✓      ✓
  室内：桌案上的烛台、墙上的壁灯                    ✓
  洞里：崖面上的火把                               ✓

坐标一律以格为单位（格中心 = x+0.5）。颜色取自主色板（palette.c / mix）。
"""

from __future__ import annotations

from palette import c, mix

import pix

# dof（移轴）：0.3 上下几乎看不出糊，室外取 0.6 左右才读得出「桌面模型」；夜里暗，糊多了
# 远处成一片黑，少一点；室内的图窄，镜头常贴图边、主角常在画面上下沿，取 0.4。
# maps.json 每张图另写了 dof 与 dof_why，这里只是新图的缺省。
TIME_DEFAULTS = {
    "day": {"ambient": [236, 232, 222], "dof": 0.6, "bloom": 0.2, "vignette": 0.25},
    "dusk": {"ambient": [218, 164, 132], "dof": 0.6, "bloom": 0.45, "vignette": 0.4},
    "night": {"ambient": [84, 96, 142], "dof": 0.55, "bloom": 0.6, "vignette": 0.5},
    "indoor": {"ambient": [132, 116, 98], "dof": 0.4, "bloom": 0.5, "vignette": 0.45},
}

LIGHT_KINDS = {
    #            半径  颜色                               强度  闪烁
    "furnace": (4.5, mix("red3", "gold3", 0.35), 1.0, 0.35),
    "fire": (4.0, mix("red3", "gold3", 0.45), 0.9, 0.4),
    "torch": (4.0, mix("red4", "gold3", 0.5), 0.85, 0.4),
    "lantern": (3.0, mix("red4", "gold3", 0.55), 0.8, 0.15),
    "window": (2.0, mix("gold3", "paper", 0.3), 0.55, 0.05),
    "stone_lantern": (2.5, mix("gold3", "paper", 0.2), 0.6, 0.1),
    "candle": (2.5, c("gold4"), 0.7, 0.25),
    "sconce": (3.0, mix("gold3", "red4", 0.3), 0.7, 0.2),
    "save": (1.8, c("jade3"), 0.5, 0.0),
}


def light(kind: str, x: float, y: float, scale: float = 1.0) -> dict:
    r, col, inten, fl = LIGHT_KINDS[kind]
    return {"x": round(float(x), 2), "y": round(float(y), 2), "r": round(r * scale, 2),
            "color": [int(v) for v in col], "intensity": round(inten, 2), "flicker": round(fl, 2),
            "kind": kind}


def meta(ctx, preset: dict) -> dict:
    sc = ctx.sc
    md = sc.md
    time = preset["time"]
    d = TIME_DEFAULTS[time]
    salt = pix.salt_of(md.map_id, "lights")
    lights: list[dict] = []
    emitters: list[dict] = []

    for cand in ctx.lights:
        if "emitter" in cand:
            emitters.append({"kind": cand["emitter"], "x": cand["x"], "y": cand["y"]})
            continue
        k = cand["kind"]
        h = pix.hash01(int(cand["x"] * 10), int(cand["y"] * 10), salt)
        if k == "furnace":
            lights.append(light("furnace", cand["x"], cand["y"]))
            emitters.append({"kind": "ember", "x": cand["x"], "y": round(cand["y"] - 0.5, 2)})
        elif k == "stone_lantern" and time != "day":
            lights.append(light("stone_lantern", cand["x"], cand["y"]))
        elif k == "window" and ((time == "night" and h < 0.7) or (time == "dusk" and h < 0.3)):
            lights.append(light("window", cand["x"], cand["y"]))
        elif k == "door" and sc.theme in ("town", "manor", "sect", "village", "valley", "dock") and \
                ((time == "night") or (time == "dusk" and h < 0.5)):
            lights.append(light("lantern", cand["x"] - 0.45, cand["y"] - 1.1))
            lights.append(light("lantern", cand["x"] + 0.45, cand["y"] - 1.1))

    for o in md.objects:
        if o.type != "facility":
            continue
        cx, cy = o.x + o.w / 2.0, o.y + o.h / 2.0
        kind = o.props.get("kind")
        if kind in ("alchemy", "forge"):
            if not any(abs(l["x"] - cx) < 1.5 and abs(l["y"] - cy) < 1.5 and l["kind"] == "furnace" for l in lights):
                lights.append(light("furnace", cx, cy))
                emitters.append({"kind": "ember", "x": round(cx, 2), "y": round(cy - 0.5, 2)})
        elif kind == "save":
            lights.append(light("save", cx, cy))

    if time == "indoor":
        indoor_lights(ctx, lights, emitters)
    if sc.theme == "wild" and time == "night":
        guard_torches(ctx, lights, emitters)

    water = [[int(x), int(y)] for y, x in zip(*sc.water.nonzero())]
    # 水面起雾：每隔几格一个发射点（运行时按 kind=mist 往上飘）
    if water and time in ("dusk", "night") or (water and sc.theme in ("valley", "dock")):
        for i, (x, y) in enumerate(water):
            if pix.hash01(x, y, salt + 3) < 0.06:
                emitters.append({"kind": "mist", "x": x + 0.5, "y": y + 0.5})
            if time in ("dusk", "night") and pix.hash01(x, y, salt + 5) < 0.04:
                emitters.append({"kind": "firefly", "x": x + 0.5, "y": y - 0.5})

    out = {
        "map": md.map_id,
        "tile": 16,
        "width": md.w,
        "height": md.h,
        "theme": sc.theme,
        "time": time,
        "season": sc.season,
        "ambient": preset.get("ambient", d["ambient"]),
        "dof": preset.get("dof", d["dof"]),
        "bloom": preset.get("bloom", d["bloom"]),
        "vignette": preset.get("vignette", d["vignette"]),
        "particles": preset.get("particles", []),
        "lights": dedupe(lights),
        "water": water,
        "emitters": emitters,
        "backdrop": preset["backdrop"],
    }
    return out


def dedupe(lights: list[dict]) -> list[dict]:
    """同一处叠了两盏（门洞既是门又挨着设施）只留一盏。"""
    out = []
    for l in lights:
        if any(o["kind"] == l["kind"] and abs(o["x"] - l["x"]) < 0.6 and abs(o["y"] - l["y"]) < 0.6 for o in out):
            continue
        out.append(l)
    return out


def indoor_lights(ctx, lights: list, emitters: list) -> None:
    """屋里：桌案上一盏烛台；墙面每隔五六格一盏壁灯；洞里改成火把。"""
    sc = ctx.sc
    cave = sc.theme == "cave"
    for p in sc.props:
        if p.kind in ("table", "desk", "stone_table", "counter", "dais", "herb_table", "coffin", "stone_bed"):
            lights.append(light("candle", p.x + p.w / 2.0, p.y + 0.4))
    step = 7 if cave else 5
    for y in range(sc.H - 1):
        for x in range(sc.W):
            if not (sc.wall[y, x] and sc.walk[y + 1, x]):
                continue
            if (x + y * 3) % step != 0:
                continue
            if cave:
                lights.append(light("torch", x + 0.5, y + 0.6))
                emitters.append({"kind": "ember", "x": x + 0.5, "y": y + 0.4})
            else:
                lights.append(light("sconce", x + 0.5, y + 0.6))


def guard_torches(ctx, lights: list, emitters: list) -> None:
    """夜里的山庄：外墙上每隔几格一支巡庄的火把（节点 12c 巡庄）。"""
    sc = ctx.sc
    for st in sc.structs:
        if not st.compound:
            continue
        for y in range(sc.H - 1):
            for x in range(sc.W):
                if st.mask[y, x] and sc.walk[y + 1, x] and x % 6 == 2:
                    lights.append(light("torch", x + 0.5, y + 0.5))
                    emitters.append({"kind": "ember", "x": x + 0.5, "y": y + 0.3})
    for p in sc.props:
        if p.kind == "pavilion":
            lights.append(light("lantern", p.x + p.w / 2.0, p.y + 0.2, 1.2))
