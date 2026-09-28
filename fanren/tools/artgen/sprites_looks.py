"""外观表 data/visual/looks.json 的读取、校验、继承与上色。

外观表是人工维护的输入（施工图 1.2）。这里做三件事：
  1. 校验：字段名、取值全部白名单，拼错当场报错——外观表是给人改的，
     改错一个字母若静默退成默认值，要到预览图里才发现「怎么穿成了白的」。
  2. 继承：条目可写 "base": "<原型>"，只覆写不同的字段。原型也写在同一张表里。
  3. 上色：把「衣型 + 主色/副色/腰带色」翻译成身体模板各区域的色阶。

衣料颜色一律由主色板 palette.py 的颜色经 mix() 调出来（CLOTH 表），
不另立 RGB 常量——人物的青与地图的青是同一种青，放进同一帧里才不打架。
"""

from __future__ import annotations

import json
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

import palette as P
from sprites_core import RGB, FIXED_RAMPS, Materials, cloth_ramp

# ---------------------------------------------------------------------------
# 衣料色（全部由主色板调和）
# ---------------------------------------------------------------------------

CLOTH: dict[str, RGB] = {
    # 青、蓝一系
    "qinghui": P.mix("jade1", "stone2", 0.55),     # 青灰：韩立
    "qingbu": P.mix("water1", "ink3", 0.45),       # 青布：墨大夫、贾天龙「一件寻常青布袍子」
    "zangqing": P.mix("water0", "ink2", 0.40),     # 藏青
    "tianqing": P.mix("water3", "stone4", 0.45),   # 天青
    "lanshan": P.mix("water2", "stone3", 0.30),    # 蓝衫：蓝衣人
    "shuilv": P.mix("jade3", "stone3", 0.45),      # 水绿
    "cang": P.mix("pine", "ink3", 0.40),           # 苍（暗青绿）
    "huilv": P.mix("moss", "stone2", 0.50),        # 灰绿
    "songlv": P.mix("leaf2", "stone1", 0.35),      # 松绿
    # 灰、黑、白
    "huibu": P.mix("stone2", "ink4", 0.30),        # 灰布：考官「穿灰袍的中年人」
    "qianhui": P.mix("stone3", "ink5", 0.25),      # 浅灰
    "hei": P.c("ink2"),                            # 黑衣
    "xuan": P.mix("ink2", "violet0", 0.30),        # 玄（带一点紫的黑）
    "bai": P.c("plaster2"),                        # 白
    "xingbai": P.mix("plaster2", "soil5", 0.30),   # 杏白（旧白布）
    # 土、褐、麻
    "ma": P.mix("stone4", "soil4", 0.40),          # 本色麻布
    "he": P.mix("soil2", "wood2", 0.50),           # 褐（短褐）
    "tuhuang": P.mix("soil3", "stone3", 0.40),     # 土黄
    "cha": P.mix("wood2", "stone2", 0.50),         # 茶
    "zhe": P.mix("soil3", "red2", 0.30),           # 赭
    # 红、紫
    "zhehong": P.mix("red2", "soil2", 0.35),       # 赭红：野狼帮
    "zhu": P.mix("red2", "red3", 0.25),            # 朱红：金光上人「一件红袍滚着金线」
    "jiang": P.mix("red1", "violet1", 0.25),       # 绛
    "zi": P.mix("violet2", "stone2", 0.35),        # 紫
    "ouhe": P.mix("violet3", "plaster1", 0.45),    # 藕荷
    "fen": P.mix("blossom2", "plaster2", 0.25),    # 桃粉（主色板 A1 追加的桃花粉；朱砂兑白调出来的粉发灰）
    "yanzhi": P.mix("blossom1", "red2", 0.35),     # 胭脂
    # 黄、金
    "huang": P.mix("gold2", "stone4", 0.45),       # 黄衫：墨凤舞「黄衫姑娘」
    "xiang": P.mix("gold1", "soil3", 0.45),        # 香色（暗金黄）
    "jin": P.c("gold2"),                           # 金线
    # 皮、甲
    "pi": P.mix("wood2", "soil2", 0.40),           # 皮甲
    "heipi": P.mix("wood1", "ink2", 0.45),         # 黑皮
}

