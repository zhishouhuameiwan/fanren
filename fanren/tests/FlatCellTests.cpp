// 试玩返修·不挡路（2026-11-20 用户试玩第 7 章地基包）：「像是打坐的蒲团，或者 7 章开始任务提示点，
// 类似这种都不应该阻挡走路，这样才符常理」。规则一句话：画面上是实心东西才挡路；贴地的、看不见的不挡。
//   · 蒲团（facility kind=meditate，全游戏）能走上去；坐在上面、或在旁边面朝它按确认，都开打坐面板；
//     坐在上面时「[Z] 打坐」浮在主角头顶。
//   · 第 7 章会走到的图上，按键型任务点那一格画的是空地的，碰撞清掉：拆包点 (22,3)、林边大树下 (30,25)；
//     画着床、柜、案、门、石灯、树桩、岩壁的照旧挡路（逐格判定见 docs/ch07-foundation-integration.md）。
//   · 明雷照旧不踩蒲团。
// 实图取第 7 章章首存档（百药园居室），明雷那一条用一张探针图。
#include <gtest/gtest.h>

#include <SDL3/SDL.h>

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
#include "game/FoePatrol.h"
#include "game/WorldScene.h"
#include "game/WorldView.h"
#include "io/SaveFile.h"

namespace {

namespace fs = std::filesystem;
using fanren::core::MapObject;
using fanren::core::Point;
using fanren::game::Application;
using fanren::game::WorldScene;

fs::path repoRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "data" / "text" / "ch01_main.json")) return candidate;
    }
    return ".";
}

void writeFile(const fs::path& path, const std::string& text) {
    fs::create_directories(path.parent_path());
    std::ofstream(path, std::ios::binary) << text;
}

void pushKey(SDL_Scancode scancode, bool down) {
    SDL_Event event{};
    event.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
    event.key.scancode = scancode;
    event.key.down = down;
    event.key.repeat = false;
    SDL_PushEvent(&event);
}

constexpr int kUp = 0;
constexpr int kRight = 1;
constexpr int kDown = 2;
constexpr int kLeft = 3;

// ---- 探针图 probe_flat：12×8 的空地，一只来回走 (2,1)→(2,6) 的明雷，(2,4) 一张蒲团 ----
constexpr int kW = 12;
constexpr int kH = 8;
constexpr Point kProbeCushion{2, 4};

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

std::string probeMap() {
    std::string zeros;
    for (int i = 0; i < kW * kH; ++i) zeros += (i == 0 ? "0" : ",0");
    std::string layers;
    for (const char* name : {"ground", "overlay", "building", "front", "collision"}) {
        layers += R"({"name":")" + std::string(name) + R"(","type":"tilelayer","width":)" + std::to_string(kW) +
                  R"(,"height":)" + std::to_string(kH) + R"(,"data":[)" + zeros + "]},";
    }
    const std::vector<std::string> objects{
        object("spawn_main", "spawn", 9, 6, {prop("id", "spawn_main"), propBool("default", true), prop("facing", "up")}),
        object("foe_walk", "foe", 2, 1,
               {prop("id", "probe_flat.foe_walk"), prop("battle", "be03_guwai_gulang"), prop("look", "wild_wolf"),
                prop("facing", "down"), prop("patrol", "2,6")}),
        object("facility_dazuo", "facility", kProbeCushion.x, kProbeCushion.y, {prop("kind", "meditate")}),
    };
    std::string list;
    for (const std::string& o : objects) list += (list.empty() ? "" : ",") + o;
    layers += R"({"name":"objects","type":"objectgroup","objects":[)" + list + "]}";
    return R"({"width":)" + std::to_string(kW) + R"(,"height":)" + std::to_string(kH) +
           R"(,"tilewidth":32,"tileheight":32,"orientation":"orthogonal","layers":[)" + layers + R"(],"properties":[)" +
           prop("map_id", "probe_flat") + "," + prop("display_name_key", "ch01.map.hanjiacun.name") + "," +
           prop("region", "probe") + "," + prop("bgm", "none") + "," + propInt("chapter", 7) + "," +
           propBool("outdoor", true) + "," + propBool("can_leave_edge", false) + "]}";
}

