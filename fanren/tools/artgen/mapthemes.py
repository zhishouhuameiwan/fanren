"""主题表：同一个瓦片语义在不同主题下画成什么。

地图只有 9 种瓦片语义，墙就是墙。「这堵墙是茅屋、城墙还是崖壁」由主题决定：
主题给出屋顶瓦种、立面做法、路面、树种、篱笆/摊架/石灯的默认解释。
每张图用哪个主题写在 data/visual/maps.json（人工维护）；加一个主题 = 在这里加
一行，见 docs/art-maps.md「怎么加一个主题」。

字段：
  kind      natural（周边墙体是山石）/ built（周边墙体是院墙、厚处是屋顶群）/ indoor / cave
  rock      natural 与 cave 的岩体：cliff / valley / cave / earth / dusk
  rock_top  岩体顶面：forest（松林）/ rocky（石坡疏草）/ grass（草坡）/ void（室内洞顶）
  roof      屋瓦：grey 青瓦 / thatch 茅草 / jade 宗门深青 / black 黛瓦
  facade    立面做法：village / town / sect / manor / valley / stone
  wall      院墙做法：earth / brick / sect / manor / indoor_wood / indoor_stone / fortress
  ground/road/special   地表 gid 1 / 2 / 3（可走）默认画成哪种地面
  paving    石板路的色系
  trees     各季节的树种轮换表（按杂凑取）
  fence_line / fence_single / fence_block   gid 8 连成线 / 单格 / 成块 的默认解释
"""

from __future__ import annotations

THEMES: dict[str, dict] = {
    "village": dict(
        kind="built", rock="cliff", rock_top="grass", roof="thatch", facade="village", wall="earth",
        ground="grass", road="dirt_road", special="tilled", paving="town",
        trees={"spring": ["peach", "broad", "peach", "locust"], "summer": ["broad", "locust"],
               "autumn": ["maple", "broad", "ginkgo"]},
        fence_line="bamboo_fence", fence_single="haystack", fence_block="woodpile",
    ),
    "town": dict(
        kind="built", rock="cliff", rock_top="rocky", roof="grey", facade="town", wall="brick",
        ground="grass", road="stone_road", special="tilled", paving="town",
        trees={"spring": ["willow", "broad", "peach"], "summer": ["willow", "broad"],
               "autumn": ["ginkgo", "broad", "maple", "willow"]},
        fence_line="stall_row", fence_single="stall", fence_block="shed",
    ),
    "sect": dict(
        kind="built", rock="cliff", rock_top="rocky", roof="jade", facade="sect", wall="sect",
        ground="grass", road="stone_road", special="steps", paving="sect",
        trees={"spring": ["pine", "pine", "broad"], "summer": ["pine", "broad", "pine"],
               "autumn": ["pine", "maple", "ginkgo"]},
        fence_line="balustrade", fence_single="weapon_rack", fence_block="table",
    ),
    "manor": dict(
        kind="built", rock="valley", rock_top="grass", roof="black", facade="manor", wall="manor",
        ground="grass", road="stone_road", special="tilled", paving="manor",
        trees={"spring": ["peach", "bamboo", "broad"], "summer": ["broad", "bamboo", "willow"],
               "autumn": ["maple", "bamboo", "ginkgo", "broad"]},
        fence_line="railing", fence_single="stone_lantern", fence_block="table",
    ),
    "valley": dict(
        kind="natural", rock="valley", rock_top="grass", roof="grey", facade="valley", wall="earth",
        ground="grass", road="dirt_road", special="tilled", paving="town",
        trees={"spring": ["broad", "locust", "peach", "bamboo"], "summer": ["broad", "locust", "bamboo"],
               "autumn": ["maple", "broad", "ginkgo"]},
        fence_line="bamboo_fence", fence_single="stone_lantern", fence_block="shed",
    ),
    "mountain_path": dict(
        kind="natural", rock="cliff", rock_top="forest", roof="grey", facade="stone", wall="earth",
        ground="grass", road="dirt_road", special="scree", paving="town",
        trees={"spring": ["pine"], "summer": ["pine"], "autumn": ["pine", "maple"]},
        fence_line="railing", fence_single="rock", fence_block="rock",
    ),
    "cliff": dict(
        kind="natural", rock="cliff", rock_top="rocky", roof="grey", facade="stone", wall="earth",
        ground="grass", road="steps", special="scree", paving="sect",
        trees={"spring": ["pine"], "summer": ["pine"], "autumn": ["pine", "maple", "pine"]},
        fence_line="balustrade", fence_single="stake", fence_block="table",
    ),
    "wild": dict(
        kind="natural", rock="cliff", rock_top="forest", roof="grey", facade="stone", wall="fortress",
        ground="grass", road="dirt_road", special="scree", paving="town",
        trees={"spring": ["broad", "pine"], "summer": ["broad", "pine"],
               "autumn": ["maple", "broad", "pine", "ginkgo"]},
        fence_line="railing", fence_single="rock", fence_block="rock",
    ),
    "dock": dict(
        kind="natural", rock="earth", rock_top="grass", roof="thatch", facade="village", wall="earth",
        ground="grass", road="dirt_road", special="tilled", paving="town",
        trees={"spring": ["willow", "broad"], "summer": ["willow", "broad"],
               "autumn": ["willow", "maple", "willow"]},
        fence_line="railing", fence_single="bollard", fence_block="shed",
    ),
    "indoor_wood": dict(
        kind="indoor", rock="cave", rock_top="void", roof="grey", facade="valley", wall="indoor_wood",
        ground="wood_floor", road="tile_floor", special="scree", paving="town",
        trees={"spring": ["broad"], "summer": ["broad"], "autumn": ["broad"]},
        fence_line="counter", fence_single="stool", fence_block="table",
    ),
    "indoor_stone": dict(
        kind="indoor", rock="cave", rock_top="void", roof="grey", facade="stone", wall="indoor_stone",
        ground="stone_floor", road="flagstone", special="scree", paving="sect",
        trees={"spring": ["broad"], "summer": ["broad"], "autumn": ["broad"]},
        fence_line="bookshelf", fence_single="pillar", fence_block="stone_table",
    ),
    "cave": dict(
        kind="cave", rock="cave", rock_top="void", roof="grey", facade="stone", wall="indoor_stone",
        ground="cave_floor", road="cave_path", special="scree", paving="sect",
        trees={"spring": ["broad"], "summer": ["broad"], "autumn": ["broad"]},
        fence_line="rock_row", fence_single="fallen_rock", fence_block="fallen_rock",
    ),
}


def theme(name: str) -> dict:
    return THEMES[name]
