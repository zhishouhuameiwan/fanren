// P1 垂直切片的端到端验收：地图 → 走位 → 交互触发脚本 → 带选择的对话 →
// 战斗 → 存读档。驱动的是玩家实际运行的同一套 Application 代码，只是跑在无头模式。
//
// 这条测试是 P1 的验收判据本身：它绿，切片才算通。
#include <gtest/gtest.h>

#include <algorithm>
#include <deque>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "TempDir.h"
#include "game/Application.h"
#include "game/BattleScene.h"
#include "game/WorldScene.h"
#include "io/SaveFile.h"

namespace {

using fanren::core::Point;
using fanren::game::Application;
using fanren::game::WorldScene;

// 仓库根：测试可能从 build/ 或工程根启动，两处都要能找到 data/ 与 maps/。
std::string assetRoot() {
    namespace fs = std::filesystem;
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "maps" / "ch01_hanjiacun.tmj")) return candidate;
    }
    return ".";
}

// 找到某类地图对象的位置。
const fanren::core::MapObject* findObject(const fanren::core::TileMap& map,
                                          const std::string& type) {
    for (const auto& object : map.objects) {
        if (object.type == type) return &object;
    }
    return nullptr;
}

// 从当前位置走到目标格的相邻格。用 BFS 而不是写死路线：地图一改，
// 写死的路线会悄悄失效，而 BFS 会如实报告「走不到」。
// 走位口径必须与 WorldScene::tryStep 一致，所以 NPC 那一道走 visibleNpcAt：
// 不在场的 NPC 不占格（2026-09-21 起）。旧写法用 objectAt，比引擎严 —— 那样这条
// BFS 会报「走不到」而玩家其实走得过去，红灯出在测试自己身上。
std::vector<Point> pathToNeighbour(const fanren::core::GameState& state,
                                   const fanren::core::TileMap& map, Point from, Point goal) {
    const int width = map.width;
    std::map<int, int> previous;
    const auto index = [width](Point p) { return p.y * width + p.x; };
    std::deque<Point> queue{from};
    previous[index(from)] = index(from);

    const Point steps[] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    Point found{-1, -1};
    while (!queue.empty()) {
        const Point current = queue.front();
        queue.pop_front();
        for (const Point& step : steps) {
            const Point next{current.x + step.x, current.y + step.y};
            if (next == goal) {
                found = current;
                queue.clear();
                break;
            }
            if (!map.walkable(next)) continue;
            if (WorldScene::visibleNpcAt(state, map, next) != nullptr) continue;
            if (previous.count(index(next))) continue;
            previous[index(next)] = index(current);
            queue.push_back(next);
        }
        if (found.x >= 0) break;
    }
    if (found.x < 0) return {};

    std::vector<Point> path;
    int node = index(found);
    while (node != index(from)) {
        path.push_back(Point{node % width, node / width});
        node = previous[node];
    }
    std::reverse(path.begin(), path.end());
    return path;
}

// 推进若干帧，直到条件成立或超时。返回条件是否成立。
template <typename Predicate>
bool advanceUntil(Application& app, Predicate ready, int maxFrames = 600) {
    for (int frame = 0; frame < maxFrames; ++frame) {
        if (ready()) return true;
        app.tick(1.0 / 60.0);
    }
    return ready();
}

