#pragma once
// 美术产物描述文件的读取器：精灵索引 assets/art/sprites/index.json（tools/artgen/sprites.py 生成）
// 与烘焙地图的 assets/art/maps/<id>/meta.json（tools/artgen/maplights.py 生成）。
// 读出来的样子见 core/model/Visual.h；挑外观、算帧在 game/SpriteAtlas.h，换成引擎的颜色与粒子在 game/MapVisual.h。
//
// 这两份文件原先由上层各读一遍（界面层 ui::Json + ui::SpriteIndex、世界画面 game::VisualJson +
// SpriteAtlas + MapVisual），JSON 读取器三份、精灵索引两份。分层约定是 JSON 只在 io 层读，
// 战斗画面也要读同一张索引与 meta 里的战斗背景——所以统一到这里，上层只拿纯结构体。
//
// 解析是严格的：生成器写出来的东西缺了必填项、类型不对、写了认不出的时辰或粒子种类，就是生成器坏了
// 或有人手改坏了——整份报错，调用方退回旧画法并打一行警告；可选项缺了按 docs/art-sprites.md 第 5 节
// 与 game/MapVisual.h 文件头写明的缺省补。悄悄按缺省画只会让「夜景怎么成了白天」变成没人查得出的谜。
//
// 报错口径：只报第一处，「index.json 的 sheets.hanli.walk.up：……」「meta.json 的 lights[0].y：……」；
// 语法错带行列号（「第 3 行第 14 列：……」）。
#include <map>
#include <string>
#include <string_view>

#include "core/Result.h"
#include "core/model/Visual.h"

namespace fanren::io {

// 精灵索引。parse 的每一种报错都以「index.json」起头；load 读不了文件、语法错时报错带路径。
[[nodiscard]] core::Result<core::SpriteIndex> parseSpriteIndex(std::string_view json);
[[nodiscard]] core::Result<core::SpriteIndex> loadSpriteIndex(const std::string& path);

// 烘焙地图的 meta.json。parse 的每一种报错都以「meta.json」起头；load 的每一种报错都带路径。
[[nodiscard]] core::Result<core::MapMeta> parseMapMeta(std::string_view json);
[[nodiscard]] core::Result<core::MapMeta> loadMapMeta(const std::string& path);

// 战斗背景的人工指派 data/visual/battles.json：battle_id → assets/art/battle/ 下的背景 id。
// 形状 {"_doc": …, "battles": {"<battle_id>": {"backdrop": "<id>", "map": …, "why": …}}}，
// 只取 backdrop，map / why 是给人看的。parse 的报错以「battles.json」起头。
//
// 这张表是「编成所在地图的 meta 背景」之上的覆写：同一张演武场，攻防战是夜里、切磋是白天，
// 按地图取就只能取到一种。战斗画面合入时它一度没人读，19 场里 11 场取错了背景。
[[nodiscard]] core::Result<std::map<std::string, std::string>> parseBattleBackdrops(
    std::string_view json);
[[nodiscard]] core::Result<std::map<std::string, std::string>> loadBattleBackdrops(
    const std::string& path);

}  // namespace fanren::io
