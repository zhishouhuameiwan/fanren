// 第 12 章验收第 2 条：两侧从真实第 11 章终档（T1 重生成版）起步、走同一组意图选项通关；终点照字面量断言；
// 写 / 比终档（FANREN_WRITE_CH12_FIXTURES=1 写）。另含：这几条检查各自的「故意写坏 → 确实被抓住」，
// 与平衡路的四场判据（施工图 8.1 意图玩家、10.2 B1、BattleHand）。
#include <gtest/gtest-spi.h>
#include <tuple>
#include "Ch12Driver.h"

namespace {
using namespace fanren;
using namespace fanren::test::ch12;

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

// ---- 平衡目标（施工图 8.1、8.2、10.2 B1；派工单「意图玩家不死」）----
// 共同：胜、敌方清场、韩立站着打完（heroHpLeft > 0）、回合数落在下面的窗里。施工图没给回合数，窗按平衡路
// 定稿的实测（两侧相同：① 16、② 10、③ 10、④ 8）各放宽约四成，见 notes/ch12/balance.md 第 0 节；
// 那只手先拿法术逐类试探破绽（④ 要试四五手才摸到木），回合数比真人多。
// ① 三波打得完、第三波怪笑被破势打断过（出手记录有「蓄势被打断」）；② 三波打完；③ 两波打完；④ 独战打完。
struct Target {
    int minRounds;
    int maxRounds;
};
constexpr std::array<Target, 4> kTargets{{{12, 20}, {7, 14}, {6, 14}, {5, 12}}};

bool onTarget(const Fight& f, std::size_t which) {
    const Target t = kTargets.at(which);
    const bool common = f.id == kBattles.at(which) && f.phase == BattlePhase::Won && f.heroHpLeft > 0 &&
                        f.enemyHpLeft == 0 && f.rounds >= t.minRounds && f.rounds <= t.maxRounds;
    return which == 0 ? common && f.interrupted : common;
}

std::vector<std::string> allyIds(const char* a = nullptr, const char* b = nullptr) {
    std::vector<std::string> out{test::kHandHero};
    if (a) out.push_back(a);
    if (b) out.push_back(b);
    return out;
}

void verifyRun(Driver& driver, const Side& side) {
    driver.checkEnding(side);
    ASSERT_EQ(driver.fights().size(), 4u);
    for (std::size_t i = 0; i < 4; ++i)
        EXPECT_TRUE(onTarget(driver.fights()[i], i)) << kBattles[i] << " rounds " << driver.fights()[i].rounds
                                                     << " heroHp " << driver.fights()[i].heroHpLeft;
    // 契约 E1 的真实入口版：空队伍下我方 ＝ 韩立 ＋ 编成里写死的友军，没有第四个人。
    EXPECT_EQ(driver.fights()[0].allies, allyIds("zi_ling_xutian", "yuan_yao_heipao"));
    EXPECT_EQ(driver.fights()[1].allies, allyIds("yuan_yao_jiedan"));
    EXPECT_EQ(driver.fights()[2].allies, allyIds("wu_chou", "xuan_gu"));
    EXPECT_EQ(driver.fights()[3].allies, allyIds());
    // 验收第 20 条（真实入口那一半）：ch12.done 排「第十二章　终」；第 13 章进表之后接第十三章开篇
    //（契约 interfaces-p3-ch13.md 5.3：章节卡断言改 Opening(13)）。
    const auto& cards = driver.app().cardLog();
    ASSERT_GE(cards.size(), 2u);
    EXPECT_EQ(cards[cards.size() - 2], (game::CardRequest{game::CardKind::Closing, 12}));
    EXPECT_EQ(cards.back(), (game::CardRequest{game::CardKind::Opening, 13}));
    ASSERT_FALSE(::testing::Test::HasFailure());
    const test::TempDir temporary("fanren_ch12_actual_ending");
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
        test::kWriteChapterTwelveFixturesEnv, "Chapter 13");
    EXPECT_TRUE(verdict.problem.empty()) << verdict.problem;
}

