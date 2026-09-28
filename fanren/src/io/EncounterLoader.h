#pragma once
// 遭遇表加载器（docs/interfaces-octo-encounters.md 第 2 节）：data/encounters/*.json → rules::EncounterTable。
//
// 口径与其它加载器一样：**字段写了就得写对**，写错了读不进来，报错里点名文件与字段——
// 悄悄按缺省收下的一张表，要到玩家在野地里走了半天一场也遇不上才会有人发觉。
// 编成引用（battleId）在这里就对账：遭遇表是运行期直接拿去开战的，一条悬空的
// battleId 到那时只会变成「遇上了、却什么也没发生」。
#include <map>
#include <string>

#include "core/Result.h"
#include "core/model/Types.h"
#include "core/rules/Encounter.h"

namespace fanren::io {

// 读一张表。battles 是已经读好的全部编成，用来对 battleId 的账。
[[nodiscard]] core::Result<rules::EncounterTable> loadEncounterTable(
    const std::string& path, const std::map<std::string, core::BattleSetup>& battles);

// 读整个目录（递归），按表 id 建索引；id 重复判失败。目录不存在按 0 张表处理，与 loadShops 同口径。
[[nodiscard]] core::Result<std::map<std::string, rules::EncounterTable>> loadEncounterTables(
    const std::string& encountersRoot, const std::map<std::string, core::BattleSetup>& battles);

}  // namespace fanren::io
