// 伙伴改版 · 跨路的用例（集成那一步，docs/interfaces-companions.md 第 6 节「四路交回之后」）。
// 每一条都走真实入口，核的是两路之间那根线第一次真的接上：
//   1. 在历练里偷袭一只明雷：世界逻辑（C）递 foe.battle 原样 → Application::startBattle（D）拼档的后缀 →
//      战斗场景（B）按 battleStart().battleId 认到先手；打赢，明雷记进「这一趟」的账而不是永久那份；
//      打输，整趟退出，气血回写落在这一趟的队伍上、出来之后原队伍与原图原位都回来。
//   2. 带两位伙伴在一张真图上按方向键走：世界逻辑填的足迹经世界画面（A）画出来，跟随者落在主角走过的格子上、
//      与主角同一帧起步、同一帧到格。
// 第三条（C 路「历练里打输整趟退出」在 D 路改过 leaveLilian 之后重跑）是 CompanionsWorld 里那条原样的用例，
// 集成分支的全量构建里跑。
#include <SDL3/SDL.h>
#include <gtest/gtest.h>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "BattleHand.h"
#include "CompanionsLaneDFixture.h"
#include "TempDir.h"
#include "game/Application.h"
#include "game/BattleScene.h"
#include "game/WorldScene.h"
#include "game/WorldView.h"

namespace {

namespace fs = std::filesystem;
namespace laned = fanren::test::laned;
using fanren::core::Point;
using fanren::core::battle::BattlePhase;
using fanren::game::Application;
using fanren::game::BattleScene;
using fanren::game::WorldScene;

// 照一张现成的编成另存一份：id 换掉、敌人可以换成别人。
void copyBattle(const fs::path& root, const std::string& from, const std::string& to,
                const std::vector<std::pair<std::string, std::string>>& swaps = {}) {
    std::ifstream in(laned::repoRoot() / "data" / "battles" / (from + ".json"), std::ios::binary);
    std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    std::vector<std::pair<std::string, std::string>> all{{from, to}};
    all.insert(all.end(), swaps.begin(), swaps.end());
    for (const auto& [a, b] : all) {
        for (auto at = text.find(a); at != std::string::npos; at = text.find(a, at + b.size())) text.replace(at, a.size(), b);
    }
    laned::writeFile(root / "data" / "battles" / (to + ".json"), text);
}

// 一处探针历练地 probe_cross：入口图 lilian_probe_cross，(12,11) 出生点面朝左、(11,11) 出口。
// 两只明雷都面朝右（看着 x+1 那一格）：狼在 (14,11)（谷外孤狼的编成，打得过），强敌在 (14,13)
// （同一张编成把狼换成第 13 章的强敌，凡人打不过；C 路夹具同一个做法）。都不致命、可逃（契约 3.4）。
void addCrossSite(const fs::path& root) {
    for (const char* tier : {"_t1", "_t2", "_t3"}) {
        copyBattle(root, "be03_guwai_gulang", std::string("probe_wolf") + tier);
        copyBattle(root, "be03_guwai_gulang", std::string("probe_hard") + tier, {{"wild_wolf", "feng_xi"}});
    }
    laned::writeFile(root / "data" / "lilian" / "probe_cross.json", R"({
      "id": "probe_cross", "name_key": "probe.lilian.name", "unlock_flag": "probe.lilian.open",
      "entry": {"map": "lilian_probe_cross", "spawn": "rukou"}, "maps": ["lilian_probe_cross"],
      "tiers": [{"max_realm": "QiRefining13"}, {"max_realm": "FoundationEarly"}, {"max_realm": "FoundationMid"}]})");
    std::vector<std::string> objects = laned::standAndFacility("rukou", "probe_cross");
    objects.push_back(laned::object(30, "foe_wolf", "foe", 14, 11,
                                    {laned::prop("id", "lilian_probe_cross.foe_wolf"), laned::prop("battle", "probe_wolf"),
                                     laned::prop("look", "wild_wolf"), laned::prop("facing", "right")}));
    objects.push_back(laned::object(31, "foe_hard", "foe", 14, 13,
                                    {laned::prop("id", "lilian_probe_cross.foe_hard"), laned::prop("battle", "probe_hard"),
                                     laned::prop("look", "wild_wolf"), laned::prop("facing", "right")}));
    laned::writeMap(root, "lilian_probe_cross", objects);
}

