// AI。纯函数式：只读局面，返回一个动作，不碰任何成员（docs/octopath-battle.md 第 3 节）。
//
// 每个候选动作在返回前都过一遍 isLegal，因此 AI 永远不会给出 apply 会拒绝的
// 动作——这是把「合法性」这件事只留一个判定入口带来的直接好处。
//
// 两套：
//   * 敌方——到了蓄势的节奏就宣告；有伤人的法术且法力够就按伤害挑一门，否则普攻；
//     目标能击倒的优先，否则按种子随机、偏向气血低的。
//   * 我方——只给无头的 runToCompletion 用（真玩家自己按菜单）：破势的目标蓄满劲打；
//     打得中已揭开破绽的就打它；还有没揭开的破绽就拿没试过的兵刃 / 五行去探；
//     劲攒满 5 点时蓄 1 点出手，免得白白溢出。**不吃药**——吃药是通关测试里
//     「那只手」的事，不在规则层替玩家做这个决定。
#include "core/battle/Battle.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace fanren::core::battle {
namespace {

// actorIndex 保持 -1 的空动作：表示「没什么可做的」，驱动方据此直接 endTurn()。
[[nodiscard]] Action noAction() { return Action{}; }

[[nodiscard]] Action makeAction(ActionKind kind, int actorIndex, int targetIndex = -1) {
    Action a;
    a.kind = kind;
    a.actorIndex = actorIndex;
    a.targetIndex = targetIndex;
    return a;
}

}  // namespace

std::uint64_t BattleState::aiRoll(int actorIndex) const {
    // 种子、回合、行动序的位置、行动者一起派生：同一局面每次掷出同一个数，
    // 换一个回合、换一个人就换一个数。不碰 rng_，decideAi 因此仍是 const。
    std::uint64_t h = mixBits(seed_ ^ 0xA11CE5ULL);
    h = mixBits(h ^ static_cast<std::uint64_t>(static_cast<std::uint32_t>(round_)));
    h = mixBits(h ^ (static_cast<std::uint64_t>(orderPos_) << 20U) ^
                static_cast<std::uint64_t>(static_cast<std::uint32_t>(actorIndex)));
    return h;
}

Action BattleState::decideAi(int actorIndex) const {
    if (!validIndex(actorIndex)) return noAction();
    const Unit& u = unitRef(actorIndex);
    if (!u.alive() || actorIndex != currentActor() || slotUsed_) return noAction();
    return u.ally ? allyDecision(actorIndex) : enemyDecision(actorIndex);
}

// ---------------------------------------------------------------------------
// 敌方
// ---------------------------------------------------------------------------

Action BattleState::enemyDecision(int actorIndex) const {
    const Unit& u = unitRef(actorIndex);

    // 1. 蓄势：到了节奏就宣告，不打人。节奏本身写在合法性里（checkCharge），这里只问行不行。
    if (u.chargeEvery > 0) {
        const Action charge = makeAction(ActionKind::Charge, actorIndex);
        if (isLegal(charge)) return charge;
    }

    // 2. 每个目标上最重的那一手：有伤人的法术且法力够就在法术里挑，否则普攻。
    //    perTarget 与单位下标对齐；够不着（倒下、同伴）的那一格 actorIndex 为 -1。
    std::vector<Action> perTarget(units_.size());
    bool any = false;
    for (std::size_t i = 0; i < units_.size(); ++i) {
        const int target = static_cast<int>(i);
        Action best;   // actorIndex -1：还没有
        int bestDamage = -1;
        // magics_ 是 std::map，按 id 有序遍历；配合「严格大于」，同分时取 id 小的。
        for (const auto& entry : magics_) {
            Action cast = makeAction(ActionKind::Cast, actorIndex, target);
            cast.magicId = entry.first;
            if (!isLegal(cast)) continue;
            const int damage = estimateHitDamage(cast, target) * hitCount(cast);
            if (damage > bestDamage) {
                best = cast;
                bestDamage = damage;
            }
        }
        if (best.actorIndex < 0) {
            const Action attack = makeAction(ActionKind::Attack, actorIndex, target);
            if (isLegal(attack)) best = attack;
        }
        if (best.actorIndex >= 0) {
            perTarget[i] = best;
            any = true;
        }
    }
    if (!any) return noAction();
    const int target = enemyTarget(actorIndex, perTarget);
    return perTarget[static_cast<std::size_t>(target)];
}

