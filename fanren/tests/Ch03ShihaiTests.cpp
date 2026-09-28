// P3 第 3 章：识海之战（契约 docs/interfaces-p3-ch03.md 第 3 节）。
//
// 原著 ch56 是两场连着的：先一口吞掉比自己小好几倍的黄光球，再贪心追上去咬那团
// 比自己大一圈的绿光球——后者是**设计上打不死的**，咬到它体积少三分之一就脱身
// 跑了，战果按咬下多少计。
//
// 这里既钉住纯函数（倍率、阈值、战果口径），也钉住整场打下来的结局，
// 还配了两条反向对照：不开吞噬模式时同一套循环**打得死**人，
// 咬得不够深时它**不会**跑。缺了这两条，「必然 EnemyFled」有可能只是因为
// 这场仗本来就分不出胜负。
#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "core/battle/Battle.h"
#include "core/battle/Damage.h"
#include "game/Application.h"
#include "game/BattleScene.h"

namespace {

using fanren::core::battle::Action;
using fanren::core::battle::ActionKind;
using fanren::core::battle::BattlePhase;
using fanren::core::battle::BattleState;
using fanren::core::battle::Unit;
using fanren::core::battle::devourBite;
using fanren::core::battle::devourBittenPercent;
using fanren::core::battle::devourCanBreakAway;
using fanren::core::battle::devourShouldFlee;
using fanren::core::battle::kDevourPlayerVolume;
using fanren::rules::Realm;

Unit makeBall(const std::string& name, bool ally, int volume, int speed) {
    Unit u;
    u.id = name;
    u.name = name;
    u.hp = u.maxHp = volume;
    u.attack = 10;
    u.defence = 5;
    u.speed = speed;
    u.realm = Realm::QiRefining3;
    u.ally = ally;
    return u;
}

// 韩立（快，先手）与一团光球。
BattleState makeMindDuel(int heroVolume, int foeVolume) {
    std::vector<Unit> units{makeBall("hanli", true, heroVolume, 12),
                            makeBall("ball", false, foeVolume, 8)};
    BattleState state;
    state.setup(std::move(units), 7u);
    state.setCanEscape(false);
    state.setDevourMode(true);
    return state;
}

Action biteAt(int actor, int target) {
    Action a;
    a.kind = ActionKind::Attack;
    a.actorIndex = actor;
    a.targetIndex = target;
    return a;
}

// 双方都只做一件事：咬得着就咬。返回打了多少个回合。
int fightItOut(BattleState& state, int maxRounds) {
    for (int guard = 0; guard < maxRounds * 8; ++guard) {
        if (state.phase() != BattlePhase::Ongoing) break;
        if (state.round() > maxRounds) break;
        const int actor = state.currentActor();
        if (actor < 0) {
            state.endTurn();
            continue;
        }
        const Action action = state.decideAi(actor);
        if (action.actorIndex >= 0) (void)state.apply(action);
        state.endTurn();
    }
    return state.round();
}

// ---- 纯函数 ----

TEST(Ch03Devour, BiteDepthFollowsVolume) {
    EXPECT_EQ(devourBite(kDevourPlayerVolume), kDevourPlayerVolume / 16);
    EXPECT_EQ(devourBite(480), 30) << "韩立入梦时一口正好咬掉黄光球体积的四分之一";
    EXPECT_EQ(devourBite(960), 60) << "吞大了就咬得更深";
    // 体积再小也还咬得动，否则两团小光球会互相磨到天荒地老。
    EXPECT_EQ(devourBite(3), 1);
    EXPECT_EQ(devourBite(0), 1);
}

TEST(Ch03Devour, FleesExactlyWhenAThirdIsGone) {
    EXPECT_FALSE(devourShouldFlee(900, 900));
    EXPECT_FALSE(devourShouldFlee(600, 900)) << "正好剩三分之二还不跑，再咬一口才跑";
    EXPECT_TRUE(devourShouldFlee(599, 900));
    EXPECT_TRUE(devourShouldFlee(1, 900));
    // 先验分母：底数塌成 0 时不能恒真——那是判据空转的第四种长相。
    EXPECT_FALSE(devourShouldFlee(100, 0));
    EXPECT_FALSE(devourShouldFlee(0, 0));
}

TEST(Ch03Devour, OnlyABallThatCameInBiggerCanBreakAway) {
    // 原著的两场之所以不同，分水岭就在这里：黄光球比韩立小好几倍，一口就是它的
    // 四分之一，脱不开身，被整团吞掉；绿光球比韩立大一圈，舍掉一块还剩得下自己。
    EXPECT_TRUE(devourCanBreakAway(680, 480)) << "比韩立大的那团逃得掉";
    EXPECT_FALSE(devourCanBreakAway(120, 480)) << "小好几倍的那团逃不掉，它是被吞的";
    EXPECT_FALSE(devourCanBreakAway(480, 480)) << "一般大也不行：要大过对方才脱得开";
    EXPECT_FALSE(devourCanBreakAway(680, 0)) << "没有底数就不作数";
}

TEST(Ch03Devour, SpoilsAreMeasuredAgainstTheStartingVolume) {
    EXPECT_EQ(devourBittenPercent(900, 900), 0);
    EXPECT_EQ(devourBittenPercent(600, 900), 33) << "原著：体积少了三分之一";
    EXPECT_EQ(devourBittenPercent(450, 900), 50);
    EXPECT_EQ(devourBittenPercent(0, 900), 100);
    EXPECT_EQ(devourBittenPercent(100, 0), 0) << "没有底数就不该算出一个大数来";
}

// ---- 第一场：吞掉比自己小好几倍的那一团 ----

TEST(Ch03Shihai, SwallowingASmallerBallMakesYouBigger) {
    BattleState state = makeMindDuel(kDevourPlayerVolume, 120);
    const int heroVolume = state.units()[0].maxHp;

    auto result = state.apply(biteAt(0, 1));
    ASSERT_TRUE(result.ok) << result.error;

    const int bite = devourBite(heroVolume);
    EXPECT_EQ(state.units()[0].maxHp, heroVolume + bite) << "吞噬使你变大";
    EXPECT_EQ(state.units()[0].hp, heroVolume + bite);
    EXPECT_EQ(state.units()[1].maxHp, 120 - bite) << "被咬掉的那块没了";
    EXPECT_EQ(state.units()[1].hp, 120 - bite);
    EXPECT_NE(result.value.find("体积"), std::string::npos) << "日志要说清发生了什么";
}

TEST(Ch03Shihai, TheSmallBallIsEatenOutrightAndThePhaseIsWon) {
    BattleState state = makeMindDuel(kDevourPlayerVolume, 120);
    fightItOut(state, 40);

    EXPECT_EQ(state.phase(), BattlePhase::Won) << "小的那团是被吞掉的，不是跑掉的";
    EXPECT_FALSE(state.units()[1].fled) << "比自己小好几倍的那团不该跑得掉";
    EXPECT_EQ(state.units()[1].maxHp, 0) << "整团都被吞了，一点体积不剩";
    EXPECT_EQ(state.devourSpoilsPercent(), 0) << "没人逃走就没有「咬下多少」这回事";
    // 净下来还是长了：吃进去 120，挨的那几口远不到这个数。
    EXPECT_GT(state.units()[0].maxHp, kDevourPlayerVolume) << "吞完一团之后韩立必须更大";
}

TEST(Ch03Shihai, AnInvaderTearsOffVolumeButDoesNotGrowOnIt) {
    // 只有我方吃得下去（理由见 BattleState::devourTransfer 的注释）。
    // 双方都吞噬的话，大的那个滚雪球，第二场就成了绿光球反过来把韩立吃掉。
    std::vector<Unit> units{makeBall("hanli", true, 480, 6), makeBall("ball", false, 680, 12)};
    BattleState state;
    state.setup(std::move(units), 7u);
    state.setDevourMode(true);
    ASSERT_EQ(state.currentActor(), 1) << "这一局让入侵者先手";

    auto result = state.apply(biteAt(1, 0));
    ASSERT_TRUE(result.ok) << result.error;
    const int bite = devourBite(680);
    EXPECT_EQ(state.units()[0].maxHp, 480 - bite) << "被撕下的那块确实没了";
    EXPECT_EQ(state.units()[1].maxHp, 680) << "入侵者咬下的那块带不走，体积不该长";
    EXPECT_EQ(state.units()[1].hp, 680);
}

// ---- 第二场：设计上打不死的那一团 ----

TEST(Ch03Shihai, TheBiggerBallAlwaysGetsAwayNoMatterHowLongYouFight) {
    // 契约第 3.4 节：玩家不能靠打得久把它打死。
    BattleState state = makeMindDuel(kDevourPlayerVolume, 680);
    const int rounds = fightItOut(state, 200);

    EXPECT_EQ(state.phase(), BattlePhase::EnemyFled)
        << "打了 " << rounds << " 个回合，结局仍然只能是敌人逃走";
    EXPECT_NE(state.phase(), BattlePhase::Won);
    EXPECT_TRUE(state.units()[1].fled);
    EXPECT_GT(state.units()[1].hp, 0) << "它是跑掉的，不是被打倒的";
}

TEST(Ch03Shihai, TheSpoilsMatchTheVolumeActuallyBittenOff) {
    BattleState state = makeMindDuel(kDevourPlayerVolume, 680);
    fightItOut(state, 200);
    ASSERT_EQ(state.phase(), BattlePhase::EnemyFled);

    const int left = state.units()[1].maxHp;
    // 战果口径：被咬掉的体积占开战体积的百分比。这里不写死一个数，而是与
    // 「实际少了多少」对账——只钉一个数据点的话，另一条同样错的曲线只要
    // 穿过该点就能蒙混过关。
    EXPECT_EQ(state.devourSpoilsPercent(), devourBittenPercent(left, 680));
    EXPECT_GE(state.devourSpoilsPercent(), 33) << "至少要咬下三分之一才肯跑";
    EXPECT_LE(state.devourSpoilsPercent(), 50) << "跑的是它，不是被慢慢啃掉一半";
    EXPECT_LT(left, 680) << "先验：它确实少了一块，百分比才有意义";
}

TEST(Ch03Shihai, AFledBallIsNoLongerOnTheField) {
    BattleState state = makeMindDuel(kDevourPlayerVolume, 680);
    fightItOut(state, 200);
    ASSERT_EQ(state.phase(), BattlePhase::EnemyFled);

    const Unit& gone = state.units()[1];
    EXPECT_FALSE(gone.alive()) << "逃走的一团不在场上：行动序、选中、胜负都当它不在";
    EXPECT_FALSE(state.isLegal(biteAt(0, 1))) << "跑了就咬不着了";
}

TEST(Ch03Shihai, DoesNotFleeBeforeTheThresholdIsReached) {
    // 反向对照之一：逃走不是无条件的。咬得浅（韩立体积小 → 一口只咬 1 分），
    // 三十个回合也到不了三分之一，那它就该老老实实待在场上。
    BattleState state = makeMindDuel(16, 680);
    fightItOut(state, 30);

    EXPECT_FALSE(state.units()[1].fled) << "还没咬掉三分之一就跑，等于阈值根本没在判";
    EXPECT_NE(state.phase(), BattlePhase::EnemyFled);
    EXPECT_EQ(state.devourSpoilsPercent(), 0);
}

TEST(Ch03Shihai, WithoutDevourModeTheSameFightCanBeWon) {
    // 反向对照之二：证明上面那条「必然逃走」来自吞噬模式，
    // 而不是因为这套循环本来就打不出胜负。
    std::vector<Unit> units{makeBall("hanli", true, 480, 12), makeBall("ball", false, 680, 8)};
    units[0].attack = 400;   // 一击必杀，排除掉「只是打得不够久」这个解释
    BattleState state;
    state.setup(std::move(units), 7u);
    fightItOut(state, 200);

    EXPECT_EQ(state.phase(), BattlePhase::Won);
    EXPECT_FALSE(state.units()[1].fled);
}

// ---- 真数据：data/battles/b03_shihai_duoshe.json ----

class Ch03ShihaiDataTest : public ::testing::Test {
protected:
    static std::string assetRoot() {
        namespace fs = std::filesystem;
        for (const char* candidate : {".", "..", "../..", "../../.."}) {
            if (fs::exists(fs::path(candidate) / "data" / "battles" / "b03_shihai_duoshe.json")) {
                return candidate;
            }
        }
        return ".";
    }

