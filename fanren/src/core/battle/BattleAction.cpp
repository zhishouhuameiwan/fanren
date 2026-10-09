// 动作合法性判定与结算。
//
// 本文件的头号约束：isLegal 与 apply 走同一条 checkLegal 通路。旧原型把校验散在
// 各个 doXxx 里，施法先扣 mp 再查条件、逃跑失败照样置 Acted，玩家点一次非法动作
// 就白丢一个回合。这里 apply 先整体判定，判定不过一个字节都不写。
#include "core/battle/Battle.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <string>

#include "core/battle/Damage.h"

namespace fanren::core::battle {
namespace {

// 「他不在场上」时那半句话。倒下 / 脱身写「已经倒下」，还没进场写「尚未入场」：
// 一个是打完了，一个是还没开始打，玩家该做的事完全不同。
[[nodiscard]] const char* absenceReason(const Unit& unit) {
    return unit.onField ? "已经倒下" : "尚未入场";
}

// 判定失败的统一出口：写明原因（UTF-8，可直接显示给玩家）并返回 false。
bool deny(std::string* why, std::string text) {
    if (why != nullptr) *why = std::move(text);
    return false;
}

// 一个单位的第一样兵刃（位序最前的那一位）；一样都没有时为 0。
[[nodiscard]] int primaryWeapon(int weapons) {
    const std::vector<int> bits = categoryBits(weapons);
    return bits.empty() ? kCategoryNone : bits.front();
}

// 施法与道具施法那一手的说法开头（契约 docs/interfaces-p3-ch07.md 2.3）：施法是「韩立 施展【天眼术】」，
// 道具是「韩立 使用 天雷子——」。后者以破折号收尾，接下去的半句不再加「，」。
[[nodiscard]] std::string castOpener(const Unit& caster, const Magic& magic, const Item* viaItem) {
    return viaItem != nullptr ? caster.name + " 使用 " + viaItem->name + "——"
                              : caster.name + " 施展【" + magic.name + "】";
}

}  // namespace

bool BattleState::isLegal(const Action& action) const { return checkLegal(action, nullptr); }

bool BattleState::isLegal(const Action& action, std::string* why) const {
    // 刻意只是 checkLegal 的转发：菜单画的禁用理由与 apply 回绝时给的那句话，
    // 必须字字相同。
    return checkLegal(action, why);
}

bool BattleState::checkLegal(const Action& action, std::string* why) const {
    if (phase_ != BattlePhase::Ongoing) return deny(why, "战斗已经结束");
    if (!validIndex(action.actorIndex)) return deny(why, "行动者不存在");

    const Unit& actor = unitRef(action.actorIndex);
    if (!actor.alive()) return deny(why, actor.name + " " + absenceReason(actor));
    if (action.actorIndex != currentActor()) return deny(why, "还没轮到 " + actor.name + " 行动");
    // 按「位」判，不按人判：首领一回合两次行动就占两位，第二位上它照样出得了手。
    if (slotUsed_) return deny(why, actor.name + " 本回合已经行动过");

    bool ok = false;
    switch (action.kind) {
        case ActionKind::Attack: ok = checkAttack(action, actor, why); break;
        case ActionKind::Cast:   ok = checkCast(action, actor, why); break;
        case ActionKind::Item:   ok = checkItem(action, actor, why); break;
        case ActionKind::Defend: ok = true; break;
        case ActionKind::Escape: ok = checkEscape(actor, why); break;
        case ActionKind::Charge: ok = checkCharge(actor, why); break;
    }
    if (!ok) return false;
    // 蓄劲是这一手的修饰，排在「这一手本身行不行」之后：先说东西不对路，再说劲不够。
    return checkBoost(action, actor, why);
}

bool BattleState::checkBoost(const Action& a, const Unit& u, std::string* why) const {
    if (a.boost == 0) return true;
    // 看破（天眼术）与削架势不吃蓄劲：蓄了也只当没蓄——不拦、不扣（applyReveal / applyStagger
    // 不花劲），与界面上「蓄了劲再防御」一样按 0 算（契约 docs/interfaces-p3-ch06.md 1.3 / 1.6、
    // docs/interfaces-p3-ch07.md 2.3）。道具施法不在此列：物品本来就蓄不了劲，下面照旧回绝。
    if (a.kind == ActionKind::Cast) {
        const Magic* magic = findMagic(a.magicId);
        if (magic != nullptr && magic->effect != MagicEffect::None) return true;
    }
    if (a.boost < 0) return deny(why, "蓄劲不能是负数");
    if (a.kind != ActionKind::Attack && a.kind != ActionKind::Cast) {
        return deny(why, "只有攻击与法术蓄得了劲");
    }
    if (a.boost > kBoostMax) {
        return deny(why, "一次最多蓄 " + std::to_string(kBoostMax) + " 点劲");
    }
    if (a.boost > u.bp) {
        return deny(why, u.name + " 的劲不够：要蓄 " + std::to_string(a.boost) + " 点，只有 " +
                             std::to_string(u.bp) + " 点");
    }
    return true;
}

bool BattleState::checkEnemyTarget(int targetIndex, const Unit& actor, std::string* why) const {
    if (!validIndex(targetIndex)) return deny(why, "目标不存在");
    const Unit& target = unitRef(targetIndex);
    if (!target.alive()) return deny(why, target.name + " " + absenceReason(target));
    if (target.ally == actor.ally) return deny(why, "不能以同伴为敌");
    return true;
}

bool BattleState::checkAttack(const Action& a, const Unit& u, std::string* why) const {
    if (!checkEnemyTarget(a.targetIndex, u, why)) return false;
    // 点名了兵刃就得真有：一个类别位，而且是这个人手上有的那几样之一。
    if (a.category != 0 && !devour_) {
        if (!isSingleCategory(a.category) || (a.category & u.weapons) == 0) {
            const std::string name = categoryName(a.category);
            return deny(why, u.name + " 手上没有" + (name.empty() ? std::string("这样兵刃")
                                                               : "「" + name + "」这样兵刃"));
        }
    }
    return true;
}

bool BattleState::checkCast(const Action& a, const Unit& u, std::string* why) const {
    const Magic* magic = findMagic(a.magicId);
    if (magic == nullptr) return deny(why, "没有这门法术：" + a.magicId);
    // magics 非空时一律按已习得列表卡；留空时看 magicsExhaustive：置位的是
    // 「一门都不会」（game 层建的场都是这一档），缺省的才退回「战场登记的法术都会」。
    if (u.magicsExhaustive || !u.magics.empty()) {
        if (std::find(u.magics.begin(), u.magics.end(), a.magicId) == u.magics.end()) {
            return deny(why, u.name + " 未习得【" + magic->name + "】");
        }
    }
    if (rules::toValue(u.realm) < rules::toValue(magic->needRealm)) {
        return deny(why, "境界不足，施展不了【" + magic->name + "】");
    }
    // 不伤人的法术（御风决、护身罡）挡在这里。从前挡它的是「castRange 0 够不着任何人」，
    // 菜单上那句「超出施法距离」对一门辅助法术是误导；格子没了，理由就得说实话。
    // 放行的判据是 castableMagic：带看破效果的天眼术不伤人，却施展得了（契约 interfaces-p3-ch06 1.3）。
    if (!castableMagic(*magic)) return deny(why, "【" + magic->name + "】不是伤人的法术");
    if (u.mp < magic->needMp) return deny(why, "法力不足，施展不了【" + magic->name + "】");
    return checkMagicTarget(a, *magic, u, why);
}

bool BattleState::checkMagicTarget(const Action& a, const Magic& magic, const Unit& u,
                                   std::string* why) const {
    if (magic.target == MagicTarget::All &&
        (magic.effect != MagicEffect::None || !offensiveMagic(magic))) {
        return deny(why, "全体只许用于伤人的法术");
    }
    // 看破照的是整个场子，不挑目标：targetIndex 不看。
    if (magic.effect == MagicEffect::Reveal) return true;
    if (!checkEnemyTarget(a.targetIndex, u, why)) return false;
    if (magic.effect == MagicEffect::Stagger) {
        // 没有架势可削、已经破势：拦在前头并说清楚，法力与物品都不扣——与「解毒药用在
        // 没中毒的人身上」同一个口径（契约 docs/interfaces-p3-ch07.md 2.3）。
        const Unit& target = unitRef(a.targetIndex);
        if (target.maxToughness <= 0) return deny(why, target.name + " 没有架势可削");
        if (target.broken()) return deny(why, target.name + " 已经破势");
    }
    return true;
}

bool BattleState::checkItem(const Action& a, const Unit& u, std::string* why) const {
    // 物品 id 借用 magicId 字段传递：契约的 Action 没有 itemId。
    const Item* item = findItem(a.magicId);
    if (item == nullptr) return deny(why, "没有这件物品：" + a.magicId);
    // 带 castMagic 的符箓：用它 = 以那门法术施展一次，目标照那门法术挑（契约
    // docs/interfaces-p3-ch07.md 2.3）；下面「丹药只能给自己人用」那一条管不到它。
    // 不查法力、不查习得：那是施法的门槛，符箓的力气在纸上。
    if (!item->castMagic.empty()) {
        const Magic* magic = findMagic(item->castMagic);
        if (magic == nullptr) return deny(why, "没有这门法术：" + item->castMagic);
        return checkMagicTarget(a, *magic, u, why);
    }
    if (!validIndex(a.targetIndex)) return deny(why, "目标不存在");
    const Unit& target = unitRef(a.targetIndex);
    if (!target.alive()) return deny(why, target.name + " " + absenceReason(target));
    // 毒药是往对手身上使的，丹药是给自己人用的，按数据分流而不是按物品 id 写死。
    if (item->poison > 0) {
        if (target.ally == u.ally) return deny(why, "毒药不能用在自己人身上");
    } else if (target.ally != u.ally) {
        return deny(why, "丹药只能给自己人用");
    }
    // 解毒药用在没中毒的人身上：拦下来，并说清楚为什么（第 3 章校对 MEDIUM-1）。
    // 判据只收「纯解毒药」：回血药满血时能不能吃是另一件事，不在这一条里。
    const bool pureAntidote = item->curesPoison && item->restoreHp <= 0 && item->restoreMp <= 0;
    if (pureAntidote && target.poison <= 0) {
        return deny(why, target.name + " 没有中毒，" + item->name + " 用不上");
    }
    // 使用距离随格子一起删掉（技术债 G-7 那一条的前提——隔着整张战场扔毒药——
    // 在横版里不存在了：两边本来就面对面站着）。
    return true;
}

bool BattleState::checkEscape(const Unit& u, std::string* why) const {
    if (!canEscape_) return deny(why, "此战无法回避");
    // Escape 这个**动作**永远只属于我方，语义是「我方脱离战斗」（phase 落在 Escaped）。
    // 敌人脱身走的是另一条路：吞噬模式下体积被咬掉三分之一自动遁走，落在 EnemyFled。
    if (!u.ally) return deny(why, "敌人不会脱离战斗");
    return true;
}

bool BattleState::checkCharge(const Unit& u, std::string* why) const {
    if (u.chargeEvery <= 0) return deny(why, u.name + " 不会蓄势");
    if (u.charging) return deny(why, u.name + " 已经在蓄势");
    // 节奏是规则，不只是 AI 的打法（docs/octopath-battle.md 2.6）：它自己第 every、2×every……
    // 个出手回合的**第一手**才宣告。这一回合已经出过手（多次行动的后几位），或者还没到那个
    // 回合，都不行——写在这里，AI 与任何别的驱动方问的就是同一条。
    if (u.acted || (u.roundsActed + 1) % u.chargeEvery != 0) {
        return deny(why, u.name + " 还没到蓄势的时候");
    }
    return true;
}

// ---------------------------------------------------------------------------
// 结算
// ---------------------------------------------------------------------------

core::Result<std::string> BattleState::apply(const Action& action) {
    std::string why;
    if (!checkLegal(action, &why)) {
        // 非法动作不改变任何状态、不消耗任何资源：这里必须是纯粹的早退。
        return core::Result<std::string>::failure(why);
    }

    lastActionBegin_ = events_.size();
    Unit& actor = unitRef(action.actorIndex);
    // 蓄势的节奏按「出过手的回合」数：这一回合的第一手才记一次。
    if (!actor.acted) ++actor.roundsActed;

    std::string line;
    switch (action.kind) {
        case ActionKind::Attack: line = applyAttack(action); break;
        case ActionKind::Cast:   line = applyCast(action); break;
        case ActionKind::Item:   line = applyItem(action); break;
        case ActionKind::Defend: line = applyDefend(action); break;
        case ActionKind::Escape: line = applyEscape(action); break;
        case ActionKind::Charge: line = applyCharge(action); break;
    }

    unitRef(action.actorIndex).acted = true;
    slotUsed_ = true;
    log_.push_back(line);
    refreshPhase();
    lastActionEnd_ = events_.size();
    return core::Result<std::string>::success(line);
}

int BattleState::attackCategory(const Action& action) const {
    if (devour_ || !validIndex(action.actorIndex)) return kCategoryNone;
    if (action.category != 0) return action.category;
    return primaryWeapon(unitRef(action.actorIndex).weapons);
}

int BattleState::hitCategories(const Action& action) const {
    if (devour_) return kCategoryNone;
    switch (action.kind) {
        case ActionKind::Attack:
            return attackCategory(action);
        case ActionKind::Cast: {
            const Magic* magic = findMagic(action.magicId);
            return magic == nullptr ? kCategoryNone : magicCategories(*magic);
        }
        case ActionKind::Item: {
            // 带 castMagic 的符箓打的是那门法术的类别（契约 docs/interfaces-p3-ch07.md 2.3）；
            // 毒药撒过去算「毒」类一击；丹药、解毒药不是打人的。
            if (const Magic* magic = castMagicOf(action)) return magicCategories(*magic);
            const Item* item = findItem(action.magicId);
            return item != nullptr && item->poison > 0 ? kCategoryPoison : kCategoryNone;
        }
        case ActionKind::Defend:
        case ActionKind::Escape:
        case ActionKind::Charge:
            return kCategoryNone;
    }
    return kCategoryNone;
}

const Magic* BattleState::castMagicOf(const Action& action) const {
    if (action.kind != ActionKind::Item) return nullptr;
    const Item* item = findItem(action.magicId);
    return item == nullptr || item->castMagic.empty() ? nullptr : findMagic(item->castMagic);
}

int BattleState::hitCount(const Action& action) const {
    const int boost = std::max(0, action.boost);
    switch (action.kind) {
        case ActionKind::Attack:
            return 1 + boost;
        case ActionKind::Cast: {
            const Magic* magic = findMagic(action.magicId);
            return magic != nullptr && magic->boost == MagicBoost::Hits ? 1 + boost : 1;
        }
        // 道具施法恒一击：连发型的法术装进符里也只打一发——不吃劲（契约 2.3）。
        case ActionKind::Item:
        case ActionKind::Defend:
        case ActionKind::Escape:
        case ActionKind::Charge:
            return 1;
    }
    return 1;
}

int BattleState::finalDamage(int baseDamage, const Unit& attacker, const Unit& target) const {
    // 基数是 Damage.cpp 的公式（平砍 攻×2−防、法术 威力×2−防×0.5、境界压制、五行相克、
    // 下限 1）原样算出来的；这里只在外面乘三样：破势 ×2、防御减半、重招的倍数。
    // 沿用公式是有意的：前四章设计文档里所有「几下打死一个」的推算仍然成立。
    double damage = baseDamage;
    if (target.broken()) damage = damage * kBrokenDamagePercent / 100.0;
    if (target.guarding) damage = damage * kGuardDamagePercent / 100.0;
    if (attacker.charging) damage *= attacker.chargeMult;
    const auto rounded = static_cast<int>(std::lround(damage));
    return rounded < 1 ? 1 : rounded;
}

int BattleState::estimateHitDamage(const Action& action, int targetIndex) const {
    if (!validIndex(action.actorIndex) || !validIndex(targetIndex)) return 0;
    const Unit& attacker = unitRef(action.actorIndex);
    const Unit& target = unitRef(targetIndex);
    switch (action.kind) {
        case ActionKind::Attack: {
            const int base = devour_ ? devourBite(attacker.maxHp)
                                     : physicalDamage(attacker.attack, target.defence, attacker.realm,
                                                      target.realm, attacker.element, target.element);
            return devour_ ? base : finalDamage(base, attacker, target);
        }
        case ActionKind::Cast: {
            const Magic* magic = findMagic(action.magicId);
            if (magic == nullptr) return 0;
            const int power = magic->boost == MagicBoost::Power
                                  ? magic->power * (1 + std::max(0, action.boost))
                                  : magic->power;
            const int base = magicDamage(power, target.defence, attacker.realm, target.realm,
                                         magic->element, target.element);
            return finalDamage(base, attacker, target);
        }
        case ActionKind::Item: {
            // 道具施法按那门法术、劲 0 估（契约 docs/interfaces-p3-ch07.md 2.3）；别的物品不是打人的。
            const Magic* magic = castMagicOf(action);
            if (magic == nullptr) return 0;
            Action asCast = action;
            asCast.kind = ActionKind::Cast;
            asCast.magicId = magic->id;
            asCast.boost = 0;
            return estimateHitDamage(asCast, targetIndex);
        }
        case ActionKind::Defend:
        case ActionKind::Escape:
        case ActionKind::Charge:
            return 0;
    }
    return 0;
}

void BattleState::spendBoost(int actorIndex, int boost) {
    if (boost <= 0) return;
    Unit& u = unitRef(actorIndex);
    u.bp -= boost;
    u.boosted = true;   // 下一回合开始不加劲
    emit(BattleEvent{BattleEventKind::BoostSpent, actorIndex, -1, boost});
}

std::vector<int> BattleState::strikeTargets(int actorIndex, int targetIndex, bool allTargets) const {
    const Unit& actor = unitRef(actorIndex);
    if (!allTargets && (!actor.charging || !actor.chargeAll)) return {targetIndex};
    // Charged and ordinary all-target attacks share the same live-opponent filter.
    std::vector<int> all;
    for (std::size_t i = 0; i < units_.size(); ++i) {
        if (units_[i].alive() && units_[i].ally != actor.ally) all.push_back(static_cast<int>(i));
    }
    return all;
}

std::string BattleState::applyAttack(const Action& a) {
    Unit& attacker = unitRef(a.actorIndex);
    spendBoost(a.actorIndex, a.boost);
    const int category = attackCategory(a);
    const int hits = hitCount(a);
    const bool heavy = attacker.charging;

    // 识海里没有兵器也没有护甲：咬下多少只看谁大。
    std::string head;
    if (heavy) {
        head = attacker.name + " 使出重招，" +
               (attacker.chargeAll ? std::string("横扫全场") : "扑向 " + unitRef(a.targetIndex).name);
    } else {
        head = attacker.name + (devour_ ? " 一口咬向 " : " 攻击 ") + unitRef(a.targetIndex).name;
    }
    // 我方报出所用的兵刃（玩家挑的就是它）；敌方的普攻不是破绽的来源，不必报。
    if (category != kCategoryNone && (hits > 1 || attacker.ally)) {
        head += "（" + std::string(categoryName(category));
        if (hits > 1) head += " ×" + std::to_string(hits);
        head += "）";
    }
    BattleEvent act{BattleEventKind::Act, a.actorIndex, a.targetIndex,
                    static_cast<int>(ActionKind::Attack)};
    act.category = category;
    act.hits = hits;
    act.text = head;
    emit(std::move(act));

    std::string text = head + "，";
    bool first = true;
    for (const int target : strikeTargets(a.actorIndex, a.targetIndex)) {
        for (int hit = 1; hit <= hits; ++hit) {
            if (!unitRef(target).alive()) break;
            const Unit& t = unitRef(target);
            const int base = devour_ ? devourBite(attacker.maxHp)
                                     : physicalDamage(attacker.attack, t.defence, attacker.realm, t.realm,
                                                      attacker.element, t.element);
            if (!first) text += "；";
            first = false;
            if (heavy && attacker.chargeAll) text += "波及 " + t.name + "，";
            text += strike(a.actorIndex, target, base, category, hit, hits);
        }
    }
    // 重招出过了。放在打完之后清：打的时候 finalDamage 要读它。
    attacker.charging = false;
    return text;
}

std::string BattleState::applyCast(const Action& a) {
    const Magic magic = *findMagic(a.magicId);   // 合法性已确认；取拷贝，免得与成员别名
    return resolveMagic(a, magic, nullptr);
}

std::string BattleState::resolveMagic(const Action& a, const Magic& magic, const Item* viaItem) {
    if (magic.effect == MagicEffect::Reveal) return applyReveal(a, magic, viaItem);
    if (magic.effect == MagicEffect::Stagger) return applyStagger(a, magic, viaItem);
    Unit& caster = unitRef(a.actorIndex);
    // 道具施法与施法只差三处（契约 docs/interfaces-p3-ch07.md 2.3）：① 不扣法力；② 不吃蓄劲
    //（checkBoost 回绝带劲的物品，所以 a.boost 恒 0、hitCount 恒 1）；③ Act 那一行记 Item、
    // 说法是「X 使用 Y——」。伤害公式、五行、境界压制、破绽、削架势、破势、打断蓄势一概照旧。
    if (viaItem == nullptr) {
        caster.mp -= magic.needMp;
        spendBoost(a.actorIndex, a.boost);
    }
    const int category = magicCategories(magic);
    const int hits = hitCount(a);
    // 威力型蓄劲：威力 ×(1+N) 喂进同一条公式，仍是一击。
    const int power = magic.boost == MagicBoost::Power ? magic.power * (1 + std::max(0, a.boost))
                                                       : magic.power;
    const bool heavy = caster.charging;
    const bool all = magic.target == MagicTarget::All || (heavy && caster.chargeAll);

    std::string head = viaItem != nullptr
                           ? castOpener(caster, magic, viaItem)
                           : caster.name + (heavy ? " 使出重招，施展【" : " 施展【") + magic.name + "】";
    if (hits > 1) head += "（连发 " + std::to_string(hits) + " 发）";
    if (magic.boost == MagicBoost::Power && a.boost > 0) {
        head += "（蓄劲 " + std::to_string(a.boost) + " 点，威力 ×" + std::to_string(1 + a.boost) + "）";
    }
    head += all ? std::string("横扫全场") : "击中 " + unitRef(a.targetIndex).name;
    BattleEvent act{BattleEventKind::Act, a.actorIndex, a.targetIndex,
                    static_cast<int>(viaItem != nullptr ? ActionKind::Item : ActionKind::Cast)};
    act.category = category;
    act.hits = hits;
    act.text = head;
    emit(std::move(act));

    std::string text = head + "，";
    bool first = true;
    for (const int target : strikeTargets(a.actorIndex, a.targetIndex, magic.target == MagicTarget::All)) {
        for (int hit = 1; hit <= hits; ++hit) {
            if (!unitRef(target).alive()) break;
            const Unit& t = unitRef(target);
            const int base = magicDamage(power, t.defence, caster.realm, t.realm, magic.element, t.element);
            if (!first) text += "；";
            first = false;
            if (all) text += "波及 " + t.name + "，";
            text += strike(a.actorIndex, target, base, category, hit, hits);
        }
        // 法术是毒的第二条来源（契约第 2.3 节）。下毒在伤害之后：目标已经倒下时
        // 再挂一层毒只会在日志上留下一句看不懂的话。一发法术只下一次毒，不按连发数叠。
        if (unitRef(target).alive()) {
            const std::string poisoned = applyPoison(target, magic.poison, magic.poisonPower);
            if (!poisoned.empty()) text += "，" + poisoned;
        }
    }
    caster.charging = false;
    return text;
}

std::string BattleState::applyReveal(const Action& a, const Magic& magic, const Item* viaItem) {
    // 契约 docs/interfaces-p3-ch06.md 1.3：扣法力；本场每一个站着的敌人破绽全部揭开——与「打中破绽
    // 揭开」是同一份 Unit::revealed，同 id 的一起揭开，收场时照旧并进 GameState::knownWeaknesses
    //（BattleScene::finish）。不是一击：不判破绽、不削架势、不打断蓄势、没有伤害数字；劲不扣。
    // 道具施展的不扣法力（契约 docs/interfaces-p3-ch07.md 2.3）。
    Unit& caster = unitRef(a.actorIndex);
    if (viaItem == nullptr) caster.mp -= magic.needMp;
    const bool casterAlly = caster.ally;
    const std::string head = castOpener(caster, magic, viaItem);
    const std::string lead = viaItem != nullptr ? "" : "，";
    BattleEvent act{BattleEventKind::Act, a.actorIndex, -1,
                    static_cast<int>(viaItem != nullptr ? ActionKind::Item : ActionKind::Cast)};
    act.hits = 1;
    act.text = head;
    emit(std::move(act));

    std::string text = head;
    bool any = false;
    for (std::size_t i = 0; i < units_.size(); ++i) {
        const Unit& target = units_[i];
        if (!target.alive() || target.ally == casterAlly) continue;
        const int fresh = target.weaknesses & ~target.revealed;
        for (Unit& same : units_) {
            if (same.id == target.id && same.ally == target.ally) same.revealed |= same.weaknesses;
        }
        BattleEvent reveal{BattleEventKind::Reveal, a.actorIndex, static_cast<int>(i), target.revealed};
        reveal.revealed = fresh;
        emit(std::move(reveal));
        if (target.weaknesses == 0) continue;
        text += (any ? std::string("；") : lead) + target.name + " 的破绽「" + categoryNames(target.weaknesses) +
                "」尽收眼底";
        any = true;
    }
    if (!any) text += lead + "场上没有看得出的破绽";
    return text;
}

std::string BattleState::applyStagger(const Action& a, const Magic& magic, const Item* viaItem) {
    // 契约 docs/interfaces-p3-ch07.md 2.3：扣法力（道具不扣）；目标架势减 stagger（下限 0），削到 0 即破势——
    // 走「打中破绽削到 0」那一条（breakUnit：同样的破势回合数、同样打断蓄势，emitBreak：同样的 Break 事件）。
    // 不是一击：不判破绽、不揭开、不伤人、没有伤害数字；劲不扣（蓄了也只当没蓄）。
    // 没有架势、已经破势的目标到不了这里（checkMagicTarget 拦在前头）。
    Unit& caster = unitRef(a.actorIndex);
    if (viaItem == nullptr) caster.mp -= magic.needMp;
    Unit& target = unitRef(a.targetIndex);
    const std::string head = castOpener(caster, magic, viaItem);
    BattleEvent act{BattleEventKind::Act, a.actorIndex, a.targetIndex,
                    static_cast<int>(viaItem != nullptr ? ActionKind::Item : ActionKind::Cast)};
    act.category = magicCategories(magic);   // 只给画面挑施法光的颜色；这一手不判破绽
    act.hits = 1;
    act.text = head;
    emit(std::move(act));

    const int before = target.toughness;
    target.toughness = std::max(0, target.toughness - magic.stagger);
    const int cut = before - target.toughness;
    BattleEvent stagger{BattleEventKind::Stagger, a.actorIndex, a.targetIndex, cut};
    stagger.hp = target.hp;
    stagger.toughness = target.toughness;
    emit(std::move(stagger));

    std::string text = head + (viaItem != nullptr ? "" : "，") + target.name + " 的架势被削去 " +
                       std::to_string(cut) + " 点";
    if (target.toughness == 0) {
        StrikeOutcome out;
        breakUnit(a.targetIndex, text, out);
        emitBreak(a.actorIndex, a.targetIndex, out);
    }
    return text;
}

std::string BattleState::strike(int attackerIndex, int targetIndex, int baseDamage, int category,
                                int hit, int hits) {
    // 伤害先按**这一击出手那一刻**的状态落下去，再判破绽：把它打到破势的这一击本身
    // 不吃 ×2，同一手里之后的几击才吃——八方旅人也是这么算的，而且只有这样，
    // 「蓄满劲先破势」与「破势之后再蓄满劲」才是两种不同的打法。
    const Unit& attacker = unitRef(attackerIndex);
    StrikeOutcome out;
    out.damage = devour_ ? baseDamage : finalDamage(baseDamage, attacker, unitRef(targetIndex));
    const int killableBy = unitRef(targetIndex).killableBy;
    const bool lethal = killableBy == 0 || (category & killableBy) != 0;
    std::string text = inflict(targetIndex, out.damage, attackerIndex, out, lethal);
    text += resolveWeakness(targetIndex, category, out);
    emitStrike(attackerIndex, targetIndex, category, hit, hits, out);
    return text;
}

std::string BattleState::probe(int attackerIndex, int targetIndex, int category) {
    StrikeOutcome out;
    std::string text = resolveWeakness(targetIndex, category, out);
    emitStrike(attackerIndex, targetIndex, category, 1, 1, out);
    return text;
}

void BattleState::emitStrike(int attackerIndex, int targetIndex, int category, int hit, int hits,
                             const StrikeOutcome& out) {
    const Unit& target = unitRef(targetIndex);
    BattleEvent event{BattleEventKind::Hit, attackerIndex, targetIndex, out.damage};
    event.category = category;
    event.weakness = out.weakness;
    event.revealed = out.revealed;
    event.hit = hit;
    event.hits = hits;
    event.hp = target.hp;
    event.toughness = target.toughness;
    emit(std::move(event));
    emitBreak(attackerIndex, targetIndex, out);
    if (out.fled) emit(BattleEvent{BattleEventKind::Fled, attackerIndex, targetIndex});
    if (out.fell) emit(BattleEvent{BattleEventKind::Fall, attackerIndex, targetIndex});
}

void BattleState::emitBreak(int attackerIndex, int targetIndex, const StrikeOutcome& out) {
    const Unit& target = unitRef(targetIndex);
    if (out.broke) {
        BattleEvent broke{BattleEventKind::Break, attackerIndex, targetIndex,
                          target.breakRecoverRound - round_};
        broke.hp = target.hp;
        broke.toughness = target.toughness;
        emit(std::move(broke));
    }
    if (out.interrupted) emit(BattleEvent{BattleEventKind::ChargeInterrupt, attackerIndex, targetIndex});
}

std::string BattleState::resolveWeakness(int targetIndex, int category, StrikeOutcome& out) {
    Unit& target = unitRef(targetIndex);
    if (devour_ || category == kCategoryNone) return {};

    // 打中过就算试过：一样兵刃砍过了没揭开什么，我方 AI 就不再拿它去试这一种。
    // 同 id 的单位一起记——知道的是「恶狼怕什么」，不是「这一条恶狼怕什么」，
    // 存档里按 role_id 记的也是这个（GameState::knownWeaknesses）。
    // 只在挨打的这一边里传：码头打手敌友两用（夺帮那一夜），我方那一位没有破绽，
    // 打在他身上什么也试不出来，不能顺着 id 把「拳试过了」记到对面那几个头上。
    const int hitWeak = category & target.weaknesses;
    out.revealed = hitWeak & ~target.revealed;
    for (Unit& same : units_) {
        if (same.id != target.id || same.ally != target.ally) continue;
        same.tested |= category;
        same.revealed |= hitWeak & same.weaknesses;
    }
    if (hitWeak == 0 || target.maxToughness <= 0) return {};

    out.weakness = true;
    std::string text = "，破绽「" + categoryNames(hitWeak) + "」";
    // 已经破势的不再削：架势就是 0，再削也还是 0。这一击已经把它打倒的也不必削。
    if (target.broken() || !target.alive()) return text;
    target.toughness = std::max(0, target.toughness - 1);
    if (target.toughness == 0) breakUnit(targetIndex, text, out);
    return text;
}

void BattleState::breakUnit(int targetIndex, std::string& text, StrikeOutcome& out) {
    Unit& target = unitRef(targetIndex);
    target.toughness = 0;
    target.breakRecoverRound = round_ + kBreakRecoverAfterRounds;
    out.broke = true;
    text += "，" + target.name + " 破势！";
    // 蓄势中的首领被打到破势：重招作废——八方旅人最有名的那一口气。
    if (target.charging) {
        target.charging = false;
        out.interrupted = true;
        text += "蓄势被打断";
    }
}

std::string BattleState::inflict(int targetIndex, int damage, int attackerIndex, StrikeOutcome& out,
                                 bool lethal) {
    Unit& target = unitRef(targetIndex);
    // 要不了他的命的那一类：伤害照算照报，只是压不到 1 以下（Unit::killableBy）。
    const int floorHp = lethal ? 0 : std::min(target.hp, 1);
    const int dealt = std::min(damage, target.hp - floorHp);   // 真正咬进血肉的那一部分
    target.hp = std::max(floorHp, target.hp - damage);
    std::string text = "造成 " + std::to_string(damage) + " 点伤害";
    if (!lethal && target.hp == floorHp && dealt < damage) text += "，却伤不到" + target.name + "的要害";
    if (devour_ && attackerIndex >= 0) {
        // 吞噬按**实际扣掉的气血**算，不按名义伤害。
        text += devourTransfer(attackerIndex, targetIndex, dealt, out);
    }
    if (target.hp <= 0) {
        text += "，" + target.name + " 倒地不起";
        out.fell = true;
    }
    return text;
}

std::string BattleState::devourTransfer(int attackerIndex, int targetIndex, int hpDamage,
                                        StrikeOutcome& out) {
    if (hpDamage <= 0) return {};
    Unit& attacker = unitRef(attackerIndex);
    Unit& target = unitRef(targetIndex);

    // 咬下的那块从守方身上掉下来：守方 maxHp 同落，hp 已在 inflict 里扣过。
    const int bite = std::min(hpDamage, target.maxHp);
    target.maxHp = std::max(0, target.maxHp - bite);

    // **只有我方吃得下去。** 理由三条（报告与契约里说明过）：双方都吞噬的话大的那个
    // 滚雪球，第二场就成了绿光球反过来把韩立吃掉；原著里贪吃的是韩立；这是自己的识海，
    // 入侵者咬下的那块带不走。于是敌方体积**单调下降**，「必然逃走」是结构性保证。
    std::string text;
    if (attacker.ally) {
        attacker.maxHp += bite;
        attacker.hp += bite;
        text = "，吞下 " + std::to_string(bite) + " 分体积";
    } else {
        text = "，" + target.name + " 被撕下 " + std::to_string(bite) + " 分体积";
    }

    // 逃遁只对敌方生效：这是「敌方逃遁」这个落点，玩家那一侧走的是 Escape 动作。
    if (target.ally || target.fled || !target.alive()) return text;
    const auto index = static_cast<std::size_t>(targetIndex);
    const auto biterIndex = static_cast<std::size_t>(attackerIndex);
    if (index >= startMaxHp_.size() || biterIndex >= startMaxHp_.size()) return text;
    // 先问逃不逃得掉（进来时比吞噬者大才脱得开身），再问该不该逃（少了三分之一）。
    if (!devourCanBreakAway(startMaxHp_[index], startMaxHp_[biterIndex])) return text;
    if (!devourShouldFlee(target.maxHp, startMaxHp_[index])) return text;

    target.fled = true;
    anyEnemyFled_ = true;
    spoilsPercent_ = std::max(spoilsPercent_, devourBittenPercent(target.maxHp, startMaxHp_[index]));
    text += "。" + target.name + " 脱开被咬住的那块，趁势遁走";
    out.fled = true;
    return text;
}

std::string BattleState::applyPoison(int targetIndex, int turns, int power) {
    if (turns <= 0 || power <= 0) return {};
    Unit& target = unitRef(targetIndex);
    // 续毒取「回合数取长、强度取高」，不叠加：叠加等于开了毒理系统的头（契约 2.1）。
    target.poison = std::max(target.poison, turns);
    target.poisonPower = std::max(target.poisonPower, power);
    emit(BattleEvent{BattleEventKind::Poisoned, -1, targetIndex, target.poison, target.poisonPower});
    // 不带前导标点：法术那条接在半句话后面用「，」，物品那条另起用「。」。
    return target.name + " 中毒（" + std::to_string(target.poison) + " 回合，每回合 " +
           std::to_string(target.poisonPower) + " 点）";
}

std::string BattleState::applyItem(const Action& a) {
    const Item item = *findItem(a.magicId);
    // 带 castMagic 的符箓：以那门法术施展一次（契约 docs/interfaces-p3-ch07.md 2.3）。
    // 排在取目标之前：看破的符不挑目标，targetIndex 可以是 -1。
    if (!item.castMagic.empty()) {
        const Magic magic = *findMagic(item.castMagic);   // 合法性已确认（checkItem）
        return resolveMagic(a, magic, &item);
    }
    Unit& user = unitRef(a.actorIndex);
    Unit& target = unitRef(a.targetIndex);

    std::string text = user.name + " 使用 " + item.name + "，" + target.name;
    BattleEvent act{BattleEventKind::Act, a.actorIndex, a.targetIndex, static_cast<int>(ActionKind::Item)};
    act.text = user.name + " 使用 " + item.name;
    emit(std::move(act));

    const int hpGain = std::max(0, std::min(item.restoreHp, target.maxHp - target.hp));
    const int mpGain = std::max(0, std::min(item.restoreMp, target.maxMp - target.mp));
    target.hp += hpGain;
    target.mp += mpGain;

    // 背包扣减由 game 层负责：战斗只认效果，不持有玩家的物品栏。
    bool changed = hpGain > 0 || mpGain > 0;
    if (hpGain > 0) text += " 回复 " + std::to_string(hpGain) + " 点气血";
    if (mpGain > 0) text += (hpGain > 0 ? "、" : " 回复 ") + std::to_string(mpGain) + " 点法力";
    if (changed) {
        BattleEvent heal{BattleEventKind::Heal, a.actorIndex, a.targetIndex, hpGain, mpGain};
        heal.hp = target.hp;
        heal.toughness = target.toughness;
        emit(std::move(heal));
    }

    // 解毒：解的是战斗里的毒，**不碰尸虫丸**。
    if (item.curesPoison && target.poison > 0) {
        target.poison = 0;
        target.poisonPower = 0;
        text += (changed ? "，" : " ");
        text += "体内毒性尽解";
        changed = true;
        emit(BattleEvent{BattleEventKind::Cured, a.actorIndex, a.targetIndex});
    }
    // 毒药：往对手身上使。撒过去算「毒」类一击（不带伤害），先判破绽再挂毒。
    if (item.poison > 0) {
        const std::string weak = probe(a.actorIndex, a.targetIndex, kCategoryPoison);
        if (!weak.empty()) {
            text += weak;
            changed = true;
        }
        const std::string poisoned = applyPoison(a.targetIndex, item.poison, item.poisonPower);
        if (!poisoned.empty()) {
            text += "。" + poisoned;
            changed = true;
        }
    }
    if (!changed) text += " 并无变化";
    return text;
}

std::string BattleState::applyDefend(const Action& a) {
    Unit& u = unitRef(a.actorIndex);
    // 防御：从这一刻起到自己下一次出手之前受伤减半，且下一回合排在最前
    // （docs/octopath-battle.md 2.4）。它替掉了旧的「护体罡气」：罡气是一个会被打穿的数，
    // 减半是一个比例——前者要一套「凝多少、什么时候散」的账，后者只要一个开关。
    // 散的时机与罡气相同：轮到自己下一次出手的那一刻（beginActorTurn）。
    u.guarding = true;
    u.priority = true;
    emit(BattleEvent{BattleEventKind::Guard, a.actorIndex});
    return u.name + " 摆开守势：受伤减半，下回合先手";
}

std::string BattleState::applyEscape(const Action& a) {
    const Unit& u = unitRef(a.actorIndex);
    std::uniform_int_distribution<int> roll(0, 99);
    const bool away = roll(rng_) < escapeChance(u);
    emit(BattleEvent{BattleEventKind::Escape, a.actorIndex, -1, away ? 1 : 0});
    if (away) {
        endBattle(BattlePhase::Escaped);
        return u.name + " 脱离了战斗";
    }
    return u.name + " 脱身不得，逃跑失败";
}

std::string BattleState::applyCharge(const Action& a) {
    Unit& u = unitRef(a.actorIndex);
    u.charging = true;
    BattleEvent declare{BattleEventKind::ChargeDeclare, a.actorIndex};
    declare.value = u.chargeAll ? 1 : 0;
    declare.text = u.chargeTextKey;
    emit(std::move(declare));
    return u.name + " 蓄势待发——下一手是重招" + (u.chargeAll ? "，要横扫全场" : "");
}

}  // namespace fanren::core::battle