TEST(Ch12Walkthrough, FirstSideUsesTheActualC11EndingAndTheIntentChoices) {
    Driver driver;
    ASSERT_TRUE(driver.load(kFirst)) << driver.problem();
    ASSERT_TRUE(driver.runTo(33)) << driver.problem();
    verifyRun(driver, kFirst);
}

TEST(Ch12Walkthrough, SecondSideUsesTheActualC11EndingAndTheSameChoices) {
    Driver driver;
    ASSERT_TRUE(driver.load(kSecond)) << driver.problem();
    ASSERT_TRUE(driver.runTo(33)) << driver.problem();
    verifyRun(driver, kSecond);
}

// 验收第 2 条与第 13 条末句：中阶两侧都不动，低阶两侧差恰是起点差 20，day 差两侧都恰 182。
TEST(Ch12Acceptance, BothSidesKeepTheTwentyLowStoneGapNoMidStoneSpentAndOneHundredEightyTwoDays) {
    const Walk& first = intentWalk(kFirst);
    const Walk& second = intentWalk(kSecond);
    ASSERT_TRUE(first.problem.empty()) << first.problem;
    ASSERT_TRUE(second.problem.empty()) << second.problem;
    EXPECT_EQ(first.end.itemCount("material_lingshi") - second.end.itemCount("material_lingshi"), 20);
    EXPECT_EQ(first.end.itemCount("material_lingshi_zhong"), 20);
    EXPECT_EQ(second.end.itemCount("material_lingshi_zhong"), 20);
    EXPECT_EQ(first.end.day - kFirst.day, 182);
    EXPECT_EQ(second.end.day - kSecond.day, 182);
}

// ---- 平衡：四场从两侧真实走到的那一刻起打（不手搭起点）----
// ① n12 走完（鬼雾土坡）；② n20 走完（冰火道光壁之后）；③ n28 走完（寒骊台台背）；④ n30 走完（补天丹之后）。
constexpr std::array<int, 4> kFightNodes{13, 21, 29, 31};

Fight replay(const Side& side, std::size_t which, FightMode mode, test::HandPolicy policy = intentPolicy()) {
    const Walk& walk = intentWalk(side);
    const int node = kFightNodes.at(which);
    Driver driver;
    driver.policy = std::move(policy);
    driver.audit = false;
    EXPECT_TRUE(driver.resumeReached(reachedAfter(walk, node - 1), node)) << driver.problem();
    driver.nextModes = {mode};
    // 败致命的场次会走到 game_over：脚本不正常收尾，但这一场的记录照样在。
    static_cast<void>(driver.runTo(node));
    if (driver.fights().empty()) {
        ADD_FAILURE() << "No fight at node " << node << ": " << driver.problem();
        return {};
    }
    const Fight& f = driver.fights().back();
    std::cout << "[ch12 balance] " << side.file << ' ' << f.id << " mode=" << static_cast<int>(mode)
              << " phase=" << static_cast<int>(f.phase) << " rounds=" << f.rounds << " enemyHp=" << f.enemyHpLeft
              << " hero " << f.before.hp << "->" << f.heroHpLeft << " mp " << f.before.mp << "->" << f.after.mp
              << " interrupted=" << f.interrupted << '\n';
    return f;
}

// 先证那只手稳：同一份真实存档、同一场仗打三遍，结局指纹（回合、胜负、出手记录、战后存档）一字不差，
// 且与通关那一趟里的同一场一字不差。
TEST(Ch12Balance, TheHandIsSteadyThreeTimesOnEachOfTheFourFromBothSides) {
    for (const Side* side : {&kFirst, &kSecond}) {
        const Walk& walk = intentWalk(*side);
        ASSERT_TRUE(walk.problem.empty()) << walk.problem;
        ASSERT_EQ(walk.fights.size(), 4u);
        for (std::size_t i = 0; i < 4; ++i) {
            const auto first = fightFingerprint(replay(*side, i, FightMode::Intent));
            ASSERT_GT(first.size(), 20u);
            EXPECT_EQ(first, fightFingerprint(walk.fights[i])) << side->file << ' ' << kBattles[i];
            for (int again = 0; again < 2; ++again)
                EXPECT_EQ(fightFingerprint(replay(*side, i, FightMode::Intent)), first) << side->file << ' ' << kBattles[i];
        }
    }
}

