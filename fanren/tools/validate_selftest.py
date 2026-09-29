#!/usr/bin/env python3
# 文档串用 raw：G 那一条要原样写出正则里的 `bottle\.mature`，普通串会让
# Python 报 SyntaxWarning（"\." is an invalid escape sequence），而本文件讲的
# 正是「转义被吃掉」这件事，自己先被吃一次实在说不过去。
r"""门禁自身的负向自检：故意写坏，看 validate.py 与 genmaps.py 是不是真抓得住。

    python tools/validate_selftest.py

为什么需要这个文件：本项目最惨的一次教训是校验器的一条正则被 heredoc 吃掉了
转义，从此永远报通过，而没人发现——因为没有任何人拿一份**确实是坏的**内容喂过
它。一条检查只跑过正例，等于没被验证过：它可能在查，也可能永远返回绿灯，
两种情况下门禁的输出一模一样。

所以每加一条检查，这里就配一条「故意写坏 → 必须被抓住」，外加一条「不动它的
副本 → 必须是干净的」作对照。全部在临时目录的副本上做，仓库里的 maps/ 一个
字节都不碰。

覆盖的三处：

  A. once 触发器契约 —— 缺 set_flag / set_flag 指向脚本从不置的旗标 /
     once=false 却写了 set_flag，三种写法都必须报错。
  B. 地图生成器重跑 —— 在副本上真跑一次 genmaps.py，闸门与对象一处都不许丢；
     再把副本改坏，genmaps.py --check 必须报漂移。
  B2. 漂移门禁的覆盖面 —— 第 2 章那四张图也必须被盯住。
  C. 连通性口径 —— 把一个 2x2 的设施压在唯一走廊上（**只加对象，不动
     collision 层**），必须报不可达；同一张图用旧的「只走 collision」口径算，
     则一片绿——这正是这条修复要拿掉的假绿灯。

  D. 跨图可达性 —— 抹掉神手谷南墙那个通往第 2 章的传送点，ch02 的四张图必须
     被报成走不到；同时断言逐图检查（规则 13）对这处断链确实一个字都不说。
  E. 同格遮蔽 —— npc 压设施、靠前的哑巴 npc 压靠后的有脚本 npc、enter 触发压
     传送点，三种都必须报；带 visible_flag 的 npc 与 once=true 的触发器则不许报，
     它们会让位，不是永久遮蔽。

  F. 旗标顺序死锁 —— require_flag / guard_flag / visible_flag 三种成环都必须报；
     而「看着像环、其实旗标另有来源」的那一种不许报。

  G. 脚本里的物品 id —— give / take / bottle.mature 三种写法用了不存在的药名
     都必须报；而 my_bottle.mature / bottleXmature 这类形近写法不许报
     （这一条同时钉住那条正则里 `bottle\.mature` 的转义没被吃掉）。

  H. deny_text_key —— 退回旧的按源图命名、与 require_flag 落单，都必须报；
     而两者皆无的普通传送点不许报。

  K. 脚本首部的挂点声明（`-- @hook`）—— 声明与 maps/*.tmj 对不上必须报，
     **两个方向都要**：改脚本的声明要报，改地图的属性也要报（少了后半条，
     一条只读脚本、从不打开 tmj 的假检查也能全绿）。写坏的声明不许被当成普通
     注释放过，而形近的 `@hookpoint` 不许误报——那一条同时钉住正则里 `@hook\b`
     的 `\b` 没有被吃掉，正是本项目最惨那次事故的形状。
     没有声明的老脚本已一次性补齐（55 个），所以这里不设过渡期用例。

  J. NPC 可见性 —— 不在场的 npc 既不占格也不抢话（引擎 2026-09-21 改）。
     同格两个 npc、各带互补旗标不许报；去掉旗标必须报。一个带 visible_flag 的
     npc 堵在唯一走廊上不许报不可达；去掉旗标必须报。每一对的后半条都是在问
     「这条检查还有没有牙」——只写前半条的话，把整条检查注释掉也能全绿。

  M. 任务文件（规则 23，docs/interfaces-p3-ch05.md 1.8）—— 一份引用全部真实存在的
     探针支线写进副本，先验它一条不报；再逐条改坏一处：文件名、kind、章号、必填 key、
     非空、成对、谓词三种写法、旗标登记且有人置、物品存在、表外字段，外加「一章一条
     主线」。每一条都必须被抓住。

  N. 破势与蓄劲（规则 24、25，docs/octopath-battle.md）—— 角色的类别名、架势、行动次数、
     蓄势四项；法术的 boost；兵器的 weapon；格子时代的遗留字段（宽高、坐标、castRange、
     useRange）；上场的敌人必须有架势与破绽；第 1–5 章每个敌人至少一样破绽是那一场我方
     必有的手段打得到的——外加那张手段表自己的来路（火弹术的五行、脚本里的 magic.learn、
     蚀心散带不带毒）。每一条都配一条「按这一场算、不该报」的对照：同一只狼在第 5 章有火弹、
     僵兽只剩毒、第 6 章不在范围、识海没有架势。

  R. 衔接朝向与回弹（规则 29）—— 落点掉头且回程门就在跟前：朝向与来回弹都必须报；只把
     落点朝向拧 90°：只许报朝向、不许报来回弹；多入口的内门落点朝其中一面不许报，改朝
     进不去的那一面必须报。门与落点从几张不在返修的图里现找，不写死坐标。

  S. 同图 npc 不重名（规则 30）—— 复制一个没旗标的 npc 成同 role 必须报、且只报一条；同一对
     写成互补旗标、或换成群像白名单里的 role 不许报；两个都挂同一个 visible_flag 仍然必须报；
     孪生换成一个照抄了名字的新 role（id 不同、名牌相同）必须报，写成互补旗标则不许报。

退出码 0 表示每条负向用例都如期被抓住。
"""

from __future__ import annotations

import contextlib
import io
import re
import json
import shutil
import sys
import tempfile
from collections import deque
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
sys.path.insert(0, str(ROOT / "tools" / "mapgen"))

import validate  # noqa: E402

TILE = 32
# 外刃堂的二门夹道：整张图南北之间唯一的通路（genmaps_ch02.py 的注释自陈
# 「进后院的唯一通路」）。把设施压在这两格上，后院就彻底断了。
CHOKE_MAP = "ch02_wairentang"
CHOKE_CELL = (23, 27)


class Case:
    """一条自检用例的记账。"""

    def __init__(self) -> None:
        self.failed = 0
        self.total = 0

    def check(self, name: str, ok: bool, detail: str = "") -> None:
        self.total += 1
        print("  [%s] %s" % ("PASS" if ok else "FAIL", name))
        if detail:
            for line in detail.strip().split(chr(10)):
                print("         " + line)
        if not ok:
            self.failed += 1


# ---------------------------------------------------------------------------
# 副本与运行
# ---------------------------------------------------------------------------

def make_workdir(base: Path) -> Path:
    """搭一份只读副本：maps/ 与 scripts/ 就够 --maps 这一档用了。"""
    work = base / "repo"
    work.mkdir(parents=True, exist_ok=True)
    shutil.copytree(ROOT / "maps", work / "maps")
    shutil.copytree(ROOT / "scripts", work / "scripts")
    return work


def run_validate(work: Path) -> list[str]:
    """在副本上跑一次真正的 validate.main()，返回报出来的错误行。

    调的是玩家……不，是 CI 跑的同一个入口，不是内部函数的拼装：门禁真正的行为
    包括「哪些检查被 main 串起来了」，绕开 main 就测不到漏接的那一条。
    """
    saved_root, saved_argv = validate.ROOT, sys.argv
    validate.ROOT = work
    sys.argv = ["validate.py", "--maps"]
    buffer = io.StringIO()
    try:
        with contextlib.redirect_stdout(buffer):
            validate.main()
    finally:
        validate.ROOT, sys.argv = saved_root, saved_argv
    return [line for line in buffer.getvalue().split(chr(10)) if line.startswith("[error]")]


def make_full_workdir(base: Path) -> Path:
    """连 data/ 一起的副本。

    check_scripts 只在「全量」模式下才被 main 串起来，而它要拿 data/ 里的物品
    id 才比对得动：ids 为空时那条检查会整条跳过——那种跳过与「查过了、没问题」
    在门禁的输出里一模一样，正是这个文件存在的理由。
    """
    work = base / "repo"
    work.mkdir(parents=True, exist_ok=True)
    for name in ("maps", "scripts", "data"):
        shutil.copytree(ROOT / name, work / name)
    return work


def run_validate_full(work: Path) -> list[str]:
    """在副本上跑一次全量 validate.main()（不带 --maps），返回报出来的错误行。"""
    saved_root, saved_argv = validate.ROOT, sys.argv
    validate.ROOT = work
    sys.argv = ["validate.py"]
    buffer = io.StringIO()
    try:
        with contextlib.redirect_stdout(buffer):
            validate.main()
    finally:
        validate.ROOT, sys.argv = saved_root, saved_argv
    return [line for line in buffer.getvalue().split(chr(10)) if line.startswith("[error]")]


def load_map(work: Path, map_id: str) -> dict:
    return json.loads((work / "maps" / (map_id + ".tmj")).read_text(encoding="utf-8"))


def save_map(work: Path, map_id: str, payload: dict) -> None:
    (work / "maps" / (map_id + ".tmj")).write_text(
        json.dumps(payload, ensure_ascii=False, indent=2) + chr(10), encoding="utf-8")


def objects_layer(payload: dict) -> list[dict]:
    for layer in payload["layers"]:
        if layer.get("type") == "objectgroup":
            return layer["objects"]
    raise AssertionError("这张图没有对象层")


def find_object(payload: dict, name: str) -> dict:
    for entry in objects_layer(payload):
        if entry["name"] == name:
            return entry
    raise AssertionError("找不到对象 " + name)


def props_of(entry: dict) -> dict[str, object]:
    return {p["name"]: p["value"] for p in entry.get("properties", [])}


def set_prop(entry: dict, name: str, value) -> None:
    for prop in entry.get("properties", []):
        if prop["name"] == name:
            prop["value"] = value
            return
    entry.setdefault("properties", []).append(
        {"name": name, "type": "bool" if isinstance(value, bool) else "string", "value": value})


def drop_prop(entry: dict, name: str) -> None:
    entry["properties"] = [p for p in entry.get("properties", []) if p["name"] != name]


# ---------------------------------------------------------------------------
# C：旧口径的对照实现
# ---------------------------------------------------------------------------

