// P3 第 4 章前置之二：多波次战斗（契约 docs/interfaces-p3-ch04.md 第 2 节）。
//
// 契约的三条硬要求，本文件逐条钉：
//   2.1 打完当前波之后下一波进场；**波与波之间不回血、不重置**；最后一波打完才算 Won。
//   2.2 **老的单波战斗数据一字不改仍按单波跑**（扫一片，不是只钉一场）。
//   2.3 **进入下一波必须在界面上说得出来**。
//
// 「不回血」那一条是本文件最要紧的断言，写法上刻意做成**加一行回血就会转红**：
// 每一条都先验「转场之前他确实带着伤 / 确实中着毒」（分母 > 0），再验转场之后
// 那两个数一个字节没变。少了先验那一半，一份把所有人回满血的实现照样能让
// 「hp == hpBefore」在满血局面下全绿——那正是 docs/README.md 那张表上
// 「比值分母塌成 0」的同一个形状。
#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "TempDir.h"
#include "core/battle/Battle.h"
#include "core/model/Types.h"
#include "game/Application.h"
#include "game/BattleScene.h"
#include "io/BattleLoader.h"

namespace {

namespace fs = std::filesystem;

using fanren::core::battle::Action;
using fanren::core::battle::ActionKind;
using fanren::core::battle::BattlePhase;
using fanren::core::battle::BattleState;
using fanren::core::battle::Unit;
using fanren::game::BattleScene;
using fanren::game::waveStatusText;
using fanren::rules::Realm;

Unit makeUnit(const std::string& name, bool ally, int wave, int hp = 100) {
    Unit u;
    u.id = name;
    u.name = name;
    u.hp = u.maxHp = hp;
    u.mp = u.maxMp = 0;
    u.attack = 20;
    u.defence = 1;
    u.speed = ally ? 9 : 1;   // 我方先动，测试才好一步步推
    u.realm = Realm::Mortal;
    u.ally = ally;
    u.magicsExhaustive = true;
    u.wave = wave;
    return u;
}

int indexOf(const BattleState& battle, const std::string& name) {
    const auto& units = battle.units();
    for (std::size_t i = 0; i < units.size(); ++i) {
        if (units[i].name == name) return static_cast<int>(i);
    }
    return -1;
}

const Unit& unitNamed(const BattleState& battle, const std::string& name) {
    return battle.units()[static_cast<std::size_t>(indexOf(battle, name))];
}

bool logHas(const BattleState& battle, const std::string& needle) {
    return std::any_of(
        battle.log().begin(), battle.log().end(),
        [&needle](const std::string& line) { return line.find(needle) != std::string::npos; });
}

// 打倒 name 这个单位：由 hero 一刀一刀砍，中间该谁动就谁动。
// 不直接把 hp 写成 0——那样走不到 refreshPhase，而推波正是在那里发生的。
void strikeDown(BattleState& battle, const std::string& attacker, const std::string& victim,
                int maxSteps = 200) {
    for (int step = 0; step < maxSteps; ++step) {
        if (battle.phase() != BattlePhase::Ongoing) return;
        const int target = indexOf(battle, victim);
        if (target < 0 || !battle.units()[static_cast<std::size_t>(target)].alive()) return;

        const int actor = battle.currentActor();
        if (actor < 0) {
            battle.endTurn();
            continue;
        }
        if (battle.units()[static_cast<std::size_t>(actor)].name != attacker) {
            battle.endTurn();   // 别人的回合直接过掉：本文件测的是波次，不是 AI
            continue;
        }
        Action hit;
        hit.kind = ActionKind::Attack;
        hit.actorIndex = actor;
        hit.targetIndex = target;
        static_cast<void>(battle.apply(hit));
        battle.endTurn();
    }
}

// 一场两波的仗：我方一人，第 0 波一个敌人，第 1 波两个敌人。
BattleState twoWaveBattle() {
    std::vector<Unit> units;
    units.push_back(makeUnit("韩立", /*ally=*/true, /*wave=*/0, /*hp=*/200));
    units.push_back(makeUnit("头阵", /*ally=*/false, /*wave=*/0, /*hp=*/40));
    units.push_back(makeUnit("二波甲", /*ally=*/false, /*wave=*/1, /*hp=*/40));
    units.push_back(makeUnit("二波乙", /*ally=*/false, /*wave=*/1, /*hp=*/40));

    BattleState battle;
    battle.setup(std::move(units), 4040u);
    return battle;
}

// ---------------------------------------------------------------------------
// 一、开场：后面几波不在场
// ---------------------------------------------------------------------------

TEST(Ch04Wave, LaterWavesAreNotOnTheFieldWhenTheFightStarts) {
    BattleState battle = twoWaveBattle();

    EXPECT_EQ(battle.waveCount(), 2) << "编成里最大波次是 1，所以共两波";
    EXPECT_EQ(battle.currentWave(), 0);
    EXPECT_EQ(battle.phase(), BattlePhase::Ongoing);

    EXPECT_TRUE(unitNamed(battle, "头阵").alive());
    // 不在场的三件事一次钉全：alive() 说不在、onField 说不在、行动序里也没有。
    // 只验其中一件的话，另外两件各自漏掉都不会有人喊。
    //（从前第三件是「格子上没人」；横版没有格子，换成行动序与下一回合的预览。）
    EXPECT_FALSE(unitNamed(battle, "二波甲").alive());
    EXPECT_FALSE(unitNamed(battle, "二波甲").onField);
    const int later = indexOf(battle, "二波甲");
    const std::vector<int>& order = battle.roundOrder();
    EXPECT_EQ(std::count(order.begin(), order.end(), later), 0) << "第二波的单位不该排进行动序";
    const std::vector<int> preview = battle.nextRoundOrder();
    EXPECT_EQ(std::count(preview.begin(), preview.end(), later), 0) << "下一回合的预览里也不该有它";
    EXPECT_EQ(std::count(order.begin(), order.end(), indexOf(battle, "头阵")), 1);
}

TEST(Ch04Wave, AUnitThatHasNotComeInYetCannotBeTargetedAndTheRefusalSaysWhy) {
    BattleState battle = twoWaveBattle();
    const int actor = battle.currentActor();
    ASSERT_GE(actor, 0);
    ASSERT_EQ(battle.units()[static_cast<std::size_t>(actor)].name, "韩立");

    Action hit;
    hit.kind = ActionKind::Attack;
    hit.actorIndex = actor;
    hit.targetIndex = indexOf(battle, "二波甲");
    std::string why;
    EXPECT_FALSE(battle.isLegal(hit, &why));
    // 先验理由真有内容，再验它说的是「还没来」而不是「已经倒下」——两者玩家
    // 该做的事完全不同，一个是打完了，一个是还没开始打。
    ASSERT_FALSE(why.empty());
    EXPECT_NE(why.find("尚未入场"), std::string::npos) << why;

    // 对照：同一个动作打第一波那个，是合法的。少了这一条，上面那句可能只是
    // 因为这个 Action 本来就哪儿都不对（坐标、轮次、距离……）。
    hit.targetIndex = indexOf(battle, "头阵");
    EXPECT_TRUE(battle.isLegal(hit, &why)) << why;
}

// ---------------------------------------------------------------------------
// 二、推波，以及「不回血、不重置」
// ---------------------------------------------------------------------------

TEST(Ch04Wave, ClearingTheFirstWaveBringsInTheSecondInsteadOfWinning) {
    BattleState battle = twoWaveBattle();
    strikeDown(battle, "韩立", "头阵");

    ASSERT_FALSE(unitNamed(battle, "头阵").alive()) << "先验：第一波确实被打完了";
    EXPECT_EQ(battle.phase(), BattlePhase::Ongoing) << "还有一波没上，这时候就判赢是错的";
    EXPECT_EQ(battle.currentWave(), 1);
    EXPECT_TRUE(unitNamed(battle, "二波甲").alive());
    EXPECT_TRUE(unitNamed(battle, "二波乙").alive());
}

TEST(Ch04Wave, TheNewWaveIsAnnouncedInTheBattleLog) {
    // 契约第 2.3 节：玩家正打着忽然多出两个敌人而界面上一个字也不提，
    // 是本项目明令禁止的那类观感。
    BattleState battle = twoWaveBattle();
    const std::size_t linesBefore = battle.log().size();
    strikeDown(battle, "韩立", "头阵");

    ASSERT_GT(battle.log().size(), linesBefore);
    EXPECT_TRUE(logHas(battle, "第 2 波")) << "日志里没有一句交代第二波来了";
    // 报的是谁来了，不是一句笼统的「敌人增援」：玩家得知道自己面对的是什么。
    EXPECT_TRUE(logHas(battle, "二波甲"));
    EXPECT_TRUE(logHas(battle, "二波乙"));
}

TEST(Ch04Wave, NobodyIsHealedOrResetBetweenWaves) {
    // **本文件最要紧的一条。** 契约第 2.1 节：波与波之间不回血、不重置，
    // 「那是攻防战的全部张力」。
    //
    // 一步一步手推而不用 strikeDown：头阵每一回合都会再砍韩立一刀，混着跑的话
    // 「转场后 hp 没变」根本对不上——要钉的那一刻是**杀死最后一个敌人的那一击
    // 前后**，不是整场的头尾。
    std::vector<Unit> units;
    units.push_back(makeUnit("韩立", /*ally=*/true, /*wave=*/0, /*hp=*/200));
    units.push_back(makeUnit("头阵", /*ally=*/false, /*wave=*/0, /*hp=*/40));
    units.push_back(makeUnit("二波甲", /*ally=*/false, /*wave=*/1, /*hp=*/40));
    units[0].attack = 40;   // 一刀带走头阵，转场前后就只隔着这一击

    BattleState battle;
    battle.setup(std::move(units), 4045u);

    const int hero = indexOf(battle, "韩立");
    const int foe = indexOf(battle, "头阵");
    ASSERT_GE(hero, 0);
    ASSERT_GE(foe, 0);

    // 第一回合：韩立什么也不做，让头阵砍他一刀。
    ASSERT_EQ(battle.currentActor(), hero) << "身法 9 对 1，韩立先动";
    battle.endTurn();
    ASSERT_EQ(battle.currentActor(), foe);
    {
        Action hit;
        hit.kind = ActionKind::Attack;
        hit.actorIndex = foe;
        hit.targetIndex = hero;
        const auto hitResult = battle.apply(hit);
        ASSERT_TRUE(hitResult.ok) << hitResult.error;
    }
    battle.endTurn();   // 本回合走完
    battle.endTurn();   // 开新回合

    const int hpBefore = battle.units()[static_cast<std::size_t>(hero)].hp;
    const int maxHp = battle.units()[static_cast<std::size_t>(hero)].maxHp;
    const int roundBefore = battle.round();
    // **先验：分母不是 0。** 他确实带着伤，否则「转场后 hp 没变」在满血局面下
    // 与「转场把所有人回满了血」是同一个结果，这条断言就什么也没钉住。
    ASSERT_LT(hpBefore, maxHp) << "先验：转场之前韩立确实带着伤";
    ASSERT_GT(hpBefore, 0);
    ASSERT_EQ(battle.currentWave(), 0) << "先验：这时候还没推过波";

    // 就是这一击把第一波清空、把第二波带上来。
    ASSERT_EQ(battle.currentActor(), hero);
    Action kill;
    kill.kind = ActionKind::Attack;
    kill.actorIndex = hero;
    kill.targetIndex = foe;
    const auto killResult = battle.apply(kill);
    ASSERT_TRUE(killResult.ok) << killResult.error;
    ASSERT_FALSE(battle.units()[static_cast<std::size_t>(foe)].alive());
    ASSERT_EQ(battle.currentWave(), 1) << "先验：确实推波了";

    const Unit& after = battle.units()[static_cast<std::size_t>(hero)];
    EXPECT_EQ(after.hp, hpBefore) << "波间回血了——攻防战的张力就没了";
    EXPECT_LT(after.hp, after.maxHp) << "同上，换个说法钉一遍：他仍然是带伤的";
    EXPECT_EQ(after.maxHp, maxHp) << "上限也不该被重置";
    // 回合数不重来：重置回合等于把「打了多久」这件事抹掉，而它是战斗日志与
    // 一切按回合走的效果（毒、劲、破势）的共同刻度。
    EXPECT_EQ(battle.round(), roundBefore) << "波次转场把回合数拨动了";
    EXPECT_GT(roundBefore, 1) << "先验：这时候确实已经打过一个回合了";
}

TEST(Ch04Wave, PoisonAndBpCarryAcrossTheWaveBoundary) {
    // 「不重置状态」的另一半：毒与劲都留着（契约第 2.1 节原话是「毒、罡气、位置」；
    // 横版里罡气换成了防御、位置不复存在，留下来要验的是毒与劲）。
    // 与上一条分开写，是因为它们会因完全不同的实现失误而各自失效。
    std::vector<Unit> units;
    units.push_back(makeUnit("韩立", /*ally=*/true, /*wave=*/0, /*hp=*/200));
    units.push_back(makeUnit("头阵", /*ally=*/false, /*wave=*/0, /*hp=*/40));
    units.push_back(makeUnit("二波甲", /*ally=*/false, /*wave=*/1, /*hp=*/40));
    units[0].poison = 20;   // 给足余量：转场前要打好几个回合，毒每回合递减
    units[0].poisonPower = 3;

    BattleState battle;
    battle.setup(std::move(units), 4041u);

    const int hero = indexOf(battle, "韩立");
    ASSERT_GE(hero, 0);
    ASSERT_GT(battle.units()[static_cast<std::size_t>(hero)].poison, 0) << "先验：他确实中着毒";

    const int poisonBeforeWave = battle.units()[static_cast<std::size_t>(hero)].poison;
    strikeDown(battle, "韩立", "头阵");
    ASSERT_EQ(battle.currentWave(), 1) << "先验：确实推波了";

    const Unit& after = battle.units()[static_cast<std::size_t>(hero)];
    // 毒只会因为回合推进而递减，绝不会因为换了一波就被清掉。
    EXPECT_GT(after.poison, 0) << "波次转场把毒清了";
    EXPECT_LE(after.poison, poisonBeforeWave) << "毒只该随回合递减，不该反涨";
    EXPECT_EQ(after.poisonPower, 3) << "毒的强度不该被重置";
    // 劲：头阵 40 血、韩立一下 39（攻 20×2−1），两个回合才砍得倒，于是转场时他的劲
    // 已经攒过一回合；转场不该把它打回开场的 1 点。
    EXPECT_GT(battle.round(), 1) << "先验：转场之前确实过了至少一个回合";
    EXPECT_GT(after.bp, 1) << "波次转场把劲清回了开场的 1 点";
}

TEST(Ch04Wave, OnlyTheLastWaveEndsTheFight) {
    BattleState battle = twoWaveBattle();
    strikeDown(battle, "韩立", "头阵");
    ASSERT_EQ(battle.phase(), BattlePhase::Ongoing);

    strikeDown(battle, "韩立", "二波甲");
    EXPECT_EQ(battle.phase(), BattlePhase::Ongoing) << "同一波里还剩一个，不该判赢";
    strikeDown(battle, "韩立", "二波乙");
    EXPECT_EQ(battle.phase(), BattlePhase::Won) << "最后一波打完才算赢";
    EXPECT_EQ(battle.currentWave(), battle.waveCount() - 1);
}

TEST(Ch04Wave, LosingMidwayIsStillALossEvenWithWavesLeft) {
    // 契约第 2.1 节：中途全灭即 Lost。推波那段写在胜负判定之前，所以要专门验
    // 一下它没有把「我方全灭」这条也顺手推没了。
    std::vector<Unit> units;
    units.push_back(makeUnit("韩立", /*ally=*/true, /*wave=*/0, /*hp=*/10));
    units.push_back(makeUnit("头阵", /*ally=*/false, /*wave=*/0, /*hp=*/200));
    units.push_back(makeUnit("二波甲", /*ally=*/false, /*wave=*/1, /*hp=*/40));
    units[1].speed = 20;   // 敌人先动
    units[1].attack = 50;

    BattleState battle;
    battle.setup(std::move(units), 4042u);
    strikeDown(battle, "头阵", "韩立");

    EXPECT_EQ(battle.phase(), BattlePhase::Lost);
    EXPECT_EQ(battle.currentWave(), 0) << "我方全灭时不该还去推下一波";
    EXPECT_FALSE(unitNamed(battle, "二波甲").alive());
}

TEST(Ch04Wave, TheNewWaveActsFromTheFollowingRound) {
    // 行动序是回合开始时按当时在场的人排的，中途进场的排不进去——他们刚走上
    // 战场。这一条钉住「下一回合他们真的动得了」，免得实现成永远排不进去。
    BattleState battle = twoWaveBattle();
    strikeDown(battle, "韩立", "头阵");
    ASSERT_EQ(battle.currentWave(), 1);

    bool acted = false;
    for (int step = 0; step < 64 && !acted; ++step) {
        const int actor = battle.currentActor();
        if (actor < 0) {
            battle.endTurn();
            continue;
        }
        if (battle.units()[static_cast<std::size_t>(actor)].name == "二波甲") acted = true;
        battle.endTurn();
    }
    EXPECT_TRUE(acted) << "第二波进了场却永远轮不到它行动";
}

// 从前这里还有一条 AWaveThatWouldLandOnAnOccupiedCellIsNudgedAside（新一波落在有人的格子上
// 要挪开一格）。横版没有格子，「两个单位叠在同一格」这件事不复存在，那一条连同推波时的
// 找空格一起删了。

// ---------------------------------------------------------------------------
// 三、界面上说得出来
// ---------------------------------------------------------------------------

TEST(Ch04Wave, TheWaveLineOnScreenCarriesBothNumbersAndIsBlankForSingleWaveFights) {
    BattleState multi = twoWaveBattle();
    const std::string shown = waveStatusText(multi);
    // 先验它真有内容，再验两个数都在里面（只说「第二波」的话，玩家不知道还剩几波）。
    ASSERT_FALSE(shown.empty());
    EXPECT_NE(shown.find("1"), std::string::npos) << shown;
    EXPECT_NE(shown.find("2"), std::string::npos) << shown;

    // 单波战斗一个像素也不该多：老编成全是这一档。
    std::vector<Unit> units;
    units.push_back(makeUnit("韩立", true, 0));
    units.push_back(makeUnit("头阵", false, 0));
    BattleState single;
    single.setup(std::move(units), 4044u);
    EXPECT_TRUE(waveStatusText(single).empty());
}

// ---------------------------------------------------------------------------
// 四、老数据一字不改仍按单波跑
// ---------------------------------------------------------------------------

std::string projectRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "data" / "battles" / "b03_gu_wai_elang.json")) {
            return candidate;
        }
    }
    return ".";
}

