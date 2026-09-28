// 隐身的 NPC 既不该挡路，也不该抢话。
//
// 从前 WorldScene 只在「画不画」与「谈不谈」这两处问可见性，占格与命中走的都是
// core::TileMap::objectAt —— 它按矩形命中，只返回对象表里**第一个**，根本不看
// visible_flag / hidden_flag。于是一个还没现身（或已经离开）的 NPC 干两件坏事：
//
//   1. 当一堵隐形墙 —— 玩家撞上去，没有任何反馈；
//   2. 抢先应声 —— 同格另一个该出面的 NPC 连命中都轮不到。
//
// 第 2 条不是假想。一个 npc 对象只挂得住一个 `script`，所以「同一个位置在不同
// 章节由不同的人站着」只能同格摆两个、各带互补的旗标，而这正是 tools/validate.py
// 规则 17 当场拦下的写法（「被同类靠前的 npc 抢先命中」）。校验器报的是实情，
// 错的是引擎；这一批把引擎改了，校验器的口径随之跟上（见 tools/validate_selftest.py
// 的 J 节）。
//
// 每一条都写成**两个方向**：旗标未置时不挡路 / 不应声，置上之后立刻挡路 / 应声。
// 只写前半条的话，把 npc 对象整个删掉也能全绿 —— 那种断言测的是空气。
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

// 夹具地图 tests/maps/t_npc_visible.tmj 约定好的名字与坐标。
// 走道是 y=1 的那一行，x 从 1 到 8 都可通行。
constexpr const char* kMapId = "t_npc_visible";
constexpr Point kGhostCell{3, 1};        // npc_ghost，visible_flag = t.ghost_here
constexpr Point kSharedCell{6, 1};       // npc_early 与 npc_late 同踩这一格
constexpr Point kEmptyCell{4, 1};        // 谁都没站的一格，做对照用
constexpr const char* kGhostFlag = "t.ghost_here";
constexpr const char* kTurnFlag = "t.turned";     // 互补：未置早的在场，置上晚的在场

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

