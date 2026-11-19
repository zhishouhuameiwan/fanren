// 伙伴改版 · 截图口（docs/interfaces-companions.md 第 7 节第 5 条、第 6 节「四路交回之后」截图口那一条）。
//
// 两件事：
//   1. 截图口新加的入口（game/ShotScenes.h，main.cpp 的 --scene 调的就是它）在软件渲染器上真拍得出来：
//      历练组队面板、主菜单的队伍页与名册页、带两位跟随者和四种新对象的世界层（走几步再拍）、四人战斗面板。
//      真树里没有伙伴数据，用 D 路夹具那份临时资产根（tests/CompanionsLaneDFixture.h）。
//   2. 世界层被对白压着时，截图口的时钟按定数走：同一屏隔多久拍都一样；换一个定数画面就变（阳性对照）。
#include <SDL3/SDL.h>
#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "CompanionsLaneDFixture.h"
#include "TempDir.h"
#include "game/Application.h"
#include "game/BattleScene.h"
#include "game/ShotScenes.h"
#include "game/WorldScene.h"

namespace {

namespace fs = std::filesystem;
namespace laned = fanren::test::laned;
using fanren::game::Application;

// 一张截图：RGB 逐像素。
struct Shot {
    int w = 0;
    int h = 0;
    std::vector<std::uint8_t> rgb;
};

std::size_t differing(const Shot& a, const Shot& b) {
    if (a.w != b.w || a.h != b.h) return static_cast<std::size_t>(-1);
    std::size_t n = 0;
    for (std::size_t i = 0; i < a.rgb.size(); i += 3) {
        n += (a.rgb[i] != b.rgb[i] || a.rgb[i + 1] != b.rgb[i + 1] || a.rgb[i + 2] != b.rgb[i + 2]) ? 1u : 0u;
    }
    return n;
}

std::size_t distinctColours(const Shot& shot) {
    std::set<std::uint32_t> colours;
    for (std::size_t i = 0; i < shot.rgb.size(); i += 3) {
        colours.insert((std::uint32_t{shot.rgb[i]} << 16) | (std::uint32_t{shot.rgb[i + 1]} << 8) | shot.rgb[i + 2]);
    }
    return colours.size();
}

// 与 PortraitTests 同一个做法：dummy 视频驱动 + 软件渲染器，像截图口那样 beginFrame → drawScenes → captureFrame。
class CompanionShots : public ::testing::Test {
protected:
    static void SetUpTestSuite() { root_ = laned::makeRoot("fanren_companion_shots"); }
    static void TearDownTestSuite() { root_.reset(); }

    void SetUp() override {
        SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
        SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
        SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "dummy");
        dir_ = fanren::test::uniqueTempPath("fanren_companion_shot_png");
        fs::create_directories(dir_);
    }
    void TearDown() override {
        SDL_ResetHint(SDL_HINT_VIDEO_DRIVER);
        SDL_ResetHint(SDL_HINT_RENDER_DRIVER);
        SDL_ResetHint(SDL_HINT_AUDIO_DRIVER);
        std::error_code ignored;
        fs::remove_all(dir_, ignored);
    }

    Shot capture(Application& app) {
        Shot shot;
        const fs::path png = dir_ / ("shot" + std::to_string(count_++) + ".png");
        app.engine().beginFrame();
        app.drawScenes();
        const auto saved = app.engine().captureFrame(png.string());
        app.engine().endFrame();
        EXPECT_TRUE(saved.ok) << saved.error;
        if (!saved.ok) return shot;
        SDL_Surface* loaded = SDL_LoadPNG(png.string().c_str());
        EXPECT_NE(loaded, nullptr) << SDL_GetError();
        if (loaded == nullptr) return shot;
        SDL_Surface* rgba = SDL_ConvertSurface(loaded, SDL_PIXELFORMAT_RGBA32);
        SDL_DestroySurface(loaded);
        if (rgba == nullptr) return shot;
        shot.w = rgba->w;
        shot.h = rgba->h;
        for (int y = 0; y < shot.h; ++y) {
            const auto* row = static_cast<const std::uint8_t*>(rgba->pixels) + y * rgba->pitch;
            for (int x = 0; x < shot.w; ++x) shot.rgb.insert(shot.rgb.end(), row + x * 4, row + x * 4 + 3);
        }
        SDL_DestroySurface(rgba);
        return shot;
    }

    // 像截图口那样起一台：站在 mapId 上、世界层在底，时钟按截图口的定数走。
    bool start(Application& app, const std::string& root, const std::string& mapId) {
        const auto ready = app.init(root, /*headless=*/false);
        EXPECT_TRUE(ready.ok) << ready.error;
        if (!ready.ok) return false;
        app.setFixedWorldClock(fanren::game::kShotWorldClockSeconds);
        app.state().day = 40;
        EXPECT_TRUE(app.loadMap(mapId, std::string{}).ok);
        app.pushScene(std::make_unique<fanren::game::WorldScene>());
        return true;
    }
    static void tick(Application& app, int frames) {
        for (int i = 0; i < frames; ++i) app.tick(1.0 / 60.0);
    }
    static std::string top(Application& app) {
        return app.topScene() == nullptr ? std::string{} : app.topScene()->name();
    }
    // 截图口的入口：认得就压场景、按它给的帧数空转。
    bool shoot(Application& app, const std::string& kind, const std::string& arg) {
        int warmup = 3;
        if (!fanren::game::pushCompanionShot(app, kind, arg, warmup)) return false;
        tick(app, warmup);
        return true;
    }