TEST(Ch04WaveLoading, ABattleThatDeclaresNoWavesLoadsAsASingleWaveFight) {
    // 契约第 2.2 节：「老的战斗数据必须一字不改地继续按单波跑」。
    //
    // 这条从前写的是「**每一场**战斗的每一个单位波次都是 0」——
    // 那不是契约，那是一次**点名**：它把「今天仓库里有几场多波次」
    // 当成了规格。第 4 章的门派攻防战本就是多波次的，
    // 于是它恰恰因为那个功能真的做成了而转红。
    //
    // 现在问的是规则：**没声明过波次的那些，全部得是单波。**
    // 仓库里现有 22 场落在这一类里，够它查的。
    const auto loaded = fanren::io::loadBattles(projectRoot() + "/data/battles");
    ASSERT_TRUE(loaded.ok) << loaded.error;
    ASSERT_GE(loaded.value.size(), 10u)
        << "只读到 " << loaded.value.size() << " 场，夹具不对";

    int singleWave = 0;
    int multiWave = 0;
    for (const auto& entry : loaded.value) {
        int highest = 0;
        for (const fanren::core::BattleUnitSpec& unit : entry.second.units) {
            EXPECT_GE(unit.wave, 0) << entry.first << " 的单位 " << unit.roleId;
            highest = std::max(highest, unit.wave);
        }
        if (highest == 0) {
            ++singleWave;
            continue;
        }
        ++multiWave;
        // 波次号必须从 0 起连续无空档（加载器自己的规矩）。
        // 拿仓库里真的数据查，而不是只拿夹具查。
        std::vector<bool> seen(static_cast<std::size_t>(highest) + 1, false);
        for (const fanren::core::BattleUnitSpec& unit : entry.second.units) {
            seen[static_cast<std::size_t>(unit.wave)] = true;
        }
        for (std::size_t w = 0; w < seen.size(); ++w) {
            EXPECT_TRUE(seen[w]) << entry.first << " 的波次号缺了 " << w;
        }
    }

    // 两侧都得有东西，否则上面那两条循环里至少有一条是空转的
    //（README 那张表：分母塌成 0）。
    EXPECT_GT(singleWave, 0) << "一场单波战斗都没有，「老数据不变」无从查起";
    EXPECT_GT(multiWave, 0) << "一场多波次都没有，连续性那一段从来没跑过";
}