def reachable_under_collision_only(payload: dict) -> set[tuple[int, int]]:
    """只走 collision 层的 BFS —— 修复之前 check_reachability 就是这么算的。

    留着它不是为了怀旧：C 这条修复的全部意义在于「旧口径看不见的东西」，
    不把旧口径也算一遍，就没法证明新口径抓到的是真问题而不是噪音。
    """
    width, height = payload["width"], payload["height"]
    layers = {l["name"]: l for l in payload["layers"]}
    collision = layers["collision"]["data"]
    start = None
    for entry in objects_layer(payload):
        if entry.get("type") == "spawn" and props_of(entry).get("default") is True:
            start = (int(entry["x"]) // TILE, int(entry["y"]) // TILE)
    if start is None:
        return set()

    def blocked(cell):
        x, y = cell
        if x < 0 or y < 0 or x >= width or y >= height:
            return True
        return collision[y * width + x] != 0

    seen = {start}
    queue = deque([start])
    while queue:
        x, y = queue.popleft()
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            nxt = (x + dx, y + dy)
            if nxt in seen or blocked(nxt):
                continue
            seen.add(nxt)
            queue.append(nxt)
    return seen


def plug_the_corridor(payload: dict) -> None:
    """把一座 2x2 的设施摆在唯一走廊上。**只加对象，collision 层一格不动**。

    这正是引擎里会挡路而校验器曾经看不见的那种东西：TileMap::objectAt 是矩形
    命中，facility 占的四格玩家一格也走不过去。
    """
    objects_layer(payload).append({
        "id": 9001,
        "name": "facility_selftest_blocker",
        "type": "facility",
        "x": CHOKE_CELL[0] * TILE,
        "y": CHOKE_CELL[1] * TILE,
        "width": 2 * TILE,
        "height": 2 * TILE,
        "rotation": 0,
        "visible": True,
        "properties": [{"name": "kind", "type": "string", "value": "board"}],
    })


# ---------------------------------------------------------------------------
def selftest_baseline(case: Case, work: Path) -> None:
    print("0. 对照：没动过的副本必须是干净的")
    errors = run_validate(work)
    case.check("未改动的副本 0 错误", not errors, chr(10).join(errors[:5]))


def selftest_c_reachability(case: Case, base: Path) -> None:
    print()
    print("C. 设施堵死唯一走廊（只加对象，不动 collision）")
    work = make_workdir(base / "c")
    payload = load_map(work, CHOKE_MAP)
    plug_the_corridor(payload)
    save_map(work, CHOKE_MAP, payload)

    # 先用旧口径算一遍：它看不见 facility，后院照样「可达」。
    seen = reachable_under_collision_only(payload)
    guarded = find_object(payload, "npc_tongmen_luchun")
    guarded_cell = (int(guarded["x"]) // TILE, int(guarded["y"]) // TILE)
    old_blind = any((guarded_cell[0] + dx, guarded_cell[1] + dy) in seen
                    for dx, dy in ((0, 0), (1, 0), (-1, 0), (0, 1), (0, -1)))
    case.check("旧口径（只走 collision）确实看不见这堵墙", old_blind,
               "后院的 npc_tongmen_luchun 在旧口径下仍算可达 —— 这就是那盏假绿灯")

    errors = run_validate(work)
    caught = [e for e in errors if "无法到达" in e or "踏不响" in e or "不可通行" in e]
    case.check("新口径报出不可达", bool(caught),
               chr(10).join(caught[:4]) + (chr(10) + "……共 %d 条" % len(caught) if len(caught) > 4 else ""))


def selftest_a_once_contract(case: Case, base: Path) -> None:
    print()
    print("A. once 触发器契约（set_flag 必须指向脚本自己置的完成旗标）")

    work = make_workdir(base / "a1")
    payload = load_map(work, "ch02_yabi")
    drop_prop(find_object(payload, "trigger_shiping"), "set_flag")
    save_map(work, "ch02_yabi", payload)
    errors = [e for e in run_validate(work) if "trigger_shiping" in e]
    case.check("once=true 却没有 set_flag → 报错", bool(errors), chr(10).join(errors[:2]))

    work = make_workdir(base / "a2")
    payload = load_map(work, "ch02_yabi")
    set_prop(find_object(payload, "trigger_shiping"), "set_flag", "ch02.duan2_start")
    save_map(work, "ch02_yabi", payload)
    errors = [e for e in run_validate(work) if "trigger_shiping" in e]
    case.check("set_flag 指向脚本从不置的旗标 → 报错", bool(errors), chr(10).join(errors[:2]))

    work = make_workdir(base / "a3")
    payload = load_map(work, "ch02_yabi")
    set_prop(find_object(payload, "trigger_shiping"), "once", False)
    save_map(work, "ch02_yabi", payload)
    errors = [e for e in run_validate(work) if "trigger_shiping" in e]
    case.check("once=false 却留着 set_flag → 报错", bool(errors), chr(10).join(errors[:2]))


def selftest_b_regen(case: Case, base: Path) -> None:
    print()
    print("B. 重跑地图生成器不得丢失任何闸门")
    import genmaps

    gate_props = ("require_flag", "deny_text_key", "guard_flag", "set_flag",
                  "visible_flag", "hidden_flag")

    def snapshot(work: Path) -> dict[str, dict[str, object]]:
        out: dict[str, dict[str, object]] = {}
        for path in sorted((work / "maps").glob("*.tmj")):
            payload = json.loads(path.read_text(encoding="utf-8"))
            for entry in objects_layer(payload):
                key = path.stem + "/" + entry["name"]
                props = props_of(entry)
                out[key] = {k: props[k] for k in gate_props if k in props}
        return out

    work = make_workdir(base / "b")
    before = snapshot(work)
    saved_maps, saved_tsets = genmaps.MAPS, genmaps.TSETS
    genmaps.MAPS = str(work / "maps")
    genmaps.TSETS = str(work / "maps" / "tilesets")
    try:
        with contextlib.redirect_stdout(io.StringIO()):
            genmaps.main([])
        after = snapshot(work)
        lost = []
        for key, props in before.items():
            if key not in after:
                lost.append("整个对象没了：" + key)
                continue
            for name, value in props.items():
                if after[key].get(name) != value:
                    lost.append("%s 的 %s：%r → %r" % (key, name, value, after[key].get(name)))
        case.check("在副本上真跑一次 genmaps.py，闸门一处不丢", not lost,
                   chr(10).join(lost[:6]))
        case.check("重跑后的副本仍然通过校验", not run_validate(work))

        # 负向：把副本改坏，--check 必须报漂移。这条是给「生成器与 maps/ 走散」
        # 装的报警器本身做验证——报警器坏了和没装是一样的。
        payload = load_map(work, "ch01_hanjiacun")
        drop_prop(find_object(payload, "portal_to_qingniuzhen"), "require_flag")
        save_map(work, "ch01_hanjiacun", payload)
        buffer = io.StringIO()
        with contextlib.redirect_stdout(buffer):
            code = genmaps.main(["--check"])
        case.check("副本里抹掉一处 require_flag → --check 报漂移", code != 0,
                   chr(10).join(line for line in buffer.getvalue().split(chr(10))
                                if "漂移" in line or "require_flag" in line)[:400])
    finally:
        genmaps.MAPS, genmaps.TSETS = saved_maps, saved_tsets


def add_object(payload: dict, entry: dict, first: bool = False) -> None:
    """往对象层塞一个对象。first=True 插到最前面 —— objectAt 每类只返回第一个
    命中的，「谁在前面」本身就是被测行为的一部分。"""
    layer = objects_layer(payload)
    layer.insert(0, entry) if first else layer.append(entry)


def make_object(name: str, otype: str, cell: tuple[int, int], props: dict,
                size: tuple[int, int] = (1, 1)) -> dict:
    return {
        "id": 9100 + len(name), "name": name, "type": otype,
        "x": cell[0] * TILE, "y": cell[1] * TILE,
        "width": size[0] * TILE, "height": size[1] * TILE,
        "rotation": 0, "visible": True,
        "properties": [{"name": k,
                        "type": "bool" if isinstance(v, bool) else "string",
                        "value": v} for k, v in props.items()],
    }


def selftest_d_world_reachability(case: Case, base: Path) -> None:
    """跨图可达性：逐图全绿也可能整章从游戏里掉出去。"""
    print()
    print("D. 跨图可达性（第 2 章入口断链）")

    work = make_workdir(base / "d")
    payload = load_map(work, "ch01_shenshougu")
    layer = objects_layer(payload)
    layer[:] = [e for e in layer if e["name"] != "portal_to_ch02_yaopu"]
    save_map(work, "ch01_shenshougu", payload)

    errors = run_validate(work)
    broken = [e for e in errors if "走不到这张图" in e]
    case.check("抹掉 portal_to_ch02_yaopu → 报第 2 章断链", bool(broken),
               chr(10).join(broken[:4]))

    # 两个方向都要问：断链下游的图必须被报出来，上游的图一张都不许误伤。
    # 早先这里写的是「报出来的集合恰好等于 ch02 那四张」。第 3 章一落地它就红了，
    # 而且红得没道理——第 3 章挂在第 2 章后面，掐掉第 2 章的入口，第 3 章跟着
    # 走不到才是对的，检查器变好了，断言却把它判成失败。
    # 钉内容的形状（有几张图、叫什么名字）是债，第 1 章的审查就点过这一条。
    named = {e.split(chr(92))[-1].split(".tmj")[0] for e in broken}
    downstream = {"ch02_yaopu", "ch02_jusuo", "ch02_yabi", "ch02_wairentang"}
    upstream = {"ch01_hanjiacun", "ch01_qingniuzhen", "ch01_caixiashan",
                "ch01_qixuanmen", "ch01_liangu_ya", "ch01_shenshougu"}
    case.check("断链下游的图全都报出来了", downstream <= named,
               "漏报 %s" % sorted(downstream - named))
    # 这一条挡的是「见谁都报」：那种检查同样能让上面两条通过。
    case.check("断链上游的图一张没误伤", not (upstream & named),
               "误伤 %s" % sorted(upstream & named))

    # 对照：这四张图单看每一张都完全合规，规则 13 一个字都不会说。
    per_map = [e for e in errors if "无法到达" in e or "踏不响" in e or "按不出来" in e]
    case.check("逐图检查（规则 13）对这处断链确实无话可说", not per_map,
               chr(10).join(per_map[:3]))


# ---------------------------------------------------------------------------
# E、R、S 三节动手的图
# ---------------------------------------------------------------------------

# 只在这几张图上动手。修地图本来就是挪门、挪落点、挪人、给人加旗标，正在返修的图
# 不适合拿来造样例：用例若恰好挑中一张画到一半的图，报出来的就不是本用例造的那件事。
# 挑哪道门、落点在哪、谁是没旗标的 npc，一律在这几张里从数据现找，不写死坐标。
#
# E 节早先写死了坐标（三叔站 (20,18)、韩家村南口在 (19,29)）。2026-09-27 修衔接时那道门
# 挪了、三叔带上了旗标，E1、E4 当场变红 —— 红的是用例的前提，不是门禁。
STABLE_MAPS = ("ch01_caixiashan", "ch01_qixuanmen", "ch05_dukou", "ch05_mofu")
STABLE_PREFIXES = ("ch04_",)


def stable_maps(work: Path) -> list[str]:
    return [path.stem for path in sorted((work / "maps").glob("*.tmj"))
            if path.stem in STABLE_MAPS or path.stem.startswith(STABLE_PREFIXES)]


def find_speaker(work: Path) -> tuple[str, dict]:
    """现找一个有脚本、一定在场（不带 visible_flag / hidden_flag）的 npc：(图, 解析出的对象)。
    这样的 npc 永远先应声，E 节拿它当「压住别人」和「被同类压住」的那一个。"""
    for map_id in stable_maps(work):
        parsed, _collision = parse_one(work, map_id)
        for record in parsed.objects:
            if (record["type"] == "npc" and record["props"].get("script")
                    and validate.npc_always_present(record)):
                return map_id, record
    raise AssertionError("%s 里找不到有脚本、不带旗标的 npc" % "、".join(stable_maps(work)))


def find_bare_portal(work: Path) -> tuple[str, dict]:
    """现找一道格子上没压任何触发器的传送点：(图, 解析出的对象)。
    E4 要往它身上盖一个 enter 触发，原本就压着触发器的门测不出「是新盖的那个压住了它」。"""
    for map_id in stable_maps(work):
        parsed, _collision = parse_one(work, map_id)
        triggered = {cell for record in parsed.objects if record["type"] == "trigger"
                     for cell in validate.object_cells(record)}
        for portal in parsed.portals:
            if not triggered & set(validate.object_cells(portal)):
                return map_id, portal
    raise AssertionError("%s 里找不到没压触发器的传送点" % "、".join(stable_maps(work)))


def selftest_e_interaction_shadow(case: Case, base: Path) -> None:
    """交互遮蔽：够得着不等于按得出来。"""
    print()
    print("E. 同格遮蔽（interact 的问询顺序 npc > interact 触发 > facility）")

    # 一个有脚本、没有 visible_flag/hidden_flag 的 npc，永远先应声。
    probe = make_workdir(base / "e0")
    host, speaker = find_speaker(probe)
    cell = (speaker["x"], speaker["y"])
    under = lambda: make_object("facility_selftest_under", "facility", cell, {"kind": "save"})

    work = make_workdir(base / "e1")
    payload = load_map(work, host)
    add_object(payload, under())
    save_map(work, host, payload)
    errors = [e for e in run_validate(work) if "facility_selftest_under" in e]
    case.check("有脚本的 npc（%s 的 %s）压住同格设施 → 报按不出来" % (host, speaker["name"]),
               bool(errors), chr(10).join(errors[:2]))

    # 对照：同一处，给 npc 加上 visible_flag —— 它会有不在场的时候，不算永久遮蔽。
    work = make_workdir(base / "e2")
    payload = load_map(work, host)
    set_prop(find_object(payload, speaker["name"]), "visible_flag", "ch01.sanshu_met")
    add_object(payload, under())
    save_map(work, host, payload)
    errors = [e for e in run_validate(work) if "facility_selftest_under" in e]
    case.check("npc 带 visible_flag → 不报（会让位，不是永久遮蔽）", not errors,
               chr(10).join(errors[:2]))

    # 同类之间：objectAt 只返回对象表里第一个命中的，哪怕它没脚本、本不该应声。
    work = make_workdir(base / "e3")
    payload = load_map(work, host)
    add_object(payload, make_object("npc_selftest_mute", "npc", cell,
                                    {"role_id": speaker["props"].get("role_id", ""),
                                     "facing": "down"}), first=True)
    save_map(work, host, payload)
    errors = [e for e in run_validate(work) if speaker["name"] in e and "按不出来" in e]
    case.check("对象表靠前的哑巴 npc 压住靠后的 %s → 报按不出来" % speaker["name"], bool(errors),
               chr(10).join(errors[:2]))

    # tryStep：enter 触发命中就 return，排在它后面的 portal 收不到这一步。
    door_map, door = find_bare_portal(probe)
    always_enter = lambda once: make_object(
        "trigger_selftest_enter", "trigger", (door["x"], door["y"]),
        {"script": speaker["props"]["script"], "mode": "enter", "once": once},
        size=(door["w"], door["h"]))

    def stepped_dead(work: Path) -> list[str]:
        return [e for e in run_validate(work)
                if door_map in e and door["name"] in e and "踏不响" in e]

    work = make_workdir(base / "e4")
    payload = load_map(work, door_map)
    add_object(payload, always_enter(False))
    save_map(work, door_map, payload)
    errors = stepped_dead(work)
    case.check("once=false 的 enter 触发压住同格传送点（%s 的 %s）→ 报踏不响"
               % (door_map, door["name"]), bool(errors), chr(10).join(errors[:2]))

    work = make_workdir(base / "e5")
    payload = load_map(work, door_map)
    add_object(payload, always_enter(True))
    save_map(work, door_map, payload)
    errors = stepped_dead(work)
    case.check("同一处改成 once=true → 不报（烧掉之后传送点照常）", not errors,
               chr(10).join(errors[:2]))


def gate(work: Path, map_id: str, obj_name: str, prop: str, value: str) -> None:
    """把某个对象的闸门旗标改成别的 —— 造死锁只需要动这一个字段。"""
    payload = load_map(work, map_id)
    set_prop(find_object(payload, obj_name), prop, value)
    save_map(work, map_id, payload)


def selftest_f_flag_order(case: Case, base: Path) -> None:
    """旗标顺序死锁：路是通的，钥匙锁在门后。"""
    print()
    print("F. 旗标顺序死锁（规则 19）")

    # F1 · require_flag 成环：出韩家村要一个只有青牛镇才拿得到的旗标。
    work = make_workdir(base / "f1")
    gate(work, "ch01_hanjiacun", "portal_to_qingniuzhen", "require_flag", "ch01.baoming_done")
    errors = run_validate(work)
    deadlock = [e for e in errors if "顺序死锁" in e]
    case.check("出村的钥匙锁在镇上 → 报顺序死锁", bool(deadlock),
               chr(10).join(deadlock[:2]) + (chr(10) + "……共 %d 张图" % len(deadlock)
                                             if len(deadlock) > 2 else ""))
    case.check("同时点名那道开不了的闸门",
               any("portal_to_qingniuzhen" in e and "永远置不上" in e for e in errors),
               chr(10).join(e for e in errors if "portal_to_qingniuzhen" in e))
    # 这条是两条规则的分界线：路本身没断，规则 18 看不出任何问题。
    case.check("规则 18（不看旗标的连通性）对它确实无话可说",
               not [e for e in errors if "没有任何走得到的传送点" in e],
               chr(10).join(e for e in errors if "没有任何走得到的传送点" in e)[:200])

    # F2 · guard_flag 成环：放榜（ch01.fangbang_done 的唯一来源）要等章末旗标，
    # 而章末在神手谷，进神手谷又要 ch01.fangbang_done。
    work = make_workdir(base / "f2")
    gate(work, "ch01_liangu_ya", "trigger_fangbang", "guard_flag", "ch01.done")
    errors = run_validate(work)
    case.check("guard_flag 成环 → 报触发器起不来",
               any("trigger_fangbang" in e and "永远置不上" in e for e in errors),
               chr(10).join(e for e in errors if "trigger_fangbang" in e))
    case.check("连带报出被它卡住的传送点",
               any("portal_to_shenshougu" in e and "永远置不上" in e for e in errors),
               chr(10).join(e for e in errors if "portal_to_shenshougu" in e))

    # F3 · visible_flag 自环：墨大夫要等自己那一幕演完才出现，而那一幕由他起。
    work = make_workdir(base / "f3")
    gate(work, "ch01_liangu_ya", "npc_mo_daifu", "visible_flag", "ch01.shoutu_done")
    errors = run_validate(work)
    case.check("visible_flag 自环 → 报 NPC 不出现",
               any("npc_mo_daifu" in e and "永远置不上" in e for e in errors),
               chr(10).join(e for e in errors if "npc_mo_daifu" in e))

    # F4 · 对照：把村口话别的 guard 改成第 2 章的旗标，看上去像死锁，其实不是 ——
    # 韩母站在旁边跑同一个脚本，ch01.muqin_bie 照样拿得到。一条「见环就报」的
    # 检查会在这里误报，那正是这条对照要拦的。
    work = make_workdir(base / "f4")
    gate(work, "ch01_hanjiacun", "trigger_cunkou_bie", "guard_flag", "ch02.dazuo_done")
    errors = run_validate(work)
    case.check("旗标还有第二条来源时不误报", not errors,
               chr(10).join(errors[:3]))


def patch_script(work: Path, relative: str, old: str, new: str) -> None:
    """在副本的某个脚本里做一次替换，并要求被替换的原文确实在。

    找不到原文就当场炸掉，而不是悄悄写回去：脚本被改写之后自检还「全绿」，
    等于这条用例从此不再检验任何东西。
    """
    path = work / relative
    source = path.read_text(encoding="utf-8")
    if old not in source:
        raise AssertionError("%s 里找不到 %r，这条自检要跟着脚本一起改" % (relative, old))
    path.write_text(source.replace(old, new, 1), encoding="utf-8")


def selftest_g_script_item_ids(case: Case, base: Path) -> None:
    """脚本里的物品 id：三种写法都要查，形近的不许误报。"""
    print()
    print("G. 脚本里的物品 id（give / take / bottle.mature）")

    # G0 · 对照：不动的副本在全量模式下必须干净，否则下面几条报的可能是别的事。
    errors = run_validate_full(make_full_workdir(base / "g0"))
    case.check("不动的副本 → 全量校验干净", not errors, chr(10).join(errors[:3]))

    # G1 · bottle.mature 用了不存在的药名。
    #     引擎侧的后果是安静的：背包里没有这一株，命令回填 no_item，脚本以为
    #     自己浇了三滴，玩家什么也没看见，而绿液一滴没少。
    work = make_full_workdir(base / "g1")
    patch_script(work, "scripts/ch02/cuishu.lua",
                 'bottle.mature("herb_huangjing_cao"', 'bottle.mature("herb_bu_cun_zai"')
    errors = run_validate_full(work)
    case.check("bottle.mature 的物品 id 不存在 → 报错",
               any("herb_bu_cun_zai" in e for e in errors), chr(10).join(errors[:3]))

    # G2 · give / take 那两条老路径同样要抓得住：改正则时最容易顺手碰坏的
    #     就是原来就在的那一半。
    work = make_full_workdir(base / "g2")
    patch_script(work, "scripts/ch02/suanzhang.lua",
                 'take("herb_huangjing_cao", 1, 44)', 'take("herb_ye_bu_cun_zai", 1, 44)')
    errors = run_validate_full(work)
    case.check("take 的物品 id 不存在 → 报错",
               any("herb_ye_bu_cun_zai" in e for e in errors), chr(10).join(errors[:3]))

    # G3 · 形近写法不许误报。两个探针各挡一种写法错：
    #       my_bottle.mature —— 前面那道 (?<![A-Za-z0-9_]) 负向回顾；
    #       bottleXmature    —— `bottle\.mature` 里那个点的转义。
    #     转义一旦被吃掉（本项目栽过四次），第二个探针会被当成真调用而报错。
    work = make_full_workdir(base / "g3")
    probe = (chr(10) + 'local my_bottle = { mature = function() end }' + chr(10) +
             'my_bottle.mature("herb_tan_zhen_yi", 1)' + chr(10) +
             'bottleXmature("herb_tan_zhen_er", 1)' + chr(10))
    with (work / "scripts" / "ch02" / "cuishu.lua").open("a", encoding="utf-8") as out:
        out.write(probe)
    errors = run_validate_full(work)
    case.check("形近写法不误报（回顾断言与点的转义都还在）",
               not any("herb_tan_zhen" in e for e in errors),
               chr(10).join(e for e in errors if "herb_tan_zhen" in e))


def selftest_k_hook_declarations(case: Case, base: Path) -> None:
    """脚本首部的 @hook 声明：与 tmj 对不上必须报，写对了必须闭嘴。

    钉的是第 3 章那条最要害的缺陷（独立校对 HIGH-1）：两种杀的触发方式在地图上
    装反了，而两个脚本的首部注释写的正好相反。四条测试对它满格敏感却一条也没
    发现——它们的驱动方式是照着 tmj 写出来的，从被测物推导出来的判据发现不了
    被测物与规格不一致。

    所以这一组刻意从**两侧**各推一次：改脚本要报，改地图也要报。
    只推一侧的话，一条「读脚本、从不打开 tmj」的假检查照样全绿。
    """
    print()
    print("K. 脚本首部的挂点声明（@hook）")

    hook_line = "-- @hook ch02_jusuo trigger_duoshe enter once"
    npc_line = "-- @hook ch03_mishi npc_mo_daifu npc"
    script = "scripts/ch03/duoshe.lua"

    # K0 · 对照：不动的副本必须干净。下面每一条报的才是自己那件事。
    errors = run_validate(make_workdir(base / "k0"))
    case.check("不动的副本 → 地图档校验干净", not errors, chr(10).join(errors[:3]))

    def hook_errors(errors: list[str], where: str = script) -> list[str]:
        return [e for e in errors if where in e and "@hook" in e]

    # K1 · 脚本说 interact，地图挂的是 enter。**这正是 HIGH-1 的形状。**
    work = make_workdir(base / "k1")
    patch_script(work, script, hook_line, "-- @hook ch02_jusuo trigger_duoshe interact once")
    errors = run_validate(work)
    case.check("声明的 mode 与 tmj 不符 → 报错",
               bool(hook_errors(errors)), chr(10).join(errors[:3]))

    # K2 · 图写错了（MEDIUM-6 里有四个脚本是这一种）。
    work = make_workdir(base / "k2")
    patch_script(work, script, hook_line, "-- @hook ch03_mishi trigger_duoshe enter once")
    errors = run_validate(work)
    case.check("声明的地图与 tmj 不符 → 报错",
               bool(hook_errors(errors)), chr(10).join(errors[:3]))

    # K3 · 对象名写错了：图对、mode 对，只有名字不存在。
    work = make_workdir(base / "k3")
    patch_script(work, script, hook_line, "-- @hook ch02_jusuo trigger_bu_cun_zai enter once")
    errors = run_validate(work)
    case.check("声明的对象名在图上不存在 → 报错",
               bool(hook_errors(errors)), chr(10).join(errors[:3]))

    # K4 · once 漏了。tmj 是 once=true，声明里不写就是在说 once=false。
    work = make_workdir(base / "k4")
    patch_script(work, script, hook_line, "-- @hook ch02_jusuo trigger_duoshe enter")
    errors = run_validate(work)
    case.check("声明漏了 once → 报错", bool(hook_errors(errors)), chr(10).join(errors[:3]))

    # K5 · 反方向：tmj 引用了这个脚本，脚本却一个字也不声明。
    #     少了这一条，删掉声明就能让整条检查闭嘴。
    work = make_workdir(base / "k5")
    patch_script(work, script, hook_line + chr(10), "")
    errors = run_validate(work)
    case.check("tmj 挂了它而脚本没声明 → 报错",
               bool(hook_errors(errors)), chr(10).join(errors[:3]))

    # K6 · npc 对象没有 once 属性，声明里不许写。
    work = make_workdir(base / "k6")
    patch_script(work, "scripts/ch03/mojuren.lua", npc_line, npc_line + " once")
    errors = run_validate(work)
    case.check("npc 声明里写了 once → 报错",
               bool(hook_errors(errors, "scripts/ch03/mojuren.lua")), chr(10).join(errors[:3]))

    # K7 · 位置：声明埋在正文中间等于没有，必须报。
    work = make_workdir(base / "k7")
    with (work / script).open("a", encoding="utf-8") as out:
        out.write(chr(10) + hook_line + chr(10))
    errors = run_validate(work)
    case.check("声明写在正文中间 → 报错",
               any("首部" in e for e in hook_errors(errors)), chr(10).join(errors[:3]))

    # K8 · 写坏的声明**不许**被当成普通注释悄悄放过。这是「找不到某个词就算过」
    #     那一类空转法的直接堵法：字段数不对时必须炸，而不是匹配不上就走开。
    work = make_workdir(base / "k8")
    patch_script(work, script, hook_line, "-- @hook ch02_jusuo trigger_duoshe")
    errors = run_validate(work)
    case.check("字段数不对的声明 → 报错，不是静默放过",
               bool(hook_errors(errors)), chr(10).join(errors[:3]))

    # K9 · **另一侧**：脚本一个字不动，改地图。这一条是本组的关键——
    #     它是唯一一条能把「只读脚本、从不打开 tmj」的假检查照出来的用例。
    work = make_workdir(base / "k9")
    payload = load_map(work, "ch02_jusuo")
    set_prop(find_object(payload, "trigger_duoshe"), "mode", "interact")
    save_map(work, "ch02_jusuo", payload)
    errors = run_validate(work)
    case.check("地图改了而脚本没改 → 报错（证明这条检查真的读了 tmj）",
               bool(hook_errors(errors)), chr(10).join(errors[:3]))

    # K10 · 形近写法不许误报。`@hookpoint` 若被当成声明，说明正则里 `@hook\b`
    #     的 `\b` 已经没了——本项目最惨的一次事故正是一个 `\b` 被 heredoc 吃掉，
    #     检查器从此永远报通过而无人发现。
    work = make_workdir(base / "k10")
    patch_script(work, script, hook_line,
                 hook_line + chr(10) + "-- 关卡侧把这个位置叫作 @hookpoint，不是一条声明")
    errors = run_validate(work)
    case.check("形近的 @hookpoint 不误报（正则里的 \\b 还在）",
               not hook_errors(errors), chr(10).join(hook_errors(errors)))

    # K11 · 改对了必须闭嘴：把两侧一起挪到另一个合法取值上。只验「写坏会报」而
    #     不验「写对不报」，一条恒报错的检查也能全绿，那样门禁就永远过不去。
    work = make_workdir(base / "k11")
    payload = load_map(work, "ch02_jusuo")
    set_prop(find_object(payload, "trigger_duoshe"), "mode", "interact")
    save_map(work, "ch02_jusuo", payload)
    patch_script(work, script, hook_line, "-- @hook ch02_jusuo trigger_duoshe interact once")
    errors = run_validate(work)
    case.check("两侧一起改成一致 → 闭嘴",
               not hook_errors(errors), chr(10).join(hook_errors(errors)))


def selftest_i_empty_category(case: Case, base: Path) -> None:
    """空类别：一条 id 都没定义的目录，曾经会把自己那道引用检查关掉。"""
    print()
    print("I. 空类别不得静默放行")

    # I0 · 对照：不动的副本必须干净，否则下面那条报的可能是别的事。
    errors = run_validate_full(make_full_workdir(base / "i0"))
    case.check("不动的副本 → 全量校验干净", not errors, chr(10).join(errors[:3]))

    # I1 · 把 data/magics/ 清空。角色身上那些 magics 引用一个也没变，
    #      旧实现会因为「这个类别一条都没有」把它们全放过去，
    #      然后在某一天有人加进第一个法术文件时一起变成错误。
    work = make_full_workdir(base / "i1")
    magics = work / "data" / "magics"
    removed = 0
    if magics.exists():
        for item in sorted(magics.rglob("*.json")):
            item.unlink()
            removed += 1
    case.check("夹具前提：确实删掉了法术文件", removed > 0,
               "删了 %d 个" % removed)

    errors = run_validate_full(work)
    named = [e for e in errors if "一条 id 都没定义" in e]
    case.check("清空 data/magics/ → 报「被引用却无人校验」",
               bool(named), chr(10).join(errors[:3]))

    # I2 · 不允许滴水不漏地淦没报告：每个空类别只该报一条。
    #      一条「见谁都报」的检查同样能让 I1 通过。
    case.check("每个空类别只报一条", len(named) == 1,
               "报了 %d 条" % len(named))

    # I3 · 反向：类别只是「少」不是「空」时必须闭嘴。
    #      修成「只要引用对不上就报」的实现会在这里红。
    work = make_full_workdir(base / "i3")
    magics = work / "data" / "magics"
    files = sorted(magics.rglob("*.json")) if magics.exists() else []
    for item in files[1:]:
        item.unlink()
    errors = run_validate_full(work)
    noisy = [e for e in errors if "一条 id 都没定义" in e]
    case.check("类别只剩一条时不再报空", not noisy, chr(10).join(noisy[:2]))

def selftest_b2_ch02_drift(case: Case, base: Path) -> None:
    """漂移门禁要盖到第 2 章。

    第 2 章那四张图同样由生成器产出、同样带闸门（ch02.renyao_done /
    ch02.dazuo_done / ch02.duan2_start），而门禁一度只盯第 1 章六张——
    同一个缺口，只是换了一章。
    """
    print()
    print("B2. 漂移门禁要盖到第 2 章的四张图")
    import genmaps

    work = make_workdir(base / "b2")
    saved = genmaps.MAPS, genmaps.TSETS
    genmaps.MAPS = str(work / "maps")
    genmaps.TSETS = str(work / "maps" / "tilesets")
    try:
        buffer = io.StringIO()
        with contextlib.redirect_stdout(buffer):
            code = genmaps.main(["--check"])
        text = buffer.getvalue()
        # 盯住的张数必须与生成器实际产出的张数一致。
        # 早先这里写死成「10 张图」，第 3 章一落地就红了——而门禁本身没有任何问题，
        # 只是图变多了。要问的是「有没有漏掉哪一张」，不是「是不是正好十张」。
        # 真源是 check() 汇总的那一份，不是本文件的 build_all()——后者只管第 1 章
        # 六张，第 2、3 章各归自己的 genmaps_chNN.py。这里照 check() 同样的口径
        # 汇总一次，所以再加一章时这条判据自己就跟上了，不必回来改数字。
        # 章节生成器**自己发现**，不写死成元组。
        # 上一版写的是 (genmaps_ch02, genmaps_ch03)，注释还写着「再加一章时
        # 这条判据自己就跟上」——可汇总那一侧是写死的，第 4 章一落地它就红了，
        # 而红的不是门禁、是这条判据自己没跟上。这正是它要防的那个毛病。
        import importlib, glob, os
        chapters = []
        here = os.path.dirname(os.path.abspath(genmaps.__file__))
        for path in sorted(glob.glob(os.path.join(here, "genmaps_ch*.py"))):
            chapters.append(importlib.import_module(
                os.path.splitext(os.path.basename(path))[0]))
        case.check("章节生成器发现得到（至少两章）", len(chapters) >= 2,
                   "只发现 %d 个" % len(chapters))
        owned_ids = dict(genmaps.build_all())
        for chapter in chapters:
            for map_id, payload in chapter.build_all().items():
                owned_ids.setdefault(map_id, payload)
        owned = len(owned_ids)
        reported = re.search(r"(\d+) 张图", text)
        case.check(
            "未改动的副本：生成器产出的每一张都在盯",
            code == 0 and reported is not None and int(reported.group(1)) == owned,
            text.strip()[:160])

        # 整道闸门一起抹掉：旧生成器重跑就是这个效果——它 require_flag 与
        # deny_text_key 两个都不产出。只抹一个会留下孤儿属性，那是规则 20 的活。
        payload = load_map(work, "ch02_yaopu")
        entry = find_object(payload, "portal_to_jusuo")
        drop_prop(entry, "require_flag")
        drop_prop(entry, "deny_text_key")
        save_map(work, "ch02_yaopu", payload)

        buffer = io.StringIO()
        with contextlib.redirect_stdout(buffer):
            code = genmaps.main(["--check"])
        text = buffer.getvalue()
        case.check("抹掉 ch02_yaopu 一整道闸门 → --check 报漂移",
                   code != 0 and "ch02_yaopu" in text,
                   chr(10).join(l for l in text.split(chr(10))
                                if "漂移" in l or "require_flag" in l).strip()[:300])

        # 对照，也是这道门禁存在的全部理由：同一处改动，validate.py 一个字都不说。
        # 一道闸门被整个抹掉之后，地图**依旧完全合规**——没有孤儿属性可查，
        # 连通性也没变，只是再没人拦着玩家跳过整段。这正是静默漂移的可怕之处。
        case.check("validate.py 对同一处改动确实无话可说", not run_validate(work),
                   chr(10).join(run_validate(work)[:3]))
    finally:
        genmaps.MAPS, genmaps.TSETS = saved


def selftest_h_deny_keys(case: Case, base: Path) -> None:
    """deny_text_key 的命名约定与配对（规则 20）。"""
    print()
    print("H. deny_text_key：与 require_flag 成对，且按目标图命名")

    # 退回到旧的「按源图命名」写法。第 1 章一度就是这么写的，所以这条用例
    # 同时是那次统一的回归闸门——改回去必须立刻被抓住。
    work = make_workdir(base / "h1")
    payload = load_map(work, "ch01_hanjiacun")
    set_prop(find_object(payload, "portal_to_qingniuzhen"),
             "deny_text_key", "ch01.block.hanjiacun")
    save_map(work, "ch01_hanjiacun", payload)
    errors = [e for e in run_validate(work) if "portal_to_qingniuzhen" in e]
    case.check("退回按源图命名 → 报不合约定并给出应有的名字",
               any("应为 ch01.block.qingniuzhen" in e for e in errors),
               chr(10).join(errors[:2]))

    # 拦住却不说明还差什么，等于没拦。
    work = make_workdir(base / "h2")
    payload = load_map(work, "ch01_hanjiacun")
    drop_prop(find_object(payload, "portal_to_qingniuzhen"), "deny_text_key")
    save_map(work, "ch01_hanjiacun", payload)
    errors = [e for e in run_validate(work) if "portal_to_qingniuzhen" in e]
    case.check("有 require_flag 却没 deny_text_key → 报错",
               any("没有 deny_text_key" in e for e in errors), chr(10).join(errors[:2]))

    # 反过来：没有 require_flag 的 deny_text_key 一辈子播不出来。
    work = make_workdir(base / "h3")
    payload = load_map(work, "ch01_hanjiacun")
    drop_prop(find_object(payload, "portal_to_qingniuzhen"), "require_flag")
    save_map(work, "ch01_hanjiacun", payload)
    errors = [e for e in run_validate(work) if "portal_to_qingniuzhen" in e]
    case.check("有 deny_text_key 却没 require_flag → 报错",
               any("没有 require_flag" in e for e in errors), chr(10).join(errors[:2]))

    # 对照：不带闸门的传送点（两者都没有）本来就合法，不许报。
    work = make_workdir(base / "h4")
    payload = load_map(work, "ch01_hanjiacun")
    entry = find_object(payload, "portal_to_qingniuzhen")
    drop_prop(entry, "require_flag")
    drop_prop(entry, "deny_text_key")
    save_map(work, "ch01_hanjiacun", payload)
    errors = [e for e in run_validate(work) if "portal_to_qingniuzhen" in e]
    case.check("两者都没有的普通传送点 → 不报", not errors, chr(10).join(errors[:2]))


def load_objectives(work: Path, chapter: str) -> dict:
    return json.loads((work / "data" / "objectives" / (chapter + ".json")).read_text("utf-8"))


def save_objectives(work: Path, chapter: str, payload: dict) -> None:
    (work / "data" / "objectives" / (chapter + ".json")).write_text(
        json.dumps(payload, ensure_ascii=False, indent=2), "utf-8")


def selftest_l_objectives(case: Case, base: Path) -> None:
    """目标链与地图名 key（规则 21、22）。

    这两条守的是屏幕上那一行「当前目标 · 前往某地」。它们错的方式都不会自己
    暴露：指到不存在的对象上，画面就是没有标记；完成旗标没人会置，那一步永远
    完不成，目标行从此卡死。两种在玩家眼里都是「目标系统没做」。
    """
    print()
    print("L. 目标链：指得到、走得完；地图名 key 推得出来")

    # 指到一个不存在的对象上。
    work = make_full_workdir(base / "l1")
    payload = load_objectives(work, "ch01")
    payload["steps"][0]["target_object"] = "npc_bu_cun_zai"
    save_objectives(work, "ch01", payload)
    errors = [e for e in run_validate_full(work) if "npc_bu_cun_zai" in e]
    case.check("target_object 指向不存在的对象 → 报错",
               any("没有名为" in e for e in errors), chr(10).join(errors[:2]))

    # 完成旗标没有任何脚本会置。story.xiuxian_known 正是这样一个：它在
    # data/flags.json 里登记着，全仓却一处也没有置过（技术债里记着这笔账）。
    work = make_full_workdir(base / "l2")
    payload = load_objectives(work, "ch01")
    payload["steps"][0]["done_flag"] = "story.xiuxian_known"
    save_objectives(work, "ch01", payload)
    errors = [e for e in run_validate_full(work) if "story.xiuxian_known" in e]
    case.check("done_flag 没有任何脚本会置 → 报错（这一步永远完不成）",
               any("没有任何脚本会置它" in e for e in errors), chr(10).join(errors[:2]))

    # 指到一张不存在的图上。
    work = make_full_workdir(base / "l3")
    payload = load_objectives(work, "ch01")
    payload["steps"][0]["target_map"] = "ch99_wuci_ditu"
    save_objectives(work, "ch01", payload)
    errors = [e for e in run_validate_full(work) if "ch99_wuci_ditu" in e]
    case.check("target_map 指向不存在的地图 → 报错",
               any("target_map 不存在" in e for e in errors), chr(10).join(errors[:2]))

    # 地图名 key 改成别的形状。引擎要按地图 id 推这个 key 才说得出「前往某地」，
    # 推出来查不到的话，屏幕上会出现一行 ch02.map.xxx.name。
    work = make_workdir(base / "l4")
    payload = load_map(work, "ch01_qingniuzhen")
    props = {p.get("name"): p for p in payload.get("properties", [])}
    props["display_name_key"]["value"] = "ch01.qingniuzhen.name"
    save_map(work, "ch01_qingniuzhen", payload)
    errors = [e for e in run_validate(work) if "display_name_key" in e]
    case.check("display_name_key 不合命名约定 → 报错并给出应有的名字",
               any("应为 ch01.map.qingniuzhen.name" in e for e in errors),
               chr(10).join(errors[:2]))


# ---------------------------------------------------------------------------
# M. 任务（规则 23，docs/interfaces-p3-ch05.md 1.8）
# ---------------------------------------------------------------------------

QUEST_PROBE_ID = "q_selftest_probe"


def good_quest() -> dict:
    """一份什么都写全了、引用全部真实存在的支线。每条负例只改其中一处。

    引用全挑第 4 章已冻结的东西：旗标 ch04.liuxia / ch04.kailu / ch04.done 都登记了、
    都有脚本置；落日峰上有 facility_danlu；文案 key 取第 4 章目标链那几条。
    正式的 data/quests/ 归内容路，这份只写进临时副本。
    """
    return {
        "id": QUEST_PROBE_ID,
        "name": "门禁自检探针",
        "chapter": 4,
        "kind": "side",
        "title_key": "objective.ch04.n1_liuxia",
        "summary_key": "objective.ch04.n2_huodan",
        "accept": [{"flag": "ch04.liuxia", "op": ">=", "value": 1}],
        "steps": [
            {"id": "s1", "text_key": "objective.ch04.n3_kailu",
             "done": [{"item": "pill_yangjing_dan", "op": ">=", "value": 1}],
             "target_map": "ch04_luorifeng", "target_object": "facility_danlu"},
            {"id": "s2", "text_key": "objective.ch04.n4_yufeng",
             "done": [{"flag": "ch04.kailu", "op": "==", "value": 2}]},
        ],
        "complete": [{"flag": "ch04.kailu", "op": ">=", "value": 1}],
        "fail": [{"flag": "ch04.done", "op": ">=", "value": 1}],
        "fail_text_key": "objective.ch04.n5_qiecuo",
        "rewards": [{"item_id": "herb_huangjing_cao", "count": 6, "herb_age": 40}],
        "origin": "自检",
        "note": "门禁自检用，不进正式数据",
    }


def write_quest(work: Path, payload, file_stem: str = QUEST_PROBE_ID) -> Path:
    quests = work / "data" / "quests"
    quests.mkdir(parents=True, exist_ok=True)
    for old in quests.glob(QUEST_PROBE_ID + "*.json"):
        old.unlink()
    for old in quests.glob("q_selftest_*.json"):
        old.unlink()
    path = quests / (file_stem + ".json")
    path.write_text(json.dumps(payload, ensure_ascii=False, indent=2), "utf-8")
    return path


def selftest_m_quests(case: Case, base: Path) -> None:
    """规则 23：任务文件每一条约束都要有一条负例，外加一条干净的对照。"""
    print()
    print("M. 任务：形状、成对、谓词三种写法、旗标有人置、引用存在、一章一条主线")

    work = make_full_workdir(base / "m")

    def errors_for(payload, file_stem: str = QUEST_PROBE_ID) -> list[str]:
        write_quest(work, payload, file_stem)
        return [e for e in run_validate_full(work) if "q_selftest_" in e]

    # M0 · 对照：好文件一条也不许报。少了这一条，下面每一条「报了」都可能只是因为底子就坏了。
    clean = errors_for(good_quest())
    case.check("写全了的支线 → 不报", not clean, chr(10).join(clean[:3]))

    def expect(name: str, mutate, needle: str, file_stem: str = QUEST_PROBE_ID) -> None:
        payload = good_quest()
        mutate(payload)
        found = errors_for(payload, file_stem)
        case.check(name, any(needle in e for e in found),
                   chr(10).join(found[:3]) or "（一条也没报）")

    def set_field(field, value):
        return lambda p: p.__setitem__(field, value)

    def drop_field(field):
        return lambda p: p.pop(field)

    def first_step(mutate):
        return lambda p: mutate(p["steps"][0])

    def complete_cond(mutate):
        return lambda p: mutate(p["complete"][0])

    expect("文件名与 id 不一致 → 报", lambda p: None, "文件名必须是", file_stem="q_selftest_other")
    expect("kind 写成 main → 报", set_field("kind", "main"), "kind 只许")
    expect("chapter 为 0 → 报", set_field("chapter", 0), "chapter 必须是")
    expect("chapter 写成字符串 → 报", set_field("chapter", "4"), "chapter 必须是")
    expect("缺 title_key → 报", drop_field("title_key"), "缺少 title_key")
    expect("title_key 指向不存在的文案 → 报", set_field("title_key", "ch99.quest.bu_cun_zai"),
           "文案 key 不存在：ch99.quest.bu_cun_zai")
    expect("accept 为空 → 报", set_field("accept", []), "accept 不许为空")
    expect("缺 complete → 报", drop_field("complete"), "缺少 complete")
    expect("steps 为空 → 报", set_field("steps", []), "steps 必须是非空数组")
    expect("步骤 done 为空 → 报", first_step(lambda s: s.__setitem__("done", [])), "done 不许为空")
    expect("步骤缺 text_key → 报", first_step(lambda s: s.pop("text_key")), "步骤缺少 text_key")
    expect("步骤 id 重复 → 报", lambda p: p["steps"][1].__setitem__("id", "s1"), "步骤 id 在本任务内重复")
    expect("target_map 落单 → 报", first_step(lambda s: s.pop("target_object")), "必须成对")
    expect("target_map 指向不存在的地图 → 报",
           first_step(lambda s: s.__setitem__("target_map", "ch99_wuci_ditu")), "target_map 不存在")
    expect("target_object 在那张图上不存在 → 报",
           first_step(lambda s: s.__setitem__("target_object", "npc_bu_cun_zai")), "没有名为 npc_bu_cun_zai")
    expect("有 fail 没原因 → 报", drop_field("fail_text_key"), "必须写 fail_text_key")
    expect("没 fail 有原因 → 报", drop_field("fail"), "fail 为空却写了 fail_text_key")
    expect("谓词 flag 与 item 都写 → 报", complete_cond(lambda c: c.__setitem__("item", "pill_yangjing_dan")),
           "恰好出现一个")
    expect("谓词 flag 与 item 都没写 → 报", complete_cond(lambda c: c.pop("flag")), "恰好出现一个")
    expect("op 写成 > → 报", complete_cond(lambda c: c.__setitem__("op", ">")), "op 只认")
    expect("物品谓词用 == → 报",
           first_step(lambda s: s["done"][0].__setitem__("op", "==")), "物品谓词只认")
    expect("旗标 >= 0 → 报", complete_cond(lambda c: c.__setitem__("value", 0)), "至少为 1")
    expect("value 写成 true → 报", complete_cond(lambda c: c.__setitem__("value", True)), "value 必须是整数")
    expect("旗标没登记 → 报", complete_cond(lambda c: c.__setitem__("flag", "ch99.bu_cun_zai")),
           "旗标未在 data/flags.json 登记")
    # story.xiuxian_known 登记着、全仓却一处也没有置过（L2 用的也是它）。
    expect("旗标登记了却没有任何脚本会置 → 报",
           complete_cond(lambda c: c.__setitem__("flag", "story.xiuxian_known")), "没有任何脚本会置它")
    expect("谓词里的物品不存在 → 报",
           first_step(lambda s: s["done"][0].__setitem__("item", "pill_bu_cun_zai")), "物品 id 不存在")
    expect("奖励的物品不存在 → 报",
           lambda p: p["rewards"][0].__setitem__("item_id", "herb_bu_cun_zai"), "item id 不存在")
    expect("奖励 count 为 0 → 报", lambda p: p["rewards"][0].__setitem__("count", 0), "count 必须是")
    expect("顶层字段拼错 → 报", lambda p: p.__setitem__("fail_text", p.pop("fail_text_key")), "不认识的字段")
    expect("步骤字段拼错 → 报",
           first_step(lambda s: s.__setitem__("target_obj", s.pop("target_object"))), "不认识的字段")
    expect("谓词字段拼错 → 报", complete_cond(lambda c: c.__setitem__("val", c.pop("value"))), "不认识的字段")

    # 一章一条主线：把第 1 章的目标链复制一份、换个 id，仍是第 1 章。
    work_dup = make_full_workdir(base / "m_dup")
    payload = load_objectives(work_dup, "ch01")
    payload["id"] = "objectives_ch01_dup"
    (work_dup / "data" / "objectives" / "ch01_dup.json").write_text(
        json.dumps(payload, ensure_ascii=False, indent=2), "utf-8")
    errors = [e for e in run_validate_full(work_dup) if "一章只许一条主线" in e]
    case.check("两个目标链文件同属一章 → 报", bool(errors), chr(10).join(errors[:2]) or "（一条也没报）")


# ---------------------------------------------------------------------------
# P. 路径行动（规则 26，docs/interfaces-octo-pathactions.md 第 4 节）
# ---------------------------------------------------------------------------

# 探针条目的时段用 ch04.bushu == N 彼此隔开（== 不同的值互相矛盾），于是几十条探针挂在
# 同一个 NPC 上也不会被「时段重叠」那一条误报——重叠检查另有两对专门的探针。
PATH_PROBE_WHEN = "ch04.bushu"


def path_probe(number: int, kind: str) -> dict:
    """一条什么都写全了、引用全部真实存在的探针条目（第 4 章）。每条负例只改其中一处。"""
    word = {"inquire": "dating", "purchase": "qiugou", "challenge": "qiecuo"}[kind]
    entry_id = "p%02d_%s" % (number, word)
    entry = {
        "id": entry_id,
        "kind": kind,
        "map": "ch04_getang",
        "npc": {"inquire": "npc_wang_juechu", "purchase": "npc_li_feiyu"}.get(kind, "npc_zhen_min_yaofan"),
        "when": [{"flag": PATH_PROBE_WHEN, "op": "==", "value": number}],
        "until": [{"flag": "ch04.done", "op": ">=", "value": 1}],
        "done_flag": "ch04.path." + entry_id,
        "text_key": "ch04.npc.feiyu.war_talk",
    }
    if kind == "inquire":
        entry["reveal"] = [{"role": "yelangbang_mazei", "category": "剑"}]
        entry["give"] = [{"item_id": "herb_huangjing_cao", "count": 1}]
    elif kind == "purchase":
        entry.update({"realm": 5, "refuse_key": "ch04.npc.wang_juechu.pre", "item_id": "pill_jinchuang_yao",
                      "count": 1, "price": 5, "deal_key": "ch04.npc.feiyu.war_dead",
                      "poor_key": "ch04.npc.feiyu.war_now"})
    else:
        entry.update({"map": "ch04_shanxiazhen", "battle": "b04_selftest_probe",
                      "win_key": "ch04.npc.feiyu.war_dead", "lose_key": "ch04.npc.feiyu.war_now",
                      "reward": {"cultivation": 10}})
    return entry


def path_probe_battle(battle_id: str, fatal: bool, cultivation: int) -> dict:
    # 横版格式（规则 24）：没有 width / height / player_spawn / 坐标——战斗合回之前这里
    # 写的是战棋格式，合回后被规则 24 当作格子遗留字段拦下，对照组因此误报。
    return {"id": battle_id, "name": "门禁自检探针", "chapter": 4, "terrain": "battlefield",
            "can_escape": True, "defeat_is_fatal": fatal,
            "units": [{"role_id": "li_feiyu", "faction": "enemy"}],
            "rewards": {"cultivation": cultivation, "spirit_stones": 0, "drops": []}}


# 每条负例：(探针号, 种类, 改法, 该报出来的那句话里的字)。改法只动这一条探针的一处。
def _set(field, value):
    return lambda e: e.__setitem__(field, value)


def _drop(field):
    return lambda e: e.pop(field)


PATH_NEGATIVES = [
    (10, "inquire", "条目字段拼错", _set("colour", 1), "不认识的字段"),
    (11, "inquire", "kind 写成表外的词", _set("kind", "ask"), "kind 只认"),
    (13, "inquire", "done_flag 与 id 对不上", _set("done_flag", "ch04.path.p99_dating"), "done_flag 必须是"),
    (14, "inquire", "when 为空", _set("when", []), "when 不许为空"),
    (15, "inquire", "until 漏写本章收尾", _set("until", [{"flag": "ch04.kaizhan", "op": ">=", "value": 1}]),
     "until 里必须有"),
    (17, "inquire", "realm 不是境界编号", _set("realm", 15), "realm 必须是"),
    (18, "inquire", "有门槛没回绝", _set("realm", 5), "必须写 refuse_key"),
    (19, "inquire", "没门槛却写回绝", _set("refuse_key", "ch04.npc.wang_juechu.pre"), "永远说不出口"),
    (20, "inquire", "文案 key 不存在", _set("text_key", "ch99.bu_cun_zai"), "文案 key 不存在"),
    (21, "inquire", "地图不存在", _set("map", "ch99_wu"), "map 不存在"),
    (22, "inquire", "那张图上没有这个 npc", _set("npc", "npc_bu_cun_zai"), "没有名为"),
    (23, "inquire", "谓词旗标没登记",
     lambda e: e["when"].append({"flag": "ch99.bu_cun_zai", "op": ">=", "value": 1}), "旗标未在 data/flags.json 登记"),
    (24, "inquire", "谓词旗标登记了却没人置",
     lambda e: e["when"].append({"flag": "story.xiuxian_known", "op": ">=", "value": 1}), "没有任何脚本会置它"),
    (25, "inquire", "谓词 op 写成 >", lambda e: e["when"][0].__setitem__("op", ">"), "op 只认"),
    (26, "inquire", "破绽类别是表外的词", lambda e: e["reveal"][0].__setitem__("category", "laser"), "不是十种攻击类别"),
    (27, "inquire", "揭破绽的角色不上阵", lambda e: e["reveal"][0].__setitem__("role", "han_mu"), "不在任何一场编成里"),
    (28, "inquire", "给的物品不存在", lambda e: e["give"][0].__setitem__("item_id", "herb_bu_cun_zai"), "物品 id 不存在"),
    (29, "inquire", "打探去置剧情旗标", _set("set_flags", ["ch04.kaizhan"]), "只许写本章的路径行动旗标"),
    (30, "inquire", "置的旗标没人读", _set("set_flags", ["ch04.path.selftest_nobody"]), "没有任何条目的 when / until 读它"),
    (31, "purchase", "求购白送", _set("price", 0), "price 必须是"),
    (32, "purchase", "求购漏了钱不够那一句", _drop("poor_key"), "缺少 poor_key"),
    (33, "challenge", "切磋的编成不存在", _set("battle", "b04_bu_cun_zai"), "编成不存在"),
    (34, "challenge", "编成建好了还挂着 pending", _set("pending", True), "去掉 pending"),
    (35, "challenge", "切磋输了会 game over", _set("battle", "b04_selftest_fatal"), "输了会 game over"),
    (36, "challenge", "切磋的编成自己发奖励", _set("battle", "b04_selftest_rich"), "自己带了 rewards"),
    (37, "challenge", "切磋赢了什么也没有", _set("reward", {}), "至少给一样"),
    (43, "inquire", "set_flags 里同一面旗写两遍",
     _set("set_flags", ["ch04.path.selftest_zhi", "ch04.path.selftest_zhi"]), "与前一项重复"),
]


def write_path_probe(work: Path, entries: list, extra_flags: list, file_stem: str = "ch04",
                     top_id: str = "pathactions_ch04") -> None:
    """把探针写成副本里的第 4 章文件（原文件移走），探针的旗标登记进副本的 flags.json。"""
    actions = work / "data" / "pathactions"
    for old in actions.glob("*.json"):
        if old.stem in ("ch04", "ch09"):
            old.unlink()
    (actions / (file_stem + ".json")).write_text(json.dumps(
        {"id": top_id, "name": "门禁自检探针", "chapter": 4, "entries": entries},
        ensure_ascii=False, indent=2), "utf-8")
    flags_path = work / "data" / "flags.json"
    original = json.loads((ROOT / "data" / "flags.json").read_text(encoding="utf-8"))
    flags = {k: v for k, v in original.items() if not k.startswith("ch04.path.")}
    for entry in entries:
        flags[entry["done_flag"]] = "自检探针"
    for name in extra_flags:
        flags[name] = "自检探针"
    flags_path.write_text(json.dumps(flags, ensure_ascii=False, indent=2) + chr(10), "utf-8")


def run_validate_lines(work: Path) -> list[str]:
    """全量跑一遍，[error] 与 [warn] 都要：pending 那一条该是警告、不该是错误。"""
    saved_root, saved_argv = validate.ROOT, sys.argv
    validate.ROOT = work
    sys.argv = ["validate.py"]
    buffer = io.StringIO()
    try:
        with contextlib.redirect_stdout(buffer):
            validate.main()
    finally:
        validate.ROOT, sys.argv = saved_root, saved_argv
    return [line for line in buffer.getvalue().split(chr(10)) if line.startswith(("[error]", "[warn]"))]


def path_clean_probes() -> list:
    """全部写对的探针：每条负例一条、两对时段探针、一条在场探针、一条 pending，外加一对「置旗标→读旗标」。"""
    probes = [path_probe(number, kind) for number, kind, _name, _mutate, _needle in PATH_NEGATIVES]
    chain = path_probe(2, "inquire")
    chain["set_flags"] = ["ch04.path.selftest_zhi"]
    reader = path_probe(3, "purchase")
    reader["when"].append({"flag": "ch04.path.selftest_zhi", "op": ">=", "value": 1})
    probes += [chain, reader]
    # 重叠：同一个 NPC、同为打探，后一条的 when（开战）推不出前一条的任何 until。
    overlap_a, overlap_b = path_probe(38, "inquire"), path_probe(39, "inquire")
    for probe in (overlap_a, overlap_b):
        probe["npc"] = "npc_li_feiyu"
    overlap_a["when"] = [{"flag": "ch04.liuxia", "op": ">=", "value": 1}]
    overlap_b["when"] = [{"flag": "ch04.kaizhan", "op": ">=", "value": 1}]
    # 对照：同样两段时段，前一条的 until 写了开战，就证明得了不重叠。
    fine_a, fine_b = path_probe(40, "inquire"), path_probe(41, "inquire")
    for probe in (fine_a, fine_b):
        probe["map"], probe["npc"] = "ch02_wairentang", "npc_tongmen_luchun"
    fine_a["when"] = [{"flag": "ch04.liuxia", "op": ">=", "value": 1}]
    fine_a["until"].insert(0, {"flag": "ch04.kaizhan", "op": ">=", "value": 1})
    fine_b["when"] = [{"flag": "ch04.kaizhan", "op": ">=", "value": 1}]
    # 在场：外刃堂那个厉飞雨第 2 章人情结下才在、第 3 章墨大夫归谷就下场，第 4 章的条目挂不上他。
    absent = path_probe(42, "inquire")
    absent["map"], absent["npc"] = "ch02_wairentang", "npc_li_feiyu"
    pending = path_probe(50, "challenge")
    pending.update({"map": "ch04_getang", "npc": "npc_wang_juechu", "battle": "b04_selftest_pending",
                    "pending": True})
    return probes + [overlap_a, overlap_b, fine_a, fine_b, absent, pending]


def selftest_p_path_actions(case: Case, base: Path) -> None:
    """规则 26：路径行动的每一条约束都有一条负例，外加干净的对照。

    探针挂在第 4 章的图上、引用全部真实存在；副本里第 4 章的正式文件换成探针文件。几十条负例
    放进同一趟校验：每条只改自己那一条探针的一处，报错里带着探针 id，按 id 认领——一趟就能判完，
    不必每条负例各跑一遍全量校验（本文件已经很慢了）。文件层面的三条（文件名、顶层 id、条目 id
    重复）各跑一趟。
    """
    print()
    print("P. 路径行动：形状、引用、谓词、时段不重叠、在场、pending、孤儿旗标、脚本不碰")

    work = make_full_workdir(base / "p")
    battles = work / "data" / "battles"
    for battle_id, fatal, cultivation in (("b04_selftest_probe", False, 0), ("b04_selftest_fatal", True, 0),
                                          ("b04_selftest_rich", False, 10)):
        (battles / (battle_id + ".json")).write_text(
            json.dumps(path_probe_battle(battle_id, fatal, cultivation), ensure_ascii=False, indent=2), "utf-8")

    def mine(lines: list, entry_id: str) -> list:
        return [line for line in lines if entry_id in line]

    # P0 · 对照：全部写对的探针一条也不许报错；pending 那一条只许是警告。
    clean_probes = path_clean_probes()
    write_path_probe(work, clean_probes, ["ch04.path.selftest_zhi"])
    clean = run_validate_lines(work)
    errors = [line for line in clean if line.startswith("[error]") and ("pathactions" in line or "selftest" in line)]
    mismatched = [line for line in errors if "时段证明不了不重叠" not in line and "才在场" not in line
                  and "下场" not in line]
    case.check("写全了的探针 → 不报（除了专门写来重叠与不在场的那几条）", not mismatched,
               chr(10).join(mismatched[:4]))
    case.check("时段证明得了不重叠的一对 → 不报", not mine(errors, "p40_dating") and not mine(errors, "p41_dating"),
               chr(10).join(mine(errors, "p40_dating")[:2]))
    case.check("pending 的切磋 → 只是警告、不是错误",
               not mine(errors, "p50_qiecuo") and any("p50_qiecuo" in line and line.startswith("[warn]")
                                                      for line in clean),
               chr(10).join(mine(clean, "p50_qiecuo")[:2]))
    case.check("同一 NPC 两条打探、时段推不出不重叠 → 报",
               any("时段证明不了不重叠" in line for line in mine(errors, "p38_dating")),
               chr(10).join(mine(errors, "p38_dating")[:2]) or "（一条也没报）")
    case.check("条目挂在一个那时不在场的 NPC 身上 → 报",
               any("才在场" in line for line in mine(errors, "p42_dating")),
               chr(10).join(mine(errors, "p42_dating")[:2]) or "（一条也没报）")

    # P1 · 负例：一趟里每条探针各坏一处，再加孤儿旗标与一个去置路径行动旗标的剧情脚本。
    broken = []
    for probe in clean_probes:
        match = [n for n in PATH_NEGATIVES if "p%02d_" % n[0] in probe["id"]]
        if match:
            probe = json.loads(json.dumps(probe))
            match[0][3](probe)
        broken.append(probe)
    write_path_probe(work, broken, ["ch04.path.selftest_zhi", "ch04.path.selftest_nobody",
                                    "ch04.path.selftest_orphan"])
    patch_script(work, "scripts/ch04/feiyu.lua", "talk(", 'flag.set("ch04.path.selftest_script")' + chr(10) + "talk(")
    # 章节公理的前提（validate.check_path_axiom_premise）：第 4 章的脚本置第 5 章的旗标、公共脚本置带章号的旗标。
    patch_script(work, "scripts/ch04/feiyu.lua", "talk(", 'flag.set("ch05.kaipian")' + chr(10) + "talk(")
    (work / "scripts" / "common" / "selftest_probe.lua").write_text('flag.set("ch03.done")' + chr(10), "utf-8")
    lines = run_validate_lines(work)
    for number, kind, name, _mutate, needle in PATH_NEGATIVES:
        entry_id = "p%02d_%s" % (number, {"inquire": "dating", "purchase": "qiugou", "challenge": "qiecuo"}[kind])
        found = mine(lines, entry_id)
        case.check(name + " → 报", any(needle in line for line in found), chr(10).join(found[:2]) or "（一条也没报）")
    found = [line for line in lines if "ch04.path.selftest_orphan" in line]
    case.check("登记表里有个没人用的路径行动旗标 → 报", any("登记了，却不是" in line for line in found),
               chr(10).join(found[:2]) or "（一条也没报）")
    found = [line for line in lines if "ch04.path.selftest_script" in line]
    case.check("剧情脚本去置路径行动的旗标 → 报", any("剧情脚本置了路径行动的旗标" in line for line in found),
               chr(10).join(found[:2]) or "（一条也没报）")
    found = [line for line in lines if "feiyu.lua" in line and "ch05.kaipian" in line]
    case.check("第 4 章的脚本置第 5 章的旗标 → 报", any("置了第 5 章的旗标" in line for line in found),
               chr(10).join(found[:2]) or "（一条也没报）")
    found = [line for line in lines if "selftest_probe.lua" in line]
    case.check("不在任何一章目录里的脚本置带章号的旗标 → 报", any("不在任何一章的目录里" in line for line in found),
               chr(10).join(found[:2]) or "（一条也没报）")

    # P2 · 文件层面：各自一趟，探针内容本身是对的。
    fine = [path_probe(10, "inquire")]
    file_cases = [
        ("文件名不是 chNN（与 chapter 对不上）→ 报", dict(file_stem="ch09"), "一章一个文件", "ch09.json"),
        ("顶层 id 不是 pathactions_chNN → 报", dict(top_id="pathactions_ch05"), "顶层 id 必须是", "ch04.json"),
    ]
    work_file = make_full_workdir(base / "p_file")
    for name, options, needle, marker in file_cases:
        write_path_probe(work_file, fine, [], **options)
        found = [line for line in run_validate_lines(work_file) if marker in line]
        case.check(name, any(needle in line for line in found), chr(10).join(found[:2]) or "（一条也没报）")
    write_path_probe(work_file, [path_probe(10, "inquire"), path_probe(10, "inquire")], [])
    found = [line for line in run_validate_lines(work_file) if "ch04.json" in line]
    case.check("同一章两条条目 id 重复 → 报", any("条目 id 在本章内重复" in line for line in found),
               chr(10).join(found[:2]) or "（一条也没报）")

# ---------------------------------------------------------------------------
# N. 破势与蓄劲（规则 24、25，docs/octopath-battle.md 第 5、7 节）
# ---------------------------------------------------------------------------

# 规则 24 / 25 报错里一定会出现的词，拿来从全量输出里筛出「这两条说的话」。
# 每一条报错都至少命中其中一个——新加报错时对一遍：第一版漏了「weaknesses」，于是
# 「类别名拼错」「写了两遍」那两条报是报了，却被这张表筛掉，自检当场红了两条。
BREAK_RULE_WORDS = ("破绽", "架势", "已作废", "手段", "weaknesses", "weapons", "charge", "boost",
                    "weapon", "actions", "toughness", "killable_by")


def selftest_n_battle_break(case: Case, base: Path) -> None:
    """规则 24（字段形状 ＋ 格子遗留字段）与规则 25（第 1–5 章破绽打得到）。

    一份全量副本，逐条改坏一处、跑一遍、原样改回：每一条之间互不沾染，
    而又不必为每一条复制一整份仓库。"""
    print()
    print("N. 破势与蓄劲：类别名、架势、行动次数、蓄势、遗留格子字段、破绽打不打得到")

    work = make_full_workdir(base / "n")

    def rule_errors() -> list[str]:
        # 规则 24 / 25 自己的话，外加「文案 key 不存在」（蓄势预告句的 key 走的是全仓那一条）。
        return [e for e in run_validate_full(work)
                if any(w in e for w in BREAK_RULE_WORDS) or "文案 key 不存在" in e]

    # N0 · 对照：不动的副本一条也不许报。少了这一条，下面每一条「报了」都可能只是因为底子就坏了。
    clean = rule_errors()
    case.check("不动的副本 → 规则 24 / 25 一条不报", not clean, chr(10).join(clean[:3]))

    def mutated(relative: str, mutate) -> list[str]:
        path = work / relative
        original = path.read_bytes()
        payload = json.loads(original.decode("utf-8"))
        mutate(payload)
        path.write_text(json.dumps(payload, ensure_ascii=False, indent=2), "utf-8")
        try:
            return rule_errors()
        finally:
            path.write_bytes(original)

    def expect(name: str, relative: str, mutate, needle: str, where: str = "") -> None:
        found = mutated(relative, mutate)
        hit = any(needle in e and (not where or where in e) for e in found)
        case.check(name, hit, chr(10).join(found[:3]) or "（一条也没报）")

    def expect_quiet(name: str, relative: str, mutate, where: str) -> None:
        found = [e for e in mutated(relative, mutate) if where in e]
        case.check(name, not found, chr(10).join(found[:3]))

    wolf = "data/roles/wild_wolf.json"
    # ---- 规则 24：角色 ----
    expect("破绽写了认不出的类别名 → 报", wolf, lambda p: p.__setitem__("weaknesses", ["剑剑"]), "不是可用的类别")
    expect("同一样破绽写两遍 → 报", wolf, lambda p: p.__setitem__("weaknesses", ["剑", "剑"]), "写了两遍")
    expect("架势写 0 → 报", wolf, lambda p: p.__setitem__("toughness", 0), "toughness 必须是")
    expect("有架势却没有破绽 → 报", wolf, lambda p: p.pop("weaknesses"), "有架势却一样破绽都没有")
    expect("行动次数写 0 → 报", wolf, lambda p: p.__setitem__("actions", 0), "actions 必须是")
    expect("兵刃写成五行 → 报", "data/roles/qu_hun.json", lambda p: p.__setitem__("weapons", ["火"]),
           "weapons 里的")
    expect("蓄势每 1 回合一次 → 报", "data/roles/jia_tianlong.json",
           lambda p: p["charge"].__setitem__("every", 1), "charge.every")
    # 只有这几类要得了他的命（欧阳飞天的 killable_by）：空数组 = 打不死，写错名字同样要拦。
    expect("要命的类别写成空数组 → 报", "data/roles/ouyang_feitian.json",
           lambda p: p.__setitem__("killable_by", []), "killable_by 不能是空数组")
    expect("要命的类别写了认不出的名字 → 报", "data/roles/ouyang_feitian.json",
           lambda p: p.__setitem__("killable_by", ["金子"]), "killable_by 里的")
    expect("蓄势不写预告句 → 报", "data/roles/jia_tianlong.json",
           lambda p: p["charge"].pop("text_key"), "charge.text_key 必须写")
    # 预告句的 key 走的是全仓那条「_key 字段必须指得到文案」（规则 6 那一族）：
    # 这一条问的是 charge 这一层嵌套有没有被那条检查照顾到。
    expect("蓄势预告句的 key 不存在 → 报", "data/roles/jia_tianlong.json",
           lambda p: p["charge"].__setitem__("text_key", "ch04.battle.bu_cun_zai"), "文案 key 不存在")
    expect("上场的敌人没有架势与破绽 → 报", "data/roles/long_zhong_shou.json",
           lambda p: (p.pop("toughness"), p.pop("weaknesses")), "没有架势或破绽", "b03_andao_shishou")

    # ---- 规则 24：法术、物品、编成 ----
    expect("法术 boost 拼错 → 报", "data/magics/huodan_shu.json",
           lambda p: p.__setitem__("boost", "hit"), "boost 只许")
    expect("法术还写着 castRange → 报", "data/magics/huodan_shu.json",
           lambda p: p.__setitem__("castRange", 4), "castRange 已作废")
    expect("兵器给的类别写成五行 → 报", "data/items/weapons/yudai_duanjian.json",
           lambda p: p.__setitem__("weapon", "火"), "weapon 只许兵刃类别")
    expect("物品还写着 useRange → 报", "data/items/pills/shixin_san.json",
           lambda p: p.__setitem__("useRange", 3), "useRange 已作废")
    expect("编成还写着战场宽度 → 报", "data/battles/b03_gu_wai_elang.json",
           lambda p: p.__setitem__("width", 14), "width 已作废")
    expect("编成还写着单位坐标 → 报", "data/battles/b03_gu_wai_elang.json",
           lambda p: p["units"][0].__setitem__("x", 10), "units[0] 的 x 已作废")
    # 识海那一场没有架势这回事：两团光球没写 toughness，不许报。
    case.check("识海那一场的敌人没有架势 → 不报",
               not [e for e in clean if "b03_shihai_duoshe" in e], "（见 N0）")

    # ---- 规则 25：破绽打不打得到 ----
    # 谷外那一仗韩立手里只有石头（拳）：狼的破绽里拿掉拳，第 3 章那一场就打不出破绽；
    # 第 5 章野宿那一场还有火弹，不该跟着报——这一对问的是「按这一场算」而不是「按角色算」。
    no_fist = lambda p: p.__setitem__("weaknesses", ["剑", "火"])  # noqa: E731
    expect("第 3 章的狼没有拳这一样破绽 → 谷外那一场报", wolf, no_fist, "没有一样是这一场", "b03_gu_wai_elang")
    expect_quiet("同一只狼在第 5 章 → 有火弹，不报", wolf, no_fist, "b05_yesu_yelang")
    expect("僵兽只剩火 → 暗道那一场报（第 3 章还没有火弹）", "data/roles/jiang_shou.json",
           lambda p: p.__setitem__("weaknesses", ["火"]), "没有一样是这一场", "b03_andao_shishou")
    expect_quiet("僵兽只剩毒 → 不报（蚀心散在暗道之前必给）", "data/roles/jiang_shou.json",
                 lambda p: p.__setitem__("weaknesses", ["毒"]), "b03_andao_shishou")
    # 韩立不在场的那一仗，手段只有编成里友军的兵刃：第 5 章的通表里有火，这里却不算。
    expect("夺帮那一夜的头目只怕火与刀 → 报（我方只有拳）", "data/roles/sipingbang_toumu.json",
           lambda p: p.__setitem__("weaknesses", ["火", "刀"]), "没有一样是这一场", "b05_duobang")
    expect_quiet("第 6 章的敌人不在规则 25 的范围 → 不报", "data/roles/jiexiu.json",
                 lambda p: p.__setitem__("weaknesses", ["木"]), "b06_shengxianling_jiesha")
    expect("编成落在没登记手段的章 → 报", "data/battles/b03_gu_wai_elang.json",
           lambda p: p.__setitem__("chapter", 2), "没有登记我方必有手段")

    # ---- 规则 25 的那张表自己站不站得住 ----
    expect("火弹术的五行被改掉 → 报（它打不出「火」）", "data/magics/huodan_shu.json",
           lambda p: p.__setitem__("element", 4), "五行不是「火」")
    expect("蚀心散不带毒了 → 报", "data/items/pills/shixin_san.json",
           lambda p: (p.pop("poison"), p.pop("poisonPower")), "不带毒")
    script = work / "scripts" / "ch04" / "huodan.lua"
    original = script.read_bytes()
    patch_script(work, "scripts/ch04/huodan.lua", 'magic.learn("magic_huodan_shu")', 'magic.learn("magic_bu_cun_zai")')
    try:
        found = rule_errors()
    finally:
        script.write_bytes(original)
    case.check("第 4 章的脚本不再教火弹术 → 报（火就不是必有的手段）",
               any("没有 magic.learn" in e for e in found), chr(10).join(found[:3]) or "（一条也没报）")


def parse_one(work: Path, map_id: str):
    """在副本上跑一遍 validate 自己的解析，拿回 ParsedMap 与 collision 层。

    用校验器自己的解析而不是另写一份：这一条自检要问的正是「校验器眼里的地形」，
    照着它的口径重写一遍，重写错了就永远问不出来。
    """
    saved_root = validate.ROOT
    validate.ROOT = work
    try:
        report = validate.Report()
        parsed = validate.check_map(work / "maps" / (map_id + ".tmj"), report)
        assert parsed is not None, "副本里的 %s 解析不出来" % map_id
        collision = validate.collision_layer(parsed, report)
        assert collision is not None
        return parsed, collision
    finally:
        validate.ROOT = saved_root


def free_open_cell(work: Path, map_id: str) -> tuple[int, int]:
    """找一格：走得到、没有任何对象压着、且四邻里至少三格也走得通。

    三个条件各有各的用处 —— 走得到才进得了规则 13 的视野；没有对象压着，
    报出来的遮蔽才确实是本用例摆上去的那两个造成的；四邻宽敞则基本排除了
    「这一格本身是唯一走廊」，否则把 npc 摆上去会顺带报一串不相干的断路。

    **不写死坐标。** 第 2 章复审点过「钉内容的形状是债」：地图一改，写死的格子
    会悄悄变成一格墙，而用例照样绿着 —— 它测的东西没了，输出却一模一样。
    """
    parsed, collision = parse_one(work, map_id)
    occupied = validate.occupied_cells(parsed)
    taken = {cell for record in parsed.objects for cell in validate.object_cells(record)}
    defaults = [s for s in parsed.spawns.values()
                if str(s["props"].get("default", "")).lower() == "true"]
    assert defaults, "%s 没有 default spawn" % map_id
    start = (defaults[0]["x"], defaults[0]["y"])
    seen = validate.flood(parsed, collision, occupied, start)

    for cell in sorted(seen):
        if cell in taken or cell == start:
            continue
        neighbours = sum(1 for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))
                         if (cell[0] + dx, cell[1] + dy) in seen)
        if neighbours >= 3:
            return cell
    raise AssertionError("%s 上找不到一格空旷的可达格" % map_id)