TEST(Ch12Balance, TheIntentPlayerMeetsTheFourTargetsFromBothActualCheckpoints) {
    for (const Side* side : {&kFirst, &kSecond}) {
        SCOPED_TRACE(side->file);
        for (std::size_t i = 0; i < 4; ++i) {
            const Fight f = replay(*side, i, FightMode::Intent);
            EXPECT_TRUE(onTarget(f, i)) << kBattles[i] << " rounds " << f.rounds << " heroHp " << f.heroHpLeft
                                        << " interrupted " << f.interrupted;
        }
    }
}

// 施工图 10.2 B1 的四句「只有……要得了命」：捂住那一类，意图手就赢不下来（胜负只看气血归零）。
// ② 捂住水、金（冰冻术；祭金蚨、小刀符宝、红线针、噬金虫群）→ 剑拳火只压到 1；
// ③ 韩立只防、两名友军照打 → 银形压不死（乌丑水、玄骨火，都不配金）；
// ④ 捂住木（青元剑芒、祭青蛟旗）→ 玄骨压不死；人人只防 → 败。
TEST(Ch12Balance, OnlyTheDesignedElementsKillAndTheAlliesAloneCannotClaimTheSilverWolf) {
    test::HandPolicy noWaterNoMetal = intentPolicy();
    noWaterNoMetal.withheldMagics = {"magic_bingdong_shu", "magic_ji_jinfu", "magic_ji_xiaodao",
                                     "magic_ji_hongxianzhen", "magic_shijin_chongqun"};
    // 测试审查 T-3：③④ 不只「没赢」，要钉住「被压在 1」（敌余恰 1：只剩那一个 killable_by 的首领、压在 1 上）；
    // ② 敌余很多、韩立先倒，分不出「锁在 1」与「单纯打不过」，所以另断出手记录里真有 killable_by 拦下的那一击
    //（引擎 BattleAction.cpp 那半句「却伤不到……的要害」，不是文案）。
    const auto lockedAtOne = [](const Fight& f) {
        return std::any_of(f.log.begin(), f.log.end(),
                           [](const std::string& line) { return line.find("却伤不到") != std::string::npos; });
    };
    const Fight ants = replay(kFirst, 1, FightMode::Intent, noWaterNoMetal);
    EXPECT_NE(ants.phase, BattlePhase::Won);
    EXPECT_GT(ants.enemyHpLeft, 0);
    EXPECT_TRUE(lockedAtOne(ants)) << "② 捂住水金：剑拳火该被 killable_by 拦在 1 上过";
    const Fight wolves = replay(kFirst, 2, FightMode::AlliesOnly);
    EXPECT_NE(wolves.phase, BattlePhase::Won);
    EXPECT_EQ(wolves.enemyHpLeft, 1) << "③ 只靠友军：银形压在 1";
    EXPECT_TRUE(lockedAtOne(wolves));
    test::HandPolicy noWood = intentPolicy();
    noWood.withheldMagics = {"magic_qingyuan_jianmang", "magic_ji_qingjiao"};
    const Fight bones = replay(kFirst, 3, FightMode::Intent, noWood);
    EXPECT_NE(bones.phase, BattlePhase::Won);
    EXPECT_EQ(bones.enemyHpLeft, 1) << "④ 捂住木：玄骨压在 1";
    EXPECT_TRUE(lockedAtOne(bones));
    const Fight idle = replay(kFirst, 3, FightMode::Defend);
    EXPECT_EQ(idle.phase, BattlePhase::Lost);
    // 对照：同一份存档、意图手照常打——上面四场的「不胜」确实来自被捂住的那一类。
    for (std::size_t i = 1; i < 4; ++i) EXPECT_EQ(replay(kFirst, i, FightMode::Intent).phase, BattlePhase::Won) << i;
}

