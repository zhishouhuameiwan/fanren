// 野外遭遇的游戏层（docs/interfaces-octo-encounters.md）：无头下走真地图、真遭遇表。
//
// 规则层自己的判定（步数区间、每日上限、境界过滤、权重）在 tests/EconomyTests.cpp 的 Encounter 组；
// 这里只问「游戏层照着规则层的话做了没有」：
//   · 开关：Application 缺省关闭，启动器才打开——关着的时候走多少步都不数；
//   · 区：只有站在遭遇区里、区的 require_flag 已置，这一步才数；
//   · 每日上限：到了上限这一天不再遇，过一天恢复；
//   · 负了不 game over：气血 1、就地不动、旁白一句；胜了发编成奖励；
//   · 地名横幅的危险度随境界与区的开关变；
//   · 计数器进存档（v8），v7 老档读进来是全 0，写坏了的拒读；
//   · 脚本 bgm 命令：点播换图不撤，bgm("map") 撤回地图曲。
#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>

#include "TempDir.h"
#include "core/model/Types.h"
#include "core/rules/Realm.h"
#include "game/Application.h"
#include "game/ShopScene.h"
#include "game/WorldHud.h"
#include "game/WorldScene.h"
#include "io/SaveFile.h"

namespace {

namespace fs = std::filesystem;

using fanren::core::GameState;
using fanren::core::MapObject;
using fanren::core::Point;
using fanren::game::Application;
using fanren::game::WorldScene;
using fanren::rules::Realm;

constexpr double kFrame = 1.0 / 60.0;

fs::path findProjectRoot() {
    fs::path dir = fs::current_path();
    for (int depth = 0; depth < 8; ++depth) {
        if (fs::exists(dir / "scripts" / "common" / "api.lua") && fs::is_directory(dir / "data")) return dir;
        const fs::path parent = dir.parent_path();
        if (parent.empty() || parent == dir) break;
        dir = parent;
    }
    return {};
}

class EncounterFlow : public ::testing::Test {
protected:
    void SetUp() override {
        const fs::path root = findProjectRoot();
        ASSERT_FALSE(root.empty()) << "找不到工程根目录";
        auto ready = app_.init(root.string(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
    }
    void TearDown() override { app_.shutdown(); }

    GameState& state() { return app_.state(); }
    std::string top() { return app_.topScene() == nullptr ? std::string("<空>") : app_.topScene()->name(); }

    void enter(const std::string& mapId) {
        auto loaded = app_.loadMap(mapId, std::string{});
        ASSERT_TRUE(loaded.ok) << loaded.error;
        app_.pushScene(std::make_unique<WorldScene>());
        app_.tick(kFrame);
        ASSERT_EQ(top(), "World");
    }

    // 谷外荒坡里找一步：两格都站得上、两格上都没有会抢先应声的对象（触发器、门、人、设施）。
    // 找到了把人摆在起点、面朝东；返回落脚那一格。
    Point standInZone(const std::string& zoneName) {
        const fanren::core::TileMap* map = app_.currentMap();
        const MapObject* zone = nullptr;
        for (const MapObject& object : map->objects) {
            if (object.type == "encounter" && object.name == zoneName) zone = &object;
        }
        if (zone == nullptr) {
            ADD_FAILURE() << "这张图上没有遭遇区 " << zoneName;
            return {};
        }
        const auto clear = [map](Point p) {
            if (!map->walkable(p)) return false;
            for (const char* type : {"trigger", "portal", "npc", "facility"}) {
                if (map->objectAt(p, type) != nullptr) return false;
            }
            return true;
        };
        for (int y = zone->position.y; y < zone->position.y + zone->height; ++y) {
            for (int x = zone->position.x; x + 1 < zone->position.x + zone->width; ++x) {
                const Point from{x, y};
                const Point to{x + 1, y};
                if (!clear(from) || !clear(to)) continue;
                state().position = from;
                state().facing = 1;
                return to;
            }
        }
        ADD_FAILURE() << zoneName << " 里找不到能走的一步";
        return {};
    }

    // 下一步就该摇中：阈值 1 步、今天还一场没遇、当天已是「今天」。
    void primeForNextStep() {
        state().encounter = fanren::rules::EncounterState{};
        state().encounter.stepsUntilNext = 1;
        state().encounter.lastDay = state().day;
    }

    bool step() {
        auto* world = dynamic_cast<WorldScene*>(app_.topScene());
        if (world == nullptr) {
            ADD_FAILURE() << "栈顶不是世界层：" << top();
            return false;
        }
        return world->tryStep(app_, 1, 0);
    }

    // 谷外、教学那一仗打完、炼气一层：第 3 章刚能遇上野兽的那一刻。
    Point readyAtGuwai(Realm realm) {
        enter("ch03_guwai");
        state().setFlag("ch03.elang_done");
        state().realm = realm;
        return standInZone("encounter_huangpo");
    }

    void runScript(const std::string& path) {
        const auto started = app_.startEvent(path);
        ASSERT_TRUE(started.ok) << path << ": " << started.error;
        for (int frame = 0; frame < 4000 && app_.scripts().isRunning(); ++frame) {
            app_.tick(kFrame);
            if (app_.awaitingCommand()) {
                fanren::script::CommandResult result;
                result.ok = true;
                result.choiceIndex = 0;
                app_.completeCommand(result);
            }
        }
        ASSERT_FALSE(app_.scripts().isRunning()) << path << " 没能跑到结束";
    }

    Application app_;
};

TEST_F(EncounterFlow, AStepInsideAnOpenZoneStartsABattleOnceTheLauncherSwitchesItOn) {
    ASSERT_FALSE(app_.encountersEnabled()) << "缺省必须关着：几百条无头测试不该冷不丁被拖进一场仗";
    app_.setEncountersEnabled(true);
    readyAtGuwai(Realm::QiRefining1);
    primeForNextStep();
    ASSERT_TRUE(step());
    app_.tick(kFrame);
    EXPECT_EQ(top(), "Battle") << "摇中了就开战";
    EXPECT_EQ(state().encounter.triggeredToday, 1);
    EXPECT_EQ(state().encounter.stepsSinceLast, 0) << "遇过一场，步数从头数";
}

TEST_F(EncounterFlow, WhileSwitchedOffNoStepIsEvenCounted) {
    const Point to = readyAtGuwai(Realm::QiRefining1);
    primeForNextStep();
    ASSERT_TRUE(step());
    app_.tick(kFrame);
    EXPECT_EQ(state().position, to) << "先验：这一步真的迈出去了";
    EXPECT_EQ(top(), "World");
    EXPECT_EQ(state().encounter.stepsSinceLast, 0) << "关着的时候一步也不数";
    EXPECT_EQ(state().encounter.triggeredToday, 0);
}

TEST_F(EncounterFlow, AZoneStaysQuietUntilItsRequireFlagIsSet) {
    app_.setEncountersEnabled(true);
    enter("ch03_guwai");
    state().realm = Realm::QiRefining1;
    standInZone("encounter_huangpo");
    primeForNextStep();
    ASSERT_EQ(state().flag("ch03.elang_done"), 0) << "先验：教学那一仗还没打";
    ASSERT_TRUE(step());
    app_.tick(kFrame);
    EXPECT_EQ(top(), "World") << "教学那一仗打完之前谷外没有野兽";
    EXPECT_EQ(state().encounter.stepsSinceLast, 0) << "区没开，这一步不数";
    EXPECT_EQ(app_.dangerStars(), 0) << "区没开，横幅不画危险度";
}

TEST_F(EncounterFlow, TheDailyCapHoldsForTheDayAndResetsTheNext) {
    app_.setEncountersEnabled(true);
    const Point to = readyAtGuwai(Realm::QiRefining1);
    primeForNextStep();
    state().encounter.triggeredToday = app_.encounterTables().at("encounter_ch03_guwai").dailyCap;
    ASSERT_TRUE(step());
    app_.tick(kFrame);
    EXPECT_EQ(top(), "World") << "今天的上限到了";

    // 第二天：往回走一格再迈一次（同一格上原地踏不出一步）。
    state().day += 1;
    state().position = Point{to.x - 1, to.y};
    ASSERT_TRUE(step());
    app_.tick(kFrame);
    EXPECT_EQ(top(), "Battle") << "过了一天，上限清零";
    EXPECT_EQ(state().encounter.triggeredToday, 1);
}

TEST_F(EncounterFlow, LosingIsNotGameOverTheHeroStaysPutWithOneHp) {
    app_.setEncountersEnabled(true);
    const Point to = readyAtGuwai(Realm::QiRefining1);
    state().hp = state().maxHp = 1;
    state().mp = state().maxMp = 0;
    const int cultivation = state().cultivation;
    primeForNextStep();
    ASSERT_TRUE(step());
    app_.tick(kFrame);
    ASSERT_EQ(top(), "Battle");
    app_.tick(kFrame);   // 无头：一帧打完
    ASSERT_EQ(top(), "Dialogue") << "先验：这一场要真的输掉，旁白说一句";
    EXPECT_EQ(app_.dialogueLog().back().body, app_.text("ui.encounter.lost"));
    EXPECT_FALSE(app_.quitRequested()) << "野外遭遇输了不 game over";
    EXPECT_EQ(state().hp, 1) << "气血按至少 1 放回来";
    EXPECT_EQ(state().position, to) << "就地不动";
    EXPECT_EQ(state().cultivation, cultivation) << "输了什么也不发";
    app_.popScene();
    app_.tick(kFrame);
    EXPECT_EQ(top(), "World");
}

TEST_F(EncounterFlow, WinningPaysOnlyTheSmallSetupReward) {
    app_.setEncountersEnabled(true);
    readyAtGuwai(Realm::QiRefining4);
    state().hp = state().maxHp = 5000;
    state().mp = state().maxMp = 0;
    const int cultivation = state().cultivation;
    const int money = fanren::game::spiritStones(state());
    primeForNextStep();
    ASSERT_TRUE(step());
    app_.tick(kFrame);
    ASSERT_EQ(top(), "Battle");
    app_.tick(kFrame);
    EXPECT_EQ(top(), "World") << "赢了不说话，直接回到行走";
    // 谷外那几场的编成奖励：修为 3–4、碎银 0–2（data/battles/be03_* / be04_guwai_*）。
    EXPECT_GE(state().cultivation - cultivation, 3) << "先验：这一场要真的打赢";
    EXPECT_LE(state().cultivation - cultivation, 4);
    EXPECT_LE(fanren::game::spiritStones(state()) - money, 2);
}

TEST_F(EncounterFlow, DangerStarsFollowTheRealmAndTheZoneFlag) {
    // 星数的刻度：每多一颗，凶一倍（Application.h 的表）。
    EXPECT_EQ(fanren::game::menaceStars(0), 1);
    EXPECT_EQ(fanren::game::menaceStars(199), 1);
    EXPECT_EQ(fanren::game::menaceStars(200), 2);
    EXPECT_EQ(fanren::game::menaceStars(399), 2);
    EXPECT_EQ(fanren::game::menaceStars(400), 3);
    EXPECT_EQ(fanren::game::menaceStars(1600), 5);
    EXPECT_EQ(fanren::game::menaceStars(1000000), fanren::game::hud::PlaceBanner::kMaxStars);

    enter("ch03_guwai");
    state().realm = Realm::QiRefining1;
    EXPECT_EQ(app_.dangerStars(), 0) << "区还没开";
    state().setFlag("ch03.elang_done");
    EXPECT_EQ(app_.dangerStars(), 1) << "第 3 章：一头狼或一头野猪（凶 72 / 130）";
    state().realm = Realm::QiRefining4;
    EXPECT_EQ(app_.dangerStars(), 2) << "第 4 章的韩立回来：两头野猪（凶 260）";
    state().realm = Realm::QiRefining8;
    EXPECT_EQ(app_.dangerStars(), 0) << "第 5 章的韩立在这里一场也遇不上";

    enter("ch04_getang");
    EXPECT_EQ(app_.dangerStars(), 0) << "没有遭遇区的图";
    enter("ch05_dubashanzhuang");
    EXPECT_EQ(app_.dangerStars(), 2) << "庄外林子：三头野猪（凶 390）";

    EXPECT_EQ(fanren::game::hud::PlaceBanner::dangerLine(2), app_.text("ui.world.danger") + "　★★☆☆☆");
}

// ---- 存档 ----

std::uint64_t fnv1a64(const std::string& data) {
    std::uint64_t hash = 0xcbf29ce484222325ULL;
    for (const char c : data) {
        hash ^= static_cast<unsigned char>(c);
        hash *= 0x100000001b3ULL;
    }
    return hash;
}

// 手拼一份存档：nlohmann 的 dump() 按 key 字母序、无空格，校验和按它算（与 tests/BattleTests.cpp 同法）。
std::string handSave(int version, const std::string& payload) {
    std::ostringstream sum;
    sum << std::hex << std::setw(16) << std::setfill('0') << fnv1a64(std::to_string(version) + "|" + payload);
    return "{\"save_version\":" + std::to_string(version) + ",\"checksum\":\"" + sum.str() +
           "\",\"payload\":" + payload + "}";
}

std::string payloadWith(const std::string& encounter) {
    return R"({"bag":[],"chapter":3,"cultivation":0,"day":1)" + encounter +
           R"(,"facing":2,"flags":{},"hp":60,"knownWeaknesses":{},"mapId":"ch03_guwai","maxHp":60,"maxMp":30,)"
           R"("mp":30,"playSecondsGameplay":0.0,"playSecondsSystem":0.0,"position":{"x":3,"y":4},"realm":3,"realmCap":3})";
}

fanren::core::Result<GameState> loadText(const std::string& text) {
    fanren::test::TempDir dir("fanren_encounter_save");
    const fs::path path = dir.path() / "save.json";
    {
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        out << text;
    }
    return fanren::io::loadGame(path.string());
}

TEST(EncounterSave, TheSaveVersionIsEight) {
    // 设计原文（docs/interfaces-octo-encounters.md 第 7 节、io/SaveFile.h 第 8 条）：计数器进存档，升到 8。
    EXPECT_EQ(fanren::io::kSaveVersion, 8);
}

TEST(EncounterSave, TheCounterRoundTripsThroughASave) {
    GameState state;
    state.encounter.stepsSinceLast = 5;
    state.encounter.stepsUntilNext = 23;
    state.encounter.triggeredToday = 2;
    state.encounter.lastDay = 7;
    fanren::test::TempDir dir("fanren_encounter_roundtrip");
    const std::string path = (dir.path() / "save.json").string();
    ASSERT_TRUE(fanren::io::saveGame(state, path).ok);
    const auto loaded = fanren::io::loadGame(path);
    ASSERT_TRUE(loaded.ok) << loaded.error;
    EXPECT_EQ(loaded.value.encounter.stepsSinceLast, 5);
    EXPECT_EQ(loaded.value.encounter.stepsUntilNext, 23);
    EXPECT_EQ(loaded.value.encounter.triggeredToday, 2) << "不存的话，读一次档就把今天的上限清零";
    EXPECT_EQ(loaded.value.encounter.lastDay, 7);
}

TEST(EncounterSave, AVersionSevenSaveLoadsWithACleanCounter) {
    // 旧档 = 从没遇过。7→8 迁移若忘了登记，这里读都读不回来。
    const auto loaded = loadText(handSave(7, payloadWith("")));
    ASSERT_TRUE(loaded.ok) << loaded.error;
    EXPECT_EQ(loaded.value.mapId, "ch03_guwai") << "先验：这份 v7 档确实读进来了";
    EXPECT_EQ(loaded.value.encounter.stepsSinceLast, 0);
    EXPECT_EQ(loaded.value.encounter.stepsUntilNext, 0);
    EXPECT_EQ(loaded.value.encounter.triggeredToday, 0);
    EXPECT_EQ(loaded.value.encounter.lastDay, 0);
}

TEST(EncounterSave, AVersionEightCounterIsReadBackAndABrokenOneIsRefused) {
    const auto good = loadText(handSave(
        8, payloadWith(R"(,"encounter":{"lastDay":1,"stepsSinceLast":4,"stepsUntilNext":9,"triggeredToday":1})")));
    ASSERT_TRUE(good.ok) << good.error;
    EXPECT_EQ(good.value.encounter.stepsUntilNext, 9);

    // 负向：写了却写坏了（步数写成字符串）→ 拒读，不悄悄按 0 收——那会把一份坏档的症状推迟成「今天怎么又遇上了」。
    const auto broken = loadText(handSave(
        8, payloadWith(R"(,"encounter":{"lastDay":1,"stepsSinceLast":"4","stepsUntilNext":9,"triggeredToday":1})")));
    EXPECT_FALSE(broken.ok);
    EXPECT_NE(broken.error.find("encounter"), std::string::npos) << broken.error;
}

// ---- 脚本 bgm ----

TEST_F(EncounterFlow, ScriptBgmSurvivesATeleportAndMapRestoresTheMapTrack) {
    enter("ch05_nancheng");
    EXPECT_TRUE(app_.bgmOverride().empty());
    EXPECT_EQ(app_.worldBgm(), app_.currentMap()->bgm) << "没点播时放地图曲";

    // 第 5 章节点 6a：三更翻墙，夜探的曲子点上；teleport 进墨府后园也不撤。
    runScript("ch05/yeru.lua");
    ASSERT_EQ(state().mapId, "ch05_mofu") << "先验：翻墙进了墨府后园";
    EXPECT_EQ(app_.bgmOverride(), "bgm_night");
    EXPECT_EQ(app_.worldBgm(), "bgm_night") << "换图时点播的那一首仍然作数";

    // 节点 6b：听完 bgm("map")，撤回墨府的地图曲。
    runScript("ch05/toutin.lua");
    EXPECT_TRUE(app_.bgmOverride().empty());
    EXPECT_EQ(app_.worldBgm(), app_.currentMap()->bgm);
}

}  // namespace