// 百药园居室（第 7 章章首）：蒲团 (23,4)(24,4)，拆包点 (22,3)，书桌两格 (26,4)(26,5)，茅屋前石灯 (23,8)。
constexpr Point kCushion{23, 4};
constexpr Point kBelowCushion{23, 5};
constexpr Point kChaibao{22, 3};
constexpr Point kBelowChaibao{22, 4};

class FlatCells : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        root_ = std::make_unique<fanren::test::TempDir>("fanren_flat_cells");
        for (const char* dir : {"data", "scripts", "maps"}) {
            fs::copy(repoRoot() / dir, root_->path() / dir, fs::copy_options::recursive);
        }
        writeFile(root_->path() / "maps" / "probe_flat.tmj", probeMap());
    }
    static void TearDownTestSuite() { root_.reset(); }

    void SetUp() override {
        auto ready = app_.init(root_->path().string(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
        app_.setTutorialsEnabled(false);   // 走到田边、蒲团边的教学卡不来压栈
    }
    void TearDown() override { app_.shutdown(); }

    // 第 7 章章首：人在百药园居室里，拆包点正等着按。
    void enterChapterStart() {
        auto save = fanren::io::loadGame((repoRoot() / "saves" / "ch07-start.sav").string());
        ASSERT_TRUE(save.ok) << save.error;
        app_.state() = save.value;
        ASSERT_EQ(app_.state().mapId, "ch06_baiyaoyuan");
        ASSERT_EQ(app_.state().flag("ch07.chaibao"), 0) << "先验：章首还没拆包";
        enterWorld("ch06_baiyaoyuan");
    }
    void enterWorld(const std::string& mapId) {
        ASSERT_TRUE(app_.loadMap(mapId, std::string{}).ok);
        app_.pushScene(std::make_unique<WorldScene>());
        tick(2);
        world_ = dynamic_cast<WorldScene*>(app_.topScene());
        ASSERT_NE(world_, nullptr) << "栈顶是 " << top();
    }
    void tick(int frames = 1) {
        for (int i = 0; i < frames; ++i) app_.tick(1.0 / 60.0);
    }
    void press(SDL_Scancode scancode) {
        pushKey(scancode, true);
        tick();
        pushKey(scancode, false);
        tick();
    }
    std::string top() {
        return app_.topScene() == nullptr ? std::string{} : app_.topScene()->name();
    }
    void stand(Point at, int facing) {
        app_.state().position = at;
        app_.state().facing = facing;
    }
    // 把正在跑的那段剧情一句句按完（拆包没有选项）。
    void playThrough() {
        for (int i = 0; i < 200 && (app_.scripts().isRunning() || app_.awaitingCommand()); ++i) {
            press(SDL_SCANCODE_RETURN);
        }
        tick(2);
        ASSERT_FALSE(app_.scripts().isRunning()) << "200 下确认还没演完";
        ASSERT_EQ(top(), "World");
    }

    static inline std::unique_ptr<fanren::test::TempDir> root_;
    Application app_;
    WorldScene* world_ = nullptr;
};

}  // namespace

// ---------------------------------------------------------------------------
// 蒲团
// ---------------------------------------------------------------------------

TEST(FlatCellRule, OnlyTheMeditationCushionIsWalkedOnto) {
    const auto facility = [](const std::string& kind) {
        MapObject object;
        object.type = "facility";
        object.properties["kind"] = kind;
        return object;
    };
    EXPECT_TRUE(WorldScene::walksOnto(facility("meditate")));
    for (const char* solid : {"save", "lilian", "field", "shop", "alchemy", "board", "talisman", "forge"}) {
        EXPECT_FALSE(WorldScene::walksOnto(facility(solid))) << solid << " 照旧占格";
    }
    MapObject trigger;
    trigger.type = "trigger";
    trigger.properties["kind"] = "meditate";
    EXPECT_FALSE(WorldScene::walksOnto(trigger)) << "只认设施";
}