def npc_object(name: str, cell: tuple[int, int], props: dict,
               size: tuple[int, int] = (1, 1)) -> dict:
    base = {"role_id": "tongmen_luchun", "facing": "down"}
    base.update(props)
    return make_object(name, "npc", cell, base, size)


def selftest_j_npc_visibility(case: Case, base: Path) -> None:
    """NPC 的可见性：不在场的那个既不占格，也不抢话（引擎 2026-09-21 改）。

    这一节盯的是**校验器与引擎的口径有没有一起走**。引擎从前占格与抢话都走
    `TileMap::objectAt`，它不问可见性，所以校验器按「一律占格、一律抢先」算是对的。
    现在 `WorldScene` 两处都改走 `visibleNpcAt`，校验器若不跟着改，绿灯就开始
    描述一个不存在的游戏 —— 那比任何一边单独错都糟。

    四条成两对，每对都是「带旗标 → 不许报」配「同样的东西去掉旗标 → 必须报」。
    只写前半条的话，把整条检查注释掉也能全绿。
    """
    print()
    print("J. NPC 可见性（不在场的 npc 不占格、不抢话）")

    # ---- J1/J2：同格两个 npc、各带互补旗标（一个格子挂两段剧情的唯一办法）----
    shared_map = "ch01_hanjiacun"
    probe = make_workdir(base / "j0")
    cell = free_open_cell(probe, shared_map)
    # 互补：同一个旗标，一个置位前在场，一个置位后在场。任何时刻恰好一个在。
    switch = "ch01.sanshu_met"
    pair = (("npc_selftest_ch1", {"script": "ch01/sanshu.lua", "hidden_flag": switch}),
            ("npc_selftest_ch2", {"script": "ch01/sanshu.lua", "visible_flag": switch}))

    def build(work: Path, flagged: bool) -> None:
        payload = load_map(work, shared_map)
        for name, props in pair:
            kept = dict(props) if flagged else {"script": props["script"]}
            add_object(payload, npc_object(name, cell, kept))
        save_map(work, shared_map, payload)

    work = make_workdir(base / "j1")
    build(work, flagged=True)
    errors = [e for e in run_validate(work)
              if "按不出来" in e and any(name in e for name, _ in pair)]
    case.check("同格两个 npc、各带互补旗标 → 规则 17 不报 %s" % (cell,),
               not errors, chr(10).join(errors[:3]))

    work = make_workdir(base / "j2")
    build(work, flagged=False)
    errors = [e for e in run_validate(work)
              if "按不出来" in e and pair[1][0] in e]
    case.check("同样两个 npc、去掉旗标 → 报靠前的抢先命中（同类遮蔽仍有牙）",
               any("抢先命中" in e for e in errors), chr(10).join(errors[:3]))

    # ---- J3/J4：占格。把一个 2x2 的 npc 压在外刃堂唯一的走廊上 ----
    # 与 C 那条同一处堵点、同一手法，只把设施换成 npc —— 差别全在可见性上。
    def plug(work: Path, flagged: bool) -> None:
        payload = load_map(work, CHOKE_MAP)
        props = {"visible_flag": "ch02.renqing_jiexia"} if flagged else {}
        add_object(payload, npc_object("npc_selftest_blocker", CHOKE_CELL, props, size=(2, 2)))
        save_map(work, CHOKE_MAP, payload)

    work = make_workdir(base / "j4")
    plug(work, flagged=False)
    errors = [e for e in run_validate(work) if "无法到达" in e or "踏不响" in e]
    case.check("无旗标的 npc 堵死唯一走廊 → 规则 13 报不可达（占格仍有牙）",
               bool(errors), chr(10).join(errors[:3]))

    work = make_workdir(base / "j3")
    plug(work, flagged=True)
    still = [e for e in run_validate(work) if "无法到达" in e or "踏不响" in e]
    case.check("同一个 npc 加上 visible_flag → 不再当墙，规则 13 不报",
               not still, chr(10).join(still[:3]))


