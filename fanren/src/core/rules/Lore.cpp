#include "core/rules/Lore.h"

#include <algorithm>

namespace fanren::rules {

bool loreUnlocked(const core::LoreEntry& entry, const core::GameState& state) {
    return entry.unlockFlag.empty() || state.flag(entry.unlockFlag) != 0;
}

std::vector<const core::LoreEntry*> unlockedLore(const std::vector<core::LoreEntry>& entries,
                                                 const core::GameState& state) {
    std::vector<const core::LoreEntry*> out;
    for (const core::LoreEntry& entry : entries) {
        if (loreUnlocked(entry, state)) out.push_back(&entry);
    }
    return out;
}

std::set<std::string> unlockedLoreIds(const std::vector<core::LoreEntry>& entries, const core::GameState& state) {
    std::set<std::string> ids;
    for (const core::LoreEntry& entry : entries) {
        if (loreUnlocked(entry, state)) ids.insert(entry.id);
    }
    return ids;
}

std::vector<const core::LoreEntry*> newlyUnlockedLore(const std::vector<core::LoreEntry>& entries,
                                                      const std::set<std::string>& before,
                                                      const std::set<std::string>& after) {
    std::vector<const core::LoreEntry*> out;
    for (const core::LoreEntry& entry : entries) {
        if (after.count(entry.id) != 0 && before.count(entry.id) == 0) out.push_back(&entry);
    }
    return out;
}

int loreKnownWeaknesses(const core::LoreEntry& entry, const core::GameState& state) {
    if (entry.roleId.empty()) return 0;
    return state.knownWeaknessesOf(entry.roleId);
}

std::vector<std::string> loreDrops(const core::LoreEntry& entry,
                                   const std::map<std::string, core::BattleSetup>& battles) {
    std::vector<std::string> drops;
    if (entry.roleId.empty()) return drops;
    for (const auto& [id, battle] : battles) {
        const bool fieldsIt = std::any_of(battle.units.begin(), battle.units.end(), [&entry](const core::BattleUnitSpec& unit) {
            return !unit.ally && unit.roleId == entry.roleId;
        });
        if (!fieldsIt) continue;
        for (const core::BagEntry& drop : battle.reward.drops) {
            if (std::find(drops.begin(), drops.end(), drop.itemId) == drops.end()) drops.push_back(drop.itemId);
        }
    }
    std::sort(drops.begin(), drops.end());
    return drops;
}

}  // namespace fanren::rules
