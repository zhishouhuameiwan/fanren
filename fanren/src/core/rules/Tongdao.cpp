#include "core/rules/Tongdao.h"

#include <algorithm>

#include "core/model/AttackCategory.h"

namespace fanren::rules {
namespace {

using core::BondLine;
using core::Companion;
using core::CompanionKind;
using core::GameState;
using core::PerkKind;

[[nodiscard]] const Companion* findCompanion(const std::map<std::string, Companion>& companions,
                                             const std::string& id) {
    const auto it = companions.find(id);
    return it == companions.end() ? nullptr : &it->second;
}

// 人物相处四件事共用的前几道：有这位、在册、是人物。
[[nodiscard]] TongdaoCheck personCheck(const Companion* companion, const GameState& state) {
    if (companion == nullptr) return TongdaoCheck::NoCompanion;
    if (!inRoster(*companion, state)) return TongdaoCheck::NotMet;
    if (companion->kind != CompanionKind::Person) return TongdaoCheck::NotPerson;
    return TongdaoCheck::Ok;
}

[[nodiscard]] int tierOf(const Companion& companion, const GameState& state) {
    return bondTier(state.bondOf(companion.id));
}

[[nodiscard]] std::string flagName(const std::string& companionId, const char* what) {
    return "tongdao." + companionId + "." + what;
}

}  // namespace

// ---------------------------------------------------------------------------
// 1. 生效判据
// ---------------------------------------------------------------------------

bool rosterEffectsActive(const GameState& state) {
    return state.flag(kRosterEffectsStartFlag) != 0 && state.flag(kRosterEffectsEndFlag) == 0;
}

// ---------------------------------------------------------------------------
// 2. 档名
// ---------------------------------------------------------------------------

const char* bondLineName(BondLine line) {
    switch (line) {
        case BondLine::General: return "general";
        case BondLine::Hongyan: return "hongyan";
        case BondLine::Moni:    return "moni";
        case BondLine::Beast:   return "beast";
    }
    return "general";
}

bool bondLineFromName(const std::string& name, BondLine& out) {
    for (const BondLine line : {BondLine::General, BondLine::Hongyan, BondLine::Moni, BondLine::Beast}) {
        if (name == bondLineName(line)) {
            out = line;
            return true;
        }
    }
    return false;
}

std::string bondTierNameKey(BondLine line, int tier) {
    const std::string n = std::to_string(std::clamp(tier, 1, 5));
    if (line == BondLine::General) return "ui.bond.tier" + n;
    return std::string("ui.bond.") + bondLineName(line) + ".tier" + n;
}

int bondTierCap(const Companion& companion, const GameState& state) {
    return bondTier(bondCap(companion, state));
}

// ---------------------------------------------------------------------------
// 3. 种类表
// ---------------------------------------------------------------------------

const std::vector<PerkInfo>& perkTable() {
    // 数值范围是「写成这样就肯定写错了」的护栏，不是平衡；平衡由路 N 定、主会话复审。
    static const std::vector<PerkInfo> table = {
        {PerkKind::ChasePower, "chase_power", kPerkSlotPassive, 1, 30, false, "她追击的伤害 +N%（破势本来就翻倍，上限三成）"},
        {PerkKind::OpeningBp, "opening_bp", kPerkSlotPassive, 1, 2, false,
         "她在场时我方全员开场多 N 点劲（与先手、同修不叠加，取大）"},
        {PerkKind::VictoryHeal, "victory_heal", kPerkSlotPassive, 1, 50, false, "打赢后我方站着的人各回 N% 气血"},
        {PerkKind::AmbushBp, "ambush_bp", kPerkSlotRoster, 1, 2, false, "偷袭明雷得手时我方全员开场再多 N 点劲"},
        {PerkKind::ElementStagger, "element_stagger", kPerkSlotRoster, 1, 2, true,
         "主角 element 属的法术打中破绽时多削 N 点架势"},
        {PerkKind::TalismanPower, "talisman_power", kPerkSlotRoster, 1, 50, false, "用符施展的威力 +N%"},
    };
    return table;
}

const PerkInfo* perkInfo(PerkKind kind) {
    for (const PerkInfo& info : perkTable()) {
        if (info.kind == kind) return &info;
    }
    return nullptr;
}

const PerkInfo* perkByName(const std::string& name) {
    for (const PerkInfo& info : perkTable()) {
        if (name == info.name) return &info;
    }
    return nullptr;
}

int passiveValueAtTier(const core::CompanionPerk& perk, int tier) {
    if (perk.kind == PerkKind::None || tier < kPassiveMinTier) return 0;
    if (tier >= kPassiveStrongTier && perk.value5 > 0) return perk.value5;
    return perk.value;
}

// ---------------------------------------------------------------------------
// 角色那一段
// ---------------------------------------------------------------------------

const core::CompanionRole* companionRoleEntry(const Companion& companion, const std::string& roleId) {
    for (const core::CompanionRole& role : companion.roles) {
        if (role.roleId == roleId) return &role;
    }
    return nullptr;
}

CompanionKit companionKit(const Companion& companion, const std::string& roleId, int bond, bool active) {
    CompanionKit kit;
    kit.duo = companion.duo;
    const core::CompanionRole* role = companionRoleEntry(companion, roleId);
    if (!active || role == nullptr) return kit;
    const int tier = bondTier(bond);
    for (const core::TierMagic& magic : role->tierMagics) {
        if (tier >= magic.tier) kit.extraMagics.push_back(magic.magicId);
    }
    if (!role->duo.empty()) kit.duo = role->duo;
    if (const int value = passiveValueAtTier(role->passive, tier); value > 0) {
        kit.passive = role->passive;
        kit.passive.value = value;
        kit.passive.value5 = value;
    }
    return kit;
}

// ---------------------------------------------------------------------------
// 旗标
// ---------------------------------------------------------------------------

std::string qingjiaoFlag(const std::string& companionId) { return flagName(companionId, "qingjiao"); }
std::string xuhuaFlag(const std::string& companionId) { return flagName(companionId, "xuhua"); }
std::string sparDayFlag(const std::string& companionId) { return flagName(companionId, "spar_day"); }
std::string sparPrizeFlag(const std::string& companionId) { return flagName(companionId, "spar_prize"); }
std::string bonusOnFlag(const std::string& companionId) { return flagName(companionId, "bonus_on"); }

// ---------------------------------------------------------------------------
// 4. 在册加成
// ---------------------------------------------------------------------------

int rosterBonusSlots(Realm realm) {
    return toValue(realm) >= toValue(Realm::FoundationEarly) ? kRosterBonusSlotsFoundation : kRosterBonusSlotsQi;
}

bool rosterBonusUnlocked(const Companion& companion, const GameState& state) {
    if (companion.rosterBonus.kind == PerkKind::None || !inRoster(companion, state)) return false;
    if (companion.kind == CompanionKind::Beast) return tierOf(companion, state) >= 2;
    return state.flag(qingjiaoFlag(companion.id)) != 0;
}

std::vector<std::string> activeRosterBonuses(const std::map<std::string, Companion>& companions,
                                             const GameState& state) {
    std::vector<std::string> picked;
    std::vector<std::string> unpicked;
    for (const auto& [id, companion] : companions) {
        if (!rosterBonusUnlocked(companion, state)) continue;
        (state.flag(bonusOnFlag(id)) != 0 ? picked : unpicked).push_back(id);
    }
    const auto slots = static_cast<std::size_t>(rosterBonusSlots(state.realm));
    std::vector<std::string> out;
    for (const std::vector<std::string>* list : {&picked, &unpicked}) {
        for (const std::string& id : *list) {
            if (out.size() < slots) out.push_back(id);
        }
    }
    std::sort(out.begin(), out.end());
    return out;
}

// ---------------------------------------------------------------------------
// 战斗修正
// ---------------------------------------------------------------------------

bool tongxiuBonusReady(const GameState& state) {
    const int day = state.flag(kTongxiuBonusDayFlag);
    return day != 0 && day == state.day;
}

BattleBonuses battleBonuses(const std::map<std::string, Companion>& companions, const GameState& state,
                            const std::vector<core::CompanionPerk>& fieldedPassives, bool spar, bool heroPresent) {
    BattleBonuses out;
    if (spar || !heroPresent || !rosterEffectsActive(state)) return out;

    // 伙伴被动（上场的才算）。同种几位都有时取大：开场的劲本来就「不叠加取大」，回血两位一起回也不该翻倍。
    for (const core::CompanionPerk& perk : fieldedPassives) {
        switch (perk.kind) {
            case PerkKind::OpeningBp:   out.openingBp = std::max(out.openingBp, perk.value); break;
            case PerkKind::VictoryHeal: out.victoryHealPercent = std::max(out.victoryHealPercent, perk.value); break;
            default: break;   // ChasePower 挂在她自己的单位上（Unit::chasePowerPercent），不在这里
        }
    }
    // 同修：第二天第一场战斗全队开场多 1 点劲（与先手、她的被动不叠加，取大）。
    if (tongxiuBonusReady(state)) {
        out.openingBp = std::max(out.openingBp, kTongxiuOpeningBp);
        out.usesTongxiu = true;
    }
    // 在册加成：此刻生效的那几条，不论她在不在场。
    for (const std::string& id : activeRosterBonuses(companions, state)) {
        const core::CompanionPerk& perk = companions.at(id).rosterBonus;
        switch (perk.kind) {
            case PerkKind::AmbushBp:      out.ambushBp += perk.value; break;
            case PerkKind::TalismanPower: out.talismanPowerPercent += perk.value; break;
            case PerkKind::ElementStagger:
                out.heroStagger.push_back(StaggerBonus{core::categoriesOfElement(perk.element), perk.value});
                break;
            default: break;   // 伙伴被动的种类不许写进在册加成（加载器挡）
        }
    }
    return out;
}

// ---------------------------------------------------------------------------
// 5. 相处
// ---------------------------------------------------------------------------

const char* tongdaoCheckCode(TongdaoCheck check) {
    switch (check) {
        case TongdaoCheck::Ok:          return "";
        case TongdaoCheck::NoCompanion: return "no_companion";
        case TongdaoCheck::NotMet:      return "not_met";
        case TongdaoCheck::NotPerson:   return "not_person";
        case TongdaoCheck::TierLow:     return "tier_low";
        case TongdaoCheck::DoneToday:   return "done_today";
        case TongdaoCheck::AlreadyDone: return "already";
        case TongdaoCheck::NoSpar:      return "no_spar";
    }
    return "";
}

bool inRoster(const Companion& companion, const GameState& state) {
    return !companion.metFlag.empty() && state.flag(companion.metFlag) != 0;
}

TongdaoCheck canTongxiu(const std::map<std::string, Companion>& companions, const std::string& companionId,
                        const GameState& state) {
    const Companion* companion = findCompanion(companions, companionId);
    if (const TongdaoCheck check = personCheck(companion, state); check != TongdaoCheck::Ok) return check;
    if (state.flag(kTongxiuDayFlag) == state.day) return TongdaoCheck::DoneToday;
    return TongdaoCheck::Ok;
}

BondGain tongxiu(const std::map<std::string, Companion>& companions, const std::string& companionId,
                 GameState& state) {
    BondGain out;
    out.check = canTongxiu(companions, companionId, state);
    if (out.check != TongdaoCheck::Ok) return out;
    const Companion& companion = companions.at(companionId);
    core::BondRecord& record = state.bonds[companionId];
    const int before = record.value;
    addBond(record, kBondTongxiu, bondCap(companion, state));
    out.gained = record.value - before;
    state.setFlag(kTongxiuDayFlag, state.day);
    state.setFlag(kTongxiuBonusDayFlag, state.day + 1);
    return out;
}

TongdaoCheck canSpar(const std::map<std::string, Companion>& companions, const std::string& companionId,
                     const GameState& state) {
    const Companion* companion = findCompanion(companions, companionId);
    if (const TongdaoCheck check = personCheck(companion, state); check != TongdaoCheck::Ok) return check;
    if (companion->sparBattle.empty()) return TongdaoCheck::NoSpar;
    if (tierOf(*companion, state) < kSparMinTier) return TongdaoCheck::TierLow;
    if (state.flag(sparDayFlag(companionId)) == state.day) return TongdaoCheck::DoneToday;
    return TongdaoCheck::Ok;
}

SparOutcome recordSpar(const std::map<std::string, Companion>& companions, const std::string& companionId,
                       GameState& state, bool won) {
    SparOutcome out;
    out.check = canSpar(companions, companionId, state);
    if (out.check != TongdaoCheck::Ok) return out;
    const Companion& companion = companions.at(companionId);
    const int tier = tierOf(companion, state);   // 开打时的档
    state.setFlag(sparDayFlag(companionId), state.day);
    core::BondRecord& record = state.bonds[companionId];
    const int before = record.value;
    addBond(record, won ? kBondSparWin : kBondSparLoss, bondCap(companion, state));
    out.gained = record.value - before;
    if (const int paid = state.flag(sparPrizeFlag(companionId)); won && paid < tier) {
        state.setFlag(sparPrizeFlag(companionId), tier);
        // 此刻这一档及以下、还没领过的都发：一次升了两档，中间那一档的奖品不丢。
        for (const core::SparPrize& prize : companion.sparPrizes) {
            if (prize.tier <= paid || prize.tier > tier) continue;
            out.prizeTier = prize.tier;
            for (const core::BagEntry& entry : prize.items) {
                out.prize.push_back(entry);
                state.addItem(entry.itemId, entry.count, entry.herbAge);
            }
        }
    }
    return out;
}

TongdaoCheck canQingjiao(const std::map<std::string, Companion>& companions, const std::string& companionId,
                         const GameState& state) {
    const Companion* companion = findCompanion(companions, companionId);
    if (const TongdaoCheck check = personCheck(companion, state); check != TongdaoCheck::Ok) return check;
    if (tierOf(*companion, state) < kQingjiaoMinTier) return TongdaoCheck::TierLow;
    if (state.flag(qingjiaoFlag(companionId)) != 0) return TongdaoCheck::AlreadyDone;
    return TongdaoCheck::Ok;
}

TongdaoCheck recordQingjiao(const std::map<std::string, Companion>& companions, const std::string& companionId,
                            GameState& state) {
    const TongdaoCheck check = canQingjiao(companions, companionId, state);
    if (check == TongdaoCheck::Ok) state.setFlag(qingjiaoFlag(companionId), 1);
    return check;
}

int xuhuaPending(const std::map<std::string, Companion>& companions, const std::string& companionId,
                 const GameState& state) {
    const Companion* companion = findCompanion(companions, companionId);
    if (personCheck(companion, state) != TongdaoCheck::Ok) return 0;
    const int tier = tierOf(*companion, state);
    const int next = std::max(kXuhuaMinTier, state.flag(xuhuaFlag(companionId)) + 1);
    return next <= tier ? next : 0;
}

TongdaoCheck recordXuhua(const std::map<std::string, Companion>& companions, const std::string& companionId,
                         GameState& state, int tier) {
    const Companion* companion = findCompanion(companions, companionId);
    if (const TongdaoCheck check = personCheck(companion, state); check != TongdaoCheck::Ok) return check;
    if (tier < kXuhuaMinTier || tier > tierOf(*companion, state)) return TongdaoCheck::TierLow;
    if (state.flag(xuhuaFlag(companionId)) >= tier) return TongdaoCheck::AlreadyDone;
    state.setFlag(xuhuaFlag(companionId), tier);
    return TongdaoCheck::Ok;
}

GiftResult giveGiftTo(const std::map<std::string, Companion>& companions, const std::string& companionId,
                      const std::string& itemId, GameState& state) {
    GiftResult out;
    const Companion* companion = findCompanion(companions, companionId);
    if (companion == nullptr) {
        out.check = TongdaoCheck::NoCompanion;
        return out;
    }
    if (!inRoster(*companion, state)) {
        out.check = TongdaoCheck::NotMet;
        return out;
    }
    out.outcome = giveGift(state.bonds[companionId], *companion, itemId, state.day, bondCap(*companion, state));
    return out;
}

}  // namespace fanren::rules