// 施工图 8.1 末条 / 10.2 B1：最低合法起点——中阶灵石 0、一颗丹药也没有（连蚀心散这类「pill_」毒药一并拿走），
// 上一场剩多少带多少。②③④ 各打两版（测试审查 T-6）：
//   静养版：战前在最近的打坐设施静养十日（一日回一成，十日回满）——证的是「②③④ 之前的打坐兜底走得通」；
//   压力版：不静养，带着意图玩家上一场打完、一路走来的余血直接上（③ 619 起、④ 553 起，不满）——这才是
//   「任何合法消耗之后仍走得下去」的那一半：手里少了丹药与中阶灵石，余血不回也要打得过。
// ② 冰火道光壁旁（n20 之后、光壁触发带已演过）；③ 寒骊台台下（n27 落地之后、上台背之前——上去之后台阶顶
// 是 n29 的触发格，下不来；n27 单向传送之后回不去塔前石室，所以 ③ 的兜底就在寒骊台）；④ 寒骊台台下（n30 之后）。
TEST(Ch12Balance, TheMinimumLegalStartStillWinsTheLastThreeRestedAndUnrested) {
    struct Rest { std::size_t which; int from; const char* facility; };
    for (const Side* side : {&kFirst, &kSecond}) {
        SCOPED_TRACE(side->file);
        const Walk& walk = intentWalk(*side);
        ASSERT_TRUE(walk.problem.empty()) << walk.problem;
        for (const bool rested : {true, false}) {
            SCOPED_TRACE(rested ? "rested ten days" : "unrested, carried hp");
            for (const Rest r : {Rest{1, 20, "facility_dazuo_binghuo"}, Rest{2, 27, "facility_dazuo_hanlitai"},
                                 Rest{3, 30, "facility_dazuo_hanlitai"}}) {
                const GameState reached = reachedAfter(walk, r.from);
                Driver driver;
                driver.audit = false;
                ASSERT_TRUE(driver.resumeReached(reached, r.from + 1)) << driver.problem();
                auto& bag = driver.state();
                std::vector<std::pair<std::string, int>> pills;
                for (const auto& slot : bag.bag) if (slot.itemId.rfind("pill_", 0) == 0) pills.emplace_back(slot.itemId, slot.count);
                for (const auto& [id, count] : pills) ASSERT_TRUE(bag.removeItem(id, count)) << id;
                const int mid = bag.itemCount("material_lingshi_zhong");
                if (mid > 0) ASSERT_TRUE(bag.removeItem("material_lingshi_zhong", mid));
                if (rested) {
                    ASSERT_TRUE(driver.sit(r.facility, 10)) << driver.problem();
                    EXPECT_EQ(driver.state().hp, driver.state().maxHp);
                    EXPECT_EQ(driver.state().mp, driver.state().maxMp);
                } else {
                    EXPECT_EQ(driver.state().hp, reached.hp);
                    // ③④ 之前意图玩家一路不回血（8.1）：压力版真是带着残血上的。
                    if (r.which >= 2) EXPECT_LT(driver.state().hp, driver.state().maxHp) << kBattles[r.which];
                }
                static_cast<void>(driver.runTo(kFightNodes[r.which]));
                ASSERT_FALSE(driver.fights().empty()) << driver.problem();
                const Fight& f = driver.fights().back();
                std::cout << "[ch12 minimum] " << side->file << (rested ? " rested " : " unrested ") << f.id
                          << " phase=" << static_cast<int>(f.phase) << " rounds=" << f.rounds << " hero "
                          << f.before.hp << "->" << f.heroHpLeft << '\n';
                EXPECT_EQ(f.id, kBattles[r.which]);
                EXPECT_EQ(f.phase, BattlePhase::Won) << kBattles[r.which];
                EXPECT_GT(f.heroHpLeft, 0) << kBattles[r.which];
                EXPECT_EQ(f.before.itemCount("material_lingshi_zhong"), 0);
                for (const auto& slot : f.before.bag) EXPECT_NE(slot.itemId.rfind("pill_", 0), 0u) << slot.itemId;
            }
        }
    }
}

