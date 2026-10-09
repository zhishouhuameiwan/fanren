#include <gtest/gtest.h>
#include <algorithm>
#include <string>
#include "BattleHand.h"
#include "ChapterFixture.h"
#include "game/FieldScene.h"
#include "game/MenuScene.h"
#include "game/WorldScene.h"
#include "io/DataLoader.h"

namespace {
using namespace fanren;
using core::GameState;
class Ch09Slice : public ::testing::Test {
protected:
    void SetUp() override {
        const auto ready = app_.init(root_, true);
        ASSERT_TRUE(ready.ok) << ready.error;
        fresh();
    }
    void TearDown() override { app_.shutdown(); }
    GameState& state() { return app_.state(); }
    void fresh() {
        const auto loaded = io::loadGame(test::chapterFixturePath(root_, test::kChapterEightEndingSecond).string());
        ASSERT_TRUE(loaded.ok) << loaded.error;
        state() = loaded.value;
        choices_ = 0;
    }
    void spendTo(const char* item, int count) {
        const int have = state().itemCount(item);
        ASSERT_GE(have, count);
        if (have > count) ASSERT_TRUE(state().removeItem(item, have - count));
    }
    void run(const std::string& file, int pick = 0) {
        ASSERT_TRUE(app_.startEvent("ch09/" + file + ".lua").ok);
        app_.clearSpokenKeys();
        for (int n = 0; n < 8000 && app_.scripts().isRunning(); ++n) {
            app_.tick(1.0 / 60.0);
            ASSERT_EQ(dynamic_cast<game::BattleScene*>(app_.topScene()), nullptr) << "unexpected battle in accounting slice " << file;
            if (!app_.awaitingCommand()) continue;
            script::CommandResult result;
            result.ok = true;
            result.choiceIndex = 0;
            if (app_.spokenKeys().empty()) { result.choiceIndex = pick; ++choices_; }
            app_.clearSpokenKeys();
            app_.completeCommand(result);
            app_.popScene();
        }
        ASSERT_FALSE(app_.scripts().isRunning()) << file;
        app_.tick(1.0 / 60.0);
    }
    std::string root_ = test::chapterNineAssetRoot();
    game::Application app_;
    int choices_ = 0;
};

TEST_F(Ch09Slice, AShortPursePaysThreePillsAndCannotReceiveFreeCurrency) {
    for (int purse : {0, 299, 300}) {
        fresh();
        spendTo("material_lingshi", purse);
        const int pills = state().itemCount("pill_dingyan_dan");
        const int day = state().day;
        run("xingchenge");
        EXPECT_EQ(state().itemCount("material_lingshi"), 0);
        EXPECT_EQ(state().itemCount("pill_dingyan_dan"), pills - (purse < 300 ? 3 : 2));
        EXPECT_EQ(state().flag("ch09.xingchen"), purse < 300 ? 2 : 1);
        EXPECT_EQ(state().day, day + 1);
        EXPECT_EQ(state().itemCount("pill_zhuji_dan"), 16);
        EXPECT_TRUE(state().knowsMagic("magic_ji_hongxianzhen"));
    }
}
TEST_F(Ch09Slice, RefusingTheJadeCostsFortyStonesAndSixExtraRealDays) {
    const int stones = state().itemCount("material_lingshi");
    const int mid = state().itemCount("material_lingshi_zhong");
    const int day = state().day;
    run("shudong", 1);
    EXPECT_EQ(state().flag("ch09.heyuan"), 2);
    EXPECT_EQ(state().itemCount("material_heyuanyu"), 0);
    EXPECT_EQ(state().itemCount("material_lingshi_zhong"), mid + 30);
    EXPECT_EQ(state().realm, rules::Realm::QiRefining3);
    EXPECT_EQ(state().formerRealm, rules::Realm::FoundationMid);
    run("huicheng");
    EXPECT_EQ(state().day, day + 1 + 2 + 6);
    EXPECT_EQ(state().itemCount("material_lingshi"), stones - 40);
    EXPECT_EQ(state().itemCount("material_heyuanyu"), 3);
    run("wangong");
    EXPECT_EQ(state().itemCount("material_heyuanyu"), 0);
}
TEST_F(Ch09Slice, DirectMiddleStonesLeaveLowStonesUntouchedAndEmptyStockStillHasAnExit) {
    for (int mid : {0, 1, 6, 19}) {
        fresh();
        spendTo("material_lingshi_zhong", mid);
        const int low = state().itemCount("material_lingshi");
        const int day = state().day;
        run("qidong", 1);
        EXPECT_EQ(state().itemCount("material_lingshi"), low);
        EXPECT_EQ(state().itemCount("material_lingshi_zhong"), std::max(0, mid - 6));
        EXPECT_EQ(state().itemCount("story_diandao_zhenqi_gai"), 0);
        EXPECT_EQ(state().itemCount("story_danuoyi_ling"), 1);
        EXPECT_EQ(state().day, day);
        EXPECT_EQ(state().flag("ch09.shizhen"), 2);
        EXPECT_EQ(state().flag("ch09.done"), 1);
        EXPECT_EQ(state().mapId, "ch10_gudao");
        EXPECT_EQ(state().position, (core::Point{12, 16}));
    }
}
TEST_F(Ch09Slice, EveryArrayBitIsIdempotentAndAnIncompleteArrayCannotChargeItsEye) {
    state().setFlag("ch09.goucai"); // Isolate the array, without adding any resource or advancing the chapter.
    for (int bit = 0; bit < 4; ++bit) {
        const char* files[] = {"zhenwei_jin", "zhenwei_mu", "zhenwei_shui", "zhenwei_huo"};
        run(files[bit]);
        EXPECT_EQ(state().flag("ch09.zhenqi"), (1 << (bit + 1)) - 1);
        const auto once = test::comparableSaveLines(state());
        run(files[bit]);
        EXPECT_EQ(test::comparableSaveLines(state()), once);
        if (bit < 3) {
            run("zhenyan_jiu");
            EXPECT_EQ(state().flag("ch09.buzhen"), 0);
            EXPECT_EQ(test::comparableSaveLines(state()), once);
        }
    }
    spendTo("material_lingshi_zhong", 0);
    spendTo("material_lingshi", 0);
    run("zhenyan_jiu");
    EXPECT_EQ(state().flag("ch09.buzhen"), 1);
    EXPECT_EQ(state().itemCount("material_lingshi_zhong"), 0);
    EXPECT_EQ(state().itemCount("material_lingshi"), 0);
}
TEST_F(Ch09Slice, TheRealRipeHerbCanPostponeSealingAndActualHarvestReopensProgress) {
    run("zhongsheng");
    run("laozu");
    const int day = state().day;
    run("fengfu", 0);
    EXPECT_EQ(state().flag("ch09.fengfu"), 0);
    EXPECT_EQ(state().day, day);
    const auto map = io::loadTileMap(root_ + "/maps/ch08_dongfu.tmj");
    ASSERT_TRUE(map.ok);
    int hooks = 0;
    for (const auto& o : map.value.objects) if (o.name == "trigger_fengfu") {
        ++hooks;
        EXPECT_TRUE(game::WorldScene::triggerReady(state(), o));
    }
    ASSERT_EQ(hooks, 1);
    // FieldScene harvest is the real panel operation; this slice only isolates the resource predicate.
    game::FieldScene field("field_baiyaoyuan");
    field.onEnter(app_);
    ASSERT_NE(state().findField("field_baiyaoyuan"), nullptr);
    ASSERT_TRUE(field.harvestAt(app_, 5)) << field.feedback();
    EXPECT_EQ(state().itemCountAtLeastAge("herb_xuehong_zhi", 13), 1);
    run("fengfu");
    EXPECT_EQ(state().flag("ch09.fengfu"), 1);
    EXPECT_EQ(state().day, day + 1);
    EXPECT_EQ(state().mapId, "ch09_huangshan");
}
}  // namespace