TEST_F(FlatCells, TheCushionCanBeWalkedOnto) {
    enterChapterStart();
    stand(kBelowCushion, kDown);
    EXPECT_TRUE(world_->tryStep(app_, 0, -1)) << "蒲团贴地，走得上去";
    EXPECT_EQ(app_.state().position, kCushion);
    EXPECT_TRUE(world_->tryStep(app_, 1, 0)) << "两格宽的蒲团，另一半也走得上去";
    EXPECT_EQ(app_.state().position, (Point{24, 4}));
}

TEST_F(FlatCells, SittingOnTheCushionAndPressingConfirmOpensTheMeditationPanel) {
    enterChapterStart();
    stand(kBelowCushion, kDown);
    ASSERT_TRUE(world_->tryStep(app_, 0, -1));
    ASSERT_EQ(app_.state().position, kCushion);
    // 面朝上：(23,3) 是一格空地板，面前什么都没有——只能是脚下的蒲团接住这一下。
    ASSERT_EQ(app_.state().facing, kUp);
    EXPECT_TRUE(world_->interact(app_)) << "坐在蒲团上按确认毫无反应";
    tick();
    EXPECT_EQ(top(), "Cultivation");
}

TEST_F(FlatCells, FacingTheCushionFromBesideItStillOpensTheMeditationPanel) {
    enterChapterStart();
    stand(kBelowCushion, kUp);
    EXPECT_TRUE(world_->interact(app_));
    tick();
    EXPECT_EQ(top(), "Cultivation");
}

TEST_F(FlatCells, OnTheCushionTheMeditatePromptFloatsOverTheHero) {
    enterChapterStart();
    stand(kBelowCushion, kUp);
    Point cell{};
    const MapObject* npc = nullptr;
    const std::string beside = world_->interactHint(app_, cell, npc);
    EXPECT_EQ(cell, kCushion) << "在旁边面朝它：提示浮在蒲团上方";
    EXPECT_NE(beside.find(app_.text("ui.world.interact.meditate")), std::string::npos) << beside;

    ASSERT_TRUE(world_->tryStep(app_, 0, -1));
    const std::string on = world_->interactHint(app_, cell, npc);
    EXPECT_EQ(cell, kCushion) << "坐在上面：提示的那一格就是主角自己那格（画在头顶）";
    EXPECT_EQ(cell, app_.state().position);
    EXPECT_EQ(npc, nullptr);
    EXPECT_EQ(on, app_.text("ui.world.interact.key") + " " + app_.text("ui.world.interact.meditate"));
    EXPECT_EQ(app_.text("ui.world.interact.meditate"), "打坐");
}

TEST_F(FlatCells, AFoeOnPatrolNeverStepsOntoACushion) {
    enterWorld("probe_flat");
    const MapObject* foe = nullptr;
    for (const MapObject& object : app_.currentMap()->objects) {
        if (object.name == "foe_walk") foe = &object;
    }
    ASSERT_NE(foe, nullptr);
    bool reachedBefore = false;
    for (int frame = 0; frame < 12 * 60; ++frame) {
        tick();
        const auto* walker = world_->foePatrol().find(app_.currentMap(), *foe);
        ASSERT_NE(walker, nullptr);
        ASSERT_NE(walker->cell, kProbeCushion) << "第 " << frame << " 帧：明雷踩上了蒲团";
        reachedBefore = reachedBefore || walker->cell == (Point{2, 3});
    }
    EXPECT_TRUE(reachedBefore) << "先验：它确实走到了蒲团跟前";
    // 主角照样走得上去。
    stand(Point{3, 4}, kLeft);
    EXPECT_TRUE(world_->tryStep(app_, -1, 0));
    EXPECT_EQ(app_.state().position, kProbeCushion);
}