// 负向：四条目标各自拒得住一场没达标的仗（回合出窗、没胜、韩立倒下、敌人没清、① 怪笑没被打断），
// 指纹改一个字就不等。
TEST(Ch12Balance, EachTargetRejectsAFightThatMissesIt) {
    const Walk& walk = intentWalk(kFirst);
    ASSERT_TRUE(walk.problem.empty()) << walk.problem;
    ASSERT_EQ(walk.fights.size(), 4u);
    for (std::size_t i = 0; i < 4; ++i) {
        SCOPED_TRACE(kBattles[i]);
        const Fight& good = walk.fights[i];
        ASSERT_TRUE(onTarget(good, i));
        for (const int rounds : {kTargets[i].minRounds - 1, kTargets[i].maxRounds + 1}) {
            Fight bad = good;
            bad.rounds = rounds;
            EXPECT_FALSE(onTarget(bad, i)) << rounds;
        }
        { Fight bad = good; bad.phase = BattlePhase::Lost; EXPECT_FALSE(onTarget(bad, i)); }
        { Fight bad = good; bad.heroHpLeft = 0; EXPECT_FALSE(onTarget(bad, i)); }
        { Fight bad = good; bad.enemyHpLeft = 1; EXPECT_FALSE(onTarget(bad, i)); }
        { Fight bad = good; bad.id = kBattles[(i + 1) % 4]; EXPECT_FALSE(onTarget(bad, i)); }
    }
    { Fight bad = walk.fights[0]; bad.interrupted = false; EXPECT_FALSE(onTarget(bad, 0)); }
    // 指纹（「手稳」那条比的东西）：同一场仗改掉回合、胜负、出手记录里一个字、战后存档里一点气血，
    // 喂给 fightFingerprint 都必须与原仗不等（测试审查 T-5：从前比的是同一个 vector 改动前后的自身，恒真）。
    const auto good = fightFingerprint(walk.fights[0]);
    ASSERT_FALSE(walk.fights[0].log.empty());
    { Fight bad = walk.fights[0]; bad.rounds += 1; EXPECT_NE(fightFingerprint(bad), good) << "rounds"; }
    { Fight bad = walk.fights[0]; bad.phase = BattlePhase::Lost; EXPECT_NE(fightFingerprint(bad), good) << "phase"; }
    { Fight bad = walk.fights[0]; bad.log.back() += " "; EXPECT_NE(fightFingerprint(bad), good) << "log"; }
    { Fight bad = walk.fights[0]; bad.after.hp -= 1; EXPECT_NE(fightFingerprint(bad), good) << "after.hp"; }
    { Fight bad = walk.fights[0]; EXPECT_EQ(fightFingerprint(bad), good) << "对照：原样复制的一场指纹相同"; }
}

// ---- 起点、终点、逐节点账：检查自己的负向 ----
TEST(Ch12Acceptance, TheStartGuardCatchesTheOtherSidesLiterals) {
    const Side swapped{kSecond.file, kSecond.ending, kFirst.day, kFirst.low, kFirst.mid};
    Driver driver;
    const auto [count, text] = capturedFailures([&] { static_cast<void>(driver.load(swapped)); });
    // 中阶两侧都是 20，换过来照样对得上；只有 day 与低阶两条报。
    EXPECT_EQ(count, 2) << text;
    for (const char* literal : {"49378", "2339"}) EXPECT_NE(text.find(literal), std::string::npos) << literal;
}