SKINS: dict[str, list[RGB]] = {
    "fair": [P.c("skin0"), P.c("skin1"), P.c("skin2"), P.c("skin3")],
    "pale": [P.mix("skin0", "skin1", 0.6), P.mix("skin1", "skin2", 0.6), P.mix("skin2", "skin3", 0.55), P.mix("skin3", "paper", 0.4)],
    "tan": [P.mix("skin0", "ink2", 0.2), P.c("skin0"), P.mix("skin1", "skin2", 0.45), P.mix("skin2", "skin3", 0.2)],
    "dark": [P.mix("skin0", "ink1", 0.45), P.mix("skin0", "ink2", 0.15), P.mix("skin0", "skin1", 0.6), P.mix("skin1", "skin2", 0.35)],
    "ruddy": [P.mix("skin0", "red1", 0.2), P.mix("skin1", "red2", 0.15), P.mix("skin2", "red3", 0.15), P.c("skin3")],
    # 尸傀、傀儡：青灰的死人皮色
    "corpse": cloth_ramp(P.mix("stone3", "jade2", 0.30)),
}

HAIR_COLORS: dict[str, list[RGB]] = {
    # 黑发不用最黑的墨：底色落在 ink3，受光面 ink5——黑发才有「一绺一绺」的体积，
    # 否则发与描边同色，整颗头糊成一块剪影
    "black": [P.c("ink1"), P.c("ink2"), P.c("ink3"), P.mix("ink4", "ink5", 0.35)],
    "darkbrown": [P.c("ink0"), P.mix("wood0", "ink1", 0.4), P.mix("wood1", "ink2", 0.5), P.mix("wood2", "ink4", 0.4)],
    "brown": [P.c("wood0"), P.c("wood1"), P.c("wood2"), P.c("wood3")],
    "grey": [P.c("ink3"), P.c("ink4"), P.c("ink5"), P.c("ink6")],
    "white": [P.c("ink5"), P.c("ink6"), P.c("ink7"), P.c("ink8")],
}

# 配件的固定材质
PROP_RAMPS: dict[str, list[RGB]] = {
    "steel_sheath": [P.c("ink0"), P.c("ink1"), P.c("ink2"), P.c("ink4")],
    "hilt": [P.c("wood0"), P.c("wood1"), P.c("wood2"), P.c("wood3")],
    "dao_sheath": [P.c("wood0"), P.c("wood1"), P.c("wood2"), P.c("wood3")],
    "straw_dark": [P.c("wood1"), P.c("wood2"), P.c("wood3"), P.c("wood4")],
    "steel_dark": [P.c("ink2"), P.c("ink3"), P.c("ink5"), P.c("ink6")],
    "gourd": [P.c("soil1"), P.c("soil3"), P.c("soil4"), P.c("soil5")],
    "cord": [P.c("red0"), P.c("red1"), P.c("red2"), P.c("red3")],
    "paper_fan": [P.c("stone3"), P.c("stone4"), P.c("plaster2"), P.c("paper")],
    "horsetail": [P.c("stone3"), P.c("plaster1"), P.c("plaster2"), P.c("plaster3")],
    "abacus_bead": [P.c("red0"), P.c("red1"), P.c("red2"), P.c("red3")],
    "leather_c": [P.c("wood0"), P.c("wood1"), P.c("wood2"), P.c("wood3")],
    "bundle": cloth_ramp(CLOTH["qingbu"]),
    "gold_c": [P.c("gold0"), P.c("gold1"), P.c("gold2"), P.c("gold3")],
    "lantern": [P.c("red1"), P.c("red2"), P.c("red3"), P.c("red4")],
}

# ---------------------------------------------------------------------------
# 取值白名单
# ---------------------------------------------------------------------------

