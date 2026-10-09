#include "Ch10Driver.h"

namespace {
using namespace fanren;
using namespace fanren::test::ch10;

TEST(Ch10Language, ZeroOneAndThreeActualLessonsArriveOnTheSameDayWithDifferentInformation) {
    std::vector<int> arrivalDays;
    for (const int lessons : {0,1,3}) {
        Driver driver;
        driver.lessons=lessons;
        ASSERT_TRUE(driver.load()) << driver.problem();
        ASSERT_TRUE(driver.runTo(6)) << driver.problem();
        arrivalDays.push_back(driver.state().day);
        EXPECT_EQ(driver.state().day-driver.start().day,5);
        EXPECT_EQ(driver.state().flag("ch10.xuehua"),lessons==3 ? 7 : lessons==1 ? 1 : 0);
        EXPECT_EQ(driver.state().flag("ch10.yuyan"),lessons==3 ? 1 : 0);
        const auto available=driver.app().pathActionsFor("npc_gangkou_xiaofan");
        ASSERT_EQ(available.size(),1u);
        EXPECT_EQ(available.front()->id,lessons==3 ? "gangkou_xiaofan_dating" : "gangkou_xiaofan_tingbudong_dating");
        EXPECT_TRUE(driver.spoke(lessons==3 ? "ch10.kaoan.land.clear" : "ch10.kaoan.land.garbled"));
        driver.clearKeys();
        ASSERT_TRUE(driver.runTo(7)) << driver.problem();
        EXPECT_TRUE(driver.spoke(lessons==3 ? "ch10.dengji.ask.clear" : "ch10.dengji.ask.garbled"));
        ASSERT_TRUE(driver.runTo(8)) << driver.problem();
        EXPECT_EQ(driver.state().flag("ch10.yuyan"),2);
    }
    ASSERT_EQ(arrivalDays.size(),3u);
    EXPECT_EQ(arrivalDays[0],arrivalDays[1]);
    EXPECT_EQ(arrivalDays[1],arrivalDays[2]);
}

TEST(Ch10Language, LessonsCannotStartBeforeNightTalkAndTheFourthVisitCannotAdvanceTime) {
    Driver driver;
    ASSERT_TRUE(driver.load()) << driver.problem();
    ASSERT_TRUE(driver.runTo(4)) << driver.problem();
    const int early=driver.state().day;
    ASSERT_TRUE(driver.press("npc_wang_changqing")) << driver.problem();
    EXPECT_EQ(driver.state().day,early);
    EXPECT_EQ(driver.state().flag("ch10.xuehua"),0);
    EXPECT_TRUE(driver.spoke("ch10.xuehua.early"));
    ASSERT_TRUE(driver.runTo(5)) << driver.problem();
    const int day=driver.state().day;
    for (const int mask : {1,3,7}) {
        const int before=driver.state().day;
        ASSERT_TRUE(driver.press("npc_wang_changqing")) << driver.problem();
        EXPECT_EQ(driver.state().flag("ch10.xuehua"),mask);
        EXPECT_EQ(driver.state().day,before+1);
    }
    ASSERT_TRUE(driver.press("npc_wang_changqing")) << driver.problem();
    EXPECT_EQ(driver.state().day,day+3);
    EXPECT_EQ(driver.state().flag("ch10.yuyan"),1);
    EXPECT_TRUE(driver.spoke("ch10.xuehua.done"));
}

TEST(Ch10Slice, TheRealmEightGateKeepsAllPillsAndTheRealHookCanBeRetried) {
    Driver driver;
    ASSERT_TRUE(driver.load()) << driver.problem();
    ASSERT_TRUE(driver.runTo(16)) << driver.problem();
    ASSERT_TRUE(driver.reachNine(8)) << driver.problem();
    const GameState before=driver.state();
    driver.setCurrentNode(17);
    ASSERT_TRUE(driver.press("trigger_zhuji")) << driver.problem();
    EXPECT_EQ(driver.state().flag("ch10.zhuji"),0);
    EXPECT_EQ(driver.state().itemCount("pill_zhuji_dan"),before.itemCount("pill_zhuji_dan"));
    EXPECT_EQ(driver.state().day,before.day);
    EXPECT_EQ(driver.state().realm,rules::Realm::QiRefining8);
    EXPECT_TRUE(game::WorldScene::triggerReady(driver.state(),driver.object("trigger_zhuji")));
    ASSERT_TRUE(driver.runTo(17)) << driver.problem();
    EXPECT_EQ(driver.state().realm,rules::Realm::FoundationEarly);
    EXPECT_EQ(driver.state().itemCount("pill_zhuji_dan"),13);
}

TEST(Ch10Slice, ThreeFlagsCannotFinishEitherFormationButTheFourthCan) {
    Driver driver;
    ASSERT_TRUE(driver.load()) << driver.problem();
    ASSERT_TRUE(driver.runTo(14)) << driver.problem();
    ASSERT_TRUE(driver.formation(false,3)) << driver.problem();
    EXPECT_EQ(driver.state().flag("ch10.zhenqi"),7);
    ASSERT_TRUE(driver.press("trigger_zhenyan")) << driver.problem();
    EXPECT_EQ(driver.state().flag("ch10.buzhen"),0);
    EXPECT_TRUE(game::WorldScene::triggerReady(driver.state(),driver.object("trigger_zhenyan")));
    ASSERT_TRUE(driver.runTo(26)) << driver.problem();
    driver.choose({1});
    ASSERT_TRUE(driver.press("trigger_jiaoshi")) << driver.problem();
    ASSERT_TRUE(driver.formation(true,3)) << driver.problem();
    const int day=driver.state().day;
    ASSERT_TRUE(driver.press("trigger_bishui_yan")) << driver.problem();
    EXPECT_EQ(driver.state().flag("ch10.bishui_cheng"),0);
    EXPECT_EQ(driver.state().day,day);
    ASSERT_TRUE(driver.press("trigger_bishui_bei")) << driver.problem();
    ASSERT_TRUE(driver.press("trigger_bishui_yan")) << driver.problem();
    EXPECT_EQ(driver.state().flag("ch10.bishui_cheng"),1);
    EXPECT_EQ(driver.state().day,day+1);
}

TEST(Ch10Slice, ActualFacilityMeditationIsEightfoldAndThePanelShowsNinetyFive) {
    Driver driver;
    ASSERT_TRUE(driver.load()) << driver.problem();
    ASSERT_TRUE(driver.runTo(16)) << driver.problem();
    const GameState reached=driver.state();
    ASSERT_TRUE(driver.open("facility_jingshi")) << driver.problem();
    auto* panel=dynamic_cast<CultivationScene*>(driver.app().topScene());
    ASSERT_NE(panel,nullptr);
    EXPECT_NE(panel->reclimbLine(driver.state()).find("8"),std::string::npos);
    const auto boosted=panel->meditateFor(driver.app(),60);
    driver.close();
    ASSERT_TRUE(driver.resumeReached(reached,17)) << driver.problem();
    // Design 15.6 explicitly specifies this one-field counterfactual, from a reached save.
    driver.state().formerRealm=rules::Realm::Mortal;
    ASSERT_TRUE(driver.open("facility_jingshi")) << driver.problem();
    panel=dynamic_cast<CultivationScene*>(driver.app().topScene());
    ASSERT_NE(panel,nullptr);
    EXPECT_TRUE(panel->reclimbLine(driver.state()).empty());
    const auto ordinary=panel->meditateFor(driver.app(),60);
    driver.close();
    ASSERT_GT(ordinary.cultivation,0);
    EXPECT_GE(boosted.cultivation,8*ordinary.cultivation);
    EXPECT_LT(boosted.cultivation,8*ordinary.cultivation+8);
    ASSERT_TRUE(driver.resumeReached(reached,17)) << driver.problem();
    ASSERT_TRUE(driver.reachNine()) << driver.problem();
    const auto before=driver.state();
    ASSERT_TRUE(driver.open("facility_jingshi")) << driver.problem();
    panel=dynamic_cast<CultivationScene*>(driver.app().topScene());
    ASSERT_NE(panel,nullptr);
    const auto rows=CultivationScene::buildMainItems(driver.state());
    ASSERT_GE(rows.size(),2u);
    EXPECT_EQ(rows[1].disabledReason,game::cultivationLexicon(game::PanelStage::Immortal).pushAtStoryCapReclimb);
    EXPECT_EQ(panel->breakthrough(driver.app()).blocked,rules::BreakthroughBlock::StoryCap);
    EXPECT_EQ(driver.state().cultivation,before.cultivation);
    EXPECT_EQ(driver.state().realm,before.realm);
    driver.close();
}

TEST(Ch10Slice, OneActuallyCraftedPillCannotOpenTheTwoPillGate) {
    Driver driver;
    ASSERT_TRUE(driver.load()) << driver.problem();
    ASSERT_TRUE(driver.runTo(18)) << driver.problem();
    ASSERT_TRUE(driver.preparePills()) << driver.problem();
    ASSERT_EQ(driver.state().itemCount("pill_zhenyuan_dan"),2);
    // A shortage perturbation consumes a real crafted product; it never grants one.
    ASSERT_TRUE(driver.state().removeItem("pill_zhenyuan_dan",1));
    const auto before=driver.state();
    driver.setCurrentNode(19);
    ASSERT_TRUE(driver.press("trigger_sanzhuan")) << driver.problem();
    EXPECT_EQ(driver.state().itemCount("pill_zhenyuan_dan"),1);
    EXPECT_EQ(driver.state().flag("ch10.chuguan"),0);
    EXPECT_EQ(driver.state().day,before.day);
    EXPECT_TRUE(game::WorldScene::triggerReady(driver.state(),driver.object("trigger_sanzhuan")));
}

TEST(Ch10Slice, APlantedSeedPreventsTheShortageBranchFromGivingMoreSeeds) {
    Driver driver;
    ASSERT_TRUE(driver.load()) << driver.problem();
    ASSERT_TRUE(driver.runTo(18)) << driver.problem();
    ASSERT_TRUE(driver.open("facility_yaoyuan_xh")) << driver.problem();
    auto* field=dynamic_cast<FieldScene*>(driver.app().topScene());
    ASSERT_NE(field,nullptr);
    ASSERT_TRUE(field->plantAt(driver.app(),0,"herb_zishen_cao"));
    driver.close();
    const int purple=driver.state().itemCount("herb_zishen_cao");
    const int red=driver.state().itemCount("herb_xuehong_zhi");
    ASSERT_TRUE(driver.press("trigger_sanzhuan")) << driver.problem();
    EXPECT_EQ(driver.state().itemCount("herb_zishen_cao"),purple);
    EXPECT_EQ(driver.state().itemCount("herb_xuehong_zhi"),red);
    EXPECT_EQ(driver.state().flag("ch10.chuguan"),0);
}

TEST(Ch10Slice, TheEmptyFieldAndEmptyHerbBagRefillOnlyThroughTheRealShortageScript) {
    Driver driver;
    ASSERT_TRUE(driver.load()) << driver.problem();
    ASSERT_TRUE(driver.runTo(18)) << driver.problem();
    for (const char* herb : {"herb_zishen_cao","herb_xuehong_zhi"}) {
        const int count=driver.state().itemCount(herb);
        if (count) ASSERT_TRUE(driver.state().removeItem(herb,count));
    }
    ASSERT_EQ(driver.state().itemCount("pill_zhenyuan_dan"),0);
    ASSERT_TRUE(driver.press("trigger_sanzhuan")) << driver.problem();
    EXPECT_EQ(driver.state().itemCount("herb_zishen_cao"),1);
    EXPECT_EQ(driver.state().itemCount("herb_xuehong_zhi"),1);
    EXPECT_EQ(driver.state().flag("ch10.chuguan"),0);
    ASSERT_TRUE(driver.press("trigger_sanzhuan")) << driver.problem();
    EXPECT_EQ(driver.state().itemCount("herb_zishen_cao"),1);
    EXPECT_EQ(driver.state().itemCount("herb_xuehong_zhi"),1);
}

TEST(Ch10Slice, RealWindowTimeIsAbsorbedAndLateWindowsUseOnlyTheMinimumYear) {
    Driver driver;
    ASSERT_TRUE(driver.load()) << driver.problem();
    ASSERT_TRUE(driver.runTo(18)) << driver.problem();
    ASSERT_TRUE(driver.preparePills()) << driver.problem();
    const GameState reached=driver.state();
    int normalDay=0;
    for (const int extra : {0,1000}) {
        ASSERT_TRUE(driver.resumeReached(reached,19)) << driver.problem();
        if (extra) ASSERT_TRUE(driver.sit(extra)) << driver.problem();
        ASSERT_TRUE(driver.press("trigger_sanzhuan")) << driver.problem();
        EXPECT_EQ(driver.state().day-driver.state().flag("ch10.kaifu_ri"),7560);
        if (extra==0) normalDay=driver.state().day;
        else EXPECT_EQ(driver.state().day,normalDay);
    }
    for (const int elapsed : {7201,8281}) {
        ASSERT_TRUE(driver.resumeReached(reached,19)) << driver.problem();
        const int missing=elapsed-(driver.state().day-driver.state().flag("ch10.kaifu_ri"));
        ASSERT_GT(missing,0);
        ASSERT_TRUE(driver.sit(missing)) << driver.problem();
        const int before=driver.state().day;
        ASSERT_TRUE(driver.press("trigger_sanzhuan")) << driver.problem();
        EXPECT_EQ(driver.state().day,before+360);
        EXPECT_EQ(driver.spoke("ch10.sanzhuan.long"),elapsed==8281);
    }
}

TEST(Ch10Slice, ZeroAndFiftyStonePursesStillPayWhatTheyHaveAndProgress) {
    Driver driver;
    ASSERT_TRUE(driver.load()) << driver.problem();
    ASSERT_TRUE(driver.runTo(12)) << driver.problem();
    ASSERT_TRUE(driver.state().removeItem("material_lingshi",driver.state().itemCount("material_lingshi")));
    driver.setCurrentNode(13);
    ASSERT_TRUE(driver.press("trigger_matou")) << driver.problem();
    EXPECT_EQ(driver.state().flag("ch10.zhenzhang"),1);
    EXPECT_EQ(driver.state().itemCount("material_lingshi"),0);
    EXPECT_TRUE(driver.spoke("ch10.matou.zero"));
    Driver fifty;
    ASSERT_TRUE(fifty.load()) << fifty.problem();
    ASSERT_TRUE(fifty.runTo(18)) << fifty.problem();
    ASSERT_TRUE(fifty.preparePills()) << fifty.problem();
    const int excess=fifty.state().itemCount("material_lingshi")-50;
    ASSERT_GE(excess,0);
    ASSERT_TRUE(fifty.state().removeItem("material_lingshi",excess));
    fifty.setCurrentNode(19);
    ASSERT_TRUE(fifty.press("trigger_sanzhuan")) << fifty.problem();
    EXPECT_EQ(fifty.state().itemCount("material_lingshi"),0);
    EXPECT_EQ(fifty.state().flag("ch10.chuguan"),1);
    EXPECT_TRUE(fifty.spoke("ch10.sanzhuan.short"));
}

TEST(Ch10Slice, ARealArenaLossRestoresThePuppetAndLeavesTheHookUnburned) {
    Driver driver;
    ASSERT_TRUE(driver.load()) << driver.problem();
    ASSERT_TRUE(driver.runTo(9)) << driver.problem();
    driver.setCurrentNode(10);
    driver.loseNextFight=true;
    ASSERT_TRUE(driver.press("trigger_leitai")) << driver.problem();
    ASSERT_EQ(driver.fights().size(),1u);
    EXPECT_EQ(driver.fights().front().phase,BattlePhase::Lost);
    EXPECT_EQ(partyIds(driver.state()),std::vector<std::string>{"kuilei_shou"});
    EXPECT_EQ(driver.state().flag("ch10.leitai"),0);
    EXPECT_FALSE(driver.app().quitRequested());
    EXPECT_TRUE(game::WorldScene::triggerReady(driver.state(),driver.object("trigger_leitai")));
}

TEST(Ch10Slice, TheOtherLateChoiceKeepsSameFourthYearAndTheResourceAccount) {
    Driver driver;
    driver.continueFourthYear=true;
    ASSERT_TRUE(driver.load()) << driver.problem();
    ASSERT_TRUE(driver.runTo(32)) << driver.problem();
    driver.checkEnding();
    EXPECT_EQ(driver.state().flag("ch10.shadan"),2);
}
} // namespace
