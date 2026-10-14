// 第 11 章验收第 2 条：两侧从真实第 10 章终档起步、走同一组意图选项通关；终点照字面量断言；写 / 比终档。
// 另含：这几条检查各自的「故意写坏 → 确实被抓住」，与平衡路的四场判据（8.1 意图玩家、BattleHand）。
#include <gtest/gtest-spi.h>
#include <tuple>
#include "Ch11Driver.h"

namespace {
using namespace fanren;
using namespace fanren::test::ch11;

// 在 body 里跑一段检查，把它报的失败收起来（不记到当前用例上），返回失败条数与全文。
template <typename Body>
std::pair<int, std::string> capturedFailures(Body&& body) {
    ::testing::TestPartResultArray results;
    {
        ::testing::ScopedFakeTestPartResultReporter reporter(
            ::testing::ScopedFakeTestPartResultReporter::INTERCEPT_ONLY_CURRENT_THREAD, &results);
        body();
    }
    std::string text;
    for (int i = 0; i < results.size(); ++i) text += std::string(results.GetTestPartResult(i).message()) + "\n";
    return {results.size(), text};
}

// 平衡目标（施工图第 8 节、派工单）：① 意图三到五回合；② 三回合内逃得出；③ 四回合内清场；
// ④ 韩立与曲魂只防、只靠友军两门法术（蛛网 水、紫火 火）也能把它气血打到 0。
bool lureOnTarget(const Fight& f) {
    return f.id == kBattles[0] && f.phase == BattlePhase::Won && f.rounds >= 3 && f.rounds <= 5;
}
bool jiguanOnTarget(const Fight& f) {
    return f.id == kBattles[1] && f.mode == FightMode::Flee && f.phase == BattlePhase::Escaped && f.rounds <= 3;
}
bool shayaoOnTarget(const Fight& f) {
    return f.id == kBattles[2] && f.phase == BattlePhase::Won && f.rounds <= 4 && f.enemyHpLeft == 0;
}
bool tianduOnTarget(const Fight& f) {
    return f.id == kBattles[3] && f.mode == FightMode::AlliesOnly && f.phase == BattlePhase::Won && f.enemyHpLeft == 0;
}

void verifyRun(Driver& driver, const Side& side) {
    driver.checkEnding(side);
    ASSERT_EQ(driver.fights().size(), 7u);
    for (std::size_t i = 0; i < 4; ++i) EXPECT_TRUE(lureOnTarget(driver.fights()[i])) << i << " rounds " << driver.fights()[i].rounds;
    EXPECT_TRUE(jiguanOnTarget(driver.fights()[4])) << driver.fights()[4].rounds;
    EXPECT_TRUE(shayaoOnTarget(driver.fights()[5])) << driver.fights()[5].rounds;
    const std::vector<std::string> party{"hanli", "qu_hun_shadan"};
    for (std::size_t i = 0; i < 6; ++i) EXPECT_EQ(driver.fights()[i].allies, party) << driver.fights()[i].id;
    // ④：编成里写死的四友军排在韩立之后、队员曲魂之前（BattleScene 先摆编成、再 addPartyUnits）。
    EXPECT_EQ(driver.fights()[6].allies, (std::vector<std::string>{"hanli", "xueyu_zhizhu_ally",
        "xueyu_zhizhu_ally", "kuilei_yuan", "kuilei_yuan", "qu_hun_shadan"}));
    // 验收第 21 条：第 12 章进了 data/chapters.json 之后，ch11.done 排「第十一章　终」再接 Opening(12)
    // （第 12 章契约 5.3）；「未完待续」只跟表里最后一章。
    const auto& cards = driver.app().cardLog();
    ASSERT_GE(cards.size(), 2u);
    EXPECT_EQ(cards[cards.size() - 2], (game::CardRequest{game::CardKind::Closing, 11}));
    EXPECT_EQ(cards.back(), (game::CardRequest{game::CardKind::Opening, 12}));
    ASSERT_FALSE(::testing::Test::HasFailure());
    const test::TempDir temporary("fanren_ch11_actual_ending");
    const auto path = temporary.path() / "ending.sav";
    const auto saved = io::saveGame(driver.state(), path.string());
    ASSERT_TRUE(saved.ok) << saved.error;
    const auto loaded = io::loadGame(path.string());
    ASSERT_TRUE(loaded.ok) << loaded.error;
    EXPECT_EQ(test::comparableSaveLines(loaded.value), test::comparableSaveLines(driver.state()));
    Application resumed;
    const auto ready = resumed.init(root().string(), true);
    ASSERT_TRUE(ready.ok) << ready.error;
    const auto continued = resumed.continueJourney(path.string());
    ASSERT_TRUE(continued.ok) << continued.error;
    const auto drift = test::saveLineDifferences(test::comparableSaveLines(driver.state()),
                                                 test::comparableSaveLines(resumed.state()));
    EXPECT_TRUE(drift.empty()) << test::joinLines(drift);
    resumed.shutdown();
    ASSERT_FALSE(::testing::Test::HasFailure());
    const auto verdict = test::settleAgainstFixture(driver.state(), root().string(), side.ending,
        test::kWriteChapterElevenFixturesEnv, "Chapter 12");
    EXPECT_TRUE(verdict.problem.empty()) << verdict.problem;
}

TEST(Ch11Walkthrough, FirstSideUsesTheActualC10EndingAndTheIntentChoices) {
    Driver driver;
    ASSERT_TRUE(driver.load(kFirst)) << driver.problem();
    ASSERT_TRUE(driver.runTo(31)) << driver.problem();
    verifyRun(driver, kFirst);
}

TEST(Ch11Walkthrough, SecondSideUsesTheActualC10EndingAndTheSameChoices) {
    Driver driver;
    ASSERT_TRUE(driver.load(kSecond)) << driver.problem();
    ASSERT_TRUE(driver.runTo(31)) << driver.problem();
    verifyRun(driver, kSecond);
}

// 验收第 2 条与第 15 条末句：中阶两侧恰 20，低阶两侧差恰 20（= 起点差），day 差两侧都恰 33375。
TEST(Ch11Acceptance, BothSidesEndOnTwentyMidStonesAndKeepTheTwentyLowStoneGap) {
    std::array<int, 2> low{}, mid{}, days{};
    std::size_t index = 0;
    for (const Side* side : {&kFirst, &kSecond}) {
        Driver driver;
        ASSERT_TRUE(driver.load(*side)) << driver.problem();
        ASSERT_TRUE(driver.runTo(31)) << driver.problem();
        low[index] = driver.state().itemCount("material_lingshi");
        mid[index] = driver.state().itemCount("material_lingshi_zhong");
        days[index] = driver.state().day - side->day;
        ++index;
    }
    EXPECT_EQ(mid[0], 20);
    EXPECT_EQ(mid[1], 20);
    EXPECT_EQ(low[0] - low[1], 20);
    EXPECT_EQ(days[0], 33375);
    EXPECT_EQ(days[1], 33375);
}

// ---- 平衡：四场从两侧真实走到的那一刻起打（不手搭起点）----
struct Checkpoint {
    GameState state;
    int node;
    const char* hook;
    bool enter;
};

// ① 红瑚岛布阵之后、② 四胜之后、③ 赴约登岛之前、④ 乌丑之后——都是驱动真走到的存档。
std::vector<Checkpoint> checkpoints(const Side& side, std::string& problem) {
    Driver driver;
    std::vector<Checkpoint> out;
    if (!driver.load(side)) { problem = driver.problem(); return out; }
    const std::array<std::tuple<int, const char*, bool>, 4> stops{{
        {11, "trigger_youyao", false}, {12, "trigger_youyao", false}, {21, "trigger_guiju", true}, {23, "trigger_daoge", true}}};
    for (const auto& [node, hook, enter] : stops) {
        if (!driver.runTo(node)) { problem = driver.problem(); return {}; }
        out.push_back({driver.state(), node + 1, hook, enter});
    }
    return out;
}

Fight replay(const Checkpoint& at, FightMode mode, test::HandPolicy policy = intentPolicy()) {
    Driver driver;
    driver.policy = std::move(policy);
    EXPECT_TRUE(driver.resumeReached(at.state, at.node)) << driver.problem();
    driver.nextModes = {mode};
    // 败致命的场次会走到 game_over：脚本不正常收尾，但这一场的记录照样在。
    static_cast<void>(at.enter ? driver.enter(at.hook) : driver.press(at.hook));
    if (driver.fights().empty()) {
        ADD_FAILURE() << "No fight from " << at.hook << ": " << driver.problem();
        return {};
    }
    const Fight& f = driver.fights().back();
    std::cout << "[ch11 balance] " << f.id << " mode=" << static_cast<int>(mode) << " phase=" << static_cast<int>(f.phase)
              << " rounds=" << f.rounds << " enemyHp=" << f.enemyHpLeft << " hero " << f.before.hp << "->" << f.after.hp
              << " mp " << f.before.mp << "->" << f.after.mp << " pills "
              << f.before.itemCount("pill_yangjing_dan") + f.before.itemCount("pill_jinchuang_yao") -
                 f.after.itemCount("pill_yangjing_dan") - f.after.itemCount("pill_jinchuang_yao") << '\n';
    return f;
}

constexpr std::array<FightMode, 4> kIntentModes{FightMode::Intent, FightMode::Flee, FightMode::Intent, FightMode::Intent};

// 先证那只手稳：同一份真实存档、同一场仗打三遍，结局指纹（回合、胜负、出手记录、战后存档）一字不差。
TEST(Ch11Balance, TheHandIsSteadyThreeTimesOnEachOfTheFourFromBothSides) {
    for (const Side* side : {&kFirst, &kSecond}) {
        std::string problem;
        const auto stops = checkpoints(*side, problem);
        ASSERT_EQ(stops.size(), 4u) << problem;
        for (std::size_t i = 0; i < stops.size(); ++i) {
            const auto first = fightFingerprint(replay(stops[i], kIntentModes[i]));
            ASSERT_GT(first.size(), 20u);
            for (int again = 0; again < 2; ++again)
                EXPECT_EQ(fightFingerprint(replay(stops[i], kIntentModes[i])), first) << side->file << ' ' << kBattles[i];
        }
    }
}

TEST(Ch11Balance, TheIntentPlayerMeetsTheFourTargetsFromBothActualCheckpoints) {
    for (const Side* side : {&kFirst, &kSecond}) {
        SCOPED_TRACE(side->file);
        std::string problem;
        const auto stops = checkpoints(*side, problem);
        ASSERT_EQ(stops.size(), 4u) << problem;
        EXPECT_TRUE(lureOnTarget(replay(stops[0], FightMode::Intent)));
        EXPECT_TRUE(jiguanOnTarget(replay(stops[1], FightMode::Flee)));
        EXPECT_TRUE(shayaoOnTarget(replay(stops[2], FightMode::Intent)));
        EXPECT_EQ(replay(stops[3], FightMode::Intent).phase, BattlePhase::Won);
        EXPECT_TRUE(tianduOnTarget(replay(stops[3], FightMode::AlliesOnly)));
    }
}

// 负向：四条目标各自拒得住一场没达标的仗；④ 人人只防就输——上一条里的胜确实来自友军那两门法术。
TEST(Ch11Balance, EachTargetRejectsAFightThatMissesItAndNobodyCastingLosesTheCorpseFight) {
    std::string problem;
    const auto stops = checkpoints(kFirst, problem);
    ASSERT_EQ(stops.size(), 4u) << problem;
    const Fight lure = replay(stops[0], FightMode::Intent);
    const Fight jiguan = replay(stops[1], FightMode::Flee);
    const Fight shayao = replay(stops[2], FightMode::Intent);
    const Fight tiandu = replay(stops[3], FightMode::AlliesOnly);
    ASSERT_TRUE(lureOnTarget(lure) && jiguanOnTarget(jiguan) && shayaoOnTarget(shayao) && tianduOnTarget(tiandu));
    for (const int rounds : {2, 6}) { Fight bad = lure; bad.rounds = rounds; EXPECT_FALSE(lureOnTarget(bad)) << rounds; }
    { Fight bad = lure; bad.phase = BattlePhase::Escaped; EXPECT_FALSE(lureOnTarget(bad)); }
    { Fight bad = jiguan; bad.rounds = 4; EXPECT_FALSE(jiguanOnTarget(bad)); }
    { Fight bad = jiguan; bad.phase = BattlePhase::Won; EXPECT_FALSE(jiguanOnTarget(bad)); }
    { Fight bad = shayao; bad.rounds = 5; EXPECT_FALSE(shayaoOnTarget(bad)); }
    { Fight bad = tiandu; bad.mode = FightMode::Intent; EXPECT_FALSE(tianduOnTarget(bad)); }
    { Fight bad = tiandu; bad.enemyHpLeft = 1; EXPECT_FALSE(tianduOnTarget(bad)); }
    const Fight idle = replay(stops[3], FightMode::Defend);
    EXPECT_EQ(idle.phase, BattlePhase::Lost);
    EXPECT_FALSE(tianduOnTarget(idle));
    auto print = fightFingerprint(lure);
    print.back() += " ";
    EXPECT_NE(print, fightFingerprint(lure));
}

// ---- 终点与起点检查自己的负向 ----
TEST(Ch11Acceptance, TheStartGuardCatchesTheOtherSidesLiterals) {
    const Side swapped{kSecond.file, kSecond.ending, kFirst.day, kFirst.low, kFirst.mid};
    Driver driver;
    const auto [count, text] = capturedFailures([&] { static_cast<void>(driver.load(swapped)); });
    EXPECT_EQ(count, 3) << text;
    for (const char* literal : {"16003", "254", "117"}) EXPECT_NE(text.find(literal), std::string::npos) << literal;
}

TEST(Ch11Acceptance, TheEndingCatchesOneDayAfterTheShopClosedAndAThirdShopVisit) {
    Driver late;
    ASSERT_TRUE(late.load(kFirst)) << late.problem();
    ASSERT_TRUE(late.runTo(30) && late.sit(1) && late.runTo(31)) << late.problem();
    auto [count, text] = capturedFailures([&] { late.checkEnding(kFirst); });
    EXPECT_EQ(count, 1) << text;
    EXPECT_NE(text.find("33375"), std::string::npos) << text;
    // 开店后多坐 30 日、多回访一次：日子被第六根的补足吸掉（day 差照旧 33375），回访数与低阶对不上。
    Driver busy;
    ASSERT_TRUE(busy.load(kFirst)) << busy.problem();
    ASSERT_TRUE(busy.runTo(27) && busy.travel("ch11_dongfu") && busy.sit(30) && busy.visitShop() && busy.runTo(31))
        << busy.problem();
    std::tie(count, text) = capturedFailures([&] { busy.checkEnding(kFirst); });
    EXPECT_EQ(busy.state().day - busy.start().day, 33375);
    EXPECT_NE(text.find("ch11.jiesuan"), std::string::npos) << text;
    EXPECT_NE(text.find("material_lingshi"), std::string::npos) << text;
}

TEST(Ch11Acceptance, TheNodeLedgerCatchesAnExtraBambooAndAnExtraDay) {
    Driver driver;
    ASSERT_TRUE(driver.load(kFirst)) << driver.problem();
    ASSERT_TRUE(driver.runTo(26)) << driver.problem();
    Entry bamboo = driver.ledger().back();
    ASSERT_EQ(bamboo.node, 26);
    bamboo.after.addItem("material_jinleizhu", 1);
    auto [count, text] = capturedFailures([&] { driver.checkEntry(bamboo, driver.fights().size()); });
    EXPECT_EQ(count, 1) << text;
    EXPECT_NE(text.find("material_jinleizhu"), std::string::npos) << text;
    Entry day = driver.ledger().back();
    day.after.day += 1;
    std::tie(count, text) = capturedFailures([&] { driver.checkEntry(day, driver.fights().size()); });
    EXPECT_EQ(count, 1) << text;
}
} // namespace