# ---------------------------------------------------------------------------
# Q. 野外遭遇与脚本 BGM（规则 27、28，docs/interfaces-octo-encounters.md）
# ---------------------------------------------------------------------------

ENCOUNTER_RULE_WORDS = ("野外遭遇", "table_id", "不放野外遭遇", "entries[", "bgm 点的曲子")


def selftest_q_encounters(case: Case, base: Path) -> None:
    """规则 27（表、区、编成）与规则 28（bgm 点的曲子在不在）。

    与 N 同一个做法：一份全量副本，逐条改坏一处、跑一遍、原样改回。"""
    print()
    print("Q. 野外遭遇：表、区、编成；脚本 bgm 点的曲子")

    work = make_full_workdir(base / "q")
    # 全量副本不带 assets/：规则 28 只看 assets/bgm/ 下有没有那个文件，放一个空的占位就够。
    (work / "assets" / "bgm").mkdir(parents=True)
    (work / "assets" / "bgm" / "bgm_night.ogg").write_bytes(b"")

    def rule_errors() -> list[str]:
        return [e for e in run_validate_full(work) if any(w in e for w in ENCOUNTER_RULE_WORDS)]

    clean = rule_errors()
    case.check("不动的副本 → 规则 27 / 28 一条不报", not clean, chr(10).join(clean[:3]))

    def mutated_json(relative: str, mutate) -> list[str]:
        path = work / relative
        original = path.read_bytes()
        payload = json.loads(original.decode("utf-8"))
        mutate(payload)
        path.write_text(json.dumps(payload, ensure_ascii=False, indent=2), "utf-8")
        try:
            return rule_errors()
        finally:
            path.write_bytes(original)

    def expect(name: str, found: list[str], needle: str) -> None:
        case.check(name, any(needle in e for e in found), chr(10).join(found[:3]) or "（一条也没报）")

    boar = "data/battles/be03_guwai_yezhu.json"
    expect("遭遇编成输了会 game over → 报",
           mutated_json(boar, lambda b: b.__setitem__("defeat_is_fatal", True)), "不许 game over")
    expect("遭遇编成不许逃 → 报",
           mutated_json(boar, lambda b: b.__setitem__("can_escape", False)), "必须许逃")
    expect("遭遇编成里混进墨府尸傀 → 报",
           mutated_json(boar, lambda b: b["units"].append({"role_id": "mofu_shigui", "faction": "enemy"})),
           "不许进野外遭遇")
    expect("遭遇编成发的修为比上限多 → 报",
           mutated_json(boar, lambda b: b["rewards"].__setitem__("cultivation", 50)), "修为至多")
    expect("遭遇编成掉落不是灵草 → 报",
           mutated_json(boar, lambda b: b["rewards"].__setitem__(
               "drops", [{"item_id": "pill_jinchuang_yao", "count": 1}])), "只许灵草")
    expect("遭遇编成比同章最轻的剧情战还凶 → 报",
           mutated_json("data/roles/ye_zhu.json", lambda r: r.__setitem__("attack", 50)), "比同章剧情战轻")
    expect("遭遇表的权重写 0 → 报",
           mutated_json("data/encounters/ch03_guwai.json",
                        lambda t: t["entries"][0].__setitem__("weight", 0)), "weight 必须是")
    expect("遭遇表的境界下限高于上限 → 报",
           mutated_json("data/encounters/ch03_guwai.json",
                        lambda t: t["entries"][0].__setitem__("minRealm", "CoreLate")), "高于 maxRealm")

    def with_zone(map_id: str, props: dict) -> list[str]:
        original = (work / "maps" / (map_id + ".tmj")).read_bytes()
        payload = load_map(work, map_id)
        objects_layer(payload).append(make_object("encounter_selftest", "encounter", (2, 2), props, (2, 2)))
        save_map(work, map_id, payload)
        try:
            return rule_errors()
        finally:
            (work / "maps" / (map_id + ".tmj")).write_bytes(original)

    zone = {"table_id": "encounter_ch03_guwai", "steps_min": "10", "steps_max": "20"}
    expect("遭遇区指向不存在的表 → 报", with_zone("ch03_guwai", dict(zone, table_id="encounter_nope")),
           "没有这张表")
    expect("第 2 章的图上放遭遇区 → 报", with_zone("ch02_yaopu", zone), "不放野外遭遇")
    # 早年那三张引用剧情战的旧表 2026-09-29 已删：改拿谷外那张挂在图上的现行表，就地换进一场剧情战。
    expect("地图用着的遭遇表里混进一场剧情战 → 报",
           mutated_json("data/encounters/ch03_guwai.json",
                        lambda t: t["entries"][0].__setitem__("battleId", "b03_gu_wai_elang")), "要以 be 打头")

    patch_script(work, "scripts/ch05/yeru.lua", 'bgm("bgm_night")', 'bgm("bgm_nope")')
    expect("脚本 bgm 点了一首不存在的曲子 → 报", rule_errors(), "bgm 点的曲子不存在")
    patch_script(work, "scripts/ch05/yeru.lua", 'bgm("bgm_nope")', 'bgm("bgm_night")')


