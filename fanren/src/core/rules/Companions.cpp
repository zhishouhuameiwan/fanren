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

int addBattleBond(core::BondRecord& record, int today) {
    if (record.battleDay != today) {
        record.battleDay = today;
        record.battleGainedThatDay = 0;
    }
    const int room = std::max(0, kBondBattleDailyCap - record.battleGainedThatDay);
    const int step = std::min(kBondPerBattleWin, room);
    record.battleGainedThatDay += step;
    const int before = record.value;
    addBond(record, step);
    return record.value - before;
}

GiftTaste giftTaste(const core::Companion& companion, const std::string& itemId) {
    const auto listed = [&itemId](const std::vector<std::string>& items) {
        return std::find(items.begin(), items.end(), itemId) != items.end();
    };
    if (listed(companion.giftsLike)) return GiftTaste::Liked;
    if (listed(companion.giftsDislike)) return GiftTaste::Disliked;
    return GiftTaste::Plain;
}

GiftOutcome giveGift(core::BondRecord& record, const core::Companion& companion, const std::string& itemId,
                     int today) {
    GiftOutcome outcome;
    outcome.taste = giftTaste(companion, itemId);
    if (record.giftDay == today) return outcome;
    record.giftDay = today;
    outcome.given = true;
    const int amount = outcome.taste == GiftTaste::Liked   ? kBondGiftLiked
                       : outcome.taste == GiftTaste::Plain ? kBondGiftPlain
                                                           : kBondGiftDisliked;
    const int before = record.value;
    addBond(record, amount);
    outcome.gained = record.value - before;
    return outcome;
}

}  // namespace fanren::rules
