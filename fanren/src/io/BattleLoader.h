#pragma once
// 战斗配置加载：data/battles/<id>.json → core::BattleSetup。
//
// 单位只带 role_id，属性从已加载的 GameData::roles 里取。战斗文件因此不重复
// 声明境界与数值，也不必再实现一份境界名解析——两份解析表迟早会走样。
#include <map>
#include <string>

#include "core/Result.h"
#include "core/model/Types.h"

namespace fanren::io {

// 读取单个战斗配置。缺字段用默认值，但 id 缺失、units 为空、波次号不连续、
// backdrop 不是字符串都判失败，error 里写明是哪个文件的哪一项。
// 横版战斗没有格子：width / height / player_spawn / 单位的 x / y 一概不读
//（docs/octopath-battle.md 第 5 节；残留在数据里的由 tools/validate.py 报错）。
//
// hero_absent（契约 docs/interfaces-p3-ch05.md 第 2 节）：缺省 false；写了却不是布尔、
// 或者为真而第 0 波里一个 ally 也没有，都判失败。
[[nodiscard]] core::Result<core::BattleSetup> loadBattle(const std::string& path);

// 扫描整个目录。重复 id 判失败（不是后者覆盖前者），与 loadGameData 同口径。
[[nodiscard]] core::Result<std::map<std::string, core::BattleSetup>> loadBattles(
    const std::string& battlesRoot);

}  // namespace fanren::io