// ---------------------------------------------------------------------------
// 按键型任务点
// ---------------------------------------------------------------------------

TEST_F(FlatCells, StandingOnTheChaibaoSpotTriggersItOnceOnly) {
    enterChapterStart();
    stand(kBelowChaibao, kRight);
    EXPECT_TRUE(world_->tryStep(app_, 0, -1)) << "拆包点那一格画的是空地板，走得上去";
    ASSERT_EQ(app_.state().position, kChaibao);
    ASSERT_EQ(app_.state().facing, kUp);
    EXPECT_TRUE(world_->interact(app_)) << "站在上面按确认";
    tick(2);
    ASSERT_TRUE(app_.scripts().isRunning() || app_.awaitingCommand());
    playThrough();
    EXPECT_EQ(app_.state().flag("ch07.chaibao"), 1);

    // 只触发一次：站在上面再按、退下来面朝它再按，都不再演。
    EXPECT_FALSE(world_->interact(app_));
    tick(2);
    EXPECT_FALSE(app_.scripts().isRunning());
    ASSERT_TRUE(world_->tryStep(app_, 0, 1));
    stand(kBelowChaibao, kUp);
    EXPECT_FALSE(world_->interact(app_));
    tick(2);
    EXPECT_FALSE(app_.scripts().isRunning());
}

TEST_F(FlatCells, FacingTheChaibaoSpotTriggersItOnceOnly) {
    enterChapterStart();
    stand(kBelowChaibao, kUp);
    EXPECT_TRUE(world_->interact(app_)) << "面朝它按确认";
    tick(2);
    playThrough();
    EXPECT_EQ(app_.state().flag("ch07.chaibao"), 1);
    EXPECT_FALSE(world_->interact(app_));
    ASSERT_TRUE(world_->tryStep(app_, 0, -1));
    EXPECT_FALSE(world_->interact(app_)) << "走上去再按也不演第二遍";
    tick(2);
    EXPECT_FALSE(app_.scripts().isRunning());
}

TEST_F(FlatCells, TheSpotsThatAreDrawnAsSolidThingsStillBlock) {
    enterChapterStart();
    // 书桌两格（读方、三年）与茅屋前的石灯（药篓）：画着实心东西，照旧走不上去。
    for (const auto& [from, step, name] : std::vector<std::tuple<Point, Point, const char*>>{
             {Point{25, 4}, Point{1, 0}, "trigger_dufang"},
             {Point{25, 5}, Point{1, 0}, "trigger_sannian"},
             {Point{23, 9}, Point{0, -1}, "trigger_yaolou"},
         }) {
        stand(from, kDown);
        ASSERT_TRUE(app_.currentMap()->walkable(from)) << name << " 旁边那一格本该站得住";
        EXPECT_FALSE(world_->tryStep(app_, step.x, step.y)) << name;
        EXPECT_EQ(app_.state().position, from) << name;
    }
}