# ---------------------------------------------------------------------------
# R. 衔接朝向与回弹（规则 29）；S. 同图 npc 不重名（规则 30）
# 动手的图与 E 节同一批（stable_maps，理由见那里），门、落点、人一律现找。
# ---------------------------------------------------------------------------

BACK = {"up": "down", "down": "up", "left": "right", "right": "left"}
TURN = {"up": "right", "right": "down", "down": "left", "left": "up"}


def find_spawn(payload: dict, spawn_id: str) -> dict:
    """按 spawn 的 id 属性找对象 —— 对象名与 id 不一定是同一个字。"""
    for entry in objects_layer(payload):
        if entry.get("type") == "spawn" and props_of(entry).get("id") == spawn_id:
            return entry
    raise AssertionError("找不到 spawn " + spawn_id)


def find_door(work: Path, need_front: bool) -> tuple[str, str, str, str, str, tuple[int, int]]:
    """在 stable_maps 里现找一道门：A 图的 P 通往 B 图的落点 S，两头都在那几张里，
    P 有唯一的出门方向 d。need_front 时还要求 S 沿 d 的下一格走得上去、又不是别的门
    —— 用例要往那一格摆一道回程门。返回 (A, P, B, S, d, S 沿 d 的下一格)。

    进门方向与出门方向用校验器自己的函数算：要造的正是「校验器眼里」的那种门，
    照着它的口径另写一份，写错了就永远问不出来（与 parse_one 同一个理由）。
    """
    names = stable_maps(work)
    parsed = {name: parse_one(work, name) for name in names}
    for a in names:
        pa, ca = parsed[a]
        for portal in pa.portals:
            b = portal["props"].get("target_map", "")
            s = portal["props"].get("target_spawn", "")
            if b not in parsed or s not in parsed[b][0].spawns:
                continue
            entry = validate.portal_entry_directions(pa, ca, validate.occupied_cells(pa), portal)
            main = validate.portal_main_direction(pa, portal, entry)
            if main is None:
                continue
            pb, cb = parsed[b]
            dx, dy = validate.STEP_DIRECTIONS[main]
            front = (pb.spawns[s]["x"] + dx, pb.spawns[s]["y"] + dy)
            doors = {cell for q in pb.portals for cell in validate.object_cells(q)}
            if need_front and (front in doors or validate.cell_blocked(
                    pb, cb, validate.occupied_cells(pb), front)):
                continue
            return a, portal["name"], b, s, main, front
    raise AssertionError("%s 里找不到合用的门" % "、".join(names))