    static inline std::unique_ptr<fanren::test::TempDir> root_;
    fs::path dir_;
    int count_ = 0;
};

// 一张图：四种新对象各一个摆在主角四周，(12,11) 站位、(11,11) 历练入口（夹具那一套）。
void writeObjectsMap(const fs::path& root) {
    std::vector<std::string> objects = laned::standAndFacility("zhanwei");
    objects.push_back(laned::object(20, "chest_a", "chest", 6, 12,
                                    {laned::prop("id", "probe_objects.chest_a"), laned::prop("loot", "pill_huiqi_dan")}));
    objects.push_back(laned::object(21, "gather_a", "gather", 7, 12,
                                    {laned::prop("id", "probe_objects.gather_a"), laned::prop("item", "pill_huiqi_dan"),
                                     laned::prop("respawn_days", 3)}));
    objects.push_back(laned::object(22, "foe_a", "foe", 16, 9,
                                    {laned::prop("id", "probe_objects.foe_a"), laned::prop("battle", "probe_foe"),
                                     laned::prop("look", "zhang_tie"), laned::prop("facing", "down")}));
    objects.push_back(laned::object(23, "gate_a", "gate", 5, 13,
                                    {laned::prop("id", "probe_objects.gate_a"), laned::prop("skill", "break_seal")}));
    laned::writeMap(root, "probe_objects", objects);
}

}  // namespace

TEST_F(CompanionShots, TheLilianPartyPanelEntryShoots) {
    Application app;
    ASSERT_TRUE(start(app, root_->path().string(), "probe_home"));
    app.state().setFlag("probe.lilian.open");
    app.state().setFlag("probe.a.met");
    app.state().setFlag("probe.b.met");
    ASSERT_TRUE(shoot(app, "lilian", laned::kSite));
    EXPECT_EQ(top(app), "LilianParty");
    const Shot shot = capture(app);
    ASSERT_GT(shot.w, 0);
    EXPECT_GT(distinctColours(shot), 50u) << "拍出来的不是一片空";
    app.shutdown();
}

TEST_F(CompanionShots, ThePartyAndRosterPageEntriesShoot) {
    for (const char* page : {"party", "roster"}) {
        Application app;
        ASSERT_TRUE(start(app, root_->path().string(), "probe_home"));
        app.state().setFlag("probe.a.met");
        app.state().partyAdd("zhang_tie");
        ASSERT_TRUE(shoot(app, "menu", page)) << page;
        EXPECT_EQ(top(app), "Menu") << page;
        const Shot shot = capture(app);
        ASSERT_GT(shot.w, 0) << page;
        EXPECT_GT(distinctColours(shot), 50u) << page;
        app.shutdown();
    }
}

TEST_F(CompanionShots, TheWalkEntryLaysOutTwoFollowersAmongTheFourNewObjects) {
    writeObjectsMap(root_->path());
    Application app;
    ASSERT_TRUE(start(app, root_->path().string(), "probe_objects"));
    app.state().partyAdd("zhang_tie");      // probe_a
    app.state().partyAdd("baiyi_shaonv");   // probe_b
    ASSERT_TRUE(shoot(app, "walk", "DDRR"));
    auto* world = dynamic_cast<fanren::game::WorldScene*>(app.topScene());
    ASSERT_NE(world, nullptr);
    EXPECT_EQ(app.state().position.x, 14);
    EXPECT_EQ(app.state().position.y, 13);
    const auto& frame = world->describeFrame(app);
    ASSERT_EQ(frame.followers.size(), 2u);
    // 走过的格子：(12,12) (12,13) (13,13) (14,13)。第 0 位紧跟主角，第 1 位再后一格。
    EXPECT_EQ(frame.followers[0].cell.x, 13);
    EXPECT_EQ(frame.followers[0].cell.y, 13);
    EXPECT_EQ(frame.followers[1].cell.x, 12);
    EXPECT_EQ(frame.followers[1].cell.y, 13);
    EXPECT_EQ(frame.chests.size(), 1u);
    EXPECT_EQ(frame.gathers.size(), 1u);
    EXPECT_EQ(frame.foes.size(), 1u);
    EXPECT_EQ(frame.gates.size(), 1u);
    // 画面上（WorldView 真在画的那一份）两位跟随者已经滑到那两格、停下了：拍的不是半路上的样子。
    const auto& slots = world->view().followers();
    ASSERT_EQ(slots.size(), 2u);
    EXPECT_FLOAT_EQ(slots[0].slide.position().x, 13.f);
    EXPECT_FLOAT_EQ(slots[1].slide.position().x, 12.f);
    EXPECT_FALSE(slots[0].slide.moving());
    EXPECT_FALSE(slots[1].slide.moving());
    EXPECT_FALSE(world->view().playerMoving());
    const Shot shot = capture(app);
    ASSERT_GT(shot.w, 0);
    EXPECT_GT(distinctColours(shot), 50u);
    app.shutdown();
}

