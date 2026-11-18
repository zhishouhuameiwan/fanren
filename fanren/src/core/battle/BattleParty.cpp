// 伙伴改版在战斗规则层的那一半（docs/interfaces-companions.md 3.2 追击、3.3 合招、3.10 人数系数）。
// 与 BattleAction.cpp、BattleAi.cpp 一样是 BattleState 的成员，只是按关注点另起一个文件。
//
// 追击与合招只认 Unit 上的 hero / companion / bond / canChase / duo 这几样（game 层建场时按伙伴数据填），
// 不认角色 id。不是伙伴的队员（傀儡、化身、曲魂、编成里写死的友军）这几样全是缺省值，下面每一条都碰不到他们：
// 第 8–12 章带着他们的仗一个事件也不会多。
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <string>

#include "core/battle/Battle.h"
#include "core/battle/Damage.h"
#include "core/rules/Companions.h"

namespace fanren::core::battle {
namespace {

bool deny(std::string* why, std::string text) {
    if (why != nullptr) *why = std::move(text);
    return false;
}

[[nodiscard]] const char* absenceReason(const Unit& unit) { return unit.onField ? "已经倒下" : "尚未入场"; }

// 一个单位的第一样兵刃（位序最前的那一位）；一样都没有时为 0。与 BattleAction.cpp 的 primaryWeapon 同一口径：
// 追击「按追击者自己的兵刃类别」，就是他不点名兵刃时普攻出的那一样。
[[nodiscard]] int firstWeapon(int weapons) {
    const std::vector<int> bits = categoryBits(weapons);
    return bits.empty() ? kCategoryNone : bits.front();
}

[[nodiscard]] bool isCompanion(const Unit& unit) { return unit.ally && !unit.companion.empty(); }

}  // namespace

// ---------------------------------------------------------------------------
// 人数系数（3.10）
// ---------------------------------------------------------------------------

int partyHpPercent(int allies) {
    constexpr int kLast = static_cast<int>(std::size(kPartyHpPercent)) - 1;
    const int row = std::clamp(allies - 1, 0, kLast);
    return kPartyHpPercent[row];
}

int scaledPartyHp(int maxHp, int allies) {
    const long long scaled = (static_cast<long long>(maxHp) * partyHpPercent(allies) + 50) / 100;
    return static_cast<int>(std::max(1LL, scaled));
}

// ---------------------------------------------------------------------------
// 合招（3.3）
// ---------------------------------------------------------------------------

int BattleState::duoCompanionIndex(int first, int second) const {
    if (!validIndex(first) || !validIndex(second) || first == second) return -1;
    const Unit& a = unitRef(first);
    const Unit& b = unitRef(second);
    if (!a.ally || !b.ally) return -1;
    if (a.hero && isCompanion(b)) return second;
    if (b.hero && isCompanion(a)) return first;
    return -1;   // 伙伴之间不合招，不是伙伴的队员也不合
}

std::vector<int> BattleState::duoPartners(int actorIndex) const {
    std::vector<int> partners;
    if (!validIndex(actorIndex)) return partners;
    for (std::size_t i = 0; i < units_.size(); ++i) {
        const int other = static_cast<int>(i);
        const int companion = duoCompanionIndex(actorIndex, other);
        if (companion >= 0 && !unitRef(companion).duo.empty()) partners.push_back(other);
    }
    return partners;
}

std::string BattleState::duoMagicOf(int actorIndex, int partnerIndex) const {
    const int companion = duoCompanionIndex(actorIndex, partnerIndex);
    return companion < 0 ? std::string{} : unitRef(companion).duo;
}

bool BattleState::checkDuo(const Action& a, const Unit& u, std::string* why) const {
    if (devour_) return deny(why, "这一场使不出合招");
    if (!validIndex(a.partnerIndex) || a.partnerIndex == a.actorIndex) return deny(why, "合招要两个人一起出");
    const Unit& partner = unitRef(a.partnerIndex);
    const int companionIndex = duoCompanionIndex(a.actorIndex, a.partnerIndex);
    if (companionIndex < 0) return deny(why, u.name + " 与 " + partner.name + " 合不了招");
    const Unit& companion = unitRef(companionIndex);
    if (companion.duo.empty()) return deny(why, companion.name + " 没有合招");
    if (rules::bondTier(companion.bond) < kDuoMinBondTier) {
        return deny(why, "与 " + companion.name + " 的默契还不到「知己」，合不了招");
    }
    if (!partner.alive()) return deny(why, partner.name + " " + absenceReason(partner));
    if (partner.acted || partner.duoSpentRound == round_) return deny(why, partner.name + " 本回合已经行动过");
    for (const Unit* who : {&u, &partner}) {
        if (who->bp < kDuoBpCost) {
            return deny(why, who->name + " 的劲不够：合招要 " + std::to_string(kDuoBpCost) + " 点，只有 " +
                                 std::to_string(who->bp) + " 点");
        }
    }
    const Magic* magic = findMagic(companion.duo);
    if (magic == nullptr) return deny(why, "没有这门法术：" + companion.duo);
    if (a.magicId != companion.duo) return deny(why, "这一对合的不是【" + a.magicId + "】");
    if (!castableMagic(*magic)) return deny(why, "【" + magic->name + "】不是伤人的法术");
    return checkMagicTarget(a, *magic, u, why);
}

std::string BattleState::applyDuo(const Action& a) {
    // 合法性已确认（checkDuo）。代价：两人各扣 kDuoBpCost 点劲；两人本回合的行动都算用掉；两人都算
    //「本回合蓄过劲」，下一回合开始不加劲（与蓄劲同一条规矩，docs/octopath-battle.md 2.3）。
    Unit& initiator = unitRef(a.actorIndex);
    Unit& partner = unitRef(a.partnerIndex);
    initiator.bp -= kDuoBpCost;
    partner.bp -= kDuoBpCost;
    initiator.boosted = true;
    partner.boosted = true;
    partner.acted = true;
    partner.duoSpentRound = round_;
    // 陪同的那一位这一回合的行动用掉了，轮到他时会被跳过（skipIneligibleActors），也就走不到 beginActorTurn——
    // 他上一回合摆的守势得在这里当场散去，说法与 beginActorTurn 同一句，否则他白得一整回合的减半。
    if (partner.guarding) {
        partner.guarding = false;
        log_.push_back(partner.name + " 收起守势");
        emit(BattleEvent{BattleEventKind::GuardEnd, a.partnerIndex});
    }
    // 效果：那门法术由发起人施放——境界压制、killableBy、事件归属都算发起人的（resolveMagic 按 actorIndex 走）。
    const Magic magic = *findMagic(a.magicId);
    return resolveMagic(a, magic, nullptr, a.partnerIndex);
}

void BattleState::emitCastAct(const Action& a, int target, int category, const Item* viaItem, int duoPartner,
                              int hits, std::string text) {
    const ActionKind kind = duoPartner >= 0     ? ActionKind::Duo
                            : viaItem != nullptr ? ActionKind::Item
                                                 : ActionKind::Cast;
    BattleEvent act{BattleEventKind::Act, a.actorIndex, target, static_cast<int>(kind)};
    act.category = category;
    act.hits = hits;
    act.text = std::move(text);
    emit(std::move(act));
    if (duoPartner >= 0) emit(BattleEvent{BattleEventKind::DuoJoin, a.actorIndex, duoPartner, kDuoBpCost});
}

// ---------------------------------------------------------------------------
// 追击（3.2）
// ---------------------------------------------------------------------------

int BattleState::pickChaser(int actorIndex) const {
    const Unit& actor = unitRef(actorIndex);
    const auto canStillChase = [this](const Unit& u) { return u.alive() && u.chasedRound != round_; };
    if (actor.hero) {
        // 韩立出手：场上默契到「相熟」、chase 为真、站着、本回合还没追过的伙伴里取默契最高的；
        // 并列取队伍次序靠前的（伙伴按队伍次序上场，单位下标就是队伍次序）。
        int best = -1;
        for (std::size_t i = 0; i < units_.size(); ++i) {
            const Unit& u = units_[i];
            if (!isCompanion(u) || !u.canChase || !canStillChase(u)) continue;
            if (rules::bondTier(u.bond) < kChaseMinBondTier) continue;
            if (best < 0 || u.bond > unitRef(best).bond) best = static_cast<int>(i);
        }
        return best;
    }
    if (!isCompanion(actor) || rules::bondTier(actor.bond) < kChaseMinBondTier) return -1;
    // 伙伴出手：韩立追（他在场、站着、本回合还没追过）。伙伴之间不互相追击。
    for (std::size_t i = 0; i < units_.size(); ++i) {
        const Unit& u = units_[i];
        if (u.ally && u.hero && canStillChase(u)) return static_cast<int>(i);
    }
    return -1;
}

std::string BattleState::resolveChase(int actorIndex, std::size_t fromEvent) {
    if (phase_ != BattlePhase::Ongoing || devour_ || !validIndex(actorIndex)) return {};
    const Unit& actor = unitRef(actorIndex);
    if (!actor.ally || (!actor.hero && actor.companion.empty())) return {};

    // 这一手里**第一个**进入破势的敌人（群攻打出好几个破势也只追这一个）。它已经倒下了就不追：
    // 追击补的是一记普攻，打不了倒下的人；也不改去追第二个——契约说只对第一个追一次。
    int target = -1;
    for (std::size_t i = fromEvent; i < events_.size(); ++i) {
        const BattleEvent& e = events_[i];
        if (e.kind != BattleEventKind::Break || !validIndex(e.target) || unitRef(e.target).ally) continue;
        target = e.target;
        break;
    }
    if (target < 0 || !unitRef(target).alive()) return {};

    const int chaser = pickChaser(actorIndex);
    if (chaser < 0) return {};
    Unit& c = unitRef(chaser);
    c.chasedRound = round_;
    // 补一记普攻：按追击者自己的兵刃类别；不占行动、不耗劲、不能蓄劲；目标在破势里，伤害照常吃 ×2。
    const Unit& t = unitRef(target);
    const int category = firstWeapon(c.weapons);
    std::string head = c.name + " 追击 " + t.name;
    if (category != kCategoryNone) head += "（" + std::string(categoryName(category)) + "）";
    BattleEvent chase{BattleEventKind::Chase, chaser, target, actorIndex};
    chase.category = category;
    chase.hits = 1;
    chase.text = head;
    emit(std::move(chase));
    const int base = physicalDamage(c.attack, t.defence, c.realm, t.realm, c.element, t.element);
    return head + "，" + strike(chaser, target, base, category, 1, 1);
}

}  // namespace fanren::core::battle