def rule29_errors(errors: list[str], map_id: str, door: str) -> list[str]:
    """规则 29 报给某一道门的那几条。按报错开头的「A 的 P」认，不按子串认：
    别的门的「来回弹」里也会提到这道门的名字（踩上的是它）。"""
    head = "规则 29：%s 的 %s" % (map_id, door)
    return [e for e in errors if head in e]


def selftest_r_portal_direction(case: Case, base: Path) -> None:
    """规则 29：出门往哪走、进门就朝哪；落地后按住方向键不许一路走回原图。

    三组，每组都问一件别的组问不到的事：
      R1 造一处真会来回弹的衔接 —— 落点掉头、正前方一格就是回原图的门。这就是韩家村
         → 青牛镇当初的形状（落在回程门跟前、朝向还反了），(a)(b) 都得报。
      R2 只把落点朝向拧 90°：(a) 要报，(b) 不许报 —— 按住走的是进门时按着的那个键，
         与落点朝哪无关。把两件事混成一件的实现会在这里多报。
      R3 多入口的内门（ch01_qixuanmen 的 portal_to_caixiashan，下、左、右三面都进得去）：
         落点朝下是其中一面，不许报；把朝向改成进不去的那一面，必须报。只写前半条的话，
         把「没有主方向」那一支整个删掉也能全绿。
    """
    print()
    print("R. 衔接朝向与回弹（规则 29）")

    def shown(found: list[str]) -> str:
        return chr(10).join(found[:3]) or "（一条也没报）"

    # ---- R1：掉头 + 回程门就在跟前 ----
    work = make_workdir(base / "r1")
    a, door, b, landing, main, front = find_door(work, need_front=True)
    home = validate.spawn_landing(parse_one(work, a)[0], "")["props"]["id"]
    payload = load_map(work, b)
    set_prop(find_spawn(payload, landing), "facing", BACK[main])
    add_object(payload, make_object("portal_selftest_huitou", "portal", front,
                                    {"target_map": a, "target_spawn": home}))
    save_map(work, b, payload)
    mine = rule29_errors(run_validate(work), a, door)
    case.check("%s 的 %s 落点 %s:%s 掉头 → 报 (a) 朝向" % (a, door, b, landing),
               any("面朝" + validate.direction_word(BACK[main]) in e for e in mine), shown(mine))
    case.check("落点正前方一格就是回 %s 的门 → 报 (b) 来回弹，点名那道门、走 1 步" % a,
               any("来回弹" in e and "portal_selftest_huitou" in e and "走 1 步" in e for e in mine),
               shown(mine))

    # ---- R2：只拧 90°，路一格没变 ----
    work = make_workdir(base / "r2")
    a, door, b, landing, main, _front = find_door(work, need_front=False)
    payload = load_map(work, b)
    set_prop(find_spawn(payload, landing), "facing", TURN[main])
    save_map(work, b, payload)
    mine = rule29_errors(run_validate(work), a, door)
    case.check("%s 的 %s 落点朝向拧 90°（朝%s）→ 报 (a)"
               % (a, door, validate.direction_word(TURN[main])),
               any("是往%s走进去" % validate.direction_word(main) in e
                   and "面朝" + validate.direction_word(TURN[main]) in e for e in mine), shown(mine))
    case.check("同一处不报 (b)：只动了朝向，按住走的路一格没变",
               not any("来回弹" in e for e in mine), chr(10).join(e for e in mine if "来回弹" in e))

    # ---- R3：多入口的内门 ----
    work = make_workdir(base / "r3")
    parsed, collision = parse_one(work, "ch01_qixuanmen")
    portal = next(p for p in parsed.portals if p["name"] == "portal_to_caixiashan")
    entry = validate.portal_entry_directions(parsed, collision, validate.occupied_cells(parsed), portal)
    outside = [d for d in validate.STEP_DIRECTIONS if d not in entry]
    case.check("夹具前提：portal_to_caixiashan 确是多面可进、没有唯一出门方向的内门（%s）"
               % "、".join(validate.direction_word(d) for d in validate.STEP_DIRECTIONS if d in entry),
               len(entry) >= 2 and bool(outside)
               and validate.portal_main_direction(parsed, portal, entry) is None)
    mine = rule29_errors(run_validate(work), "ch01_qixuanmen", "portal_to_caixiashan")
    case.check("多入口内门、落点朝其中一面 → 不报", not mine, chr(10).join(mine[:3]))

    wrong = outside[0] if outside else "up"
    target, spawn_id = portal["props"]["target_map"], portal["props"]["target_spawn"]
    payload = load_map(work, target)
    set_prop(find_spawn(payload, spawn_id), "facing", wrong)
    save_map(work, target, payload)
    mine = rule29_errors(run_validate(work), "ch01_qixuanmen", "portal_to_caixiashan")
    case.check("同一道门、落点改朝进不去的那一面（%s）→ 报" % validate.direction_word(wrong),
               any("哪一面都不是" in e for e in mine), shown(mine))


