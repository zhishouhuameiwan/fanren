#pragma once
// 通关测试与平衡扫描共用的「那只手」（第 3、4、5 章）：替玩家在战斗里按菜单出手。
//
// ---- 为什么只许有一份 ----
// 从前第 4、5 章各有一只手，两份拷贝悄悄走散过（第 5 章那只多了「逃」，第 4 章那只多了
// 「再挨一轮就死」），而且都不会用横版这一套：不蓄劲、不挑破绽，等于放弃了破势 ×2 与连击，
// 量出来的是手的脾气（docs/octopath-battle.md 9.2 第 2 条）。第 3 章从前干脆让规则层的
// 我方 AI 自己打，那一套不吃药、不撒毒。三章如今共用这一只，取舍全在 HandPolicy 里写明。
//
// ---- 它怎么打（docs/octopath-battle.md 第 10 节是同一张表，改一处要改两处）----
//   1. 吃药：按物品 id 找（policy.pills 的先后）；气血不高于门槛就吃，policy.healWhenDoomed
//      时再加一条兜底——到他下一次出手之前对面打得出的上界不小于当前气血也吃。只喂韩立
//     （policy.feedCompanion 是扫描用的另一种取舍）。
//   2. 逃：policy.fleeWhenSpent 时，半血以下又没药可吃、这一场能逃就逃；任何一场里「手上
//      没有一样东西还伤得了对面」（欧阳飞天没练剑符那一侧，火弹要不了他的命）也走。
//   3. 首领蓄势：打得死就打死；这一手破得了势就破（劲蓄够再蓄满）；都不行而重招会在他下一次
//      出手之前落下来，就防御；落在那之后，就先削它的架势。
//   4. 出手：打得死的先打死（挑最凶的，劲蓄最少）→ 这一手破得了势就破（蓄满，破势之后那几击
//      吃 ×2）→ 已经破势的蓄满打 → 打已知破绽（架势剩得最少的先）→ 还有没揭开的破绽就拿
//      没试过的类别去探（凡人先试兵刃、修仙界的先试法术，毒药最后）→ 伤害最重的一手。
//      劲攒满 5 点时至少蓄 1 点。
//   毒药只撒 policy.poisons 里的那几样：七毒水、五毒水是第 3 章剧情要用的，不许它顺手扔掉。
//
// 敌人一律走 BattleState::decideAi（BattleScene::runToAllyTurn 里）；这只手只替我方按菜单，
// 每一手都收在 issuePlayerAction，与玩家按键同一条路，背包因此照真扣。
// 伤害、破绽、击数一律问规则层（estimateHitDamage / hitCategories / hitCount），不另算一套。
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "core/battle/Battle.h"
#include "core/model/AttackCategory.h"
#include "core/model/Types.h"
#include "game/Application.h"
#include "game/BattleScene.h"

namespace fanren::test {

inline constexpr const char* kHandHero = "hanli";
inline constexpr const char* kHandYangjingPill = "pill_yangjing_dan";
inline constexpr const char* kHandShixinPoison = "pill_shixin_san";

// 这只手的全部取舍。缺省是第 4 章通关测试那一只（门槛六成 ＋ 再挨一轮就死）；
// 第 5 章的意图玩家改三项：门槛五成、不算下一轮、没药能逃就逃。
struct HandPolicy {
    int healAtPercent = 60;
    bool healWhenDoomed = true;
    std::vector<std::string> pills{kHandYangjingPill};
    std::vector<std::string> poisons{kHandShixinPoison};
    bool feedCompanion = false;
    bool fleeWhenSpent = false;
};

class BattleHand {
public:
    using Action = core::battle::Action;
    using ActionKind = core::battle::ActionKind;
    using BattlePhase = core::battle::BattlePhase;
    using BattleState = core::battle::BattleState;
    using Unit = core::battle::Unit;

    explicit BattleHand(game::Application& app, HandPolicy policy = {})
        : app_(&app), policy_(std::move(policy)) {}

    [[nodiscard]] HandPolicy& policy() { return policy_; }
    [[nodiscard]] const HandPolicy& policy() const { return policy_; }
    // 「这一场一开打就逃」（第 5 章「逃了之后还往下走吗」那一组用）。只管下一场 play。
    void fleeAtOnce(bool on) { fleeAtOnce_ = on; }