int BattleState::enemyTarget(int actorIndex, const std::vector<Action>& perTarget) const {
    // 能击倒的优先：一手打得死的里挑气血最低的（同分取下标小的）。
    int killable = -1;
    for (std::size_t i = 0; i < perTarget.size(); ++i) {
        const Action& option = perTarget[i];
        if (option.actorIndex < 0) continue;
        const Unit& t = units_[i];
        const int damage = estimateHitDamage(option, static_cast<int>(i)) * hitCount(option);
        if (damage < t.hp) continue;
        if (killable < 0 || t.hp < unitRef(killable).hp) killable = static_cast<int>(i);
    }
    if (killable >= 0) return killable;

    // 都击不倒：按种子随机，权重与气血成反比——偏向气血低的，但不是非它不打。
    // 从前（战棋）是「永远打最脆的那个」，于是曲魂一上场就被五个人追着打，
    // 那是 AI 的脾气替平衡做了决定；现在它仍偏向脆的，但不再是一个定数。
    std::vector<std::uint64_t> weights(perTarget.size(), 0);
    std::uint64_t total = 0;
    for (std::size_t i = 0; i < perTarget.size(); ++i) {
        if (perTarget[i].actorIndex < 0) continue;
        weights[i] = 1000000ULL / static_cast<std::uint64_t>(std::max(1, units_[i].hp));
        weights[i] = std::max<std::uint64_t>(1, weights[i]);
        total += weights[i];
    }
    // roll < total，所以一定停在某一个有效目标上；pick 记的是「走到哪一个了」。
    std::uint64_t roll = aiRoll(actorIndex) % total;
    int pick = -1;
    for (std::size_t i = 0; i < weights.size(); ++i) {
        if (weights[i] == 0) continue;
        pick = static_cast<int>(i);
        if (roll < weights[i]) break;
        roll -= weights[i];
    }
    return pick;
}

// ---------------------------------------------------------------------------
// 我方（无头自动战）
// ---------------------------------------------------------------------------