def rule30_errors(errors: list[str], role: str) -> list[str]:
    """规则 30 报给某个 role 的那几条（按「role X、」认，免得 han 认成 han_sanshu）。"""
    return [e for e in errors if "规则 30" in e and ("role %s、" % role) in e]


def find_plain_npc(work: Path) -> tuple[str, str, str]:
    """在 stable_maps 里现找一个没有 visible_flag / hidden_flag 的 npc：(图, 对象名, role)。"""
    for map_id in stable_maps(work):
        parsed, _collision = parse_one(work, map_id)
        for record in parsed.objects:
            if record["type"] == "npc" and validate.npc_always_present(record) \
                    and record["props"].get("role_id"):
                return map_id, record["name"], record["props"]["role_id"]
    raise AssertionError("%s 里找不到没旗标的 npc" % "、".join(stable_maps(work)))


def role_file(work: Path, role_id: str) -> dict:
    """按 id 找副本里的角色文件内容（文件名按约定就是 id，但以文件里写的 id 为准）。"""
    for path in sorted((work / "data" / "roles").rglob("*.json")):
        payload = json.loads(path.read_text(encoding="utf-8"))
        if isinstance(payload, dict) and payload.get("id") == role_id:
            return payload
    raise AssertionError("副本的 data/roles 里没有 " + role_id)