    // 把一场仗打到分出胜负。
    BattlePhase play(game::BattleScene& scene) {
        for (int step = 0; step < kMaxSteps; ++step) {
            if (scene.battle().phase() != BattlePhase::Ongoing) break;
            const int actor = scene.runToAllyTurn();
            if (actor < 0) break;
            if (!act(scene, actor)) {
                scene.closeMenu();
                break;   // 菜单全灰、防御也点不动：别空转
            }
        }
        fleeAtOnce_ = false;
        return scene.battle().phase();
    }

    // 已轮到 actor：出一手。返回 false 表示一件也做不了。
    bool act(game::BattleScene& scene, int actor) {
        if (fleeAtOnce_ && issue(scene, make(ActionKind::Escape, actor))) return true;
        if (tryHeal(scene, actor)) return true;
        const Action planned = decide(scene.battle(), actor);
        if (planned.actorIndex >= 0 && issue(scene, planned)) return true;
        return issue(scene, make(ActionKind::Defend, actor));
    }

    // 吃药那一步（第 1 条）。吃了返回 true。
    bool tryHeal(game::BattleScene& scene, int actor) {
        const BattleState& b = scene.battle();
        const Unit& me = unitAt(b, actor);
        if (me.id != kHandHero) {
            return policy_.feedCompanion && low(me) && feed(scene, actor, actor);
        }
        const bool doomed = policy_.healWhenDoomed && me.hp <= incomingUpperBound(b, actor);
        if ((low(me) || doomed) && feed(scene, actor, actor)) return true;
        if (!policy_.feedCompanion) return false;
        for (std::size_t i = 0; i < b.units().size(); ++i) {
            const Unit& other = b.units()[i];
            if (!other.ally || !other.alive() || other.id == kHandHero || !low(other)) continue;
            if (feed(scene, actor, static_cast<int>(i))) return true;
        }
        return false;
    }

    // 不吃药时这一手打什么（第 2–4 条）。纯函数：只读局面。
    [[nodiscard]] Action decide(const BattleState& b, int actor) const {
        const Unit& me = unitAt(b, actor);
        // 识海里没有破绽与劲，体积就是一切：照规则层那一套咬最小的。
        if (b.devourMode()) return b.decideAi(actor);
        const Action escape = make(ActionKind::Escape, actor);
        if (policy_.fleeWhenSpent && me.id == kHandHero && low(me) && legal(b, escape)) return escape;

        std::vector<Scored> options = scoredOptions(b, actor);
        if (options.empty()) return make(ActionKind::Defend, actor);
        // 毒不算：毒只削不杀（kPoisonFloorHp），把人毒到一口气也赢不下来。
        const bool progress = std::any_of(options.begin(), options.end(), [](const Scored& s) {
            return s.dealt > 0 || s.breaks;
        });
        if (!progress && legal(b, escape)) return escape;

        for (std::size_t i = 0; i < b.units().size(); ++i) {
            const Unit& boss = b.units()[i];
            if (!boss.alive() || boss.ally == me.ally || !boss.charging) continue;
            const int bossIndex = static_cast<int>(i);
            if (const Scored* kill = cheapest(options, bossIndex, &Scored::kills)) return kill->action;
            if (const Scored* brk = cheapest(options, bossIndex, &Scored::breaks)) {
                return topUp(b, options, *brk);
            }
            if (heavyLandsFirst(b, actor, bossIndex)) return make(ActionKind::Defend, actor);
            if (const Scored* chip = bestChip(b, options, bossIndex, /*maxBoost=*/true)) {
                return chip->action;
            }
        }

        if (const Scored* kill = bestKill(b, options)) return kill->action;
        if (const Scored* brk = bestBreak(b, options)) return topUp(b, options, *brk);
        if (const Scored* hit = bestOnBroken(b, options)) return hit->action;
        if (const Scored* chip = bestChip(b, options, -1, /*maxBoost=*/false)) return chip->action;
        if (const Scored* probe = bestProbe(b, options)) return probe->action;
        return bestDamage(b, options)->action;
    }

