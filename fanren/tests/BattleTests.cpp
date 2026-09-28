// 横版「破势与蓄劲」的规则单测（docs/octopath-battle.md）。
//
// 判据的写法：**每条规则至少一条断言写死成设计原文的数字**（开场 1 点劲、上限 5、
// 一次最多蓄 3、破势 ×2、防御减半、破势那一回合 + 下一整回合、恢复时架势回满……），
// 不从 Battle.h 的常量推——从被测物推出来的判据，改了口径它跟着改，等于没有判据
//（docs/handoff.md 第 8 节第一条）。常量只在「先验」里出现，用来说明这条断言量的是哪一条。
//
// 夹具：攻 10 / 防 5 / 同境界的两个人，平砍一下正好 10×2−5 = 15 点。下面所有伤害数字
// 都从这一个 15 推出来，写在断言旁边。
#include "core/battle/Battle.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "TempDir.h"
#include "core/battle/Damage.h"
#include "core/model/Types.h"
#include "game/Application.h"
#include "game/BattleScene.h"
#include "io/SaveFile.h"

namespace {

namespace fs = std::filesystem;

using fanren::core::GameState;
using fanren::core::Item;
using fanren::core::Magic;
using fanren::core::MagicBoost;
using fanren::core::battle::Action;
using fanren::core::battle::ActionKind;
using fanren::core::battle::BattleEvent;
using fanren::core::battle::BattleEventKind;
using fanren::core::battle::BattlePhase;
using fanren::core::battle::BattleState;
using fanren::core::battle::Unit;
using fanren::rules::Realm;

namespace ele = fanren::core;
constexpr int kSword = fanren::core::kCategorySword;
constexpr int kFist = fanren::core::kCategoryFist;
constexpr int kFire = fanren::core::kCategoryFire;
constexpr int kPoison = fanren::core::kCategoryPoison;

// ---- 夹具 ----

Unit makeUnit(const std::string& name, bool ally, int speed) {
    Unit u;
    u.id = name;
    u.name = name;
    u.hp = u.maxHp = 100;
    u.mp = u.maxMp = 30;
    u.attack = 10;
    u.defence = 5;
    u.speed = speed;
    u.realm = Realm::QiRefining3;
    u.element = ele::kElementNone;
    u.ally = ally;
    // 我方空手加一口剑；敌人没有兵刃类别（敌方普攻不是破绽的来源）。
    u.weapons = ally ? (kFist | kSword) : 0;
    return u;
}

// 一个有架势的敌人：破绽是剑与火。
Unit makeFoe(const std::string& name, int speed, int toughness) {
    Unit u = makeUnit(name, false, speed);
    u.maxToughness = toughness;
    u.weaknesses = kSword | kFire;
    return u;
}

Magic makeFireball(MagicBoost boost = MagicBoost::Power) {
    Magic m;
    m.id = "huoqiu";
    m.name = "火球术";
    m.element = ele::kElementFire;
    m.needMp = 10;
    m.power = 12;
    m.needRealm = Realm::QiRefining1;
    m.boost = boost;
    return m;
}

BattleState makeBattle(std::vector<Unit> units, std::uint32_t seed = 1) {
    BattleState state;
    state.setup(std::move(units), seed);
    return state;
}

Action attackOn(int actor, int target, int category = 0, int boost = 0) {
    Action a;
    a.kind = ActionKind::Attack;
    a.actorIndex = actor;
    a.targetIndex = target;
    a.category = category;
    a.boost = boost;
    return a;
}

Action castOn(int actor, int target, const std::string& magicId, int boost = 0) {
    Action a;
    a.kind = ActionKind::Cast;
    a.actorIndex = actor;
    a.targetIndex = target;
    a.magicId = magicId;
    a.boost = boost;
    return a;
}

Action simple(ActionKind kind, int actor) {
    Action a;
    a.kind = kind;
    a.actorIndex = actor;
    return a;
}

// 推到下一回合开始（本回合剩下的人什么也不做）。
void nextRound(BattleState& state) {
    const int from = state.round();
    for (int guard = 0; guard < 64 && state.round() == from; ++guard) state.endTurn();
}

// 让到 who 出手为止（中间的人什么也不做）。到不了返回 false。
bool skipTo(BattleState& state, int who) {
    for (int guard = 0; guard < 64; ++guard) {
        if (state.currentActor() == who) return true;
        if (state.phase() != BattlePhase::Ongoing) return false;
        state.endTurn();
    }
    return false;
}

int countEvents(const std::vector<BattleEvent>& events, BattleEventKind kind) {
    return static_cast<int>(std::count_if(events.begin(), events.end(),
                                          [kind](const BattleEvent& e) { return e.kind == kind; }));
}

bool logHas(const BattleState& state, const std::string& needle) {
    for (const std::string& line : state.log()) {
        if (line.find(needle) != std::string::npos) return true;
    }
    return false;
}

// 逐字段比对：非法动作之后这两个快照必须一模一样。
void expectUnitsIdentical(const std::vector<Unit>& before, const std::vector<Unit>& after) {
    ASSERT_EQ(before.size(), after.size());
    for (std::size_t i = 0; i < before.size(); ++i) {
        const Unit& a = before[i];
        const Unit& b = after[i];
        EXPECT_EQ(a.hp, b.hp) << "unit " << i;
        EXPECT_EQ(a.maxHp, b.maxHp) << "unit " << i;
        EXPECT_EQ(a.mp, b.mp) << "unit " << i;
        EXPECT_EQ(a.acted, b.acted) << "unit " << i;
        EXPECT_EQ(a.bp, b.bp) << "unit " << i;
        EXPECT_EQ(a.boosted, b.boosted) << "unit " << i;
        EXPECT_EQ(a.toughness, b.toughness) << "unit " << i;
        EXPECT_EQ(a.revealed, b.revealed) << "unit " << i;
        EXPECT_EQ(a.tested, b.tested) << "unit " << i;
        EXPECT_EQ(a.breakRecoverRound, b.breakRecoverRound) << "unit " << i;
        EXPECT_EQ(a.guarding, b.guarding) << "unit " << i;
        EXPECT_EQ(a.priority, b.priority) << "unit " << i;
        EXPECT_EQ(a.charging, b.charging) << "unit " << i;
        EXPECT_EQ(a.poison, b.poison) << "unit " << i;
        EXPECT_EQ(a.magics, b.magics) << "unit " << i;
    }
}

// ===========================================================================
// 一 · 行动序
// ===========================================================================

TEST(BattleTurnOrder, FastestUnitActsFirst) {
    // 5 / 20 / 12 / 1：相邻两档之比都大于 1.1 / 0.9，±10% 的扰动翻不过来。
    BattleState state = makeBattle({makeUnit("慢", true, 5), makeUnit("快", true, 20),
                                    makeUnit("中", true, 12), makeUnit("敌", false, 1)});
    EXPECT_EQ(state.currentActor(), 1);
    state.endTurn();
    EXPECT_EQ(state.currentActor(), 2);
    state.endTurn();
    EXPECT_EQ(state.currentActor(), 0);
    state.endTurn();
    EXPECT_EQ(state.currentActor(), 3);
}

TEST(BattleTurnOrder, TheJitterIsTenPercentEachWayAndNoMore) {
    // 设计原文：身法乘一个 ±10% 的扰动。100 对 81：最坏 90 对 89.1，永远是 100 先；
    // 100 对 95：最坏 90 对 104.5，总有种子让 95 抢到前头——两条合起来才说明扰动
    // 确实在、而且不超过一成。
    int fastFirstAlways = 0;
    bool swapSeen = false;
    for (std::uint32_t seed = 0; seed < 200; ++seed) {
        BattleState a = makeBattle({makeUnit("甲", true, 100), makeUnit("乙", false, 81)}, seed);
        if (a.currentActor() == 0) ++fastFirstAlways;
        BattleState b = makeBattle({makeUnit("甲", true, 100), makeUnit("乙", false, 95)}, seed);
        if (b.currentActor() == 1) swapSeen = true;
    }
    EXPECT_EQ(fastFirstAlways, 200) << "差两成以上的身法被扰动翻过来了：扰动超过了一成";
    EXPECT_TRUE(swapSeen) << "差一成以内的身法两百个种子里从没翻过：扰动根本没起作用";
}

TEST(BattleTurnOrder, TheSameSeedAlwaysGivesTheSameOrder) {
    for (std::uint32_t seed = 0; seed < 20; ++seed) {
        BattleState a = makeBattle({makeUnit("甲", true, 10), makeUnit("乙", true, 10),
                                    makeUnit("丙", false, 10)}, seed);
        BattleState b = makeBattle({makeUnit("甲", true, 10), makeUnit("乙", true, 10),
                                    makeUnit("丙", false, 10)}, seed);
        EXPECT_EQ(a.roundOrder(), b.roundOrder()) << "种子 " << seed;
    }
}

TEST(BattleTurnOrder, ZeroSpeedTiesFallBackToUnitIndex) {
    // 身法 0 乘什么扰动都是 0：真相等的时候按单位下标，回放不能因为排序不稳而分叉。
    BattleState state = makeBattle({makeUnit("甲", true, 0), makeUnit("乙", true, 0),
                                    makeUnit("丙", false, 0)}, 7);
    const std::vector<int> expected{0, 1, 2};
    EXPECT_EQ(state.roundOrder(), expected);
}

TEST(BattleTurnOrder, RoundEndsWithMinusOneThenAdvances) {
    BattleState state = makeBattle({makeUnit("韩立", true, 12), makeUnit("野狗", false, 8)});
    EXPECT_EQ(state.round(), 1);
    EXPECT_EQ(state.currentActor(), 0);
    state.endTurn();
    EXPECT_EQ(state.currentActor(), 1);
    state.endTurn();
    // 全部行动完毕：先观察到「回合结束」，再调一次才进下一回合。
    EXPECT_EQ(state.currentActor(), -1);
    EXPECT_EQ(state.round(), 1);
    state.endTurn();
    EXPECT_EQ(state.round(), 2);
    EXPECT_EQ(state.currentActor(), 0);
}

TEST(BattleTurnOrder, TheNextRoundPreviewIsExactlyTheOrderThatHappens) {
    // 预览不骗人：每一回合走完时记下预览，下一回合开始后的真实行动序必须一模一样。
    // 同速的五个人，扰动每回合都重掷，所以这不是「每回合一样」碰巧对上。
    BattleState state = makeBattle({makeUnit("甲", true, 10), makeUnit("乙", true, 10),
                                    makeUnit("丙", false, 10), makeUnit("丁", false, 10),
                                    makeUnit("戊", false, 10)}, 20260925u);
    bool orderChanged = false;
    std::vector<int> previous = state.roundOrder();
    for (int round = 0; round < 12; ++round) {
        while (state.currentActor() >= 0) state.endTurn();
        const std::vector<int> preview = state.nextRoundOrder();
        ASSERT_EQ(preview.size(), 5u);
        state.endTurn();   // 开新回合
        EXPECT_EQ(state.roundOrder(), preview) << "第 " << state.round() << " 回合与预览对不上";
        orderChanged = orderChanged || state.roundOrder() != previous;
        previous = state.roundOrder();
    }
    EXPECT_TRUE(orderChanged) << "先验：十二个回合里行动序至少变过一次，否则上面那条量的是常数";
}

TEST(BattleTurnOrder, TheDefenderGoesFirstNextRoundAndThePreviewSaysSoAtOnce) {
    // 设计原文：防御过的单位下一回合排最前。慢的那个（身法 5）一防御，下一回合就排第一；
    // 预览在他防御的那一刻就得变，不能等到下一回合开始才露出来。
    BattleState state = makeBattle({makeUnit("快敌", false, 20), makeUnit("慢我", true, 5)});
    ASSERT_EQ(state.currentActor(), 0);
    state.endTurn();
    ASSERT_EQ(state.currentActor(), 1);
    ASSERT_EQ(state.nextRoundOrder().front(), 0) << "先验：不防御的话下一回合还是快的先";

    ASSERT_TRUE(state.apply(simple(ActionKind::Defend, 1)).ok);
    const std::vector<int> preview = state.nextRoundOrder();
    ASSERT_FALSE(preview.empty());
    EXPECT_EQ(preview.front(), 1) << "防御之后预览上他就该排第一";
    nextRound(state);
    EXPECT_EQ(state.roundOrder(), preview);
    EXPECT_EQ(state.currentActor(), 1);
    nextRound(state);
    EXPECT_EQ(state.currentActor(), 0) << "先手只管下一回合，再下一回合又按身法排";
}

// ===========================================================================
// 二 · 劲
// ===========================================================================

TEST(BattleBp, AlliesStartWithOnePointAndEnemiesWithNone) {
    BattleState state = makeBattle({makeUnit("韩立", true, 12), makeUnit("野狗", false, 8)});
    EXPECT_EQ(state.units()[0].bp, 1) << "设计原文：我方每人开场 1 点劲";
    EXPECT_EQ(state.units()[1].bp, 0) << "敌方没有劲";
}

TEST(BattleBp, EachRoundAddsOneUpToFive) {
    BattleState state = makeBattle({makeUnit("韩立", true, 12), makeUnit("野狗", false, 8)});
    const std::vector<int> expected{1, 2, 3, 4, 5, 5, 5};
    for (std::size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(state.units()[0].bp, expected[i]) << "第 " << state.round() << " 回合";
        nextRound(state);
    }
}

TEST(BattleBp, BoostingSkipsNextRoundsGain) {
    // 设计原文：上一回合蓄过劲的人这一回合开始不加。
    BattleState state = makeBattle({makeUnit("韩立", true, 12), makeUnit("野狗", false, 8)});
    nextRound(state);
    nextRound(state);
    ASSERT_EQ(state.units()[0].bp, 3);
    ASSERT_TRUE(state.apply(attackOn(0, 1, kFist, 2)).ok);
    EXPECT_EQ(state.units()[0].bp, 1) << "蓄 2 点就少 2 点";
    nextRound(state);
    EXPECT_EQ(state.units()[0].bp, 1) << "蓄过劲的下一回合开始不加";
    nextRound(state);
    EXPECT_EQ(state.units()[0].bp, 2) << "再下一回合照常加";
}

TEST(BattleBp, AtMostThreeAndNeverMoreThanYouHave) {
    BattleState state = makeBattle({makeUnit("韩立", true, 12), makeUnit("野狗", false, 8)});
    for (int i = 0; i < 4; ++i) nextRound(state);
    ASSERT_EQ(state.units()[0].bp, 5);
    std::string why;
    EXPECT_FALSE(state.isLegal(attackOn(0, 1, kFist, 4), &why)) << "设计原文：一次最多蓄 3 点";
    EXPECT_NE(why.find("3"), std::string::npos) << why;
    EXPECT_TRUE(state.isLegal(attackOn(0, 1, kFist, 3)));

    BattleState poor = makeBattle({makeUnit("韩立", true, 12), makeUnit("野狗", false, 8)});
    ASSERT_EQ(poor.units()[0].bp, 1);
    EXPECT_FALSE(poor.isLegal(attackOn(0, 1, kFist, 2), &why)) << "只有 1 点劲却要蓄 2 点";
    EXPECT_NE(why.find("劲不够"), std::string::npos) << why;
}

TEST(BattleBp, ItemsDefendAndEscapeCannotBeBoosted) {
    BattleState state = makeBattle({makeUnit("韩立", true, 12), makeUnit("野狗", false, 8)});
    Item pill;
    pill.id = "huiqi";
    pill.name = "回气丹";
    pill.restoreHp = 30;
    state.addItem(pill);
    Action use = simple(ActionKind::Item, 0);
    use.magicId = "huiqi";
    use.targetIndex = 0;
    use.boost = 1;
    std::string why;
    EXPECT_FALSE(state.isLegal(use, &why));
    EXPECT_NE(why.find("蓄"), std::string::npos) << why;
    Action defend = simple(ActionKind::Defend, 0);
    defend.boost = 1;
    EXPECT_FALSE(state.isLegal(defend));
    Action run = simple(ActionKind::Escape, 0);
    run.boost = 1;
    EXPECT_FALSE(state.isLegal(run));
}

// ===========================================================================
// 三 · 连击与法术的两种蓄劲
// ===========================================================================

TEST(BattleBoost, AnAttackBoostedByNHitsOnePlusNTimes) {
    BattleState state = makeBattle({makeUnit("韩立", true, 12), makeUnit("野狗", false, 8)});
    nextRound(state);
    nextRound(state);
    ASSERT_TRUE(state.apply(attackOn(0, 1, kFist, 2)).ok);
    EXPECT_EQ(countEvents(state.lastActionEvents(), BattleEventKind::Hit), 3) << "蓄 2 点 = 连击 3 下";
    EXPECT_EQ(state.units()[1].hp, 100 - 3 * 15) << "每一下都是 15 点";
}

TEST(BattleBoost, AHitsSpellFiresOnePlusNBolts) {
    BattleState state = makeBattle({makeUnit("韩立", true, 12), makeUnit("野狗", false, 8)});
    state.addMagic(makeFireball(MagicBoost::Hits));
    nextRound(state);
    ASSERT_TRUE(state.apply(castOn(0, 1, "huoqiu", 1)).ok);
    // 12×2 − 5×0.5 = 21.5 → 22（四舍五入离零）；连发 2 发。
    EXPECT_EQ(countEvents(state.lastActionEvents(), BattleEventKind::Hit), 2);
    EXPECT_EQ(state.units()[1].hp, 100 - 2 * 22);
    EXPECT_EQ(state.units()[0].mp, 30 - 10) << "法力按一次施法扣，不按发数";
}

TEST(BattleBoost, APowerSpellMultipliesItsPowerInOneHit) {
    BattleState state = makeBattle({makeUnit("韩立", true, 12), makeUnit("野狗", false, 8)});
    state.addMagic(makeFireball(MagicBoost::Power));
    nextRound(state);
    ASSERT_TRUE(state.apply(castOn(0, 1, "huoqiu", 1)).ok);
    // 威力 ×(1+1) = 24：24×2 − 2.5 = 45.5 → 46，仍是一击。
    EXPECT_EQ(countEvents(state.lastActionEvents(), BattleEventKind::Hit), 1);
    EXPECT_EQ(state.units()[1].hp, 100 - 46);
}

// ===========================================================================
// 四 · 破绽、揭开、架势、破势
// ===========================================================================

TEST(BattleBreak, AWeaknessHitTakesOnePointAndRevealsTheSlot) {
    BattleState state = makeBattle({makeUnit("韩立", true, 12), makeFoe("恶狼", 8, 3)});
    ASSERT_EQ(state.units()[1].toughness, 3) << "开场架势是满的";
    ASSERT_EQ(state.units()[1].revealed, 0);
    const auto result = state.apply(attackOn(0, 1, kSword));
    ASSERT_TRUE(result.ok) << result.error;
    EXPECT_EQ(state.units()[1].toughness, 2) << "设计原文：每一击命中破绽，架势 −1";
    EXPECT_EQ(state.units()[1].revealed, kSword) << "打中即揭开";
    const std::vector<BattleEvent> events = state.lastActionEvents();
    const auto hit = std::find_if(events.begin(), events.end(),
                                  [](const BattleEvent& e) { return e.kind == BattleEventKind::Hit; });
    ASSERT_NE(hit, events.end());
    EXPECT_TRUE(hit->weakness);
    EXPECT_EQ(hit->revealed, kSword);
    EXPECT_EQ(hit->toughness, 2);
    EXPECT_NE(result.value.find("破绽"), std::string::npos) << result.value;
}

TEST(BattleBreak, ANonWeaknessHitNeitherCutsNorReveals) {
    BattleState state = makeBattle({makeUnit("韩立", true, 12), makeFoe("恶狼", 8, 3)});
    ASSERT_TRUE(state.apply(attackOn(0, 1, kFist)).ok);
    EXPECT_EQ(state.units()[1].toughness, 3);
    EXPECT_EQ(state.units()[1].revealed, 0) << "拳不是它的破绽，一格也不该亮";
    EXPECT_EQ(state.units()[1].tested & kFist, kFist) << "但试过了要记着（给我方 AI 探破绽用）";
}

TEST(BattleBreak, EveryHitOfAComboIsJudgedOnItsOwn) {
    BattleState state = makeBattle({makeUnit("韩立", true, 12), makeFoe("恶狼", 8, 3)});
    nextRound(state);
    nextRound(state);
    ASSERT_EQ(state.units()[0].bp, 3);
    ASSERT_TRUE(state.apply(attackOn(0, 1, kSword, 2)).ok);
    EXPECT_EQ(state.units()[1].toughness, 0) << "三击三次判破绽：3 − 3 = 0";
    EXPECT_TRUE(state.units()[1].broken());
}

TEST(BattleBreak, TheBreakingHitIsNotDoubledButTheOnesAfterItAre) {
    // 架势 1：连击两下，第一下 15 点破势，第二下吃 ×2 → 30。合计 45。
    BattleState state = makeBattle({makeUnit("韩立", true, 12), makeFoe("恶狼", 8, 1)});
    nextRound(state);
    ASSERT_TRUE(state.apply(attackOn(0, 1, kSword, 1)).ok);
    EXPECT_EQ(state.units()[1].hp, 100 - 15 - 30);
}

TEST(BattleBreak, ABrokenFoeLosesTheRestOfThisRoundAndAllOfTheNext) {
    // 我方快（12）、敌方慢（8）：第 1 回合我方先把它打到破势，它这一回合还没出手，就此失去；
    // 第 2 回合整回合没有它；第 3 回合开始时恢复、照常出手。
    BattleState state = makeBattle({makeUnit("韩立", true, 12), makeFoe("恶狼", 8, 1)});
    ASSERT_TRUE(state.apply(attackOn(0, 1, kSword)).ok);
    ASSERT_TRUE(state.units()[1].broken());
    state.endTurn();
    EXPECT_EQ(state.currentActor(), -1) << "第 1 回合：它还没出手就破势了，这一位跳过";

    state.endTurn();
    ASSERT_EQ(state.round(), 2);
    EXPECT_EQ(std::count(state.roundOrder().begin(), state.roundOrder().end(), 1), 0)
        << "第 2 回合：整回合没有它的位置";
    EXPECT_TRUE(state.units()[1].broken());

    nextRound(state);
    ASSERT_EQ(state.round(), 3);
    EXPECT_FALSE(state.units()[1].broken()) << "第 3 回合开始时恢复";
    EXPECT_EQ(state.units()[1].toughness, 1) << "设计原文：恢复时架势回满";
    EXPECT_EQ(std::count(state.roundOrder().begin(), state.roundOrder().end(), 1), 1);
    EXPECT_TRUE(logHas(state, "重整架势"));
}

TEST(BattleBreak, ABrokenFoeTakesDoubleDamageFromAnything) {
    BattleState state = makeBattle({makeUnit("韩立", true, 12), makeFoe("恶狼", 8, 1)});
    ASSERT_TRUE(state.apply(attackOn(0, 1, kSword)).ok);   // 15，破势
    nextRound(state);
    const int before = state.units()[1].hp;
    ASSERT_TRUE(state.apply(attackOn(0, 1, kFist)).ok);    // 拳不是破绽，也吃 ×2
    EXPECT_EQ(before - state.units()[1].hp, 30) << "设计原文：破势期间伤害 ×2（15 → 30）";
}

TEST(BattleBreak, BreakingItAfterItHasActedStillCostsItTheWholeNextRound) {
    // 敌方快（12）、我方慢（8）：它第 1 回合先出了手，我方再把它打到破势——
    // 本回合没什么可失去的了，失去的是第 2 回合；第 3 回合开始恢复。
    BattleState state = makeBattle({makeFoe("恶狼", 12, 1), makeUnit("韩立", true, 8)});
    ASSERT_EQ(state.currentActor(), 0);
    state.endTurn();
    ASSERT_TRUE(state.apply(attackOn(1, 0, kSword)).ok);
    ASSERT_TRUE(state.units()[0].broken());
    nextRound(state);
    EXPECT_EQ(std::count(state.roundOrder().begin(), state.roundOrder().end(), 0), 0);
    nextRound(state);
    EXPECT_EQ(std::count(state.roundOrder().begin(), state.roundOrder().end(), 0), 1);
}

TEST(BattleBreak, RevealingOneWolfRevealsEveryWolf) {
    // 知道的是「恶狼怕什么」，不是「这一条恶狼怕什么」——存档里也是按 role_id 记的。
    BattleState state = makeBattle({makeUnit("韩立", true, 12), makeFoe("恶狼", 8, 3),
                                    makeFoe("恶狼", 7, 3)});
    ASSERT_EQ(state.units()[1].id, state.units()[2].id);
    ASSERT_TRUE(state.apply(attackOn(0, 1, kSword)).ok);
    EXPECT_EQ(state.units()[2].revealed, kSword);
    EXPECT_EQ(state.units()[2].toughness, 3) << "揭开的是破绽，削的只是挨打的那一条";
}

TEST(BattleBreak, ADockhandOnOurSideTellsNothingAboutTheOnesAgainstUs) {
    // 码头打手敌友两用（夺帮那一夜）。我方那一位没有破绽（game 层不给我方架势与破绽），
    // 打在他身上什么也试不出来——不许顺着 id 把「拳试过了」记到对面那几个头上，
    // 否则我方 AI 以为「拳」在对面身上试过、又没揭开，就再也不拿拳去探了。
    Unit ours = makeUnit("码头打手", true, 6);
    Unit theirs = makeFoe("码头打手", 12, 2);
    theirs.weaknesses = kFist;
    theirs.weapons = kFist;
    BattleState state = makeBattle({ours, theirs});
    ASSERT_EQ(state.units()[0].id, state.units()[1].id) << "先验：两边是同一个 role id";
    ASSERT_EQ(state.currentActor(), 1) << "先验：对面那一位先手";
    ASSERT_TRUE(state.apply(attackOn(1, 0, kFist)).ok);
    EXPECT_EQ(state.units()[1].tested, 0) << "打在我方身上的那一拳，不算在对面身上试过";
    EXPECT_EQ(state.units()[1].revealed, 0);
}

TEST(BattleBreak, ThePreviewLeavesTheBrokenFoeOutAtOnce) {
    BattleState state = makeBattle({makeUnit("韩立", true, 12), makeFoe("恶狼", 8, 1)});
    // 先接住再数：nextRoundOrder() 每次返回一个新的 vector，两次调用的 begin/end 不是同一个。
    const std::vector<int> before = state.nextRoundOrder();
    ASSERT_EQ(std::count(before.begin(), before.end(), 1), 1);
    ASSERT_TRUE(state.apply(attackOn(0, 1, kSword)).ok);
    const std::vector<int> preview = state.nextRoundOrder();
    EXPECT_EQ(std::count(preview.begin(), preview.end(), 1), 0) << "破势的那一刻预览就该把它拿掉";
}

TEST(BattleBreak, APoisonItemIsAPoisonHit) {
    // 设计原文：毒粉类物品算「毒」类一击。
    Unit foe = makeFoe("僵兽", 3, 2);
    foe.weaknesses = kPoison | kFire;
    BattleState state = makeBattle({makeUnit("韩立", true, 12), foe});
    Item powder;
    powder.id = "shixin";
    powder.name = "蚀心散";
    powder.poison = 3;
    powder.poisonPower = 6;
    state.addItem(powder);
    Action use = simple(ActionKind::Item, 0);
    use.magicId = "shixin";
    use.targetIndex = 1;
    ASSERT_TRUE(state.apply(use).ok);
    EXPECT_EQ(state.units()[1].toughness, 1) << "撒中破绽：架势 2 → 1";
    EXPECT_EQ(state.units()[1].revealed, kPoison);
    EXPECT_EQ(state.units()[1].poison, 3) << "毒照常挂上";
    EXPECT_EQ(state.units()[1].hp, 100) << "撒毒这一下本身不带伤害";
}

TEST(BattleBreak, AUnitWithoutToughnessNeverBreaks) {
    // 没有架势的（我方就是）：哪怕这一击的类别正落在它的「破绽」上，也削不出破势来。
    Unit hero = makeUnit("韩立", true, 8);
    hero.weaknesses = kSword;
    Unit dog = makeUnit("野狗", false, 12);
    dog.weapons = kSword;
    BattleState state = makeBattle({hero, dog});
    ASSERT_EQ(state.currentActor(), 1);
    ASSERT_TRUE(state.apply(attackOn(1, 0, kSword)).ok);
    EXPECT_FALSE(state.units()[0].broken());
    EXPECT_EQ(state.units()[0].toughness, 0);
}

// ---- 只有这几类要得了他的命（Unit::killableBy，欧阳飞天的霸王甲）----
// 第 5 章校对 MEDIUM-1 的拍板：火弹最多把他逼得狼狈，取首级只有祭剑符（金）那一下。
TEST(BattleKillableBy, AnyOtherCategoryStopsAtOneHitPoint) {
    Unit hero = makeUnit("韩立", true, 12);
    hero.attack = 60;   // 一刀就远超他的气血：要看的是「压到 1 就停」，不是「砍不动」
    Unit armoured = makeUnit("欧阳飞天", false, 8);
    armoured.killableBy = fanren::core::kCategoryMetal;
    BattleState state = makeBattle({hero, armoured});
    nextRound(state);
    ASSERT_TRUE(state.apply(attackOn(0, 1, kSword, 1)).ok);
    EXPECT_EQ(state.units()[1].hp, 1) << "剑砍不死他：两刀下去该停在一口气上";
    EXPECT_TRUE(state.units()[1].alive());
    EXPECT_EQ(state.phase(), BattlePhase::Ongoing);
    EXPECT_NE(state.log().back().find("伤不到"), std::string::npos) << state.log().back();
}

TEST(BattleKillableBy, TheOneCategoryThatIsListedStillKills) {
    Unit hero = makeUnit("韩立", true, 12);
    Unit armoured = makeUnit("欧阳飞天", false, 8);
    armoured.hp = 20;
    armoured.killableBy = fanren::core::kCategoryMetal;
    BattleState state = makeBattle({hero, armoured});
    Magic talisman = makeFireball();
    talisman.id = "jianfu";
    talisman.element = ele::kElementMetal;
    state.addMagic(talisman);
    nextRound(state);
    ASSERT_TRUE(state.apply(castOn(0, 1, "jianfu")).ok);
    EXPECT_FALSE(state.units()[1].alive()) << "金要得了他的命";
    EXPECT_EQ(state.phase(), BattlePhase::Won);
}

TEST(BattleKillableBy, UnsetMeansAnythingKills) {
    Unit hero = makeUnit("韩立", true, 12);
    hero.attack = 60;
    BattleState state = makeBattle({hero, makeUnit("野狗", false, 8)});
    nextRound(state);
    ASSERT_TRUE(state.apply(attackOn(0, 1, kSword)).ok);
    EXPECT_FALSE(state.units()[1].alive()) << "killableBy 为 0 的敌人，什么都要得了命";
}

// ===========================================================================
// 五 · 防御
// ===========================================================================

TEST(BattleGuard, DefendingHalvesDamageUntilTheDefenderActsAgain) {
    // 快敌（20）先打一下 15；我（10）防御；慢敌（5）再打一下——减半：7.5 → 8。
    BattleState state = makeBattle({makeUnit("快敌", false, 20), makeUnit("韩立", true, 10),
                                    makeUnit("慢敌", false, 5)});
    ASSERT_EQ(state.currentActor(), 0);
    ASSERT_TRUE(state.apply(attackOn(0, 1)).ok);
    EXPECT_EQ(state.units()[1].hp, 85);
    state.endTurn();
    ASSERT_TRUE(state.apply(simple(ActionKind::Defend, 1)).ok);
    EXPECT_TRUE(state.units()[1].guarding);
    state.endTurn();
    ASSERT_EQ(state.currentActor(), 2);
    ASSERT_TRUE(state.apply(attackOn(2, 1)).ok);
    EXPECT_EQ(state.units()[1].hp, 85 - 8) << "设计原文：防御受伤减半（15 × 0.5 = 7.5 → 8）";

    // 下一回合他排第一；轮到他那一刻守势散去（日志与事件都要有）。
    nextRound(state);
    EXPECT_EQ(state.currentActor(), 1);
    EXPECT_FALSE(state.units()[1].guarding);
    EXPECT_TRUE(logHas(state, "收起守势"));
    EXPECT_GE(countEvents(state.events(), BattleEventKind::GuardEnd), 1);
}

TEST(BattleGuard, DefendingForeverIsNeverMoreThanHalf) {
    // 旧的护体罡气能攒成无限（第 3 章实测攒到 190 点）。减半是一个比例，攒不起来：
    // 一直防御的人每一下都还是挨一半。
    BattleState state = makeBattle({makeUnit("韩立", true, 10), makeUnit("野狗", false, 5)});
    for (int round = 0; round < 6; ++round) {
        ASSERT_EQ(state.currentActor(), 0);
        ASSERT_TRUE(state.apply(simple(ActionKind::Defend, 0)).ok);
        state.endTurn();
        ASSERT_EQ(state.currentActor(), 1);
        const int before = state.units()[0].hp;
        ASSERT_TRUE(state.apply(attackOn(1, 0)).ok);
        EXPECT_EQ(before - state.units()[0].hp, 8) << "第 " << round + 1 << " 回合";
        nextRound(state);
    }
}

// ===========================================================================
// 六 · 首领：多次行动、蓄势与打断
// ===========================================================================

TEST(BattleBoss, TwoActionsMeanTwoSlotsSpreadAcrossTheRound) {
    // 首领身法 10、两次行动：第一次按全速 10、第二次按半速 5 排。我方身法 7 夹在中间。
    Unit boss = makeFoe("首领", 10, 8);
    boss.actions = 2;
    BattleState state = makeBattle({boss, makeUnit("韩立", true, 7)});
    const std::vector<int> expected{0, 1, 0};
    EXPECT_EQ(state.roundOrder(), expected);

    ASSERT_TRUE(state.apply(attackOn(0, 1)).ok);
    EXPECT_FALSE(state.isLegal(attackOn(0, 1))) << "同一位上不能再出一手";
    state.endTurn();
    ASSERT_EQ(state.currentActor(), 1);
    state.endTurn();
    ASSERT_EQ(state.currentActor(), 0);
    EXPECT_TRUE(state.isLegal(attackOn(0, 1))) << "第二位上它照样出得了手";
    ASSERT_TRUE(state.apply(attackOn(0, 1)).ok);
    EXPECT_EQ(state.units()[1].hp, 100 - 2 * 15);
}

TEST(BattleBoss, BreakingABossSkipsItsRemainingSlotThisRound) {
    Unit boss = makeFoe("首领", 10, 1);
    boss.actions = 2;
    BattleState state = makeBattle({boss, makeUnit("韩立", true, 7)});
    ASSERT_TRUE(state.apply(attackOn(0, 1)).ok);
    state.endTurn();
    ASSERT_EQ(state.currentActor(), 1);
    ASSERT_TRUE(state.apply(attackOn(1, 0, kSword)).ok);
    ASSERT_TRUE(state.units()[0].broken());
    state.endTurn();
    EXPECT_EQ(state.currentActor(), -1) << "它排在后面的第二位随破势一起没了";
}

Unit makeChargingBoss(bool all) {
    Unit boss = makeFoe("首领", 10, 1);
    boss.chargeEvery = 3;
    boss.chargeMult = 2.5;
    boss.chargeAll = all;
    boss.chargeTextKey = "probe.charge";
    return boss;
}

// 让首领（0 号）按 AI 走完它的一位，我方其余人什么也不做。
void bossActs(BattleState& state) {
    ASSERT_TRUE(skipTo(state, 0));
    const Action action = state.decideAi(0);
    ASSERT_GE(action.actorIndex, 0);
    ASSERT_TRUE(state.apply(action).ok);
    state.endTurn();
}

TEST(BattleBoss, TheBossDeclaresOnItsThirdActingRoundAndStrikesHardOnTheNext) {
    Unit ally = makeUnit("韩立", true, 5);
    ally.hp = ally.maxHp = 1000;
    BattleState state = makeBattle({makeChargingBoss(false), ally});
    for (int round = 1; round <= 2; ++round) {
        ASSERT_TRUE(skipTo(state, 0));
        EXPECT_EQ(state.decideAi(0).kind, ActionKind::Attack) << "第 " << round << " 回合还是普攻";
        bossActs(state);
        nextRound(state);
    }
    ASSERT_TRUE(skipTo(state, 0));
    ASSERT_EQ(state.decideAi(0).kind, ActionKind::Charge) << "设计原文：每隔几回合宣告蓄势";
    bossActs(state);
    EXPECT_TRUE(state.units()[0].charging);
    EXPECT_TRUE(logHas(state, "蓄势"));
    const auto declare = std::find_if(state.events().begin(), state.events().end(), [](const BattleEvent& e) {
        return e.kind == BattleEventKind::ChargeDeclare;
    });
    ASSERT_NE(declare, state.events().end()) << "宣告要进事件流";
    EXPECT_EQ(declare->text, "probe.charge") << "事件带着那一句预告的文案 key";

    nextRound(state);
    const int before = state.units()[1].hp;
    bossActs(state);
    EXPECT_EQ(before - state.units()[1].hp, 38) << "重招 ×2.5：15 × 2.5 = 37.5 → 38";
    EXPECT_FALSE(state.units()[0].charging) << "重招出过就散";
}

TEST(BattleBoss, ItCannotDeclareOffItsRhythm) {
    // 节奏是规则（checkCharge），不只是 AI 的打法：第一个出手回合就宣告，要被回绝，
    // 而且不留任何痕迹。
    Unit ally = makeUnit("韩立", true, 5);
    BattleState state = makeBattle({makeChargingBoss(false), ally});
    ASSERT_TRUE(skipTo(state, 0));
    std::string why;
    EXPECT_FALSE(state.isLegal(simple(ActionKind::Charge, 0), &why));
    EXPECT_NE(why.find("还没到蓄势的时候"), std::string::npos) << why;
    EXPECT_FALSE(state.apply(simple(ActionKind::Charge, 0)).ok);
    EXPECT_FALSE(state.units()[0].charging);
}

TEST(BattleBoss, AnAllOutHeavyStrikeHitsEveryAlly) {
    Unit a = makeUnit("韩立", true, 5);
    Unit b = makeUnit("曲魂", true, 4);
    a.hp = a.maxHp = b.hp = b.maxHp = 1000;
    BattleState state = makeBattle({makeChargingBoss(true), a, b});
    for (int round = 0; round < 3; ++round) {
        bossActs(state);
        nextRound(state);
    }
    ASSERT_TRUE(state.units()[0].charging);
    const int hpA = state.units()[1].hp;
    const int hpB = state.units()[2].hp;
    bossActs(state);
    EXPECT_EQ(hpA - state.units()[1].hp, 38);
    EXPECT_EQ(hpB - state.units()[2].hp, 38) << "all 为真：我方全体都挨这一下";
}

TEST(BattleBoss, BreakingTheBossBeforeItStrikesCancelsTheHeavyStrike) {
    Unit ally = makeUnit("韩立", true, 5);
    ally.hp = ally.maxHp = 1000;
    BattleState state = makeBattle({makeChargingBoss(false), ally});
    for (int round = 0; round < 3; ++round) {
        bossActs(state);
        if (round < 2) nextRound(state);
    }
    ASSERT_TRUE(state.units()[0].charging) << "先验：第 3 回合宣告了";
    ASSERT_TRUE(skipTo(state, 1));
    const auto result = state.apply(attackOn(1, 0, kSword));   // 架势 1，一下破势
    ASSERT_TRUE(result.ok) << result.error;
    EXPECT_TRUE(state.units()[0].broken());
    EXPECT_FALSE(state.units()[0].charging) << "设计原文：出重招之前破势即打断";
    EXPECT_NE(result.value.find("打断"), std::string::npos) << result.value;
    EXPECT_EQ(countEvents(state.lastActionEvents(), BattleEventKind::ChargeInterrupt), 1);

    // 它失去第 4 回合；第 5 回合恢复后那一手是普通的一击，不是重招。
    nextRound(state);
    EXPECT_EQ(std::count(state.roundOrder().begin(), state.roundOrder().end(), 0), 0);
    nextRound(state);
    const int before = state.units()[1].hp;
    bossActs(state);
    EXPECT_EQ(before - state.units()[1].hp, 15) << "打断了的重招作废";
}

// ===========================================================================
// 七 · 伤害公式沿用（Damage.cpp）
// ===========================================================================

TEST(BattleDamage, ElementRestraintIsTwoWay) {
    using fanren::core::battle::elementFactor;
    EXPECT_DOUBLE_EQ(elementFactor(ele::kElementMetal, ele::kElementWood), 1.3);
    EXPECT_DOUBLE_EQ(elementFactor(ele::kElementWood, ele::kElementMetal), 0.8);
    EXPECT_DOUBLE_EQ(elementFactor(ele::kElementWood, ele::kElementEarth), 1.3);
    EXPECT_DOUBLE_EQ(elementFactor(ele::kElementEarth, ele::kElementWater), 1.3);
    EXPECT_DOUBLE_EQ(elementFactor(ele::kElementWater, ele::kElementFire), 1.3);
    EXPECT_DOUBLE_EQ(elementFactor(ele::kElementFire, ele::kElementMetal), 1.3);
    EXPECT_DOUBLE_EQ(elementFactor(ele::kElementFire, ele::kElementWater), 0.8);
}

TEST(BattleDamage, ElementNeutralWhenUnrelatedOrMutual) {
    using fanren::core::battle::elementFactor;
    EXPECT_DOUBLE_EQ(elementFactor(ele::kElementNone, ele::kElementWood), 1.0);
    EXPECT_DOUBLE_EQ(elementFactor(ele::kElementMetal, ele::kElementMetal), 1.0);
    EXPECT_DOUBLE_EQ(elementFactor(ele::kElementMetal | ele::kElementEarth,
                                   ele::kElementWood | ele::kElementWater),
                     1.0);
}

TEST(BattleDamage, RealmSuppressionStillApplies) {
    Unit hero = makeUnit("韩立", true, 12);
    hero.realm = Realm::FoundationEarly;
    BattleState state = makeBattle({hero, makeUnit("野狗", false, 8)});
    const double factor = fanren::rules::suppressionFactor(Realm::FoundationEarly, Realm::QiRefining3);
    ASSERT_GT(factor, 1.0);
    ASSERT_TRUE(state.apply(attackOn(0, 1)).ok);
    EXPECT_EQ(state.units()[1].hp, 100 - static_cast<int>(std::lround(15 * factor)));
}

TEST(BattleDamage, NeverDropsBelowOne) {
    using fanren::core::battle::physicalDamage;
    EXPECT_EQ(physicalDamage(1, 999, Realm::QiRefining1, Realm::CoreLate, ele::kElementWood,
                             ele::kElementMetal),
              1);
}

// ===========================================================================
// 八 · 法术与物品
// ===========================================================================

TEST(BattleCast, ConsumesMpAndUsesMagicElement) {
    std::vector<Unit> units{makeUnit("韩立", true, 12), makeUnit("野狗", false, 8)};
    units[1].element = ele::kElementMetal;   // 火克金
    BattleState state = makeBattle(units);
    state.addMagic(makeFireball());
    const auto result = state.apply(castOn(0, 1, "huoqiu"));
    ASSERT_TRUE(result.ok) << result.error;
    EXPECT_EQ(state.units()[0].mp, 20);
    // 12×2 − 5×0.5 = 21.5，火克金 ×1.3 = 27.95 → 28
    EXPECT_EQ(state.units()[1].hp, 100 - 28);
    EXPECT_NE(result.value.find("火球术"), std::string::npos);
}

TEST(BattleCast, AFireSpellIsAFireHit) {
    // 法术的类别 = 它的五行。
    BattleState state = makeBattle({makeUnit("韩立", true, 12), makeFoe("恶狼", 8, 3)});
    state.addMagic(makeFireball());
    ASSERT_TRUE(state.apply(castOn(0, 1, "huoqiu")).ok);
    EXPECT_EQ(state.units()[1].revealed, kFire);
    EXPECT_EQ(state.units()[1].toughness, 2);
}

TEST(BattleCast, NotEnoughMpIsIllegalAndCostsNothing) {
    std::vector<Unit> units{makeUnit("韩立", true, 12), makeUnit("野狗", false, 8)};
    units[0].mp = 3;
    BattleState state = makeBattle(units);
    state.addMagic(makeFireball());
    const auto result = state.apply(castOn(0, 1, "huoqiu"));
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(state.units()[0].mp, 3);
    EXPECT_EQ(state.units()[1].hp, 100);
    EXPECT_FALSE(state.units()[0].acted);
}

TEST(BattleCast, AMagicThatHurtsNobodyIsRefusedWithThatReason) {
    // 御风决、护身罡这类 power 0 的法术：从前靠「超出施法距离」挡住，现在挡它的是实话。
    BattleState state = makeBattle({makeUnit("韩立", true, 12), makeUnit("野狗", false, 8)});
    Magic wind;
    wind.id = "yufeng";
    wind.name = "御风决";
    wind.power = 0;
    wind.needMp = 6;
    state.addMagic(wind);
    std::string why;
    EXPECT_FALSE(state.isLegal(castOn(0, 1, "yufeng"), &why));
    EXPECT_NE(why.find("不是伤人的法术"), std::string::npos) << why;
}

TEST(BattleCast, UnlearnedMagicAndRealmAreStillChecked) {
    std::vector<Unit> units{makeUnit("韩立", true, 12), makeUnit("野狗", false, 8)};
    units[0].magics = {"qingyuan"};
    BattleState state = makeBattle(units);
    state.addMagic(makeFireball());
    EXPECT_FALSE(state.isLegal(castOn(0, 1, "huoqiu")));

    std::vector<Unit> mortal{makeUnit("韩立", true, 12), makeUnit("野狗", false, 8)};
    mortal[0].realm = Realm::Mortal;
    BattleState low = makeBattle(mortal);
    low.addMagic(makeFireball());
    EXPECT_FALSE(low.isLegal(castOn(0, 1, "huoqiu")));
}

TEST(BattleItem, HealsFriendlyTargetUpToMax) {
    std::vector<Unit> units{makeUnit("韩立", true, 12), makeUnit("野狗", false, 8)};
    units[0].hp = 90;
    BattleState state = makeBattle(units);
    Item pill;
    pill.id = "huiqi";
    pill.name = "回气丹";
    pill.restoreHp = 30;
    state.addItem(pill);
    Action use = simple(ActionKind::Item, 0);
    use.magicId = "huiqi";
    use.targetIndex = 0;
    ASSERT_TRUE(state.apply(use).ok);
    EXPECT_EQ(state.units()[0].hp, 100);
    EXPECT_TRUE(state.units()[0].acted);
}

TEST(BattleAttack, AWeaponTheActorDoesNotHoldIsRefused) {
    BattleState state = makeBattle({makeUnit("韩立", true, 12), makeFoe("恶狼", 8, 3)});
    std::string why;
    EXPECT_FALSE(state.isLegal(attackOn(0, 1, fanren::core::kCategoryBlade), &why)) << "他手上没有刀";
    EXPECT_NE(why.find("刀"), std::string::npos) << why;
    EXPECT_FALSE(state.isLegal(attackOn(0, 1, kSword | kFist))) << "一手只出一样兵刃";
    EXPECT_TRUE(state.isLegal(attackOn(0, 1, 0))) << "0 = 第一样兵刃";
}

TEST(BattleAttack, CannotTargetOwnSide) {
    BattleState state = makeBattle({makeUnit("韩立", true, 12), makeUnit("张铁", true, 9),
                                    makeUnit("野狗", false, 8)});
    EXPECT_FALSE(state.isLegal(attackOn(0, 1)));
}

// ===========================================================================
// 九 · 非法动作不改变状态、胜负、逃跑
// ===========================================================================

TEST(BattleLegality, IllegalActionsChangeNothingAtAll) {
    std::vector<Unit> units{makeUnit("韩立", true, 12), makeUnit("张铁", true, 9), makeFoe("野狗", 8, 2)};
    units[0].mp = 5;   // 不够放火球
    BattleState state = makeBattle(units, 42);
    state.addMagic(makeFireball());
    state.setCanEscape(false);

    const std::vector<Unit> unitsBefore = state.units();
    const int roundBefore = state.round();
    const int actorBefore = state.currentActor();
    const std::size_t logBefore = state.log().size();
    const std::size_t eventsBefore = state.events().size();

    Action unknownItem = simple(ActionKind::Item, 0);
    unknownItem.magicId = "buzhidao";
    unknownItem.targetIndex = 0;
    const std::vector<Action> illegal{
        attackOn(0, 1),                                // 打自己人
        attackOn(0, 9),                                // 目标不存在
        attackOn(0, 2, fanren::core::kCategoryBlade),  // 手上没有刀
        attackOn(0, 2, kSword, 2),                     // 只有 1 点劲却蓄 2 点
        attackOn(0, 2, kSword, 4),                     // 一次最多蓄 3 点
        castOn(0, 2, "huoqiu"),                        // 法力不足
        castOn(0, 2, "meiyou"),                        // 法术不存在
        unknownItem,                                   // 物品不存在
        simple(ActionKind::Escape, 0),                 // 本战禁止逃跑
        simple(ActionKind::Charge, 0),                 // 他不会蓄势
        simple(ActionKind::Defend, 1),                 // 还没轮到张铁
        attackOn(2, 0),                                // 敌人不在行动序位上
    };
    for (const Action& action : illegal) {
        EXPECT_FALSE(state.isLegal(action));
        const auto result = state.apply(action);
        EXPECT_FALSE(result.ok);
        EXPECT_FALSE(result.error.empty());   // 失败原因要能直接显示给玩家
    }
    expectUnitsIdentical(unitsBefore, state.units());
    EXPECT_EQ(state.round(), roundBefore);
    EXPECT_EQ(state.currentActor(), actorBefore);
    EXPECT_EQ(state.log().size(), logBefore) << "非法动作连日志都不该留";
    EXPECT_EQ(state.events().size(), eventsBefore) << "也不该留事件";
}

TEST(BattleLegality, IsLegalAgreesWithApply) {
    BattleState state = makeBattle({makeUnit("韩立", true, 12), makeFoe("野狗", 8, 2)}, 3);
    state.addMagic(makeFireball());
    const std::vector<Action> probes{attackOn(0, 1), attackOn(0, 1, kSword, 1), attackOn(0, 1, kSword, 2),
                                     castOn(0, 1, "huoqiu"), simple(ActionKind::Defend, 0),
                                     simple(ActionKind::Charge, 0)};
    for (const Action& action : probes) {
        BattleState copy = state;
        const bool legal = copy.isLegal(action);
        EXPECT_EQ(legal, copy.apply(action).ok);
    }
}

TEST(BattleLegality, OneActionPerSlot) {
    BattleState state = makeBattle({makeUnit("韩立", true, 12), makeUnit("野狗", false, 8)});
    ASSERT_TRUE(state.apply(simple(ActionKind::Defend, 0)).ok);
    std::string why;
    EXPECT_FALSE(state.isLegal(attackOn(0, 1), &why));
    EXPECT_NE(why.find("本回合已经行动过"), std::string::npos) << why;
    EXPECT_EQ(state.decideAi(0).actorIndex, -1) << "这一位用过了，AI 也给不出动作";
}

TEST(BattleOutcome, WonWhenAllEnemiesFallAndLostWhenAllAlliesFall) {
    std::vector<Unit> units{makeUnit("韩立", true, 12), makeUnit("野狗", false, 8)};
    units[1].hp = 3;
    BattleState won = makeBattle(units);
    const auto result = won.apply(attackOn(0, 1));
    ASSERT_TRUE(result.ok);
    EXPECT_NE(result.value.find("倒地不起"), std::string::npos);
    EXPECT_EQ(won.phase(), BattlePhase::Won);
    EXPECT_EQ(countEvents(won.events(), BattleEventKind::End), 1);
    EXPECT_FALSE(won.apply(attackOn(0, 1)).ok) << "打完了什么也不许再做";

    std::vector<Unit> other{makeUnit("韩立", true, 8), makeUnit("野狗", false, 20)};
    other[0].hp = 3;
    BattleState lost = makeBattle(other);
    ASSERT_TRUE(lost.apply(attackOn(1, 0)).ok);
    EXPECT_EQ(lost.phase(), BattlePhase::Lost);
}

TEST(BattleOutcome, AFallenUnitIsSkippedInTheOrder) {
    std::vector<Unit> units{makeUnit("韩立", true, 12), makeUnit("野狗", false, 8),
                            makeUnit("恶犬", false, 6)};
    units[1].hp = 3;
    BattleState state = makeBattle(units);
    ASSERT_TRUE(state.apply(attackOn(0, 1)).ok);
    EXPECT_EQ(state.phase(), BattlePhase::Ongoing);
    state.endTurn();
    EXPECT_EQ(state.currentActor(), 2) << "倒下的那一位被跳过";
}

TEST(BattleEscape, BothOutcomesReachableAndSeedStable) {
    const auto runEscape = [](std::uint32_t seed) {
        BattleState state = makeBattle({makeUnit("韩立", true, 12), makeUnit("野狗", false, 8)}, seed);
        EXPECT_TRUE(state.apply(simple(ActionKind::Escape, 0)).ok);
        return state.phase() == BattlePhase::Escaped;
    };
    bool sawSuccess = false;
    bool sawFailure = false;
    std::uint32_t successSeed = 0;
    std::uint32_t failureSeed = 0;
    for (std::uint32_t seed = 0; seed < 64; ++seed) {
        if (runEscape(seed)) {
            sawSuccess = true;
            successSeed = seed;
        } else {
            sawFailure = true;
            failureSeed = seed;
        }
    }
    ASSERT_TRUE(sawSuccess);
    ASSERT_TRUE(sawFailure);
    EXPECT_TRUE(runEscape(successSeed));
    EXPECT_FALSE(runEscape(failureSeed));
}

TEST(BattleEscape, DisabledOrEnemySideIsIllegal) {
    BattleState state = makeBattle({makeUnit("韩立", true, 12), makeUnit("野狗", false, 8)});
    state.setCanEscape(false);
    EXPECT_FALSE(state.isLegal(simple(ActionKind::Escape, 0)));
    BattleState other = makeBattle({makeUnit("韩立", true, 12), makeUnit("野狗", false, 8)});
    other.endTurn();
    ASSERT_EQ(other.currentActor(), 1);
    EXPECT_FALSE(other.isLegal(simple(ActionKind::Escape, 1)));
}

// ===========================================================================
// 十 · 识海：吞噬模式里没有架势、破绽与劲
// ===========================================================================

TEST(BattleDevour, TheMindHasNoToughnessNoWeaknessAndNoBp) {
    Unit hero = makeUnit("hanli", true, 12);
    Unit ball = makeFoe("ball", 8, 2);
    hero.hp = hero.maxHp = 480;
    ball.hp = ball.maxHp = 660;
    BattleState state = makeBattle({hero, ball});
    state.setDevourMode(true);
    EXPECT_EQ(state.units()[0].bp, 0);
    EXPECT_EQ(state.units()[1].maxToughness, 0);
    EXPECT_FALSE(state.isLegal(attackOn(0, 1, 0, 1))) << "梦里没有劲可蓄";
    ASSERT_TRUE(state.apply(attackOn(0, 1, kSword)).ok);
    EXPECT_FALSE(state.units()[1].broken());
    EXPECT_EQ(state.units()[1].revealed, 0);
    for (int i = 0; i < 6; ++i) nextRound(state);
    EXPECT_EQ(state.units()[0].bp, 0) << "回合过去也不长劲";
}

// ===========================================================================
// 十一 · AI
// ===========================================================================

TEST(BattleAi, AnEnemyWithManaCastsItsHardestSpell) {
    std::vector<Unit> units{makeUnit("韩立", true, 5), makeUnit("妖修", false, 20)};
    units[1].magicsExhaustive = true;
    units[1].magics = {"huoqiu"};
    BattleState state = makeBattle(units);
    state.addMagic(makeFireball());
    ASSERT_EQ(state.currentActor(), 1);
    const Action action = state.decideAi(1);
    EXPECT_EQ(action.kind, ActionKind::Cast) << "设计原文：有法术且法力够就挑一门";
    EXPECT_EQ(action.magicId, "huoqiu");
}

TEST(BattleAi, WithoutManaItFallsBackToAnAttack) {
    std::vector<Unit> units{makeUnit("韩立", true, 5), makeUnit("妖修", false, 20)};
    units[1].magicsExhaustive = true;
    units[1].magics = {"huoqiu"};
    units[1].mp = 3;
    BattleState state = makeBattle(units);
    state.addMagic(makeFireball());
    EXPECT_EQ(state.decideAi(1).kind, ActionKind::Attack);
}

TEST(BattleAi, AnEnemyGoesForTheOneItCanKnockOut) {
    std::vector<Unit> units{makeUnit("韩立", true, 5), makeUnit("张铁", true, 4), makeUnit("野狗", false, 20)};
    units[0].hp = 80;
    units[1].hp = 10;   // 15 点一下就倒
    for (std::uint32_t seed = 0; seed < 8; ++seed) {
        BattleState s = makeBattle(units, seed);
        const Action action = s.decideAi(2);
        EXPECT_EQ(action.targetIndex, 1) << "种子 " << seed << "：打得倒的优先，不看运气";
    }
}

TEST(BattleAi, OtherwiseItPicksBySeedButLeansTowardTheWeaker) {
    // 都打不倒时按种子随机、偏向气血低的：同一局面每次同一个答案；换种子两个都打过，
    // 而低血的那个被挑中的次数更多。
    std::vector<Unit> units{makeUnit("韩立", true, 5), makeUnit("张铁", true, 4), makeUnit("野狗", false, 20)};
    units[0].hp = 90;
    units[1].hp = 30;
    int weaker = 0;
    int stronger = 0;
    for (std::uint32_t seed = 0; seed < 200; ++seed) {
        BattleState a = makeBattle(units, seed);
        BattleState b = makeBattle(units, seed);
        const int target = a.decideAi(2).targetIndex;
        EXPECT_EQ(target, b.decideAi(2).targetIndex) << "同一局面、同一种子必须同一个目标";
        (target == 1 ? weaker : stronger) += 1;
    }
    EXPECT_GT(stronger, 0) << "不是非打低血的不可";
    EXPECT_GT(weaker, stronger) << "但偏向低血的那个";
}

TEST(BattleAi, OurAutopilotHitsAKnownWeakness) {
    std::vector<Unit> units{makeUnit("韩立", true, 12), makeFoe("恶狼", 8, 3)};
    units[1].revealed = kSword;
    BattleState state = makeBattle(units);
    const Action action = state.decideAi(0);
    EXPECT_EQ(action.kind, ActionKind::Attack);
    EXPECT_EQ(action.category, kSword);
}

TEST(BattleAi, OurAutopilotProbesWhatItHasNotTriedYet) {
    // 拳试过、没揭开什么：下一手换剑去探。
    std::vector<Unit> units{makeUnit("韩立", true, 12), makeFoe("恶狼", 8, 3)};
    BattleState state = makeBattle(units);
    ASSERT_TRUE(state.apply(attackOn(0, 1, kFist)).ok);
    nextRound(state);
    ASSERT_EQ(state.currentActor(), 0);
    const Action action = state.decideAi(0);
    EXPECT_EQ(action.category, kSword);
}

TEST(BattleAi, OurAutopilotBoostsFullyOnABrokenFoeAndSpendsOneWhenFull) {
    BattleState state = makeBattle({makeUnit("韩立", true, 12), makeFoe("恶狼", 8, 1)});
    nextRound(state);
    nextRound(state);
    ASSERT_TRUE(state.apply(attackOn(0, 1, kSword)).ok);   // 破势
    nextRound(state);
    ASSERT_GE(state.units()[0].bp, 3);
    EXPECT_EQ(state.decideAi(0).boost, 3) << "目标破势时把劲蓄满打";

    BattleState full = makeBattle({makeUnit("韩立", true, 12), makeFoe("恶狼", 8, 99)});
    for (int i = 0; i < 4; ++i) nextRound(full);
    ASSERT_EQ(full.units()[0].bp, 5);
    EXPECT_EQ(full.decideAi(0).boost, 1) << "劲满 5 点时蓄 1 点，免得溢出";
}

// ===========================================================================
// 十二 · 同种子可复现
// ===========================================================================

BattleState runScriptedBattle(std::uint32_t seed, int maxRounds) {
    std::vector<Unit> units{makeUnit("韩立", true, 12), makeUnit("张铁", true, 9), makeFoe("野狗", 11, 2),
                            makeFoe("恶犬", 7, 3)};
    units[0].element = fanren::core::kElementFire;
    units[2].element = fanren::core::kElementMetal;
    BattleState state = makeBattle(units, seed);
    state.addMagic(makeFireball(MagicBoost::Hits));
    while (state.phase() == BattlePhase::Ongoing && state.round() <= maxRounds) {
        const int actor = state.currentActor();
        if (actor >= 0) {
            const Action action = state.decideAi(actor);
            if (action.actorIndex >= 0) EXPECT_TRUE(state.apply(action).ok) << "AI 给出了非法动作";
        }
        state.endTurn();
    }
    return state;
}

TEST(BattleDeterminism, SameSeedSameSequenceSameResult) {
    const BattleState first = runScriptedBattle(20260920, 40);
    const BattleState second = runScriptedBattle(20260920, 40);
    ASSERT_EQ(first.log().size(), second.log().size());
    EXPECT_EQ(first.log(), second.log());
    EXPECT_EQ(first.round(), second.round());
    EXPECT_EQ(first.phase(), second.phase());
    ASSERT_EQ(first.events().size(), second.events().size());
    for (std::size_t i = 0; i < first.events().size(); ++i) {
        EXPECT_EQ(first.events()[i].kind, second.events()[i].kind) << "事件 " << i;
        EXPECT_EQ(first.events()[i].value, second.events()[i].value) << "事件 " << i;
    }
    expectUnitsIdentical(first.units(), second.units());
    EXPECT_FALSE(first.log().empty());
    EXPECT_NE(first.phase(), BattlePhase::Ongoing) << "先验：两边的 AI 真的把仗打完了";
}

// ===========================================================================
// 十三 · 已揭开的破绽跨战斗记住（GameState / 存档 v7 / BattleScene）
// ===========================================================================

TEST(BattleKnowledge, LearningWeaknessesOnlyEverAdds) {
    GameState s;
    s.learnWeaknesses("wild_wolf", kSword);
    s.learnWeaknesses("wild_wolf", kFire);
    s.learnWeaknesses("wild_wolf", 0);
    EXPECT_EQ(s.knownWeaknessesOf("wild_wolf"), kSword | kFire);
    EXPECT_EQ(s.knownWeaknessesOf("jiang_shou"), 0);
    s.learnWeaknesses("jiang_shou", 0);
    EXPECT_EQ(s.knownWeaknesses.count("jiang_shou"), 0u) << "知道了零样不留空账";
}

GameState minimalState() {
    GameState s;
    s.mapId = "ch03_guwai";
    s.position = fanren::core::Point{3, 4};
    s.realm = Realm::QiRefining3;
    s.hp = s.maxHp = 60;
    s.mp = s.maxMp = 30;
    s.chapter = 3;
    return s;
}

TEST(BattleKnowledge, TheSaveKeepsThemByNameAndBringsThemBack) {
    GameState before = minimalState();
    before.learnWeaknesses("wild_wolf", kSword | kFist);
    before.learnWeaknesses("jiang_shou", kPoison);
    const fs::path path = fanren::test::uniqueTempPath("fanren_battle_knowledge", ".json");
    ASSERT_TRUE(fanren::io::saveGame(before, path.string()).ok);
    std::string text;
    {
        std::ifstream in(path, std::ios::binary);
        std::ostringstream buffer;
        buffer << in.rdbuf();
        text = buffer.str();
    }
    // 存的是名字，不是位号（SaveFile.h 第 7 条）。
    EXPECT_NE(text.find("\"knownWeaknesses\""), std::string::npos);
    EXPECT_NE(text.find("拳"), std::string::npos);
    // 设计原文：破绽让存档升到 v7；此后野外遭遇的计数器又推到 v8（docs/interfaces-octo-encounters.md 第 7 节）。
    EXPECT_NE(text.find("\"save_version\": 8"), std::string::npos) << "设计原文：存档升到 v8";
    const auto after = fanren::io::loadGame(path.string());
    std::error_code ec;
    fs::remove(path, ec);
    ASSERT_TRUE(after.ok) << after.error;
    EXPECT_EQ(after.value.knownWeaknesses, before.knownWeaknesses);
}

std::uint64_t testFnv1a64(const std::string& data) {
    std::uint64_t hash = 0xcbf29ce484222325ULL;
    for (const char c : data) {
        hash ^= static_cast<unsigned char>(c);
        hash *= 0x100000001b3ULL;
    }
    return hash;
}

std::string handSave(int version, const std::string& payload) {
    std::ostringstream sum;
    sum << std::hex << std::setw(16) << std::setfill('0')
        << testFnv1a64(std::to_string(version) + "|" + payload);
    return "{\"save_version\":" + std::to_string(version) + ",\"checksum\":\"" + sum.str() +
           "\",\"payload\":" + payload + "}";
}

// 一份最小的 v6 payload：nlohmann 的 dump() 按 key 字母序、无空格，校验和按它算。
std::string v6Payload(const std::string& extra = {}) {
    return R"({"bag":[],"chapter":3,"cultivation":0,"day":1,"facing":2,"flags":{},"hp":60)" + extra +
           R"(,"mapId":"ch03_guwai","maxHp":60,"maxMp":30,"mp":30,"playSecondsGameplay":0.0,)"
           R"("playSecondsSystem":0.0,"position":{"x":3,"y":4},"realm":3,"realmCap":3})";
}

fs::path writeTemp(const std::string& tag, const std::string& text) {
    const fs::path path = fanren::test::uniqueTempPath(tag, ".json");
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out << text;
    return path;
}

TEST(BattleKnowledge, AVersionSixSaveLoadsWithNothingKnown) {
    // 设计原文：旧档 = 全未知。6→7 迁移若忘了登记，这里读都读不回来。
    const fs::path path = writeTemp("fanren_v6_save", handSave(6, v6Payload()));
    const auto loaded = fanren::io::loadGame(path.string());
    std::error_code ec;
    fs::remove(path, ec);
    ASSERT_TRUE(loaded.ok) << loaded.error;
    EXPECT_TRUE(loaded.value.knownWeaknesses.empty());
    EXPECT_EQ(loaded.value.mapId, "ch03_guwai") << "先验：这份 v6 档确实读进来了";
}

TEST(BattleKnowledge, TheShippedChapterEndSavesStillLoad) {
    // tests/fixtures/ 下那六份章末交接存档：一份也不能读不回来。B2 重生成之后它们是 v7，带着一路
    // 揭开过的破绽（knownWeaknesses 非空是对的）；「v6 旧档 = 全都不知道」由上面手搭的那份 v6 档钉着。
    std::string root = ".";
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "tests" / "fixtures" / "ch05-end-first.sav")) {
            root = candidate;
            break;
        }
    }
    int loaded = 0;
    for (const char* name : {"ch03-end-first.sav", "ch03-end-second.sav", "ch04-end-first.sav",
                             "ch04-end-second.sav", "ch05-end-first.sav", "ch05-end-second.sav"}) {
        const auto save = fanren::io::loadGame((fs::path(root) / "tests" / "fixtures" / name).string());
        EXPECT_TRUE(save.ok) << name << "：" << save.error;
        if (save.ok) ++loaded;
    }
    EXPECT_EQ(loaded, 6);
}

