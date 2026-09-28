#include "core/rules/Encounter.h"

#include <algorithm>
#include <random>

namespace fanren::rules {

std::string step(const EncounterTable& table, EncounterState& state, Realm realm,
                 int currentDay, std::uint32_t seed) {
    // 跨天重置每日上限：不重置的话，玩家挂机刷够一次上限后就永久失效了。
    if (currentDay != state.lastDay) {
        state.triggeredToday = 0;
        state.lastDay = currentDay;
    }

    ++state.stepsSinceLast;

    if (state.stepsUntilNext <= 0) {
        // 步数区间随机化，不能固定步数——固定的话玩家数着步子就能掐点刷。
        const int lo = std::min(table.stepsMin, table.stepsMax);
        const int hi = std::max(table.stepsMin, table.stepsMax);
        std::mt19937 thresholdRng(seed);
        std::uniform_int_distribution<int> stepsDist(lo, hi);
        state.stepsUntilNext = std::max(1, stepsDist(thresholdRng));
    }

    if (state.stepsSinceLast < state.stepsUntilNext) return {};
    if (state.triggeredToday >= table.dailyCap) return {};   // 每日上限，防刷

    // 境界过滤：低阶遭遇在高境界不该再出现，否则后期满地都是杂兵。
    std::vector<const EncounterEntry*> candidates;
    int totalWeight = 0;
    for (const EncounterEntry& entry : table.entries) {
        if (entry.weight <= 0) continue;
        if (toValue(realm) < toValue(entry.minRealm) || toValue(realm) > toValue(entry.maxRealm)) {
            continue;
        }
        candidates.push_back(&entry);
        totalWeight += entry.weight;
    }
    if (candidates.empty() || totalWeight <= 0) {
        // 空表，或本境界下全部条目都被过滤掉：不触发，也不消耗这次「本该
        // 触发」的机会，让 stepsSinceLast/stepsUntilNext 原样保留——等下次
        // 真正有合适条目再判定。不崩溃、不死循环（本函数本身是直线执行，
        // 没有可能失控的循环体）。
        return {};
    }

    std::mt19937 pickRng(seed);
    std::uniform_int_distribution<int> pick(0, totalWeight - 1);
    int roll = pick(pickRng);
    const EncounterEntry* chosen = candidates.back();
    for (const EncounterEntry* entry : candidates) {
        if (roll < entry->weight) {
            chosen = entry;
            break;
        }
        roll -= entry->weight;
    }

    state.stepsSinceLast = 0;
    state.stepsUntilNext = 0;
    ++state.triggeredToday;
    return chosen->battleId;
}

}  // namespace fanren::rules