    // 从此刻到 target 下一次出手之前，对面在他身上打得出的上界（外加他自己下一次毒发）。
    // 横版里敌人每回合都出手，所以这笔账按**行动序**数：这一回合排在他后面的每一位，加上
    // 下一回合排在他前面的每一位（nextRoundOrder 就是下一回合真用的那个序）——身法相近的
    // 敌人可能在他两次出手之间动两次，按「每人一回合一次」数会漏。每一位按那个敌人此刻最重的
    // 一手算；正在蓄势的，它的下一位按重招算（蓄势的宣告本身不伤人，所以「宣告一手、重招一手」
    // 不会比「两手普通的」更重——倍数都不超过 2）。当前这一波只剩最后一个时，下一波整波
    // 也算进来：打倒它的那一下之后，新进场的人下一回合就出手，可能排在他前头。
    // 口径故意取上界，宁可多吃一颗，不在再挨一轮就死的时候还去砍人。
    [[nodiscard]] int incomingUpperBound(const BattleState& b, int target) const {
        const Unit& t = unitAt(b, target);
        int total = t.poison > 0 ? t.poisonPower : 0;
        std::vector<int> window;
        const std::vector<int>& order = b.roundOrder();
        for (std::size_t i = b.orderPosition() + 1; i < order.size(); ++i) window.push_back(order[i]);
        for (const int who : b.nextRoundOrder()) {
            if (who == target) break;
            window.push_back(who);
        }
        std::vector<bool> struckOnce(b.units().size(), false);
        for (const int who : window) {
            const Unit& e = unitAt(b, who);
            if (!e.alive() || e.ally == t.ally) continue;
            const auto at = static_cast<std::size_t>(who);
            total += struckOnce[at] || !e.charging ? plainHit(b, who, target) : worstHit(b, who, target);
            struckOnce[at] = true;
        }
        int standing = 0;
        for (const Unit& e : b.units()) {
            if (e.alive() && e.ally != t.ally) ++standing;
        }
        if (standing > 1) return total;
        for (std::size_t i = 0; i < b.units().size(); ++i) {
            const Unit& e = b.units()[i];
            if (e.ally == t.ally || e.onField || e.hp <= 0 || e.wave != b.currentWave() + 1) continue;
            total += worstHit(b, static_cast<int>(i), target) * e.actions;
        }
        return total;
    }

    // 把一手从菜单里点出去：蓄劲 → 根菜单 →（兵刃 / 法术 / 物品）→ 目标，每一级都走
    // menuChoose，最终收在 issuePlayerAction。点不动返回 false，菜单收起。
    bool issue(game::BattleScene& scene, const Action& a) {
        game::Application& app = *app_;
        scene.openMenu(app);
        if (scene.menuMode() != game::BattleMenuMode::Root) return false;
        bool ok = false;
        switch (a.kind) {
            case ActionKind::Defend:
                ok = scene.menuChoose(app, game::kBattleMenuDefend);
                break;
            case ActionKind::Escape:
                ok = scene.menuChoose(app, game::kBattleMenuEscape);
                break;
            case ActionKind::Attack:
                scene.setBoost(a.boost);
                ok = scene.menuChoose(app, game::kBattleMenuAttack) && chooseWeapon(scene, a) &&
                     chooseTarget(scene, a);
                break;
            case ActionKind::Cast:
                scene.setBoost(a.boost);
                ok = scene.menuChoose(app, game::kBattleMenuCast) && chooseListed(scene, a, true) &&
                     chooseTarget(scene, a);
                break;
            case ActionKind::Item:
                ok = scene.menuChoose(app, game::kBattleMenuItem) && chooseListed(scene, a, false) &&
                     chooseTarget(scene, a);
                break;
            case ActionKind::Charge:
                break;
        }
        if (!ok) scene.closeMenu();
        return ok;
    }

private:
    static constexpr int kMaxSteps = 8000;

    // 一手连同它的预估后果（只凭**看得见**的东西估：已揭开的破绽，不偷看没揭开的）。
    struct Scored {
        Action action;
        int dealt = 0;          // 这一手在目标身上实打的气血
        bool kills = false;     // 这一手打得死它
        bool breaks = false;    // 这一手把它打到破势
        bool knownWeak = false; // 打的是已揭开的破绽
        bool probes = false;    // 打的里头有这个目标身上没试过的类别，而它还有没揭开的破绽
        bool poisons = false;   // 毒药：这一下挂得上毒（它眼下没中毒）
    };