    void SetUp() override {
        auto ready = app_.init(assetRoot(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
    }
    void TearDown() override { app_.shutdown(); }

    fanren::game::Application app_;
};

TEST_F(Ch03ShihaiDataTest, TheDataHasTwoBallsOneSmallOneBig) {
    const fanren::core::BattleSetup* setup = app_.battleSetup("b03_shihai_duoshe");
    ASSERT_NE(setup, nullptr);
    EXPECT_EQ(setup->terrain, fanren::game::kMindTerrain) << "吞噬模式挂在地形上";
    EXPECT_FALSE(setup->canEscape) << "识海里不能逃";
    ASSERT_EQ(setup->units.size(), 2u) << "原著是两场连着的，不是一场";

    const fanren::core::RoleTemplate* first = app_.data().findRole(setup->units[0].roleId);
    const fanren::core::RoleTemplate* second = app_.data().findRole(setup->units[1].roleId);
    ASSERT_NE(first, nullptr) << "第一团的角色 id 在 data/roles 里查不到";
    ASSERT_NE(second, nullptr) << "第二团的角色 id 在 data/roles 里查不到";

    // 第一团「比韩立小好几倍」：至少小三倍。
    EXPECT_LE(first->maxHp * 3, kDevourPlayerVolume)
        << "第一团不够小，吞不成「轻易吞掉」那回事";
    // 第二团「比韩立大一圈」。
    EXPECT_GT(second->maxHp, kDevourPlayerVolume) << "第二团不够大";
    EXPECT_LT(second->maxHp, kDevourPlayerVolume * 2) << "大一圈，不是大一倍";
    // 「两场连着的」：小的那团开场就在，大的那团是第 1 波——吞掉第一团之后它才进场。
    //
    // 从前（战棋）这一句写的是「两团站的不是同一格」：远的那团在对角，要韩立自己走过去。
    // 横版没有距离，同时在场的话绿光球每回合都咬韩立一口（660 ÷ 16 = 41 分），韩立一边吞
    // 黄光球一边被它啃，第一场就不再是「靠体积轻易吞掉」。所以先后改由波次说话
    //（data/battles/b03_shihai_duoshe.json 的 note）。
    EXPECT_EQ(setup->units[0].wave, 0) << "第一团开场就在";
    EXPECT_EQ(setup->units[1].wave, 1) << "第二团是第二场：第一团没了它才来";
    for (const fanren::core::BattleUnitSpec& spec : setup->units) EXPECT_FALSE(spec.ally);
}

TEST_F(Ch03ShihaiDataTest, PlayingTheRealBattleEndsWithTheEnemyFleeing) {
    fanren::game::BattleScene scene("b03_shihai_duoshe");
    scene.onEnter(app_);
    ASSERT_TRUE(scene.battle().devourMode()) << "识海这一场必须是吞噬模式";
    // 韩立在识海里的体积由引擎给，与存档里的气血无关。
    EXPECT_EQ(scene.battle().units()[0].maxHp, kDevourPlayerVolume);

    const bool won = scene.runToCompletion(200);
    EXPECT_FALSE(won);
    EXPECT_EQ(scene.battle().phase(), BattlePhase::EnemyFled)
        << "整章的高潮压在这个落点上：一团被吞掉，另一团带着伤跑了";
    EXPECT_GE(scene.battle().devourSpoilsPercent(), 33);
}

TEST_F(Ch03ShihaiDataTest, TheMindBattleDoesNotWriteVolumeBackIntoTheSave) {
    // 识海打的是元神，肉身正躺在床上。照写回去的话，一个 maxHp 只有几十的韩立
    // 会带着五百多点气血醒过来。
    app_.state().hp = 26;
    app_.state().maxHp = 42;

    fanren::game::BattleScene scene("b03_shihai_duoshe");
    scene.onEnter(app_);
    scene.runToCompletion(200);
    scene.update(app_, 1.0 / 60.0);   // 这一帧里 finish() 会跑

    EXPECT_EQ(app_.state().hp, 26) << "肉身的气血不该被识海里的体积改写";
    EXPECT_EQ(app_.state().maxHp, 42);
}

TEST_F(Ch03ShihaiDataTest, AnOrdinaryBattleStillWritesHpBack) {
    // 反向对照：不写回只是识海这一场的规矩，别把气血带出战场这条整个拆了。
    app_.state().hp = 100;
    app_.state().maxHp = 100;

    fanren::game::BattleScene scene("b03_gu_wai_elang");
    scene.onEnter(app_);
    ASSERT_FALSE(scene.battle().devourMode());
    scene.runToCompletion(200);
    scene.update(app_, 1.0 / 60.0);

    EXPECT_EQ(app_.state().hp, std::max(1, scene.battle().units()[0].hp))
        << "普通战斗的气血照旧要带出战场";
}

}  // namespace