GENDERS = ("m", "f")
AGES = ("child", "youth", "adult", "elder")
BUILD_WIDTHS = {"slim": -1, "normal": 0, "stout": 2, "fat": 3, "giant": 0, "dwarf": 0}
HEIGHT_RANGE = (-3, 3)   # height 能加减的行数：再多，模板里没有那么多行可删可补
FACES = ("round", "narrow", "old", "fierce", "blank")
OUTFITS: dict[str, dict[str, Any]] = {
    # family：剪影族；regions：身体模板各区域用哪种颜色（main/sub/armor/skin）
    # wide：宽袖；open：敞怀；trim_default：领口缺省色
    "changshan": {"family": "long", "regions": {"torso": "main", "sleeve": "main", "lower": "main", "pants": "sub"}, "trim_default": "xingbai"},
    "daopao": {"family": "long", "regions": {"torso": "main", "sleeve": "main", "lower": "main", "pants": "sub"}, "trim_default": "sub", "wide": True},
    "kuanpao": {"family": "long", "regions": {"torso": "main", "sleeve": "main", "lower": "main", "pants": "sub"}, "trim_default": "sub", "wide": True},
    "ruqun": {"family": "long", "regions": {"torso": "main", "sleeve": "main", "lower": "sub", "pants": "sub"}, "trim_default": "xingbai"},
    "shanqun": {"family": "long", "regions": {"torso": "main", "sleeve": "main", "lower": "sub", "pants": "sub"}, "trim_default": "xingbai", "wide": True},
    "duanda": {"family": "short", "regions": {"torso": "main", "sleeve": "main", "lower": "main", "pants": "sub"}, "trim_default": "xingbai"},
    "duanhe": {"family": "short", "regions": {"torso": "main", "sleeve": "main", "lower": "main", "pants": "sub"}, "trim_default": "main"},
    "jinzhuang": {"family": "short", "regions": {"torso": "main", "sleeve": "main", "lower": "main", "pants": "sub"}, "trim_default": "sub"},
    "pijia": {"family": "short", "regions": {"torso": "armor", "sleeve": "main", "lower": "armor", "pants": "sub"}, "trim_default": "armor"},
    "open": {"family": "short", "regions": {"torso": "main", "sleeve": "main", "lower": "main", "pants": "sub"}, "trim_default": "main", "open": True},
    "beixin": {"family": "short", "regions": {"torso": "sub", "sleeve": "main", "lower": "sub", "pants": "main"}, "trim_default": "sub"},
    # 斗篷：main 是斗篷，sub 是底下的袍子。斗篷盖住腰带、往下再披 cloak_rows 行，最后一行是下摆的影子
    "doupeng": {"family": "long", "regions": {"torso": "main", "sleeve": "main", "lower": "sub", "pants": "sub"},
                "trim_default": "sub", "wide": True, "cloak_rows": 3},
    # 连帽斗篷的巨汉（曲魂）：模板按 24×32 手画好了（sprites_quhun），不走体型手术——
    # 手术撑出来的斗篷是一整块灰，终审说它「像一块墓碑」
    "doupeng_giant": {"family": "cloak_giant", "regions": {"torso": "main", "sleeve": "main", "lower": "sub", "pants": "sub"},
                      "trim_default": "sub", "prebuilt": "giant"},
}
SKIN_KEYS = tuple(SKINS)
HAIR_COLOR_KEYS = tuple(HAIR_COLORS)

LOOK_FIELDS = {
    "base", "name", "note", "gender", "age", "build", "height", "skin", "face", "brows", "blush", "mole",
    "hair", "hair_color", "band", "headwear", "headwear_colors", "outfit", "colors", "props", "weapon",
    "beard", "frame",
}
COLOR_SLOTS = ("main", "sub", "belt", "trim", "shoe", "wrap", "armor")
WEAPON_KINDS = ("none", "fist", "sword", "dao", "knife", "staff", "fan", "podao")