class CrossLane : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        root_ = laned::makeRoot("fanren_companions_cross");
        addCrossSite(root_->path());
    }
    static void TearDownTestSuite() { root_.reset(); }

    void SetUp() override {
        auto ready = app_.init(root_->path().string(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
        ASSERT_NE(app_.lilianSite("probe_cross"), nullptr);
        ASSERT_TRUE(app_.loadMap("probe_home", std::string{}).ok);
        app_.state().day = 40;
        // 进去之前的队伍：曲魂，带着伤（17 / 55）。
        app_.state().party = {{"qu_hun", 17, true}};
        app_.pushScene(std::make_unique<WorldScene>());
        tick();
    }
    void TearDown() override { app_.shutdown(); }

    void tick(int frames = 1) {
        for (int i = 0; i < frames; ++i) app_.tick(1.0 / 60.0);
    }
    // 栈顶的世界层；不是世界层（压着战斗、对白）时记一条失败并返回空，调用方判空——不解引用空指针崩掉。
    WorldScene* world() {
        auto* scene = dynamic_cast<WorldScene*>(app_.topScene());
        EXPECT_NE(scene, nullptr) << "栈顶不是世界层：" << (app_.topScene() != nullptr ? app_.topScene()->name() : "空栈");
        return scene;
    }
    void enter() {
        ASSERT_TRUE(app_.enterLilian({"probe_cross", "lilian_probe_cross", "rukou", {"zhang_tie"}}).ok);
        tick();
        ASSERT_NE(dynamic_cast<WorldScene*>(app_.topScene()), nullptr);
        ASSERT_EQ(app_.state().mapId, "lilian_probe_cross");
        ASSERT_EQ(app_.state().position, (Point{12, 11}));
    }
    // 绕到明雷背后（它面朝右，背后是左边那一格），面朝它按确认：偷袭（契约 3.4）。真开打，交给那只手打完。
    BattlePhase ambushAndFight(const std::vector<std::pair<int, int>>& path, const std::string& expectedBattle) {
        for (const auto& [dx, dy] : path) {
            WorldScene* scene = world();
            if (scene == nullptr) return BattlePhase::Ongoing;
            EXPECT_TRUE(scene->tryStep(app_, dx, dy));
            tick(10);
        }
        WorldScene* scene = world();
        if (scene == nullptr) return BattlePhase::Ongoing;
        EXPECT_TRUE(scene->interact(app_));
        tick();
        auto* battle = dynamic_cast<BattleScene*>(app_.topScene());
        EXPECT_NE(battle, nullptr) << "面朝明雷按确认就开打";
        if (battle == nullptr) return BattlePhase::Ongoing;
        // D 拼的后缀与 B 认先手用的是同一个 id：韩立凡人，第 1 档（第一个 max_realm 不低于他的）。
        EXPECT_EQ(app_.battleStart().battleId, expectedBattle);
        EXPECT_TRUE(app_.battleStart().preemptive) << "从背后上去是偷袭";
        EXPECT_TRUE(battle->battle().preemptive()) << "战斗场景认到了先手：编成 id 对得上";
        fanren::test::BattleHand hand(app_);
        const BattlePhase phase = hand.play(*battle);
        tick();
        EXPECT_NE(dynamic_cast<WorldScene*>(app_.topScene()), nullptr) << "收场之后回到世界层";
        return phase;
    }

    static inline std::unique_ptr<fanren::test::TempDir> root_;
    Application app_;
};

}  // namespace

TEST_F(CrossLane, AnAmbushInsideALilianFightsTheTieredBattleWithTheInitiativeAndAWinOnlyCountsForThisTrip) {
    enter();
    app_.state().maxHp = app_.state().hp = 400;   // 这一仗要打赢
    // (12,11) → (13,11)：狼在 (14,11) 面朝右，(13,11) 是它背后。
    ASSERT_EQ(ambushAndFight({{1, 0}}, "probe_wolf_t1"), BattlePhase::Won);
    EXPECT_TRUE(app_.inLilian()) << "打赢了还在历练里";
    EXPECT_EQ(app_.worldObjects().clearedFoes.count("lilian_probe_cross.foe_wolf"), 1u) << "记进这一趟";
    EXPECT_TRUE(app_.state().worldObjects.clearedFoes.empty()) << "永久那份没被写";
    WorldScene* scene = world();
    ASSERT_NE(scene, nullptr);
    for (const auto& foe : scene->describeFrame(app_).foes) {
        EXPECT_NE(foe.object->name, "foe_wolf") << "这一趟里它不在了";
    }
}

TEST_F(CrossLane, LosingAnAmbushInsideALilianEndsTheTripAndTheHpWrittenBackStaysWithTheTripsParty) {
    // 原队伍与这一趟的队伍是同一个人：张铁进去前带着伤（17 / 32），这一趟又选了他（进去时满血）。
    // 战场回写按角色 id 找人——两份队伍里都有张铁，回写落在哪一份，出来之后他的气血就看得出来。
    app_.state().party = {{"zhang_tie", 17, true}};
    enter();
    // (12,11) → (12,12) → (12,13) → (13,13)：强敌在 (14,13) 面朝右，(13,13) 是它背后。
    ASSERT_EQ(ambushAndFight({{0, 1}, {0, 1}, {1, 0}}, "probe_hard_t1"), BattlePhase::Lost);
    EXPECT_FALSE(app_.inLilian()) << "历练里打输：整趟退出";
    EXPECT_EQ(app_.state().mapId, "probe_home");
    EXPECT_EQ(app_.state().position, (Point{12, 11})) << "回到进去之前那一格";
    EXPECT_EQ(app_.state().day, 41) << "出来日历加一天";
    // 战场上回写的气血（打输了，他只剩 1）落在这一趟的队伍上，那份队伍出来就扔了；原队伍的张铁还是
    // 进去之前那一份：17，加出来那一天的静养（上限 32 的一成取整 3）= 20。回写要是错落到原队伍上
    //（先退出历练、后回写），这里会是战场上剩的那个 1。
    ASSERT_EQ(app_.state().party.size(), 1u);
    EXPECT_EQ(app_.state().party[0].roleId, "zhang_tie");
    EXPECT_EQ(app_.state().party[0].hp, 20);
    EXPECT_GE(app_.state().hp, 1) << "韩立打输了也至少留 1 点气血（BattleScene 的老规矩）";
    EXPECT_TRUE(app_.state().worldObjects.clearedFoes.empty());
}