class NpcVisibilityTest : public ::testing::Test {
protected:
    // 与 TriggerTests 同一套搭法：夹具地图与夹具脚本不进正式的 maps/ 与 scripts/，
    // 但要和真的 api.lua、真的 data/ 同根 —— 测的必须是玩家跑的那一条链路。
    static void SetUpTestSuite() {
        const fs::path source = findProjectRoot();
        ASSERT_FALSE(source.empty()) << "找不到工程根目录（应含 scripts/common/api.lua 与 data/）";

        fs::path& root = tempAssetRoot();
        root = fanren::test::uniqueTempPath("fanren_npcvis_root");
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

        // 正式的第 3 章脚本也搬一份：底下那条跑真图的用例要点着 ch03_mishi 上
        // 真的 npc_mo_daifu，而那个对象挂的是 scripts/ch03/mojuren.lua。
        // 少了这一步，「按确认起不来脚本」会因为**文件根本不存在**而成立，
        // 与「这个人不在场」长得一模一样——那正是判据空转的一种。
        fs::create_directories(root / "scripts" / "ch03", ec);
        for (const auto& entry : fs::directory_iterator(source / "scripts" / "ch03")) {
            if (entry.path().extension() != ".lua") continue;
            fs::copy_file(entry.path(), root / "scripts" / "ch03" / entry.path().filename(),
                          fs::copy_options::overwrite_existing, ec);
            ASSERT_FALSE(ec) << "复制 " << entry.path().string() << " 失败: " << ec.message();
        }

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
    const fanren::core::TileMap& map() const { return *app_.currentMap(); }

    // 站到某格左边一格、面朝右。交互看的是「面朝的前一格」。
    void faceFromTheLeftOf(Point cell) {
        state().position = Point{cell.x - 1, cell.y};
        state().facing = 1;
    }

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

    Application app_;
    WorldScene world_;
};

// ---------------------------------------------------------------------------
// 夹具自身的先验判据：底下每一条断言都压在这几件事上，先把它们钉住。
// 少了这一条，「隐身的人没应声」在 NPC 对象根本不存在时照样全绿。
// ---------------------------------------------------------------------------

TEST_F(NpcVisibilityTest, TheFixtureReallyHasTwoNpcsStackedOnOneCell) {
    int atShared = 0;
    const MapObject* first = nullptr;
    for (const MapObject& object : map().objects) {
        if (object.type != "npc") continue;
        if (object.position == kSharedCell) {
            ++atShared;
            if (first == nullptr) first = &object;
        }
    }
    EXPECT_EQ(atShared, 2) << "这份夹具的全部意思就是同格摆两个";
    ASSERT_NE(first, nullptr);
    EXPECT_EQ(first->name, "npc_early") << "靠前的那个必须是带 hidden_flag 的早期那位";

    // 旧路径照旧只认对象表里第一个，与可见性无关 —— 下面 visibleNpcAt 的每一条
    // 都是在它之上多做的那一步。objectAt 要是自己也开始看旗标，这条会立刻转红，
    // 提醒后人：口径搬家了，校验器那边也得跟着搬。
    const MapObject* raw = map().objectAt(kSharedCell, "npc");
    ASSERT_NE(raw, nullptr);
    EXPECT_EQ(raw->name, "npc_early");
    state().setFlag(kTurnFlag, 1);
    raw = map().objectAt(kSharedCell, "npc");
    ASSERT_NE(raw, nullptr);
    EXPECT_EQ(raw->name, "npc_early") << "objectAt 不看可见性，这一条是本次修改的起点";
}

// ---------------------------------------------------------------------------
// 占格
// ---------------------------------------------------------------------------

TEST_F(NpcVisibilityTest, AnAbsentNpcIsNotAWall) {
    ASSERT_EQ(state().flag(kGhostFlag), 0) << "开局它就不该在场";

    const Point justLeft{kGhostCell.x - 1, kGhostCell.y};
    state().position = justLeft;
    EXPECT_TRUE(world_.tryStep(app_, 1, 0)) << "不画、不能谈的 NPC 不该挡路";
    EXPECT_EQ(state().position, kGhostCell);

    // 反向：它一现身，这一格立刻就走不上去了。缺了这一半，把占格那一条整个
    // 删掉（谁都不挡路）也能让上面那句通过。
    state().setFlag(kGhostFlag, 1);
    state().position = justLeft;
    EXPECT_FALSE(world_.tryStep(app_, 1, 0)) << "在场的 NPC 照旧占格";
    EXPECT_EQ(state().position, justLeft);
}

TEST_F(NpcVisibilityTest, TheSharedCellIsBlockedInBothChapters) {
    // 两个互补旗标的 NPC 意味着任何时刻恰好有一个在场，于是这一格**一直**是墙。
    // 这条与上一条是一对：占格跟着「在不在场」走，不是跟着「有没有旗标」走。
    const Point justLeft{kSharedCell.x - 1, kSharedCell.y};
    for (const int turned : {0, 1}) {
        state().setFlag(kTurnFlag, turned);
        state().position = justLeft;
        EXPECT_FALSE(world_.tryStep(app_, 1, 0)) << "t.turned=" << turned;
        EXPECT_EQ(state().position, justLeft);
    }
}

// ---------------------------------------------------------------------------
// 应声
// ---------------------------------------------------------------------------

TEST_F(NpcVisibilityTest, AnAbsentNpcDoesNotAnswer) {
    faceFromTheLeftOf(kGhostCell);
    EXPECT_FALSE(world_.interact(app_));
    EXPECT_FALSE(app_.scripts().isRunning());
    EXPECT_EQ(state().flag("t.ghost_spoke"), 0) << "不在场的人不该开口";

    state().setFlag(kGhostFlag, 1);
    faceFromTheLeftOf(kGhostCell);
    ASSERT_TRUE(world_.interact(app_)) << "现了身就该谈得上";
    pumpUntilIdle();
    EXPECT_EQ(state().flag("t.ghost_spoke"), 1);
}

TEST_F(NpcVisibilityTest, TwoNpcsOnOneCellTakeTurnsByChapter) {
    // 前半章：早的那位在场。他本来就排在对象表前面，这一半从前也是对的。
    ASSERT_EQ(state().flag(kTurnFlag), 0);
    faceFromTheLeftOf(kSharedCell);
    ASSERT_TRUE(world_.interact(app_));
    pumpUntilIdle();
    EXPECT_EQ(state().flag("t.early_spoke"), 1);
    EXPECT_EQ(state().flag("t.late_spoke"), 0);

    // 后半章：早的那位退场，晚的那位应声。**这一半从前做不到** —— npc_early
    // 虽然已经不画也不能谈，却仍然被 objectAt 第一个命中，把 npc_late 压死。
    state().flags.erase("t.early_spoke");   // 清干净，好分辨这一次是谁说的话
    state().setFlag(kTurnFlag, 1);
    faceFromTheLeftOf(kSharedCell);
    ASSERT_TRUE(world_.interact(app_));
    pumpUntilIdle();
    EXPECT_EQ(state().flag("t.late_spoke"), 1) << "该轮到后一位了";
    EXPECT_EQ(state().flag("t.early_spoke"), 0) << "退场的那位不该再开口";
}

// ---------------------------------------------------------------------------
// 纯函数：visibleNpcAt
// ---------------------------------------------------------------------------

TEST_F(NpcVisibilityTest, VisibleNpcAtPicksWhoeverIsOnStage) {
    const MapObject* who = WorldScene::visibleNpcAt(state(), map(), kSharedCell);
    ASSERT_NE(who, nullptr);
    EXPECT_EQ(who->name, "npc_early");

    state().setFlag(kTurnFlag, 1);
    who = WorldScene::visibleNpcAt(state(), map(), kSharedCell);
    ASSERT_NE(who, nullptr);
    EXPECT_EQ(who->name, "npc_late") << "排在后面也要轮得到";
}

// ---------------------------------------------------------------------------
// 真图上的那一位：墨居仁死后不该还在密室里拨算盘（第 3 章独立校对 HIGH-2）
// ---------------------------------------------------------------------------
// 上面几条跑的全是合成夹具，钉的是**机制**。机制对不代表真图接对了：
// maps/ch03_mishi.tmj 的 npc_mo_daifu 一度**既没有 visible_flag 也没有
// hidden_flag**，于是本章高潮把他埋掉之后（ch03.mo_siwang，登记原话就是
// 「墨居仁已暴毙」，ch03.ligu.clean 还写着「师父埋在药圃东头那棵老槐底下」），
// 玩家从 ch03_andao 折回密室——那道门没有任何 require_flag，一步就到——
// 按一下确认键，mojuren.lua 走到第三条分支照旧播 ch03.npc.mojuren_again：
// 一个正在数东西、数到一半会停下来重数的活人。
//
// 同一批 NPC（药圃管事、厉飞雨）都老老实实做了按章切换，独独漏了这一个，
// 而漏掉的恰好是全章唯一一个会死的人。**为什么没被发现**：上面那七条一次也
// 没碰过真图上的这个对象（`grep mo_daifu` 在本文件里零命中），
// 于是「不在场的人不答话」这条机制绿了一整章，而那间屋子里的死人照旧在答话。
//
// 所以这一条**跑真图**，两个方向都验：死之前他在、死之后他不在。
// 只写后半条的话，把这个 NPC 对象整个删掉也能绿。
TEST_F(NpcVisibilityTest, TheDeadMasterStopsAnsweringInTheRealChamber) {
    constexpr const char* kMishi = "ch03_mishi";
    constexpr const char* kDeadFlag = "ch03.mo_siwang";

    // 「人站进了密室」这个前提，取自通密室的那道门本身（神手谷那道门的 require_flag），
    // 不取他自己的出场旗标：那样判据就是从被测物推出来的——他的出场旗标哪天被改成
    // 一个他死后才置的旗标，他在游戏里就永远不会出现，而这一条「死之前他在」照样绿。
    auto valley = app_.loadMap("ch01_shenshougu", std::string{});
    ASSERT_TRUE(valley.ok) << valley.error;
    std::string doorFlag;
    for (const MapObject& object : app_.currentMap()->objects) {
        if (object.type == "portal" && object.property("target_map") == kMishi) {
            doorFlag = object.property("require_flag");
        }
    }
    ASSERT_FALSE(doorFlag.empty()) << "神手谷通密室的门不挂闸门旗标了：「人站进了密室」这个前提无从取起";

    auto loaded = app_.loadMap(kMishi, std::string{});
    ASSERT_TRUE(loaded.ok) << loaded.error;
    const fanren::core::TileMap& mishi = *app_.currentMap();

    const MapObject* master = nullptr;
    for (const MapObject& object : mishi.objects) {
        if (object.type == "npc" && object.name == "npc_mo_daifu") master = &object;
    }
    ASSERT_NE(master, nullptr) << kMishi << " 上没有 npc_mo_daifu";
    // 先验三条，缺一不可：他确实挂着脚本（否则「不答话」毫无意义）、
    // 用的确实是那个「已暴毙」的旗标、而且是 hidden_flag 不是 visible_flag
    // （写成 visible_flag 语义正好反过来：人活着的时候他反而不在场）。
    ASSERT_FALSE(master->property("script").empty()) << "他本来就该有话说";
    EXPECT_EQ(master->property("hidden_flag"), kDeadFlag)
        << "章末把他埋了之后他还站在密室里——这一条正是要挡住那件事";
    // 这一条从前写的是「不许有 visible_flag」——防的正是这种反写，拿「没有」当判据却也
    // 挡住了对的写法：2026-09-27 起他挂了出场旗标（归谷那一刻，此前他在药圃、神手谷堂前，
    // 同一个人一时只在一处，tests/NpcPresenceTests.cpp 管）。出场旗标是哪一个不归这里钉。
    const std::string shownFlag = master->property("visible_flag");
    EXPECT_NE(shownFlag, kDeadFlag) << "写成 visible_flag 的话语义是反的：他得先死才肯出场";

    const Point cell = master->position;
    const Point justLeft{cell.x - 1, cell.y};
    ASSERT_TRUE(mishi.walkable(justLeft)) << "他左边一格站不了人，下面都做不成";

    // ---- 死之前：他在场，占格，也答话 ----
    // 人站进了密室，通密室那道门的旗标就一定已置（上面取的 doorFlag）；他此刻该在。
    state().setFlag(doorFlag, 1);
    ASSERT_EQ(state().flag(kDeadFlag), 0) << "先验：这会儿他还活着";
    EXPECT_NE(WorldScene::visibleNpcAt(state(), mishi, cell), nullptr);
    state().position = justLeft;
    EXPECT_FALSE(world_.tryStep(app_, 1, 0)) << "活着的人照旧占格";
    // 摊牌之前那两句：脚本第一条分支。演完再往下走——**不能只是 abort 掉**：
    // 半路丢弃会把 Application 留在 awaitingCommand 上，而 interact 开头那道闸
    // 一见 awaitingCommand 就直接 return false，于是下面「死后没人答话」
    // 会因为一个完全无关的理由而通过。
    state().position = justLeft;
    state().facing = 1;
    EXPECT_TRUE(world_.interact(app_)) << "他活着的时候该谈得上话";
    EXPECT_TRUE(app_.scripts().isRunning());
    pumpUntilIdle();

    // ---- 死之后：不在场，不占格，也不答话 ----
    state().setFlag(kDeadFlag, 1);
    // 先验：世界层此刻是受理输入的。少了这一句，下面两条 EXPECT_FALSE 可能只是
    // 那道防重入闸的回声（docs/README.md 那张空转法表）。
    ASSERT_FALSE(app_.scripts().isRunning());
    ASSERT_FALSE(app_.awaitingCommand());
    EXPECT_EQ(WorldScene::visibleNpcAt(state(), mishi, cell), nullptr)
        << "他已经埋在药圃东头那棵老槐底下了";
    state().position = justLeft;
    state().facing = 1;
    EXPECT_FALSE(world_.interact(app_)) << "走进这间屋子按确认，不该再有人答话";
    EXPECT_FALSE(app_.scripts().isRunning());
    // 顺带：不在场的人也不该继续当一堵隐形墙。
    state().position = justLeft;
    EXPECT_TRUE(world_.tryStep(app_, 1, 0)) << "死人不该还占着屋子正中那一格";
    EXPECT_EQ(state().position, cell);
}

TEST_F(NpcVisibilityTest, VisibleNpcAtReturnsNothingWhereNobodyStands) {
    // 没人的格子，与「有人但不在场」的格子，答案都该是 nullptr；
    // 而一旦在场就必须取得到。三个方向一起写，免得某一条退化成恒 nullptr。
    EXPECT_EQ(WorldScene::visibleNpcAt(state(), map(), kEmptyCell), nullptr);
    EXPECT_EQ(WorldScene::visibleNpcAt(state(), map(), kGhostCell), nullptr);
    state().setFlag(kGhostFlag, 1);
    const MapObject* who = WorldScene::visibleNpcAt(state(), map(), kGhostCell);
    ASSERT_NE(who, nullptr);
    EXPECT_EQ(who->name, "npc_ghost");
}

}  // namespace
