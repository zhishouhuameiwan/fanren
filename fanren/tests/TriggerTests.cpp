// once 触发器的时序验收。
//
// 要证的是两件互相拉扯的事，缺一不可：
//
//   1. 提前 return 的脚本**不会**烧掉 once 触发器。引擎此前在 startEvent 之前就把
//      记号打上（markTriggered 先于 startEvent），于是「查前置 → 说一句还不到时候
//      → return」这种脚本，被踩到的那一瞬间触发器就永久没了；玩家后来满足条件再
//      走回来，这一格已经不响。关卡那边只能靠「凡会提前 return 的脚本一律不设
//      once」这条口头约定绕开，而约定必然失效。
//
//   2. 同时**绝不能**重复触发。记号往后挪，就多出一段「脚本在跑、记号还没落」的
//      空窗；防重入改由 tryStep / interact 开头那道「脚本在跑就不受理」来担。
//      这段空窗正是本文件里 SteppingAgainWhileTheScriptRunsStartsNothing 那一条要钉死的地方。
//
// 驱动的是玩家实际跑的那一整条链路：真地图 → WorldScene::tryStep → Application →
// ScriptHost → 真 api.lua。只测 triggerReady 这个纯函数的话，时序上的错一条也测不出来。
#include <gtest/gtest.h>

#include <filesystem>
#include <string>
#include <system_error>

#include "TempDir.h"
#include "core/model/Types.h"
#include "game/Application.h"
#include "game/WorldScene.h"

namespace {

namespace fs = std::filesystem;

using fanren::core::MapObject;
using fanren::core::Point;
using fanren::game::Application;
using fanren::game::WorldScene;

// 夹具地图与夹具脚本约定好的名字。地图见 tests/maps/t_trigger_gate.tmj。
constexpr const char* kMapId = "t_trigger_gate";
constexpr const char* kStepTrigger = "trigger_step_gate";      // mode=enter，在 (4,1)
constexpr const char* kPressTrigger = "trigger_press_gate";    // mode=interact，在 (7,1)
constexpr const char* kOpenFlag = "t.gate_open";               // 脚本的前置
constexpr const char* kDoneFlag = "t.gate_done";               // 脚本的完成旗标 = set_flag
constexpr const char* kCounterItem = "herb_qingfeng_cao";      // 脚本起一次给一株

fs::path& tempAssetRoot() {
    static fs::path path;
    return path;
}

fs::path findProjectRoot() {
    fs::path dir = fs::current_path();
    for (int depth = 0; depth < 8; ++depth) {
        if (fs::exists(dir / "scripts" / "common" / "api.lua") && fs::is_directory(dir / "data")) {
            return dir;
        }
        const fs::path parent = dir.parent_path();
        if (parent.empty() || parent == dir) break;
        dir = parent;
    }
    return {};
}

class TriggerTest : public ::testing::Test {
protected:
    // 自己搭一套资源根：夹具地图与夹具脚本不该塞进正式的 maps/ 与 scripts/，
    // 但又必须和真的 api.lua、真的 data/ 同根，测的才是同一条链路。
    static void SetUpTestSuite() {
        const fs::path source = findProjectRoot();
        ASSERT_FALSE(source.empty()) << "找不到工程根目录（应含 scripts/common/api.lua 与 data/）";

        // 每次跑都换一个目录名：写死成 %TEMP%\fanren_trigger_root 的话，两个测试
        // 进程同时跑就会互相 remove_all 掉对方正在读的 data/。口径见 tests/TempDir.h。
        fs::path& root = tempAssetRoot();
        root = fanren::test::uniqueTempPath("fanren_trigger_root");
        std::error_code ec;
        fs::remove_all(root, ec);
        fs::create_directories(root / "scripts" / "common", ec);
        fs::create_directories(root / "scripts" / "t", ec);

        fs::copy(source / "data", root / "data", fs::copy_options::recursive, ec);
        ASSERT_FALSE(ec) << "复制 data/ 失败: " << ec.message();
        // 连 tilesets/ 一起搬：TileMapLoader 会核对 tileset 文件确实存在。
        fs::copy(source / "maps", root / "maps", fs::copy_options::recursive, ec);
        ASSERT_FALSE(ec) << "复制 maps/ 失败: " << ec.message();

        fs::copy_file(source / "scripts" / "common" / "api.lua",
                      root / "scripts" / "common" / "api.lua",
                      fs::copy_options::overwrite_existing, ec);
        ASSERT_FALSE(ec) << "复制 api.lua 失败: " << ec.message();

        for (const auto& entry : fs::directory_iterator(source / "tests" / "scripts")) {
            if (entry.path().extension() != ".lua") continue;
            fs::copy_file(entry.path(), root / "scripts" / "t" / entry.path().filename(),
                          fs::copy_options::overwrite_existing, ec);
            ASSERT_FALSE(ec) << "复制 " << entry.path().string() << " 失败: " << ec.message();
        }
        for (const auto& entry : fs::directory_iterator(source / "tests" / "maps")) {
            if (entry.path().extension() != ".tmj") continue;
            fs::copy_file(entry.path(), root / "maps" / entry.path().filename(),
                          fs::copy_options::overwrite_existing, ec);
            ASSERT_FALSE(ec) << "复制 " << entry.path().string() << " 失败: " << ec.message();
        }
    }