// 一趟真走完，再在内存里一处一处改坏终局：每一处恰报一条、报的就是那一项。
TEST(Ch12Acceptance, TheEndingAndTheNodeLedgerCatchEachSingleMutation) {
    Driver driver;
    ASSERT_TRUE(driver.load(kFirst)) << driver.problem();
    ASSERT_TRUE(driver.runTo(33)) << driver.problem();
    {
        const auto [count, text] = capturedFailures([&] { driver.checkEnding(kFirst); });
        ASSERT_EQ(count, 0) << text;
    }
    const GameState good = driver.state();
    struct Mutation { const char* what; void (*apply)(GameState&); const char* mentions; };
    const Mutation mutations[] = {
        {"one more day", [](GameState& s) { s.day += 1; }, "182"},
        {"one more low stone", [](GameState& s) { s.addItem("material_lingshi", 1); }, "material_lingshi"},
        {"a mid stone spent", [](GameState& s) { static_cast<void>(s.removeItem("material_lingshi_zhong", 1)); }, "material_lingshi_zhong"},
        {"a thunder left over", [](GameState& s) { s.addItem("talisman_bixie_shenlei", 1); }, "talisman_bixie_shenlei"},
        {"no mirror cloak", [](GameState& s) { static_cast<void>(s.removeItem("story_xuese_pifeng", 1)); }, "story_xuese_pifeng"},
        {"the ring picked first", [](GameState& s) { s.setFlag("ch12.quba_xian", 2); }, "ch12.quba_xian"},
        {"one bit of the heart", [](GameState& s) { s.setFlag("ch12.xinjing", 1); }, "ch12.xinjing"},
        {"the companion kept", [](GameState& s) { s.party.push_back({"qu_hun_shadan", 820, true}); }, "party"},
        {"standing beside her", [](GameState& s) { s.position = Point{14, 9}; }, "position"},
        // 第 13 章契约 5.3：终点是 teleport 落下的 ch13_nanming；还留在灵泉那张图上恰报 mapId 那一条。
        {"still on the spring map", [](GameState& s) { s.mapId = "ch12_lingquan"; }, "ch13_nanming"},
        {"one step into chapter 13", [](GameState& s) { s.position = Point{6, 28}; }, "position"},
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
    // 逐节点账：n24 多一块低阶、n33 多一日、n13 神雷没收尽，各恰报一条。
    const auto& ledger = driver.ledger();
    ASSERT_EQ(ledger.size(), 33u);
    Entry stones = ledger[23];
    ASSERT_EQ(stones.node, 24);
    stones.after.addItem("material_lingshi", 1);
    auto [count, text] = capturedFailures([&] { driver.checkEntry(stones, driver.fights().size()); });
    EXPECT_EQ(count, 1) << text;
    EXPECT_NE(text.find("material_lingshi"), std::string::npos) << text;
    Entry day = ledger[32];
    ASSERT_EQ(day.node, 33);
    day.after.day += 1;
    std::tie(count, text) = capturedFailures([&] { driver.checkEntry(day, driver.fights().size()); });
    EXPECT_EQ(count, 1) << text;
    Entry thunder = ledger[12];
    ASSERT_EQ(thunder.node, 13);
    thunder.after.addItem("talisman_bixie_shenlei", 1);
    // n13 那一趟恰是第一场仗（下标 [0, 1)）。
    std::tie(count, text) = capturedFailures([&] { driver.checkEntry(thunder, 0, 1); });
    EXPECT_EQ(count, 1) << text;
    EXPECT_NE(text.find("talisman_bixie_shenlei"), std::string::npos) << text;
    // 战斗收支直接断 0（测试审查 T-6）：① 仗里「偷偷多给 3 块低阶」、n13 节点账也多 3 块——从前的口径把它当
    // 合法战斗收支抵掉、一条不报；现在仗里那一笔先报，节点账（只认 3.2 的 give / take）再报。
    const GameState ghostAfter = driver.fights()[0].after;
    Entry loot = ledger[12];
    loot.after.addItem("material_lingshi", 3);
    driver.mutableFights()[0].after.addItem("material_lingshi", 3);
    std::tie(count, text) = capturedFailures([&] { driver.checkEntry(loot, 0, 1); });
    driver.mutableFights()[0].after = ghostAfter;
    EXPECT_EQ(count, 2) << text;
    EXPECT_NE(text.find("changed inside b12_guiwu_guiwang"), std::string::npos) << text;
    std::tie(count, text) = capturedFailures([&] { driver.checkEntry(ledger[12], 0, 1); });
    EXPECT_EQ(count, 0) << "对照：还原后 n13 的账一条不报\n" << text;
}
} // namespace
