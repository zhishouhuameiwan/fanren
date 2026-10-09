#include <gtest/gtest.h>
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include "BattleHand.h"
#include "ChapterFixture.h"
#include "game/AlchemyScene.h"
#include "game/WorldScene.h"
#include "io/DataLoader.h"

namespace {
using namespace fanren;
class Ch08Slice : public ::testing::Test {
protected:
    void SetUp() override {
        const auto ready = app_.init(root_, true);
        ASSERT_TRUE(ready.ok) << ready.error;
        fresh();
    }
    void TearDown() override { app_.shutdown(); }
    void fresh(const char* file = test::kChapterSevenEndingSecond) {
        const auto loaded = io::loadGame(test::chapterFixturePath(root_, file).string());
        ASSERT_TRUE(loaded.ok) << loaded.error;
        app_.state() = loaded.value;
    }
    core::GameState& state() { return app_.state(); }
    void run(const std::string& file, int pick = 0) {
        const auto started = app_.startEvent("ch08/" + file + ".lua");
        ASSERT_TRUE(started.ok) << started.error;
        app_.clearSpokenKeys();
        for (int n = 0; n < 8000 && app_.scripts().isRunning(); ++n) {
            app_.tick(1.0 / 60.0);
            ASSERT_EQ(dynamic_cast<game::BattleScene*>(app_.topScene()), nullptr) << "unexpected battle in " << file;
            if (!app_.awaitingCommand()) continue;
            script::CommandResult answer;
            answer.ok = true;
            answer.choiceIndex = app_.spokenKeys().empty() ? pick : 0;
            app_.clearSpokenKeys();
            app_.completeCommand(answer);
            app_.popScene();
        }
        ASSERT_FALSE(app_.scripts().isRunning()) << file;
        app_.tick(1.0 / 60.0);
    }
    void exhaust(const char* item) {
        const int count = state().itemCount(item);
        if (count > 0) EXPECT_TRUE(state().removeItem(item, count));
    }
    std::string root_ = test::chapterEightAssetRoot();
    game::Application app_;
};

TEST_F(Ch08Slice, EveryArrayBitIsIdempotentAndThreeFlagsCannotCompleteAnArray) {
    const char* guards[] = {"ch08.quqi", "ch08.qb_yuanbing", "ch08.quhun"};
    const char* centers[] = {"zhenyan", "zhulin_zhenyan", "milin_zhenyan"};
    const char* first[] = {"zhenwei_jin", "zhenwei_mu", "zhenwei_shui", "zhenwei_huo"};
    for (int group = 1; group <= 3; ++group) {
        fresh();
        state().setFlag(guards[group - 1]);
        const std::string mask = "ch08.zhenqi" + std::to_string(group);
        const std::string done = "ch08.buzhen" + std::to_string(group);
        // Negative slices deliberately set only the preceding guard, never a completion flag.
        for (int bit = 0; bit < 4; ++bit) {
            const std::string file = group == 1 ? first[bit]
                : std::string(group == 2 ? "zhulin_" : "milin_") + std::to_string(bit + 1);
            run(file);
            EXPECT_EQ(state().flag(mask), (1 << (bit + 1)) - 1);
            const auto once = test::comparableSaveLines(state());
            run(file);
            EXPECT_EQ(test::comparableSaveLines(state()), once);
            if (bit < 3) {
                const auto before = test::comparableSaveLines(state());
                run(centers[group - 1]);
                EXPECT_EQ(state().flag(done), 0);
                EXPECT_EQ(test::comparableSaveLines(state()), before);
            }
        }
        if (group == 1) exhaust("material_lingshi_zhong");
        run(centers[group - 1]);
        EXPECT_EQ(state().flag(done), 1);
        const auto settled = test::comparableSaveLines(state());
        run(centers[group - 1]);
        EXPECT_EQ(test::comparableSaveLines(state()), settled);
    }
}

TEST_F(Ch08Slice, ExhaustedRentAndArrayFundsStillPermitProgressWithoutMintingMoney) {
    state().setFlag("ch08.shixiong");
    exhaust("material_lingshi");
    const int day = state().day;
    const int mid = state().itemCount("material_lingshi_zhong");
    run("dengji", 1);
    EXPECT_EQ(state().flag("ch08.dengji"), 0);
    EXPECT_EQ(state().day, day);
    run("dengji", 0);
    EXPECT_EQ(state().flag("ch08.dengji"), 1);
    EXPECT_EQ(state().day, day + 4);
    EXPECT_EQ(state().itemCount("material_lingshi"), 0);
    EXPECT_EQ(state().itemCount("material_lingshi_zhong"), mid + 3);
    state().setFlag("ch08.lingquan");
    run("kaifu");
    EXPECT_EQ(state().flag("ch08.kaifu"), 1);
    ASSERT_NE(state().findField("field_dongfu"), nullptr);
    EXPECT_EQ(state().findField("field_dongfu")->slots.size(), 4u);
}

TEST_F(Ch08Slice, ArrayEyeUsesHalfTheActualPurseIncludingZeroAndSingleStoneBranches) {
    // Controller M2, 2026-10-09: the live purse, after lawful spending, is the sole basis.
    for (int purse : {0, 1, 2, 3, 20, 21}) {
        SCOPED_TRACE(purse);
        fresh(purse == 21 ? test::kChapterSevenEndingFirst : test::kChapterSevenEndingSecond);
        run("shixiong");
        run("dengji");
        const int registered = state().itemCount("material_lingshi_zhong");
        ASSERT_EQ(registered, purse == 21 ? 21 : 20);
        if (registered > purse) EXPECT_TRUE(state().removeItem("material_lingshi_zhong", registered - purse));
        // Isolate the array eye's accounting; no items, stats or victory flags are injected.
        state().setFlag("ch08.quqi");
        state().setFlag("ch08.zhenqi1", 15);
        const int day = state().day;
        run("zhenyan");
        EXPECT_EQ(state().flag("ch08.buzhen1"), 1);
        EXPECT_EQ(state().itemCount("material_lingshi_zhong"), purse - purse / 2);
        EXPECT_EQ(state().day, day);
        const auto settled = test::comparableSaveLines(state());
        run("zhenyan");
        EXPECT_EQ(test::comparableSaveLines(state()), settled);
    }
}

TEST_F(Ch08Slice, ExhaustedPreparationIsRepeatableLaborThenAnActualAlchemyAttempt) {
    state().setFlag("ch08.gufang");
    state().setFlag("ch08.shuye");
    exhaust("herb_zishen_cao"); exhaust("material_lianqi_yao"); exhaust("pill_lianqi_san");
    const int money = state().itemCount("material_lingshi");
    const int day = state().day;
    run("biguan");
    EXPECT_EQ(state().flag("ch08.biguan"), 0);
    EXPECT_EQ(state().day, day);
    for (int attempt = 0; attempt < 3; ++attempt) {
        exhaust("herb_zishen_cao"); exhaust("material_lianqi_yao"); exhaust("pill_lianqi_san");
        const int before = state().day;
        run("beiyao");
        EXPECT_EQ(state().day, before + 20);
        EXPECT_EQ(state().itemCount("material_lingshi"), money);
        EXPECT_EQ(state().itemCountAtLeastAge("herb_zishen_cao", 100), 1);
        EXPECT_EQ(state().itemCount("material_lianqi_yao"), 1);
        EXPECT_EQ(state().itemCount("pill_lianqi_san"), 0);
        EXPECT_EQ(state().flag("ch08.beiyao_bu"), attempt + 1);
        const auto paid = test::comparableSaveLines(state());
        run("beiyao");
        EXPECT_EQ(test::comparableSaveLines(state()), paid) << "no repeated grant while ingredients remain";
        game::AlchemyScene furnace(rules::CraftKind::Alchemy, 3);
        furnace.onEnter(app_);
        const auto& ids = furnace.recipeIds();
        const auto it = std::find(ids.begin(), ids.end(), "recipe_lianqi_san");
        ASSERT_NE(it, ids.end());
        ASSERT_TRUE(furnace.craftAt(app_, static_cast<int>(it - ids.begin()))) << furnace.feedback();
        EXPECT_EQ(state().itemCountAtLeastAge("herb_zishen_cao", 100), 0);
        EXPECT_EQ(state().itemCount("material_lianqi_yao"), 0);
        EXPECT_TRUE(state().itemCount("pill_lianqi_san") == 0 || state().itemCount("pill_lianqi_san") == 2);
    }
    for (int retry = 0; retry < 8 && state().itemCount("pill_lianqi_san") < 2; ++retry) {
        run("beiyao");
        game::AlchemyScene furnace(rules::CraftKind::Alchemy, 3);
        furnace.onEnter(app_);
        const auto& ids = furnace.recipeIds();
        const auto row = std::find(ids.begin(), ids.end(), "recipe_lianqi_san");
        ASSERT_NE(row, ids.end());
        ASSERT_TRUE(furnace.craftAt(app_, static_cast<int>(row - ids.begin())));
    }
    ASSERT_EQ(state().itemCount("pill_lianqi_san"), 2) << "repeatable recovery must actually allow progression";
    const int readyDay = state().day;
    run("biguan");
    EXPECT_EQ(state().flag("ch08.biguan"), 1);
    EXPECT_EQ(state().day, readyDay + 1440);
    EXPECT_EQ(state().itemCount("pill_lianqi_san"), 0);
    EXPECT_EQ(state().itemCount("pill_zhuji_dan"), 16);
    EXPECT_EQ(state().itemCount("material_lingshi"), money);
}

TEST_F(Ch08Slice, AncientRecipesAreHiddenUntilLearnedAndHaveTheirActualIngredientsAndChances) {
    for (int known : {0, 1}) {
        state().setFlag("ch08.gufang", known);
        game::AlchemyScene panel(rules::CraftKind::Alchemy, 3);
        panel.onEnter(app_);
        for (const char* id : {"recipe_lianqi_san", "recipe_juling_dan"}) {
            EXPECT_EQ(std::count(panel.recipeIds().begin(), panel.recipeIds().end(), id), known);
            ASSERT_TRUE(app_.recipes().count(id));
            const auto& r = app_.recipes().at(id);
            EXPECT_EQ(r.requireFlag, "ch08.gufang");
            EXPECT_EQ(r.productCount, 2);
            EXPECT_EQ(r.requiredProficiency, 30);
            EXPECT_EQ(r.difficulty, std::string(id) == "recipe_lianqi_san" ? 50 : 60);
            EXPECT_EQ(rules::successChance(r, state().alchemyProficiency, state().aptitude, 3),
                      std::string(id) == "recipe_lianqi_san" ? 65 : 55);
            ASSERT_EQ(r.inputs.size(), 2u);
            const bool lianqi = std::string(id) == "recipe_lianqi_san";
            EXPECT_EQ(r.inputs[0].itemId, lianqi ? "herb_zishen_cao" : "herb_xuehong_zhi");
            EXPECT_EQ(r.inputs[0].count, 1);
            EXPECT_EQ(r.inputs[0].minAge, lianqi ? 100 : 300);
            EXPECT_EQ(r.inputs[1].itemId, lianqi ? "material_lianqi_yao" : "material_juling_yao");
            EXPECT_EQ(r.inputs[1].count, 1);
        }
    }
    for (const char* id : {"recipe_huoqiu_fu", "recipe_hushen_fu", "recipe_jinci_fu", "recipe_leiming_fu"}) {
        ASSERT_TRUE(app_.recipes().count(id));
        EXPECT_EQ(app_.recipes().at(id).requireFlag, "story.recipe_later");
    }
}

TEST_F(Ch08Slice, ThousandYearGatePreservesYoungHerbsAndCanBeRetriedAfterFailure) {
    state().setFlag("ch08.kaifu");
    state().addItem("herb_zigui_hua", 1, 999);
    const auto before = test::comparableSaveLines(state());
    run("chucang");
    EXPECT_EQ(state().flag("ch08.qiannian"), 0);
    EXPECT_EQ(test::comparableSaveLines(state()), before);
    state().addItem("herb_zigui_hua", 1, 1000);
    run("chucang");
    EXPECT_EQ(state().flag("ch08.qiannian"), 1);
    EXPECT_EQ(state().itemCountOfAge("herb_zigui_hua", 1000), 1);
    state().setFlag("ch08.midian");
    run("qiyunxiao");
    EXPECT_EQ(state().flag("ch08.qiyunxiao"), 1);
    EXPECT_EQ(state().itemCountOfAge("herb_zigui_hua", 999), 1);
    EXPECT_EQ(state().itemCountOfAge("herb_zigui_hua", 1000), 0);
    EXPECT_EQ(state().itemCount("story_diandao_zhenqi"), 1);
    const auto traded = test::comparableSaveLines(state());
    run("qiyunxiao");
    EXPECT_EQ(test::comparableSaveLines(state()), traded);
}

TEST_F(Ch08Slice, SixRealDefeatsDoNotSetVictoryAndTheMonthlyBattleRemainsRetryable) {
    struct LossSpec { const char* file; const char* guard; const char* done; bool fatal; };
    const LossSpec specs[] = {
        {"xifeng", "ch08.tianheju", "ch08.tuoshen", true},
        {"zhenbian", "ch08.kuilei", "ch08.shouzhen", true},
        {"judong", "ch08.shouzhen", "ch08.zhanzhu", true},
        {"chuzhen", "ch08.qiaoqian", "ch08.daji", false},
        {"zhulinxin", "ch08.huanggong", "ch08.yuehuang", true},
        {"shanding", "ch08.yaolang", "ch08.quhun", true},
    };
    for (const auto& spec : specs) {
        SCOPED_TRACE(spec.file);
        game::Application losing;
        ASSERT_TRUE(losing.init(root_, true).ok);
        losing.state() = state();
        // Failure slice: fixture stats, preceding guard only, no healing or stat edits.
        losing.state().setFlag(spec.guard);
        const int day = losing.state().day;
        ASSERT_TRUE(losing.startEvent(std::string("ch08/") + spec.file + ".lua").ok);
        losing.clearSpokenKeys();
        int battles = 0;
        for (int frame = 0; frame < 8000 && losing.scripts().isRunning(); ++frame) {
            losing.tick(1.0 / 60.0);
            if (auto* scene = dynamic_cast<game::BattleScene*>(losing.topScene())) {
                if (scene->battle().phase() != core::battle::BattlePhase::Ongoing) continue;
                test::BattleHand hand(losing);
                for (int turn = 0; turn < 8000 && scene->battle().phase() == core::battle::BattlePhase::Ongoing; ++turn) {
                    const int actor = scene->runToAllyTurn();
                    if (actor < 0) break;
                    core::battle::Action action;
                    action.kind = core::battle::ActionKind::Defend;
                    action.actorIndex = actor;
                    ASSERT_TRUE(hand.issue(*scene, action));
                }
                ++battles;
                EXPECT_EQ(scene->battle().phase(), core::battle::BattlePhase::Lost);
                std::cout << "[ch08 loss] " << spec.file << " round=" << scene->battle().round() << '\n';
                continue;
            }
            if (!losing.awaitingCommand()) continue;
            script::CommandResult result;
            result.ok = true;
            result.choiceIndex = 0;
            losing.clearSpokenKeys();
            losing.completeCommand(result);
            losing.popScene();
        }
        losing.tick(1.0 / 60.0);
        EXPECT_EQ(battles, 1);
        EXPECT_EQ(losing.state().flag(spec.done), 0);
        EXPECT_EQ(losing.quitRequested(), spec.fatal);
        if (!spec.fatal) {
            EXPECT_EQ(losing.state().day, day + 30);
            const auto map = io::loadTileMap(root_ + "/maps/ch08_jinguyuan.tmj");
            ASSERT_TRUE(map.ok);
            int hooks = 0;
            for (const auto& o : map.value.objects) if (o.name == "trigger_chuzhen") {
                ++hooks;
                EXPECT_TRUE(game::WorldScene::triggerReady(losing.state(), o));
            }
            EXPECT_EQ(hooks, 1);
        }
        losing.shutdown();
    }
}
}  // namespace
