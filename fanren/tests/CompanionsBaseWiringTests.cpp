// 伙伴改版·底座（docs/interfaces-companions.md 第 6 节「第一步」）：走 Application 真入口的那一半。
//   1. data/companions/ 在 init 时读进 GameData；
//   2. 脚本 bond.add / bond.get / bond.tier；talk 的表情参数送到对话框；
//   3. 开战参数（先手）；历练的进出；历练里三个存档入口（存档点、F5、主菜单「存盘」）都存不下。
// 资产根是一份临时拷贝（data/、scripts/、maps/，与 QuickSaveTests 同一个做法）：要往 data/companions/
// 放一位探针伙伴、往 scripts/ 放测试脚本、往 saves/ 写档，都不碰仓库。
#include <SDL3/SDL.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "TempDir.h"
#include "core/model/Types.h"
#include "game/Application.h"
#include "game/BattleScene.h"
#include "game/DialogueScene.h"
#include "game/MenuScene.h"
#include "game/WorldScene.h"
#include "game/WorldView.h"

namespace {

namespace fs = std::filesystem;
using fanren::game::Application;
using fanren::game::DialogueScene;

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

void pushKey(SDL_Scancode scancode, bool down) {
    SDL_Event event{};
    event.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
    event.key.scancode = scancode;
    event.key.down = down;
    SDL_PushEvent(&event);
}

// 探针伙伴：两个角色 id 都是 data/roles 里现成的（厉飞雨、张铁），只为了有一份能入队的角色表。
constexpr const char* kProbeCompanion = R"({
  "id": "probe_c", "met_flag": "probe.met",
  "roles": [{"role": "zhang_tie", "until_flag": "probe.grown"}, {"role": "li_feiyu"}]
})";