class SliceTest : public ::testing::Test {
protected:
    void SetUp() override {
        root_ = assetRoot();
        auto ready = app_.init(root_, /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
        auto loaded = app_.loadMap("ch01_hanjiacun", std::string{});
        ASSERT_TRUE(loaded.ok) << loaded.error;
    }

    void TearDown() override { app_.shutdown(); }

    Application app_;
    std::string root_;
};

TEST_F(SliceTest, LoadsTheChapterOneMapAndPlacesTheHeroOnTheSpawn) {
    const auto* map = app_.currentMap();
    ASSERT_NE(map, nullptr);
    EXPECT_EQ(map->id, "ch01_hanjiacun");
    EXPECT_GT(map->width, 0);
    EXPECT_GT(map->height, 0);
    // 出生点必须落在可通行格上，否则玩家一进图就被卡住。
    EXPECT_TRUE(map->walkable(app_.state().position));
}

TEST_F(SliceTest, WalksToTheNpcAndCannotStepOntoIt) {
    const auto* map = app_.currentMap();
    ASSERT_NE(map, nullptr);
    const auto* npc = findObject(*map, "npc");
    ASSERT_NE(npc, nullptr) << "第 1 章地图应当有一个 NPC";

    WorldScene world;
    const auto path = pathToNeighbour(app_.state(), *map, app_.state().position, npc->position);
    ASSERT_FALSE(path.empty()) << "从出生点走不到 NPC 旁边";

    for (const Point& step : path) {
        const Point from = app_.state().position;
        ASSERT_TRUE(world.tryStep(app_, step.x - from.x, step.y - from.y))
            << "在 (" << from.x << "," << from.y << ") 卡住了";
    }

    // 站在 NPC 旁边，再往它身上走应当被挡下（朝向仍要转过去）。
    const Point beside = app_.state().position;
    const int dx = npc->position.x - beside.x;
    const int dy = npc->position.y - beside.y;
    EXPECT_FALSE(world.tryStep(app_, dx, dy));
    EXPECT_EQ(app_.state().position, beside) << "不该走进 NPC 占的格子";
}

TEST_F(SliceTest, RunsTheWholeDialogueWithAChoiceAndSetsTheFlag) {
    const auto* map = app_.currentMap();
    ASSERT_NE(map, nullptr);
    const auto* npc = findObject(*map, "npc");
    ASSERT_NE(npc, nullptr);
    const std::string script = npc->property("script");
    ASSERT_FALSE(script.empty()) << "NPC 应当挂着事件脚本";

    EXPECT_EQ(app_.state().flag("ch01.sanshu_met"), 0) << "开局不该已经谈过";

    auto started = app_.startEvent(script);
    ASSERT_TRUE(started.ok) << started.error;
    EXPECT_FALSE(app_.canSave()) << "脚本挂起期间禁止存档";

    // 一路推进：对话框自动显示完（无头模式跳过逐字动画），
    // 遇到选择时替玩家选第一项。
    bool sawChoice = false;
    for (int frame = 0; frame < 600 && app_.scripts().isRunning(); ++frame) {
        app_.tick(1.0 / 60.0);
        if (app_.awaitingCommand()) {
            fanren::script::CommandResult result;
            result.ok = true;
            result.choiceIndex = 0;
            sawChoice = true;
            app_.completeCommand(result);
        }
    }

    EXPECT_TRUE(sawChoice) << "整条对话里应当至少出现一次需要玩家回应的命令";
    EXPECT_FALSE(app_.scripts().isRunning()) << "脚本应当已经跑完";
    EXPECT_TRUE(app_.canSave()) << "脚本结束后应当可以存档";
    EXPECT_EQ(app_.state().flag("ch01.sanshu_met"), 1) << "谈过之后旗标应当置位";
}

TEST_F(SliceTest, ChoosingTheOtherBranchAlsoCompletes) {
    const auto* map = app_.currentMap();
    const auto* npc = findObject(*map, "npc");
    ASSERT_NE(npc, nullptr);

    auto started = app_.startEvent(npc->property("script"));
    ASSERT_TRUE(started.ok) << started.error;

    for (int frame = 0; frame < 600 && app_.scripts().isRunning(); ++frame) {
        app_.tick(1.0 / 60.0);
        if (app_.awaitingCommand()) {
            fanren::script::CommandResult result;
            result.ok = true;
            result.choiceIndex = 1;   // 另一条分支
            app_.completeCommand(result);
        }
    }

    EXPECT_FALSE(app_.scripts().isRunning());
    // 两条分支都算「谈过」：分歧体现在播放的文案上，不改变主线推进。
    EXPECT_EQ(app_.state().flag("ch01.sanshu_met"), 1);
}

TEST_F(SliceTest, RunsABattleToACompleteOutcome) {
    fanren::game::BattleScene battle("slice_probe");
    battle.onEnter(app_);

    const bool won = battle.runToCompletion();
    EXPECT_NE(battle.battle().phase(), fanren::core::battle::BattlePhase::Ongoing)
        << "战斗必须分出结果，不能悬着";
    // 谁赢不重要（数值还没配平），重要的是流程能跑完且状态自洽。
    if (won) {
        EXPECT_EQ(battle.battle().phase(), fanren::core::battle::BattlePhase::Won);
    }
}

TEST_F(SliceTest, BattleIsReproducibleWithTheSameSeed) {
    fanren::game::BattleScene first("slice_probe");
    first.onEnter(app_);
    const bool firstOutcome = first.runToCompletion();
    const int firstRounds = first.battle().round();

    fanren::game::BattleScene second("slice_probe");
    second.onEnter(app_);
    const bool secondOutcome = second.runToCompletion();

    EXPECT_EQ(firstOutcome, secondOutcome) << "同种子同流程必须复现";
    EXPECT_EQ(firstRounds, second.battle().round());
}

TEST_F(SliceTest, SaveAndLoadRoundTripsPositionFlagsAndBag) {
    const auto* map = app_.currentMap();
    const auto* npc = findObject(*map, "npc");
    ASSERT_NE(npc, nullptr);

    // 制造一点可验证的状态差异。
    WorldScene world;
    world.tryStep(app_, 1, 0);
    app_.state().setFlag("ch01.sanshu_met", 1);
    app_.state().addItem("jinchuang_yao", 2, 0);
    app_.state().playSecondsGameplay = 123.5;
    const auto expected = app_.state();

    namespace fs = std::filesystem;
    // 文件名带 pid：写死的话两个测试进程会抢同一个存档文件。见 tests/TempDir.h。
    const fs::path file = fanren::test::uniqueTempPath("fanren_slice_save", ".json");
    auto saved = fanren::io::saveGame(app_.state(), file.string());
    ASSERT_TRUE(saved.ok) << saved.error;

    auto loaded = fanren::io::loadGame(file.string());
    ASSERT_TRUE(loaded.ok) << loaded.error;
    const auto& restored = loaded.value;

    EXPECT_EQ(restored.mapId, expected.mapId);
    EXPECT_EQ(restored.position, expected.position);
    EXPECT_EQ(restored.facing, expected.facing);
    EXPECT_EQ(restored.flag("ch01.sanshu_met"), 1);
    EXPECT_EQ(restored.itemCount("jinchuang_yao"), 2);
    EXPECT_DOUBLE_EQ(restored.playSecondsGameplay, expected.playSecondsGameplay);

    fs::remove(file);
}

TEST_F(SliceTest, PortalCarriesTheHeroToTheOtherMap) {
    const auto* map = app_.currentMap();
    ASSERT_NE(map, nullptr);
    const auto* portal = findObject(*map, "portal");
    ASSERT_NE(portal, nullptr) << "第 1 章地图应当有一个传送点";

    const std::string target = portal->property("target_map");
    ASSERT_FALSE(target.empty());

    auto moved = app_.loadMap(target, portal->property("target_spawn"));
    ASSERT_TRUE(moved.ok) << moved.error;
    EXPECT_EQ(app_.state().mapId, target);
    EXPECT_TRUE(app_.currentMap()->walkable(app_.state().position))
        << "传送落点必须可通行";
}

TEST_F(SliceTest, BuildsTheEncounterFromDataFiles) {
    // 迁移过来的二十场战斗必须真的能打起来，否则它们只是躺在磁盘上的文本。
    const auto* setup = app_.battleSetup("b03_gu_wai_elang");
    ASSERT_NE(setup, nullptr) << "data/battles/ 里的战斗没被加载";
    EXPECT_EQ(setup->chapter, 3);
    ASSERT_FALSE(setup->units.empty());

    fanren::game::BattleScene scene("b03_gu_wai_elang");
    scene.onEnter(app_);

    // 编成 = 数据里的敌人 + 主角，主角固定 0 号。
    const auto& units = scene.battle().units();
    ASSERT_EQ(units.size(), setup->units.size() + 1);
    EXPECT_TRUE(units[0].ally) << "0 号必须是主角";

    // 敌人的属性应当来自 data/roles，而不是代码里写死的兜底数值。
    const auto* role = app_.data().findRole(setup->units.front().roleId);
    ASSERT_NE(role, nullptr);
    EXPECT_EQ(units[1].name, role->name);
    EXPECT_EQ(units[1].maxHp, role->maxHp);
    EXPECT_FALSE(units[1].ally);

    // 从前这里判「战场尺寸也来自数据」。横版没有格子，换成同一个意思的那两样：
    // 架势与破绽也来自数据（docs/octopath-battle.md 第 5 节），不是代码里写死的。
    ASSERT_GT(role->toughness, 0) << "先验：野狼有架势，否则下面两条比的是两个 0";
    EXPECT_EQ(units[1].maxToughness, role->toughness);
    EXPECT_EQ(units[1].toughness, role->toughness) << "开场架势是满的";
    EXPECT_EQ(units[1].weaknesses, role->weaknesses);
}

TEST_F(SliceTest, EveryMigratedBattleIsPlayableToACompleteOutcome) {
    // 逐场跑完。任何一场卡住不分胜负，都说明那份配置有问题——
    // 这是 P2「二十场战斗迁移完成并可打」的判据本身。
    const std::vector<std::string> ids = {
        "b03_gu_wai_elang", "b04_yelangbang_laifan", "b05_mofu_shigui",
        "b06_huangfenggu_qiecuo", "b07_zhaoze_shouyao", "b10_gujia_bidou",
    };
    for (const std::string& id : ids) {
        const auto* setup = app_.battleSetup(id);
        ASSERT_NE(setup, nullptr) << id << " 没被加载";

        fanren::game::BattleScene scene(id);
        scene.onEnter(app_);
        scene.runToCompletion();
        EXPECT_NE(scene.battle().phase(), fanren::core::battle::BattlePhase::Ongoing)
            << id << " 打不出结果";
    }
}

TEST_F(SliceTest, UnknownBattleIdFallsBackInsteadOfEmptyField) {
    // 脚本笔误不该把玩家丢进空战场。
    EXPECT_EQ(app_.battleSetup("no_such_battle"), nullptr);

    fanren::game::BattleScene scene("no_such_battle");
    scene.onEnter(app_);
    EXPECT_GE(scene.battle().units().size(), 2u) << "兜底遭遇也得有敌我双方";
    scene.runToCompletion();
    EXPECT_NE(scene.battle().phase(), fanren::core::battle::BattlePhase::Ongoing);
}

TEST_F(SliceTest, BlockedExitExplainsItselfInsteadOfDoingNothing) {
    // 出口挂了通行条件却没提示，玩家踩上去只会以为按键坏了。
    // 这四个地图属性此前全是死的：规范写了、关卡照着用了、引擎一个没读。
    const auto* map = app_.currentMap();
    ASSERT_NE(map, nullptr);

    const fanren::core::MapObject* gated = nullptr;
    for (const auto& object : map->objects) {
        if (object.type == "portal" && !object.property("require_flag").empty()) {
            gated = &object;
            break;
        }
    }
    ASSERT_NE(gated, nullptr) << "第 1 章第一张图应当有带通行条件的出口";
    ASSERT_FALSE(gated->property("deny_text_key").empty()) << "拦人就要说明原因";

    // 条件未满足：站到出口上，不该换图，且该弹出提示。
    const std::string before = app_.state().mapId;
    app_.state().flags.erase(gated->property("require_flag"));

    WorldScene world;
    app_.state().position = fanren::core::Point{gated->position.x, gated->position.y - 1};
    world.tryStep(app_, 0, 1);

    EXPECT_EQ(app_.state().mapId, before) << "条件没满足却换图了";
    app_.tick(1.0 / 60.0);   // 让延迟入栈的提示框真正进场景栈
    ASSERT_NE(app_.topScene(), nullptr);
    EXPECT_EQ(app_.topScene()->name(), "Dialogue") << "被拦下时应当看到一句说明";
}

TEST_F(SliceTest, SatisfiedExitLetsThePlayerThrough) {
    const auto* map = app_.currentMap();
    ASSERT_NE(map, nullptr);
    const fanren::core::MapObject* gated = nullptr;
    for (const auto& object : map->objects) {
        if (object.type == "portal" && !object.property("require_flag").empty()) {
            gated = &object;
            break;
        }
    }
    ASSERT_NE(gated, nullptr);

    // 换图会重建 TileMap，gated 随之失效。期望值必须在那之前取出来，
    // 否则读的是悬空指针（第一版就栽在这里，拿到空串还以为是功能坏了）。
    const std::string expected = gated->property("target_map");
    const std::string gate = gated->property("require_flag");
    const fanren::core::Point at = gated->position;

    // 满足条件后必须放行——闸门不能把必经路径变成死路。
    app_.state().setFlag(gate, 1);
    WorldScene world;
    app_.state().position = fanren::core::Point{at.x, at.y - 1};
    world.tryStep(app_, 0, 1);

    EXPECT_EQ(app_.state().mapId, expected) << "条件满足了却过不去";
}

TEST_F(SliceTest, OnceTriggerDoesNotFireTwice) {
    // 没有 once，玩家每走回那一格就重播一次剧情。
    //
    // 兑现 once 的是脚本自己置的完成旗标，不是引擎预写的记号——所以这里模拟的是
    // 「脚本演完了，末尾那句 flag.set 落了地」，而不是调用某个 markTriggered。
    // 那个函数连同它那套「先打记号再跑脚本」的时序已经删掉了，见 WorldScene.h。
    fanren::core::MapObject trigger;
    trigger.name = "trigger_probe";
    trigger.type = "trigger";
    trigger.properties["mode"] = "enter";
    trigger.properties["once"] = "true";
    trigger.properties["set_flag"] = "ch01.sanshu_met";

    fanren::core::GameState& state = app_.state();
    state.flags.erase("ch01.sanshu_met");
    EXPECT_TRUE(WorldScene::triggerReady(state, trigger)) << "首次应当可触发";

    state.setFlag("ch01.sanshu_met", 1);   // 脚本演完，置上自己的完成旗标
    EXPECT_FALSE(WorldScene::triggerReady(state, trigger)) << "once 之后不该再触发";
}

TEST_F(SliceTest, HiddenNpcIsNeitherDrawnNorTalkable) {
    fanren::core::MapObject npc;
    npc.name = "npc_probe";
    npc.type = "npc";
    fanren::core::GameState& state = app_.state();

    EXPECT_TRUE(WorldScene::npcVisible(state, npc)) << "没挂条件的 NPC 默认在场";

    npc.properties["visible_flag"] = "ch01.probe_show";
    state.flags.erase("ch01.probe_show");
    EXPECT_FALSE(WorldScene::npcVisible(state, npc)) << "visible_flag 未置位时不该在场";
    state.setFlag("ch01.probe_show", 1);
    EXPECT_TRUE(WorldScene::npcVisible(state, npc));

    npc.properties["hidden_flag"] = "ch01.probe_hide";
    state.setFlag("ch01.probe_hide", 1);
    EXPECT_FALSE(WorldScene::npcVisible(state, npc)) << "hidden_flag 置位后应当隐去";
}

}  // namespace