Action BattleState::allyDecision(int actorIndex) const {
    const Unit& u = unitRef(actorIndex);

    // 候选：对每个站着的敌人，每一样兵刃一手普攻、每一门会的法术一手。先不蓄劲。
    std::vector<Action> options;
    for (std::size_t i = 0; i < units_.size(); ++i) {
        const Unit& t = units_[i];
        if (!t.alive() || t.ally == u.ally) continue;
        const int target = static_cast<int>(i);
        std::vector<int> weapons = categoryBits(u.weapons);
        if (weapons.empty() || devour_) weapons = {kCategoryNone};
        for (const int weapon : weapons) {
            Action attack = makeAction(ActionKind::Attack, actorIndex, target);
            attack.category = weapon;
            if (isLegal(attack)) options.push_back(attack);
        }
        for (const auto& entry : magics_) {
            Action cast = makeAction(ActionKind::Cast, actorIndex, target);
            cast.magicId = entry.first;
            if (isLegal(cast)) options.push_back(cast);
        }
    }
    if (options.empty()) {
        const Action defend = makeAction(ActionKind::Defend, actorIndex);
        return isLegal(defend) ? defend : noAction();
    }

    const auto totalDamage = [this](const Action& a) {
        return estimateHitDamage(a, a.targetIndex) * hitCount(a);
    };
    // 蓄劲：给定一手，按局面挑蓄几点，并且保证合法（劲不够就少蓄）。
    const auto withBoost = [this, &u](Action a, int wanted) {
        a.boost = std::clamp(wanted, 0, std::min(kBoostMax, u.bp));
        while (a.boost > 0 && !isLegal(a)) --a.boost;
        return a;
    };
    // 平时：劲满了蓄一点，免得下一回合那一点白白溢出；否则攒着留给破势。
    const int idleBoost = u.bp >= kBpMax ? 1 : 0;
    // 同分时的偏好：普攻在前（不花法力），再按伤害、再按候选序（目标下标、兵刃位序）。
    const auto better = [&totalDamage](const Action& a, const Action& b) {
        const bool aAttack = a.kind == ActionKind::Attack;
        const bool bAttack = b.kind == ActionKind::Attack;
        if (aAttack != bAttack) return aAttack;
        return totalDamage(a) > totalDamage(b);
    };

    // 1. 有破势的敌人：挑伤害最重的那一手，劲蓄满——破势期间伤害 ×2，这是蓄劲最值的时候。
    {
        const Action* best = nullptr;
        for (const Action& a : options) {
            if (!unitRef(a.targetIndex).broken()) continue;
            Action boosted = withBoost(a, kBoostMax);
            if (best == nullptr || totalDamage(boosted) > totalDamage(withBoost(*best, kBoostMax))) {
                best = &a;
            }
        }
        if (best != nullptr) return withBoost(*best, kBoostMax);
    }

    // 2. 打得中已揭开的破绽：先挑架势剩得最少的那个敌人（离破势最近），再挑这一手。
    {
        const Action* best = nullptr;
        for (const Action& a : options) {
            const Unit& t = unitRef(a.targetIndex);
            if (t.maxToughness <= 0 || (hitCategories(a) & t.revealed) == 0) continue;
            if (best == nullptr) {
                best = &a;
                continue;
            }
            const Unit& bt = unitRef(best->targetIndex);
            if (t.toughness != bt.toughness) {
                if (t.toughness < bt.toughness) best = &a;
                continue;
            }
            if (t.hp != bt.hp) {
                if (t.hp < bt.hp) best = &a;
                continue;
            }
            if (a.targetIndex == best->targetIndex && better(a, *best)) best = &a;
        }
        if (best != nullptr) return withBoost(*best, idleBoost);
    }

    // 3. 还有没揭开的破绽：拿没试过的类别去探（普攻先于法术，气血低的先探）。
    {
        const Action* best = nullptr;
        for (const Action& a : options) {
            const Unit& t = unitRef(a.targetIndex);
            if (t.maxToughness <= 0 || (t.weaknesses & ~t.revealed) == 0) continue;
            if ((hitCategories(a) & ~t.tested) == 0) continue;
            if (best == nullptr) {
                best = &a;
                continue;
            }
            const bool aAttack = a.kind == ActionKind::Attack;
            const bool bAttack = best->kind == ActionKind::Attack;
            if (aAttack != bAttack) {
                if (aAttack) best = &a;
                continue;
            }
            const Unit& bt = unitRef(best->targetIndex);
            if (t.hp < bt.hp) best = &a;
        }
        if (best != nullptr) return withBoost(*best, idleBoost);
    }

    // 4. 都没有：打气血最低的那个，挑最重的一手（普攻优先，不白花法力）。
    const Action* best = nullptr;
    for (const Action& a : options) {
        if (best == nullptr) {
            best = &a;
            continue;
        }
        const Unit& t = unitRef(a.targetIndex);
        const Unit& bt = unitRef(best->targetIndex);
        if (t.hp != bt.hp) {
            if (t.hp < bt.hp) best = &a;
            continue;
        }
        if (a.targetIndex == best->targetIndex && better(a, *best)) best = &a;
    }
    return withBoost(*best, idleBoost);
}

}  // namespace fanren::core::battle