TEST(Ch04WaveLoading, ABattleWithoutWavesReportsExactlyOneWaveWhenItRuns) {
    // 上一条看的是数据，这一条看的是**跑起来**：同一件事换一条完全
    // 不同的路径验（README：形状要与主测试不同）。
    //
    // **样本按性质挑，不写死名字。** 从前写的是
    // b04_yelangbang_laifan——而那恰恰是野狼帮来犯那一场，
    // 从设计上就注定要变成多波次。拿一个注定要变的东西当
    // 「不变的那一个」的例子，只是把红灯往后拖。
    const auto loaded = fanren::io::loadBattles(projectRoot() + "/data/battles");
    ASSERT_TRUE(loaded.ok) << loaded.error;

    std::string pick;
    for (const auto& entry : loaded.value) {
        bool flat = true;
        for (const fanren::core::BattleUnitSpec& unit : entry.second.units) {
            if (unit.wave != 0) { flat = false; break; }
        }
        if (flat && !entry.second.units.empty()) { pick = entry.first; break; }
    }
    ASSERT_FALSE(pick.empty()) << "找不到一场没声明波次的战斗";

    fanren::game::Application app;
    auto ready = app.init(projectRoot(), /*headless=*/true);
    ASSERT_TRUE(ready.ok) << ready.error;

    BattleScene scene(pick);
    scene.onEnter(app);
    EXPECT_EQ(scene.battle().waveCount(), 1) << pick << " 没声明波次，却不是单波";
    EXPECT_EQ(scene.battle().currentWave(), 0);
    EXPECT_TRUE(waveStatusText(scene.battle()).empty())
        << "单波战斗界面上不该多出波次那一行";
}

