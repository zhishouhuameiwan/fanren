#include "Ch10Driver.h"

namespace {
using namespace fanren;
using namespace fanren::test::ch10;

void verifyRun(Driver& driver,const char* fixture) {
    driver.checkEnding();
    ASSERT_EQ(driver.fights().size(),4u);
    for (const auto& fight : driver.fights()) EXPECT_EQ(fight.phase,BattlePhase::Won) << fight.id;
    EXPECT_EQ(driver.fights()[0].allies,std::vector<std::string>{"hanli"});
    EXPECT_EQ(driver.fights()[1].allies,(std::vector<std::string>{
        "qu_hun_huashen","feng_sanniang","qing_suanzi","yan_daoyou"}));
    EXPECT_EQ(driver.fights()[2].allies,(std::vector<std::string>{"hanli","qu_hun_huashen"}));
    EXPECT_EQ(driver.fights()[3].allies,std::vector<std::string>{"qu_hun_shadan"});
    for (const std::size_t index : {1u,3u}) {
        const auto& fight=driver.fights()[index];
        EXPECT_EQ(test::comparableSaveLines(fight.before),test::comparableSaveLines(fight.after))
            << fight.id << " hero_absent battle must preserve hero, party and bag";
    }
    EXPECT_LE(driver.fights()[3].rounds,4);
    const auto& cards=driver.app().cardLog();
    ASSERT_GE(cards.size(),2u);
    EXPECT_EQ(cards[cards.size()-2],(game::CardRequest{game::CardKind::Closing,10}));
    EXPECT_EQ(cards.back(),(game::CardRequest{game::CardKind::ToBeContinued,0}));
    ASSERT_FALSE(::testing::Test::HasFailure());
    const test::TempDir temporary("fanren_ch10_actual_ending");
    const auto path=temporary.path()/"ending.sav";
    const auto saved=io::saveGame(driver.state(),path.string());
    ASSERT_TRUE(saved.ok) << saved.error;
    const auto loaded=io::loadGame(path.string());
    ASSERT_TRUE(loaded.ok) << loaded.error;
    EXPECT_EQ(test::comparableSaveLines(loaded.value),test::comparableSaveLines(driver.state()));
    Application resumed;
    const auto ready=resumed.init(root().string(),true);
    ASSERT_TRUE(ready.ok) << ready.error;
    const auto continued=resumed.continueJourney(path.string());
    ASSERT_TRUE(continued.ok) << continued.error;
    EXPECT_EQ(test::comparableSaveLines(resumed.state()),test::comparableSaveLines(driver.state()));
    resumed.shutdown();
    ASSERT_FALSE(::testing::Test::HasFailure());
    const auto verdict=test::settleAgainstFixture(driver.state(),root().string(),fixture,
        test::kWriteChapterTenFixturesEnv,"Chapter 11");
    EXPECT_TRUE(verdict.problem.empty()) << verdict.problem;
}

TEST(Ch10Walkthrough, FirstSideUsesTheActualC9EndingAndEveryPlayerWindow) {
    Driver driver;
    ASSERT_TRUE(driver.load("ch09-end-first.sav")) << driver.problem();
    ASSERT_TRUE(driver.runTo(32)) << driver.problem();
    verifyRun(driver,test::kChapterTenEndingFirst);
}

TEST(Ch10Walkthrough, SecondSideUsesTheActualC9EndingAndTheSameStoryChoices) {
    Driver driver;
    ASSERT_TRUE(driver.load("ch09-end-second.sav")) << driver.problem();
    ASSERT_TRUE(driver.runTo(32)) << driver.problem();
    verifyRun(driver,test::kChapterTenEndingSecond);
}

TEST(Ch10Acceptance, BothActualInheritedPursesKeepTheirDifference) {
    std::array<int,2> lowStart{},lowEnd{},midStart{},midEnd{};
    int index=0;
    for (const char* file : {"ch09-end-first.sav","ch09-end-second.sav"}) {
        Driver driver;
        ASSERT_TRUE(driver.load(file)) << driver.problem();
        lowStart[static_cast<std::size_t>(index)]=driver.state().itemCount("material_lingshi");
        midStart[static_cast<std::size_t>(index)]=driver.state().itemCount("material_lingshi_zhong");
        ASSERT_TRUE(driver.runTo(32)) << driver.problem();
        driver.checkEnding();
        lowEnd[static_cast<std::size_t>(index)]=driver.state().itemCount("material_lingshi");
        midEnd[static_cast<std::size_t>(index)]=driver.state().itemCount("material_lingshi_zhong");
        ++index;
    }
    EXPECT_EQ(lowEnd[0]-lowEnd[1],lowStart[0]-lowStart[1]);
    // Mid-grade consumption is measured per battle by the walkthrough, not estimated here.
    EXPECT_GT(midEnd[0],midStart[0]);
    EXPECT_GT(midEnd[1],midStart[1]);
}
} // namespace
