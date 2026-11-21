// 同道录 2a·路 K（docs/ch07-tongdaolu.md 第 3 节；接口文档 docs/ch07-tongdaolu-k.md）：规则层 core/rules/Tongdao.*
// 与 Companions.* 的封顶。纯函数，不开 Application：生效判据、档名、种类表、按档的 kit、在册加成与封顶、
// 建场修正、同修 / 切磋 / 请教 / 叙话 / 赠礼的记账与每日限制。
#include <gtest/gtest.h>

#include <map>
#include <set>
#include <string>
#include <vector>

#include "core/model/AttackCategory.h"
#include "core/model/Types.h"
#include "core/rules/Companions.h"
#include "core/rules/Realm.h"
#include "core/rules/Tongdao.h"

namespace {

using fanren::core::BondLine;
using fanren::core::Companion;
using fanren::core::CompanionKind;
using fanren::core::CompanionPerk;
using fanren::core::GameState;
using fanren::core::PerkKind;
namespace rules = fanren::rules;
using rules::TongdaoCheck;

CompanionPerk perk(PerkKind kind, int value, int value5 = 0, int element = 0) {
    CompanionPerk p;
    p.kind = kind;
    p.value = value;
    p.value5 = value5 == 0 ? value : value5;
    p.element = element;
    return p;
}

// 三位探针：「晴」是人物、红颜线、有切磋与奖品、有在册加成；「婉」封在四档；「兽」是灵兽。
std::map<std::string, Companion> probes() {
    std::map<std::string, Companion> out;
    Companion qing;
    qing.id = "qing";
    qing.metFlag = "probe.qing";
    qing.line = BondLine::Hongyan;
    fanren::core::CompanionRole early;
    early.roleId = "qing_ch07";
    early.untilFlag = "ch07.done";
    early.tierMagics = {{3, "magic_two"}, {4, "magic_three"}};
    early.passive = perk(PerkKind::ChasePower, 20, 40);
    early.duo = "magic_duo_qing";
    fanren::core::CompanionRole later;
    later.roleId = "qing_ch08";
    qing.roles = {early, later};
    qing.duo = "magic_duo_old";
    qing.rosterBonus = perk(PerkKind::AmbushBp, 1);
    qing.sparBattle = "spar_qing";
    qing.sparPrizes = {{2, {{"pill_a", 2, 0}}}, {4, {{"pill_b", 1, 0}}}};
    out.emplace(qing.id, qing);

    Companion wan;
    wan.id = "wan";
    wan.metFlag = "probe.wan";
    wan.roles = {{"wan_ch07"}};
    wan.capFlag = "ch07.done";
    wan.capTier = 4;
    wan.rosterBonus = perk(PerkKind::ElementStagger, 1, 0, fanren::core::kElementFire);
    wan.sparBattle = "spar_wan";
    out.emplace(wan.id, wan);

    Companion beast;
    beast.id = "beast";
    beast.metFlag = "probe.beast";
    beast.kind = CompanionKind::Beast;
    beast.line = BondLine::Beast;
    beast.roles = {{"beast_role"}};
    beast.rosterBonus = perk(PerkKind::TalismanPower, 10);
    out.emplace(beast.id, beast);
    return out;
}

GameState ch07State() {
    GameState state;
    state.day = 10;
    state.realm = fanren::rules::Realm::QiRefining13;
    state.setFlag("ch06.done");
    for (const char* flag : {"probe.qing", "probe.wan", "probe.beast"}) state.setFlag(flag);
    return state;
}

// ---------------------------------------------------------------------------
// 1. 生效判据
// ---------------------------------------------------------------------------

TEST(TongdaoEffects, ActiveOnlyBetweenTheEndOfChapterSixAndTheEndOfChapterSeven) {
    GameState state;
    EXPECT_FALSE(rules::rosterEffectsActive(state)) << "第 6 章还没演完";
    state.setFlag("ch06.done");
    EXPECT_TRUE(rules::rosterEffectsActive(state)) << "第 7 章里";
    state.setFlag("ch07.done");
    EXPECT_FALSE(rules::rosterEffectsActive(state)) << "第 7 章演完了：第 8 章起行为不变";
}

// ---------------------------------------------------------------------------
// 2. 档名与封顶
// ---------------------------------------------------------------------------

TEST(TongdaoTierNames, GeneralKeepsTheOldKeysAndOtherLinesHaveTheirOwn) {
    EXPECT_EQ(rules::bondTierNameKey(BondLine::General, 3), "ui.bond.tier3") << "通用线沿用改版前的 key";
    EXPECT_EQ(rules::bondTierNameKey(BondLine::Hongyan, 5), "ui.bond.hongyan.tier5");
    EXPECT_EQ(rules::bondTierNameKey(BondLine::Moni, 5), "ui.bond.moni.tier5");
    EXPECT_EQ(rules::bondTierNameKey(BondLine::Beast, 1), "ui.bond.beast.tier1");
    EXPECT_EQ(rules::bondTierNameKey(BondLine::General, 0), "ui.bond.tier1") << "越界夹住";
    EXPECT_EQ(rules::bondTierNameKey(BondLine::General, 9), "ui.bond.tier5");
    for (const auto line : {BondLine::General, BondLine::Hongyan, BondLine::Moni, BondLine::Beast}) {
        BondLine back = BondLine::General;
        ASSERT_TRUE(rules::bondLineFromName(rules::bondLineName(line), back));
        EXPECT_EQ(back, line);
    }
    BondLine unused = BondLine::General;
    EXPECT_FALSE(rules::bondLineFromName("romance", unused));
}

TEST(TongdaoCap, NangongStopsAtNinetyFourUntilTheFlagAndEveryBondSourceRespectsIt) {
    const auto companions = probes();
    const Companion& wan = companions.at("wan");
    GameState state = ch07State();
    EXPECT_EQ(rules::bondCap(wan, state), 94) << "四档的最高值 = 五档门槛 95 减一";
    EXPECT_EQ(rules::bondTierCap(wan, state), 4);
    EXPECT_EQ(rules::bondCapOf(companions, "qing", state), rules::kBondMax) << "没写封顶";
    EXPECT_EQ(rules::bondCapOf(companions, "nobody", state), rules::kBondMax);

    fanren::core::BondRecord record;
    record.value = 90;
    EXPECT_TRUE(rules::addBond(record, 10, rules::bondCap(wan, state)));
    EXPECT_EQ(record.value, 94) << "剧情 bond.add 也到 94 为止";
    EXPECT_EQ(rules::addBattleBond(record, state.day, rules::bondCap(wan, state)), 0);
    EXPECT_EQ(record.value, 94);
    const rules::GiftOutcome gift = rules::giveGift(record, wan, "pill_x", state.day, rules::bondCap(wan, state));
    EXPECT_TRUE(gift.given) << "到了顶也算今天送过了";
    EXPECT_EQ(gift.gained, 0);
    EXPECT_EQ(rules::bondTier(record.value), 4);

    fanren::core::BondRecord old;
    old.value = 97;   // 旧档里已经高过封顶
    EXPECT_TRUE(rules::addBond(old, 5, 94));
    EXPECT_EQ(old.value, 97) << "不往下压，只是不再涨";

    state.setFlag("ch07.done");
    EXPECT_EQ(rules::bondCap(wan, state), rules::kBondMax) << "第 7 章演完封顶解开";
    EXPECT_EQ(rules::bondTierCap(wan, state), 5);
    EXPECT_TRUE(rules::addBond(record, 10, rules::bondCap(wan, state)));
    EXPECT_EQ(record.value, 100);
}

TEST(TongdaoCap, TheOldCallsWithoutACapBehaveExactlyAsBefore) {
    fanren::core::BondRecord record;
    EXPECT_TRUE(rules::addBond(record, 250));
    EXPECT_EQ(record.value, 100);
    fanren::core::BondRecord battle;
    for (int i = 0; i < 5; ++i) static_cast<void>(rules::addBattleBond(battle, 1));
    EXPECT_EQ(battle.value, rules::kBondBattleDailyCap);
}

// ---------------------------------------------------------------------------
// 3. 种类表与 kit
// ---------------------------------------------------------------------------

TEST(TongdaoPerks, TheTableIsFiniteNamedUniquelyAndCoversTheSixKindsOfTheBlueprint) {
    std::set<std::string> names;
    std::set<PerkKind> kinds;
    for (const rules::PerkInfo& info : rules::perkTable()) {
        EXPECT_TRUE(names.insert(info.name).second) << info.name;
        EXPECT_TRUE(kinds.insert(info.kind).second);
        EXPECT_NE(info.kind, PerkKind::None);
        EXPECT_NE(info.slots, 0);
        EXPECT_LE(info.minValue, info.maxValue);
        EXPECT_EQ(rules::perkByName(info.name), &info);
        EXPECT_EQ(rules::perkInfo(info.kind), &info);
    }
    EXPECT_EQ(names, (std::set<std::string>{"chase_power", "opening_bp", "victory_heal", "ambush_bp",
                                            "element_stagger", "talisman_power"}));
    EXPECT_EQ(rules::perkByName("stun_all"), nullptr) << "表外的种类认不出（加载器据此报错）";
    EXPECT_EQ(rules::perkInfo(PerkKind::None), nullptr);
}

TEST(TongdaoPerks, APassiveStartsAtTierThreeAndGrowsOneStepAtTierFive) {
    const CompanionPerk p = perk(PerkKind::ChasePower, 20, 40);
    EXPECT_EQ(rules::passiveValueAtTier(p, 2), 0);
    EXPECT_EQ(rules::passiveValueAtTier(p, 3), 20);
    EXPECT_EQ(rules::passiveValueAtTier(p, 4), 20);
    EXPECT_EQ(rules::passiveValueAtTier(p, 5), 40);
    CompanionPerk flat = perk(PerkKind::OpeningBp, 1);
    flat.value5 = 0;
    EXPECT_EQ(rules::passiveValueAtTier(flat, 5), 1) << "没写五档的数按三档的";
    EXPECT_EQ(rules::passiveValueAtTier(CompanionPerk{}, 5), 0);
}

TEST(TongdaoKit, SpellsUnlockByTierOnlyInChapterSevenAndOnlyForTheRoleThatListsThem) {
    const auto companions = probes();
    const Companion& qing = companions.at("qing");
    const auto bondFor = [](int tier) { return rules::kBondTierThresholds[tier - 1]; };

    rules::CompanionKit kit = rules::companionKit(qing, "qing_ch07", bondFor(2), true);
    EXPECT_TRUE(kit.extraMagics.empty()) << "二档：只有角色表里的一门";
    EXPECT_EQ(kit.passive.kind, PerkKind::None);
    EXPECT_EQ(kit.duo, "magic_duo_qing") << "这一段写了合招，第 7 章里用它（档位门槛由战斗规则判）";

    kit = rules::companionKit(qing, "qing_ch07", bondFor(3), true);
    EXPECT_EQ(kit.extraMagics, (std::vector<std::string>{"magic_two"}));
    EXPECT_EQ(kit.passive.kind, PerkKind::ChasePower);
    EXPECT_EQ(kit.passive.value, 20);

    kit = rules::companionKit(qing, "qing_ch07", bondFor(5), true);
    EXPECT_EQ(kit.extraMagics, (std::vector<std::string>{"magic_two", "magic_three"}));
    EXPECT_EQ(kit.passive.value, 40) << "五档加强一级";

    kit = rules::companionKit(qing, "qing_ch07", bondFor(5), false);
    EXPECT_TRUE(kit.extraMagics.empty()) << "判据不成立：什么都不加";
    EXPECT_EQ(kit.passive.kind, PerkKind::None);
    EXPECT_EQ(kit.duo, "magic_duo_old") << "判据不成立：照用顶层的合招，与改版前相同";

    kit = rules::companionKit(qing, "qing_ch08", bondFor(5), true);
    EXPECT_TRUE(kit.extraMagics.empty()) << "第 8 章起的角色不写，行为不变";
    EXPECT_EQ(kit.duo, "magic_duo_old");
}

// ---------------------------------------------------------------------------
// 4. 在册加成
// ---------------------------------------------------------------------------

TEST(TongdaoRosterBonus, PeopleNeedQingjiaoBeastsNeedTierTwoAndEveryoneMustBeInTheRoster) {
    const auto companions = probes();
    GameState state = ch07State();
    EXPECT_FALSE(rules::rosterBonusUnlocked(companions.at("qing"), state)) << "人物：还没请教";
    state.setFlag(rules::qingjiaoFlag("qing"));
    EXPECT_TRUE(rules::rosterBonusUnlocked(companions.at("qing"), state));
    EXPECT_FALSE(rules::rosterBonusUnlocked(companions.at("beast"), state)) << "灵兽：还在一档";
    state.bonds["beast"].value = 20;
    EXPECT_TRUE(rules::rosterBonusUnlocked(companions.at("beast"), state));
    state.setFlag("probe.beast", 0);
    EXPECT_FALSE(rules::rosterBonusUnlocked(companions.at("beast"), state)) << "不在同道录里";
}

TEST(TongdaoRosterBonus, SlotsAreFourInQiRefiningAndSixInFoundation) {
    EXPECT_EQ(rules::rosterBonusSlots(fanren::rules::Realm::Mortal), 4);
    EXPECT_EQ(rules::rosterBonusSlots(fanren::rules::Realm::QiRefining13), 4);
    EXPECT_EQ(rules::rosterBonusSlots(fanren::rules::Realm::FoundationEarly), 6);
    EXPECT_EQ(rules::rosterBonusSlots(fanren::rules::Realm::CoreLate), 6);
}

TEST(TongdaoRosterBonus, OverTheCapTheTickedOnesComeFirstThenTheRestInOrder) {
    // 六位都解锁了，炼气只有 4 个名额。
    std::map<std::string, Companion> companions;
    GameState state = ch07State();
    for (const char* id : {"a", "b", "c", "d", "e", "f"}) {
        Companion c;
        c.id = id;
        c.metFlag = std::string("probe.") + id;
        c.roles = {{std::string(id) + "_role"}};
        c.rosterBonus = perk(PerkKind::TalismanPower, 5);
        companions.emplace(id, c);
        state.setFlag(c.metFlag);
        state.setFlag(rules::qingjiaoFlag(id));
    }
    EXPECT_EQ(rules::activeRosterBonuses(companions, state), (std::vector<std::string>{"a", "b", "c", "d"}))
        << "一个都没勾：按次序取满";
    state.setFlag(rules::bonusOnFlag("f"));
    state.setFlag(rules::bonusOnFlag("e"));
    EXPECT_EQ(rules::activeRosterBonuses(companions, state), (std::vector<std::string>{"a", "b", "e", "f"}))
        << "勾上的先取，余下的名额按次序补";
    state.realm = fanren::rules::Realm::FoundationEarly;
    EXPECT_EQ(rules::activeRosterBonuses(companions, state).size(), 6u) << "筑基 6 条，全上";
}

// ---------------------------------------------------------------------------
// 建场修正
// ---------------------------------------------------------------------------

TEST(TongdaoBattleBonuses, NothingAppliesOutsideChapterSevenInASparOrWithoutTheHero) {
    const auto companions = probes();
    GameState state = ch07State();
    state.setFlag(rules::qingjiaoFlag("qing"));
    state.setFlag(rules::kTongxiuBonusDayFlag, state.day);
    const std::vector<CompanionPerk> passives{perk(PerkKind::OpeningBp, 1), perk(PerkKind::VictoryHeal, 10)};
    const rules::BattleBonuses on = rules::battleBonuses(companions, state, passives, false, true);
    EXPECT_EQ(on.openingBp, 1);
    EXPECT_EQ(on.ambushBp, 1);
    EXPECT_EQ(on.victoryHealPercent, 10);
    EXPECT_TRUE(on.usesTongxiu);

    for (const auto& [spar, hero] : std::vector<std::pair<bool, bool>>{{true, true}, {false, false}}) {
        const rules::BattleBonuses off = rules::battleBonuses(companions, state, passives, spar, hero);
        EXPECT_EQ(off.openingBp, 0);
        EXPECT_EQ(off.ambushBp, 0);
        EXPECT_EQ(off.victoryHealPercent, 0);
        EXPECT_FALSE(off.usesTongxiu) << "切磋、韩立不在场都不用掉同修的劲";
    }
    state.setFlag("ch07.done");
    const rules::BattleBonuses later = rules::battleBonuses(companions, state, passives, false, true);
    EXPECT_EQ(later.openingBp, 0) << "第 8 章起一律不套";
    EXPECT_EQ(later.ambushBp, 0);
    EXPECT_FALSE(later.usesTongxiu);
}

TEST(TongdaoBattleBonuses, OpeningSourcesTakeTheLargestAndRosterBonusesAddUp) {
    const auto companions = probes();
    GameState state = ch07State();
    state.setFlag(rules::qingjiaoFlag("qing"));
    state.setFlag(rules::qingjiaoFlag("wan"));
    state.bonds["beast"].value = 20;
    state.setFlag(rules::kTongxiuBonusDayFlag, state.day);
    const rules::BattleBonuses b =
        rules::battleBonuses(companions, state, {perk(PerkKind::OpeningBp, 2), perk(PerkKind::VictoryHeal, 10),
                                                  perk(PerkKind::VictoryHeal, 20)},
                             false, true);
    EXPECT_EQ(b.openingBp, 2) << "她的 2 点与同修的 1 点取大";
    EXPECT_EQ(b.victoryHealPercent, 20) << "两位都会也不翻倍";
    EXPECT_EQ(b.ambushBp, 1);
    EXPECT_EQ(b.talismanPowerPercent, 10);
    ASSERT_EQ(b.heroStagger.size(), 1u);
    EXPECT_EQ(b.heroStagger[0].categories, fanren::core::kCategoryFire);
    EXPECT_EQ(b.heroStagger[0].amount, 1);

    state.day += 1;
    EXPECT_FALSE(rules::tongxiuBonusReady(state)) << "同修的劲只在第二天";
}

// ---------------------------------------------------------------------------
// 5. 相处
// ---------------------------------------------------------------------------

TEST(TongdaoTongxiu, AddsFourOncePerDayAndHangsTheBonusOnTomorrow) {
    const auto companions = probes();
    GameState state = ch07State();
    const rules::BondGain gain = rules::tongxiu(companions, "qing", state);
    EXPECT_EQ(gain.check, TongdaoCheck::Ok);
    EXPECT_EQ(gain.gained, rules::kBondTongxiu);
    EXPECT_EQ(state.bondOf("qing"), 4);
    EXPECT_EQ(state.flag(rules::kTongxiuDayFlag), 10);
    EXPECT_EQ(state.flag(rules::kTongxiuBonusDayFlag), 11);
    EXPECT_EQ(rules::tongxiu(companions, "wan", state).check, TongdaoCheck::DoneToday) << "每天一次（不论跟谁）";
    EXPECT_EQ(state.bondOf("wan"), 0);
    state.day = 11;   // 调用方过了一天
    EXPECT_TRUE(rules::tongxiuBonusReady(state));
    EXPECT_EQ(rules::canTongxiu(companions, "wan", state), TongdaoCheck::Ok);
    EXPECT_EQ(rules::canTongxiu(companions, "beast", state), TongdaoCheck::NotPerson);
    EXPECT_EQ(rules::canTongxiu(companions, "nobody", state), TongdaoCheck::NoCompanion);
    state.setFlag("probe.qing", 0);
    EXPECT_EQ(rules::canTongxiu(companions, "qing", state), TongdaoCheck::NotMet);
}

TEST(TongdaoSpar, GatesTierTwoOncePerDayAndAFormation) {
    const auto companions = probes();
    GameState state = ch07State();
    EXPECT_EQ(rules::canSpar(companions, "qing", state), TongdaoCheck::TierLow);
    state.bonds["qing"].value = 20;
    EXPECT_EQ(rules::canSpar(companions, "qing", state), TongdaoCheck::Ok);
    EXPECT_EQ(rules::canSpar(companions, "beast", state), TongdaoCheck::NotPerson);
    Companion noSpar = companions.at("qing");
    noSpar.sparBattle.clear();
    std::map<std::string, Companion> without{{"qing", noSpar}};
    EXPECT_EQ(rules::canSpar(without, "qing", state), TongdaoCheck::NoSpar);

    const rules::SparOutcome lost = rules::recordSpar(companions, "qing", state, false);
    EXPECT_EQ(lost.check, TongdaoCheck::Ok);
    EXPECT_EQ(lost.gained, rules::kBondSparLoss);
    EXPECT_EQ(lost.prizeTier, 0);
    EXPECT_EQ(state.flag(rules::sparDayFlag("qing")), 10);
    EXPECT_EQ(rules::recordSpar(companions, "qing", state, true).check, TongdaoCheck::DoneToday);
    EXPECT_EQ(state.bondOf("qing"), 22) << "今天打过了：第二场一个字节都没写";
}

TEST(TongdaoSpar, TheFirstWinAtEachTierPaysThatTiersPrizeOnce) {
    const auto companions = probes();
    GameState state = ch07State();
    state.bonds["qing"].value = 41;   // 二档，赢一场 +4 就到三档
    rules::SparOutcome won = rules::recordSpar(companions, "qing", state, true);
    EXPECT_EQ(won.gained, rules::kBondSparWin);
    EXPECT_EQ(won.prizeTier, 2) << "按开打时的档算";
    EXPECT_EQ(state.itemCount("pill_a"), 2);
    EXPECT_EQ(state.flag(rules::sparPrizeFlag("qing")), 2);
    EXPECT_EQ(rules::bondTier(state.bondOf("qing")), 3);

    state.day += 1;
    won = rules::recordSpar(companions, "qing", state, true);
    EXPECT_EQ(won.prizeTier, 0) << "三档表里没有奖品：记下三档、什么也不给";
    EXPECT_EQ(state.flag(rules::sparPrizeFlag("qing")), 3);
    state.day += 1;
    state.bonds["qing"].value = 70;
    won = rules::recordSpar(companions, "qing", state, true);
    EXPECT_EQ(won.prizeTier, 4);
    EXPECT_EQ(state.itemCount("pill_b"), 1);
    state.day += 1;
    won = rules::recordSpar(companions, "qing", state, true);
    EXPECT_EQ(won.prizeTier, 0) << "同一档第二次赢不再给";
    EXPECT_EQ(state.itemCount("pill_b"), 1);
    state.day += 1;
    const rules::SparOutcome lost = rules::recordSpar(companions, "qing", state, false);
    EXPECT_EQ(lost.prizeTier, 0) << "输了不给";
}

TEST(TongdaoSpar, AWinAfterAJumpPaysEveryUnclaimedPrizeUpToTheCurrentTier) {
    const auto companions = probes();
    GameState state = ch07State();
    state.bonds["qing"].value = 70;   // 从没赢过就到了四档：二档、四档的奖品都没领
    const rules::SparOutcome won = rules::recordSpar(companions, "qing", state, true);
    EXPECT_EQ(won.prizeTier, 4) << "领到的最高一档";
    EXPECT_EQ(state.itemCount("pill_a"), 2) << "跳过的二档不丢";
    EXPECT_EQ(state.itemCount("pill_b"), 1);
    ASSERT_EQ(won.prize.size(), 2u);
    EXPECT_EQ(won.prize[0].itemId, "pill_a") << "按档从低到高";
    EXPECT_EQ(state.flag(rules::sparPrizeFlag("qing")), 4) << "记到此刻的档";

    GameState lostFirst = ch07State();
    lostFirst.bonds["qing"].value = 70;
    static_cast<void>(rules::recordSpar(companions, "qing", lostFirst, false));
    EXPECT_EQ(lostFirst.itemCount("pill_a"), 0) << "输了一样不发";
    EXPECT_EQ(lostFirst.flag(rules::sparPrizeFlag("qing")), 0) << "输了不记档";
    lostFirst.day += 1;
    const rules::SparOutcome later = rules::recordSpar(companions, "qing", lostFirst, true);
    EXPECT_EQ(later.prizeTier, 4);
    EXPECT_EQ(lostFirst.itemCount("pill_a") + lostFirst.itemCount("pill_b"), 3) << "下一场赢了照样补齐";
}

TEST(TongdaoSpar, TheCapHoldsForSparToo) {
    const auto companions = probes();
    GameState state = ch07State();
    state.bonds["wan"].value = 93;
    const rules::SparOutcome won = rules::recordSpar(companions, "wan", state, true);
    EXPECT_EQ(won.gained, 1);
    EXPECT_EQ(state.bondOf("wan"), 94);
}

TEST(TongdaoQingjiao, TierThreeOnceAndItUnlocksTheRosterBonus) {
    const auto companions = probes();
    GameState state = ch07State();
    state.bonds["qing"].value = 44;
    EXPECT_EQ(rules::recordQingjiao(companions, "qing", state), TongdaoCheck::TierLow);
    EXPECT_EQ(state.flag(rules::qingjiaoFlag("qing")), 0);
    state.bonds["qing"].value = 45;
    EXPECT_EQ(rules::recordQingjiao(companions, "qing", state), TongdaoCheck::Ok);
    EXPECT_TRUE(rules::rosterBonusUnlocked(companions.at("qing"), state));
    EXPECT_EQ(rules::recordQingjiao(companions, "qing", state), TongdaoCheck::AlreadyDone);
    EXPECT_EQ(rules::canQingjiao(companions, "beast", state), TongdaoCheck::NotPerson);
}

TEST(TongdaoXuhua, EachTierFromTwoUpPlaysOnceInOrderEvenAfterAJump) {
    const auto companions = probes();
    GameState state = ch07State();
    EXPECT_EQ(rules::xuhuaPending(companions, "qing", state), 0) << "一档没有事件";
    state.bonds["qing"].value = 70;   // 一下子到了四档
    EXPECT_EQ(rules::xuhuaPending(companions, "qing", state), 2);
    EXPECT_EQ(rules::recordXuhua(companions, "qing", state, 2), TongdaoCheck::Ok);
    EXPECT_EQ(rules::xuhuaPending(companions, "qing", state), 3) << "一次升两档就分两次演";
    EXPECT_EQ(rules::recordXuhua(companions, "qing", state, 5), TongdaoCheck::TierLow) << "还没到五档";
    EXPECT_EQ(rules::recordXuhua(companions, "qing", state, 2), TongdaoCheck::AlreadyDone);
    EXPECT_EQ(rules::recordXuhua(companions, "qing", state, 3), TongdaoCheck::Ok);
    EXPECT_EQ(rules::recordXuhua(companions, "qing", state, 4), TongdaoCheck::Ok);
    EXPECT_EQ(rules::xuhuaPending(companions, "qing", state), 0);
    EXPECT_EQ(state.flag(rules::xuhuaFlag("qing")), 4);
    EXPECT_EQ(rules::xuhuaPending(companions, "beast", state), 0);
}

TEST(TongdaoGift, AnyoneInTheRosterCanReceiveWithoutBeingInTheParty) {
    auto companions = probes();
    companions.at("qing").giftsLike = {"herb_like"};
    GameState state = ch07State();
    ASSERT_TRUE(state.party.empty());
    const rules::GiftResult first = rules::giveGiftTo(companions, "qing", "herb_like", state);
    EXPECT_EQ(first.check, TongdaoCheck::Ok);
    EXPECT_TRUE(first.outcome.given);
    EXPECT_EQ(first.outcome.gained, rules::kBondGiftLiked);
    EXPECT_FALSE(rules::giveGiftTo(companions, "qing", "herb_like", state).outcome.given) << "每人每天一次";
    state.setFlag("probe.wan", 0);
    const rules::GiftResult notMet = rules::giveGiftTo(companions, "wan", "x", state);
    EXPECT_EQ(notMet.check, TongdaoCheck::NotMet);
    EXPECT_EQ(state.bonds.count("wan"), 0u) << "不在册：账一笔都不开";
    EXPECT_EQ(rules::giveGiftTo(companions, "nobody", "x", state).check, TongdaoCheck::NoCompanion);
}

TEST(TongdaoCodes, EveryCheckHasADistinctAsciiCodeAndOkIsEmpty) {
    std::set<std::string> codes;
    for (const TongdaoCheck check : {TongdaoCheck::NoCompanion, TongdaoCheck::NotMet, TongdaoCheck::NotPerson,
                                     TongdaoCheck::TierLow, TongdaoCheck::DoneToday, TongdaoCheck::AlreadyDone,
                                     TongdaoCheck::NoSpar}) {
        const std::string code = rules::tongdaoCheckCode(check);
        EXPECT_FALSE(code.empty());
        EXPECT_TRUE(codes.insert(code).second) << code;
        for (const char ch : code) EXPECT_LT(static_cast<unsigned char>(ch), 128u);
    }
    EXPECT_STREQ(rules::tongdaoCheckCode(TongdaoCheck::Ok), "");
}

TEST(TongdaoFlags, FlagNamesCarryTheCompanionId) {
    EXPECT_EQ(rules::qingjiaoFlag("chen_qiaoqian"), "tongdao.chen_qiaoqian.qingjiao");
    EXPECT_EQ(rules::xuhuaFlag("nangong_wan"), "tongdao.nangong_wan.xuhua");
    EXPECT_EQ(rules::sparDayFlag("x"), "tongdao.x.spar_day");
    EXPECT_EQ(rules::sparPrizeFlag("x"), "tongdao.x.spar_prize");
    EXPECT_EQ(rules::bonusOnFlag("x"), "tongdao.x.bonus_on");
}

}  // namespace
