// 明雷的巡逻与追击（2026-11-20 用户试玩第 7 章：「敌人不动的确很呆」；设计见 docs/ch07-playtest-foes.md，
// 契约 docs/interfaces-companions.md 2.4、3.4）。
//
// 驱动真的 WorldScene（压在真的 Application 上，按 1/60 秒的固定步长 tick）：明雷走到哪一格、在追还是在巡逻、
// 宽限还剩多久，从 WorldScene::foePatrol() 与 describeFrame 拿——画面拿到的就是这一份。开打从
// Application::battleStart() 取，仗的胜负不归本路，收场照 CompanionsWorldTests 的做法替战斗场景报战果。
//
// 资产根是一份临时拷贝，maps/ 里放一张探针图 probe_patrol（40×14 的空地，几处墙），几只明雷各占一片，
// 彼此离得够远、互不打扰。期望值照设计写死（docs/ch07-playtest-foes.md 第 3 节的数字）：巡逻一格 0.5 秒、
// 张望每向 0.8 秒、追一格 0.25 秒、愣 0.4 秒、拴绳 6 格、视线 3 格、开打宽限 3 秒——都不从被测代码推。
#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "TempDir.h"
#include "core/model/Types.h"
#include "game/Application.h"
#include "game/BattleScene.h"
#include "game/FoePatrol.h"
#include "game/WorldScene.h"
#include "game/WorldView.h"
#include "io/DataLoader.h"
#include "script/Command.h"

namespace {

namespace fs = std::filesystem;
using fanren::core::MapObject;
using fanren::core::Point;
using fanren::game::Application;
using fanren::game::BattleScene;
using fanren::game::FoeTrack;
using fanren::game::Vec2f;
using fanren::game::WorldFrame;
using fanren::game::WorldScene;
using fanren::game::foepatrol::Mode;
using fanren::game::foepatrol::Walker;

fs::path repoRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "data" / "text" / "ch01_main.json")) return candidate;
    }
    return ".";
}

void writeFile(const fs::path& path, const std::string& text) {
    fs::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out << text;
}

// ---- 探针图 probe_patrol：40×14 的空地 ----
constexpr int kW = 40;
constexpr int kH = 14;
constexpr const char* kWolfBattle = "be03_guwai_gulang";

std::string prop(const std::string& name, const std::string& value) {
    return R"({"name":")" + name + R"(","type":"string","value":")" + value + R"("})";
}
std::string propBool(const std::string& name, bool value) {
    return R"({"name":")" + name + R"(","type":"bool","value":)" + (value ? "true" : "false") + "}";
}
std::string propInt(const std::string& name, int value) {
    return R"({"name":")" + name + R"(","type":"int","value":)" + std::to_string(value) + "}";
}
std::string object(const std::string& name, const std::string& type, int x, int y,
                   const std::vector<std::string>& props) {
    std::string list;
    for (const std::string& p : props) list += (list.empty() ? "" : ",") + p;
    return R"({"id":0,"name":")" + name + R"(","type":")" + type + R"(","x":)" + std::to_string(x * 32) +
           R"(,"y":)" + std::to_string(y * 32) + R"(,"width":32,"height":32,"rotation":0,"visible":true,)" +
           R"("properties":[)" + list + "]}";
}
std::string foeAt(const std::string& name, int x, int y, const std::string& facing, const std::string& patrol) {
    std::vector<std::string> props{prop("id", "probe_patrol." + name), prop("battle", kWolfBattle),
                                   prop("look", "wild_wolf"), prop("facing", facing)};
    if (!patrol.empty()) props.push_back(prop("patrol", patrol));
    return object(name, "foe", x, y, props);
}

// 墙（collision = 1）：B 区挡视线的一格；C 区挡路的两格；E 区口袋上下两格。
const std::vector<Point> kWalls{{14, 2}, {21, 5}, {22, 5}, {7, 9}, {7, 11}};