    [[nodiscard]] static const Unit& unitAt(const BattleState& b, int index) {
        return b.units()[static_cast<std::size_t>(index)];
    }

    [[nodiscard]] static Action make(ActionKind kind, int actor, int target = -1) {
        Action a;
        a.kind = kind;
        a.actorIndex = actor;
        a.targetIndex = target;
        return a;
    }

    [[nodiscard]] bool low(const Unit& u) const {
        return u.hp * 100 <= u.maxHp * policy_.healAtPercent;
    }

    // 玩家那一侧的判据：背包闸在前、规则层 checkLegal 在后（与菜单置灰同一个函数）。
    [[nodiscard]] bool legal(const BattleState& b, const Action& a) const {
        return game::refusePlayerAction(app_->state(), b, a).empty();
    }

    [[nodiscard]] static int attackCost(const Scored& s) {
        // 同样打得死 / 破得了时的先后：平砍不花法力、不花东西，法术次之，毒药最后。
        switch (s.action.kind) {
            case ActionKind::Attack:
                return 0;
            case ActionKind::Cast:
                return 1;
            case ActionKind::Item:
            case ActionKind::Defend:
            case ActionKind::Escape:
            case ActionKind::Charge:
                return 2;
        }
        return 2;
    }

    // 这个敌人每回合在我方身上最重的一手 × 行动次数（打谁按 actor 那一边的韩立算，他不在场按
    // 我方第一个站着的人算）。拿来挑「先收拾谁」。
    [[nodiscard]] int threat(const BattleState& b, int enemy) const {
        int victim = -1;
        for (std::size_t i = 0; i < b.units().size(); ++i) {
            const Unit& u = b.units()[i];
            if (!u.ally || !u.alive()) continue;
            if (victim < 0 || u.id == kHandHero) victim = static_cast<int>(i);
            if (u.id == kHandHero) break;
        }
        if (victim < 0) return 0;
        return worstHit(b, enemy, victim) * unitAt(b, enemy).actions;
    }

    [[nodiscard]] int worstHit(const BattleState& b, int enemy, int target) const {
        const Unit& e = unitAt(b, enemy);
        Action swing = make(ActionKind::Attack, enemy, target);
        int worst = b.estimateHitDamage(swing, target);
        for (const std::string& id : e.magics) {
            const core::Magic* m = b.findMagic(id);
            if (m == nullptr || e.mp < m->needMp) continue;
            Action cast = make(ActionKind::Cast, enemy, target);
            cast.magicId = id;
            worst = std::max(worst, b.estimateHitDamage(cast, target) * b.hitCount(cast));
        }
        return worst;
    }

    // 同上，但不按重招算（重招那一手之后的几手是普通的）。
    [[nodiscard]] int plainHit(const BattleState& b, int enemy, int target) const {
        const Unit& e = unitAt(b, enemy);
        const int worst = worstHit(b, enemy, target);
        return e.charging ? static_cast<int>(std::lround(worst / e.chargeMult)) : worst;
    }

    // 重招会不会在 actor 下一次出手之前落下来：这一回合它在我之后还有位，或者下一回合它排在我前头。
    [[nodiscard]] static bool heavyLandsFirst(const BattleState& b, int actor, int boss) {
        const std::vector<int>& order = b.roundOrder();
        for (std::size_t i = b.orderPosition() + 1; i < order.size(); ++i) {
            if (order[i] == boss) return true;
        }
        for (const int who : b.nextRoundOrder()) {
            if (who == actor) return false;
            if (who == boss) return true;
        }
        return false;
    }

    // 估一手的后果。凭的是此刻的局面与**已揭开**的破绽；破势之后的那几击按 ×2 估。
    [[nodiscard]] Scored score(const BattleState& b, const Action& a) const {
        Scored s;
        s.action = a;
        const Unit& t = unitAt(b, a.targetIndex);
        const int cats = b.hitCategories(a);
        s.knownWeak = t.maxToughness > 0 && (cats & t.revealed) != 0;
        s.probes = t.maxToughness > 0 && (t.weaknesses & ~t.revealed) != 0 && (cats & ~t.tested) != 0;
        bool broken = t.broken();
        int tough = t.toughness;
        if (a.kind == ActionKind::Item) {
            s.poisons = t.poison == 0;
            s.breaks = s.knownWeak && !broken && tough <= 1;
            return s;
        }
        const bool lethal = t.killableBy == 0 || (cats & t.killableBy) != 0;
        const int first = b.estimateHitDamage(a, a.targetIndex);
        int hp = t.hp;
        for (int hit = 0; hit < b.hitCount(a); ++hit) {
            const int damage = broken && !t.broken() ? first * 2 : first;
            hp = lethal ? hp - damage : std::max(1, hp - damage);
            if (hp <= 0) {
                s.kills = true;
                break;
            }
            if (s.knownWeak && !broken && --tough <= 0) {
                broken = true;
                s.breaks = true;
            }
        }
        s.dealt = t.hp - std::max(0, hp);
        return s;
    }

