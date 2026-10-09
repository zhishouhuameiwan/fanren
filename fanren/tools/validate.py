#!/usr/bin/env python3
"""内容校验：数据、地图、脚本、文案的一致性检查。

CI 的门禁之一，也可本地自检：

    python tools/validate.py            # 全部检查
    python tools/validate.py --maps     # 只查地图
    python tools/validate.py --data     # 只查数据与文案
    python tools/validate_selftest.py   # 负向自检：每条地图检查都得真拦得住

地图这一侧还有一条门禁不在本文件里：maps/ch01_*.tmj 由 tools/mapgen/genmaps.py
生成，两边走散过一次（闸门只存在于 tmj、生成器那边没有，重跑即静默抹掉），
所以 `python tools/mapgen/genmaps.py --check` 与本文件同为地图的门禁。

校验规则的权威来源是 docs/map_spec.md 第 7 节；本文件的每条检查都标注了对应条目。
退出码 0 表示全部通过，1 表示存在错误（警告不影响退出码）。
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

REQUIRED_LAYERS = ["ground", "overlay", "building", "front", "collision", "objects"]
TILE_LAYERS = REQUIRED_LAYERS[:5]

REQUIRED_MAP_PROPS = [
    "map_id", "display_name_key", "region", "bgm", "chapter", "outdoor", "can_leave_edge",
]

# 对象类型 → 必填属性。来源：docs/map_spec.md 第 4 节。
OBJECT_REQUIRED_PROPS = {
    "spawn": ["id", "default", "facing"],
    "portal": ["target_map", "target_spawn"],
    "npc": ["role_id", "facing"],
    "trigger": ["script", "mode", "once"],
    "encounter": ["table_id", "steps_min", "steps_max"],
    "facility": ["kind"],
}

FLAG_PROP_SUFFIX = "_flag"
TEXT_KEY_SUFFIX = "_key"

# 数据文件里的引用字段 → 被引用的东西。
#
# 这张表是为了不再一类一类打补丁：本校验器先后漏过地图引脚本、脚本引文案、
# 数据引文案、数据引物品四类，每次都是「只查了当初写它时想到的那几种」。
# 新增一类引用，加一行；忘了加，至少缺口是显式的。
#
# 值为 'item' / 'role' / 'battle' / 'magic' 时按对应 data 子目录的 id 集合校验；
# 'text' 与 'flag' 走各自的登记表（见 TEXT_KEY_SUFFIX / FLAG_PROP_SUFFIX）。
REFERENCE_FIELDS = {
    'itemId': 'item',
    'item_id': 'item',
    'productId': 'item',
    'seedId': 'item',
    'role_id': 'role',
    'roleId': 'role',
    'battleId': 'battle',
    'battle_id': 'battle',
    'magics': 'magic',
    # 道具施法：一件符箓施展的那门法术（docs/interfaces-p3-ch07.md 5.1 第 2 条）。
    'castMagic': 'magic',
    # 归一化别名：驼峰与下划线两种写法都能命中，见 normalise_field 的说明。
    'item_id': 'item',
    'product_id': 'item',
    'seed_id': 'item',
    'cast_magic': 'magic',
}

# 每类引用对应的 data 子目录。id 从这些目录下的文件里收集。
REFERENCE_SOURCES = {
    'item': 'items',
    'role': 'roles',
    'battle': 'battles',
    'magic': 'magics',
}

IDENTIFIER_RE = re.compile(r"^[a-z0-9_]+$")


def normalise_field(name):
    """把字段名归一成小写下划线形式，供引用检测比对。

    data/ 里两种命名风格并存且都有来由：角色文件必须跟 C++ 加载器的驼峰，
    战斗文件当初按下划线规定。此前检测直接对原字段名做 endswith('_key')，
    于是 descKey / nameKey 这类驼峰字段一条都没被查过——三十四条物品描述与
    三个商店名指向不存在的文案，门禁却是绿的。

    归一化比强推统一风格稳妥：改风格会打断加载器。
    """
    out = []
    for index, char in enumerate(str(name)):
        if char.isupper() and index > 0:
            out.append('_')
        out.append(char.lower())
    return ''.join(out)

@dataclass
class Report:
    errors: list[str] = field(default_factory=list)
    warnings: list[str] = field(default_factory=list)

    def error(self, where: str, message: str) -> None:
        self.errors.append(f"{where}: {message}")

    def warn(self, where: str, message: str) -> None:
        self.warnings.append(f"{where}: {message}")

    def merge(self, other: "Report") -> None:
        self.errors.extend(other.errors)
        self.warnings.extend(other.warnings)

    @property
    def ok(self) -> bool:
        return not self.errors


def read_json(path: Path, report: Report) -> dict | None:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError:
        report.error(str(path), "文件不存在")
    except json.JSONDecodeError as exc:
        report.error(str(path), f"JSON 语法错误：{exc}")
    except UnicodeDecodeError as exc:
        report.error(str(path), f"不是合法的 UTF-8：{exc}")
    return None


# ---------------------------------------------------------------------------
# 数据与文案
# ---------------------------------------------------------------------------

def collect_data(report: Report) -> tuple[dict[str, Path], dict[str, str]]:
    """返回 (id → 定义文件, 文案 key → 文本)。重复 id 直接判错。"""
    data_dir = ROOT / "data"
    ids: dict[str, Path] = {}
    texts: dict[str, str] = {}
    if not data_dir.exists():
        report.warn("data/", "目录不存在，跳过数据检查")
        return ids, texts

    for path in sorted(data_dir.rglob("*.json")):
        payload = read_json(path, report)
        if payload is None:
            continue
        rel = path.relative_to(ROOT)

        # flags.json 是旗标登记表，不是数据条目，由 load_registered_flags 单独读。
        # 放进 one-file-per-entry 的规则里会被误判为「缺少 id 字段」。
        if path.name == "flags.json" and path.parent.name == "data":
            continue
        # data/visual/ 是 tools/artgen 的输入（外观表、地图视觉预设，一张表里多条），
        # 不是 one-file-per-entry 的数据条目；运行时只读 assets/art/** 的生成物（施工图 1.2）。
        if path.relative_to(data_dir).parts[0] == "visual":
            continue

        # data/text/*.json 是 key → 文案的大对象，其余是 one-file-per-entry。
        if path.parent.name == "text":
            if not isinstance(payload, dict):
                report.error(str(rel), "文案文件必须是 key → 文本的对象")
                continue
            for key, value in payload.items():
                if not isinstance(value, str):
                    report.error(str(rel), f"文案 {key} 不是字符串")
                elif key in texts:
                    report.error(str(rel), f"文案 key 重复：{key}")
                else:
                    texts[key] = value
            continue

        if not isinstance(payload, dict):
            report.error(str(rel), "数据文件必须是对象")
            continue
        entry_id = payload.get("id")
        if not entry_id:
            report.error(str(rel), "缺少 id 字段")
            continue
        if not IDENTIFIER_RE.match(str(entry_id)):
            report.error(str(rel), f"id 只能用小写字母、数字与下划线：{entry_id}")
        if entry_id in ids:
            report.error(str(rel), f"id 与 {ids[entry_id].relative_to(ROOT)} 重复：{entry_id}")
        else:
            ids[entry_id] = path
        if not payload.get("name"):
            report.error(str(rel), f"{entry_id} 缺少 name 字段")
    return ids, texts


def load_registered_flags(report: Report) -> set[str]:
    """旗标必须先在 data/flags.json 登记再使用，避免魔数式的旗标失控。"""
    path = ROOT / "data" / "flags.json"
    if not path.exists():
        report.warn("data/flags.json", "旗标登记表不存在，跳过旗标检查")
        return set()
    payload = read_json(path, report)
    if payload is None:
        return set()
    if isinstance(payload, dict):
        return set(payload.keys())
    if isinstance(payload, list):
        return {str(item) for item in payload}
    report.error("data/flags.json", "格式应为对象或数组")
    return set()


# ---------------------------------------------------------------------------
# 地图
# ---------------------------------------------------------------------------

def map_properties(payload: dict) -> dict[str, str]:
    result: dict[str, str] = {}
    for prop in payload.get("properties", []) or []:
        if isinstance(prop, dict) and "name" in prop:
            result[str(prop["name"])] = str(prop.get("value", ""))
    return result


def object_properties(obj: dict) -> dict[str, str]:
    result: dict[str, str] = {}
    for prop in obj.get("properties", []) or []:
        if isinstance(prop, dict) and "name" in prop:
            result[str(prop["name"])] = str(prop.get("value", ""))
    return result


# 设施属性 effectiveness（docs/map_spec.md 4.6；契约 docs/interfaces-p3-ch08.md 5.1 第 1 条）：
# 在这一处打坐的修为效率，百分比，缺省 100。引擎只在 kind=meditate 的设施上读它
# （Application::openFacility），写在别处等于没写。三条各报各的：写错一处只报那一处。
# 类型要看 Tiled 的原始属性：object_properties 把值一律转成字符串，"125" 与 125 在那里分不出来。
EFFECTIVENESS_PROP = "effectiveness"
EFFECTIVENESS_RANGE = (100, 300)


def check_facility_effectiveness(where: str, obj_type: str, oprops: dict[str, str], obj: dict,
                                 report: Report) -> None:
    raw = next((p for p in obj.get("properties", []) or []
                if isinstance(p, dict) and p.get("name") == EFFECTIVENESS_PROP), None)
    if raw is None:
        return
    if obj_type != "facility" or oprops.get("kind") != "meditate":
        report.error(where, f"{EFFECTIVENESS_PROP} 只许写在 kind=meditate 的设施上"
                            f"（这里是 {obj_type or '无 type'}、kind={oprops.get('kind', '')}）：引擎只在打坐处读它")
    value = raw.get("value")
    if raw.get("type") != "int" or isinstance(value, bool) or not isinstance(value, int):
        report.error(where, f"{EFFECTIVENESS_PROP} 必须是整数（Tiled 的 int 属性），这里写的是 "
                            f"{raw.get('type')} {value!r}")
        return
    low, high = EFFECTIVENESS_RANGE
    if not low <= value <= high:
        report.error(where, f"{EFFECTIVENESS_PROP}={value} 超出 {low}–{high}（百分比，缺省 100 = 普通蒲团）")


@dataclass
class ParsedMap:
    path: Path
    map_id: str
    width: int
    height: int
    spawns: dict[str, dict]
    portals: list[dict]
    objects: list[dict]


def check_map(path: Path, report: Report) -> ParsedMap | None:
    payload = read_json(path, report)
    if payload is None:
        return None
    rel = str(path.relative_to(ROOT))

    layers = payload.get("layers", []) or []
    by_name = {str(layer.get("name", "")): layer for layer in layers}

    # 规则 1：六个图层齐备、名称正确。
    for name in REQUIRED_LAYERS:
        if name not in by_name:
            report.error(rel, f"缺少图层 {name}")
    # 规则 1（续）：顺序正确。
    present = [str(layer.get("name", "")) for layer in layers if str(layer.get("name", "")) in REQUIRED_LAYERS]
    if present != [n for n in REQUIRED_LAYERS if n in by_name]:
        report.error(rel, f"图层顺序不符合规范，应为 {REQUIRED_LAYERS}，实际 {present}")

    # 规则 2：图块层为 CSV（Tiled 的 JSON 里体现为整数数组 + encoding 不是 base64）。
    for name in TILE_LAYERS:
        layer = by_name.get(name)
        if layer is None:
            continue
        if layer.get("encoding") == "base64" or isinstance(layer.get("data"), str):
            report.error(rel, f"图层 {name} 使用了 base64 编码，规范要求 CSV")
        elif not isinstance(layer.get("data"), list):
            report.error(rel, f"图层 {name} 缺少 data 数组")

    width = int(payload.get("width", 0) or 0)
    height = int(payload.get("height", 0) or 0)
    if width <= 0 or height <= 0:
        report.error(rel, f"地图尺寸非法：{width}x{height}")
    for name in TILE_LAYERS:
        layer = by_name.get(name)
        if layer is None or not isinstance(layer.get("data"), list):
            continue
        expected = width * height
        if len(layer["data"]) != expected:
            report.error(rel, f"图层 {name} 数据长度 {len(layer['data'])} 与尺寸 {width}x{height} 不符")

    # 规则 3：地图属性齐备，map_id 与文件名一致。
    props = map_properties(payload)
    for prop in REQUIRED_MAP_PROPS:
        if prop not in props:
            report.error(rel, f"缺少地图属性 {prop}")
    map_id = props.get("map_id", "")
    if map_id and map_id != path.stem:
        report.error(rel, f"map_id({map_id}) 与文件名({path.stem}) 不一致")

    # 规则 14：chapter 属性与文件名前缀一致。
    chapter_prop = props.get("chapter", "")
    prefix = re.match(r"^ch(\d+)_", path.stem)
    if prefix and chapter_prop:
        if str(int(prefix.group(1))) != str(chapter_prop).strip():
            report.error(rel, f"chapter({chapter_prop}) 与文件名章号({prefix.group(1)}) 不一致")

    # 对象层
    obj_layer = by_name.get("objects", {})
    objects = obj_layer.get("objects", []) or []
    tile = 32
    spawns: dict[str, dict] = {}
    portals: list[dict] = []
    names_seen: set[str] = set()
    parsed_objects: list[dict] = []

    for obj in objects:
        obj_name = str(obj.get("name", ""))
        obj_type = str(obj.get("type", "") or obj.get("class", ""))
        oprops = object_properties(obj)
        where = f"{rel} 对象 {obj_name or '(未命名)'}"

        # 规则 11：名称唯一、矩形对齐网格。
        if not obj_name:
            report.error(where, "对象必须有名称")
        elif obj_name in names_seen:
            report.error(where, "对象名称在图内重复")
        else:
            names_seen.add(obj_name)

        for axis in ("x", "y", "width", "height"):
            value = obj.get(axis, 0) or 0
            if float(value) % tile != 0:
                report.warn(where, f"{axis}={value} 未对齐 {tile}px 网格")

        if not obj_type:
            report.error(where, "对象缺少 type（Tiled 的 Class 字段）")
        elif obj_type not in OBJECT_REQUIRED_PROPS:
            report.error(where, f"未知对象类型 {obj_type}")
        else:
            for prop in OBJECT_REQUIRED_PROPS[obj_type]:
                if prop not in oprops:
                    report.error(where, f"{obj_type} 缺少必填属性 {prop}")

        check_facility_effectiveness(where, obj_type, oprops, obj, report)

        record = {
            "name": obj_name,
            "type": obj_type,
            "props": oprops,
            "x": int(float(obj.get("x", 0) or 0)) // tile,
            "y": int(float(obj.get("y", 0) or 0)) // tile,
            # 宽高也得记下来：引擎的 core::TileMap::objectAt 是矩形命中，一个 2x2
            # 的设施实际挡住四格。只记左上角那一格，校验器眼里的地形就和玩家脚下的
            # 不是同一张图。
            "w": max(1, int(float(obj.get("width", 0) or 0)) // tile),
            "h": max(1, int(float(obj.get("height", 0) or 0)) // tile),
        }
        parsed_objects.append(record)

        if obj_type == "spawn":
            spawns[oprops.get("id", obj_name)] = record
        elif obj_type == "portal":
            portals.append(record)

    # 规则 4：恰有一个 default spawn。
    defaults = [s for s in spawns.values() if str(s["props"].get("default", "")).lower() == "true"]
    if len(defaults) == 0:
        report.error(rel, "没有 default=true 的 spawn")
    elif len(defaults) > 1:
        report.error(rel, f"有 {len(defaults)} 个 default spawn，应恰有一个")

    # 规则 10：遭遇区步数区间合法。
    for record in parsed_objects:
        if record["type"] != "encounter":
            continue
        try:
            lo = int(record["props"].get("steps_min", "0"))
            hi = int(record["props"].get("steps_max", "0"))
            if lo > hi:
                report.error(f"{rel} 对象 {record['name']}", f"steps_min({lo}) 大于 steps_max({hi})")
        except ValueError:
            report.error(f"{rel} 对象 {record['name']}", "steps_min/steps_max 不是整数")

    return ParsedMap(path, map_id or path.stem, width, height, spawns, portals, parsed_objects)


def check_cross_map(maps: list[ParsedMap], report: Report) -> None:
    """规则 5：传送点双向可达 —— 目标地图存在，且目标 spawn 在那张图里存在。"""
    by_id = {m.map_id: m for m in maps}
    for parsed in maps:
        rel = str(parsed.path.relative_to(ROOT))
        for portal in parsed.portals:
            target_map = portal["props"].get("target_map", "")
            target_spawn = portal["props"].get("target_spawn", "")
            where = f"{rel} 传送点 {portal['name']}"
            target = by_id.get(target_map)
            if target is None:
                report.error(where, f"目标地图不存在：{target_map}")
                continue
            if target_spawn not in target.spawns:
                report.error(where, f"目标地图 {target_map} 中没有 spawn：{target_spawn}")


def expected_deny_key(target_map: str) -> str | None:
    """deny_text_key 的规范名：一律按**目标图**命名。

    为什么不能按源图：一张图可以有好几个带闸门的出口——ch02_yaopu 就有三个
    （居所 / 外刃堂 / 崖壁），按源图命名时三条不同的文案都叫 ch02.block.yaopu，
    根本表达不了。目标这一侧天然唯一：一道闸门就是「拦住你进某张图」这一件事，
    读法也顺——`block.X` 就是被拦在 X 门外时说的那句话。

    第 1 章曾按源图命名、第 2 章按目标图命名，两套并存了一阵；那时八条文案
    内容都是对的，只是没有单一约定可供机器校验，下一个写地图的人只能靠猜。
    """
    match = re.match(r"^(ch\d+)_(.+)$", target_map)
    if match is None:
        return None
    return "%s.block.%s" % (match.group(1), match.group(2))


def check_deny_text_keys(maps: list[ParsedMap], report: Report) -> None:
    """规则 20：require_flag 与 deny_text_key 成对，且后者按目标图命名。

    引擎拦下玩家时播的就是 deny_text_key（WorldScene::tryStep）。缺了它，被拦的
    人只会觉得按键没反应，而不是「我还有事没办」——拦住却不说明还差什么，等于没拦。
    反过来，没有 require_flag 的 deny_text_key 一辈子播不出来，是个会骗人的摆设，
    十有八九是 require_flag 写漏了。

    命名这一条是给下一个写地图的人立的规矩：约定不成文就会分叉，而分叉之后
    「这个 key 取对了吗」就不再是机器能回答的问题。
    """
    for parsed in maps:
        rel = str(parsed.path.relative_to(ROOT))
        for portal in parsed.portals:
            props = portal["props"]
            need = props.get("require_flag", "")
            key = props.get("deny_text_key", "")
            where = f"{rel} 传送点 {portal['name']}"

            if need and not key:
                report.error(where, f"有 require_flag={need} 却没有 deny_text_key："
                                    "被拦住的玩家听不到自己还差什么，只会觉得按键没反应")
                continue
            if key and not need:
                report.error(where, f"有 deny_text_key={key} 却没有 require_flag："
                                    "引擎只在 require_flag 拦下时播它，这条文案永远播不出来"
                                    "（是不是漏写了 require_flag？）")
                continue
            if not key:
                continue
            want = expected_deny_key(props.get("target_map", ""))
            if want and key != want:
                report.error(where, f"deny_text_key={key} 不合命名约定，应为 {want}"
                                    "：一律按目标图命名，见 docs/map_spec.md 第 4.2 节")


def check_reference_pair(where, field, value, texts, flags, ids, report):
    """校验单条「字段 → 值」的引用。对象属性与地图属性共用这一处。"""
    if not isinstance(value, str) or not value:
        return
    probe = normalise_field(field)
    if probe.endswith(TEXT_KEY_SUFFIX):
        if texts and value not in texts:
            report.error(where, '文案 key 不存在：' + value + '（字段 ' + field + '）')
        return
    if probe.endswith(FLAG_PROP_SUFFIX):
        if flags and value not in flags:
            report.error(where, '旗标未在 data/flags.json 登记：' + value)
        return
    kind = REFERENCE_FIELDS.get(field) or REFERENCE_FIELDS.get(probe)
    if kind is None:
        return
    known = ids.get(kind, set())
    if known and value not in known:
        report.error(where, kind + ' id 不存在：' + value + '（字段 ' + field + '）')

def check_map_references(
    maps: list[ParsedMap], ids: dict[str, Path], texts: dict[str, str],
    flags: set[str], report: Report,
) -> None:
    """规则 6/7/8/9：角色、脚本、旗标、文案 key 的引用完整性。"""
    for parsed in maps:
        rel = str(parsed.path.relative_to(ROOT))
        # 地图属性本身也会引用文案：display_name_key 一直没人查过，
        # 第 1 章四个地名 key 就这样悬空躺在绿灯后面。
        payload = read_json(parsed.path, report)
        if isinstance(payload, dict):
            for key, value in map_properties(payload).items():
                check_reference_pair(rel, key, value, texts, flags, {}, report)

        for record in parsed.objects:
            where = f"{rel} 对象 {record['name']}"
            props = record["props"]

            if record["type"] == "npc":
                role_id = props.get("role_id", "")
                if ids and role_id and role_id not in ids:
                    report.error(where, f"role_id 在 data/ 中不存在：{role_id}")

            script = props.get("script", "")
            if script:
                if not (ROOT / "scripts" / script).exists():
                    report.error(where, f"脚本不存在：scripts/{script}")

            for key, value in props.items():
                if normalise_field(key).endswith(FLAG_PROP_SUFFIX) and value:
                    if flags and value not in flags:
                        report.error(where, f"旗标未在 data/flags.json 登记：{value}")
                if normalise_field(key).endswith(TEXT_KEY_SUFFIX) and value:
                    if texts and value not in texts:
                        report.error(where, f"文案 key 不存在：{value}")


def object_cells(record: dict) -> list[tuple[int, int]]:
    """对象占的全部格子。引擎的 objectAt 是矩形命中，校验器必须按同样的形状看。"""
    return [(record["x"] + dx, record["y"] + dy)
            for dx in range(record["w"])
            for dy in range(record["h"])]


def object_halo(record: dict) -> set[tuple[int, int]]:
    """能跟这个对象交互的站位：矩形本身 ∪ 矩形的四邻。

    WorldScene::interact 看的是「面朝的前一格」，所以贴着矩形站就够；矩形自身也
    算进来，是因为不挡路的对象（trigger）玩家可以站上去。
    """
    halo = set(object_cells(record))
    for x, y in object_cells(record):
        halo |= {(x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)}
    return halo


def spawn_landing(parsed: ParsedMap, spawn_id: str) -> dict | None:
    """按 spawn id 找落点；id 为空按引擎的规矩取 default spawn。"""
    if spawn_id:
        return parsed.spawns.get(spawn_id)
    for record in parsed.spawns.values():
        if str(record["props"].get("default", "")).lower() == "true":
            return record
    return None


def is_interactive(record: dict) -> bool:
    """按确认键能按出来的对象。portal 与 enter 触发是踏入型，不走这条。"""
    if record["type"] in ("npc", "facility"):
        return True
    return record["type"] == "trigger" and record["props"].get("mode") == "interact"


def is_step_activated(record: dict) -> bool:
    """踏上去才生效的对象：够得着不算，得真能站上去。"""
    return record["type"] == "portal" or (
        record["type"] == "trigger" and record["props"].get("mode") == "enter"
    )


def npc_always_present(record: dict) -> bool:
    """这个 npc 是不是**一定**在场。

    「在场」在引擎里是一件事，不是三件：画不画得出来、谈不谈得上话、占不占格，
    全看 `WorldScene::npcVisible` 这一个答案（tryStep 与 interact 都走
    `visibleNpcAt`）。带 visible_flag / hidden_flag 的那些于是「有时在、有时不在」。

    这一条是 2026-09-21 随引擎一起改的。在那之前引擎里占格与抢话都走
    `objectAt`，它压根不问可见性，所以校验器按「一律占格、一律抢先」算才是对的
    ——那时的注释写着「隐去的 NPC 就是一堵隐形墙」，如实描述了当时的引擎。
    引擎不再那样了，校验器必须跟着改：两边口径不一致比任何一边单独错更坏，
    因为绿灯会开始描述一个不存在的游戏。

    不确定的东西一律按「不算」处理，与规则 17 一贯的**只报必然**同一口径：
    不必然的墙不当墙，不必然的遮蔽不算遮蔽。
    """
    return not (record["props"].get("visible_flag") or record["props"].get("hidden_flag"))


def occupied_cells(parsed: ParsedMap) -> dict[tuple[int, int], str]:
    """npc 与 facility 占住的格子 → 占它的对象名。

    这就是 WorldScene::tryStep 在 walkable() 之外另挡的那两道，而命中按矩形算，
    所以一个 2x2 的设施实挡四格。

    带 visible_flag / hidden_flag 的 npc **不算**：引擎已改成「不画、不能谈的
    npc 也不占格」（tryStep 走 visibleNpcAt），它挡不挡路随旗标变，算不得一堵墙。
    见 npc_always_present。facility 没有可见性属性，一律占格。
    """
    occupied: dict[tuple[int, int], str] = {}
    for record in parsed.objects:
        if record["type"] not in ("npc", "facility"):
            continue
        if record["type"] == "npc" and not npc_always_present(record):
            continue
        for cell in object_cells(record):
            occupied.setdefault(cell, record["name"])
    return occupied


def collision_layer(parsed: ParsedMap, report: Report) -> list | None:
    """取这张图的 collision 数据；取不到就返回 None（规则 1/2 已经报过了）。"""
    payload = read_json(parsed.path, report)
    if payload is None:
        return None
    by_name = {str(layer.get("name", "")): layer for layer in payload.get("layers", []) or []}
    collision = by_name.get("collision", {}).get("data")
    if not isinstance(collision, list) or parsed.width <= 0:
        return None
    return collision


def cell_blocked(parsed: ParsedMap, collision: list, occupied: dict, cell: tuple[int, int]) -> bool:
    x, y = cell
    if x < 0 or y < 0 or x >= parsed.width or y >= parsed.height:
        return True
    index = y * parsed.width + x
    if index >= len(collision) or collision[index] != 0:
        return True
    return cell in occupied


def flood(parsed: ParsedMap, collision: list, occupied: dict,
          start: tuple[int, int]) -> set[tuple[int, int]]:
    """从 start 起按引擎的通行口径漫延。start 自身不判：玩家确实站在那一格上。"""
    seen = {start}
    queue = [start]
    while queue:
        x, y = queue.pop()
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            nxt = (x + dx, y + dy)
            if nxt in seen or cell_blocked(parsed, collision, occupied, nxt):
                continue
            seen.add(nxt)
            queue.append(nxt)
    return seen


def check_reachability(parsed: ParsedMap, report: Report) -> None:
    """规则 13：从 default spawn 出发，所有交互对象必须可达。

    连通口径必须与引擎逐字一致，否则这盏绿灯是假的。引擎里挡路的不只是
    collision 层：WorldScene::tryStep 还会挡下 npc 与 facility 所占的**整块矩形**
    （命中按矩形算）。只走 collision 的 BFS 会让「一个 2x2 的设施压在唯一走廊上」
    的地图一路绿灯，而玩家根本走不过去——校验器看的地形与玩家脚下的地形不是
    同一张图，是比不查更坏的事。

    带 visible_flag / hidden_flag 的 npc **不当墙**：tryStep 现在走 visibleNpcAt，
    不在场的 npc 不占格。口径见 npc_always_present——这一条 2026-09-21 随引擎
    一起改过，改之前是「一律占格」，那描述的是当时的引擎。

    portal 与 mode=enter 的 trigger 另有一条：它们是踏入型，玩家必须真的站上去
    才会生效，所以它们自己至少要有一格可走且可达。挨着够不着的传送点等于没有。
    """
    collision = collision_layer(parsed, report)
    if collision is None:
        return
    rel = str(parsed.path.relative_to(ROOT))

    defaults = [s for s in parsed.spawns.values() if str(s["props"].get("default", "")).lower() == "true"]
    if not defaults:
        return
    start = (defaults[0]["x"], defaults[0]["y"])
    occupied = occupied_cells(parsed)

    if cell_blocked(parsed, collision, occupied, start):
        where = occupied.get(start)
        reason = f"被 {where} 占住" if where else "collision 非 0"
        report.error(rel, f"default spawn 位于不可通行格 {start}（{reason}）")
        return

    seen = flood(parsed, collision, occupied, start)

    interactive = {"portal", "npc", "facility"}
    for record in parsed.objects:
        cells = object_cells(record)
        is_interactive = record["type"] in interactive or (
            record["type"] == "trigger" and record["props"].get("mode") == "interact"
        )
        step_on = record["type"] == "portal" or (
            record["type"] == "trigger" and record["props"].get("mode") == "enter"
        )
        where = f"{rel} 对象 {record['name']}"

        if is_interactive:
            # 对象自身可以放在不可通行格上（如柜台），只要矩形四周有一格可达即可交互。
            if not (object_halo(record) & seen):
                report.error(where, f"从出生点无法到达 {(record['x'], record['y'])}")

        if step_on and not any(c in seen for c in cells):
            report.error(where,
                         f"踏入型对象自身没有一格可走且可达 {(record['x'], record['y'])}"
                         "（玩家踩不上去，等于这个对象不存在）")



# WorldScene::interact 的问询顺序：npc → mode=interact 的 trigger → facility，
# 谁先应声谁就独占这一次确认键。tryStep 那边同理：enter 触发命中就 return，
# 排在它后面的 portal 收不到这一步。
INTERACT_ORDER = ("npc", "trigger", "facility")


def npc_verdict(record: dict) -> str:
    """这个 npc 拿到一次确认键会怎么处置：always 吃掉 / maybe 看旗标 / pass 放过。"""
    if not record["props"].get("script"):
        return "pass"      # 没脚本：命中了，但那个 if 不成立，落到下一档
    if not npc_always_present(record):
        return "maybe"     # 不在场时放过
    return "always"


def trigger_verdict(record: dict) -> str:
    if record["props"].get("mode") != "interact":
        return "pass"      # enter 触发在 interact 那条路上命中了也不成立
    if record["props"].get("guard_flag"):
        return "maybe"
    if str(record["props"].get("once", "")).lower() == "true":
        # 烧掉之后就让位，不是永久遮蔽——「蒲团上压一段剧情」正是这么设计的
        # （WorldScene::interact 的注释：剧情随时可错过，蒲团随时能坐）。
        return "maybe"
    return "always"


def check_interaction_shadow(parsed: ParsedMap, report: Report) -> None:
    """规则 17：没有对象会被同格的更高优先级对象永久压住。

    与规则 13 同族：那条问「走不走得到」，这条问「走到了按不按得出来」。

    可达不等于按得出来。引擎在一格上只问一遍：WorldScene::interact 依次问
    npc、trigger、facility，而每类只取**对象表里第一个**命中的。于是两种死法，
    都不会有任何报错，玩家只会觉得按键没反应：

      · 跨类：npc 压住 facility —— 面朝蒲团按确认，说话的是 NPC；
      · 同类：同类型里靠前的压住靠后的 —— 靠后的连命中都轮不到，哪怕靠前的
        那个没有脚本、本来就不该应声。

    只报**必然**发生的那些：带 visible_flag / hidden_flag 的 npc、带 guard_flag
    或 once=true 的触发器都可能让位，算不上遮蔽。

    2026-09-21 随引擎一起改的一条：同类里「靠前的那个」现在只从**一定在场**的
    npc 里取（npc_always_present）。引擎的 interact 走 visibleNpcAt，跳过不在场的
    npc，所以带旗标的那个既压不住谁，也不占那个「第一个命中」的位子。
    这一条正是「同格摆两个 npc、各带互补旗标」这种写法能成立的前提——
    一个 npc 对象只挂得住一个 script，那是给一个格子挂两段剧情的唯一办法。
    """
    rel = str(parsed.path.relative_to(ROOT))
    verdict = {"npc": npc_verdict, "trigger": trigger_verdict,
               "facility": lambda _record: "always"}

    # 每类、每格的「第一个命中」，连同它在对象表里的位置。对象表里的先后就是
    # 引擎逐个比对的先后。npc 这一类只收**一定在场**的：不在场的那些引擎直接跳过。
    #
    # 位置必须一起记下来：带旗标的 npc 自己不在这张表里，于是查出来的那个可能
    # 排在它**后面** —— 排在后面的压不住人，不能拿来当遮蔽的理由。
    first_hit: dict[str, dict[tuple[int, int], tuple[int, dict]]] = {t: {} for t in INTERACT_ORDER}
    for index, record in enumerate(parsed.objects):
        if record["type"] not in first_hit:
            continue
        if record["type"] == "npc" and not npc_always_present(record):
            continue
        for cell in object_cells(record):
            first_hit[record["type"]].setdefault(cell, (index, record))

    def scan(record, own_index, own_type, rank, higher_types):
        """逐格看这个对象有没有一格轮得到它。全被压住才报。"""
        reasons = []
        for cell in object_cells(record):
            mine = first_hit[own_type].get(cell)
            if mine is not None and mine[1] is not record and mine[0] < own_index:
                reasons.append(f"{cell} 被同类靠前的 {mine[1]['name']} 抢先命中")
                continue
            shadow = None
            for higher in higher_types[:rank]:
                other = first_hit[higher].get(cell)
                if other is not None and verdict[higher](other[1]) == "always":
                    shadow = other[1]
                    break
            if shadow is None:
                return None            # 这一格按得出来，够了
            reasons.append(f"{cell} 被 {shadow['name']}（{shadow['type']}）压住")
        return reasons

    for index, record in enumerate(parsed.objects):
        if not is_interactive(record):
            continue
        if record["type"] == "npc" and npc_verdict(record) == "pass":
            continue        # 没脚本的闲人本来就不接受交互，压不压都一样
        reasons = scan(record, index, record["type"],
                       INTERACT_ORDER.index(record["type"]), INTERACT_ORDER)
        if reasons:
            report.error(f"{rel} 对象 {record['name']}", "按不出来：" + "；".join(reasons))

    # tryStep 那边：objectAt(target, "trigger") 不分 mode 只取第一个，命中且
    # 是 enter 且 ready 就 return——排在后面的 portal 这一步收不到。
    step_first: dict[str, dict[tuple[int, int], dict]] = {"trigger": {}, "portal": {}}
    for record in parsed.objects:
        if record["type"] in step_first:
            for cell in object_cells(record):
                step_first[record["type"]].setdefault(cell, record)
    always_enter = {
        cell: record for cell, record in step_first["trigger"].items()
        if record["props"].get("mode") == "enter"
        and not record["props"].get("guard_flag")
        and str(record["props"].get("once", "")).lower() != "true"
    }

    for record in parsed.objects:
        if not is_step_activated(record):
            continue
        reasons = []
        blocked_all = True
        for cell in object_cells(record):
            mine = step_first[record["type"]].get(cell)
            if mine is not record:
                reasons.append(f"{cell} 被同类靠前的 {mine['name']} 抢先命中")
                continue
            if record["type"] == "portal" and cell in always_enter:
                reasons.append(f"{cell} 被 {always_enter[cell]['name']}"
                               "（无 guard 且非 once 的 enter 触发）抢在前面 return")
                continue
            blocked_all = False
            break
        if blocked_all and reasons:
            report.error(f"{rel} 对象 {record['name']}", "踏不响：" + "；".join(reasons))



# ---------------------------------------------------------------------------
# 脚本引用
# ---------------------------------------------------------------------------

# Lua 事件脚本里的引用。自造一个不存在的文案 key 或旗标不会让脚本报错——
# 缺文案时游戏原样回显 key，缺旗标时读出 0——所以必须在门禁里拦。
# 这个缺口真实发生过：第一版第 1 章脚本自造了 8 个文案 key 与 3 个旗标，
# 一路混到交付才被发现，因为当时只校验地图对脚本的引用，不看脚本引了什么。
SCRIPT_TEXT_CALLS = (
    # talk(speaker_id, text_key) —— 取第二个参数
    (re.compile(r"""(?<![A-Za-z0-9_])talk\s*\(\s*['"][^'"]*['"]\s*,\s*['"]([^'"]+)['"]"""), "文案"),
    # ending(title_key, text_key) —— 两个都是文案
    (re.compile(r"""(?<![A-Za-z0-9_])ending\s*\(\s*['"]([^'"]+)['"]\s*,\s*['"]([^'"]+)['"]"""), "文案"),
)
# talk() 的第一个参数是发言人 role id。此前只查了第二个参数（文案 key），
# 于是脚本引一个不存在的角色是绿的、数据建一个没人引的角色也是绿的，
# 两边各自通过门禁却互不相认，跑起来发言人栏会是空的。
SCRIPT_SPEAKER_CALL = re.compile(r"""(?<![A-Za-z0-9_])talk\s*\(\s*['"]([^'"]+)['"]""")
# flag.set 与 flag.get 都要查引用，但「谁置了这个旗标」只能看 set。
SCRIPT_FLAG_SET = re.compile(r"""(?<![A-Za-z0-9_])flag\.set\s*\(\s*['"]([^'"]+)['"]""")
SCRIPT_FLAG_CALL = re.compile(r"""(?<![A-Za-z0-9_])flag\.(?:set|get)\s*\(\s*['"]([^'"]+)['"]""")
# 只取写的那一半。once 触发器的兑现判据是「脚本自己置了那个完成旗标」，
# flag.get 不算数。
SCRIPT_FLAG_SET_CALL = re.compile(r"""(?<![A-Za-z0-9_])flag\.set\s*\(\s*['"]([^'"]+)['"]""")
# give / take 之外还要收 bottle.mature：它同样拿物品 id 当第一个参数，
# 漏了它的话，一个拼错的药名会安静地变成「背包里没有这一株」——脚本以为
# 自己浇了三滴，玩家什么也没看见，而门禁一片绿。
# 反斜杠在这里是要紧的：`bottle.mature` 的点若没转义，`bottleXmature` 也会
# 匹配。本项目栽过一次「正则的转义被 heredoc 吃掉、从此永远报通过」，
# 所以这条与它的负向自检（validate_selftest.py 用例 G）是一起加的。
# 第 7 章又加了两种按年份下限的写法（docs/interfaces-p3-ch07.md 第 4 节）：take_aged 与
# item.count_aged，同样拿物品 id 当第一个参数；`item\.count_aged` 的点也转义，负向自检是 G4–G6。
SCRIPT_ITEM_CALL = re.compile(
    r"""(?<![A-Za-z0-9_])(?:give|take|take_aged|bottle\.mature|item\.count_aged)\s*\(\s*['"]([^'"]+)['"]""")
SCRIPT_MAP_CALL = re.compile(r"""(?<![A-Za-z0-9_])teleport\s*\(\s*['"]([^'"]+)['"]""")
SCRIPT_CHOICE_CALL = re.compile(r"""(?<![A-Za-z0-9_])choice\s*[({]([^)}]*)[)}]""", re.S)
LUA_STRING = re.compile(r"""['"]([^'"]+)['"]""")


def strip_lua_comments(source: str) -> str:
    """去掉整行注释与行尾注释，避免注释里的示例 key 被当成真引用。"""
    out = []
    for line in source.split(chr(10)):
        marker = line.find("--")
        out.append(line if marker < 0 else line[:marker])
    return chr(10).join(out)


def check_trigger_once_contract(maps: list[ParsedMap], report: Report) -> None:
    """once 触发器必须由脚本自己的完成旗标兑现。

    引擎判「这一幕演过没有」，看的只是 set_flag 指的那个旗标
    （WorldScene::triggerReady），而这个旗标由脚本在演完之后自己置——引擎一个字
    都不预写。这条口径是为了让「查前置 → 说一句话 → return」的脚本烧不掉触发器，
    代价是三种写法会让 once 悄悄失效。它们都不会让任何工具报错，只在玩到那里才
    发作，所以必须在门禁上拦：

      · once=true 却没写 set_flag —— 引擎无从判断兑现与否，退化成可重复触发；
      · set_flag 指的旗标，脚本里一次都没置 —— 同上，这一幕会一遍遍重演；
      · once=false 却写了 set_flag —— 引擎不会去置它，这个属性在地图上不起任何
        作用，是个会骗人的摆设，十有八九是 once 写漏了。

    只查「脚本里出现过 flag.set(那个旗标)」，不做控制流分析：静态能确定的就这些。
    旗标被置在一条永远走不到的分支上，仍然只能靠测试与试玩抓；但「一次都没置」
    这种一眼可见的错，不该留到试玩。
    """
    for parsed in maps:
        rel = str(parsed.path.relative_to(ROOT))
        for record in parsed.objects:
            if record["type"] != "trigger":
                continue
            props = record["props"]
            once = str(props.get("once", "")).lower() == "true"
            mark = props.get("set_flag", "")
            where = f"{rel} 对象 {record['name']}"

            if not once:
                if mark:
                    report.error(where, "once=false 却写了 set_flag=" + mark +
                                 "：引擎不会去置它，这个属性不起任何作用"
                                 "（是不是漏写了 once=true？）")
                continue
            if not mark:
                report.error(where, "once=true 必须写 set_flag，指向脚本末尾置的那个"
                                    "完成旗标；缺了它引擎无从判断这一幕演过没有，"
                                    "once 形同虚设，剧情会一遍遍重演")
                continue

            script = props.get("script", "")
            if not script:
                continue
            path = ROOT / "scripts" / script
            if not path.exists():
                continue      # 脚本不存在由 check_map_references 报，不重复刷屏
            source = strip_lua_comments(path.read_text(encoding="utf-8"))
            if mark not in set(SCRIPT_FLAG_SET_CALL.findall(source)):
                report.error(where, "set_flag=" + mark + "，但 scripts/" + script +
                             " 里一次都没有 flag.set(" + mark + ")："
                             "这个 once 永远兑现不了，剧情会一遍遍重演")


# ---------------------------------------------------------------------------
# 脚本首部的挂点声明（@hook）
# ---------------------------------------------------------------------------
#
# 为什么有这一条（第 3 章独立校对 HIGH-1 / MEDIUM-6）：
#
# 本章最要害的缺陷是两种杀的触发方式在地图上装反了——夺舍挂成 interact、处决
# 挂成 enter，而两个脚本的首部注释写的正好相反，整段设计论证建立在脚本那一侧。
# 四条测试对这件事满格敏感，却一条也没发现，因为它们的驱动方式是**照着 tmj 写
# 出来的**：从被测物推导出来的判据永远发现不了被测物与规格不一致。
# 实测那一刻 19 个 ch03 脚本里有 12 个的首部误述了自己的挂点或触发方式。
#
# 散文注释治不了这件事：它可信度越高越危险，而且没有任何工具读得懂它。
# 所以每个脚本首部写一行机器可读的声明，由本检查比着 maps/*.tmj 校验：
#
#     -- @hook <map_id> <object_name> <mode> [once]
#
# 格式上的几处选择，各有来由：
#
#   * **一行一处挂点，一个脚本可以有多行。** 有的脚本确实挂在多处
#     （ch01/cunkou_bie.lua 既是村口的 enter 触发，又挂在韩母身上）。
#     写成一行多值就得再定一套分隔符，而多行是 grep 得动、diff 看得清的。
#   * **mode 取 enter / interact / npc 三个值。** 前两个是引擎认的触发方式
#     （src/game/WorldScene.cpp 的 tryStep 与 interact）；npc 对象没有 mode
#     属性——面朝它按确认是引擎对 npc 的固定做法，不是地图上的一个取值——
#     所以给它一个自己的取值，而不是硬塞成 interact。硬塞的话，「npc 与
#     interact 触发器写反了」这种错就查不出来了。
#     `touch` 不是有效取值（引擎不认），从前有脚本这么写过。
#   * **once 是一个可出现可不出现的字面量，不写 once=true/false。**
#     tmj 里 once 是 bool，这里只有「写了」与「没写」两种，与之一一对应。
#     代价是「没写」也是一种断言而不是沉默，所以第四个位置上出现任何别的词
#     一律报错，不许当成注释放过——否则写坏的声明会静静地变成 once=false。
#   * **声明必须在首部**（第一行代码之前的注释块里）。写在文件中段的声明
#     一律报错：这一行的作用是「打开文件第一眼就看得见」，埋在中间等于没有。
#
# 校验的是**双向**的集合相等，不是单向包含：
#   声明了 tmj 里没有的挂点 → 报；tmj 引用了脚本而脚本没声明这一处 → 同样报。
#   只查一个方向的话，删掉一行声明就能让检查闭嘴，那是又一条空转法。
#
# 负向自检在 tools/validate_selftest.py 用例 K。

HOOK_MODES = ("enter", "interact", "npc")

# 两步匹配是有意的：先**宽松**地认出「这一行想当一条声明」，再**严格**判它写得
# 对不对。若只留一条严格正则，一条写坏的声明会匹配不上、被当成普通注释悄悄放过
# ——那正是本项目那张空转法表里「找不到某个词就算过」的长相。
SCRIPT_HOOK_LINE = re.compile(r"^\s*--\s*@hook\b(.*)$")
# 注释块到此为止：第一行既不是空行也不是 `--` 注释的，就是正文了。
SCRIPT_COMMENT_LINE = re.compile(r"^\s*(--.*)?$")


def parse_hook_declarations(path: Path, rel: str, report: Report) -> set[tuple]:
    """读一个脚本首部的 @hook 声明。写坏的当场判错，不静默跳过。"""
    found: set[tuple] = set()
    in_header = True
    for number, line in enumerate(path.read_text(encoding="utf-8").split(chr(10)), 1):
        if in_header and not SCRIPT_COMMENT_LINE.match(line):
            in_header = False
        match = SCRIPT_HOOK_LINE.match(line)
        if match is None:
            continue
        where = rel + ":" + str(number)
        if not in_header:
            report.error(where, "@hook 声明必须写在文件首部（第一行代码之前的注释块里），"
                                "埋在正文中间的声明没有人第一眼看得见")
            continue
        parts = match.group(1).split()
        if len(parts) < 3 or len(parts) > 4:
            report.error(where, "@hook 的写法是 `-- @hook <地图 id> <对象名> <mode> [once]`，"
                                "实得 " + str(len(parts)) + " 个字段：" + repr(match.group(1)))
            continue
        map_id, obj_name, mode = parts[0], parts[1], parts[2]
        bad = False
        for label, value in (("地图 id", map_id), ("对象名", obj_name)):
            if not IDENTIFIER_RE.match(value):
                report.error(where, label + " 只能是小写字母、数字与下划线，实得：" + value)
                bad = True
        if mode not in HOOK_MODES:
            report.error(where, "mode 只能是 " + " / ".join(HOOK_MODES) +
                         "，实得：" + mode + "（`touch` 不是引擎认的取值）")
            bad = True
        once = False
        if len(parts) == 4:
            if parts[3] != "once":
                report.error(where, "第四个字段只能是字面量 once（表示 tmj 里 once=true），"
                                    "实得：" + parts[3])
                bad = True
            elif mode == "npc":
                report.error(where, "npc 对象没有 once 属性，这一行不许写 once")
                bad = True
            else:
                once = True
        if not bad:
            found.add((map_id, obj_name, mode, once))
    return found


def hook_truth(maps: list[ParsedMap]) -> dict[str, set[tuple]]:
    """maps/*.tmj 那一侧的真相：脚本相对路径 → 它被挂在哪几处。"""
    truth: dict[str, set[tuple]] = {}
    for parsed in maps:
        for record in parsed.objects:
            script = record["props"].get("script", "")
            if not script:
                continue
            if record["type"] == "npc":
                entry = (parsed.map_id, record["name"], "npc", False)
            else:
                entry = (parsed.map_id, record["name"],
                         str(record["props"].get("mode", "")),
                         str(record["props"].get("once", "")).lower() == "true")
            truth.setdefault(script.replace("\\", "/"), set()).add(entry)
    return truth


def describe_hook(entry: tuple) -> str:
    text = "%s %s %s" % (entry[0], entry[1], entry[2])
    return text + " once" if entry[3] else text


def check_hook_declarations(maps: list[ParsedMap], report: Report) -> None:
    """脚本首部的 @hook 声明与 maps/*.tmj 必须双向一致。"""
    scripts_dir = ROOT / "scripts"
    if not scripts_dir.exists():
        report.warn("scripts/", "目录不存在，跳过挂点声明检查")
        return

    truth = hook_truth(maps)
    # 先验分母：地图那一侧必须真的解析出了挂点。一张图也没解析出来时，下面每一条
    # 「声明了一个 tmj 里没有的挂点」都会成立，检查看着很忙，其实什么也没查。
    if maps and not truth:
        report.error("maps/", "解析出 " + str(len(maps)) +
                     " 张图，却一处带 script 的对象也没有："
                     "挂点声明检查会因此失去比对基准，先查地图解析")
        return

    seen: set[str] = set()
    for path in sorted(scripts_dir.rglob("*.lua")):
        if path.parent.name == "common":
            continue      # API 实现本身，不是事件脚本
        rel = str(path.relative_to(ROOT)).replace("\\", "/")
        script_id = rel[len("scripts/"):]
        seen.add(script_id)
        declared = parse_hook_declarations(path, rel, report)
        actual = truth.get(script_id, set())

        if not declared and not actual:
            report.error(rel, "既没有 @hook 声明，也没有任何地图引用它："
                              "这是一个谁也走不到的脚本。要么在地图上挂起来并补声明，"
                              "要么删掉")
            continue

        # 已经由「同一处、属性对不上」解释掉的 tmj 挂点，不再重复报一遍
        # 「首部没有这一条声明」——同一个毛病报两句话，读的人会以为是两件事。
        explained: set[tuple] = set()
        for entry in sorted(declared - actual):
            # 尽量说清楚差在哪一处：同名对象存在但属性对不上，与对象根本不存在，
            # 是两种完全不同的毛病，混成一句「声明与地图不符」等于要人自己去查。
            same_place = [e for e in actual if e[0] == entry[0] and e[1] == entry[1]]
            if same_place:
                explained.add(same_place[0])
                report.error(rel, "@hook " + describe_hook(entry) +
                             "：地图上这个对象的实际挂法是 " +
                             describe_hook(same_place[0]) + "，两者必须一致（以地图为准）")
                continue
            map_ids = {m.map_id for m in maps}
            if entry[0] not in map_ids:
                report.error(rel, "@hook " + describe_hook(entry) + "：没有这张图")
                continue
            owner = None
            for script, entries in truth.items():
                for other in entries:
                    if other[0] == entry[0] and other[1] == entry[1]:
                        owner = script
            if owner is not None:
                report.error(rel, "@hook " + describe_hook(entry) + "：" + entry[0] +
                             " 的对象 " + entry[1] + " 挂的是 scripts/" + owner +
                             "，不是本脚本")
            else:
                report.error(rel, "@hook " + describe_hook(entry) + "：" + entry[0] +
                             " 上没有叫 " + entry[1] + " 的带脚本对象")

        for entry in sorted(actual - declared - explained):
            report.error(rel, "地图把它挂在 " + describe_hook(entry) +
                         "，而首部没有这一条 @hook 声明："
                         "挂点必须由脚本自己写明，否则改了地图没有人会发现")

    for script_id in sorted(set(truth) - seen):
        # 脚本文件缺失由 check_map_references 报；这里只管 common/ 被挂上去这一种。
        if script_id.startswith("common/"):
            report.error("scripts/" + script_id,
                         "common/ 下是 API 实现，不该被地图当成事件脚本挂起来")


# 游戏从这张图开局：src/main.cpp 的 app.loadMap("ch01_hanjiacun", std::string{})，
# 第二个参数为空表示用该图自己的 default spawn。改开局地图要一并改这里。
START_MAP = "ch01_hanjiacun"


def script_teleport_targets() -> dict[str, set[str]]:
    """脚本相对路径 → 它 teleport() 到的那几张图。

    地图对象的 script 属性写的就是这个相对路径（如 ch01/sanshu.lua），两边对得上。
    有的图按设计只能靠剧情传送进去，没有任何传送点指向它 —— 那不叫断链。
    """
    targets: dict[str, set[str]] = {}
    scripts_dir = ROOT / "scripts"
    if not scripts_dir.exists():
        return targets
    for path in sorted(scripts_dir.rglob("*.lua")):
        try:
            source = strip_lua_comments(path.read_text(encoding="utf-8"))
        except (OSError, UnicodeDecodeError):
            continue
        found = set(SCRIPT_MAP_CALL.findall(source))
        if found:
            targets[path.relative_to(scripts_dir).as_posix()] = found
    return targets


def script_flag_sets() -> dict[str, set[str]]:
    """脚本相对路径 → 它会置的旗标。

    不做控制流分析：脚本里任何一处 flag.set(X) 都算「玩家有办法拿到 X」。这个
    方向的宽松是有意的 —— 死锁检查只该报**必然**拿不到的，不报「某个分支下可能
    拿不到」，否则每一处二选一都会变成一条噪音。
    """
    sets: dict[str, set[str]] = {}
    scripts_dir = ROOT / "scripts"
    if not scripts_dir.exists():
        return sets
    for path in sorted(scripts_dir.rglob("*.lua")):
        try:
            source = strip_lua_comments(path.read_text(encoding="utf-8"))
        except (OSError, UnicodeDecodeError):
            continue
        found = set(SCRIPT_FLAG_SET.findall(source))
        if found:
            sets[path.relative_to(scripts_dir).as_posix()] = found
    return sets


@dataclass
class World:
    """推演一遍世界所需的全部静态料。"""
    by_id: dict[str, ParsedMap]
    terrain: dict[str, tuple[list, dict]]
    teleports: dict[str, set[str]]
    script_flags: dict[str, set[str]]


def load_world(maps: list[ParsedMap], report: Report) -> World | None:
    by_id = {m.map_id: m for m in maps}
    if START_MAP not in by_id:
        report.error("maps/", f"起始地图 {START_MAP} 不在 maps/ 里"
                              "（src/main.cpp 从它开局，缺了就无从算起）")
        return None
    terrain: dict[str, tuple[list, dict]] = {}
    for parsed in maps:
        collision = collision_layer(parsed, report)
        if collision is None:
            return None         # 有图读不出来，算出来的断链多半是假的，不如不报
        terrain[parsed.map_id] = (collision, occupied_cells(parsed))
    return World(by_id, terrain, script_teleport_targets(), script_flag_sets())


def world_closure(world: World, honour_flags: bool) -> tuple[set[str], set[str]]:
    """从开局起把世界推到不动点，返回 (走得到的地图, 拿得到的旗标)。

    honour_flags=False 是规则 18 的口径：把 require_flag / guard_flag / visible_flag
    全当空气，只问「有没有路」。True 是规则 19 的口径：接上旗标，问「有没有一条
    合法的先后顺序」。两者共用这同一段推演，差集就是顺序死锁 —— 分成两份各写
    一遍，迟早会算出两种互相矛盾的「可达」。

    不动点是必须的：拿到新旗标会打开新的门，走到新的图又会带来新的旗标。
    """
    nodes: set[tuple[str, str]] = {(START_MAP, "")}
    flags: set[str] = set()
    changed = True
    while changed:
        changed = False
        for map_id, spawn_id in list(nodes):
            parsed = world.by_id.get(map_id)
            if parsed is None:
                continue        # 目标地图不存在，规则 5 已经报过
            here = spawn_landing(parsed, spawn_id)
            if here is None:
                continue        # 目标 spawn 不存在，规则 5 已经报过
            collision, occupied = world.terrain[map_id]
            seen = flood(parsed, collision, occupied, (here["x"], here["y"]))

            def gained(new: set[str]) -> bool:
                if new - flags:
                    flags.update(new)
                    return True
                return False

            for record in parsed.objects:
                props = record["props"]
                kind = record["type"]

                if kind == "portal":
                    if not any(c in seen for c in object_cells(record)):
                        continue    # 这个出口从这个落点走不到，不能当边用
                    need = props.get("require_flag", "")
                    if honour_flags and need and need not in flags:
                        continue
                    node = (props.get("target_map", ""), props.get("target_spawn", ""))
                    if node not in nodes:
                        nodes.add(node)
                        changed = True
                    continue

                script = props.get("script", "")
                if kind == "trigger":
                    # enter 要站得上去，interact 贴着站就行。
                    usable = (any(c in seen for c in object_cells(record))
                              if props.get("mode") == "enter"
                              else bool(object_halo(record) & seen))
                    guard = props.get("guard_flag", "")
                    if not usable or (honour_flags and guard and guard not in flags):
                        continue
                    produced = set(world.script_flags.get(script, ()))
                    if props.get("set_flag"):
                        produced.add(props["set_flag"])
                elif kind == "npc":
                    if not script or not (object_halo(record) & seen):
                        continue
                    shown = props.get("visible_flag", "")
                    if honour_flags and shown and shown not in flags:
                        continue
                    # hidden_flag 不拦：旗标只增不减，玩家总能赶在它置上之前先谈。
                    produced = set(world.script_flags.get(script, ()))
                else:
                    continue

                if gained(produced):
                    changed = True
                for target in world.teleports.get(script, ()):
                    if (target, "") not in nodes:
                        nodes.add((target, ""))
                        changed = True

    return {map_id for map_id, _ in nodes}, flags


def check_world_reachability(maps: list[ParsedMap], report: Report) -> None:
    """规则 18：每张图都要能从游戏起点一路走到。

    规则 13 是逐图做的：它只问「这张图里的东西够不够得着」，问不出「有没有人
    通向这张图」。第 2 章的入口正是这样一处 —— 神手谷南墙的 portal_to_ch02_yaopu
    一旦掉了，ch02 的四张图会整体从游戏里消失，而每一张单看都完全合规，
    validate 一路绿灯。这一处不是假想：重跑 tools/mapgen/genmaps.py 就会抹掉它
    （见那份 docstring），当时全套门禁一声不吭。

    走的是 (地图, 落点) 二元组而不是地图：从哪个 spawn 落地，决定了你在这张图里
    走得到哪几个传送点 —— 一张图「进得去」不等于它的每个出口都用得上。通行口径
    与规则 13 共用同一份 flood()，包括 npc / facility 占的整块矩形。

    不参与判定的两样：require_flag 拦的是时机不是通路，照走；踏不上去的传送点
    不算通路，但那一条规则 13 已经报过，这里只是不把它当边用，不重复报。
    """
    world = load_world(maps, report)
    if world is None:
        return
    reached, _flags = world_closure(world, honour_flags=False)

    for parsed in maps:
        if parsed.map_id in reached:
            continue
        report.error(str(parsed.path.relative_to(ROOT)),
                     f"从起始地图 {START_MAP} 走不到这张图："
                     "没有任何走得到的传送点、也没有任何 teleport() 指向它")


GATE_PROPS = (
    ("require_flag", "传送点过不去"),
    ("guard_flag", "触发器起不来"),
    ("visible_flag", "NPC 不出现"),
)


def check_flag_order(maps: list[ParsedMap], report: Report) -> None:
    """规则 19：每道闸门的旗标都要拿得到 —— 存在一条合法的推进顺序。

    规则 18 是把 require_flag 当空气算的，它问「有没有路」。这一条把旗标接上，
    问「按旗标的先后，走不走得通」。两者的差集正是顺序死锁：**门是通的，钥匙
    锁在门后**。

    典型写法：韩家村出村要 ch01.muqin_bie，而置这个旗标的村口话别却被挂上了
    只有镇上才拿得到的 guard_flag —— 每一处单看都合规，连通性也满分，玩家却
    卡在第一张图里出不去。这种错不会有任何工具报警，只会在有人从头玩一遍时
    才发作，而「从头玩一遍」正是最贵的那种验证。

    口径上刻意宽松，只报**必然**的死锁：
      · 脚本里任何一处 flag.set(X) 都算拿得到 X，不做分支分析 —— 否则每个
        二选一都会变成噪音；
      · hidden_flag 不拦：旗标只增不减，玩家总能赶在它置上之前先把事办了；
      · 遮蔽（规则 17）不参与推演 —— 那边已经报过，这里再报一遍只是刷屏。
    """
    world = load_world(maps, report)
    if world is None:
        return
    open_reach, _ = world_closure(world, honour_flags=False)
    reached, flags = world_closure(world, honour_flags=True)

    # 1. 因为旗标顺序而掉出去的图。纯断链归规则 18 报，这里不重复。
    for parsed in maps:
        if parsed.map_id in reached or parsed.map_id not in open_reach:
            continue
        report.error(str(parsed.path.relative_to(ROOT)),
                     "按旗标顺序走不到这张图（不看 require_flag 时走得到）："
                     "门是通的，钥匙锁在门后 —— 顺序死锁")

    # 2. 永远开不了的闸门。只看走得到的图，免得一处断链牵出一串下游噪音。
    for parsed in maps:
        if parsed.map_id not in reached:
            continue
        rel = str(parsed.path.relative_to(ROOT))
        for record in parsed.objects:
            for prop_name, consequence in GATE_PROPS:
                need = record["props"].get(prop_name, "")
                if not need or need in flags:
                    continue
                report.error(f"{rel} 对象 {record['name']}",
                             f"{prop_name}={need} 从开局出发永远置不上"
                             f"（没有任何拿得到的触发器或脚本会置它），{consequence}")


def check_scripts(ids, texts, flags, maps, report, defined=None):
    scripts_dir = ROOT / "scripts"
    if not scripts_dir.exists():
        report.warn("scripts/", "目录不存在，跳过脚本检查")
        return
    map_ids = {m.map_id for m in maps}

    for path in sorted(scripts_dir.rglob("*.lua")):
        # common/ 下是 API 实现本身，里面的字符串是参数名不是引用。
        if path.parent.name == "common":
            continue
        rel = str(path.relative_to(ROOT))
        source = strip_lua_comments(path.read_text(encoding="utf-8"))

        wanted_texts: set[str] = set()
        for pattern, _kind in SCRIPT_TEXT_CALLS:
            for match in pattern.finditer(source):
                wanted_texts.update(g for g in match.groups() if g)
        for match in SCRIPT_CHOICE_CALL.finditer(source):
            wanted_texts.update(LUA_STRING.findall(match.group(1)))

        for key in sorted(wanted_texts):
            if texts and key not in texts:
                report.error(rel, f"文案 key 不存在：{key}")

        for name in sorted(set(SCRIPT_FLAG_CALL.findall(source))):
            if flags and name not in flags:
                report.error(rel, f"旗标未在 data/flags.json 登记：{name}")

        for item in sorted(set(SCRIPT_ITEM_CALL.findall(source))):
            if ids and item not in ids:
                report.error(rel, f"物品 id 在 data/ 中不存在：{item}")

        for target in sorted(set(SCRIPT_MAP_CALL.findall(source))):
            if map_ids and target not in map_ids:
                report.error(rel, f"传送目标地图不存在：{target}")

        roles = (defined or {}).get('role', set())
        for speaker in sorted(set(SCRIPT_SPEAKER_CALL.findall(source))):
            if roles and speaker not in roles:
                report.error(rel, f"说话人不在 data/roles 中：{speaker}")


def walk_json(node, prefix, out):
    """把嵌套 JSON 摊平成 (字段路径, 值) 对，供引用检查逐条看。"""
    if isinstance(node, dict):
        for key, value in node.items():
            walk_json(value, prefix + [str(key)], out)
    elif isinstance(node, list):
        for item in node:
            walk_json(item, prefix, out)
    else:
        out.append((prefix[-1] if prefix else '', node))


def collect_reference_ids(report):
    """按类别收集 data/ 下各子目录里定义的 id，供引用校验比对。"""
    result = {}
    for kind, subdir in REFERENCE_SOURCES.items():
        target = ROOT / 'data' / subdir
        found = set()
        if target.exists():
            for path in sorted(target.rglob('*.json')):
                payload = read_json(path, report)
                if isinstance(payload, dict) and payload.get('id'):
                    found.add(str(payload['id']))
        result[kind] = found
    return result


def derived_map_name_key(map_id: str) -> str:
    """地图名文案 key 的规范形：ch02_yaopu → ch02.map.yaopu.name。"""
    head, sep, tail = map_id.partition("_")
    if not sep or not tail:
        return ""
    return head + ".map." + tail + ".name"


def check_map_name_keys(maps: list[ParsedMap], report: Report) -> None:
    """规则 21：display_name_key 必须是由地图 id 推出来的那一个。

    这条规则是为了让引擎**能在没载入某张图的时候说出它的名字**：目标提示要写
    「前往 七玄门外刃堂」而不是「前往 ch02_wairentang」，而引擎同一时刻只载一张图。
    core::mapDisplayNameKey 按这个形状拼 key，拼出来的东西能不能查到，全靠这条规则。

    没有它就不许那么拼——一条没人守的命名约定，正是本工程在瓦片编号上栽过的
    那种「两份约定分了家」：两边各自自洽，谁也不报警。
    """
    for parsed in maps:
        payload = read_json(parsed.path, report)
        if payload is None:
            continue
        props = {p.get("name"): p.get("value") for p in payload.get("properties", [])}
        actual = str(props.get("display_name_key", ""))
        wanted = derived_map_name_key(parsed.map_id)
        rel = str(parsed.path.relative_to(ROOT))
        if not wanted:
            report.error(rel, f"地图 id {parsed.map_id} 不含下划线，推不出地图名 key")
        elif actual != wanted:
            report.error(rel, f"display_name_key 应为 {wanted}，实际是 {actual or '(空)'}："
                              "引擎要按 id 推这个 key 才能说出别张图的名字")


def check_objectives(maps: list[ParsedMap], flags: set[str], report: Report) -> None:
    """规则 22：目标链的每一步都得指得到、也走得完。

    目标链（data/objectives/chNN.json）是玩家在屏幕上看到的「现在该做什么、
    该去哪」。它错的方式只有两种，两种都不会自己暴露：

      · **指到一个不存在的对象上** —— 画面上就是没有标记，与「目标系统没做」
        长得一模一样；
      · **完成旗标没有任何脚本会置** —— 那一步永远完不成，目标行从此卡死在
        这里，后面的步骤再也不会出现。

    所以两条都在这里查死。旗标是否登记、文案 key 是否存在由 check_data_references
    顺带查了（字段名以 _flag / _key 结尾），这里不重复。
    """
    objectives_dir = ROOT / "data" / "objectives"
    if not objectives_dir.exists():
        return

    by_id = {parsed.map_id: parsed for parsed in maps}
    sets_by_script = script_flag_sets()
    settable = set()
    for found in sets_by_script.values():
        settable |= found

    for path in sorted(objectives_dir.glob("*.json")):
        payload = read_json(path, report)
        if payload is None:
            continue
        rel = str(path.relative_to(ROOT))
        steps = payload.get("steps")
        if not isinstance(steps, list) or not steps:
            report.error(rel, "目标链没有 steps，或 steps 是空的")
            continue

        seen_ids = set()
        for index, step in enumerate(steps):
            if not isinstance(step, dict):
                report.error(rel, f"第 {index + 1} 步不是对象")
                continue
            where = f"{rel} 第 {index + 1} 步（{step.get('id', '(无 id)')}）"

            step_id = str(step.get("id", ""))
            if not step_id:
                report.error(where, "缺少 id")
            elif step_id in seen_ids:
                report.error(where, f"id 在本章内重复：{step_id}")
            else:
                seen_ids.add(step_id)

            done_flag = str(step.get("done_flag", ""))
            if not done_flag:
                report.error(where, "缺少 done_flag：这一步永远完不成")
            elif flags and done_flag not in flags:
                # check_data_references 也会报一次，但那条只说「没登记」；
                # 这里要说清楚后果，否则改的人未必知道这一步会把目标链卡死。
                report.error(where, f"done_flag 未登记：{done_flag}")
            elif settable and done_flag not in settable:
                report.error(where, f"done_flag={done_flag} 没有任何脚本会置它："
                                    "这一步永远完不成，目标提示会永远卡在这里")

            target_map = str(step.get("target_map", ""))
            target_object = str(step.get("target_object", ""))
            parsed = by_id.get(target_map)
            if not target_map:
                report.error(where, "缺少 target_map")
            elif parsed is None:
                report.error(where, f"target_map 不存在：{target_map}")
            elif not target_object:
                report.error(where, "缺少 target_object")
            elif all(obj["name"] != target_object for obj in parsed.objects):
                report.error(where, f"{target_map} 上没有名为 {target_object} 的对象："
                                    "目标标记会画不出来")


# ---------------------------------------------------------------------------
# 规则 23：任务（docs/interfaces-p3-ch05.md 第 1 节，设计 10.4 Q10）
# ---------------------------------------------------------------------------

# 四层各自认得的字段。表外的一律报错：拼错一个 fail 会让任务永远不会过期，拼错一个
# target_object 会让它悄悄变成「不指路」——那两种错都不会自己暴露。C++ 加载器
# （src/io/QuestLoader.cpp）拒收同样的东西，两边的表要一起改。
QUEST_TOP_FIELDS = {
    "id", "name", "chapter", "kind", "title_key", "summary_key", "accept", "steps",
    "complete", "fail", "fail_text_key", "rewards", "origin", "note",
}
QUEST_STEP_FIELDS = {"id", "text_key", "done", "target_map", "target_object"}
QUEST_CONDITION_FIELDS = {"flag", "item", "op", "value"}
QUEST_REWARD_FIELDS = {"item_id", "count", "herb_age"}
QUEST_FIRST_CHAPTER = 1
QUEST_LAST_CHAPTER = 14


def is_plain_int(value) -> bool:
    """整数，且不是布尔（Python 里 True 也是 int，而 JSON 的 true 不是整数）。"""
    return isinstance(value, int) and not isinstance(value, bool)


def check_quest_condition(where, cond, flags, settable, items, report):
    """一条谓词：恰好三种写法之一，旗标已登记且有脚本会置，物品存在。"""
    if not isinstance(cond, dict):
        report.error(where, "谓词必须是对象")
        return
    unknown = sorted(set(cond) - QUEST_CONDITION_FIELDS)
    if unknown:
        report.error(where, f"谓词里有不认识的字段 {'、'.join(unknown)}（只认 flag / item / op / value）")
    has_flag = "flag" in cond
    has_item = "item" in cond
    if has_flag == has_item:
        report.error(where, "谓词里 flag 与 item 必须恰好出现一个")
        return
    subject = cond.get("flag" if has_flag else "item")
    if not isinstance(subject, str) or not subject:
        report.error(where, ("flag" if has_flag else "item") + " 必须是非空字符串")
        subject = ""
    op = cond.get("op")
    value = cond.get("value")
    if op not in (">=", "=="):
        report.error(where, f"op 只认 \">=\" 或 \"==\"，实际是 {op!r}")
    elif not is_plain_int(value):
        report.error(where, f"value 必须是整数，实际是 {value!r}")
    elif has_item and op != ">=":
        report.error(where, "物品谓词只认 \">=\"")
    elif op == ">=" and value < 1:
        report.error(where, f"{'物品' if has_item else '旗标'} >= 的 value 至少为 1（>= {value} 恒真，是一句空话）")
    elif op == "==" and value < 0:
        report.error(where, "旗标 == 的 value 不许为负")

    if has_flag and subject:
        if flags and subject not in flags:
            report.error(where, f"旗标未在 data/flags.json 登记：{subject}")
        elif subject not in settable:
            # 没人置的旗标：放在 accept 里任务永远接不到，放在 complete 里永远了结不了，
            # 放在 fail 里永远不会过期——三种都不会自己暴露。
            report.error(where, f"旗标 {subject} 没有任何脚本会置它：这条谓词永远不会成立")
        elif PATH_FLAG_RE.match(subject) and op in (">=", "==") and is_plain_int(value) and value > 1:
            report.error(where, f"路径行动旗标 {subject} 只会置为 1，无法满足 {op} {value}")
    if has_item and subject and items and subject not in items:
        report.error(where, f"物品 id 不存在：{subject}")


def check_quest_condition_list(where, payload, field, required, flags, settable, items, report):
    """一张谓词表。required 为真时缺字段或空表都报。"""
    if field not in payload:
        if required:
            report.error(where, f"缺少 {field}")
        return []
    conds = payload[field]
    if not isinstance(conds, list):
        report.error(where, f"{field} 必须是数组")
        return []
    if required and not conds:
        report.error(where, f"{field} 不许为空")
    for index, cond in enumerate(conds):
        check_quest_condition(f"{where} 的 {field}[{index}]", cond, flags, settable, items, report)
    return conds


def check_quests(maps: list[ParsedMap], flags: set[str], defined: dict, report: Report,
                 path_sources: set[str]) -> None:
    """规则 23：任务文件（data/quests/**/*.json）的形状与引用。

    任务只是一张「怎么从旗标和背包读出进度」的表（契约 1.1）。它错的方式与目标链一样
    不会自己暴露：一条没人置的旗标让任务永远接不到或永远了结不了，一个拼错的字段让任务
    永远不会过期，而这些在第一次游玩时看起来一切正常。

    文案 key 存不存在、奖励的 item_id 存不存在，由 check_data_references 顺带查了（字段名以
    _key / _id 结尾），这里只查它们**必须写**；谓词里的 flag / item 不走那条通路，这里查。
    另查「一章一条主线」：主线由目标链原地生成（契约 1.5），两个目标链文件同属一章，
    主线任务就说不清是哪一条。
    """
    rel_root = ROOT / "data"
    objectives_dir = rel_root / "objectives"
    if objectives_dir.exists():
        seen_chapter: dict[int, str] = {}
        for path in sorted(objectives_dir.glob("*.json")):
            payload = read_json(path, report)
            if not isinstance(payload, dict):
                continue
            chapter = payload.get("chapter")
            rel = str(path.relative_to(ROOT))
            if chapter in seen_chapter:
                report.error(rel, f"与 {seen_chapter[chapter]} 同属第 {chapter} 章：一章只许一条主线")
            else:
                seen_chapter[chapter] = rel

    quests_dir = rel_root / "quests"
    if not quests_dir.exists():
        return

    by_id = {parsed.map_id: parsed for parsed in maps}
    settable = set(path_sources)
    for found in script_flag_sets().values():
        settable |= found
    items = defined.get("item", set())

    for path in sorted(quests_dir.rglob("*.json")):
        payload = read_json(path, report)
        if payload is None:
            continue
        rel = str(path.relative_to(ROOT))
        if not isinstance(payload, dict):
            report.error(rel, "任务文件顶层必须是对象")
            continue

        unknown = sorted(set(payload) - QUEST_TOP_FIELDS)
        if unknown:
            report.error(rel, f"有不认识的字段 {'、'.join(unknown)}"
                              "（字段表见 docs/interfaces-p3-ch05.md 1.2）")

        quest_id = payload.get("id")
        if isinstance(quest_id, str) and quest_id and path.stem != quest_id:
            report.error(rel, f"文件名必须是 {quest_id}.json（一任务一文件，契约 1.2）")

        kind = payload.get("kind")
        if kind != "side":
            report.error(rel, f"kind 只许 \"side\"，实际是 {kind!r}：主线由目标链原地生成（契约 1.5）")

        chapter = payload.get("chapter")
        if not is_plain_int(chapter) or not (QUEST_FIRST_CHAPTER <= chapter <= QUEST_LAST_CHAPTER):
            report.error(rel, f"chapter 必须是 {QUEST_FIRST_CHAPTER}–{QUEST_LAST_CHAPTER} 的整数，实际是 {chapter!r}")

        for key_field in ("title_key", "summary_key"):
            value = payload.get(key_field)
            if not isinstance(value, str) or not value:
                report.error(rel, f"缺少 {key_field}")

        check_quest_condition_list(rel, payload, "accept", True, flags, settable, items, report)

        steps = payload.get("steps")
        if not isinstance(steps, list) or not steps:
            report.error(rel, "steps 必须是非空数组")
            steps = []
        step_ids: set[str] = set()
        for index, step in enumerate(steps):
            where = f"{rel} 的 steps[{index}]"
            if not isinstance(step, dict):
                report.error(where, "步骤必须是对象")
                continue
            where = f"{where}（{step.get('id', '(无 id)')}）"
            unknown = sorted(set(step) - QUEST_STEP_FIELDS)
            if unknown:
                report.error(where, f"步骤里有不认识的字段 {'、'.join(unknown)}")
            step_id = step.get("id")
            if not isinstance(step_id, str) or not step_id:
                report.error(where, "步骤缺少 id")
            elif step_id in step_ids:
                report.error(where, f"步骤 id 在本任务内重复：{step_id}")
            else:
                step_ids.add(step_id)
            text_key = step.get("text_key")
            if not isinstance(text_key, str) or not text_key:
                report.error(where, "步骤缺少 text_key")
            check_quest_condition_list(where, step, "done", True, flags, settable, items, report)

            target_map = step.get("target_map", "")
            target_object = step.get("target_object", "")
            if bool(target_map) != bool(target_object):
                report.error(where, "target_map 与 target_object 必须成对：要么都写，要么都不写")
            elif target_map:
                parsed = by_id.get(str(target_map))
                if parsed is None:
                    report.error(where, f"target_map 不存在：{target_map}")
                elif all(obj["name"] != target_object for obj in parsed.objects):
                    report.error(where, f"{target_map} 上没有名为 {target_object} 的对象")

        check_quest_condition_list(rel, payload, "complete", True, flags, settable, items, report)
        fail = check_quest_condition_list(rel, payload, "fail", False, flags, settable, items, report)
        fail_text_key = payload.get("fail_text_key")
        if fail and (not isinstance(fail_text_key, str) or not fail_text_key):
            report.error(rel, "写了 fail 就必须写 fail_text_key：过期要给一句原因（Q7）")
        elif not fail and "fail_text_key" in payload:
            report.error(rel, "fail 为空却写了 fail_text_key：这句话永远说不出口")

        rewards = payload.get("rewards", [])
        if not isinstance(rewards, list):
            report.error(rel, "rewards 必须是数组")
            rewards = []
        for index, reward in enumerate(rewards):
            where = f"{rel} 的 rewards[{index}]"
            if not isinstance(reward, dict):
                report.error(where, "奖励必须是对象")
                continue
            unknown = sorted(set(reward) - QUEST_REWARD_FIELDS)
            if unknown:
                report.error(where, f"奖励里有不认识的字段 {'、'.join(unknown)}")
            if not isinstance(reward.get("item_id"), str) or not reward.get("item_id"):
                report.error(where, "奖励缺少 item_id")
            count = reward.get("count")
            if not is_plain_int(count) or count < 1:
                report.error(where, f"奖励的 count 必须是 ≥ 1 的整数，实际是 {count!r}")
            if "herb_age" in reward and (not is_plain_int(reward["herb_age"]) or reward["herb_age"] < 0):
                report.error(where, f"奖励的 herb_age 必须是 ≥ 0 的整数，实际是 {reward['herb_age']!r}")

        for text_field in ("origin", "note"):
            if text_field in payload and not isinstance(payload[text_field], str):
                report.error(rel, f"{text_field} 必须是字符串")


# ---------------------------------------------------------------------------
# 规则 26：路径行动（docs/interfaces-octo-pathactions.md 第 2、4 节）
# ---------------------------------------------------------------------------
#
# 路径行动挂在 NPC 身上：拼错一个对象名，那个人身上就什么也没有，与「这一章本来就没给他写」
# 在画面上一模一样，没人会来报。所以形状与引用 C++ 加载器（src/io/PathActionLoader.cpp）
# 也查一遍；这里另查加载器查不了的几件：谓词里的旗标有人会置、同一 NPC 同类条目的时段
# 不重叠、条目时段落在 NPC 在场的时段里、揭破绽的角色真的会上阵、pending 的切磋还没建编成。
# 下面几张表与 C++ 加载器是同一张，两边要一起改。
PATH_TOP_FIELDS = {"id", "name", "chapter", "entries", "note"}
PATH_COMMON_FIELDS = {"id", "kind", "map", "npc", "when", "until", "realm", "done_flag",
                      "text_key", "refuse_key", "origin", "note"}
PATH_KIND_FIELDS = {
    "inquire": {"reveal", "give", "set_flags"},
    "purchase": {"item_id", "count", "herb_age", "price", "deal_key", "poor_key"},
    "challenge": {"battle", "pending", "win_key", "lose_key", "reward"},
}
PATH_KIND_WORD = {"inquire": "dating", "purchase": "qiugou", "challenge": "qiecuo"}
PATH_KIND_KEYS = {
    "inquire": ("text_key",),
    "purchase": ("text_key", "deal_key", "poor_key"),
    "challenge": ("text_key", "win_key", "lose_key"),
}
# 攻击类别（施工图 2.2：兵刃五种、五行五种）。
# 与下文破势与蓄劲那一块的 ALL_CATEGORIES 是同一张表（中文名，src/core/model/AttackCategory.h
# 的镜像）；这里另写一份字面量，是因为模块级常量按文件顺序求值，那一块在后面。
PATH_WEAKNESS_CATEGORIES = {"剑", "刀", "拳", "暗器", "毒", "金", "木", "水", "火", "土"}
# 境界编号，与存档同一个数（src/core/rules/Realm.h）：凡人 0、炼气 1–13、筑基 21–23、结丹 31–33。
PATH_REALMS = set(range(0, 14)) | {21, 22, 23, 31, 32, 33}
PATH_ID_RE = re.compile(r"^[a-z0-9]+(?:_[a-z0-9]+)*_(dating|qiugou|qiecuo)[0-9]*$")
PATH_FLAG_RE = re.compile(r"^ch\d\d\.path\.[a-z0-9_]+$")
PATH_FILE_RE = re.compile(r"^ch(\d\d)$")
CHAPTER_FLAG_RE = re.compile(r"^ch(\d\d)\.")
CHAPTER_DONE_RE = re.compile(r"^ch(\d\d)\.done$")


def path_condition(cond):
    """把一条谓词读成 (类别, 主语, op, value)。写坏了的返回 None——那种错由谓词检查报，这里只推理。"""
    if not isinstance(cond, dict):
        return None
    has_flag, has_item = "flag" in cond, "item" in cond
    if has_flag == has_item:
        return None
    subject = cond.get("flag" if has_flag else "item")
    op, value = cond.get("op"), cond.get("value")
    if not isinstance(subject, str) or op not in (">=", "==") or not is_plain_int(value):
        return None
    return ("flag" if has_flag else "item", subject, op, value)


def path_requires_nonzero(c) -> bool:
    """这条谓词成立时，它那个旗标一定不为 0。"""
    return c[0] == "flag" and c[3] >= 1


def path_chapter_of(flag_name: str):
    match = CHAPTER_FLAG_RE.match(flag_name)
    return int(match.group(1)) if match else None


def path_implies(a, b) -> bool:
    """a 成立时 b 必然成立。只用谓词本身与一条公理推，推不出来就算不成立（宁可多报）。

    公理：章是按顺序走的——第 M 章的旗标置上了，第 N 章（N < M）一定已经收尾。
    它靠两半前提：
      · 旗标不会在它那一章之前被置上：脚本不置以后的章的旗标（check_path_axiom_premise 查），
        路径条目只置本章的 chNN.path.*、且 when 锚在本章（check_path_entry 查）；
      · 第 M 章的脚本只在 ch(M-1).done 之后才跑得到——这一半要做可达性推演，**没有机器查**，
        靠的是每章的挂点都锚在本章（目标链、地图对象的 require_flag / guard_flag）。
    """
    if a[0] == b[0] and a[1] == b[1]:
        if b[2] == ">=":
            return a[3] >= b[3]
        return a[2] == "==" and a[3] == b[3]
    done = CHAPTER_DONE_RE.match(b[1]) if b[0] == "flag" else None
    if done and b[2] == ">=" and b[3] == 1 and a[0] == "flag" and path_requires_nonzero(a):
        later = path_chapter_of(a[1])
        return later is not None and later > int(done.group(1))
    return False


def path_contradicts(a, b) -> bool:
    """a 与 b 不可能同时成立。"""
    if a[0] == "flag" and b[0] == "flag" and a[1] == b[1]:
        if a[2] == "==" and b[2] == "==":
            return a[3] != b[3]
        if a[2] == "==":
            return a[3] < b[3]
        if b[2] == "==":
            return b[3] < a[3]
    # 公理的反面：第 N 章还没收尾（chNN.done == 0），第 M 章（M > N）的旗标就不可能已置。
    for x, y in ((a, b), (b, a)):
        done = CHAPTER_DONE_RE.match(x[1]) if x[0] == "flag" else None
        if done and x[2] == "==" and x[3] == 0 and path_requires_nonzero(y):
            later = path_chapter_of(y[1])
            if later is not None and later > int(done.group(1)):
                return True
    return False


def path_windows_disjoint(a: dict, b: dict) -> bool:
    """两条条目的时段能不能证明不重叠：一边的出现条件与另一边的出现条件相斥，
    或一边的出现条件蕴含另一边的某条消失条件。"""
    if any(path_contradicts(x, y) for x in a["when"] for y in b["when"]):
        return True
    if any(path_implies(x, u) for x in a["when"] for u in b["until"]):
        return True
    return any(path_implies(y, u) for y in b["when"] for u in a["until"])


def path_window_can_open(record: dict, available: set[str]) -> bool:
    """单条窗口的整数约束：when 全成立、until 全不成立；路径旗只能由 0 置为 1。"""
    when, until = record["when"], record["until"]
    if any(path_contradicts(a, b) for a in when for b in when):
        return False
    if any(path_implies(a, b) for a in when for b in until):
        return False
    for kind, subject in {(c[0], c[1]) for c in when + until}:
        low, high, excluded = 0, None, set()
        if kind == "flag":
            if subject not in available:
                high = 0
            elif PATH_FLAG_RE.match(subject):
                high = 1
        for c in when:
            if c[:2] != (kind, subject):
                continue
            low = max(low, c[3])
            if c[2] == "==":
                high = c[3] if high is None else min(high, c[3])
        # until 是 OR；窗口开启须同时满足每条的反面：>=n 变成 <n，==n 排除 n。
        for c in until:
            if c[:2] != (kind, subject):
                continue
            if c[2] == ">=":
                high = c[3] - 1 if high is None else min(high, c[3] - 1)
            else:
                excluded.add(c[3])
        if high is not None and (low > high or
                high - low + 1 <= sum(low <= value <= high for value in excluded)):
            return False
    return True


def load_battle_payloads(report: Report) -> dict[str, dict]:
    """data/battles 下每场编成：id → 原始 JSON。"""
    battles: dict[str, dict] = {}
    battles_dir = ROOT / "data" / "battles"
    if battles_dir.exists():
        for path in sorted(battles_dir.rglob("*.json")):
            payload = read_json(path, report)
            if isinstance(payload, dict) and payload.get("id"):
                battles[str(payload["id"])] = payload
    return battles


def check_path_bag(where, node, items, report) -> None:
    """一件物品：{item_id, count ≥ 1, herb_age ≥ 0（可缺省）}。"""
    if not isinstance(node, dict):
        report.error(where, "物件必须是对象")
        return
    unknown = sorted(set(node) - {"item_id", "count", "herb_age"})
    if unknown:
        report.error(where, f"物件里有不认识的字段 {'、'.join(unknown)}")
    item = node.get("item_id")
    if not isinstance(item, str) or not item:
        report.error(where, "物件缺少 item_id")
    elif items and item not in items:
        report.error(where, f"物品 id 不存在：{item}")
    if not is_plain_int(node.get("count")) or node["count"] < 1:
        report.error(where, f"count 必须是 ≥ 1 的整数，实际是 {node.get('count')!r}")
    if "herb_age" in node and (not is_plain_int(node["herb_age"]) or node["herb_age"] < 0):
        report.error(where, f"herb_age 必须是 ≥ 0 的整数，实际是 {node['herb_age']!r}")


def check_path_kind_fields(where, entry, kind, ctx, report) -> None:
    """三种行动各自那几个字段：打探的附带效果、求购的货与价、切磋的编成与奖励。"""
    if kind == "inquire":
        reveals = entry.get("reveal", [])
        for index, reveal in enumerate(reveals if isinstance(reveals, list) else [None]):
            here = f"{where} 的 reveal[{index}]"
            if not isinstance(reveal, dict) or set(reveal) != {"role", "category"}:
                report.error(here, "只认 {role, category} 两个字段")
                continue
            if reveal["role"] not in ctx["roles"]:
                report.error(here, f"reveal 的角色不存在：{reveal['role']}")
            if reveal["category"] not in PATH_WEAKNESS_CATEGORIES:
                report.error(here, f"category {reveal['category']!r} 不是十种攻击类别之一")
            ctx["reveals"].append((here, reveal["role"], reveal["category"]))
        gives = entry.get("give", [])
        for index, give in enumerate(gives if isinstance(gives, list) else [None]):
            check_path_bag(f"{where} 的 give[{index}]", give, ctx["items"], report)
        return
    if kind == "purchase":
        check_path_bag(where, {k: entry[k] for k in ("item_id", "count", "herb_age") if k in entry},
                       ctx["items"], report)
        if not is_plain_int(entry.get("price")) or entry["price"] < 1:
            report.error(where, f"求购的 price 必须是 ≥ 1 的整数（碎银，按块计），实际是 {entry.get('price')!r}")
        return
    battle_id = entry.get("battle")
    pending = entry.get("pending", False)
    if not isinstance(battle_id, str) or not battle_id:
        report.error(where, "切磋缺少 battle（编成 id）")
    elif not isinstance(pending, bool):
        report.error(where, "pending 必须是 true / false")
    elif pending:
        if battle_id in ctx["battles"]:
            report.error(where, f"编成 {battle_id} 已经建好了，去掉 pending：挂着它，这条切磋永远不会上屏")
        else:
            report.warn(where, f"切磋的编成 {battle_id} 尚未建好（pending）：等战斗路按横版格式补建，"
                               "规格见 docs/interfaces-octo-pathactions.md 第 6 节；第 5 章收尾前必须清零")
    elif battle_id not in ctx["battles"]:
        report.error(where, f"编成不存在：{battle_id}（还没建好就写 \"pending\": true）")
    else:
        setup = ctx["battles"][battle_id]
        if setup.get("defeat_is_fatal", True):
            report.error(where, f"切磋的编成 {battle_id} 输了会 game over：defeat_is_fatal 必须为 false")
        rewards = setup.get("rewards") or {}
        if rewards.get("cultivation", 0) or rewards.get("spirit_stones", 0) or rewards.get("drops"):
            report.error(where, f"切磋的编成 {battle_id} 自己带了 rewards：奖励只由条目发一次，编成的必须全 0")
    reward = entry.get("reward")
    if not isinstance(reward, dict) or set(reward) - {"cultivation", "items"}:
        report.error(where, "reward 必须是只含 cultivation / items 的对象")
        return
    cultivation = reward.get("cultivation", 0)
    if not is_plain_int(cultivation) or cultivation < 0:
        report.error(where, f"reward 的 cultivation 必须是 ≥ 0 的整数，实际是 {cultivation!r}")
    reward_items = reward.get("items", [])
    for index, item in enumerate(reward_items if isinstance(reward_items, list) else [None]):
        check_path_bag(f"{where} 的 reward.items[{index}]", item, ctx["items"], report)
    if not cultivation and not reward_items:
        report.error(where, "reward 里修为与物件至少给一样：赢了什么也没有，就不叫「赢了有奖励」")


def check_path_entry(rel, chapter, entry, ctx, report):
    """一条条目的形状与引用。合法到能推理的，返回规整后的记录给跨条目检查；否则 None。"""
    if not isinstance(entry, dict):
        report.error(rel, "条目必须是对象")
        return None
    entry_id = entry.get("id")
    where = f"{rel} 的 {entry_id or '(无 id)'}"
    kind = entry.get("kind")
    if kind not in PATH_KIND_FIELDS:
        report.error(where, f"kind 只认 inquire / purchase / challenge，实际是 {kind!r}")
        return None
    unknown = sorted(set(entry) - PATH_COMMON_FIELDS - PATH_KIND_FIELDS[kind])
    if unknown:
        report.error(where, f"有 {kind} 不认识的字段 {'、'.join(unknown)}"
                            "（字段表见 docs/interfaces-octo-pathactions.md 第 2 节）")
    match = PATH_ID_RE.match(entry_id) if isinstance(entry_id, str) else None
    if not match or match.group(1) != PATH_KIND_WORD[kind]:
        report.error(where, f"id 的形状是 <npc 短名>_{PATH_KIND_WORD[kind]}[序号]")
    tag = "ch%02d" % chapter
    if entry.get("done_flag") != f"{tag}.path.{entry_id}":
        report.error(where, f"done_flag 必须是 {tag}.path.{entry_id}")
    elif entry["done_flag"] not in ctx["flags"]:
        report.error(where, f"旗标未在 data/flags.json 登记：{entry['done_flag']}")

    realm = entry.get("realm", 0)
    if not is_plain_int(realm) or realm not in PATH_REALMS:
        report.error(where, f"realm 必须是合法的境界编号，实际是 {realm!r}")
        realm = 0
    if realm and not entry.get("refuse_key"):
        report.error(where, "有阅历门槛（realm > 0）就必须写 refuse_key")
    if not realm and "refuse_key" in entry:
        report.error(where, "没有阅历门槛却写了 refuse_key：这句话永远说不出口")
    for key_field in PATH_KIND_KEYS[kind] + (("refuse_key",) if realm else ()):
        value = entry.get(key_field)
        if not isinstance(value, str) or not value:
            report.error(where, f"缺少 {key_field}")
        elif ctx["texts"] and value not in ctx["texts"]:
            report.error(where, f"文案 key 不存在：{value}")

    parsed = ctx["maps"].get(str(entry.get("map", "")))
    npc = None
    if parsed is None:
        report.error(where, f"map 不存在：{entry.get('map')!r}")
    else:
        npc = next((o for o in parsed.objects if o["name"] == entry.get("npc")), None)
        if npc is None or npc["type"] != "npc":
            report.error(where, f"{parsed.map_id} 上没有名为 {entry.get('npc')!r} 的 npc 对象")
            npc = None

    lists = {}
    for field in ("when", "until"):
        conds = check_quest_condition_list(where, entry, field, True, ctx["flags"], ctx["settable"],
                                           ctx["items"], report)
        lists[field] = [c for c in (path_condition(cond) for cond in conds) if c is not None]
    anchored = any(path_requires_nonzero(c) and (c[1].startswith(tag + ".") or
                                                 (chapter > 1 and c[1] == "ch%02d.done" % (chapter - 1)))
                   for c in lists["when"])
    if not anchored:
        report.error(where, f"when 没有锚在本章：至少要有一条「{tag}.* 已置」或「上一章 done」")
    if ("flag", f"{tag}.done", ">=", 1) not in lists["until"]:
        report.error(where, f"until 里必须有 {tag}.done >= 1：本章收尾时一律收起")

    check_path_kind_fields(where, entry, kind, ctx, report)
    seen_set_flags = set()
    for flag in entry.get("set_flags", []) if isinstance(entry.get("set_flags", []), list) else [None]:
        if not isinstance(flag, str) or not flag.startswith(tag + ".path.") or not PATH_FLAG_RE.match(flag):
            report.error(where, f"set_flags 里只许写本章的路径行动旗标（{tag}.path.…），实际是 {flag!r}")
        elif flag in seen_set_flags:
            report.error(where, f"set_flags 里的 {flag} 与前一项重复")
        elif flag not in ctx["flags"]:
            report.error(where, f"旗标未在 data/flags.json 登记：{flag}")
        elif flag in ctx["done_flags"]:
            report.error(where, f"set_flags 里的 {flag} 是别的条目的 done_flag：问一句话就把那一条标成做过了")
        elif flag not in ctx["read_flags"]:
            report.error(where, f"set_flags 里的 {flag} 没有任何条目的 when / until 读它：置了也白置")
        if isinstance(flag, str):
            seen_set_flags.add(flag)
    # 谓词有写坏的、或者 when / until 是空的：上面已经报过了，不拿它去推时段（推出来的只会是噪音）。
    if not lists["when"] or not lists["until"] or len(lists["when"]) != len(entry.get("when") or []) \
            or len(lists["until"]) != len(entry.get("until") or []):
        return None
    record = {"where": where, "id": entry_id, "chapter": chapter, "kind": kind, "map": entry.get("map"),
            "npc": entry.get("npc"), "npc_record": npc, "when": lists["when"], "until": lists["until"],
            "battle": entry.get("battle"), "pending": entry.get("pending") is True}
    if not path_window_can_open(record, ctx["settable"]):
        report.error(where, "when / until 的窗口永远无法开启：完成旗与附带旗不能提供来源")
    return record


def check_path_presence(record, report) -> None:
    """条目的时段要落在 NPC 在场的时段里：地图上写了 visible_flag，when 就得要求它已置；
    写了 hidden_flag，until 就得在它置上时成立。不然条目挂着、人却不在，或者人在、条目挂在他不在的那段。"""
    npc = record["npc_record"]
    if npc is None:
        return
    shown = npc["props"].get("visible_flag", "")
    if shown and not any(path_implies(c, ("flag", shown, ">=", 1)) for c in record["when"]):
        report.error(record["where"], f"{npc['name']} 要 {shown} 置上才在场，when 里得写上它（或更晚的旗标）")
    hidden = npc["props"].get("hidden_flag", "")
    if hidden and not any(path_implies(("flag", hidden, ">=", 1), u) for u in record["until"]):
        report.error(record["where"], f"{npc['name']} 在 {hidden} 置上后下场，until 里得写上它（或更早收起）")


def check_path_actions(maps: list[ParsedMap], flags: set[str], texts: dict[str, str], defined: dict,
                       report: Report) -> set[str]:
    """规则 26：路径行动的形状、引用与时段；返回合法且非 pending 条目的置旗来源。"""
    actions_dir = ROOT / "data" / "pathactions"
    files = sorted(actions_dir.glob("*.json")) if actions_dir.exists() else []
    payloads = []
    for path in files:
        payload = read_json(path, report)
        if isinstance(payload, dict) and isinstance(payload.get("entries"), list):
            payloads.append((path, payload))
        elif payload is not None:
            report.error(str(path.relative_to(ROOT)), "路径行动文件顶层必须是对象，且 entries 是数组")

    # 第一趟只收账：谁置哪些 chNN.path.*、谁读哪些旗标——谓词里的旗标「有人会置」要算上它们。
    done_flags, set_flags, read_flags = set(), set(), set()
    for _path, payload in payloads:
        for entry in payload["entries"]:
            if not isinstance(entry, dict):
                continue
            if isinstance(entry.get("done_flag"), str):
                done_flags.add(entry["done_flag"])
            set_flags.update(f for f in entry.get("set_flags", []) if isinstance(f, str))
            for field in ("when", "until"):
                read_flags.update(c.get("flag") for c in entry.get(field, []) if isinstance(c, dict))
    settable = set(done_flags) | set_flags
    for found in script_flag_sets().values():
        settable |= found
    ctx = {"maps": {m.map_id: m for m in maps}, "flags": flags, "texts": texts, "settable": settable,
           "items": defined.get("item", set()), "roles": defined.get("role", set()),
           "battles": load_battle_payloads(report), "done_flags": done_flags, "read_flags": read_flags,
           "reveals": []}

    records, chapters_seen = [], {}
    producers = []
    for path, payload in payloads:
        rel = str(path.relative_to(ROOT))
        file_errors = len(report.errors)
        unknown = sorted(set(payload) - PATH_TOP_FIELDS)
        if unknown:
            report.error(rel, f"有不认识的字段 {'、'.join(unknown)}（只认 id / name / chapter / entries / note）")
        chapter = payload.get("chapter")
        stem = PATH_FILE_RE.match(path.stem)
        if not is_plain_int(chapter) or not (1 <= chapter <= 14) or not stem or int(stem.group(1)) != chapter:
            report.error(rel, f"一章一个文件：文件名必须是 chNN.json、chapter 与 NN 相同，实际 chapter={chapter!r}")
            continue
        if payload.get("id") != "pathactions_ch%02d" % chapter:
            report.error(rel, "顶层 id 必须是 pathactions_ch%02d" % chapter)
        if chapter in chapters_seen:
            report.error(rel, f"与 {chapters_seen[chapter]} 同属第 {chapter} 章")
        chapters_seen[chapter] = rel
        if not payload["entries"]:
            report.error(rel, "entries 不许为空")
        file_valid = len(report.errors) == file_errors
        ids = set()
        for entry in payload["entries"]:
            entry_errors = len(report.errors)
            record = check_path_entry(rel, chapter, entry, ctx, report)
            if isinstance(entry, dict) and entry.get("id") in ids:
                report.error(rel, f"条目 id 在本章内重复：{entry.get('id')}")
            ids.add(entry.get("id") if isinstance(entry, dict) else None)
            if record is not None:
                check_path_presence(record, report)
                records.append(record)
                if file_valid and len(report.errors) == entry_errors and not record["pending"]:
                    producers.append((entry, record))

    check_path_cross(records, set_flags, ctx, report)
    check_path_axiom_premise(report)
    # 只从真实来源起步：pending / 非法条目及无起点的路径旗标环不能间接提供任务来源。
    sources: set[str] = set()
    available = set().union(*script_flag_sets().values())
    while True:
        previous = set(sources)
        for entry, record in producers:
            if path_window_can_open(record, available):
                sources.add(entry["done_flag"])
                sources.update(entry.get("set_flags", []))
        if sources == previous:
            break
        available |= sources
    return sources


def check_path_cross(records, set_flags, ctx, report) -> None:
    """跨条目的几条：时段不重叠、落在 NPC 在场的时段里、揭破绽的角色会上阵、登记表里没有孤儿、
    剧情脚本不碰路径行动的旗标。"""
    for index, a in enumerate(records):
        for b in records[index + 1:]:
            if (a["map"], a["npc"], a["kind"]) != (b["map"], b["npc"], b["kind"]):
                continue
            if not path_windows_disjoint(a, b):
                report.error(a["where"], f"与 {b['where']} 挂在同一个 NPC 上、同为 {a['kind']}，时段证明不了不重叠："
                                         "让后一条的 when 蕴含前一条的某个 until（或两边的 when 相斥）")

    # 揭破绽的角色得真的会上阵：出现在某场编成里，或是某条 pending 切磋的那个 NPC。
    fighting = set()
    for setup in ctx["battles"].values():
        fighting.update(str(u.get("role_id")) for u in setup.get("units", []) if isinstance(u, dict))
    for record in records:
        if record["kind"] == "challenge" and record["pending"] and record["npc_record"] is not None:
            fighting.add(record["npc_record"]["props"].get("role_id", ""))
    unchecked = set()
    for where, role, category in ctx["reveals"]:
        if role not in fighting:
            report.error(where, f"揭的是 {role} 的破绽，可它不在任何一场编成里：揭了也用不上")
            continue
        weaknesses = read_json(ROOT / "data" / "roles" / f"{role}.json", report) or {}
        if "weaknesses" not in weaknesses:
            unchecked.add(role)
        elif category not in weaknesses.get("weaknesses", []):
            report.error(where, f"{role} 的 weaknesses 里没有 {category}：打探揭开的得是它真有的破绽")
    if unchecked:
        report.warn("data/pathactions/", f"打探揭破绽的 {len(unchecked)} 个角色还没有 weaknesses 字段，"
                                         "揭的类别对不上账（等战斗路合回再核）：" + "、".join(sorted(unchecked)))

    for flag in sorted(f for f in ctx["flags"] if PATH_FLAG_RE.match(f)):
        if flag not in ctx["done_flags"] and flag not in set_flags:
            report.error("data/flags.json", f"{flag} 登记了，却不是任何路径行动的 done_flag 或 set_flags")
    for script, found in sorted(script_flag_sets().items()):
        for flag in sorted(f for f in found if PATH_FLAG_RE.match(f)):
            report.error(script, f"剧情脚本置了路径行动的旗标 {flag}：它只由路径行动置")


def check_path_axiom_premise(report) -> None:
    """path_implies 那条章节公理的前一半前提：脚本不置「以后的章」的旗标。

    scripts/chNN/ 下的脚本在第 NN 章里跑；它若置了 chMM.*（MM > NN），第 MM 章的旗标就在第 MM 章开始之前
    置上了——公理当场不成立，时段重叠检查会把本该报的重叠「证明」成不重叠，放过坏数据。
    不在任何一章目录里的脚本（common/ 之类）什么时候都可能跑，不许置带章号的旗标。
    置以前的章的旗标不碍公理（那一章早已开始），不在这里管。
    """
    for script, found in sorted(script_flag_sets().items()):
        own = re.match(r"^ch(\d\d)/", script)
        for flag in sorted(found):
            chapter = path_chapter_of(flag)
            if chapter is None:
                continue
            if own is None:
                report.error("scripts/" + script, f"置了带章号的旗标 {flag}，可它不在任何一章的目录里："
                                                  "什么时候都可能跑，路径行动的时段推理（章按顺序走）就站不住")
            elif chapter > int(own.group(1)):
                report.error("scripts/" + script, f"第 {int(own.group(1))} 章的脚本置了第 {chapter} 章的旗标 {flag}："
                                                  "那一章还没开始它就置上了，路径行动的时段推理（章按顺序走）就站不住")

# ---------------------------------------------------------------------------
# 规则 24、25：破势与蓄劲（docs/octopath-battle.md 第 5、7 节）
# ---------------------------------------------------------------------------
# 十种攻击类别，顺序即位序（src/core/model/AttackCategory.h 那张表的镜像）。
# 这里只查名字写没写对；名字换成位号是 C++ 加载器的事，两边各有一份是因为门禁
# 要在编译之前跑——它们对不上的那一天，加载器会在测试里当场报错。
WEAPON_CATEGORIES = ("剑", "刀", "拳", "暗器", "毒")
ELEMENT_CATEGORIES = ("金", "木", "水", "火", "土")
ALL_CATEGORIES = WEAPON_CATEGORIES + ELEMENT_CATEGORIES
# 五行位掩码（core::Element）→ 类别名。
ELEMENT_BITS = {1: "金", 2: "木", 4: "水", 8: "火", 16: "土"}
MIND_TERRAIN = "mind"

# 横版没有格子：这几个字段写在数据里已经没有任何东西会读，留着只会让人以为它们还管用
#（「开场两人相距 3 格，正好够得着」这种推算就是这么来的）。一律报错，逼人删掉。
OBSOLETE_BATTLE_FIELDS = ("width", "height", "player_spawn")
OBSOLETE_UNIT_FIELDS = ("x", "y")
OBSOLETE_MAGIC_FIELDS = ("castRange",)
OBSOLETE_ITEM_FIELDS = ("useRange",)
# 与 castMagic（道具施法）互斥的四样药效：写了其中任何一样（非 0 / true），这件东西就是药不是符。
CAST_MAGIC_EXCLUSIVE_FIELDS = ("restoreHp", "restoreMp", "poison", "curesPoison")
# 规则 25 的来路里「give 一件兵器」给得出的类别：拳是空手、毒核的是 poison，剩下这三样核 weapon 字段。
GIVEN_WEAPON_CATEGORIES = ("剑", "刀", "暗器")

# 规则 25 的「我方必有手段」表：第 1–5 章每一场仗，玩家**不靠买、不靠选**一定带着的攻击类别。
#
# 韩立：空手的「拳」永远有（BattleScene::heroWeapons）；背包里的兵器再加一样——但软剑要
# 第 3 章节点 7 花三十块碎银买，买不买得起看第 2 章章末那次选择，所以「剑」不算必有。
# 每一条都写明来路，并由 check_means_sources 去脚本与数据里核一遍：来路被删了，这里当场红。
#
#   (类别, 来路种类, 来路 id, 在哪一章的脚本里)
#   来路种类：hand = 空手；learn = magic.learn；give = give() 给的毒药或兵器（兵刃类核 weapon 字段）。
CHAPTER_MEANS = {
    3: [("拳", "hand", "", "")],
    4: [("拳", "hand", "", ""),
        # 火弹术：scripts/ch04/huodan.lua 节点 2 教会；第 4 章三场仗（切磋、坊门、攻防）都在节点 5 之后。
        ("火", "learn", "magic_huodan_shu", "ch04")],
    5: [("拳", "hand", "", ""),
        # 第 5 章不新增常驻法术（施工图 1.1 第 9 条之后那一行），火弹术从第 4 章带过来。
        ("火", "learn", "magic_huodan_shu", "ch04")],
    # 第 6 章整章只有拳与火：祭剑符（金）与流沙术（土）要到节点 9b 苦修（scripts/ch06/kuxiu.lua）才学会，
    # 而节点 6 的叶家寻衅在那之前——登成整章必有，规则 25 就会对那一仗放水。9b 之后的几场按场登在下面。
    6: [("拳", "hand", "", ""),
        ("火", "learn", "magic_huodan_shu", "ch04")],
    # 第 7 章（docs/ch07-design.md 8.2）：本章第一仗（节点 13 陆师兄）在节点 11b 万宝楼之后，
    # 那一节 scripts/ch07/wanbaolou.lua 学会祭金符（金）；流沙术（土）、冰冻术（水）第 6 章 9b 起常驻。
    # 祭剑符在 ch170（scripts/ch07/yeyu.lua）收回、祭金光砖在 ch209（jiaoshi.lua）收回，都不登——金、土另有常驻来路。
    7: [("拳", "hand", "", ""),
        ("火", "learn", "magic_huodan_shu", "ch04"),
        ("水", "learn", "magic_bingdong_shu", "ch06"),
        ("土", "learn", "magic_liusha_shu", "ch06"),
        ("金", "learn", "magic_ji_jinfu", "ch07")],
    # main f6e89e0 already-approved Chapter 8 table; Chapter 9 battles precede demotion.
    8: [("拳", "hand", "", ""),
        ("火", "learn", "magic_huodan_shu", "ch04"),
        ("土", "learn", "magic_liusha_shu", "ch06"),
        ("水", "learn", "magic_bingdong_shu", "ch06"),
        ("金", "learn", "magic_ji_jinfu", "ch07"),
        ("木", "learn", "magic_qingyuan_jianmang", "ch08"),
        ("暗器", "give", "weapon_wuming_sixian", "ch07")],
    9: [("拳", "hand", "", ""),
        ("火", "learn", "magic_huodan_shu", "ch04"),
        ("土", "learn", "magic_liusha_shu", "ch06"),
        ("水", "learn", "magic_bingdong_shu", "ch06"),
        ("金", "learn", "magic_ji_jinfu", "ch07"),
        ("木", "learn", "magic_qingyuan_jianmang", "ch08"),
        ("暗器", "give", "weapon_wuming_sixian", "ch07")],
    # Chapter 10 starts at QiRefining3; Qingyuan Jianmang is unavailable after demotion.
    10: [("拳", "hand", "", ""),
         ("火", "learn", "magic_huodan_shu", "ch04"),
         ("土", "learn", "magic_liusha_shu", "ch06"),
         ("水", "learn", "magic_bingdong_shu", "ch06"),
         ("金", "learn", "magic_ji_jinfu", "ch07"),
         ("木", "learn", "magic_ji_qingjiao", "ch07"),
         ("暗器", "give", "weapon_wuming_sixian", "ch07")],
}
# 某一场仗额外必有的：蚀心散在节点 7 备毒时由 scripts/ch03/beidu.lua 给出（两包起），
# 暗道那一仗是节点 8。谷外遇狼（节点 1）那会儿还没有，所以不进第 3 章的通表。
# 欧阳飞天那一仗：祭剑符（金）由 scripts/ch05/shangyue.lua 开战前 magic.learn 借给他。没练成剑符的人
# 不与他照面（第 5 章校对 MEDIUM-1 与复验 MEDIUM-B 的拍板：只有「金」要得了他的命，data/roles 的
# killable_by；没练成的人到了赏月亭只远远看清、悄悄退回林子练符，这一仗只打有符的那一次）——
# **开得了这一仗的那一趟手里必有这一招**，所以按这一场的必有手段登记。
BATTLE_EXTRA_MEANS = {
    "b03_andao_shishou": [("毒", "give", "pill_shixin_san", "ch03")],
    "b05_ouyang_feitian": [("金", "learn", "magic_ji_jianfu", "ch05")],
    # 第 6 章节点 9b 之后的三场（docs/ch06-design.md 8.2）：9b 起祭剑符常驻、流沙术学会，
    # 袭杀在节点 12、吴风切磋在节点 16b、外门切磋的路径行动 when 要 ch06.chuwudai（16a）——都在 9b 之后。
    # 吴九指那一场切磋的 when 是 ch06.yishi（节点 7），打得在 9b 之前，所以不登。
    "b06_shanqiu_xisha": [("金", "learn", "magic_ji_jianfu", "ch06"),
                          ("土", "learn", "magic_liusha_shu", "ch06")],
    "b06_wufeng_qiecuo": [("金", "learn", "magic_ji_jianfu", "ch06"),
                          ("土", "learn", "magic_liusha_shu", "ch06")],
    "b06_huangfenggu_qiecuo": [("金", "learn", "magic_ji_jianfu", "ch06"),
                               ("土", "learn", "magic_liusha_shu", "ch06")],
    # 第 7 章 ① 陆师兄之后才到手的两样（docs/ch07-design.md 8.2）：祭青蛟旗（木）在 yeyu.lua 陆师兄死后搜身学会；
    # 无名丝线（暗器）在 liangshi.lua 给出。这几场都在那两处之后。
    **{battle_id: [("木", "learn", "magic_ji_qingjiao", "ch07"),
                   ("暗器", "give", "weapon_wuming_sixian", "ch07")]
       for battle_id in ("b07_yixiantian", "b07_fengyue", "b07_zhongxinqu_duoyao", "b07_zhaoze_shouyao",
                         "be07_huoyan_shu", "be07_tiebi_yuan", "be07_tuishan_shou")},
    # Fourth slot is the actual ally role for role_magic/role_weapon sources.
    "b10_liuliandian_weisha": [("剑", "role_weapon", "剑", "qu_hun_huashen"),
                               ("火", "role_magic", "magic_xuelian_guangzhu", "qu_hun_huashen"),
                               ("火", "role_magic", "magic_ch10_chunyang", "yan_daoyou"),
                               ("水", "role_magic", "magic_ch10_shuiqi", "feng_sanniang"),
                               ("木", "role_magic", "magic_ch10_mufu", "qing_suanzi")],
    "b10_zhifadui": [("剑", "role_weapon", "剑", "qu_hun_shadan"),
                     ("火", "role_magic", "magic_xuelian_guangzhu", "qu_hun_shadan"),
                     ("火", "role_magic", "magic_xuelian_ziyan", "qu_hun_shadan"),
                     ("金", "role_magic", "magic_ji_jinjian", "qu_hun_shadan")],
}
MEANS_LAST_CHAPTER = 10


def check_category_list(where, payload, field, allowed, report) -> list[str]:
    """一列类别名：必须是数组、每项是 allowed 里的名字、不许重复。返回合法的那几项。"""
    value = payload.get(field)
    if not isinstance(value, list):
        report.error(where, f"{field} 必须是类别名的数组")
        return []
    good: list[str] = []
    for name in value:
        if not isinstance(name, str) or name not in allowed:
            report.error(where, f"{field} 里的 {name!r} 不是可用的类别（只许：{'、'.join(allowed)}）")
        elif name in good:
            report.error(where, f"{field} 里「{name}」写了两遍")
        else:
            good.append(name)
    return good


def load_break_roles(report) -> dict[str, dict]:
    """规则 24 的角色那一半：字段形状。返回 role_id → 读出来的那几样（给后面两条用）。"""
    roles: dict[str, dict] = {}
    roles_dir = ROOT / "data" / "roles"
    if not roles_dir.exists():
        return roles
    for path in sorted(roles_dir.rglob("*.json")):
        payload = read_json(path, report)
        if not isinstance(payload, dict) or not payload.get("id"):
            continue
        where = str(path.relative_to(ROOT))
        info = {"toughness": 0, "weaknesses": [], "weapons": ["拳"],
                "realm": payload.get("realm", "Mortal"), "magics": payload.get("magics", []),
                "maxMp": payload.get("maxMp", 0)}
        if "weapons" in payload:
            info["weapons"] = check_category_list(where, payload, "weapons", WEAPON_CATEGORIES, report)
            if isinstance(payload["weapons"], list) and not payload["weapons"]:
                report.error(where, "weapons 不能是空数组（空手就不写，缺省是拳）")
        if "weaknesses" in payload:
            info["weaknesses"] = check_category_list(where, payload, "weaknesses", ALL_CATEGORIES, report)
        if "toughness" in payload:
            toughness = payload["toughness"]
            if not is_plain_int(toughness) or toughness < 1:
                report.error(where, f"toughness 必须是 ≥ 1 的整数（没有架势就不写），实际是 {toughness!r}")
            else:
                info["toughness"] = toughness
        if info["toughness"] > 0 and not info["weaknesses"]:
            report.error(where, "有架势却一样破绽都没有：永远破不了势")
        if "actions" in payload:
            actions = payload["actions"]
            if not is_plain_int(actions) or actions < 1:
                report.error(where, f"actions 必须是 ≥ 1 的整数，实际是 {actions!r}")
        if "charge" in payload:
            charge = payload["charge"]
            if not isinstance(charge, dict):
                report.error(where, "charge 必须是对象")
            else:
                every = charge.get("every")
                if not is_plain_int(every) or every < 2:
                    report.error(where, f"charge.every 必须是 ≥ 2 的整数，实际是 {every!r}")
                mult = charge.get("mult")
                if isinstance(mult, bool) or not isinstance(mult, (int, float)) or mult <= 0:
                    report.error(where, f"charge.mult 必须是正数，实际是 {mult!r}")
                if "all" in charge and not isinstance(charge["all"], bool):
                    report.error(where, "charge.all 必须是 true / false")
                if not isinstance(charge.get("text_key"), str) or not charge.get("text_key"):
                    report.error(where, "charge.text_key 必须写：宣告蓄势时玩家要看见一句预告")
        if "killable_by" in payload:
            # 只有这几类要得了他的命（欧阳飞天：祭剑符的金）。空数组 = 打不死，那一场永远赢不下来。
            check_category_list(where, payload, "killable_by", ALL_CATEGORIES, report)
            if isinstance(payload["killable_by"], list) and not payload["killable_by"]:
                report.error(where, "killable_by 不能是空数组（谁都杀得死就不写）")
        roles[str(payload["id"])] = info
    return roles


def script_sources(chapter_dir: str) -> str:
    """scripts/<chapter_dir>/ 下全部脚本去掉注释后连在一起（给手段来路的核对用）。"""
    base = ROOT / "scripts" / chapter_dir
    if not base.exists():
        return ""
    chunks = []
    for path in sorted(base.rglob("*.lua")):
        try:
            chunks.append(strip_lua_comments(path.read_text(encoding="utf-8")))
        except (OSError, UnicodeDecodeError):
            continue
    return "\n".join(chunks)


def role_means_available(entry: tuple, battle: dict, roles: dict, magics: dict) -> bool:
    category, kind, source, role_id = entry
    role = roles.get(role_id)
    if not isinstance(role, dict) or not any(
            isinstance(unit, dict) and unit.get("role_id") == role_id and unit.get("faction") == "ally"
            for unit in battle.get("units", [])):
        return False
    if kind == "role_weapon":
        return source == category and category in WEAPON_CATEGORIES and source in role.get("weapons", ["拳"])
    magic = magics.get(source)
    known = role.get("magics", [])
    if kind != "role_magic" or not isinstance(magic, dict) or not isinstance(known, list) or source not in known:
        return False
    power, poison = magic.get("power", 10), magic.get("poison", 0)
    offensive = (is_plain_int(power) and power > 0) or (is_plain_int(poison) and poison > 0)
    element = magic.get("element", 0)
    names = {name for bit, name in ELEMENT_BITS.items() if is_plain_int(element) and element & bit}
    realm, required = role.get("realm", "Mortal"), magic.get("needRealm", "QiRefining1")
    # BattleScene starts this ally at role.maxMp; checkCast rejects an unaffordable needMp.
    cost, capacity = magic.get("needMp", 5), role.get("maxMp", 0)
    can_pay = is_plain_int(cost) and is_plain_int(capacity) and cost <= capacity
    return (category in names and offensive and "effect" not in magic and
            can_pay and realm in REALM_ORDER and required in REALM_ORDER and
            REALM_ORDER.index(realm) >= REALM_ORDER.index(required))


def check_means_sources(report, roles: dict | None = None) -> None:
    """规则 25 的那张表自己得站得住：每一条来路都真的在脚本与数据里。

    表是人写的，而人写的表会漂：哪天火弹术挪到第 5 章才教，这张表还写着第 4 章必有火，
    规则 25 就会放过一整章打不出破绽的敌人。所以逐条去核：learn 的那门法术存在、
    它的五行真是这一类、那一章的脚本里真有 magic.learn；give 的那件东西存在、真的带毒
    （兵刃类：weapon 字段真是那一类）、那一章的脚本里真有 give。"""
    magics: dict[str, dict] = {}
    for path in sorted((ROOT / "data" / "magics").rglob("*.json")):
        payload = read_json(path, report)
        if isinstance(payload, dict) and payload.get("id"):
            magics[str(payload["id"])] = payload
    items: dict[str, dict] = {}
    for path in sorted((ROOT / "data" / "items").rglob("*.json")):
        payload = read_json(path, report)
        if isinstance(payload, dict) and payload.get("id"):
            items[str(payload["id"])] = payload

    entries = [(f"第 {ch} 章", e) for ch, lst in CHAPTER_MEANS.items() for e in lst]
    entries += [(battle_id, e) for battle_id, lst in BATTLE_EXTRA_MEANS.items() for e in lst]
    where = "tools/validate.py 的我方手段表"
    if roles is None:
        roles = load_break_roles(report)
    battles = load_battle_payloads(report)
    for owner, (category, kind, source, chapter_dir) in entries:
        if kind in ("role_magic", "role_weapon"):
            battle = battles.get(owner, {})
            if not role_means_available((category, kind, source, chapter_dir), battle, roles, magics):
                report.error(where, f"{owner}：友军 {chapter_dir} 的「{category}」来路 {source}"
                                    " 不在编成或不可施展，不是我方必有的手段")
            continue
        if kind == "hand":
            if category != "拳":
                report.error(where, f"{owner}：空手只能是「拳」，写成了「{category}」")
            continue
        code = script_sources(chapter_dir)
        if kind == "learn":
            magic = magics.get(source)
            if magic is None:
                report.error(where, f"{owner}：「{category}」的来路 {source} 在 data/magics 里不存在")
                continue
            element = magic.get("element", 0)
            names = {name for bit, name in ELEMENT_BITS.items() if isinstance(element, int) and element & bit}
            if category not in names:
                report.error(where, f"{owner}：{source} 的五行不是「{category}」，它打不出这一类")
            if owner == "第 10 章":
                required = magic.get("needRealm", "QiRefining1")
                if required not in REALM_ORDER or REALM_ORDER.index(required) > REALM_ORDER.index("QiRefining3"):
                    report.error(where, f"{owner}：{source} 跌落到炼气三层后不可施展，不是整章必有手段")
            if not re.search(r'magic\.learn\s*\(\s*["\']' + re.escape(source) + r'["\']', code):
                report.error(where, f"{owner}：scripts/{chapter_dir}/ 里没有 magic.learn(\"{source}\")，"
                                    f"「{category}」就不是玩家必有的手段")
        elif kind == "give":
            item = items.get(source)
            if item is None:
                report.error(where, f"{owner}：「{category}」的来路 {source} 在 data/items 里不存在")
                continue
            if category == "毒" and not (is_plain_int(item.get("poison")) and item.get("poison", 0) > 0):
                report.error(where, f"{owner}：{source} 不带毒，撒出去不算「毒」类一击")
            # 兵刃类的来路（docs/interfaces-p3-ch07.md 5.1 第 4 条）：give 的那件得真是这一类兵器——
            # 揣在身上普攻才多出这一类（BattleScene::heroWeapons 读的就是 weapon 字段）。
            if category in GIVEN_WEAPON_CATEGORIES and item.get("weapon") != category:
                report.error(where, f"{owner}：{source} 的 weapon 不是「{category}」（实际是 {item.get('weapon')!r}），"
                                    f"揣在身上也打不出这一类")
            if not re.search(r'give\s*\(\s*["\']' + re.escape(source) + r'["\']', code):
                report.error(where, f"{owner}：scripts/{chapter_dir}/ 里没有 give(\"{source}\")，"
                                    f"「{category}」就不是玩家必有的手段")
        else:
            report.error(where, f"{owner}：认不出的来路种类 {kind!r}")


def battle_means(battle: dict, roles: dict[str, dict], magics: dict | None = None) -> set[str] | None:
    """一场仗我方必有的攻击类别；这一章没登记时返回 None。"""
    allies = [u for u in battle.get("units", []) if isinstance(u, dict) and u.get("faction") == "ally"]
    ally_weapons = set()
    for unit in allies:
        ally_weapons |= set(roles.get(str(unit.get("role_id")), {}).get("weapons", ["拳"]))
    if battle.get("hero_absent") is True:
        # Hero-absent spells must belong to the actual allies; never inherit Han Li's chapter table.
        extras = {entry[0] for entry in BATTLE_EXTRA_MEANS.get(str(battle.get("id")), [])
                  if role_means_available(entry, battle, roles, magics or {})}
        return ally_weapons | extras
    chapter = battle.get("chapter")
    if chapter not in CHAPTER_MEANS:
        return None
    means = {category for category, *_ in CHAPTER_MEANS[chapter]}
    means |= {category for category, *_ in BATTLE_EXTRA_MEANS.get(str(battle.get("id")), [])}
    return means | ally_weapons


def check_magic_target(where: str, payload: dict, report: Report) -> None:
    """Chapter 10 E3: optional single/all damage scope, also used by the focused entry."""
    if "target" not in payload:
        return
    target = payload["target"]
    if not isinstance(target, str) or target not in ("single", "all"):
        report.error(where, f'target 只许 "single" 或 "all"，实际是 {target!r}')
        return
    if target == "all":
        power, poison = payload.get("power", 10), payload.get("poison", 0)
        offensive = (is_plain_int(power) and power > 0) or (is_plain_int(poison) and poison > 0)
        if "effect" in payload or not offensive:
            report.error(where, 'target "all" 只许用于伤人法术，不与 reveal / stagger 同写')


def check_magic_target_data(report: Report) -> None:
    for path in sorted((ROOT / "data" / "magics").rglob("*.json")):
        payload = read_json(path, report)
        if isinstance(payload, dict):
            check_magic_target(str(path.relative_to(ROOT)), payload, report)
        elif payload is not None:
            report.error(str(path.relative_to(ROOT)), "法术必须是对象")


def check_battle_break_data(report) -> None:
    """规则 24（形状，全仓）与规则 25（第 1–5 章：每个敌人至少一样破绽打得到）。

    24：角色的 weapons / weaknesses / toughness / actions / charge 写了就得写对；法术的 boost
        只许 hits / power，effect 只许 reveal / stagger 且不与 power / poison 同写，stagger 只许 1–9
        且只与 effect stagger 同写；物品的 castMagic 不与四样药效同写；兵器的 weapon 只许兵刃五类；格子时代的遗留字段（战场宽高、
        player_spawn、单位坐标、castRange、useRange）一律报错；上场的敌人（识海除外）
        必须有架势与破绽。
    25：第 1–5 章每一场仗（识海除外）的每一个敌人，破绽里至少有一样是那一场我方**必有**
        的手段打得到的（CHAPTER_MEANS / BATTLE_EXTRA_MEANS，外加编成里写死的友军的兵刃）。
        打不到的敌人永远破不了势——那一场就只剩「砍」这一种玩法，而玩家看着一排「？」
        不知道自己漏了什么。"""
    roles = load_break_roles(report)

    magics = {}
    for path in sorted((ROOT / "data" / "magics").rglob("*.json")):
        payload = read_json(path, report)
        if not isinstance(payload, dict):
            continue
        where = str(path.relative_to(ROOT))
        if isinstance(payload.get("id"), str):
            magics[payload["id"]] = payload
        check_magic_target(where, payload, report)
        for field_name in OBSOLETE_MAGIC_FIELDS:
            if field_name in payload:
                report.error(where, f"{field_name} 已作废：横版战斗没有距离，删掉这一项")
        if "boost" in payload and payload["boost"] not in ("hits", "power"):
            report.error(where, f"boost 只许 \"hits\" 或 \"power\"，实际是 {payload['boost']!r}")
        # effect（契约 docs/interfaces-p3-ch06.md 第 1、5 节，docs/interfaces-p3-ch07.md 5.1 第 1 条）：
        # 写了就得是 "reveal"（天眼术的看破）或 "stagger"（削架势），且不许与 power > 0 / poison > 0
        # 同写——一门法术要么伤人、要么看破或削架势。加载器（DataLoader.cpp）查同一条；power 缺省
        # 是 10，所以带效果的法术得明写 "power": 0。
        if "effect" in payload:
            if payload["effect"] not in ("reveal", "stagger"):
                report.error(where, f"effect 只许 \"reveal\" 或 \"stagger\"，实际是 {payload['effect']!r}")
            else:
                power = payload.get("power", 10)
                poison = payload.get("poison", 0)
                if (isinstance(power, int) and power > 0) or (isinstance(poison, int) and poison > 0):
                    report.error(where, f"effect 不能与 power > 0 或 poison > 0 同写（power {power!r}，"
                                        f"poison {poison!r}）：一门法术要么伤人、要么看破或削架势")
        # stagger（削几点架势）：1–9 的整数，只许与 effect "stagger" 同写（不写取 1）。写在别的法术上
        # 是一个谁也不读的数；写成 "3"、0、10 都是数据错。
        if "stagger" in payload:
            if payload.get("effect") != "stagger":
                report.error(where, "stagger 只许与 effect \"stagger\" 同写")
            elif not is_plain_int(payload["stagger"]) or not 1 <= payload["stagger"] <= 9:
                report.error(where, f"stagger 必须是 1 到 9 的整数，实际是 {payload['stagger']!r}")

    for path in sorted((ROOT / "data" / "items").rglob("*.json")):
        payload = read_json(path, report)
        if not isinstance(payload, dict):
            continue
        where = str(path.relative_to(ROOT))
        for field_name in OBSOLETE_ITEM_FIELDS:
            if field_name in payload:
                report.error(where, f"{field_name} 已作废：横版战斗没有距离，删掉这一项")
        if "weapon" in payload and payload["weapon"] not in WEAPON_CATEGORIES:
            report.error(where, f"weapon 只许兵刃类别（{'、'.join(WEAPON_CATEGORIES)}），实际是 {payload['weapon']!r}")
        # castMagic（道具施法，docs/interfaces-p3-ch07.md 5.1 第 2 条）：一件东西要么是药、要么是符，
        # 与四样药效同写报错（加载器查同一条）。它指向的法术存不存在由 REFERENCE_FIELDS 那一条查。
        if "castMagic" in payload:
            potions = [name for name in CAST_MAGIC_EXCLUSIVE_FIELDS
                       if payload.get(name) is True or (is_plain_int(payload.get(name)) and payload[name] > 0)]
            if potions:
                report.error(where, f"castMagic 不能与 {'、'.join(potions)} 同写：一件东西要么是药、要么是符")

    check_means_sources(report, roles)

    for path in sorted((ROOT / "data" / "battles").rglob("*.json")):
        battle = read_json(path, report)
        if not isinstance(battle, dict):
            continue
        where = str(path.relative_to(ROOT))
        for field_name in OBSOLETE_BATTLE_FIELDS:
            if field_name in battle:
                report.error(where, f"{field_name} 已作废：横版战斗没有格子，删掉这一项")
        if "backdrop" in battle and not isinstance(battle["backdrop"], str):
            report.error(where, "backdrop 必须是字符串")
        mind = battle.get("terrain") == MIND_TERRAIN
        chapter = battle.get("chapter")
        means = battle_means(battle, roles, magics)
        for index, unit in enumerate(battle.get("units", [])):
            if not isinstance(unit, dict):
                continue
            for field_name in OBSOLETE_UNIT_FIELDS:
                if field_name in unit:
                    report.error(where, f"units[{index}] 的 {field_name} 已作废：横版战斗没有格子，删掉这一项")
            if unit.get("faction") == "ally" or mind:
                continue
            role_id = str(unit.get("role_id", ""))
            info = roles.get(role_id)
            if info is None:
                continue   # 悬空引用由 check_data_references 报
            if info["toughness"] < 1 or not info["weaknesses"]:
                report.error(where, f"敌人 {role_id} 没有架势或破绽（data/roles/{role_id}.json 要写 toughness 与 weaknesses）")
                continue
            if not (is_plain_int(chapter) and chapter <= MEANS_LAST_CHAPTER):
                continue
            if means is None:
                report.error(where, f"第 {chapter} 章没有登记我方必有手段（tools/validate.py 的 CHAPTER_MEANS）")
                continue
            if not set(info["weaknesses"]) & means:
                report.error(where, f"敌人 {role_id} 的破绽（{'、'.join(info['weaknesses'])}）没有一样是这一场"
                                    f"我方必有的手段（{'、'.join(sorted(means, key=ALL_CATEGORIES.index))}）打得到的")


# ---------------------------------------------------------------------------
# 规则 27：野外遭遇（docs/interfaces-octo-encounters.md）
# ---------------------------------------------------------------------------

# 境界名，按 src/core/rules/Realm.h 的次序（C++ 那边的对照表在 io/DataLoader.cpp 的 parseRealmName）。
REALM_ORDER = (
    ["Mortal"] + ["QiRefining%d" % n for n in range(1, 14)]
    + ["FoundationEarly", "FoundationMid", "FoundationLate", "CoreEarly", "CoreMid", "CoreLate"]
)
# 第 1–2 章不放遭遇：韩立还是孩子（施工图 5.2）。
ENCOUNTER_FIRST_CHAPTER = 3
# 遭遇编成的奖励上限（施工图 5.2「只给少量碎银与药材」）：修为、碎银各不过这个数，掉落只许灵草、每样至多一件。
ENCOUNTER_MAX_CULTIVATION = 5
ENCOUNTER_MAX_SILVER = 5
# 不许进任何野外遭遇表的角色，及其理由。
ENCOUNTER_FORBIDDEN_ROLES = {
    "mofu_shigui": "墨府尸傀是封在墨府地窖里的东西，野地里撞不上（第 5 章设计 8.3）",
    "dubashanzhuang_zhuangding": "刺探那几天山庄「一点波澜也没起」（第 5 章校对 8.4），庄丁不许当野怪",
    "dubashanzhuang_xuntou": "同上：巡庄的人不许当野怪（第 5 章校对 8.4）",
}
# 遭遇编成的 id 前缀：与剧情战分得开，「同章剧情战」才有一张干净的名单可比。
ENCOUNTER_BATTLE_PREFIX = "be"


def battle_menace(battle: dict, roles: dict[str, dict]) -> int:
    """一场仗的「凶」：敌方每个单位 气血 × 攻击 之和（与 C++ 的 game::battleMenace 同一个算法）。"""
    total = 0
    for unit in battle.get("units", []):
        if not isinstance(unit, dict) or unit.get("faction") != "enemy":
            continue
        role = roles.get(str(unit.get("role_id")))
        if role is None:
            continue
        hp, attack = role.get("maxHp", 0), role.get("attack", 0)
        if is_plain_int(hp) and is_plain_int(attack):
            total += hp * attack
    return total


def check_encounter_table(rel: str, table: dict, battles: dict[str, dict], report: Report) -> list[str]:
    """一张遭遇表自己的形状；返回它引用的（存在的）编成 id。"""
    entries = table.get("entries")
    if not isinstance(entries, list) or not entries:
        report.error(rel, "entries 必须是非空数组")
        return []
    used = []
    for index, entry in enumerate(entries):
        where = f"{rel} entries[{index}]"
        if not isinstance(entry, dict):
            report.error(where, "不是对象")
            continue
        battle_id = str(entry.get("battleId", ""))
        if battle_id not in battles:
            report.error(where, f"battleId 在 data/battles 里不存在：{battle_id}")
        else:
            used.append(battle_id)
        weight = entry.get("weight", 1)
        if not (is_plain_int(weight) and weight >= 1):
            report.error(where, f"weight 必须是不小于 1 的整数，实际是 {weight!r}")
        low, high = entry.get("minRealm", "Mortal"), entry.get("maxRealm", "CoreLate")
        if low not in REALM_ORDER or high not in REALM_ORDER:
            report.error(where, f"minRealm / maxRealm 不是认得出的境界名：{low!r} / {high!r}")
        elif REALM_ORDER.index(low) > REALM_ORDER.index(high):
            report.error(where, f"minRealm（{low}）高于 maxRealm（{high}）：哪个境界都遇不上")
    return used


def check_encounter_battle(battle_id: str, battle: dict, roles: dict[str, dict], herbs: set[str],
                           story_floor: dict, report: Report) -> None:
    """地图上真用着的那几张表里的编成：输了不死、许逃、奖励少、只有野怪、比同章剧情战轻。"""
    where = f"data/battles/{battle_id}.json"
    if not battle_id.startswith(ENCOUNTER_BATTLE_PREFIX):
        report.error(where, f"野外遭遇的编成 id 要以 {ENCOUNTER_BATTLE_PREFIX} 打头（与剧情战分开）")
    if battle.get("defeat_is_fatal") is not False:
        report.error(where, "野外遭遇输了不许 game over：defeat_is_fatal 必须写 false")
    if battle.get("can_escape") is not True:
        report.error(where, "野外遭遇必须许逃：can_escape 必须写 true")
    for unit in battle.get("units", []):
        if not isinstance(unit, dict):
            continue
        if unit.get("faction") != "enemy":
            report.error(where, "野外遭遇的编成里只许写敌方：我方就是韩立与他当时的队伍")
            continue
        reason = ENCOUNTER_FORBIDDEN_ROLES.get(str(unit.get("role_id")))
        if reason:
            report.error(where, f"{unit.get('role_id')} 不许进野外遭遇：{reason}")
    rewards = battle.get("rewards") if isinstance(battle.get("rewards"), dict) else {}
    if rewards.get("cultivation", 0) > ENCOUNTER_MAX_CULTIVATION:
        report.error(where, f"野外遭遇的修为至多 {ENCOUNTER_MAX_CULTIVATION}，写了 {rewards.get('cultivation')}")
    if rewards.get("spirit_stones", 0) > ENCOUNTER_MAX_SILVER:
        report.error(where, f"野外遭遇的碎银至多 {ENCOUNTER_MAX_SILVER} 块，写了 {rewards.get('spirit_stones')}")
    for drop in rewards.get("drops", []):
        if not isinstance(drop, dict):
            continue
        if drop.get("item_id") not in herbs or drop.get("count", 1) > 1:
            report.error(where, f"野外遭遇的掉落只许灵草、每样至多一件：{drop.get('item_id')} × {drop.get('count', 1)}")
    floor = story_floor.get(battle.get("chapter"))
    menace = battle_menace(battle, roles)
    if floor is not None and menace >= floor[0]:
        report.error(where, f"野外遭遇要比同章剧情战轻：凶 {menace}，本章最轻的剧情战 {floor[1]} 才 {floor[0]}")


def check_encounters(maps: list[ParsedMap], report: Report) -> None:
    """规则 27：野外遭遇的表、区、编成（docs/interfaces-octo-encounters.md 第 2、3、5 节）。

    表：形状对（entries 非空、编成存在、权重 ≥ 1、境界名认得出且下不高于上）。
    区（地图上的 encounter 对象）：table_id 指得到表；只许第 3 章起的图；steps_min ≥ 1、
    daily_cap 写了就是正整数。
    编成（地图上真用着的表里的）：输了不死、许逃、只有敌方、没有不许当野怪的角色、奖励少、
    比同章剧情战（不以 be 打头、不是识海）里最轻的那一场还轻。

    没被任何一张图用上的表只查形状：它若引用剧情战，哪天有人把它挂上图，编成那一截就会当场报出来。
    （早年那三张 mountain_road / secret_realm / village_wilds 正是这样的表，2026-09-29 清理时已删。）"""
    battles: dict[str, dict] = {}
    for path in sorted((ROOT / "data" / "battles").rglob("*.json")):
        payload = read_json(path, report)
        if isinstance(payload, dict) and payload.get("id"):
            battles[str(payload["id"])] = payload
    roles: dict[str, dict] = {}
    for path in sorted((ROOT / "data" / "roles").rglob("*.json")):
        payload = read_json(path, report)
        if isinstance(payload, dict) and payload.get("id"):
            roles[str(payload["id"])] = payload
    herbs: set[str] = set()
    for path in sorted((ROOT / "data" / "items" / "herbs").glob("*.json")):
        payload = read_json(path, report)
        if isinstance(payload, dict) and payload.get("id"):
            herbs.add(str(payload["id"]))

    tables: dict[str, list[str]] = {}
    for path in sorted((ROOT / "data" / "encounters").rglob("*.json")):
        payload = read_json(path, report)
        if not isinstance(payload, dict):
            continue
        rel = str(path.relative_to(ROOT))
        tables[str(payload.get("id"))] = check_encounter_table(rel, payload, battles, report)

    used_battles: set[str] = set()
    for parsed in maps:
        rel = str(parsed.path.relative_to(ROOT))
        payload = read_json(parsed.path, report)
        chapter = map_properties(payload).get("chapter") if isinstance(payload, dict) else None
        for record in parsed.objects:
            if record["type"] != "encounter":
                continue
            where = f"{rel} 对象 {record['name']}"
            props = record["props"]
            table_id = props.get("table_id", "")
            if table_id not in tables:
                report.error(where, f"table_id 在 data/encounters 里没有这张表：{table_id}")
            else:
                used_battles.update(tables[table_id])
            try:
                if int(chapter) < ENCOUNTER_FIRST_CHAPTER:
                    report.error(where, f"第 {chapter} 章的图不放野外遭遇（第 1–2 章韩立还是孩子）")
            except (TypeError, ValueError):
                report.error(where, f"所在地图的 chapter 属性读不出整数：{chapter!r}")
            try:
                if int(props.get("steps_min", "0")) < 1:
                    report.error(where, "steps_min 至少是 1")
                if "daily_cap" in props and int(props["daily_cap"]) < 1:
                    report.error(where, "daily_cap 写了就得是正整数")
            except ValueError:
                report.error(where, "steps_min / daily_cap 不是整数")

    # 同章剧情战里最轻的那一场：(凶, 编成 id)。识海之战打的是元神，不拿来比。
    story_floor: dict = {}
    for battle_id, battle in battles.items():
        if battle_id.startswith(ENCOUNTER_BATTLE_PREFIX) or battle.get("terrain") == MIND_TERRAIN:
            continue
        chapter = battle.get("chapter")
        menace = battle_menace(battle, roles)
        if chapter not in story_floor or menace < story_floor[chapter][0]:
            story_floor[chapter] = (menace, battle_id)

    for battle_id in sorted(used_battles):
        check_encounter_battle(battle_id, battles[battle_id], roles, herbs, story_floor, report)


# ---------------------------------------------------------------------------
# 规则 28：脚本点播的 BGM（scripts/common/api.lua 的 bgm）
# ---------------------------------------------------------------------------

SCRIPT_BGM_CALL = re.compile(r'\bbgm\s*\(\s*["\']([^"\']*)["\']\s*\)')
MAP_BGM = "map"   # bgm("map") 撤掉点播、放回地图曲（src/script/Command.h 的 kMapBgm）


def check_script_bgm(report: Report) -> None:
    """规则 28：每一处 bgm("id") 点的曲子都在 assets/bgm/ 里（或是撤回地图曲的 "map"）。

    引擎找不到曲子时静默不放（Engine::playBgm），夜探那一段就成了一片死寂——没有任何报错。"""
    bgm_dir = ROOT / "assets" / "bgm"
    if not bgm_dir.exists():
        report.warn("assets/bgm/", "目录不存在，跳过脚本 bgm 引用检查")
        return
    for path in sorted((ROOT / "scripts").rglob("*.lua")):
        if path.parent.name == "common":
            continue
        source = strip_lua_comments(path.read_text(encoding="utf-8"))
        for track in sorted(set(SCRIPT_BGM_CALL.findall(source))):
            if track != MAP_BGM and not (bgm_dir / f"{track}.ogg").exists():
                report.error(str(path.relative_to(ROOT)), f"bgm 点的曲子不存在：assets/bgm/{track}.ogg")


# ---------------------------------------------------------------------------
# 规则 29：出门方向守恒、按住不回弹（docs/map_spec.md 第 7 节）
# ---------------------------------------------------------------------------

# 四个方向 → 迈一步的位移（y 朝下，与 tmj 的格子坐标同向）。键就是 spawn / npc 的
# facing 取值（map_spec 4.1）；字典的先后也是报错时列方向的先后。
STEP_DIRECTIONS = {"up": (0, -1), "down": (0, 1), "left": (-1, 0), "right": (1, 0)}
DIRECTION_WORDS = {"up": "上", "down": "下", "left": "左", "right": "右"}


def direction_word(name: str) -> str:
    """up → 上。认不得的原样给出：facing 写错了，报错里也得让人看见写的是什么。"""
    return DIRECTION_WORDS.get(name, name or "(空)")


def portal_entry_directions(parsed: ParsedMap, collision: list, occupied: dict,
                            portal: dict) -> set[str]:
    """玩家按哪几个方向键能踩上这道门 ——「进门方向」。

    对门的每一格 c、每个方向 d：c-d 在图内、不是这道门自己的格、也不挡路，玩家就能
    站在 c-d 往 d 迈一步踩上 c。挡路与规则 13 共用同一份 cell_blocked（越界、collision
    非 0、facility 与一定在场的 npc 的整块矩形），那就是 WorldScene::tryStep 挡人的
    口径；enter 触发不挡。c 自己挡路的那一格不算：tryStep 头一件事就是 walkable(target)，
    踩不上的格子从哪一面都进不来。
    """
    cells = set(object_cells(portal))
    entry: set[str] = set()
    for cx, cy in cells:
        if cell_blocked(parsed, collision, occupied, (cx, cy)):
            continue
        for name, (dx, dy) in STEP_DIRECTIONS.items():
            source = (cx - dx, cy - dy)
            if source not in cells and not cell_blocked(parsed, collision, occupied, source):
                entry.add(name)
    return entry


def portal_main_direction(parsed: ParsedMap, portal: dict, entry: set[str]) -> str | None:
    """这道门的「出门方向」：从这张图往哪边走出去。

    贴边的门就是那条边：贴上边（y==0）往上、贴下边（y+h==H）往下、贴左边（x==0）往左、
    贴右边（x+w==W）往右 —— 哪怕是侧身挤进去的，出了这条边，地理上就是往那边去了。
    同时贴两条边以上（墙角的门、横贯整条边的门）只认进得去的那一条。
    不贴边的内门：只有一面进得去，就是那一面；几面都进得去就没有唯一的出门方向，
    返回 None，由调用方改问「落点朝向是不是其中一面」。
    """
    edges = [name for name, touches in (
        ("up", portal["y"] == 0),
        ("down", portal["y"] + portal["h"] == parsed.height),
        ("left", portal["x"] == 0),
        ("right", portal["x"] + portal["w"] == parsed.width),
    ) if touches]
    if len(edges) == 1:
        return edges[0]
    candidates = [name for name in edges if name in entry] or sorted(entry)
    return candidates[0] if len(candidates) == 1 else None


def hold_walk(parsed: ParsedMap, collision: list, occupied: dict, portal_at: dict,
              start: tuple[int, int], direction: str) -> tuple[int, dict | None]:
    """从 start 按住 direction 不松手：返回 (迈了几步, 踩上的那道门)；撞墙停下时门为 None。

    逐步照 WorldScene::tryStep 的先后：下一格挡路就原地不动（按住也是白按）；否则走
    过去，那一格有传送点就换图。enter 触发不拦 —— 踩过去即可，它演不演剧情与本规则
    无关。同一格叠了几道门，取对象表里靠前的那道：objectAt 就是这么取的。
    越界也算挡路，所以这一趟一定走得到头。
    """
    dx, dy = STEP_DIRECTIONS[direction]
    x, y = start
    steps = 0
    while True:
        nxt = (x + dx, y + dy)
        if cell_blocked(parsed, collision, occupied, nxt):
            return steps, None
        x, y = nxt
        steps += 1
        if nxt in portal_at:
            return steps, portal_at[nxt]


def door_terrain(maps: list[ParsedMap], report: Report) -> dict[str, tuple[list, dict, dict]]:
    """每张图的 (collision 层, 占格, 格 → 这一格上对象表里靠前的那道门)。

    规则 29 的两头都要用：进门方向在 A 图上算，按住走在 B 图上走。读不出 collision 层
    的图不收（规则 1/2 已经报过），用到它的门跳过。
    """
    terrain: dict[str, tuple[list, dict, dict]] = {}
    for parsed in maps:
        collision = collision_layer(parsed, report)
        if collision is None:
            continue
        portal_at: dict[tuple[int, int], dict] = {}
        for record in parsed.portals:       # parsed.portals 保持对象表的先后
            for cell in object_cells(record):
                portal_at.setdefault(cell, record)
        terrain[parsed.map_id] = (collision, occupied_cells(parsed), portal_at)
    return terrain


FACE_COUNT_WORDS = {2: "两", 3: "三", 4: "四"}


def facing_complaint(door: str, label: str, facing: str, main: str | None,
                     entry: set[str]) -> str | None:
    """规则 29 (a)：落点朝向不对就返回报错正文，对了返回 None。

    有出门方向的门，朝向必须就是它；没有出门方向的（几面都进得去的内门），朝向是
    其中一面就行。
    """
    if main is not None:
        if facing == main:
            return None
        return (f"规则 29：{door} 是往{direction_word(main)}走进去的，落点 {label} 却面朝"
                f"{direction_word(facing)}——出门往哪走，进门就该朝哪（衔接掉头或拐弯）")
    if facing in entry:
        return None
    faces = "、".join(direction_word(d) for d in STEP_DIRECTIONS if d in entry)
    return (f"规则 29：{door} 从{faces}{FACE_COUNT_WORDS.get(len(entry), '几')}面都走得进去，"
            f"落点 {label} 却面朝{direction_word(facing)}，哪一面都不是"
            "——出门往哪走，进门就该朝哪（衔接掉头或拐弯）")


def check_portal_direction(maps: list[ParsedMap], report: Report) -> None:
    """规则 29：出门往哪走，进门就朝哪；进了门按住方向键，不许一路走回原图。

    它问两件事，都是玩家一迈脚就撞上的：
      (a) 朝向 —— 从 A 图往下走出去，落进 B 图时应当还面朝下：地理上是一路往南走。
          面朝上就是「衔接掉头」，人明明在往南走，一进门转身朝北，方向感当场反了；
          面朝左右是拐弯。
      (b) 不回弹 —— 进门那一刻玩家多半还按着方向键：WorldScene::update 是 keyDown 加
          步进冷却，按住就一直走。落点若离回程门只有一两格、又正对着它，不松手就被送回
          A 图，再按住又被送回来 —— 两张图之间来回弹，玩家根本进不去。

    漏了会怎样：什么都不报。每张图单看都合规，规则 5 / 13 / 16 / 18 全绿，只有真人
    按着方向键走一遍才发作。第 1 章就是这么上线的：从韩家村南口往下走，落在青牛镇
    最下面、离回村的门一两格、朝向还反了，多按一下「下」就弹回村里，再按又弹回镇上。

    口径：
      · 挡路与规则 13 共用 cell_blocked（越界、collision 非 0、facility 与不带
        visible_flag / hidden_flag 的 npc 的整块矩形）；带旗标的 npc 与 enter 触发不挡。
      · 进门方向：玩家按哪几个方向键踩得上这道门（portal_entry_directions）。为空说明
        这道门踩不上去，那归规则 16，这里跳过。
      · 主方向：贴边的门就是那条边的方向；内门只有一面进得去就是那一面。几面都进得去
        的内门没有主方向，(a) 改问「落点朝向是不是其中一面」—— ch01_qixuanmen 的
        portal_to_caixiashan 下、左、右三面都进得去，落点朝下是对的。
      · (b) 对每一个进门方向都按住走一遍：进门时按的是哪个键，落地后就还按着哪个键。
      · target_map / target_spawn 找不到归规则 5。脚本 teleport() 不在本规则范围：
        剧情传送的落点与朝向是剧本定的，玩家那时也没在走路。
    """
    by_id = {m.map_id: m for m in maps}
    terrain = door_terrain(maps, report)
    for parsed in maps:
        if parsed.map_id not in terrain:
            continue
        rel = str(parsed.path.relative_to(ROOT))
        collision, occupied, _ = terrain[parsed.map_id]
        for portal in parsed.portals:
            target = by_id.get(portal["props"].get("target_map", ""))
            spawn_id = portal["props"].get("target_spawn", "")
            landing = spawn_landing(target, spawn_id) if target is not None else None
            if landing is None or target.map_id not in terrain:
                continue        # 目标图或落点不存在：规则 5 已经报过
            entry = portal_entry_directions(parsed, collision, occupied, portal)
            if not entry:
                continue        # 从哪一面都踩不上去：规则 16 已经报过
            door = f"{parsed.map_id} 的 {portal['name']}"
            label = f"{target.map_id}:{landing['props'].get('id', spawn_id)}"

            # (a) 朝向
            complaint = facing_complaint(door, label, landing["props"].get("facing", ""),
                                         portal_main_direction(parsed, portal, entry), entry)
            if complaint:
                report.error(rel, complaint)

            # (b) 不回弹
            b_collision, b_occupied, b_portals = terrain[target.map_id]
            for direction in STEP_DIRECTIONS:
                if direction not in entry:
                    continue
                steps, hit = hold_walk(target, b_collision, b_occupied, b_portals,
                                       (landing["x"], landing["y"]), direction)
                if hit is None or hit["props"].get("target_map", "") != parsed.map_id:
                    continue
                word = direction_word(direction)
                report.error(rel, f"规则 29：{door} 往{word}走进去、落在 {label}，"
                                  f"不松手接着往{word}走 {steps} 步就踩上 {target.map_id} 的 "
                                  f"{hit['name']}，又被送回 {parsed.map_id}——来回弹")


# ---------------------------------------------------------------------------
# 规则 30：同一张图上同时在场的 NPC 不重名（docs/map_spec.md 第 7 节）
# ---------------------------------------------------------------------------

# 群像白名单：刻意让同一个 role 在一张图上并排站好几个的那几种人。role → 理由。
#
# 名牌上写的是 role 的名字（data/roles/<role_id>.json 的 name），同 role 就同名。
# 一排同名的护院是这种场面要的效果，但得先保证名牌不叠成一团才站得住——
# src/game/WorldView.cpp 画名牌时「撞上了就往上叠一行」，就是为墨府门口那四个做的。
#
# 往这里加 role 之前先问一句：玩家看见两块一模一样的名牌，会不会以为是同一个人、
# 或者以为自己找错了人？多数时候会 —— 那就去 data/roles/ 另建一个角色，别加白名单。
CROWD_ROLES = {
    "mofu_huyuan": "ch05_nancheng 墨府门口刻意并排站了 4 个护院（守门的阵仗），名牌已做防重叠",
}


def npcs_take_turns(a: dict, b: dict) -> bool:
    """两个 npc 是不是轮流在场、永不同框：一个的 hidden_flag 恰是另一个的 visible_flag。

    这是「同一个人换一段剧情」的标准写法 —— 一个 npc 对象只挂得住一个 script，所以
    ch02_wairentang 的 npc_li_feiyu / npc_li_feiyu_ch03、ch02_yaopu 的两个药圃管事
    都是这样一前一后接班：旗标置上之前是前一个，置上之后是后一个。旗标只增不减，
    任何时刻至多一个在场，名牌撞不上。
    """
    a_hide, a_show = a["props"].get("hidden_flag", ""), a["props"].get("visible_flag", "")
    b_hide, b_show = b["props"].get("hidden_flag", ""), b["props"].get("visible_flag", "")
    return bool(a_hide and a_hide == b_show) or bool(b_hide and b_hide == a_show)


def role_display_names(report: Report) -> dict[str, str]:
    """role_id → 名牌上的字：data/roles 下每份角色文件的 name。

    引擎画名牌取的就是它（Application::speakerName → RoleTemplate::name）；查不到角色时
    名牌上写的是 role_id 本身，所以调用方拿不到名字就退回 role_id —— 与引擎同一个口径。
    读不出来、缺 id 或 name 的文件跳过：那些归 collect_data 报（全量校验时），这里再报
    一遍只是刷屏。
    """
    names: dict[str, str] = {}
    roles_dir = ROOT / "data" / "roles"
    if not roles_dir.exists():
        report.warn("data/roles/", "目录不存在，规则 30 只比 role_id、不比名牌上的名字")
        return names
    for path in sorted(roles_dir.rglob("*.json")):
        try:
            payload = json.loads(path.read_text(encoding="utf-8"))
        except (OSError, UnicodeDecodeError, json.JSONDecodeError):
            continue
        if isinstance(payload, dict) and payload.get("id") and isinstance(payload.get("name"), str) \
                and payload["name"]:
            names[str(payload["id"])] = payload["name"]
    return names


def npc_clash_groups(npcs: list[dict], names: dict[str, str]) -> tuple[dict, dict]:
    """两两比一遍，返回 (同 role 撞的：role → 下标集, 不同 role 同名撞的：名字 → 下标集)。

    一对 npc 只落进其中一边：同 role 的只算同 role（role 一样名字当然一样，不再按同名
    报第二遍）。轮流在场的一对不算；两个人的 role 都在群像白名单里才放 —— 只有一边在，
    另一边就是个恰好同名的别人。
    """
    same_role: dict[str, set[int]] = {}
    same_name: dict[str, set[int]] = {}
    for i, a in enumerate(npcs):
        for j in range(i + 1, len(npcs)):
            b = npcs[j]
            role_a, role_b = a["props"]["role_id"], b["props"]["role_id"]
            if npcs_take_turns(a, b) or (role_a in CROWD_ROLES and role_b in CROWD_ROLES):
                continue
            if role_a == role_b:
                same_role.setdefault(role_a, set()).update((i, j))
            elif names.get(role_a, role_a) == names.get(role_b, role_b):
                same_name.setdefault(names.get(role_a, role_a), set()).update((i, j))
    return same_role, same_name


def check_npc_role_clash(maps: list[ParsedMap], report: Report) -> None:
    """规则 30：同一张图上可能同时在场的 npc，名牌上不许撞名。

    它问：玩家站在这张图上，会不会同时看见两块一模一样的名牌？名牌写的是 role 的
    名字（data/roles/<id>.json 的 name），玩家分不出谁是谁、该找哪一个问话。两种写法
    都会撞：同一个 role 摆了好几个；role 不同、name 却一样 —— 新建 role 时照抄了旧
    role 的名字。同 role 的只按同 role 报一条，不因为同名再报一遍。

    漏了会怎样：什么都不报。role 存在（规则 6）、人走得到（规则 13）、话按得出来
    （规则 17），样样合规，满街却站着三个「卖药郎」。青牛镇就是这么上线的：6 个路人
    复用了 3 个打探 NPC 的 role，名牌上同时出现了三个「卖药郎」。只比 role_id 的话，
    照抄名字的新 role 会让同一个「卖药郎」换个 id 再冒出来 —— 投诉的是名牌，不是 id。

    放行两种，此外一律报：
      · 轮流在场（npcs_take_turns）：一个的 hidden_flag 恰是另一个的 visible_flag；
      · 群像（CROWD_ROLES）：刻意并排站的那几种人，每一个都写了理由。

    口径与规则 17 的「只报必然」相反，**可能同框就报**：旗标之间谁先谁后静态推不全，
    而修法很便宜 —— 另建一个 role、换个名字，或者把两个人的旗标写成互补。
    """
    names = role_display_names(report)
    complementary = ("或把它们的 visible_flag / hidden_flag 写成互补"
                     "（一个的 hidden_flag 恰是另一个的 visible_flag）")
    for parsed in maps:
        rel = str(parsed.path.relative_to(ROOT))
        npcs = [record for record in parsed.objects
                if record["type"] == "npc" and record["props"].get("role_id")]
        same_role, same_name = npc_clash_groups(npcs, names)
        for role, members in sorted(same_role.items(), key=lambda item: min(item[1])):
            clash = "、".join(npcs[k]["name"] for k in sorted(members))
            report.error(rel, f"规则 30：{parsed.map_id} 上 {len(members)} 个 npc 顶着同一个 role "
                              f"{role}、可能同时在场（{clash}）——名牌会撞名。另建一个 role，"
                              + complementary)
        for name, members in sorted(same_name.items(), key=lambda item: min(item[1])):
            clash = "、".join(npcs[k]["name"] for k in sorted(members))
            roles = " 与 ".join(dict.fromkeys(npcs[k]["props"]["role_id"] for k in sorted(members)))
            report.error(rel, f"规则 30：{parsed.map_id} 上 role {roles} 的名牌都是「{name}」、"
                              f"可能同时在场（{clash}）——名牌会撞名。给其中一个换个名字"
                              "（data/roles 里的 name），" + complementary)


# 只给配方当门闸的占位旗标（docs/interfaces-p3-ch07.md 1.2、5.1 第 5 条）：**永不置**，挂在它上面的
# 方子（结丹灵药方与四张制符方）就永远不列；写到用得上那几张方子的章时换成那一章的旗标。
RECIPE_ONLY_FLAG = "story.recipe_later"


def check_recipe_only_flag(report) -> None:
    """story.recipe_later 只许出现在 data/recipes/** 的 requireFlag 里；任何脚本置它都报错。

    别处引用它——地图门闸、路径行动、任务、目标链——都等于写了一道永远开不了的门；
    脚本一旦置了它，那几张还没写到的方子就在炼制面板上提前露了名（第 4–6 章的名字泄漏，
    docs/ch07-design.md 第 7 节「堵泄漏」）。"""
    data_dir = ROOT / "data"
    sources = sorted(data_dir.rglob("*.json")) if data_dir.exists() else []
    maps_dir = ROOT / "maps"
    sources += sorted(maps_dir.glob("*.tmj")) if maps_dir.exists() else []
    for path in sources:
        # flags.json 是登记表本身；text/ 是文案，不是引用方。
        if path.name == "flags.json" or path.parent.name == "text":
            continue
        payload = read_json(path, report)
        if payload is None:
            continue
        in_recipes = path.is_relative_to(data_dir) and path.relative_to(data_dir).parts[0] == "recipes"
        pairs = []
        walk_json(payload, [], pairs)
        for field_name, value in pairs:
            if value == RECIPE_ONLY_FLAG and not (in_recipes and field_name == "requireFlag"):
                report.error(str(path.relative_to(ROOT)),
                             f"{RECIPE_ONLY_FLAG} 只许写在 data/recipes/** 的 requireFlag 里（这里是字段 "
                             f"{field_name}）：它永不置，挂在别处就是一道永远开不了的门")
    scripts_dir = ROOT / "scripts"
    for path in sorted(scripts_dir.rglob("*.lua")) if scripts_dir.exists() else []:
        if path.parent.name == "common":
            continue
        source = strip_lua_comments(path.read_text(encoding="utf-8"))
        if RECIPE_ONLY_FLAG in SCRIPT_FLAG_SET.findall(source):
            report.error(str(path.relative_to(ROOT)),
                         f"脚本置了 {RECIPE_ONLY_FLAG}：它永不置，一置就把还没写到的方子提前露在炼制面板上"
                         "（写到那一章时把那几张方子的 requireFlag 换成那一章的旗标）")


def check_data_references(texts, flags, defined, report):
    """校验 data/ 下数据文件里的引用：文案 key、旗标、以及物品/角色/战斗/法术 id。"""
    data_dir = ROOT / 'data'
    if not data_dir.exists():
        return

    # 空类别的引用先记账，最后每类汇总成一条，见下面的说明。
    empty_refs = {}

    for path in sorted(data_dir.rglob('*.json')):
        # text/ 是文案本身，flags.json 是登记表，都不是引用方。
        if path.parent.name == 'text' or path.name == 'flags.json':
            continue
        if path.relative_to(data_dir).parts[0] == 'visual':
            continue
        payload = read_json(path, report)
        if payload is None:
            continue
        rel = str(path.relative_to(ROOT))

        pairs = []
        walk_json(payload, [], pairs)
        for field, value in pairs:
            if not isinstance(value, str) or not value:
                continue
            probe = normalise_field(field)
            if probe.endswith(TEXT_KEY_SUFFIX) and texts and value not in texts:
                report.error(rel, '文案 key 不存在：' + value + '（字段 ' + field + '）')
                continue
            if probe.endswith(FLAG_PROP_SUFFIX) and flags and value not in flags:
                report.error(rel, '旗标未在 data/flags.json 登记：' + value)
                continue
            kind = REFERENCE_FIELDS.get(field) or REFERENCE_FIELDS.get(probe)
            if kind is None:
                continue
            known = defined.get(kind, set())
            if not known:
                # 该类别一条都没定义。早先这里直接跳过，理由是「空目录不该把每条
                # 引用都报成错」——可那样一来整类引用就没人查了，而且查不出来。
                # data/magics/ 空着的那阵子，七个角色的法术引用全在裸奔；
                # 谁往里加第一个文件，那七条会一起变成错误，看上去像是新文件的错。
                # 现在既不淹没也不放过：每个空类别只报一条，把账说清楚。
                empty_refs.setdefault(kind, []).append((rel, value))
                continue
            if value not in known:
                report.error(rel, kind + ' id 不存在：' + value + '（字段 ' + field + '）')

    for kind in sorted(empty_refs):
        refs = empty_refs[kind]
        sample = '、'.join(v for _rel, v in refs[:3])
        if len(refs) > 3:
            sample += ' 等 %d 处' % len(refs)
        report.error('data/' + REFERENCE_SOURCES.get(kind, kind) + '/',
                     '该类别一条 id 都没定义，却被引用了 %d 次（%s）：'
                     '这些引用现在全都没人校验' % (len(refs), sample))


# ---------------------------------------------------------------------------

def main() -> int:
    parser = argparse.ArgumentParser(description="内容校验")
    parser.add_argument("--maps", action="store_true", help="只查地图")
    parser.add_argument("--data", action="store_true", help="只查数据与文案")
    parser.add_argument("--magic-targets", action="store_true", help="只查 E3 法术 target 形状")
    args = parser.parse_args()
    if args.magic_targets and (args.maps or args.data):
        parser.error("--magic-targets 不与 --maps / --data 同用")
    check_all = not (args.maps or args.data)

    report = Report()
    if args.magic_targets:
        check_magic_target_data(report)
        for error in report.errors:
            print(f"[error] {error}")
        print("MAGIC_TARGETS_OK" if report.ok else "MAGIC_TARGETS_FAIL")
        return 0 if report.ok else 1
    ids: dict[str, Path] = {}
    texts: dict[str, str] = {}
    flags: set[str] = set()

    if check_all or args.data:
        ids, texts = collect_data(report)
        flags = load_registered_flags(report)

    if check_all or args.maps:
        maps_dir = ROOT / "maps"
        parsed_maps: list[ParsedMap] = []
        if not maps_dir.exists():
            report.warn("maps/", "目录不存在，跳过地图检查")
        else:
            for path in sorted(maps_dir.glob("*.tmj")):
                parsed = check_map(path, report)
                if parsed is not None:
                    parsed_maps.append(parsed)
                    check_reachability(parsed, report)
                    check_interaction_shadow(parsed, report)
            check_cross_map(parsed_maps, report)
            check_deny_text_keys(parsed_maps, report)
            check_world_reachability(parsed_maps, report)
            check_flag_order(parsed_maps, report)
            check_map_name_keys(parsed_maps, report)
            # 规则 29、30 只看 tmj，--maps 这一档就得查：自检跑的正是这一档。
            check_portal_direction(parsed_maps, report)
            check_npc_role_clash(parsed_maps, report)
            # 只要 scripts/ 在，这两条就能查：比对的是地图与脚本，不牵扯 data/。
            check_trigger_once_contract(parsed_maps, report)
            check_hook_declarations(parsed_maps, report)
            if check_all:
                check_map_references(parsed_maps, ids, texts, flags, report)
                defined = collect_reference_ids(report)
                check_scripts(ids, texts, flags, parsed_maps, report, defined)
                check_data_references(texts, flags, defined, report)
                check_objectives(parsed_maps, flags, report)
                path_sources = check_path_actions(parsed_maps, flags, texts, defined, report)
                check_quests(parsed_maps, flags, defined, report, path_sources)
                check_battle_break_data(report)
                check_recipe_only_flag(report)
                check_encounters(parsed_maps, report)
                check_script_bgm(report)

    for warning in report.warnings:
        print(f"[warn]  {warning}")
    for error in report.errors:
        print(f"[error] {error}")

    print()
    print(f"数据条目 {len(ids)} 项，文案 {len(texts)} 条，旗标 {len(flags)} 个")
    print(f"错误 {len(report.errors)}，警告 {len(report.warnings)}")
    if report.ok:
        print("VALIDATE_OK")
        return 0
    return 1


if __name__ == "__main__":
    sys.exit(main())