std::string probeMap() {
    std::string zeros;
    std::string walls;
    for (int i = 0; i < kW * kH; ++i) {
        const Point at{i % kW, i / kW};
        bool wall = false;
        for (const Point& w : kWalls) wall = wall || w == at;
        zeros += (i == 0 ? "0" : ",0");
        walls += std::string(i == 0 ? "" : ",") + (wall ? "1" : "0");
    }
    std::string layers;
    for (const char* name : {"ground", "overlay", "building", "front", "collision"}) {
        const std::string& data = std::string(name) == "collision" ? walls : zeros;
        layers += R"({"name":")" + std::string(name) + R"(","type":"tilelayer","width":)" + std::to_string(kW) +
                  R"(,"height":)" + std::to_string(kH) + R"(,"data":[)" + data + "]},";
    }
    const std::vector<std::string> objects{
        object("spawn_main", "spawn", 20, 12, {prop("id", "spawn_main"), propBool("default", true), prop("facing", "down")}),
        // A 区：竖着走 (2,2)→(2,5)，来回。
        foeAt("foe_walk", 2, 2, "down", "2,5"),
        // B 区：原地张望，面朝右；右边隔一格 (14,2) 是墙。
        foeAt("foe_wall", 12, 2, "right", "12,2"),
        // C 区：原地张望，面朝下；正下方 (21,5)(22,5) 是墙、(23,5) 一只宝箱。
        foeAt("foe_chase", 22, 2, "down", "22,2"),
        object("chest_block", "chest", 23, 5, {prop("id", "probe_patrol.chest_block"), prop("loot", "material_fuzhi")}),
        // D 区：原地张望，面朝下；南边一大片空地，量拴绳。
        foeAt("foe_leash", 32, 2, "down", "32,2"),
        // E 区：原地张望，面朝右；口袋 (7,10) 上下是墙、右边是传送点、左边那一格挨着踏入触发区。
        foeAt("foe_shy", 4, 10, "right", "4,10"),
        object("portal_shy", "portal", 8, 10, {prop("target_map", "probe_patrol"), prop("target_spawn", "spawn_main")}),
        object("trigger_shy", "trigger", 6, 11, {prop("script", "t/noop.lua"), prop("mode", "enter"), propBool("once", false)}),
        // 不巡逻的：与改版前一样站着不动、不看不追。
        foeAt("foe_still", 36, 10, "left", ""),
    };
    std::string list;
    for (const std::string& o : objects) list += (list.empty() ? "" : ",") + o;
    layers += R"({"name":"objects","type":"objectgroup","objects":[)" + list + "]}";
    return R"({"width":)" + std::to_string(kW) + R"(,"height":)" + std::to_string(kH) +
           R"(,"tilewidth":32,"tileheight":32,"orientation":"orthogonal","layers":[)" + layers + R"(],"properties":[)" +
           prop("map_id", "probe_patrol") + "," + prop("display_name_key", "ch01.map.hanjiacun.name") + "," +
           prop("region", "probe") + "," + prop("bgm", "none") + "," + propInt("chapter", 7) + "," +
           propBool("outdoor", true) + "," + propBool("can_leave_edge", false) + "]}";
}

constexpr int kUp = 0;
constexpr int kRight = 1;
constexpr int kDown = 2;
constexpr int kLeft = 3;