// 认得却拍不成的，一律报失败，不悄悄拍一张看着正常的图。
TEST_F(CompanionShots, EntriesThatCannotBeShotAsAskedFailInsteadOfShootingSomethingElse) {
    Application app;
    ASSERT_TRUE(start(app, root_->path().string(), "probe_home"));
    tick(app, 1);
    EXPECT_FALSE(shoot(app, "lilian", "no_such_site")) << "历练地不存在";
    EXPECT_FALSE(shoot(app, "menu", "party")) << "名册上没人：没有队伍页";
    EXPECT_FALSE(shoot(app, "menu", "roster")) << "名册上没人：没有名册页";
    EXPECT_FALSE(shoot(app, "walk", "DX")) << "认不出的方向";
    tick(app, 1);
    EXPECT_EQ(top(app), "World") << "失败的入口什么都没压";
    // 阳性对照：同一台，名册上有了人，队伍页就拍得成。
    app.state().setFlag("probe.a.met");
    EXPECT_TRUE(shoot(app, "menu", "party"));
    app.shutdown();
}

TEST_F(CompanionShots, AWalkThatRunsIntoAFoesSightStopsAndFails) {
    writeObjectsMap(root_->path());
    Application app;
    ASSERT_TRUE(start(app, root_->path().string(), "probe_objects"));
    // 明雷在 (16,9) 面朝下，看着 (16,10)。从 (12,11) 往右四步到 (16,11)，再往上一步踏进它面前那一格，开打；
    // 后面那一步不该再对着压住的世界层走。
    EXPECT_FALSE(shoot(app, "walk", "RRRRUR"));
    EXPECT_EQ(top(app), "Battle");
    EXPECT_EQ(app.state().position.x, 16) << "停在开打那一格，没有再往下走";
    EXPECT_EQ(app.state().position.y, 10);
    app.shutdown();
}

TEST_F(CompanionShots, AFourPersonBattlePanelShootsThroughTheBattleEntry) {
    Application app;
    ASSERT_TRUE(start(app, root_->path().string(), "probe_home"));
    for (const char* role : {"zhang_tie", "baiyi_shaonv", "chen_shimei"}) app.state().partyAdd(role);
    app.pushScene(std::make_unique<fanren::game::BattleScene>("probe_foe"));
    tick(app, 3);
    auto* battle = dynamic_cast<fanren::game::BattleScene*>(app.topScene());
    ASSERT_NE(battle, nullptr);
    int allies = 0;
    for (const auto& unit : battle->battle().units()) allies += unit.ally ? 1 : 0;
    EXPECT_EQ(allies, 4) << "韩立 + 三位伙伴";
    const Shot shot = capture(app);
    ASSERT_GT(shot.w, 0);
    EXPECT_GT(distinctColours(shot), 50u);
    app.shutdown();
}

TEST_F(CompanionShots, UnderADialogueTheWorldClockIsFixedSoTheShotDoesNotDependOnWhenItIsTaken) {
    const auto talkShot = [this](double clock, int sleepMs) {
        Application app;
        const auto ready = app.init(laned::repoRoot().string(), /*headless=*/false);
        EXPECT_TRUE(ready.ok) << ready.error;
        app.setFixedWorldClock(clock);
        EXPECT_TRUE(app.loadMap("ch01_hanjiacun", std::string{}).ok);
        app.pushScene(std::make_unique<fanren::game::WorldScene>());
        app.sayAs("hanli", "ch01.zhenshang.arrive");
        tick(app, 240);   // 截图口 talk 的 kTalkShotFrames
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
        Shot shot = capture(app);
        app.shutdown();
        return shot;
    };
    // 暖机：同一进程里第一台软件渲染的 Application 拍的对白框差 1–2 级（A 路记过，与本条无关），先扔一张。
    (void)talkShot(fanren::game::kShotWorldClockSeconds, 0);
    const Shot soon = talkShot(fanren::game::kShotWorldClockSeconds, 0);
    const Shot late = talkShot(fanren::game::kShotWorldClockSeconds, 250);
    ASSERT_GT(soon.w, 0);
    EXPECT_EQ(differing(soon, late), 0u) << "时钟按定数走：隔多久拍都一样";
    // 阳性对照：时钟的步长真的进了画面（灯的明灭、主角的呼吸），换一个定数就差得出来。
    const Shot other = talkShot(1.7, 0);
    EXPECT_GT(differing(soon, other), 0u);
}