class CompanionsWiring : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        root_ = std::make_unique<fanren::test::TempDir>("fanren_companions_wiring");
        for (const char* dir : {"data", "scripts", "maps"}) {
            fs::copy(repoRoot() / dir, root_->path() / dir, fs::copy_options::recursive);
        }
        writeFile(root_->path() / "data" / "companions" / "probe_c.json", kProbeCompanion);
    }
    static void TearDownTestSuite() { root_.reset(); }

    void SetUp() override {
        auto ready = app_.init(root_->path().string(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
        std::error_code ec;
        fs::remove(app_.quickSavePath(), ec);
    }
    void TearDown() override {
        app_.shutdown();
        std::error_code ec;
        fs::remove(app_.quickSavePath(), ec);
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
        const fanren::game::Scene* scene = app_.topScene();
        return scene == nullptr ? std::string{} : scene->name();
    }
    // 起一段脚本（写进临时根的 scripts/t/），推到它第一次挂起或跑完。
    void start(const std::string& source) {
        writeFile(root_->path() / "scripts" / "t" / "probe.lua", source);
        const auto started = app_.startEvent("t/probe.lua");
        ASSERT_TRUE(started.ok) << started.error;
        tick(2);
        ASSERT_TRUE(app_.scripts().lastError().empty()) << app_.scripts().lastError();
    }
    // 跑一段不会停下来等玩家的脚本，直到它跑完。
    void run(const std::string& source) {
        start(source);
        for (int frame = 0; frame < 64 && app_.scripts().isRunning(); ++frame) tick();
        ASSERT_FALSE(app_.scripts().isRunning());
        ASSERT_TRUE(app_.scripts().lastError().empty()) << app_.scripts().lastError();
    }
    DialogueScene* dialogue() { return dynamic_cast<DialogueScene*>(app_.topScene()); }
    bool quickSaveOnDisk() const { return fs::exists(app_.quickSavePath()); }

    static std::unique_ptr<fanren::test::TempDir> root_;
    Application app_;
};

std::unique_ptr<fanren::test::TempDir> CompanionsWiring::root_;

// ---------------------------------------------------------------------------
// 1. 伙伴数据经 init 读进来
// ---------------------------------------------------------------------------

TEST_F(CompanionsWiring, InitReadsDataCompanions) {
    const auto& companions = app_.data().companions;
    ASSERT_EQ(companions.count("probe_c"), 1u);
    EXPECT_EQ(companions.at("probe_c").roles.size(), 2u);
    EXPECT_EQ(companions.at("probe_c").metFlag, "probe.met");
}

TEST(CompanionsRepo, TheRealTreeLoadsEveryCompanionFileItHas) {
    // 底座不带伙伴数据（样板的数据由内容路加），但判据不写死「没有」：有几份文件就该读进几位。
    std::size_t files = 0;
    std::error_code ec;
    for (auto it = fs::recursive_directory_iterator(repoRoot() / "data" / "companions", ec);
         !ec && it != fs::recursive_directory_iterator(); it.increment(ec)) {
        if (it->path().extension() == ".json") ++files;
    }
    Application app;
    auto ready = app.init(repoRoot().string(), /*headless=*/true);
    ASSERT_TRUE(ready.ok) << ready.error;
    EXPECT_EQ(app.data().companions.size(), files);
    app.shutdown();
}

// ---------------------------------------------------------------------------
// 2. 脚本：bond.*、talk 的表情
// ---------------------------------------------------------------------------

TEST_F(CompanionsWiring, BondAddGetAndTierGoThroughTheScriptHost) {
    run(R"(
        assert(bond.get("probe_c") == 0 and bond.tier("probe_c") == 1)
        local ok, code = bond.add("probe_c", 25)
        assert(ok and code == "", code)
        assert(bond.get("probe_c") == 25, bond.get("probe_c"))
        assert(bond.tier("probe_c") == 2)
        ok, code = bond.add("nobody", 5)
        assert(not ok and code == "no_companion", code)
        ok, code = bond.add("probe_c", -3)
        assert(not ok and code == "negative", code)
        assert(bond.add("probe_c", 500))
        assert(bond.get("probe_c") == 100 and bond.tier("probe_c") == 5)
    )");
    EXPECT_EQ(app_.state().bondOf("probe_c"), 100);
    EXPECT_EQ(app_.state().bonds.count("nobody"), 0u) << "认不出的伙伴不记账";
}

TEST_F(CompanionsWiring, TalkCarriesTheFaceToTheDialogueScene) {
    start(R"(talk("hanli", "ui.encounter.lost", "smile"))");
    ASSERT_EQ(top(), "Dialogue");
    ASSERT_NE(dialogue(), nullptr);
    EXPECT_EQ(dialogue()->speakerRole(), "hanli");
    EXPECT_EQ(dialogue()->face(), "smile");
    ASSERT_FALSE(app_.dialogueLog().empty());
    EXPECT_EQ(app_.dialogueLog().back().roleId, "hanli");
    EXPECT_EQ(app_.dialogueLog().back().face, "smile");
}

TEST_F(CompanionsWiring, TalkWithoutAFaceIsNormalAndNarrationHasNoSpeaker) {
    start(R"(talk("hanli", "ui.encounter.lost"))");
    ASSERT_NE(dialogue(), nullptr);
    EXPECT_EQ(dialogue()->speakerRole(), "hanli");
    EXPECT_EQ(dialogue()->face(), "normal") << "契约 3.9：缺省 normal";
    press(SDL_SCANCODE_RETURN);
    app_.scripts().abortEvent("test");
    tick();

    start(R"(talk("", "ui.encounter.lost"))");
    ASSERT_NE(dialogue(), nullptr);
    EXPECT_EQ(dialogue()->speakerRole(), "");
    EXPECT_EQ(dialogue()->face(), "normal");
}

TEST_F(CompanionsWiring, AChoiceKeepsThePortraitOfTheLineItFollows) {
    start(R"(
        talk("hanli", "ui.encounter.lost", "angry")
        choice{ "ui.menu.save.ok" }
    )");
    ASSERT_NE(dialogue(), nullptr);
    press(SDL_SCANCODE_RETURN);
    tick(2);
    ASSERT_NE(dialogue(), nullptr) << top();
    EXPECT_EQ(dialogue()->speakerRole(), "hanli");
    EXPECT_EQ(dialogue()->face(), "angry");
}

// ---------------------------------------------------------------------------
// 3. 开战参数
// ---------------------------------------------------------------------------

TEST_F(CompanionsWiring, ThePreemptiveFlagTravelsWithTheBattleItStarts) {
    ASSERT_NE(app_.battleSetup("b04_gongfang_weijian"), nullptr);
    app_.startBattle(fanren::game::BattleStart{"b04_gongfang_weijian", true}, [](const auto&) {});
    tick();
    ASSERT_EQ(top(), "Battle");
    EXPECT_EQ(app_.battleStart().battleId, "b04_gongfang_weijian");
    EXPECT_TRUE(app_.battleStart().preemptive);
}

TEST_F(CompanionsWiring, BattlesStartedTheOldWaysAreNotPreemptive) {
    app_.startBattle(fanren::game::BattleStart{"b04_gongfang_weijian", true}, [](const auto&) {});
    tick();
    app_.startBattle("b04_gongfang_weijian", [](const auto&) {});
    tick();
    EXPECT_FALSE(app_.battleStart().preemptive) << "遭遇战、切磋那条老入口";

    Application other;
    ASSERT_TRUE(other.init(root_->path().string(), true).ok);
    other.startBattle(fanren::game::BattleStart{"b04_gongfang_weijian", true}, [](const auto&) {});
    writeFile(root_->path() / "scripts" / "t" / "fight.lua", R"(battle("b04_gongfang_weijian"))");
    ASSERT_TRUE(other.startEvent("t/fight.lua").ok);
    for (int i = 0; i < 3; ++i) other.tick(1.0 / 60.0);
    EXPECT_EQ(other.battleStart().battleId, "b04_gongfang_weijian");
    EXPECT_FALSE(other.battleStart().preemptive) << "脚本 battle() 开的仗";
    other.shutdown();
}

// ---------------------------------------------------------------------------
// 4. 历练的进出
// ---------------------------------------------------------------------------

fanren::game::LilianEntry probeTrip() {
    return fanren::game::LilianEntry{"probe_site", "ch01_hanjiacun", "", {"li_feiyu"}};
}

TEST_F(CompanionsWiring, EnteringAndLeavingALilianRestoresPartyMapAndPosition) {
    ASSERT_TRUE(app_.loadMap("ch01_qingniuzhen", std::string{}).ok);
    app_.state().position = fanren::core::Point{5, 6};
    app_.state().facing = 3;
    app_.state().day = 40;
    app_.state().party = {{"qu_hun", 17, true}};
    app_.state().worldObjects.openedChests = {"keep.me"};

    ASSERT_TRUE(app_.enterLilian(probeTrip()).ok);
    EXPECT_TRUE(app_.inLilian());
    EXPECT_EQ(app_.state().mapId, "ch01_hanjiacun");
    ASSERT_EQ(app_.state().party.size(), 1u);
    EXPECT_EQ(app_.state().party[0].roleId, "li_feiyu");
    EXPECT_EQ(app_.state().party[0].hp, -1);

    // 这一趟里开过的东西记在这一趟上，不进永久集合。
    app_.worldObjects().openedGates.insert("lilian_x.gate");
    EXPECT_EQ(app_.state().worldObjects.openedGates.count("lilian_x.gate"), 0u);

    ASSERT_TRUE(app_.leaveLilian().ok);
    EXPECT_FALSE(app_.inLilian());
    EXPECT_EQ(app_.state().mapId, "ch01_qingniuzhen");
    EXPECT_EQ(app_.state().position.x, 5);
    EXPECT_EQ(app_.state().position.y, 6);
    EXPECT_EQ(app_.state().facing, 3);
    EXPECT_EQ(app_.state().day, 41) << "契约 3.12：出来日历加一天";
    ASSERT_EQ(app_.state().party.size(), 1u);
    EXPECT_EQ(app_.state().party[0].roleId, "qu_hun");
    // 回来的是进去前那一位、带着进去前的伤：日历走的是 advanceDays 那一个入口，过一天按静养回一截，
    // 不是历练里那份满血的 -1。
    // 曲魂上限 55，一天回一成（kRestPercentPerDay 的设计口径，RestTests 钉着）：17 + 5 = 22。
    ASSERT_NE(app_.data().findRole("qu_hun"), nullptr);
    ASSERT_EQ(app_.data().findRole("qu_hun")->maxHp, 55);
    EXPECT_EQ(app_.state().party[0].hp, 22);
    EXPECT_EQ(&app_.worldObjects(), &app_.state().worldObjects);
    EXPECT_EQ(app_.worldObjects().openedChests.count("keep.me"), 1u);
    EXPECT_EQ(app_.worldObjects().openedGates.count("lilian_x.gate"), 0u) << "那一趟的状态出来就扔";

    ASSERT_TRUE(app_.enterLilian(probeTrip()).ok);
    EXPECT_TRUE(app_.worldObjects().empty()) << "第二趟从头来";
}

TEST_F(CompanionsWiring, AnEntranceThatCannotLoadLeavesEverythingAsItWas) {
    ASSERT_TRUE(app_.loadMap("ch01_qingniuzhen", std::string{}).ok);
    app_.state().position = fanren::core::Point{5, 6};
    app_.state().facing = 3;
    app_.state().party = {{"qu_hun", 17, true}};
    auto entry = probeTrip();
    entry.mapId = "lilian_no_such_map";
    EXPECT_FALSE(app_.enterLilian(entry).ok);
    EXPECT_FALSE(app_.inLilian());
    EXPECT_EQ(app_.state().mapId, "ch01_qingniuzhen");
    EXPECT_EQ(app_.state().position.x, 5);
    EXPECT_EQ(app_.state().position.y, 6);
    EXPECT_EQ(app_.state().facing, 3);
    ASSERT_EQ(app_.state().party.size(), 1u);
    EXPECT_EQ(app_.state().party[0].roleId, "qu_hun");
    EXPECT_TRUE(app_.quickSave().ok) << "没进去就照常能存";
}

TEST_F(CompanionsWiring, EnteringTwiceOrLeavingWhenOutsideIsRefused) {
    ASSERT_TRUE(app_.loadMap("ch01_qingniuzhen", std::string{}).ok);
    EXPECT_FALSE(app_.leaveLilian().ok);
    ASSERT_TRUE(app_.enterLilian(probeTrip()).ok);
    const int day = app_.state().day;
    EXPECT_FALSE(app_.enterLilian(probeTrip()).ok);
    EXPECT_TRUE(app_.inLilian());
    ASSERT_TRUE(app_.leaveLilian().ok);
    EXPECT_FALSE(app_.leaveLilian().ok);
    EXPECT_EQ(app_.state().day, day + 1) << "出来只加一天";
}

TEST_F(CompanionsWiring, TheSaveFacilityRefusesInsideALilianOnly) {
    fanren::core::MapObject stone;
    stone.name = "probe_save";
    stone.type = "facility";
    stone.properties["kind"] = "save";
    app_.openFacility(stone);
    tick();
    ASSERT_TRUE(quickSaveOnDisk()) << "对照：历练外存档点存得下";
    fs::remove(app_.quickSavePath());

    ASSERT_TRUE(app_.enterLilian(probeTrip()).ok);
    app_.openFacility(stone);
    tick();
    EXPECT_FALSE(quickSaveOnDisk());
    EXPECT_FALSE(app_.quickSave().ok);
}

TEST_F(CompanionsWiring, F5RefusesInsideALilianOnly) {
    ASSERT_TRUE(app_.loadMap(fanren::game::kNewGameMap, std::string{}).ok);
    app_.pushScene(std::make_unique<fanren::game::WorldScene>());
    tick();
    ASSERT_EQ(top(), "World");
    press(SDL_SCANCODE_F5);
    ASSERT_TRUE(quickSaveOnDisk()) << "对照：历练外 F5 存得下";
    fs::remove(app_.quickSavePath());

    ASSERT_TRUE(app_.enterLilian(probeTrip()).ok);
    tick();
    ASSERT_EQ(top(), "World");
    press(SDL_SCANCODE_F5);
    EXPECT_FALSE(quickSaveOnDisk());
}

TEST_F(CompanionsWiring, TheMainMenuSaveRefusesInsideALilianOnly) {
    ASSERT_TRUE(app_.loadMap(fanren::game::kNewGameMap, std::string{}).ok);
    app_.pushScene(std::make_unique<fanren::game::WorldScene>());
    app_.pushScene(std::make_unique<fanren::game::MenuScene>(fanren::game::MenuScene::Page::Save));
    tick(30);
    ASSERT_EQ(top(), "Menu");
    press(SDL_SCANCODE_RETURN);
    ASSERT_TRUE(quickSaveOnDisk()) << "对照：历练外主菜单存得下";
    fs::remove(app_.quickSavePath());

    ASSERT_TRUE(app_.enterLilian(probeTrip()).ok);
    tick();
    ASSERT_EQ(top(), "Menu");
    press(SDL_SCANCODE_RETURN);
    EXPECT_FALSE(quickSaveOnDisk());
}

TEST_F(CompanionsWiring, ContinuingAJourneyDropsAnyLilianInProgress) {
    ASSERT_TRUE(app_.enterLilian(probeTrip()).ok);
    ASSERT_TRUE(app_.continueJourney((repoRoot() / "tests/fixtures/ch13-end-first.sav").string()).ok);
    EXPECT_FALSE(app_.inLilian()) << "档里没有历练状态：读档就是回到进去之前";
    EXPECT_EQ(&app_.worldObjects(), &app_.state().worldObjects);
}

// ---------------------------------------------------------------------------
// 5. WorldFrame：底座只定结构（往里填归世界逻辑路，照着画归世界画面路）
// ---------------------------------------------------------------------------

TEST(CompanionsFrame, ClearEmptiesTheFollowersAndTheFourNewObjectLists) {
    fanren::game::WorldFrame frame;
    frame.followers.push_back({"li_feiyu", fanren::core::Point{3, 4}, 1});
    frame.chests.push_back({nullptr, true});
    frame.gathers.push_back({nullptr, true});
    frame.foes.push_back({nullptr, "wild_wolf", 3, true});
    frame.gates.push_back({nullptr, true});
    frame.clear();
    EXPECT_TRUE(frame.followers.empty());
    EXPECT_TRUE(frame.chests.empty());
    EXPECT_TRUE(frame.gathers.empty());
    EXPECT_TRUE(frame.foes.empty());
    EXPECT_TRUE(frame.gates.empty());
}

}  // namespace