@dataclass(frozen=True)
class Look:
    id: str
    name: str
    gender: str
    age: str
    build: str
    height: int | None
    skin: str
    face: str
    brows: bool
    blush: bool
    mole: bool
    hair: str
    hair_color: str
    band: str | None
    headwear: str
    headwear_colors: tuple[str, str]
    outfit: str
    colors: dict[str, str]
    props: tuple[str, ...]
    weapon: str
    beard: str
    note: str = ""
    raw: dict[str, Any] = field(default_factory=dict)

    @property
    def family(self) -> str:
        return OUTFITS[self.outfit]["family"]


def color_of(name: str) -> RGB:
    """衣料名先查 CLOTH，再查主色板；都没有就报错（报外观表里写错的那个名字，而不是一个裸的 KeyError）。"""
    if name in CLOTH:
        return CLOTH[name]
    if name in P.PALETTE:
        return P.c(name)
    raise LookError(f"颜色名 {name!r} 既不在衣料色表 CLOTH 里，也不在主色板 palette.py 里")


class LookError(ValueError):
    pass


def _resolve_chain(lid: str, table: dict[str, dict[str, Any]], seen: tuple[str, ...] = ()) -> dict[str, Any]:
    if lid in seen:
        raise LookError(f"外观继承成环：{' → '.join(seen + (lid,))}")
    if lid not in table:
        raise LookError(f"外观 {seen[-1] if seen else '?'} 继承了不存在的 {lid!r}")
    entry = table[lid]
    unknown = set(entry) - LOOK_FIELDS
    if unknown:
        raise LookError(f"外观 {lid!r} 有未知字段 {sorted(unknown)}")
    base = entry.get("base")
    merged: dict[str, Any] = {}
    if base:
        merged = _resolve_chain(base, table, seen + (lid,))
    for k, v in entry.items():
        if k == "colors" and isinstance(v, dict):
            merged["colors"] = {**merged.get("colors", {}), **v}
        elif k != "base":
            merged[k] = v
    return merged


def _default_face(gender: str, age: str) -> str:
    if age == "elder":
        return "old"
    if age in ("child", "youth") or gender == "f":
        return "round"
    return "narrow"