TEST(BattleKnowledge, ABrokenKnowledgeEntryIsRefusedInsteadOfDropped) {
    const std::string badName =
        R"(,"knownWeaknesses":{"wild_wolf":["剑","龙"]})";
    const fs::path path = writeTemp("fanren_v7_bad", handSave(7, v6Payload(badName)));
    const auto loaded = fanren::io::loadGame(path.string());
    std::error_code ec;
    fs::remove(path, ec);
    EXPECT_FALSE(loaded.ok) << "认不出的类别名不该被悄悄丢掉";
    EXPECT_NE(loaded.error.find("knownWeaknesses"), std::string::npos) << loaded.error;
}

std::string assetRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "data" / "battles" / "b03_gu_wai_elang.json")) return candidate;
    }
    return ".";
}

class BattleKnowledgeScene : public ::testing::Test {
protected:
    void SetUp() override {
        auto ready = app_.init(assetRoot(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
        app_.state().realm = Realm::QiRefining8;
        app_.state().hp = app_.state().maxHp = 120;
        app_.state().mp = app_.state().maxMp = 80;
    }
    void TearDown() override { app_.shutdown(); }
    fanren::game::Application app_;
};

TEST_F(BattleKnowledgeScene, WhatTheLastFightRevealedIsLitFromTheStart) {
    app_.state().learnWeaknesses("wild_wolf", kSword);
    fanren::game::BattleScene scene("b03_gu_wai_elang");
    scene.onEnter(app_);
    int wolves = 0;
    for (const Unit& u : scene.battle().units()) {
        if (u.id != "wild_wolf") continue;
        ++wolves;
        EXPECT_EQ(u.revealed, kSword) << "上一场揭开的那一格，这一场一开场就亮着";
    }
    EXPECT_EQ(wolves, 2);
}

TEST_F(BattleKnowledgeScene, WhatThisFightRevealsIsWrittenBack) {
    ASSERT_EQ(app_.state().knownWeaknessesOf("wild_wolf"), 0);
    fanren::game::BattleScene scene("b03_gu_wai_elang");
    scene.onEnter(app_);
    scene.runToCompletion(200);
    scene.update(app_, 1.0 / 60.0);   // 这一帧里 finish() 会跑
    int revealed = 0;
    for (const Unit& u : scene.battle().units()) {
        if (u.id == "wild_wolf") revealed |= u.revealed;
    }
    ASSERT_NE(revealed, 0) << "先验：这一仗确实揭开过破绽（我方 AI 会去探）";
    EXPECT_EQ(app_.state().knownWeaknessesOf("wild_wolf"), revealed);
}

TEST_F(BattleKnowledgeScene, TheFightHanLiSleptThroughRevealsNothingToHim) {
    // 夺帮那一夜他在客栈睡觉：存档一个字节不动（契约 docs/interfaces-p3-ch05.md 第 2 节），
    // 破绽也一样——他没看见。
    fanren::game::BattleScene scene("b05_duobang");
    scene.onEnter(app_);
    scene.runToCompletion(200);
    scene.update(app_, 1.0 / 60.0);
    EXPECT_TRUE(app_.state().knownWeaknesses.empty());
}

}  // namespace