    // 每个站着的敌人 × 每一样兵刃 / 每一门伤人的法术 / 每一样许撒的毒药，劲从 0 蓄到能蓄的最多。
    [[nodiscard]] std::vector<Scored> scoredOptions(const BattleState& b, int actor) const {
        const Unit& me = unitAt(b, actor);
        const int maxBoost = std::min(core::battle::kBoostMax, me.bp);
        std::vector<Scored> out;
        const auto add = [&](Action a, bool boostable) {
            for (int boost = 0; boost <= (boostable ? maxBoost : 0); ++boost) {
                a.boost = boost;
                if (legal(b, a)) out.push_back(score(b, a));
            }
        };
        for (std::size_t i = 0; i < b.units().size(); ++i) {
            const Unit& t = b.units()[i];
            if (!t.alive() || t.ally == me.ally) continue;
            const int target = static_cast<int>(i);
            std::vector<int> weapons = core::categoryBits(me.weapons);
            if (weapons.empty()) weapons.push_back(core::kCategoryNone);
            for (const int weapon : weapons) {
                Action attack = make(ActionKind::Attack, actor, target);
                attack.category = weapon;
                add(attack, true);
            }
            for (const std::string& id : me.magics) {
                const core::Magic* m = b.findMagic(id);
                if (m == nullptr || (m->power <= 0 && m->poison <= 0)) continue;
                Action cast = make(ActionKind::Cast, actor, target);
                cast.magicId = id;
                add(cast, true);
            }
            if (me.id != kHandHero) continue;   // 背包在韩立身上
            for (const std::string& id : policy_.poisons) {
                if (!b.hasItem(id)) continue;
                Action toss = make(ActionKind::Item, actor, target);
                toss.magicId = id;
                add(toss, false);
            }
        }
        return out;
    }

    // 劲攒满 5 点时至少蓄 1 点：下一回合开头那一点就不会白白溢出。
    [[nodiscard]] int idleBoost(const BattleState& b, const Action& a) const {
        const bool boostable = a.kind == ActionKind::Attack || a.kind == ActionKind::Cast;
        return boostable && unitAt(b, a.actorIndex).bp >= core::battle::kBpMax ? 1 : 0;
    }

    // options 里与 base 同一手（同目标、同类、同兵刃或同一门法术）而劲蓄 boost 点的那一条。
    [[nodiscard]] static const Scored* sameWithBoost(const std::vector<Scored>& options,
                                                     const Action& base, int boost) {
        for (const Scored& s : options) {
            const Action& a = s.action;
            if (a.kind == base.kind && a.targetIndex == base.targetIndex &&
                a.category == base.category && a.magicId == base.magicId && a.boost == boost) {
                return &s;
            }
        }
        return nullptr;
    }

    // 同一手，劲蓄到能蓄的最多（破势之后那几击吃 ×2）。
    [[nodiscard]] static Action topUp(const BattleState& b, const std::vector<Scored>& options,
                                      const Scored& chosen) {
        const int most = std::min(core::battle::kBoostMax, unitAt(b, chosen.action.actorIndex).bp);
        for (int boost = most; boost > chosen.action.boost; --boost) {
            if (const Scored* s = sameWithBoost(options, chosen.action, boost)) return s->action;
        }
        return chosen.action;
    }