def make_look(lid: str, table: dict[str, dict[str, Any]]) -> Look:
    d = _resolve_chain(lid, table)

    def need(key: str, allowed: tuple[str, ...] | None = None) -> Any:
        if key not in d:
            raise LookError(f"外观 {lid!r} 缺字段 {key!r}")
        v = d[key]
        if allowed is not None and v not in allowed:
            raise LookError(f"外观 {lid!r} 的 {key}={v!r} 不在 {allowed} 里")
        return v

    from sprites_head import HAIRS, HEADWEAR, BEARDS
    from sprites_props import CARRY

    gender = need("gender", GENDERS)
    age = need("age", AGES)
    build = need("build", tuple(BUILD_WIDTHS))
    skin = need("skin", SKIN_KEYS)
    hair = need("hair", tuple(HAIRS))
    hair_color = need("hair_color", HAIR_COLOR_KEYS)
    headwear = d.get("headwear", "none")
    if headwear != "none" and headwear not in HEADWEAR:
        raise LookError(f"外观 {lid!r} 的 headwear={headwear!r} 未定义")
    outfit = need("outfit", tuple(OUTFITS))
    prebuilt = OUTFITS[outfit].get("prebuilt")
    if prebuilt and build != prebuilt:
        raise LookError(f"外观 {lid!r}：衣型 {outfit} 是按 {prebuilt} 体型画好的模板，build 只能写 {prebuilt}")
    beard = d.get("beard", "none")
    if beard not in ("none", "stubble") and beard not in BEARDS:
        raise LookError(f"外观 {lid!r} 的 beard={beard!r} 未定义")
    face = d.get("face") or _default_face(gender, age)
    if face not in FACES:
        raise LookError(f"外观 {lid!r} 的 face={face!r} 不在 {FACES} 里")
    colors = dict(d.get("colors", {}))
    for slot, cname in colors.items():
        if slot not in COLOR_SLOTS:
            raise LookError(f"外观 {lid!r} 的 colors 有未知槽位 {slot!r}")
        color_of(cname)  # 名字不存在当场抛错
    if "main" not in colors:
        raise LookError(f"外观 {lid!r} 缺主色 colors.main")
    props = tuple(d.get("props", ()))
    for p in props:
        if p not in CARRY:
            raise LookError(f"外观 {lid!r} 的配件 {p!r} 未定义")
    weapon = d.get("weapon")
    if weapon is None:
        weapon = "fist"
        for p in props:
            w = CARRY[p].weapon
            if w:
                weapon = w
                break
    if weapon not in WEAPON_KINDS:
        raise LookError(f"外观 {lid!r} 的 weapon={weapon!r} 不在 {WEAPON_KINDS} 里")
    hwc = d.get("headwear_colors", ["ma", "he"])
    if not (isinstance(hwc, list) and len(hwc) == 2):
        raise LookError(f"外观 {lid!r} 的 headwear_colors 须是两个色名")
    for cname in hwc:
        color_of(cname)
    band = d.get("band")
    if band is not None:
        color_of(band)
    height = d.get("height")
    if height is not None:
        if not isinstance(height, int) or isinstance(height, bool):
            raise LookError(f"外观 {lid!r} 的 height 须是整数（行数增减）")
        if not HEIGHT_RANGE[0] <= height <= HEIGHT_RANGE[1]:
            raise LookError(f"外观 {lid!r} 的 height={height} 超出 {HEIGHT_RANGE}")
        if build == "giant":
            raise LookError(f"外观 {lid!r}：giant 的身高是固定的（24×32 帧），不能再写 height")
    for flag in ("brows", "blush", "mole"):
        if flag in d and not isinstance(d[flag], bool):
            raise LookError(f"外观 {lid!r} 的 {flag} 须是 true/false")
    return Look(
        id=lid, name=d.get("name", lid), gender=gender, age=age, build=build, height=height,
        skin=skin, face=face, brows=bool(d.get("brows", False)), blush=bool(d.get("blush", False)),
        mole=bool(d.get("mole", False)), hair=hair, hair_color=hair_color, band=band,
        headwear=headwear, headwear_colors=(hwc[0], hwc[1]), outfit=outfit, colors=colors,
        props=props, weapon=weapon, beard=beard, note=d.get("note", ""), raw=d,
    )


# ---------------------------------------------------------------------------
# 体型：年龄定高度、build 定宽度
# ---------------------------------------------------------------------------

def body_surgery(look: Look) -> tuple[int, int, int, int, int]:
    """返回 (frame_w, frame_h, widen, torso_rows, leg_rows)。"""
    if look.build == "giant":
        return (24, 32, 4, 3, 4)
    widen = BUILD_WIDTHS[look.build]
    torso, legs = 0, 0
    if look.age == "child":
        torso, legs = -2, -2
        widen -= 1
    elif look.age == "youth":
        torso, legs = -1, -1
        widen = min(widen, 0) if look.gender == "f" else widen
    if look.gender == "f" and look.age in ("adult", "elder"):
        legs -= 1
    if look.gender == "f" and look.build == "normal":
        widen -= 1
    if look.build == "dwarf":
        torso, legs = -2, -3
    if look.height is not None:
        # height：在年龄缺省的基础上再增减几行（正数更高），先加减腿、再加减上身
        h = look.height
        extra_legs = (h + (1 if h > 0 else 0)) // 2 if h > 0 else -((-h + 1) // 2)
        legs += extra_legs
        torso += h - extra_legs
    return (16, 24, widen, torso, legs)


# ---------------------------------------------------------------------------
# 上色
# ---------------------------------------------------------------------------

def _cloth(look: Look, slot: str, fallback: str) -> list[RGB]:
    return cloth_ramp(color_of(look.colors.get(slot, fallback)))


