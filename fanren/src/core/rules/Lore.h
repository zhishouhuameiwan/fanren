#pragma once
// 见闻的规则（第一期地基包，docs/ch07-foundation.md 2.5）：哪些条目解锁了、妖兽条目补哪几行。
//
// core 层硬约束：不含 SDL / JSON / Lua / 文件 IO。解锁只认 unlock_flag，不开新的存档字段。
#include <map>
#include <set>
#include <string>
#include <vector>

#include "core/model/Types.h"

namespace fanren::rules {

// unlock_flag 为空 = 一开始就有；否则置位即解锁。
[[nodiscard]] bool loreUnlocked(const core::LoreEntry& entry, const core::GameState& state);

// 解锁了的那些，按表的次序。
[[nodiscard]] std::vector<const core::LoreEntry*> unlockedLore(const std::vector<core::LoreEntry>& entries,
                                                               const core::GameState& state);

// 解锁了的条目 id。新解锁提示（「见闻：XXX」）拿前后两份比。
[[nodiscard]] std::set<std::string> unlockedLoreIds(const std::vector<core::LoreEntry>& entries,
                                                    const core::GameState& state);

// after 里有、before 里没有的，按表的次序。
[[nodiscard]] std::vector<const core::LoreEntry*> newlyUnlockedLore(const std::vector<core::LoreEntry>& entries,
                                                                    const std::set<std::string>& before,
                                                                    const std::set<std::string>& after);

// 妖兽条目的「已知破绽」：存档里记下的那几类（位掩码，core::categoryNames 拆成名字）。
[[nodiscard]] int loreKnownWeaknesses(const core::LoreEntry& entry, const core::GameState& state);

// 妖兽条目的「掉落」：把它编进敌方的每一场仗的 rewards.drops 并起来（同一物品只列一次，按物品 id 排）。
// 没有 roleId、或哪一场都没编它，返回空表。
[[nodiscard]] std::vector<std::string> loreDrops(const core::LoreEntry& entry,
                                                 const std::map<std::string, core::BattleSetup>& battles);

}  // namespace fanren::rules
