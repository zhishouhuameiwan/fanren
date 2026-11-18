#include "core/rules/Companions.h"

#include <algorithm>

namespace fanren::rules {

std::string companionOfRole(const std::map<std::string, core::Companion>& companions,
                            const std::string& roleId) {
    if (roleId.empty()) return {};
    for (const auto& [id, companion] : companions) {
        for (const core::CompanionRole& role : companion.roles) {
            if (role.roleId == roleId) return id;
        }
    }
    return {};
}

std::string currentRole(const std::map<std::string, core::Companion>& companions,
                        const std::string& companionId, const core::GameState& state) {
    const auto it = companions.find(companionId);
    if (it == companions.end()) return {};
    for (const core::CompanionRole& role : it->second.roles) {
        if (role.untilFlag.empty() || state.flag(role.untilFlag) == 0) return role.roleId;
    }
    return {};
}

int bondTier(int value) {
    int tier = 0;
    for (const int threshold : kBondTierThresholds) {
        if (value >= threshold) ++tier;
    }
    return std::max(tier, 1);
}

bool addBond(core::BondRecord& record, int amount) {
    if (amount < 0) return false;
    record.value = std::min(kBondMax, record.value + std::min(amount, kBondMax));
    return true;
}

}  // namespace fanren::rules
