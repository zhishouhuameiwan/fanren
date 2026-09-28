#!/usr/bin/env python3
"""内容时长审计：按玩家画像估算一周目的游玩时长。

    python tools/audit.py                # 默认画像
    python tools/audit.py --profile speedrun
    python tools/audit.py --json         # 机器可读，供 CI 记录趋势

口径来自 docs/凡人修仙传RPG-重构方案.md 第 2.2、2.3 节，三个互斥的时长桶：

    A 主线      主线对白 + 剧情战斗 + 必经地图的探索移动
    B 必经      主线前置的资源获取操作（种药、炼丹、采买、猎妖、拍卖）
    C 可选      可选支线、秘境、图鉴、情缘

门槛只看 A + B。C 桶只报「正常游玩」总量，不计入门槛。

本工具**不给出权威时长**，它给的是结构指标加换算。每类节点的秒数要由真人
抽样标定后回填到 CALIBRATION 里；未标定前用的是方案里的初始估值，会偏。
无头 bot 也不给时长，只给结构指标——bot 不是人，它的耗时由动画速度决定。
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

# 换算参数。真人抽样标定后改这里，不要散落在代码各处。
CALIBRATION = {
    # 中文 RPG 对白多短句，实测阅读速度约 400-600 字/分。取中值。
    "chars_per_minute": 500.0,
    # 一场剧情战斗的平均耗时，含布阵与结算。
    "minutes_per_story_battle": 5.0,
    # 首次通过一张地图的探索移动时间。
    "minutes_per_map_first_pass": 0.75,
    # 每章必经操作（种药、炼丹、采买）的平均时间。
    "minutes_per_chapter_mandatory": 13.0,
    # 一个可选支线的平均耗时。
    "minutes_per_side_quest": 5.0,
}

# 玩家画像：决定各类内容的触达率。验收时测试者按同一份画像游玩。
PROFILES = {
    "default": {
        "label": "正常游玩（完成 60% 支线、图鉴 50%、至少一次拍卖）",
        "main_text_reach": 1.0,
        "optional_text_reach": 0.7,
        "side_quest_reach": 0.6,
        "optional_map_reach": 0.6,
    },
    "speedrun": {
        "label": "主线速通（跳过可跳对白，不做可选内容）",
        "main_text_reach": 0.55,
        "optional_text_reach": 0.0,
        "side_quest_reach": 0.0,
        "optional_map_reach": 0.0,
    },
    "completionist": {
        "label": "全收集",
        "main_text_reach": 1.0,
        "optional_text_reach": 1.0,
        "side_quest_reach": 1.0,
        "optional_map_reach": 1.0,
    },
}

CJK_RANGE = re.compile(r"[㐀-鿿豈-﫿]")
MAIN_TEXT_HINT = re.compile(r"^ch\d+\.")


@dataclass
class Counts:
    main_chars: int = 0
    optional_chars: int = 0
    text_entries: int = 0
    maps: int = 0
    map_chapters: set = field(default_factory=set)
    story_battles: int = 0
    encounter_tables: int = 0
    scripts: int = 0
    dialogue_nodes: int = 0
    choice_nodes: int = 0
    branches: int = 0
    side_quests: int = 0
    items: int = 0
    recipes: int = 0
    shops: int = 0


def count_chinese(text: str) -> int:
    """按汉字数计，不按 len()——标点与空白不占阅读时间。"""
    return len(CJK_RANGE.findall(text))


def collect(counts: Counts) -> None:
    data = ROOT / "data"
    if data.exists():
        for path in sorted((data / "text").rglob("*.json")) if (data / "text").exists() else []:
            try:
                payload = json.loads(path.read_text(encoding="utf-8"))
            except (json.JSONDecodeError, UnicodeDecodeError):
                continue
            if not isinstance(payload, dict):
                continue
            for key, value in payload.items():
                if not isinstance(value, str):
                    continue
                counts.text_entries += 1
                chars = count_chinese(value)
                # 约定：主线文案 key 以 chN. 开头；其余算可选。
                if MAIN_TEXT_HINT.match(key):
                    counts.main_chars += chars
                else:
                    counts.optional_chars += chars

        for subdir, attr in (("battles", "story_battles"), ("encounters", "encounter_tables"),
                             ("items", "items"), ("recipes", "recipes"), ("shops", "shops"),
                             ("quests", "side_quests")):
            target = data / subdir
            if target.exists():
                setattr(counts, attr, len(list(target.rglob("*.json"))))

    maps = ROOT / "maps"
    if maps.exists():
        for path in sorted(maps.glob("*.tmj")):
            counts.maps += 1
            match = re.match(r"^ch(\d+)_", path.stem)
            if match:
                counts.map_chapters.add(int(match.group(1)))

    scripts = ROOT / "scripts"
    if scripts.exists():
        for path in sorted(scripts.rglob("*.lua")):
            if path.parent.name == "common":
                continue
            counts.scripts += 1
            source = path.read_text(encoding="utf-8", errors="replace")
            # 去掉注释，免得把注释里的示例算进节点数。
            body = "".join(
                line if line.find("--") < 0 else line[: line.find("--")]
                for line in source.splitlines(keepends=True)
            )
            counts.dialogue_nodes += len(re.findall(r"(?<![A-Za-z0-9_])talk\s*\(", body))
            choices = len(re.findall(r"(?<![A-Za-z0-9_])choice\s*[({]", body))
            counts.choice_nodes += choices
            counts.branches += len(re.findall(r"(?<![A-Za-z0-9_])if\s", body))


def estimate(counts: Counts, profile: dict) -> dict:
    cal = CALIBRATION
    chapters = len(counts.map_chapters) or 1

    main_minutes = (
        counts.main_chars * profile["main_text_reach"] / cal["chars_per_minute"]
        + counts.story_battles * cal["minutes_per_story_battle"]
        + counts.maps * cal["minutes_per_map_first_pass"]
    )
    mandatory_minutes = chapters * cal["minutes_per_chapter_mandatory"]
    optional_minutes = (
        counts.optional_chars * profile["optional_text_reach"] / cal["chars_per_minute"]
        + counts.side_quests * profile["side_quest_reach"] * cal["minutes_per_side_quest"]
    )

    return {
        "bucket_a_main_minutes": round(main_minutes, 1),
        "bucket_b_mandatory_minutes": round(mandatory_minutes, 1),
        "bucket_c_optional_minutes": round(optional_minutes, 1),
        "gate_hours": round((main_minutes + mandatory_minutes) / 60.0, 2),
        "total_hours": round((main_minutes + mandatory_minutes + optional_minutes) / 60.0, 2),
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="内容时长审计")
    parser.add_argument("--profile", choices=sorted(PROFILES), default="default")
    parser.add_argument("--json", action="store_true", help="机器可读输出")
    args = parser.parse_args()

    counts = Counts()
    collect(counts)
    profile = PROFILES[args.profile]
    result = estimate(counts, profile)

    if args.json:
        print(json.dumps({
            "profile": args.profile,
            "counts": {
                "main_chars": counts.main_chars,
                "optional_chars": counts.optional_chars,
                "text_entries": counts.text_entries,
                "maps": counts.maps,
                "chapters_with_maps": sorted(counts.map_chapters),
                "story_battles": counts.story_battles,
                "encounter_tables": counts.encounter_tables,
                "scripts": counts.scripts,
                "dialogue_nodes": counts.dialogue_nodes,
                "choice_nodes": counts.choice_nodes,
                "items": counts.items,
                "recipes": counts.recipes,
                "shops": counts.shops,
            },
            "estimate": result,
        }, ensure_ascii=False, indent=1))
        return 0

    print(f"画像：{profile['label']}")
    print()
    print("结构指标")
    print(f"  主线文案      {counts.main_chars:>8} 字   （{counts.text_entries} 条）")
    print(f"  可选文案      {counts.optional_chars:>8} 字")
    print(f"  地图          {counts.maps:>8} 张   （覆盖章节 {sorted(counts.map_chapters) or '无'}）")
    print(f"  剧情战斗      {counts.story_battles:>8} 场")
    print(f"  遭遇表        {counts.encounter_tables:>8} 张")
    print(f"  事件脚本      {counts.scripts:>8} 个")
    print(f"  对话节点      {counts.dialogue_nodes:>8} 处")
    print(f"  选择节点      {counts.choice_nodes:>8} 处")
    print(f"  物品/配方/商店 {counts.items:>3} / {counts.recipes} / {counts.shops}")
    print()
    print("时长估算（按 CALIBRATION 换算，未经真人标定）")
    print(f"  A 主线        {result['bucket_a_main_minutes']:>8.1f} 分钟")
    print(f"  B 必经        {result['bucket_b_mandatory_minutes']:>8.1f} 分钟")
    print(f"  C 可选        {result['bucket_c_optional_minutes']:>8.1f} 分钟")
    print()
    print(f"  门槛（A+B）   {result['gate_hours']:>8.2f} 小时   目标 ≥ 10")
    print(f"  正常游玩合计  {result['total_hours']:>8.2f} 小时")
    print()
    if result["gate_hours"] < 10.0:
        shortfall = 10.0 - result["gate_hours"]
        print(f"  距门槛还差 {shortfall:.2f} 小时。按方案第 2.3 节，"
              f"优先补剧情战斗与分支，不要靠加字数凑。")
    print()
    print("注：本工具给的是结构指标加换算，不是权威时长。")
    print("    每类节点的秒数需真人抽样标定后回填 CALIBRATION。")
    return 0


if __name__ == "__main__":
    sys.exit(main())