class FoePatrolWorld : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        root_ = std::make_unique<fanren::test::TempDir>("fanren_foe_patrol");
        for (const char* dir : {"data", "scripts", "maps"}) {
            fs::copy(repoRoot() / dir, root_->path() / dir, fs::copy_options::recursive);
        }
        writeFile(root_->path() / "maps" / "probe_patrol.tmj", probeMap());
        writeFile(root_->path() / "scripts" / "t" / "noop.lua", R"(flag.set("probe.noop"))");
        writeFile(root_->path() / "scripts" / "t" / "hold.lua", R"(talk("", "ui.world.chest.empty"))");
    }
    static void TearDownTestSuite() { root_.reset(); }

    void SetUp() override {
        auto ready = app_.init(root_->path().string(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
        std::error_code ec;
        fs::remove(app_.quickSavePath(), ec);
        ASSERT_TRUE(app_.loadMap("probe_patrol", std::string{}).ok);
        app_.pushScene(std::make_unique<WorldScene>());
        tick();   // 这一帧把世界层压上栈；它的 update 从下一帧起才跑——明雷的钟从这里起算
        world_ = dynamic_cast<WorldScene*>(app_.topScene());
        ASSERT_NE(world_, nullptr);
    }
    void TearDown() override {
        app_.shutdown();
        std::error_code ec;
        fs::remove(app_.quickSavePath(), ec);
    }

    void tick(int frames = 1) {
        for (int i = 0; i < frames; ++i) app_.tick(1.0 / 60.0);
    }
    // 世界层跑 seconds 秒（按帧取整）。
    void run(double seconds) { tick(static_cast<int>(std::lround(seconds * 60.0))); }

    WorldScene& scene() {
        auto* w = dynamic_cast<WorldScene*>(app_.topScene());
        EXPECT_NE(w, nullptr) << "栈顶应是世界层，实际是 " << (app_.topScene() ? app_.topScene()->name() : "空");
        return *w;
    }
    std::string top() {
        return app_.topScene() == nullptr ? std::string{} : app_.topScene()->name();
    }
    bool battling() { return dynamic_cast<BattleScene*>(app_.topScene()) != nullptr; }
    void stand(Point at, int facing) {
        app_.state().position = at;
        app_.state().facing = facing;
    }
    const MapObject& foeObject(const std::string& name) {
        for (const MapObject& object : app_.currentMap()->objects) {
            if (object.name == name) return object;
        }
        ADD_FAILURE() << "图上没有 " << name;
        return app_.currentMap()->objects.front();
    }
    // 世界层被菜单、对话、战斗压着的时候也要看得到它的明雷：取压栈时记下的那一个（读档会换一个，saveAndReload 跟着换）。
    const Walker* walker(const std::string& name) {
        const Walker* w = world_->foePatrol().find(app_.currentMap(), foeObject(name));
        EXPECT_NE(w, nullptr) << name << " 不是会走的明雷（或世界层还没认过这张图）";
        return w;
    }
    const WorldFrame::Foe* framed(const std::string& name) {
        const WorldFrame& frame = world_->describeFrame(app_);
        for (const WorldFrame::Foe& foe : frame.foes) {
            if (foe.object != nullptr && foe.object->name == name) return &foe;
        }
        return nullptr;
    }
    // 收场：替战斗场景报一个战果（仗的胜负不归本路），回到世界层。
    void finishBattle(bool won, const std::string& code) {
        ASSERT_EQ(top(), "Battle");
        fanren::script::CommandResult result;
        result.battleWon = won;
        result.code = code;
        app_.completeCommand(result);
        tick();
        ASSERT_NE(dynamic_cast<WorldScene*>(app_.topScene()), nullptr);
    }
    void saveAndReload() {
        const auto saved = app_.quickSave();
        ASSERT_TRUE(saved.ok) << saved.error;
        const auto loaded = app_.continueJourney(app_.quickSavePath());
        ASSERT_TRUE(loaded.ok) << loaded.error;
        tick();
        world_ = dynamic_cast<WorldScene*>(app_.topScene());
        ASSERT_NE(world_, nullptr);
    }

    static inline std::unique_ptr<fanren::test::TempDir> root_;
    Application app_;
    WorldScene* world_ = nullptr;
};

// ---------------------------------------------------------------------------
// 巡逻：按时间走、到头张望
// ---------------------------------------------------------------------------

// foe_walk：(2,2) 面朝下，patrol 2,5。开场先看前面 0.8 秒、左右各 0.8 秒（到 2.4 秒），朝下一格转过去，
// 再过 0.5 秒（2.9 秒）迈第一步，之后每 0.5 秒一格——主角一步 0.12 秒，它慢得多。
TEST_F(FoePatrolWorld, AWalkerStepsToItsNextCellOnTheClock) {
    run(0.4);
    const Walker* w = walker("foe_walk");
    ASSERT_NE(w, nullptr) << "写了 patrol 的明雷会走";
    EXPECT_EQ(w->route, (std::vector<Point>{{2, 2}, {2, 3}, {2, 4}, {2, 5}})) << "途经格补齐";
    EXPECT_EQ(walker("foe_walk")->cell, (Point{2, 2}));
    EXPECT_EQ(walker("foe_walk")->facing, kDown) << "先看前面";
    run(2.4);   // 2.8 秒
    EXPECT_EQ(walker("foe_walk")->cell, (Point{2, 2})) << "张望完、转过身，还没迈步";
    EXPECT_EQ(walker("foe_walk")->facing, kDown) << "朝下一格转过去";
    run(0.2);   // 3.0 秒
    EXPECT_EQ(walker("foe_walk")->cell, (Point{2, 3})) << "2.9 秒迈出第一步";
    const WorldFrame::Foe* seen = framed("foe_walk");
    ASSERT_NE(seen, nullptr);
    EXPECT_EQ(seen->cell, (Point{2, 3})) << "画面拿到的是此刻那一格";
    EXPECT_EQ(seen->facing, kDown);
    EXPECT_FLOAT_EQ(seen->stepSeconds, 0.5f) << "巡逻一格 0.5 秒，画面照这个滑";
    EXPECT_FALSE(seen->alert);
    run(0.5);   // 3.5 秒
    EXPECT_EQ(walker("foe_walk")->cell, (Point{2, 4}));
    EXPECT_EQ(walker("foe_walk")->mode, Mode::Patrol);
}

TEST_F(FoePatrolWorld, AtTheEndOfItsRouteAWalkerLooksAroundAndTurnsBack) {
    run(3.95);   // 3.9 秒走到 (2,5)，那是路的一头
    EXPECT_EQ(walker("foe_walk")->cell, (Point{2, 5}));
    EXPECT_EQ(walker("foe_walk")->facing, kDown) << "走到头先看前面";
    run(0.45);   // 4.4
    EXPECT_EQ(walker("foe_walk")->facing, kDown);
    run(0.6);    // 5.0：4.7 秒起转向一侧
    EXPECT_EQ(walker("foe_walk")->facing, kRight);
    run(0.8);    // 5.8：5.5 秒起转向另一侧
    EXPECT_EQ(walker("foe_walk")->facing, kLeft);
    run(0.7);    // 6.5：6.3 秒掉头
    EXPECT_EQ(walker("foe_walk")->facing, kUp);
    EXPECT_EQ(walker("foe_walk")->cell, (Point{2, 5})) << "掉了头，下一步 6.8 秒才迈";
    run(0.5);    // 7.0
    EXPECT_EQ(walker("foe_walk")->cell, (Point{2, 4})) << "原路往回走";
    EXPECT_EQ(walker("foe_walk")->facing, kUp);
}

// 判定读的是此刻那一格：它走开之后原地是空的、走得上去；它此刻的侧后面朝它按确认是偷袭，正面是普通开打。
TEST_F(FoePatrolWorld, AmbushAndFrontFollowTheWalkerWhereItIsNow) {
    // 开场：它在 (2,2) 面朝下；左侧 (1,2) 面朝它是偷袭。
    stand({1, 2}, kRight);
    ASSERT_TRUE(scene().interact(app_));
    tick();
    ASSERT_TRUE(battling());
    EXPECT_TRUE(app_.battleStart().preemptive) << "侧面偷袭占先手";
    finishBattle(false, "escaped");
    EXPECT_EQ(app_.state().position, (Point{1, 2}));

    stand({6, 8}, kDown);   // 走开，让它巡逻
    run(3.0);
    ASSERT_EQ(walker("foe_walk")->cell, (Point{2, 3})) << "先验：它已经走了一格";
    ASSERT_EQ(walker("foe_walk")->facing, kDown);

    // 原地 (2,2) 空了：面朝它按确认不响，迈得上去——走上去正好在它背后。
    stand({2, 1}, kDown);
    EXPECT_FALSE(scene().interact(app_)) << "它不在原地了";
    ASSERT_TRUE(scene().tryStep(app_, 0, 1)) << "它走开的那一格不占格";
    EXPECT_EQ(app_.state().position, (Point{2, 2}));
    EXPECT_FALSE(battling());
    // 背后面朝它（(2,3)）按确认：偷袭。
    Point cell;
    const MapObject* npc = nullptr;
    EXPECT_EQ(scene().interactHint(app_, cell, npc), app_.text("ui.world.interact.key") + " " +
                                                         app_.text("ui.world.interact.ambush"));
    EXPECT_EQ(cell, (Point{2, 3})) << "提示浮在它此刻那一格";
    ASSERT_TRUE(scene().interact(app_));
    tick();
    ASSERT_TRUE(battling());
    EXPECT_TRUE(app_.battleStart().preemptive) << "背后偷袭占先手（按它此刻的朝向）";
    finishBattle(false, "escaped");

    // 它面前那一格 (2,4) 面朝它按确认：普通开打。
    ASSERT_EQ(walker("foe_walk")->cell, (Point{2, 3}));
    stand({2, 4}, kUp);
    ASSERT_TRUE(scene().interact(app_));
    tick();
    ASSERT_TRUE(battling());
    EXPECT_FALSE(app_.battleStart().preemptive) << "正面不是偷袭";
    finishBattle(false, "escaped");
}

// ---------------------------------------------------------------------------
// 视线
// ---------------------------------------------------------------------------

// foe_wall：(12,2) 原地张望，右 → 上 → 下 → 右……（每向 0.8 秒）。主角在 (15,2)：正好三格，可中间 (14,2) 是墙。
TEST_F(FoePatrolWorld, AWallBlocksItsSight) {
    stand({15, 2}, kLeft);
    for (int frame = 0; frame < 252; ++frame) {   // 4.2 秒：其间面朝右两回（0–0.8、2.4–3.2）
        tick();
        ASSERT_EQ(walker("foe_wall")->mode, Mode::Patrol) << "第 " << frame << " 帧：隔着墙看不见";
        ASSERT_FALSE(walker("foe_wall")->alert);
        ASSERT_FALSE(battling());
    }
    // 对照：没有墙挡着就看得见。4.0 秒起它面朝下，正下方三格 (12,5)。
    ASSERT_EQ(walker("foe_wall")->facing, kDown);
    stand({12, 5}, kUp);
    tick();
    EXPECT_EQ(walker("foe_wall")->mode, Mode::Chase) << "直线三格以内、中间没挡着：看见了";
    EXPECT_TRUE(walker("foe_wall")->alert);
}

// foe_chase：(22,2) 原地张望，下 → 右 → 左 → 下……开场一秒的进图宽限里不看人；2.4 秒起又面朝下。
// 主角在 (22,4) 被看见；趁它愣着躲到墙后 (22,6)：(22,5) 是墙、(23,5) 是宝箱，它得绕过去。
TEST_F(FoePatrolWorld, ItChasesAroundObstaclesAndCatchingUpIsAPlainFight) {
    run(2.5);
    ASSERT_EQ(walker("foe_chase")->facing, kDown) << "先验：此刻面朝下";
    ASSERT_EQ(walker("foe_chase")->mode, Mode::Patrol);
    stand({22, 4}, kUp);
    tick();
    ASSERT_EQ(walker("foe_chase")->mode, Mode::Chase) << "隔一格，看见了";
    const WorldFrame::Foe* seen = framed("foe_chase");
    ASSERT_NE(seen, nullptr);
    EXPECT_TRUE(seen->alert) << "头顶冒「!」";
    EXPECT_FLOAT_EQ(seen->stepSeconds, 0.25f) << "追一格 0.25 秒（比巡逻快）";

    stand({22, 6}, kUp);   // 躲到墙后
    std::vector<Point> trail;
    bool caught = false;
    for (int frame = 0; frame < 240 && !caught; ++frame) {   // 至多 4 秒
        tick();
        caught = battling();
        const Point at = walker("foe_chase")->cell;
        if (trail.empty() || !(trail.back() == at)) trail.push_back(at);
    }
    ASSERT_TRUE(caught) << "追上了就开打";
    EXPECT_FALSE(app_.battleStart().preemptive) << "被追上：与「迈进它面前那一格」同一判据，不算先手";
    EXPECT_EQ(app_.battleStart().battleId, kWolfBattle);
    for (const Point& p : trail) {
        EXPECT_FALSE(p == (Point{21, 5}) || p == (Point{22, 5})) << "不穿墙";
        EXPECT_FALSE(p == (Point{23, 5})) << "不穿宝箱";
        EXPECT_LE(std::abs(p.x - 22) + std::abs(p.y - 2), 6) << "拴绳 6 格以内";
    }
    // 绕过墙的最短路：左边经 (20,4)(20,5)(20,6) 到 (21,6)，或右边经 (24,4)(24,5)(24,6) 到 (23,6)，都是 7 步。
    ASSERT_EQ(trail.size(), 8u) << "原地 + 7 步";
    const Point last = trail.back();
    EXPECT_TRUE(last == (Point{21, 6}) || last == (Point{23, 6})) << "停在主角身边";
    EXPECT_EQ(walker("foe_chase")->facing, last.x < 22 ? kRight : kLeft) << "转身面朝主角";
}

// 接上一条：逃跑回来人还站在它面前——宽限 3 秒里不开打、不看人，它放弃回家，走开了。
TEST_F(FoePatrolWorld, AfterAnEscapeThereIsAGraceBeforeItCanFightAgain) {
    run(2.5);
    ASSERT_EQ(walker("foe_chase")->facing, kDown);
    stand({22, 4}, kUp);
    tick();
    ASSERT_EQ(walker("foe_chase")->mode, Mode::Chase);
    // 不躲：它扑过来一步到 (22,3)，主角正在它面前。
    for (int frame = 0; frame < 60 && !battling(); ++frame) tick();
    ASSERT_TRUE(battling());
    EXPECT_FALSE(app_.battleStart().preemptive);
    finishBattle(false, "escaped");
    EXPECT_EQ(app_.state().position, (Point{22, 4})) << "被追上的那一仗，逃跑就站回原地";
    EXPECT_DOUBLE_EQ(scene().foePatrol().grace(), 3.0) << "开打宽限 3 秒，回到世界层才开始倒数";
    EXPECT_EQ(walker("foe_chase")->mode, Mode::Return) << "在追的放弃了";
    EXPECT_FALSE(walker("foe_chase")->alert);

    tick();
    EXPECT_EQ(walker("foe_chase")->cell, (Point{22, 3}));
    EXPECT_EQ(walker("foe_chase")->facing, kDown) << "主角此刻就在它面前那一格";
    EXPECT_FALSE(battling()) << "宽限里不因此开打";
    for (int frame = 0; frame < 168; ++frame) {   // 2.8 秒
        tick();
        ASSERT_FALSE(battling()) << "第 " << frame << " 帧";
        ASSERT_NE(walker("foe_chase")->mode, Mode::Chase) << "宽限里不看人";
    }
    EXPECT_EQ(walker("foe_chase")->cell, (Point{22, 2})) << "它走回了原地";
    EXPECT_EQ(walker("foe_chase")->mode, Mode::Patrol) << "接着原地张望";
}

// foe_leash：(32,2) 原地张望。看见 (32,4) 的主角，主角退到 (32,10)：它追到离家 6 格的 (32,8) 就不再往前，回去。
TEST_F(FoePatrolWorld, BeyondTheLeashItGivesUpAndWalksHome) {
    run(2.5);
    ASSERT_EQ(walker("foe_leash")->facing, kDown);
    stand({32, 4}, kUp);
    tick();
    ASSERT_EQ(walker("foe_leash")->mode, Mode::Chase);
    stand({32, 10}, kUp);
    int deepest = 2;
    bool gaveUp = false;
    for (int frame = 0; frame < 360; ++frame) {   // 6 秒
        tick();
        ASSERT_FALSE(battling());
        deepest = std::max(deepest, walker("foe_leash")->cell.y);
        gaveUp = gaveUp || walker("foe_leash")->mode == Mode::Return;
        ASSERT_EQ(walker("foe_leash")->cell.x, 32);
    }
    EXPECT_EQ(deepest, 8) << "拴绳 6 格：追到 (32,8) 为止";
    EXPECT_TRUE(gaveUp) << "放弃过";
    EXPECT_EQ(walker("foe_leash")->cell, (Point{32, 2})) << "走回了巡逻路线";
    EXPECT_EQ(walker("foe_leash")->mode, Mode::Patrol) << "接着巡逻";
}

// foe_shy：(4,10) 原地张望。口袋 (7,10) 上下是墙、右边是传送点、左边 (6,10) 挨着踏入触发区：它看得见主角，
// 却一格都不往那边挨——传送点与踏入触发区连同旁边一格它都不站。闻不到路两秒就放弃。
TEST_F(FoePatrolWorld, ItNeverStandsOnOrNextToAPortalOrAnEnterTrigger) {
    run(2.5);
    ASSERT_EQ(walker("foe_shy")->facing, kRight);
    stand({7, 10}, kLeft);
    tick();
    ASSERT_EQ(walker("foe_shy")->mode, Mode::Chase) << "直线三格：看见了";
    bool gaveUp = false;
    for (int frame = 0; frame < 180; ++frame) {   // 3 秒
        tick();
        ASSERT_FALSE(battling());
        ASSERT_EQ(walker("foe_shy")->cell, (Point{4, 10})) << "口袋外一格都挨不过去";
        gaveUp = gaveUp || walker("foe_shy")->mode == Mode::Return;
    }
    EXPECT_TRUE(gaveUp) << "走不过去：2 秒后放弃";
    EXPECT_EQ(app_.state().flag("probe.noop"), 0) << "踏入触发区没被惊动";
}

// ---------------------------------------------------------------------------
// 停住与回原位
// ---------------------------------------------------------------------------

TEST_F(FoePatrolWorld, ItStandsStillWhileAMenuADialogueOrAScriptIsOn) {
    run(3.0);
    ASSERT_EQ(walker("foe_walk")->cell, (Point{2, 3}));
    const auto snapshot = [&] {
        const Walker* w = walker("foe_walk");
        return std::make_tuple(w->cell.x, w->cell.y, w->facing, w->timer);
    };

    app_.openMainMenu();
    tick();
    ASSERT_EQ(top(), "Menu");
    const auto inMenu = snapshot();
    run(2.0);
    EXPECT_EQ(snapshot(), inMenu) << "菜单开着：一动不动";
    app_.popScene();
    tick();
    ASSERT_EQ(top(), "World");

    app_.showMessage("ui.world.chest.empty");
    tick();
    ASSERT_EQ(top(), "Dialogue");
    const auto inDialogue = snapshot();
    run(2.0);
    EXPECT_EQ(snapshot(), inDialogue) << "对话框开着：一动不动";
    app_.popScene();
    tick();
    ASSERT_EQ(top(), "World");

    // 剧情脚本在跑（一句对白等着按确认）：同样停住。
    ASSERT_TRUE(app_.startEvent("t/hold.lua").ok);
    tick();
    ASSERT_TRUE(app_.scripts().isRunning() || app_.awaitingCommand());
    const auto inScript = snapshot();
    run(2.0);
    EXPECT_EQ(snapshot(), inScript) << "脚本在跑：一动不动";
    for (int guard = 0; guard < 16 && (app_.scripts().isRunning() || app_.awaitingCommand()); ++guard) {
        if (app_.awaitingCommand()) {
            fanren::script::CommandResult result;
            result.ok = true;
            app_.completeCommand(result);
            app_.popScene();
        }
        tick();
    }
    ASSERT_FALSE(app_.scripts().isRunning());
    ASSERT_EQ(top(), "World");
    run(0.5);
    EXPECT_EQ(walker("foe_walk")->cell, (Point{2, 4})) << "都收了接着走";
}

TEST_F(FoePatrolWorld, LoadingASaveOrReenteringTheMapPutsItBackHome) {
    run(3.5);
    ASSERT_EQ(walker("foe_walk")->cell, (Point{2, 4}));
    saveAndReload();
    const WorldFrame::Foe* seen = framed("foe_walk");
    ASSERT_NE(seen, nullptr);
    EXPECT_EQ(seen->cell, (Point{2, 2})) << "读档：站位不进存档，回到图上那一格";
    EXPECT_EQ(seen->facing, kDown) << "面朝 facing 属性";
    EXPECT_EQ(walker("foe_walk")->cell, (Point{2, 2}));

    run(3.5);
    ASSERT_EQ(walker("foe_walk")->cell, (Point{2, 4}));
    ASSERT_TRUE(app_.loadMap("probe_patrol", std::string{}).ok);
    tick();
    EXPECT_EQ(walker("foe_walk")->cell, (Point{2, 2})) << "换图（重进这张图）：回原位";
    EXPECT_DOUBLE_EQ(scene().foePatrol().grace(), 1.0 - 1.0 / 60.0) << "进图宽限 1 秒，这一帧已经走了一帧";
}

// 没写 patrol 的明雷与改版前一样：站着不动、不看不追。
TEST_F(FoePatrolWorld, AFoeWithoutAPatrolStillStandsStill) {
    stand({34, 10}, kRight);   // 它面朝左，隔一格就在它直线上
    run(3.0);
    EXPECT_EQ(world_->foePatrol().find(app_.currentMap(), foeObject("foe_still")), nullptr) << "没写 patrol：不归巡逻管";
    ASSERT_NE(world_->foePatrol().find(app_.currentMap(), foeObject("foe_walk")), nullptr) << "对照：写了的归它管";
    EXPECT_FALSE(battling()) << "不巡逻的不看人";
    const WorldFrame::Foe* seen = framed("foe_still");
    ASSERT_NE(seen, nullptr);
    EXPECT_EQ(seen->cell, (Point{36, 10}));
    EXPECT_EQ(seen->facing, kLeft);
    EXPECT_FALSE(seen->alert);
    ASSERT_TRUE(scene().tryStep(app_, 1, 0));   // 迈进它面前那一格 (35,10)：照旧开打
    tick();
    EXPECT_TRUE(battling());
    EXPECT_FALSE(app_.battleStart().preemptive);
}

// ---------------------------------------------------------------------------
// patrol 的写法
// ---------------------------------------------------------------------------

MapObject foeWithPatrol(Point at, const std::string& patrol) {
    MapObject foe;
    foe.type = "foe";
    foe.position = at;
    foe.properties["patrol"] = patrol;
    return foe;
}

TEST(FoePatrolRoute, WaypointsAreJoinedByStraightRuns) {
    using fanren::game::foepatrol::patrolRoute;
    EXPECT_EQ(patrolRoute(foeWithPatrol({2, 2}, "2,5")), (std::vector<Point>{{2, 2}, {2, 3}, {2, 4}, {2, 5}}));
    EXPECT_EQ(patrolRoute(foeWithPatrol({2, 2}, "4,2;4,0")),
              (std::vector<Point>{{2, 2}, {3, 2}, {4, 2}, {4, 1}, {4, 0}}))
        << "拐一个弯";
    EXPECT_EQ(patrolRoute(foeWithPatrol({5, 5}, " 3 , 5 ")), (std::vector<Point>{{5, 5}, {4, 5}, {3, 5}}))
        << "往回走、带空格";
    EXPECT_EQ(patrolRoute(foeWithPatrol({5, 5}, "5,5")), (std::vector<Point>{{5, 5}})) << "只写自己那一格：原地张望";
    EXPECT_TRUE(patrolRoute(foeWithPatrol({5, 5}, "")).empty()) << "没写：不巡逻";
    EXPECT_TRUE(patrolRoute(foeWithPatrol({5, 5}, "6,6")).empty()) << "斜着走：写坏了，当没写";
    EXPECT_TRUE(patrolRoute(foeWithPatrol({5, 5}, "5,7;")).empty()) << "末尾多一个分号：写坏了";
    EXPECT_TRUE(patrolRoute(foeWithPatrol({5, 5}, "a,5")).empty());
    EXPECT_TRUE(patrolRoute(foeWithPatrol({5, 5}, "5")).empty());
}

// ---------------------------------------------------------------------------
// 第 7 章的数据：正史 12 只、历练副本 13 只都巡逻；路线走得通、不挨剧情；站在每一个入口原地不动，没有谁找上门
// ---------------------------------------------------------------------------

struct MapFoes {
    const char* map;
    int foes;   // 这张图上的明雷几只（全都写了 patrol）
};
constexpr MapFoes kChapterSevenFoes[] = {
    {"ch07_dixia_zhaoze", 1},         {"ch07_jindi_waiwei", 2},          {"ch07_jindi_zhongxin", 2},
    {"ch07_huanxingshan", 3},         {"ch07_zhaoze_shenchu", 4},        {"lilian_jindi_waiwei", 3},
    {"lilian_jindi_zhongxin", 2},     {"lilian_jindi_huanxingshan", 3},  {"lilian_jindi_zhaoze", 1},
    {"lilian_jindi_shenchu", 3},      {"lilian_jindi_shiku", 1},
};

bool covering(const MapObject& object, Point cell) {
    return cell.x >= object.position.x && cell.x < object.position.x + std::max(1, object.width) &&
           cell.y >= object.position.y && cell.y < object.position.y + std::max(1, object.height);
}

TEST(FoePatrolChapterSeven, EveryFoeWalksARouteOfOpenCellsAwayFromPortalsAndEnterTriggers) {
    int total = 0;
    for (const MapFoes& entry : kChapterSevenFoes) {
        SCOPED_TRACE(entry.map);
        const auto loaded = fanren::io::loadTileMap((repoRoot() / "maps" / (std::string(entry.map) + ".tmj")).string());
        ASSERT_TRUE(loaded.ok) << loaded.error;
        const fanren::core::TileMap& map = loaded.value;
        int foes = 0;
        std::vector<Point> claimed;
        for (const MapObject& foe : map.objects) {
            if (foe.type != "foe") continue;
            ++foes;
            SCOPED_TRACE(foe.name);
            const std::vector<Point> route = fanren::game::foepatrol::patrolRoute(foe);
            ASSERT_FALSE(route.empty()) << "写了 patrol、而且写得对";
            EXPECT_LE(route.size(), 12u) << "一小段路";
            for (const Point& cell : route) {
                EXPECT_TRUE(map.walkable(cell)) << cell.x << "," << cell.y << " 不可走";
                for (const Point& other : claimed) EXPECT_FALSE(other == cell) << "两只的路线重叠";
                for (const MapObject& object : map.objects) {
                    if (&object == &foe || object.type == "spawn" || object.type == "encounter") continue;
                    const bool stepActivated =
                        object.type == "portal" || (object.type == "trigger" && object.property("mode") == "enter");
                    if (stepActivated) {
                        for (const Point& d : {Point{0, 0}, Point{1, 0}, Point{-1, 0}, Point{0, 1}, Point{0, -1}}) {
                            EXPECT_FALSE(covering(object, Point{cell.x + d.x, cell.y + d.y}))
                                << cell.x << "," << cell.y << " 挨着 " << object.name;
                        }
                    } else if (object.type != "trigger") {
                        EXPECT_FALSE(covering(object, cell)) << cell.x << "," << cell.y << " 压着 " << object.name;
                    }
                }
            }
            claimed.insert(claimed.end(), route.begin(), route.end());
            if (foe.property("elite") == "true") {
                EXPECT_EQ(route.size(), 1u) << "守宝箱的精英碧水鳄原地转身";
            } else {
                EXPECT_GE(route.size(), 4u) << "别的都在一小段路上来回走";
            }
        }
        EXPECT_EQ(foes, entry.foes);
        total += foes;
    }
    EXPECT_EQ(total, 25) << "正史 12 只、历练 13 只";
}

class FoePatrolChapterSevenEntrances : public ::testing::Test {
protected:
    void SetUp() override {
        auto ready = app_.init(repoRoot().string(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
    }
    void TearDown() override { app_.shutdown(); }
    Application app_;
};

// 每张图的每一个落点：人站着不动 15 秒，没有哪只明雷巡逻、张望到能看见他、找上门来——进门那一刻不会被堵在门口。
TEST_F(FoePatrolChapterSevenEntrances, StandingStillOnAnyEntranceNoFoeComesCalling) {
    int entrances = 0;
    for (const MapFoes& entry : kChapterSevenFoes) {
        ASSERT_TRUE(app_.loadMap(entry.map, std::string{}).ok) << entry.map;
        std::vector<std::string> spawns;
        for (const MapObject& object : app_.currentMap()->objects) {
            if (object.type == "spawn") spawns.push_back(object.property("id"));
        }
        for (const std::string& spawn : spawns) {
            SCOPED_TRACE(std::string(entry.map) + " " + spawn);
            ASSERT_TRUE(app_.loadMap(entry.map, spawn).ok);
            app_.replaceScene(std::make_unique<WorldScene>());
            app_.tick(1.0 / 60.0);
            ASSERT_NE(dynamic_cast<WorldScene*>(app_.topScene()), nullptr);
            const Point at = app_.state().position;
            bool someoneWalked = false;
            for (int frame = 0; frame < 900; ++frame) {
                app_.tick(1.0 / 60.0);
                ASSERT_EQ(dynamic_cast<BattleScene*>(app_.topScene()), nullptr) << "第 " << frame << " 帧有明雷找上门";
                ASSERT_EQ(app_.state().position, at);
                auto* world = dynamic_cast<WorldScene*>(app_.topScene());
                ASSERT_NE(world, nullptr);
                for (const Walker& w : world->foePatrol().walkers()) {
                    someoneWalked = someoneWalked || !(w.cell == w.route.front());
                    ASSERT_NE(w.mode, Mode::Chase) << w.id << " 看见了站在入口的人";
                }
            }
            EXPECT_TRUE(someoneWalked) << "先验：世界层在走，明雷在巡逻";
            ++entrances;
        }
    }
    EXPECT_EQ(entrances, 22) << "11 张图上的落点一个不漏";
}

// ---------------------------------------------------------------------------
// 画面：滑步插值、颠动的相位、排序
// ---------------------------------------------------------------------------

MapObject foeObjectAt(const std::string& id, Point at) {
    MapObject foe;
    foe.name = id;
    foe.type = "foe";
    foe.position = at;
    foe.properties["id"] = id;
    return foe;
}

WorldFrame::Foe framedAt(const MapObject& object, Point cell, float stepSeconds) {
    WorldFrame::Foe foe{&object, "wild_wolf", 2, false};
    foe.cell = cell;
    foe.stepSeconds = stepSeconds;
    return foe;
}

TEST(FoePatrolView, AOneCellMoveSlidesForTheFoesOwnStepAndALongerOneSnaps) {
    const MapObject wolf = foeObjectAt("probe.wolf", {4, 4});
    FoeTrack track;
    track.sync({framedAt(wolf, {4, 4}, 0.5f)}, "probe");
    ASSERT_EQ(track.slots().size(), 1u);
    EXPECT_FALSE(track.slots()[0].slide.moving()) << "第一次见：就在那一格";

    track.sync({framedAt(wolf, {4, 5}, 0.5f)}, "probe");
    EXPECT_TRUE(track.slots()[0].slide.moving());
    track.advance(0.25f);
    EXPECT_NEAR(track.slots()[0].slide.position().y, 4.5f, 1e-5f) << "巡逻一格 0.5 秒：0.25 秒滑到一半";
    EXPECT_NEAR(track.slots()[0].walkClock, 0.25f, 1e-6f) << "走着：颠动的相位在走";
    track.advance(0.25f);
    EXPECT_NEAR(track.slots()[0].slide.position().y, 5.f, 1e-5f);
    EXPECT_FALSE(track.slots()[0].slide.moving());
    track.advance(0.1f);
    EXPECT_FLOAT_EQ(track.slots()[0].walkClock, 0.f) << "停下归零";

    track.sync({framedAt(wolf, {5, 5}, 0.25f)}, "probe");
    track.advance(0.125f);
    EXPECT_NEAR(track.slots()[0].slide.position().x, 4.5f, 1e-5f) << "追一格 0.25 秒";

    track.sync({framedAt(wolf, {4, 4}, 0.5f)}, "probe");
    EXPECT_FALSE(track.slots()[0].slide.moving()) << "一下变了两格（回原位）：瞬移";
    EXPECT_NEAR(track.slots()[0].slide.position().x, 4.f, 1e-5f);

    track.sync({}, "probe");
    EXPECT_TRUE(track.slots().empty()) << "帧里没有了（打赢了）就拿掉";

    track.sync({framedAt(wolf, {4, 4}, 0.5f)}, "probe");
    track.sync({framedAt(wolf, {4, 5}, 0.5f)}, "other");
    EXPECT_FALSE(track.slots()[0].slide.moving()) << "换了图：整张表重来，不从上一张图滑过来";
}

TEST(FoePatrolView, AFoeIsSortedByWhereItIsDrawnNotWhereTheMapPutsIt) {
    const MapObject wolf = foeObjectAt("probe.wolf", {3, 2});
    WorldFrame frame;
    frame.foes.push_back(framedAt(wolf, {3, 7}, 0.5f));
    const fanren::engine::RectF screen{0.f, 0.f, 1280.f, 720.f};
    using Kind = fanren::game::WorldDrawItem::Kind;
    // 主角在第 5 行：帧里那只明雷此刻在第 7 行，排在主角后面（压在前面）。
    auto items = fanren::game::depthOrder(frame, Vec2f{3.f, 5.f}, {}, screen);
    ASSERT_EQ(items.size(), 2u);
    EXPECT_EQ(items[0].kind, Kind::Player);
    EXPECT_EQ(items[1].kind, Kind::Foe);
    // 画面上它正从第 7 行滑回第 4 行的半路：按画面上的位置排，排到主角前面。
    const std::vector<Vec2f> drawn{Vec2f{3.f, 4.5f}};
    items = fanren::game::depthOrder(frame, Vec2f{3.f, 5.f}, {}, screen, &drawn);
    ASSERT_EQ(items.size(), 2u);
    EXPECT_EQ(items[0].kind, Kind::Foe);
    EXPECT_EQ(items[1].kind, Kind::Player);
}

}  // namespace