    // 在 target 身上（-1 = 不限）满足 flag 的里头，劲蓄得最少、成本最低的一条。
    [[nodiscard]] static const Scored* cheapest(const std::vector<Scored>& options, int target,
                                                bool Scored::*flag) {
        const Scored* best = nullptr;
        for (const Scored& s : options) {
            if (!(s.*flag) || (target >= 0 && s.action.targetIndex != target)) continue;
            if (best == nullptr || s.action.boost < best->action.boost ||
                (s.action.boost == best->action.boost && attackCost(s) < attackCost(*best))) {
                best = &s;
            }
        }
        return best;
    }

    // 打得死的：先挑最凶的那个敌人，再挑劲蓄得最少、成本最低的一手。
    [[nodiscard]] const Scored* bestKill(const BattleState& b, const std::vector<Scored>& options) const {
        return bestByThreat(b, options, &Scored::kills);
    }

    [[nodiscard]] const Scored* bestBreak(const BattleState& b, const std::vector<Scored>& options) const {
        return bestByThreat(b, options, &Scored::breaks);
    }

    [[nodiscard]] const Scored* bestByThreat(const BattleState& b, const std::vector<Scored>& options,
                                             bool Scored::*flag) const {
        int bestTarget = -1;
        int bestThreat = -1;
        for (const Scored& s : options) {
            if (!(s.*flag)) continue;
            const int th = threat(b, s.action.targetIndex);
            if (th > bestThreat) {
                bestThreat = th;
                bestTarget = s.action.targetIndex;
            }
        }
        return bestTarget < 0 ? nullptr : cheapest(options, bestTarget, flag);
    }

    // 已经破势的：蓄满，挑实打最多的一手。
    [[nodiscard]] static const Scored* bestOnBroken(const BattleState& b, const std::vector<Scored>& options) {
        const Scored* best = nullptr;
        for (const Scored& s : options) {
            if (!unitAt(b, s.action.targetIndex).broken() || s.action.kind == ActionKind::Item) continue;
            if (best == nullptr || s.action.boost > best->action.boost ||
                (s.action.boost == best->action.boost &&
                 (s.dealt > best->dealt || (s.dealt == best->dealt && attackCost(s) < attackCost(*best))))) {
                best = &s;
            }
        }
        return best;
    }

    // 打已知破绽：架势剩得最少的先，其次最凶的；同一个目标里平砍先于法术、法术先于毒药。
    // target >= 0 时只在它身上挑；maxBoost 时劲蓄满（首领蓄势时抢着削），否则只蓄 idleBoost。
    [[nodiscard]] const Scored* bestChip(const BattleState& b, const std::vector<Scored>& options, int target,
                                         bool maxBoost) const {
        const Scored* best = nullptr;
        for (const Scored& s : options) {
            const Unit& t = unitAt(b, s.action.targetIndex);
            if (!s.knownWeak || t.broken() || (target >= 0 && s.action.targetIndex != target)) continue;
            if (s.action.boost != 0) continue;   // 先挑一手，劲另定
            if (best == nullptr) {
                best = &s;
                continue;
            }
            const Unit& bt = unitAt(b, best->action.targetIndex);
            if (t.toughness != bt.toughness) {
                if (t.toughness < bt.toughness) best = &s;
                continue;
            }
            const int th = threat(b, s.action.targetIndex);
            const int bth = threat(b, best->action.targetIndex);
            if (th != bth) {
                if (th > bth) best = &s;
                continue;
            }
            if (attackCost(s) < attackCost(*best) ||
                (attackCost(s) == attackCost(*best) && s.dealt > best->dealt)) {
                best = &s;
            }
        }
        if (best == nullptr) return nullptr;
        if (maxBoost) {
            const Action up = topUp(b, options, *best);
            return sameWithBoost(options, up, up.boost);
        }
        const Scored* boosted = sameWithBoost(options, best->action, idleBoost(b, best->action));
        return boosted != nullptr ? boosted : best;
    }

    // 探破绽：凡人先试兵刃、再试法术；修仙界的东西（境界在凡人之上）先试法术——对着地窖里
    // 那具尸傀先抡两下剑，是这只手唯一会犯的蠢。毒药一律最后。同一档里挑最凶的敌人。
    [[nodiscard]] static int probeCost(const BattleState& b, const Scored& s) {
        const bool mortal = unitAt(b, s.action.targetIndex).realm == rules::Realm::Mortal;
        if (s.action.kind == ActionKind::Attack) return mortal ? 0 : 1;
        if (s.action.kind == ActionKind::Cast) return mortal ? 1 : 0;
        return 2;
    }

