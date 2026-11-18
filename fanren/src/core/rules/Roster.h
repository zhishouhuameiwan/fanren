#pragma once
// 名册（伙伴改版，docs/interfaces-companions.md 第 1 节）：已结识的伙伴。**不单独存**——
// 伙伴的 met_flag 置了就在名册里。历练组队面板与主菜单的名册页、队伍页问的都是这一个函数。
#include <map>
#include <string>
#include <vector>

#include "core/model/Types.h"

namespace fanren::rules {

// 名册上的伙伴 id，按伙伴 id 排（GameData::companions 的次序）。
[[nodiscard]] std::vector<std::string> roster(const std::map<std::string, core::Companion>& companions,
                                              const core::GameState& state);

}  // namespace fanren::rules