// 第 7 章会走到的图上、落在碰撞格上的按键型任务点一共 27 处：清 3 处（第一批 2 处、第二批墙角蒲团 1 处），其余 24 处照旧挡路。
// 名单与判定照 docs/ch07-foundation-integration.md「试玩返修·不挡路」那张表；图改了、判定变了，两边一起改。
TEST_F(FlatCells, EveryChapterSevenInteractSpotKeepsItsJudgedCollision) {
    const std::vector<std::pair<std::string, std::vector<std::string>>> cleared{
        {"ch06_baiyaoyuan", {"trigger_chaibao"}},
        {"ch07_jindi_zhongxin", {"trigger_shulin"}},
        {"ch07_dihuo", {"trigger_zhuji"}},   // 第二批返修：底图上的木箱改画成蒲团
    };
    const std::vector<std::pair<std::string, std::vector<std::string>>> kept{
        {"ch06_baiyaoyuan", {"trigger_yaolou", "trigger_dufang", "trigger_sannian"}},
        {"ch06_huangfenggu",
         {"trigger_ye_maidan", "trigger_dadian", "trigger_lin_lingqu", "trigger_wufeng", "trigger_zawu",
          "trigger_juanzong", "trigger_chuling", "trigger_baoming", "trigger_dengji"}},
        {"ch07_dihuo", {"trigger_banian"}},
        {"ch07_dixia_shiku", {"trigger_poding"}},
        {"ch07_dixia_zhaoze", {"trigger_fengkou"}},
        {"ch07_fangshi", {"trigger_wanbaolou"}},
        {"ch07_huanxingshan", {"trigger_sichu", "trigger_qingshidian", "trigger_yujian_dongpo"}},
        {"ch07_jindi_wai", {"trigger_liedui"}},
        {"ch07_shandong", {"trigger_yeyu", "trigger_fanhui"}},
        {"ch07_yuelu_dian", {"trigger_cangshi", "trigger_dihuo_wen"}},
    };
    int clearedCount = 0;
    int keptCount = 0;
    const auto check = [&](const std::vector<std::pair<std::string, std::vector<std::string>>>& list, bool open,
                           int& count) {
        for (const auto& [mapId, names] : list) {
            ASSERT_TRUE(app_.loadMap(mapId, std::string{}).ok) << mapId;
            const fanren::core::TileMap& map = *app_.currentMap();
            for (const std::string& name : names) {
                const MapObject* spot = nullptr;
                for (const MapObject& object : map.objects) {
                    if (object.name == name) spot = &object;
                }
                ASSERT_NE(spot, nullptr) << mapId << " 上没有 " << name;
                EXPECT_EQ(spot->property("mode"), "interact") << name;
                for (int y = 0; y < spot->height; ++y) {
                    for (int x = 0; x < spot->width; ++x) {
                        const Point cell{spot->position.x + x, spot->position.y + y};
                        EXPECT_EQ(map.walkable(cell), open)
                            << mapId << " " << name << " (" << cell.x << "," << cell.y << ")："
                            << (open ? "画的是空地，该清掉碰撞" : "画着实心东西，该照旧挡路");
                    }
                }
                ++count;
            }
        }
    };
    check(cleared, true, clearedCount);
    check(kept, false, keptCount);
    EXPECT_EQ(clearedCount, 3);
    EXPECT_EQ(keptCount, 24);
}

// 全游戏 35 张蒲团：没有一格还被碰撞层标成墙。
TEST_F(FlatCells, NoCushionAnywhereIsAWallInTheCollisionLayer) {
    int cushions = 0;
    for (const auto& entry : fs::directory_iterator(root_->path() / "maps")) {
        if (entry.path().extension() != ".tmj" || entry.path().stem() == "probe_flat") continue;
        const std::string mapId = entry.path().stem().string();
        ASSERT_TRUE(app_.loadMap(mapId, std::string{}).ok) << mapId;
        const fanren::core::TileMap& map = *app_.currentMap();
        for (const MapObject& object : map.objects) {
            if (!WorldScene::walksOnto(object)) continue;
            ++cushions;
            for (int y = 0; y < object.height; ++y) {
                for (int x = 0; x < object.width; ++x) {
                    const Point cell{object.position.x + x, object.position.y + y};
                    EXPECT_TRUE(map.walkable(cell)) << mapId << " " << object.name << " (" << cell.x << "," << cell.y
                                                    << ") 还是墙";
                }
            }
        }
    }
    EXPECT_EQ(cushions, 35);
}

