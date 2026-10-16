// 第 13 章验收第 2 条：两侧从真实第 12 章终档（T1 重生成版）起步、走同一组意图选项通关；终点照字面量断言；
// 写 / 比终档（FANREN_WRITE_CH13_FIXTURES=1 写）。另含：这几条检查各自的「故意写坏 → 确实被抓住」，
// 与平衡路的两场判据（施工图 8.1 意图玩家、8.2、验收 4「最低合法起点两场都打得过」、BattleHand）。
#include <gtest/gtest-spi.h>
#include <tuple>
#include "Ch13Driver.h"
#include "core/model/AttackCategory.h"

namespace {
using namespace fanren;
using namespace fanren::test::ch13;

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

// ---- 平衡目标（施工图 8.1、8.2；派工单「意图玩家不死」；18.5 第 1 条）----
// 共同：胜、敌方清场、韩立余血不低于 kHeroHpFloor、回合数落在下面的窗里。施工图没给回合数，窗照平衡路记录
//（notes/ch13/balance.md 第 0 节，协调者裁定）：① 10–22、② 6–18，是调后实测 16 / 13、10 各放宽约四成；
// 那只手先拿法术逐类试探破绽，回合数比真人多。「不险胜」直接写成判据：每场胜后韩立余血 ≥ 150（调后实测
// 312 / 343 / 283；调前 ① 19 回合余 75、② 第一轮 14 回合余 112，回合都在窗内，只有这条下限拒得住——
// 见 ThePreTuningRolesMissTheTargets…）。
// ① 大汉的单体蓄势至少被破势打断过一次（8.2「要在那一回合前把他打破势、或防御硬接」）；② 两波打完。
struct Target {
    int minRounds;
    int maxRounds;
};
constexpr std::array<Target, 2> kTargets{{{10, 22}, {6, 18}}};
constexpr int kHeroHpFloor = 150;

bool onTarget(const Fight& f, std::size_t which) {
    const Target t = kTargets.at(which);
    return f.id == kBattles.at(which) && f.phase == BattlePhase::Won && f.heroHpLeft >= kHeroHpFloor &&
           f.enemyHpLeft == 0 && f.rounds >= t.minRounds && f.rounds <= t.maxRounds;
}

// 三场（① 一次、② 两次）在 fights() 里的下标对应哪一张编成的目标。
constexpr std::array<std::size_t, 3> kFightTargets{0, 1, 1};

void verifyRun(Driver& driver, const Side& side) {
    driver.checkEnding(side);
    ASSERT_EQ(driver.fights().size(), 3u);
    for (std::size_t i = 0; i < 3; ++i)
        EXPECT_TRUE(onTarget(driver.fights()[i], kFightTargets[i]))
            << driver.fights()[i].id << " rounds " << driver.fights()[i].rounds << " heroHp "
            << driver.fights()[i].heroHpLeft;
    // 我方 ＝ 韩立一人（8.2：两场都无友军、队伍为空）。
    for (const auto& fight : driver.fights()) EXPECT_EQ(fight.allies, std::vector<std::string>{test::kHandHero});
    // 验收第 20 条（真实入口那一半）：ch13.done 排「第十三章　终」，第 13 章是章节表末章，接「未完待续」。
    const auto& cards = driver.app().cardLog();
    ASSERT_GE(cards.size(), 2u);
    EXPECT_EQ(cards[cards.size() - 2], (game::CardRequest{game::CardKind::Closing, 13}));
    EXPECT_EQ(cards.back(), (game::CardRequest{game::CardKind::ToBeContinued, 0}));
    ASSERT_FALSE(::testing::Test::HasFailure());
    const test::TempDir temporary("fanren_ch13_actual_ending");
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
    // continueJourney 不恢复朝向（技术债 G-31）：落图按 default spawn shiwu 的朝下。章末站位是洞口西侧、面朝东
    //（18.2 第 7 条），朝向这一行单独断，其余逐行比。
    EXPECT_EQ(resumed.state().facing, 2);
    GameState facingAligned = resumed.state();
    facingAligned.facing = driver.state().facing;
    const auto drift = test::saveLineDifferences(test::comparableSaveLines(driver.state()),
                                                 test::comparableSaveLines(facingAligned));
    EXPECT_TRUE(drift.empty()) << test::joinLines(drift);
    resumed.shutdown();
    ASSERT_FALSE(::testing::Test::HasFailure());
    const auto verdict = test::settleAgainstFixture(driver.state(), root().string(), side.ending,
        test::kWriteChapterThirteenFixturesEnv, "Chapter 14");
    EXPECT_TRUE(verdict.problem.empty()) << verdict.problem;
}

TEST(Ch13Walkthrough, FirstSideUsesTheActualC12EndingAndTheIntentChoices) {
    Driver driver;
    ASSERT_TRUE(driver.load(kFirst)) << driver.problem();
    ASSERT_TRUE(driver.runTo(34)) << driver.problem();
    verifyRun(driver, kFirst);
}

TEST(Ch13Walkthrough, SecondSideUsesTheActualC12EndingAndTheSameChoices) {
    Driver driver;
    ASSERT_TRUE(driver.load(kSecond)) << driver.problem();
    ASSERT_TRUE(driver.runTo(34)) << driver.problem();
    verifyRun(driver, kSecond);
}

// 验收第 2 条与第 14 条末句：两侧走同一组选项，章末低阶差、中阶差都等于起点差（20、0）；
// 两侧的 day 增量各是 11790 ＋ 360 × N（N 各写各的字面量）。
TEST(Ch13Acceptance, BothSidesKeepTheStartingStoneGapsAndTheirOwnRetreatYears) {
    const Walk& first = intentWalk(kFirst);
    const Walk& second = intentWalk(kSecond);
    ASSERT_TRUE(first.problem.empty()) << first.problem;
    ASSERT_TRUE(second.problem.empty()) << second.problem;
    EXPECT_EQ(first.end.itemCount(kLow) - second.end.itemCount(kLow), 2331 - 2311);
    EXPECT_EQ(first.end.itemCount(kMid) - second.end.itemCount(kMid), 20 - 20);
    EXPECT_EQ(first.end.day - kFirst.day, 11790 + 360 * kFirst.retreats);
    EXPECT_EQ(second.end.day - kSecond.day, 11790 + 360 * kSecond.retreats);
    EXPECT_EQ(first.retreats, kFirst.retreats);
    EXPECT_EQ(second.retreats, kSecond.retreats);
}

// ---- 平衡：两场从两侧真实走到的那一刻起打（不手搭起点）----
// which：0 ＝ ①（n04 走完，码头盘查之后、出海口带之前）；1 ＝ ② 第一轮（n23 走完，登礁布阵之后）；
// 2 ＝ ② 第二轮（同一处接着按：意图玩家第一轮照那只手打完、三日后再按）。perturb 是用例自己写明的扰动，
// 在第一轮之前施加（② 第二轮的扰动因此也管着第一轮）。
using Perturb = std::function<void(Driver&)>;

Fight replay(const Side& side, int which, FightMode mode, test::HandPolicy policy = intentPolicy(),
             const Perturb& perturb = nullptr) {
    const Walk& walk = intentWalk(side);
    const int node = which == 0 ? 5 : 24;
    Driver driver;
    driver.policy = std::move(policy);
    driver.audit = false;
    EXPECT_TRUE(driver.resumeReached(reachedAfter(walk, node - 1), node)) << driver.problem();
    if (perturb) perturb(driver);
    // 败致命的场次会走到 game_over：脚本不正常收尾，但这一场的记录照样在。
    if (which == 0) {
        driver.nextModes = {mode};
        static_cast<void>(driver.runTo(5));
    } else {
        if (which == 2) static_cast<void>(driver.press("trigger_youyao"));
        driver.nextModes = {mode};
        static_cast<void>(driver.press("trigger_youyao"));
    }
    const std::size_t expected = which == 2 ? 2u : 1u;
    if (driver.fights().size() != expected) {
        ADD_FAILURE() << "No fight " << which << " from node " << node << ": " << driver.problem();
        return {};
    }
    const Fight& f = driver.fights().back();
    std::cout << "[ch13 balance] " << side.file << " which=" << which << ' ' << f.id << " mode=" << static_cast<int>(mode)
              << " phase=" << static_cast<int>(f.phase) << " rounds=" << f.rounds << " enemyHp=" << f.enemyHpLeft
              << " hero " << f.before.hp << "->" << f.heroHpLeft << " interrupted=" << f.interrupted << '\n';
    return f;
}

// 先证那只手稳：同一份真实存档、同一场仗打三遍，结局指纹（回合、胜负、出手记录、战后存档）一字不差，
// 且与通关那一趟里的同一场一字不差。
TEST(Ch13Balance, TheHandIsSteadyThreeTimesOnEachOfTheThreeFightsFromBothSides) {
    for (const Side* side : {&kFirst, &kSecond}) {
        const Walk& walk = intentWalk(*side);
        ASSERT_TRUE(walk.problem.empty()) << walk.problem;
        ASSERT_EQ(walk.fights.size(), 3u);
        for (int i = 0; i < 3; ++i) {
            const auto first = fightFingerprint(replay(*side, i, FightMode::Intent));
            ASSERT_GT(first.size(), 20u);
            EXPECT_EQ(first, fightFingerprint(walk.fights[static_cast<std::size_t>(i)])) << side->file << ' ' << i;
            for (int again = 0; again < 2; ++again)
                EXPECT_EQ(fightFingerprint(replay(*side, i, FightMode::Intent)), first) << side->file << ' ' << i;
        }
    }
}

TEST(Ch13Balance, TheIntentPlayerMeetsBothTargetsFromBothActualCheckpoints) {
    for (const Side* side : {&kFirst, &kSecond}) {
        SCOPED_TRACE(side->file);
        for (int i = 0; i < 3; ++i) {
            const Fight f = replay(*side, i, FightMode::Intent);
            EXPECT_TRUE(onTarget(f, kFightTargets[static_cast<std::size_t>(i)]))
                << i << " rounds " << f.rounds << " heroHp " << f.heroHpLeft;
        }
        // ① 大汉的白玉大印至少一次被破势打断（8.2）。
        EXPECT_TRUE(replay(*side, 0, FightMode::Intent).interrupted);
    }
}

// 拿走背包里所有带 category 那一类兵刃的物品（「捂住剑」：HandPolicy 只捂得住法术，兵刃由背包给，
// BattleScene::heroWeapons）。
void dropWeapons(Driver& driver, int category) {
    std::vector<std::pair<std::string, int>> drop;
    for (const auto& slot : driver.state().bag) {
        const core::Item* item = driver.app().data().findItem(slot.itemId);
        if (item != nullptr && (item->weapon & category) != 0) drop.emplace_back(slot.itemId, slot.count);
    }
    for (const auto& [id, count] : drop) ASSERT_TRUE(driver.state().removeItem(id, count)) << id;
    ASSERT_FALSE(drop.empty()) << "背包里没有这一类兵刃，捂了等于没捂";
}

// 引擎 BattleAction.cpp 那半句「却伤不到……的要害」：killable_by 拦下的那一击（不是文案）。
bool lockedAtOne(const Fight& f) {
    return std::any_of(f.log.begin(), f.log.end(),
                       [](const std::string& line) { return line.find("却伤不到") != std::string::npos; });
}

// 施工图 8.2 的三句「只有……要得了命」：捂住那一类，意图手就赢不下来（胜负只看气血归零）。
// ① 儒生 killable_by 金、剑：捂住四门金类法术（祭金蚨、小刀符宝、红线针、噬金虫群）＋ 背包里的剑 → 儒生压在 1；
// ① 大汉 killable_by 木、剑：捂住青元剑芒、祭青蛟旗 ＋ 剑 → 大汉压在 1；
// ② 巨虾 killable_by 木、剑：捂住青元剑芒、祭青蛟旗、蜂云合剑 ＋ 剑 → 巨虾压在 1，那只手无路可进、② 可逃，逃了；
// ① 人人只防 → 败。对照：同一份存档意图手照常打——上面几场的「不胜」确实来自被捂住的那一类。
TEST(Ch13Balance, OnlyTheDesignedKindsKillAndStandingStillLoses) {
    const auto noSword = [](Driver& d) { dropWeapons(d, core::kCategorySword); };
    test::HandPolicy noMetal = intentPolicy();
    noMetal.withheldMagics = {"magic_ji_jinfu", "magic_ji_xiaodao", "magic_ji_hongxianzhen", "magic_shijin_chongqun"};
    test::HandPolicy noWood = intentPolicy();
    noWood.withheldMagics = {"magic_qingyuan_jianmang", "magic_ji_qingjiao", "magic_fengyun_heji"};
    for (const Side* side : {&kFirst, &kSecond}) {
        SCOPED_TRACE(side->file);
        const Fight scholar = replay(*side, 0, FightMode::Intent, noMetal, noSword);
        EXPECT_NE(scholar.phase, BattlePhase::Won);
        EXPECT_EQ(scholar.enemyHpLeft, 1) << "① 捂住金与剑：儒生压在 1";
        EXPECT_TRUE(lockedAtOne(scholar));
        const Fight brute = replay(*side, 0, FightMode::Intent, noWood, noSword);
        EXPECT_NE(brute.phase, BattlePhase::Won);
        EXPECT_EQ(brute.enemyHpLeft, 1) << "① 捂住木与剑：大汉压在 1";
        EXPECT_TRUE(lockedAtOne(brute));
        const Fight shrimp = replay(*side, 1, FightMode::Intent, noWood, noSword);
        EXPECT_NE(shrimp.phase, BattlePhase::Won);
        EXPECT_GT(shrimp.enemyHpLeft, 0);
        EXPECT_TRUE(lockedAtOne(shrimp)) << "② 捂住木与剑：巨虾被 killable_by 拦在 1 上过";
        EXPECT_EQ(replay(*side, 0, FightMode::Defend).phase, BattlePhase::Lost);
        // 对照：不捂。
        for (int i = 0; i < 2; ++i) EXPECT_EQ(replay(*side, i, FightMode::Intent).phase, BattlePhase::Won) << i;
    }
}

// 施工图 8.1 末条 / 验收第 4 条「最低合法起点两场都打得过」：中阶灵石 0、一颗丹药也没有（连蚀心散这类「pill_」
// 毒药一并拿走）、气血不满。
//   ①：n01 先拨 15 日（一日回一成），出海口带之前的人必是满血——合法的「不满」到不了 ①；这里另做一版
//       比合法更紧的：气血压回第 12 章终档的 614（n01 的回血不算）、不去 facility_dazuo_nanming 打坐。
//   ②：第一轮之前 n23 拨 30 日，必是满血；第二轮带着第一轮打完、三日回三成的余血直接上（意图玩家就是这样）——
//       这是「任何合法消耗之后仍走得下去」的那一半；另做一版第二轮之前在雾中岛静室静养十日（150 的静室
//       兜底走得通）。
// 「① carried」与「② second round carried」两版打出来的数与意图手那一趟一字不差（① 700→312、② 第二轮
// 553→283）：那只手本来就不吃药、不花中阶灵石（intentPolicy 的 pills 空），拿走丹药与中阶灵石不改变它的出手。
// 这两版证的是「拿走之后照样打得过」，不是另一组更紧的局面；真正加压的是「① at the chapter-12 hp」（气血压回
// 614、不打坐）与「② second round rested」（静室坐十日再打）两版（测试审查 T-6；同第 12 章 R-2、ch12 18.5 第 6 条）。
void minimumBag(Driver& driver) {
    auto& bag = driver.state();
    std::vector<std::pair<std::string, int>> pills;
    for (const auto& slot : bag.bag) if (slot.itemId.rfind("pill_", 0) == 0) pills.emplace_back(slot.itemId, slot.count);
    for (const auto& [id, count] : pills) ASSERT_TRUE(bag.removeItem(id, count)) << id;
    const int mid = bag.itemCount(kMid);
    if (mid > 0) ASSERT_TRUE(bag.removeItem(kMid, mid));
}

void expectMinimum(const Fight& f, const char* what) {
    std::cout << "[ch13 minimum] " << what << ' ' << f.id << " phase=" << static_cast<int>(f.phase)
              << " rounds=" << f.rounds << " hero " << f.before.hp << "->" << f.heroHpLeft << '\n';
    EXPECT_EQ(f.phase, BattlePhase::Won) << what;
    EXPECT_GT(f.heroHpLeft, 0) << what;
    EXPECT_EQ(f.before.itemCount(kMid), 0) << what;
    for (const auto& slot : f.before.bag) EXPECT_NE(slot.itemId.rfind("pill_", 0), 0u) << what << ' ' << slot.itemId;
}

TEST(Ch13Balance, TheMinimumLegalStartStillWinsBothFights) {
    for (const Side* side : {&kFirst, &kSecond}) {
        SCOPED_TRACE(side->file);
        const Fight carried = replay(*side, 0, FightMode::Intent, intentPolicy(), minimumBag);
        EXPECT_EQ(carried.before.hp, carried.before.maxHp) << "n01 拨过 15 日：出海口前必是满血";
        expectMinimum(carried, "① carried");
        const Fight pressed = replay(*side, 0, FightMode::Intent, intentPolicy(), [side](Driver& d) {
            minimumBag(d);
            d.state().hp = side->hp;
        });
        EXPECT_EQ(pressed.before.hp, side->hp);
        expectMinimum(pressed, "① at the chapter-12 hp");
        const Fight second = replay(*side, 2, FightMode::Intent, intentPolicy(), minimumBag);
        EXPECT_LT(second.before.hp, second.before.maxHp) << "② 第二轮真是带着残血上的";
        expectMinimum(second, "② second round carried");
        // 静养版：第一轮打完、去静室坐十日再回阵心礁。
        const Walk& walk = intentWalk(*side);
        Driver rested;
        rested.audit = false;
        ASSERT_TRUE(rested.resumeReached(reachedAfter(walk, 23), 24)) << rested.problem();
        minimumBag(rested);
        ASSERT_TRUE(rested.press("trigger_youyao")) << rested.problem();
        ASSERT_TRUE(rested.sit("facility_jingshi_wz", 10)) << rested.problem();
        EXPECT_EQ(rested.state().hp, rested.state().maxHp);
        ASSERT_TRUE(rested.press("trigger_youyao")) << rested.problem();
        ASSERT_EQ(rested.fights().size(), 2u);
        expectMinimum(rested.fights().back(), "② second round rested");
        EXPECT_EQ(rested.state().flag("ch13.lieyao"), 2);
    }
}

// 负向：两条目标各自拒得住一场没达标的仗（回合出窗、没胜、韩立倒下、余血跌破下限、敌人没清、编成不对），
// 指纹改一个字就不等。
TEST(Ch13Balance, EachTargetRejectsAFightThatMissesIt) {
    const Walk& walk = intentWalk(kFirst);
    ASSERT_TRUE(walk.problem.empty()) << walk.problem;
    ASSERT_EQ(walk.fights.size(), 3u);
    for (std::size_t i = 0; i < 3; ++i) {
        SCOPED_TRACE(i);
        const std::size_t which = kFightTargets[i];
        const Fight& good = walk.fights[i];
        ASSERT_TRUE(onTarget(good, which));
        for (const int rounds : {kTargets[which].minRounds - 1, kTargets[which].maxRounds + 1}) {
            Fight bad = good;
            bad.rounds = rounds;
            EXPECT_FALSE(onTarget(bad, which)) << rounds;
        }
        { Fight bad = good; bad.phase = BattlePhase::Lost; EXPECT_FALSE(onTarget(bad, which)); }
        { Fight bad = good; bad.phase = BattlePhase::Escaped; EXPECT_FALSE(onTarget(bad, which)); }
        { Fight bad = good; bad.heroHpLeft = 0; EXPECT_FALSE(onTarget(bad, which)); }
        // 余血下限：149 必报、恰 150 不报（边界对照：拒它的是这条下限，不是别的项）。
        { Fight bad = good; bad.heroHpLeft = kHeroHpFloor - 1; EXPECT_FALSE(onTarget(bad, which)) << "险胜"; }
        { Fight edge = good; edge.heroHpLeft = kHeroHpFloor; EXPECT_TRUE(onTarget(edge, which)) << "恰在下限"; }
        { Fight bad = good; bad.enemyHpLeft = 1; EXPECT_FALSE(onTarget(bad, which)); }
        { Fight bad = good; bad.id = kBattles[1 - which]; EXPECT_FALSE(onTarget(bad, which)); }
    }
    const auto good = fightFingerprint(walk.fights[0]);
    ASSERT_FALSE(walk.fights[0].log.empty());
    { Fight bad = walk.fights[0]; bad.rounds += 1; EXPECT_NE(fightFingerprint(bad), good) << "rounds"; }
    { Fight bad = walk.fights[0]; bad.phase = BattlePhase::Lost; EXPECT_NE(fightFingerprint(bad), good) << "phase"; }
    { Fight bad = walk.fights[0]; bad.log.back() += " "; EXPECT_NE(fightFingerprint(bad), good) << "log"; }
    { Fight bad = walk.fights[0]; bad.after.hp -= 1; EXPECT_NE(fightFingerprint(bad), good) << "after.hp"; }
    { Fight bad = walk.fights[0]; EXPECT_EQ(fightFingerprint(bad), good) << "对照：原样复制的一场指纹相同"; }
}

// 负向（测试审查 T-2，协调者裁定）：调前那组数值打出来的仗必须判不达标。不改数据文件，只在这一趟的 Application
// 里把平衡路调过的三个 role 改回调前值（notes/ch13/balance.md 第 1 节），从同一份真实存档重打：
// ①：胜 19 回合、韩立余 75（balance.md 第 0 节调前那一行）——19 落在 ① 的回合窗 10–22 之内，拒它的是余血下限；
// ② 第一轮：胜 14 回合、700→112，同样只有下限拒得住；② 第二轮（三日回三成，322 起打）：败。
// 对照：同一场只把余血抬到下限就判达标——窗没拦它，拦它的确是下限。
void restorePreTuningRoles(Driver& driver) {
    auto& roles = driver.app().data().roles;
    // 先验：数据文件是调后值，这里只动内存。
    ASSERT_EQ(roles.at("nixing_rusheng").maxHp, 360);
    ASSERT_EQ(roles.at("wuji_haishou").maxMp, 18);
    ASSERT_EQ(roles.at("wuji_haishou").attack, 30);
    ASSERT_EQ(roles.at("qingse_juxia").maxHp, 700);
    ASSERT_EQ(roles.at("qingse_juxia").attack, 30);
    roles.at("nixing_rusheng").maxHp = 480;
    roles.at("wuji_haishou").maxMp = 54;
    roles.at("wuji_haishou").attack = 34;
    roles.at("qingse_juxia").maxHp = 820;
    roles.at("qingse_juxia").attack = 36;
}

TEST(Ch13Balance, ThePreTuningRolesMissTheTargetsReplayedInMemory) {
    ASSERT_TRUE(intentWalk(kFirst).problem.empty()) << intentWalk(kFirst).problem;
    struct Expected { int which; BattlePhase phase; int rounds; int heroHp; };
    for (const Expected& e : {Expected{0, BattlePhase::Won, 19, 75}, Expected{1, BattlePhase::Won, 14, 112},
                              Expected{2, BattlePhase::Lost, 8, 0}}) {
        SCOPED_TRACE(e.which);
        const Fight f = replay(kFirst, e.which, FightMode::Intent, intentPolicy(), restorePreTuningRoles);
        const std::size_t target = kFightTargets.at(static_cast<std::size_t>(e.which));
        // 复现 balance.md 第 0 节的调前记录（同一只手、同一份存档，确定性）。
        EXPECT_EQ(f.phase, e.phase);
        EXPECT_EQ(f.rounds, e.rounds);
        EXPECT_EQ(f.heroHpLeft, e.heroHp);
        EXPECT_FALSE(onTarget(f, target)) << "调前的数值必须判不达标";
        if (e.phase != BattlePhase::Won) continue;
        EXPECT_GE(f.rounds, kTargets[target].minRounds) << "回合在窗内：窗拦不住它";
        EXPECT_LE(f.rounds, kTargets[target].maxRounds) << "回合在窗内：窗拦不住它";
        Fight lifted = f;
        lifted.heroHpLeft = kHeroHpFloor;
        EXPECT_TRUE(onTarget(lifted, target)) << "对照：只把余血抬到下限就达标";
    }
}

// ---- 起点、终点、逐节点账：检查自己的负向 ----
TEST(Ch13Acceptance, TheStartGuardCatchesTheOtherSidesLiterals) {
    const Side swapped{kSecond.file, kSecond.ending, kFirst.day, kFirst.low, kFirst.mid, kFirst.hp, kFirst.cultivation,
                       kFirst.retreats};
    Driver driver;
    const auto [count, text] = capturedFailures([&] { static_cast<void>(driver.load(swapped)); });
    // 中阶两侧都是 20、气血两侧都是 614，换过来照样对得上；只有 day、低阶、修为三条报。
    EXPECT_EQ(count, 3) << text;
    for (const char* literal : {"49560", "2331", "4072"}) EXPECT_NE(text.find(literal), std::string::npos) << literal;
    Driver control;
    const auto [none, quiet] = capturedFailures([&] { EXPECT_TRUE(control.load(kSecond)); });
    EXPECT_EQ(none, 0) << "对照：照第二侧自己的字面量一条不报\n" << quiet;
}

// 一趟真走完，再在内存里一处一处改坏终局：每一处恰报一条、报的就是那一项。
TEST(Ch13Acceptance, TheEndingAndTheNodeLedgerCatchEachSingleMutation) {
    Driver driver;
    ASSERT_TRUE(driver.load(kFirst)) << driver.problem();
    ASSERT_TRUE(driver.runTo(34)) << driver.problem();
    {
        const auto [count, text] = capturedFailures([&] { driver.checkEnding(kFirst); });
        ASSERT_EQ(count, 0) << text;
    }
    const GameState good = driver.state();
    struct Mutation { const char* what; void (*apply)(GameState&); const char* mentions; };
    const Mutation mutations[] = {
        {"one more day", [](GameState& s) { s.day += 1; }, "kChapterDays"},
        {"one more low stone", [](GameState& s) { s.addItem("material_lingshi", 1); }, "kLow"},
        {"a mid stone spent", [](GameState& s) { static_cast<void>(s.removeItem("material_lingshi_zhong", 1)); }, "kMid"},
        {"a sixth-grade core spent", [](GameState& s) { static_cast<void>(s.removeItem("material_yaodan_liuji", 1)); }, "kLiuji"},
        {"one more fifth-grade core", [](GameState& s) { s.addItem("material_yaodan_wuji", 1); }, "kWuji"},
        {"one more herb", [](GameState& s) { s.addItem("herb_nichang_cao", 1); }, "kHerb"},
        {"no copper shard", [](GameState& s) { static_cast<void>(s.removeItem("story_tongpian", 1)); }, "story_tongpian"},
        {"a silkworm left", [](GameState& s) { s.addItem("story_jinsi_can", 1); }, "story_jinsi_can"},
        {"the waybill kept", [](GameState& s) { s.addItem("story_liuqi_cailiao", 1); }, "story_liuqi_cailiao"},
        {"paid in stones", [](GameState& s) { s.setFlag("ch13.toudu", 1); }, "ch13.toudu"},
        {"three lure wins", [](GameState& s) { s.setFlag("ch13.lieyao", 3); }, "ch13.lieyao"},
        {"the cap still at 31", [](GameState& s) { s.realmCap = rules::Realm::CoreEarly; }, "realmCap"},
        {"vitals of the early core", [](GameState& s) { s.maxHp = 700; }, "maxHp"},
        {"standing on the cave", [](GameState& s) { s.position = Point{61, 45}; }, "kEndCell"},
        {"facing south", [](GameState& s) { s.facing = 2; }, "facing"},
        {"still on the fog island", [](GameState& s) { s.mapId = "ch13_wuzhong"; }, "ch13_waihai"},
        {"a companion", [](GameState& s) { s.party.push_back({"qu_hun_shadan", 820, true}); }, "party"},
    };
    for (const auto& m : mutations) {
        SCOPED_TRACE(m.what);
        driver.state() = good;
        m.apply(driver.state());
        const auto [count, text] = capturedFailures([&] { driver.checkEnding(kFirst); });
        EXPECT_EQ(count, 1) << text;
        EXPECT_NE(text.find(m.mentions), std::string::npos) << text;
    }
    driver.state() = good;
    // 逐节点账：n24 多一颗五级、n28 多一日、n27 上限没抬，各恰报一条；② 第一轮里「偷偷多给 3 块低阶」
    // 先在仗里报、再在节点账里报。
    const auto& ledger = driver.ledger();
    ASSERT_EQ(ledger.size(), 34u);
    ASSERT_EQ(driver.fights().size(), 3u);
    Entry cores = ledger[23];
    ASSERT_EQ(cores.node, 24);
    cores.after.addItem("material_yaodan_wuji", 1);
    auto [count, text] = capturedFailures([&] { driver.checkEntry(kFirst, cores, 1, 3); });
    EXPECT_EQ(count, 1) << text;
    EXPECT_NE(text.find("material_yaodan_wuji"), std::string::npos) << text;
    Entry day = ledger[27];
    ASSERT_EQ(day.node, 28);
    day.after.day += 1;
    std::tie(count, text) = capturedFailures([&] { driver.checkEntry(kFirst, day, 3, 3); });
    EXPECT_EQ(count, 1) << text;
    // n28 的日数取该侧的字面量 N（测试审查 T-4）：同一份真账拿 N ＝ 3 的字面量来核 → 恰报日数那一条；
    // 驱动自己数的按键次数仍是 2，它不进断言右边。对照：照第一侧自己的 N 一条不报。
    Side three = kFirst;
    three.retreats = 3;
    std::tie(count, text) = capturedFailures([&] { driver.checkEntry(three, ledger[27], 3, 3); });
    EXPECT_EQ(count, 1) << text;
    EXPECT_EQ(driver.retreats(), 2);
    std::tie(count, text) = capturedFailures([&] { driver.checkEntry(kFirst, ledger[27], 3, 3); });
    EXPECT_EQ(count, 0) << "对照：照第一侧的 N 一条不报\n" << text;
    Entry cap = ledger[26];
    ASSERT_EQ(cap.node, 27);
    cap.after.realmCap = rules::Realm::CoreEarly;
    std::tie(count, text) = capturedFailures([&] { driver.checkEntry(kFirst, cap, 3, 3); });
    EXPECT_EQ(count, 1) << text;
    const GameState ghostAfter = driver.fights()[1].after;
    Entry loot = ledger[23];
    loot.after.addItem("material_lingshi", 3);
    driver.mutableFights()[1].after.addItem("material_lingshi", 3);
    std::tie(count, text) = capturedFailures([&] { driver.checkEntry(kFirst, loot, 1, 3); });
    driver.mutableFights()[1].after = ghostAfter;
    EXPECT_EQ(count, 2) << text;
    EXPECT_NE(text.find("changed inside b13_waixinghai_lieyao"), std::string::npos) << text;
    std::tie(count, text) = capturedFailures([&] { driver.checkEntry(kFirst, ledger[23], 1, 3); });
    EXPECT_EQ(count, 0) << "对照：还原后 n24 的账一条不报\n" << text;
}
} // namespace