TEST(Ch04WaveLoading, AShippedMultiWaveBattleReallyReportsItsWaves) {
    // 对照组，而且是要紧的那一半：上面两条全在查「单波的仍然是单波」，
    // 而**波次压根儿没做成**的话它们一样全绿。
    const auto loaded = fanren::io::loadBattles(projectRoot() + "/data/battles");
    ASSERT_TRUE(loaded.ok) << loaded.error;

    std::string pick;
    int declared = 0;
    for (const auto& entry : loaded.value) {
        int highest = 0;
        for (const fanren::core::BattleUnitSpec& unit : entry.second.units) {
            highest = std::max(highest, unit.wave);
        }
        if (highest > 0) { pick = entry.first; declared = highest + 1; break; }
    }
    ASSERT_FALSE(pick.empty()) << "仓库里一场多波次战斗也没有";

    fanren::game::Application app;
    auto ready = app.init(projectRoot(), /*headless=*/true);
    ASSERT_TRUE(ready.ok) << ready.error;

    BattleScene scene(pick);
    scene.onEnter(app);
    EXPECT_EQ(scene.battle().waveCount(), declared)
        << pick << " 声明了 " << declared << " 波，跑起来却不是";
    EXPECT_EQ(scene.battle().currentWave(), 0) << "开场必须在第一波";
    EXPECT_FALSE(waveStatusText(scene.battle()).empty())
        << "多波次战斗界面上必须看得见打到哪一波了";
}
// 写一份只有 units 不同的战斗文件，走**真的加载器**读回来。
//
// 下面那几条负向用例靠它吃饭：波次号的校验就在加载器里，
// 自己另搭一个解析器等于测自己写的那个。
[[nodiscard]] fanren::core::Result<fanren::core::BattleSetup> loadProbe(
        const std::string& unitsJson) {
    static int counter = 0;
    fanren::test::TempDir tmp{"fanren_ch04_wave"};
    const fs::path file =
        tmp.path() / ("probe_" + std::to_string(++counter) + ".json");
    std::ofstream out(file, std::ios::binary);
    out << R"({"id":"b_probe","name":"探针","chapter":4,"terrain":"field",)"
        << R"("can_escape":true,"defeat_is_fatal":false,"units":)" << unitsJson
        << R"(,"rewards":{"cultivation":0,"spirit_stones":0,"drops":[]}})";
    out.close();
    return fanren::io::loadBattle(file.string());
}
TEST(Ch04WaveLoading, AUnitWithoutAWaveFieldLandsOnWaveZero) {
    const auto loaded = loadProbe(R"([{"role_id":"wild_wolf"}])");
    ASSERT_TRUE(loaded.ok) << loaded.error;
    ASSERT_EQ(loaded.value.units.size(), 1u);
    EXPECT_EQ(loaded.value.units[0].wave, 0) << "缺字段按 0 收，老数据一行不改照读";
}