// 坐在蒲团上的人画在蒲团之上：蒲团按矩形上沿排进深度序（按底边排时与人同一个 footY，两格宽的蒲团压在人腿上）。
// 别的设施照旧按底边排（WorldViewCompanions.WithoutTheNewListsTheOrderIsTheOldOne 钉着）。
TEST(FlatCellDepth, TheHeroAndAnyoneOnTheRowAboveAreDrawnOverTheCushion) {
    using Kind = fanren::game::WorldDrawItem::Kind;
    MapObject cushion;
    cushion.type = "facility";
    cushion.name = "facility_dazuo";
    cushion.position = Point{3, 4};
    cushion.width = 2;
    cushion.properties["kind"] = "meditate";
    MapObject above;
    above.type = "npc";
    above.name = "npc_above";
    above.position = Point{4, 3};
    fanren::game::WorldFrame frame;
    frame.facilities.push_back(fanren::game::WorldFrame::Facility{&cushion, "meditate"});
    frame.npcs.push_back(fanren::game::WorldFrame::Npc{&above, "zhang_tie", "张铁", 2, fanren::game::PathBubble::None});
    const fanren::engine::RectF screen{0.f, 0.f, 1280.f, 720.f};
    const auto items = fanren::game::depthOrder(frame, fanren::game::Vec2f{3.f, 4.f}, {}, screen);
    std::vector<Kind> kinds;
    for (const auto& item : items) kinds.push_back(item.kind);
    EXPECT_EQ(kinds, (std::vector<Kind>{Kind::Facility, Kind::Npc, Kind::Player}));
}

// ---------------------------------------------------------------------------
// 第二批返修：地火屋墙角那张蒲团（第 7 章最后一步「去墙角的蒲团坐下」）
// ---------------------------------------------------------------------------

namespace {
constexpr Point kZhuji{28, 1};
constexpr Point kBelowZhuji{28, 2};
}  // namespace

class FlatCellsZhuji : public FlatCells {
protected:
    // 章首存档，摆到「筑基丹炼成了」那一刻：只差墙角这一步。
    void enterDihuo() {
        auto save = fanren::io::loadGame((repoRoot() / "saves" / "ch07-start.sav").string());
        ASSERT_TRUE(save.ok) << save.error;
        app_.state() = save.value;
        app_.state().flags["ch07.chengdan"] = 1;
        ASSERT_EQ(app_.state().flag("ch07.done"), 0);
        enterWorld("ch07_dihuo");
    }
};

TEST_F(FlatCellsZhuji, StandingOnTheCornerCushionStartsTheFoundationScene) {
    enterDihuo();
    stand(kBelowZhuji, kLeft);
    ASSERT_TRUE(app_.currentMap()->walkable(kBelowZhuji));
    EXPECT_TRUE(world_->tryStep(app_, 0, -1)) << "墙角那张蒲团贴地，走得上去";
    ASSERT_EQ(app_.state().position, kZhuji);
    EXPECT_TRUE(world_->interact(app_)) << "坐在上面按确认";
    tick(2);
    EXPECT_TRUE(app_.scripts().isRunning() || app_.awaitingCommand()) << "服丹筑基那一段起来了";
}

TEST_F(FlatCellsZhuji, FacingTheCornerCushionStartsTheFoundationSceneToo) {
    enterDihuo();
    stand(kBelowZhuji, kUp);
    EXPECT_TRUE(world_->interact(app_)) << "在下面一格面朝它按确认";
    tick(2);
    EXPECT_TRUE(app_.scripts().isRunning() || app_.awaitingCommand());
}

// 人站在目标那一格上时目标菱形不画（它会悬在人脸上），离开那一格照常画。
TEST(FlatCellMarker, TheDiamondStepsAsideOnlyWhileTheHeroStandsOnTheTarget) {
    MapObject target;
    target.type = "trigger";
    target.position = Point{22, 3};
    EXPECT_TRUE(fanren::game::standsOnObjective(target, Point{22, 3}));
    EXPECT_FALSE(fanren::game::standsOnObjective(target, Point{22, 4}));
    EXPECT_FALSE(fanren::game::standsOnObjective(target, Point{23, 3}));
    target.width = 2;
    target.height = 2;
    EXPECT_TRUE(fanren::game::standsOnObjective(target, Point{23, 4})) << "占几格的目标，站在哪一格上都算";
    EXPECT_FALSE(fanren::game::standsOnObjective(target, Point{24, 4}));
}