# S5、S6 临时造的角色：整份照抄原件，只换 id —— 名字也照抄，正是要抓的那种写法。
ALIAS_ROLE = "selftest_tongming"


def selftest_s_npc_role_clash(case: Case, base: Path) -> None:
    """规则 30：同一张图上可能同时在场的 npc，名牌上不许撞名。

    六条都用同一个 npc、同一处空地复制出一个孪生，只差一处：
      S1 原样复制（同 role、都不带旗标）→ 必须报，两个对象名都点到；而且只报一条 ——
         role 一样名字当然一样，不许再按同名报第二遍；
      S2 写成互补旗标（原件 hidden_flag = 孪生 visible_flag）→ 不许报，这是
         「同一个人换一段剧情」的标准写法（外刃堂厉飞雨、药圃管事都这么写）；
      S3 两个都写 visible_flag、还是同一个旗标 → 必须报：旗标一置两人一起出现。
         「沾了同一个旗标」「都带了旗标」都不等于轮流在场，S2 那条放行只认互补；
      S4 两个的 role 都换成群像白名单里的那个 → 不许报；
      S5 孪生换成一个照抄了原件名字的新 role（id 不同、name 相同）、都不带旗标 → 必须报，
         两个 role 与那个名字都点到。只比 role_id 的实现在这里漏：投诉的是名牌，不是 id；
      S6 同 S5，但写成互补旗标 → 不许报。
    S2、S4、S6 只写不报的那一半的话，把整条检查注释掉也能全绿，S1、S3、S5 就是它们的牙。

    副本带上 data/：名牌上的名字在 data/roles 里，只有 maps/ 的副本比不了名字。
    """
    print()
    print("S. 同一张图上同时在场的 npc 不重名（规则 30）")

    probe = make_full_workdir(base / "s0")
    host, name, role = find_plain_npc(probe)
    spot = free_open_cell(probe, host)
    shown_name = role_file(probe, role)["name"]
    # 与 J 那一节同一个旗标。规则 30 只问两边是不是同一个旗标，不问它什么时候置上。
    switch = "ch01.sanshu_met"
    # 写死，不从 validate.CROWD_ROLES 里取：要钉的是「南城墨府门口那四个护院放行」这件事。
    # 从被测物里取的话，白名单被清空或换了人，这一条照样能挑出个什么来测、照样绿。
    crowd = "mofu_huyuan"

    def with_twin(tag: str, original_props: dict, twin_props: dict,
                  alias: bool = False) -> list[str]:
        work = make_full_workdir(base / tag)
        if alias:
            copied = dict(role_file(work, role), id=ALIAS_ROLE)
            (work / "data" / "roles" / (ALIAS_ROLE + ".json")).write_text(
                json.dumps(copied, ensure_ascii=False, indent=2), encoding="utf-8")
            twin_props = dict(twin_props, role_id=ALIAS_ROLE)
        payload = load_map(work, host)
        original = find_object(payload, name)
        twin = json.loads(json.dumps(original))
        twin.update({"id": 9990, "name": "npc_selftest_twin",
                     "x": spot[0] * TILE, "y": spot[1] * TILE})
        for key, value in original_props.items():
            set_prop(original, key, value)
        for key, value in twin_props.items():
            set_prop(twin, key, value)
        add_object(payload, twin)
        save_map(work, host, payload)
        return run_validate(work)

    where = "%s 的 %s（role %s）复制到 %s" % (host, name, role, spot)
    errors = with_twin("s1", {}, {})
    found = rule30_errors(errors, role)
    case.check("%s、都不带旗标 → 报，两个对象名都点到" % where,
               any(name in e and "npc_selftest_twin" in e for e in found),
               chr(10).join(found[:2]) or "（一条也没报）")
    twice = [e for e in errors if "规则 30" in e and "npc_selftest_twin" in e]
    case.check("同 role 只报一条（不因为同名再报一遍）", len(twice) == 1, chr(10).join(twice))

    found = rule30_errors(with_twin("s2", {"hidden_flag": switch}, {"visible_flag": switch}), role)
    case.check("同一对写成互补旗标（hidden_flag = 对方的 visible_flag）→ 不报", not found,
               chr(10).join(found[:2]))

    found = rule30_errors(with_twin("s3", {"visible_flag": switch}, {"visible_flag": switch}), role)
    case.check("两个都是 visible_flag=%s（旗标一置一起出现）→ 报" % switch, bool(found),
               chr(10).join(found[:2]) or "（一条也没报）")

    found = rule30_errors(with_twin("s4", {"role_id": crowd}, {"role_id": crowd}), crowd)
    case.check("同一对换成群像白名单里的 %s → 不报" % crowd, not found, chr(10).join(found[:2]))

    def same_name(errors: list[str]) -> list[str]:
        return [e for e in errors if "规则 30" in e and "「%s」" % shown_name in e]

    found = same_name(with_twin("s5", {}, {}, alias=True))
    case.check("孪生换成照抄名字的新 role %s（名牌都是「%s」）、都不带旗标 → 报，两个 role 与名字都点到"
               % (ALIAS_ROLE, shown_name),
               any(role in e and ALIAS_ROLE in e and name in e and "npc_selftest_twin" in e
                   for e in found), chr(10).join(found[:2]) or "（一条也没报）")

    found = same_name(with_twin("s6", {"hidden_flag": switch}, {"visible_flag": switch}, alias=True))
    case.check("同名的两个 role 写成互补旗标 → 不报", not found, chr(10).join(found[:2]))


def main() -> int:
    case = Case()
    with tempfile.TemporaryDirectory(prefix="fanren_selftest_") as tmp:
        base = Path(tmp)
        selftest_baseline(case, make_workdir(base / "base"))
        selftest_c_reachability(case, base)
        selftest_a_once_contract(case, base)
        selftest_b_regen(case, base)
        selftest_b2_ch02_drift(case, base)
        selftest_d_world_reachability(case, base)
        selftest_e_interaction_shadow(case, base)
        selftest_f_flag_order(case, base)
        selftest_h_deny_keys(case, base)
        selftest_g_script_item_ids(case, base)
        selftest_i_empty_category(case, base)
        selftest_j_npc_visibility(case, base)
        selftest_k_hook_declarations(case, base)
        selftest_l_objectives(case, base)
        selftest_m_quests(case, base)
        selftest_p_path_actions(case, base)
        selftest_n_battle_break(case, base)
        selftest_q_encounters(case, base)
        selftest_r_portal_direction(case, base)
        selftest_s_npc_role_clash(case, base)

    print()
    print("用例 %d 条，未如期抓住 %d 条" % (case.total, case.failed))
    if case.failed:
        print("SELFTEST_FAIL")
        return 1
    print("SELFTEST_OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())