TEST(Ch04WaveLoading, DataOverridesTheDefault) {
    const auto loaded =
        loadProbe(R"([{"role_id":"wild_wolf"},)"
                  R"({"role_id":"wild_wolf","wave":1}])");
    ASSERT_TRUE(loaded.ok) << loaded.error;
    ASSERT_EQ(loaded.value.units.size(), 2u);
    EXPECT_EQ(loaded.value.units[0].wave, 0);
    EXPECT_EQ(loaded.value.units[1].wave, 1);
}

TEST(Ch04WaveLoading, ANegativeWaveIsRefusedAtLoadTime) {
    // 先验：同一份夹具只把 wave 换成合法值就能读进来。少了这一句，下面那条
    // EXPECT_FALSE 可能只是因为夹具本身写坏了——而那种绿灯在校验被整段删掉
    // 之后照样是绿的。
    {
        const auto sane = loadProbe(R"([{"role_id":"wild_wolf","wave":0}])");
        ASSERT_TRUE(sane.ok) << "夹具本身要是读不进来，下面那条就什么也没证明: " << sane.error;
    }

    const auto loaded = loadProbe(R"([{"role_id":"wild_wolf","wave":-1}])");
    EXPECT_FALSE(loaded.ok) << "负波次必须当场报错，不能夹到 0 悄悄收下";
    ASSERT_FALSE(loaded.error.empty());
    EXPECT_NE(loaded.error.find("wave"), std::string::npos) << loaded.error;
}