    static void TearDownTestSuite() {
        std::error_code ec;
        fs::remove_all(tempAssetRoot(), ec);
    }

    void SetUp() override {
        auto ready = app_.init(tempAssetRoot().string(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
        auto loaded = app_.loadMap(kMapId, std::string{});
        ASSERT_TRUE(loaded.ok) << loaded.error;
    }

    void TearDown() override { app_.shutdown(); }

    fanren::core::GameState& state() { return app_.state(); }

    const MapObject* object(const std::string& name) const {
        const fanren::core::TileMap* map = app_.currentMap();
        if (map == nullptr) return nullptr;
        for (const MapObject& candidate : map->objects) {
            if (candidate.name == name) return &candidate;
        }
        return nullptr;
    }

    // 像主循环那样把脚本推到结束：对话框要人按键，这里替玩家按。
    // maxFrames 是死循环闸：脚本卡住时宁可测试失败，也不要挂住 ctest。
    void pumpUntilIdle(int maxFrames = 256) {
        for (int frame = 0; frame < maxFrames && app_.scripts().isRunning(); ++frame) {
            app_.tick(1.0 / 60.0);
            if (app_.awaitingCommand()) {
                fanren::script::CommandResult result;
                result.ok = true;
                app_.completeCommand(result);
            }
        }
        ASSERT_FALSE(app_.scripts().isRunning()) << "脚本没能跑到结束";
    }

    // 走到触发格上：先站到它左边一格，再往右迈一步。返回这一步有没有走成。
    bool stepOntoStepTrigger() {
        const MapObject* trigger = object(kStepTrigger);
        EXPECT_NE(trigger, nullptr);
        state().position = Point{trigger->position.x - 1, trigger->position.y};
        return world_.tryStep(app_, 1, 0);
    }

    int scriptRuns() { return app_.state().itemCount(kCounterItem); }

    Application app_;
    WorldScene world_;
};

// ---------------------------------------------------------------------------
// 缺陷 A 的正面要求：提前 return 不烧触发器
// ---------------------------------------------------------------------------

TEST_F(TriggerTest, EarlyReturningScriptLeavesTheOnceTriggerArmed) {
    const MapObject* trigger = object(kStepTrigger);
    ASSERT_NE(trigger, nullptr);
    ASSERT_EQ(trigger->property("once"), "true");
    ASSERT_EQ(trigger->property("set_flag"), kDoneFlag);

    // 前置不满足：脚本只说一句「还不到时候」就 return。
    state().flags.erase(kOpenFlag);
    ASSERT_TRUE(WorldScene::triggerReady(state(), *trigger)) << "首次应当可触发";

    ASSERT_TRUE(stepOntoStepTrigger());
    EXPECT_TRUE(app_.scripts().isRunning()) << "踩上去应当起了脚本";
    pumpUntilIdle();

    EXPECT_EQ(scriptRuns(), 1) << "脚本应当恰好起过一次";
    EXPECT_EQ(state().flag(kDoneFlag), 0) << "提前 return 的脚本不该置完成旗标";
    EXPECT_TRUE(WorldScene::triggerReady(state(), *object(kStepTrigger)))
        << "这一幕没演成，触发器必须留着——烧掉就意味着这段剧情永远拿不到了";
}

TEST_F(TriggerTest, TheTriggerStillFiresAfterThePlayerSatisfiesThePrerequisite) {
    // 第一次踩：条件不满足，被劝回。
    state().flags.erase(kOpenFlag);
    ASSERT_TRUE(stepOntoStepTrigger());
    pumpUntilIdle();
    ASSERT_EQ(state().flag(kDoneFlag), 0);

    // 条件满足之后再走回来：这一幕必须真的演得到。这正是旧实现拿不回来的东西。
    state().setFlag(kOpenFlag, 1);
    ASSERT_TRUE(stepOntoStepTrigger());
    EXPECT_TRUE(app_.scripts().isRunning());
    pumpUntilIdle();

    EXPECT_EQ(scriptRuns(), 2);
    EXPECT_EQ(state().flag(kDoneFlag), 1) << "演完了，完成旗标该落地";
    EXPECT_FALSE(WorldScene::triggerReady(state(), *object(kStepTrigger)))
        << "演完之后这一格就该沉默了";
}

TEST_F(TriggerTest, APlayedOnceTriggerNeverFiresAgain) {
    // 防重复触发的另一半：演完之后反复踩，一次也不许再起。
    state().setFlag(kOpenFlag, 1);
    ASSERT_TRUE(stepOntoStepTrigger());
    pumpUntilIdle();
    ASSERT_EQ(scriptRuns(), 1);
    ASSERT_EQ(state().flag(kDoneFlag), 1);

    for (int again = 0; again < 3; ++again) {
        ASSERT_TRUE(stepOntoStepTrigger());
        EXPECT_FALSE(app_.scripts().isRunning()) << "第 " << again + 2 << " 次踩不该再起脚本";
    }
    EXPECT_EQ(scriptRuns(), 1) << "脚本仍然只该起过那一次";
}

// ---------------------------------------------------------------------------
// 记号往后挪带出来的那段空窗：防重入
// ---------------------------------------------------------------------------

TEST_F(TriggerTest, SteppingAgainWhileTheScriptRunsStartsNothing) {
    state().flags.erase(kOpenFlag);
    ASSERT_TRUE(stepOntoStepTrigger());
    ASSERT_TRUE(app_.scripts().isRunning());
    const Point standing = state().position;

    // 记号还没落，triggerReady 这会儿仍然是真的——所以挡住第二次的只能是
    // 「脚本在跑就不受理」这一道。它一松，这段空窗就是重复触发。
    EXPECT_TRUE(WorldScene::triggerReady(state(), *object(kStepTrigger)));
    EXPECT_FALSE(world_.tryStep(app_, -1, 0)) << "脚本在跑时不该受理移动";
    EXPECT_EQ(state().position, standing) << "更不该把玩家挪走";

    // 脚本中途 teleport 的形状：位置被改到触发格旁边，再踩一次。
    const MapObject* trigger = object(kStepTrigger);
    state().position = Point{trigger->position.x - 1, trigger->position.y};
    EXPECT_FALSE(world_.tryStep(app_, 1, 0)) << "脚本在跑时踩上去也不该再起一次";
    EXPECT_FALSE(world_.interact(app_)) << "按确认键同样不该受理";

    state().position = standing;
    pumpUntilIdle();
    EXPECT_EQ(scriptRuns(), 1) << "整段空窗里脚本只许起过一次";
}

TEST_F(TriggerTest, InteractTriggerObeysTheSameTwoRules) {
    const MapObject* trigger = object(kPressTrigger);
    ASSERT_NE(trigger, nullptr);
    ASSERT_EQ(trigger->property("mode"), "interact");

    // 站到它左边，面朝右。
    const auto faceIt = [&] {
        state().position = Point{trigger->position.x - 1, trigger->position.y};
        state().facing = 1;
    };

    state().flags.erase(kOpenFlag);
    faceIt();
    ASSERT_TRUE(world_.interact(app_));
    pumpUntilIdle();
    EXPECT_EQ(state().flag(kDoneFlag), 0);
    EXPECT_TRUE(WorldScene::triggerReady(state(), *object(kPressTrigger)))
        << "交互型触发同样不该被一次提前 return 烧掉";

    state().setFlag(kOpenFlag, 1);
    faceIt();
    ASSERT_TRUE(world_.interact(app_));
    pumpUntilIdle();
    EXPECT_EQ(state().flag(kDoneFlag), 1);

    faceIt();
    EXPECT_FALSE(world_.interact(app_)) << "演完之后再按就不该有反应了";
    EXPECT_EQ(scriptRuns(), 2);
}

// ---------------------------------------------------------------------------
// 负向对照
// ---------------------------------------------------------------------------

TEST_F(TriggerTest, ThePreMarkOrderIsExactlyWhatUsedToBurnIt) {
    // 手工复现旧引擎那一步「跑脚本之前先把记号打上」，证明上面那些断言拦的是
    // 真问题，而不是碰巧成立。没有这条对照，EarlyReturning… 那条有可能只是
    // 因为脚本压根没被起来而通过。
    const MapObject* trigger = object(kStepTrigger);
    ASSERT_NE(trigger, nullptr);
    state().flags.erase(kOpenFlag);
    state().flags.erase(kDoneFlag);
    ASSERT_TRUE(WorldScene::triggerReady(state(), *trigger));

    // markTriggered 先于 startEvent 时，存档就是这个样子：这一幕还没演，记号先没了。
    state().setFlag(kDoneFlag, 1);

    EXPECT_FALSE(WorldScene::triggerReady(state(), *trigger));
    state().setFlag(kOpenFlag, 1);
    EXPECT_FALSE(WorldScene::triggerReady(state(), *trigger))
        << "玩家后来满足了条件也拿不回来——这正是新口径要拦掉的形状";
}

TEST_F(TriggerTest, AOnceTriggerWithoutSetFlagFallsBackToRepeatableInsteadOfSilentlyDying) {
    // 没有 set_flag 的 once 触发器，引擎无从判断兑现与否。这里钉住它的取舍：
    // 退化成可重复触发（响亮，试玩第一下就看得见），而不是生造一个
    // "_trigger.<name>" 内部旗标——那种旗标没在 data/flags.json 登记，
    // 任何工具都查不到，是一笔看不见的账。这种写法由 tools/validate.py 的
    // once 契约检查拦在门禁上，不靠引擎兜底。
    MapObject probe;
    probe.name = "trigger_probe";
    probe.type = "trigger";
    probe.properties["mode"] = "enter";
    probe.properties["once"] = "true";

    EXPECT_TRUE(WorldScene::triggerReady(state(), probe));
    state().setFlag("_trigger.trigger_probe", 1);
    EXPECT_TRUE(WorldScene::triggerReady(state(), probe))
        << "引擎不再认那个生造的内部旗标";
}

}  // namespace
