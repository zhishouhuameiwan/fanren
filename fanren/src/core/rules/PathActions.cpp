#include "core/rules/PathActions.h"

#include <algorithm>
#include <tuple>

#include "core/rules/Quests.h"
#include "core/rules/Realm.h"

namespace fanren::rules {
namespace {

using core::PathAction;
using core::PathActionKind;

[[nodiscard]] bool anyConditionHolds(const std::vector<core::QuestCondition>& conditions,
                                     const core::GameState& state) {
    return std::any_of(conditions.begin(), conditions.end(),
                       [&state](const core::QuestCondition& c) { return conditionHolds(c, state); });
}

[[nodiscard]] bool done(const PathAction& action, const core::GameState& state) {
    return !action.doneFlag.empty() && state.flag(action.doneFlag) != 0;
}

// 菜单的分组次序。**不要加 default:**：新增一种路径行动时让编译器告警，而不是悄悄落进某一组。
[[nodiscard]] int kindRank(PathActionKind kind) {
    switch (kind) {
        case PathActionKind::Inquire: return 0;
        case PathActionKind::Purchase: return 1;
        case PathActionKind::Challenge: return 2;
    }
    return 3;
}

[[nodiscard]] PathEffect say(const std::string& textKey) {
    PathEffect effect;
    effect.kind = PathEffect::Kind::Say;
    effect.textKey = textKey;
    return effect;
}

[[nodiscard]] PathEffect setFlag(const std::string& flag) {
    PathEffect effect;
    effect.kind = PathEffect::Kind::SetFlag;
    effect.flag = flag;
    return effect;
}

[[nodiscard]] PathEffect giveItem(const core::BagEntry& item) {
    PathEffect effect;
    effect.kind = PathEffect::Kind::GiveItem;
    effect.item = item;
    return effect;
}

}  // namespace

bool pathWindowOpen(const PathAction& action, const core::GameState& state) {
    if (action.when.empty() || !allConditionsHold(action.when, state)) return false;
    return !anyConditionHolds(action.until, state);
}

bool pathActionShown(const PathAction& action, const core::GameState& state) {
    if (!pathWindowOpen(action, state)) return false;
    switch (action.kind) {
        case PathActionKind::Inquire: return true;
        case PathActionKind::Purchase: return !done(action, state);
        case PathActionKind::Challenge: return !action.pending && !done(action, state);
    }
    return false;
}

std::vector<const PathAction*> pathActionsAt(const std::vector<PathAction>& actions,
                                             const core::GameState& state,
                                             const std::string& mapId, const std::string& npc) {
    std::vector<const PathAction*> found;
    for (const PathAction& action : actions) {
        if (action.mapId != mapId || action.npc != npc) continue;
        if (pathActionShown(action, state)) found.push_back(&action);
    }
    std::stable_sort(found.begin(), found.end(), [](const PathAction* a, const PathAction* b) {
        return std::make_tuple(kindRank(a->kind), a->chapter, a->id) <
               std::make_tuple(kindRank(b->kind), b->chapter, b->id);
    });
    return found;
}

PathVerdict pathActionVerdict(const PathAction& action, const core::GameState& state) {
    // 境界编号按大小排得出高低（凡人 0 < 炼气 1–13 < 筑基 21–23 < 结丹 31–33），
    // 所以门槛就是一次整数比较，不必另排一张次序表。
    if (toValue(state.realm) < toValue(action.minRealm)) {
        return PathVerdict{PathBlock::RealmTooLow, action.refuseKey};
    }
    if (action.kind == PathActionKind::Purchase &&
        state.itemCount(kPathCurrencyItemId) < action.price) {
        return PathVerdict{PathBlock::NotEnoughMoney, action.poorKey};
    }
    return PathVerdict{};
}

std::vector<PathEffect> pathActionEffects(const PathAction& action, const core::GameState& state) {
    std::vector<PathEffect> effects;
    switch (action.kind) {
        case PathActionKind::Inquire: {
            effects.push_back(say(action.textKey));
            // 做过的打探只是重看：揭过的破绽、给过的东西、置过的旗标都不再来一遍——
            // 否则按两下 E 就多拿一份物件。
            if (done(action, state)) break;
            for (const core::PathReveal& reveal : action.reveals) {
                PathEffect effect;
                effect.kind = PathEffect::Kind::RevealWeakness;
                effect.reveal = reveal;
                effects.push_back(effect);
            }
            for (const core::BagEntry& item : action.gives) effects.push_back(giveItem(item));
            for (const std::string& flag : action.setFlags) effects.push_back(setFlag(flag));
            effects.push_back(setFlag(action.doneFlag));
            break;
        }
        case PathActionKind::Purchase: {
            effects.push_back(say(action.dealKey));
            PathEffect pay;
            pay.kind = PathEffect::Kind::TakeItem;
            pay.item = core::BagEntry{kPathCurrencyItemId, action.price, 0};
            effects.push_back(pay);
            effects.push_back(giveItem(action.goods));
            effects.push_back(setFlag(action.doneFlag));
            break;
        }
        case PathActionKind::Challenge: {
            effects.push_back(say(action.textKey));
            PathEffect fight;
            fight.kind = PathEffect::Kind::StartBattle;
            fight.battleId = action.battleId;
            effects.push_back(fight);
            break;
        }
    }
    return effects;
}

std::vector<PathEffect> challengeResultEffects(const PathAction& action, bool won) {
    std::vector<PathEffect> effects;
    if (action.kind != PathActionKind::Challenge) return effects;
    if (!won) {
        // 输了不记：条目照旧挂着，歇好了可以再来。奖励一个也不发。
        effects.push_back(say(action.loseKey));
        return effects;
    }
    effects.push_back(say(action.winKey));
    if (action.rewardCultivation > 0) {
        PathEffect gain;
        gain.kind = PathEffect::Kind::GainCultivation;
        gain.amount = action.rewardCultivation;
        effects.push_back(gain);
    }
    for (const core::BagEntry& item : action.rewardItems) effects.push_back(giveItem(item));
    effects.push_back(setFlag(action.doneFlag));
    return effects;
}

std::vector<core::PathReveal> pathRevealedWeaknesses(const std::vector<PathAction>& actions,
                                                     const core::GameState& state) {
    std::vector<core::PathReveal> revealed;
    for (const PathAction& action : actions) {
        if (action.kind != PathActionKind::Inquire || !done(action, state)) continue;
        revealed.insert(revealed.end(), action.reveals.begin(), action.reveals.end());
    }
    const auto key = [](const core::PathReveal& r) { return std::tie(r.roleId, r.category); };
    std::sort(revealed.begin(), revealed.end(),
              [&key](const core::PathReveal& a, const core::PathReveal& b) { return key(a) < key(b); });
    revealed.erase(std::unique(revealed.begin(), revealed.end(),
                               [&key](const core::PathReveal& a, const core::PathReveal& b) {
                                   return key(a) == key(b);
                               }),
                   revealed.end());
    return revealed;
}

std::vector<const PathAction*> pendingChallenges(const std::vector<PathAction>& actions) {
    std::vector<const PathAction*> pending;
    for (const PathAction& action : actions) {
        if (action.kind == PathActionKind::Challenge && action.pending) pending.push_back(&action);
    }
    std::stable_sort(pending.begin(), pending.end(), [](const PathAction* a, const PathAction* b) {
        return std::tie(a->chapter, a->id) < std::tie(b->chapter, b->id);
    });
    return pending;
}

}  // namespace fanren::rules