namespace {

void pushKey(SDL_Scancode scancode, bool down) {
    SDL_Event event{};
    event.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
    event.key.scancode = scancode;
    event.key.down = down;
    SDL_PushEvent(&event);
}

// 这一格能不能平白走过去：可走、上面没有任何对象（不踩触发器、不过门、不碰人）。
bool plainCell(const fanren::core::TileMap& map, Point cell) {
    if (!map.walkable(cell)) return false;
    for (const auto& object : map.objects) {
        if (cell.x >= object.position.x && cell.x < object.position.x + object.width && cell.y >= object.position.y &&
            cell.y < object.position.y + object.height) {
            return false;
        }
    }
    return true;
}

}  // namespace

TEST_F(CrossLane, TwoCompanionsFollowTheHerosTrailOnARealMapAndArriveOnTheSameFrame) {
    // 青牛镇（真图）：找一排六格空地，往右走五步不碰任何对象。
    ASSERT_TRUE(app_.loadMap("ch01_qingniuzhen", std::string{}).ok);
    const auto& map = *app_.currentMap();
    Point from{-1, -1};
    for (int y = 2; y < map.height - 2 && from.x < 0; ++y) {
        for (int x = 2; x + 5 < map.width - 2; ++x) {
            bool clear = true;
            for (int i = 0; i <= 5 && clear; ++i) clear = plainCell(map, Point{x + i, y});
            if (clear) {
                from = Point{x, y};
                break;
            }
        }
    }
    ASSERT_GE(from.x, 0) << "青牛镇找不到一排六格空地";
    app_.state().position = from;
    app_.state().facing = 1;
    app_.state().party = {{"zhang_tie", -1, true}, {"baiyi_shaonv", -1, true}};   // probe_a、probe_b
    app_.replaceScene(std::make_unique<WorldScene>());
    tick();
    app_.drawScenes();

    // 按住右键：一帧一帧照游戏的次序走（tick = WorldScene::update，drawScenes = WorldScene::render →
    // 世界逻辑填帧 → WorldView 照帧排跟随者）。
    pushKey(SDL_SCANCODE_RIGHT, true);
    int sharedFrames = 0;
    for (int frame = 0; frame < 90 && app_.state().position.x < from.x + 5; ++frame) {
        app_.tick(1.0 / 60.0);
        app_.drawScenes();
        WorldScene* scene = world();
        ASSERT_NE(scene, nullptr);
        const auto& view = scene->view();
        const auto& slots = view.followers();
        if (slots.size() != 2u || app_.state().position.x < from.x + 3) continue;   // 头两步队伍还在排开
        SCOPED_TRACE(frame);
        // 排开之后：两位伙伴与主角同一个进度——各隔一格，谁滑谁停都一样。
        const float hero = view.playerCell().x;
        EXPECT_NEAR(hero - slots[0].slide.position().x, 1.f, 1e-4f);
        EXPECT_NEAR(hero - slots[1].slide.position().x, 2.f, 1e-4f);
        EXPECT_EQ(slots[0].slide.moving(), view.playerMoving()) << "同一帧起步、同一帧到格";
        EXPECT_EQ(slots[1].slide.moving(), view.playerMoving());
        ++sharedFrames;
    }
    pushKey(SDL_SCANCODE_RIGHT, false);
    tick(20);
    app_.drawScenes();
    ASSERT_GT(sharedFrames, 10) << "排开之后走了好几帧";
    EXPECT_EQ(app_.state().position.x, from.x + 5);

    // 停下来之后：画面上的跟随者站在主角走过的最后两格上，与世界逻辑填的帧一致。
    WorldScene* scene = world();
    ASSERT_NE(scene, nullptr);
    const auto& frame = scene->describeFrame(app_);
    ASSERT_EQ(frame.followers.size(), 2u);
    EXPECT_EQ(frame.followers[0].cell, (Point{from.x + 4, from.y}));
    EXPECT_EQ(frame.followers[1].cell, (Point{from.x + 3, from.y}));
    const auto& slots = scene->view().followers();
    ASSERT_EQ(slots.size(), 2u);
    EXPECT_EQ(slots[0].roleId, "zhang_tie");
    EXPECT_EQ(slots[1].roleId, "baiyi_shaonv");
    EXPECT_NEAR(slots[0].slide.position().x, static_cast<float>(from.x + 4), 1e-4f);
    EXPECT_NEAR(slots[1].slide.position().x, static_cast<float>(from.x + 3), 1e-4f);
    EXPECT_FALSE(slots[0].slide.moving());
    EXPECT_FALSE(scene->view().playerMoving());
}