TEST(Ch04WaveLoading, ANonIntegerWaveIsRefusedAtLoadTime) {
    const auto loaded = loadProbe(R"([{"role_id":"wild_wolf","wave":"第二波"}])");
    EXPECT_FALSE(loaded.ok);
    ASSERT_FALSE(loaded.error.empty());
    EXPECT_NE(loaded.error.find("wave"), std::string::npos) << loaded.error;
}

TEST(Ch04WaveLoading, AGapInTheWaveNumbersIsRefused) {
    // 拦的是 "wave": 2 这种笔误（本想写第二波，而波次从 0 起算）。
    // 引擎容得下空档，但那意味着战斗中途凭空拖一拍，玩家看到的是「打完了却没结束」。
    const auto loaded =
        loadProbe(R"([{"role_id":"wild_wolf"},)"
                  R"({"role_id":"wild_wolf","wave":2}])");
    EXPECT_FALSE(loaded.ok);
    ASSERT_FALSE(loaded.error.empty());
    EXPECT_NE(loaded.error.find("不连续"), std::string::npos) << loaded.error;

    // 对照：补上中间那一波就读得进来。证明这道闸拦的是空档，不是「有波次就报错」。
    const auto filled =
        loadProbe(R"([{"role_id":"wild_wolf"},)"
                  R"({"role_id":"wild_wolf","wave":1},)"
                  R"({"role_id":"wild_wolf","wave":2}])");
    EXPECT_TRUE(filled.ok) << filled.error;
}

TEST(Ch04WaveLoading, ABattleWithNoWaveZeroIsRefused) {
    const auto loaded = loadProbe(R"([{"role_id":"wild_wolf","wave":1}])");
    EXPECT_FALSE(loaded.ok) << "开场一个人也没有的编成第一件事就是推波，那与直接写成第 0 波是同一场仗";
    ASSERT_FALSE(loaded.error.empty());
}

}  // namespace