    [[nodiscard]] const Scored* bestProbe(const BattleState& b, const std::vector<Scored>& options) const {
        const Scored* best = nullptr;
        for (const Scored& s : options) {
            if (!s.probes || s.action.boost != 0) continue;
            if (s.action.kind == ActionKind::Item && !s.poisons) continue;   // 已经中着毒，撒了也白撒
            if (best == nullptr || probeCost(b, s) < probeCost(b, *best) ||
                (probeCost(b, s) == probeCost(b, *best) &&
                 threat(b, s.action.targetIndex) > threat(b, best->action.targetIndex))) {
                best = &s;
            }
        }
        if (best == nullptr) return nullptr;
        const Scored* boosted = sameWithBoost(options, best->action, idleBoost(b, best->action));
        return boosted != nullptr ? boosted : best;
    }

    // 实打最多的一手（同分平砍先，再挑最凶的敌人）。options 非空时必有一条（毒药也算一条，
    // 它实打为 0，只在别的都打不动时才轮得到）。
    [[nodiscard]] const Scored* bestDamage(const BattleState& b, const std::vector<Scored>& options) const {
        const Scored* best = nullptr;
        for (const Scored& s : options) {
            if (s.action.boost != 0) continue;
            if (best == nullptr || s.dealt > best->dealt ||
                (s.dealt == best->dealt && attackCost(s) < attackCost(*best)) ||
                (s.dealt == best->dealt && attackCost(s) == attackCost(*best) &&
                 threat(b, s.action.targetIndex) > threat(b, best->action.targetIndex))) {
                best = &s;
            }
        }
        if (best == nullptr) return &options.front();
        const Scored* boosted = sameWithBoost(options, best->action, idleBoost(b, best->action));
        return boosted != nullptr ? boosted : best;
    }

    // 按 policy.pills 的先后找一样有、点得动的药，喂给 target。
    bool feed(game::BattleScene& scene, int actor, int target) {
        for (const std::string& id : policy_.pills) {
            if (app_->state().itemCount(id) <= 0 || !scene.battle().hasItem(id)) continue;
            Action eat = make(ActionKind::Item, actor, target);
            eat.magicId = id;
            if (!legal(scene.battle(), eat)) continue;
            if (issue(scene, eat)) return true;
        }
        return false;
    }

    bool chooseWeapon(game::BattleScene& scene, const Action& a) {
        if (scene.menuMode() != game::BattleMenuMode::Weapon) return true;   // 只有一样兵刃
        std::vector<int> categories;
        static_cast<void>(game::BattleScene::buildWeaponItems(scene.battle(), a.actorIndex, categories));
        const int wanted = scene.battle().attackCategory(a);
        const auto found = std::find(categories.begin(), categories.end(), wanted);
        return found != categories.end() &&
               scene.menuChoose(*app_, static_cast<int>(found - categories.begin()));
    }

    // 法术那一级（cast）或物品那一级：按 id 找行，行序与菜单同源。
    bool chooseListed(game::BattleScene& scene, const Action& a, bool cast) {
        std::vector<std::string> ids;
        if (cast) {
            static_cast<void>(game::BattleScene::buildMagicItems(app_->data(), app_->state(), scene.battle(),
                                                                 a.actorIndex, ids));
        } else {
            static_cast<void>(game::BattleScene::buildItemItems(app_->data(), app_->state(), scene.battle(),
                                                                a.actorIndex, ids));
        }
        const auto found = std::find(ids.begin(), ids.end(), a.magicId);
        return found != ids.end() && scene.menuChoose(*app_, static_cast<int>(found - ids.begin()));
    }

    bool chooseTarget(game::BattleScene& scene, const Action& a) {
        if (scene.menuMode() != game::BattleMenuMode::Target) return false;
        std::vector<int> indices;
        static_cast<void>(game::BattleScene::buildTargetItems(app_->state(), scene.battle(), a, indices));
        const auto found = std::find(indices.begin(), indices.end(), a.targetIndex);
        return found != indices.end() && scene.menuChoose(*app_, static_cast<int>(found - indices.begin()));
    }

    game::Application* app_;
    HandPolicy policy_;
    bool fleeAtOnce_ = false;
};

}  // namespace fanren::test