def materials_for(look: Look) -> Materials:
    spec = OUTFITS[look.outfit]
    main = look.colors["main"]
    sub = look.colors.get("sub", main)
    armor = look.colors.get("armor", "pi")
    named = {"main": main, "sub": sub, "armor": armor}
    ramps: dict[str, list[RGB]] = dict(FIXED_RAMPS)
    ramps.update(PROP_RAMPS)
    for region, which in spec["regions"].items():
        ramps[region] = cloth_ramp(color_of(named[which]))
    trim_default = spec["trim_default"]
    trim_name = look.colors.get("trim", named.get(trim_default, trim_default))
    ramps["trim"] = cloth_ramp(color_of(trim_name))
    ramps["belt"] = _cloth(look, "belt", "hei")
    ramps["shoe"] = _cloth(look, "shoe", "hei")
    ramps["wrap"] = _cloth(look, "wrap", "xingbai")
    ramps["skin"] = SKINS[look.skin]
    ramps["hair"] = HAIR_COLORS[look.hair_color]
    ramps["beard"] = HAIR_COLORS[look.hair_color]
    ramps["band"] = cloth_ramp(color_of(look.band)) if look.band else HAIR_COLORS[look.hair_color]
    ramps["hat"] = cloth_ramp(color_of(look.headwear_colors[0]))
    ramps["hat2"] = cloth_ramp(color_of(look.headwear_colors[1]))
    ramps["stubble"] = [P.mix(SKINS[look.skin][i], HAIR_COLORS[look.hair_color][1], 0.45) for i in range(4)]
    return Materials.build(ramps)


# ---------------------------------------------------------------------------
# 读表
# ---------------------------------------------------------------------------

@dataclass
class LookTable:
    looks: dict[str, Look]
    archetypes: dict[str, Look]
    roles: dict[str, dict[str, Any]]
    raw: dict[str, Any]


ROLE_FIELDS = {"look", "variants", "note"}


def load_table(path: Path) -> LookTable:
    raw = json.loads(path.read_text(encoding="utf-8"))
    for key in ("archetypes", "looks", "roles"):
        if key not in raw or not isinstance(raw[key], dict):
            raise LookError(f"{path} 缺少对象 {key!r}")
    arche_raw: dict[str, dict[str, Any]] = raw["archetypes"]
    look_raw: dict[str, dict[str, Any]] = raw["looks"]
    clash = set(arche_raw) & set(look_raw)
    if clash:
        raise LookError(f"原型与外观重名：{sorted(clash)}")
    table = {**arche_raw, **look_raw}
    archetypes = {k: make_look(k, table) for k in sorted(arche_raw)}
    looks = {k: make_look(k, table) for k in sorted(look_raw)}
    roles: dict[str, dict[str, Any]] = {}
    for rid, entry in raw["roles"].items():
        if isinstance(entry, str):
            entry = {"look": entry}
        unknown = set(entry) - ROLE_FIELDS
        if unknown:
            raise LookError(f"roles.{rid} 有未知字段 {sorted(unknown)}")
        lk = entry.get("look")
        if lk not in looks and lk not in archetypes:
            raise LookError(f"roles.{rid} 指向不存在的外观 {lk!r}")
        for v in entry.get("variants", []):
            if set(v) - {"max_chapter", "min_chapter", "look"}:
                raise LookError(f"roles.{rid}.variants 有未知字段")
            if not ({"max_chapter", "min_chapter"} & set(v)):
                raise LookError(f"roles.{rid}.variants 每条都要写 max_chapter 或 min_chapter")
            for key in ("max_chapter", "min_chapter"):
                if key in v and (not isinstance(v[key], int) or isinstance(v[key], bool)):
                    raise LookError(f"roles.{rid}.variants.{key} 须是整数（章号）")
            if v.get("look") not in looks:
                raise LookError(f"roles.{rid}.variants 指向不存在的外观 {v.get('look')!r}")
        roles[rid] = entry
    return LookTable(looks, archetypes, roles, raw)
