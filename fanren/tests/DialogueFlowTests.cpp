// 对白减负的三样东西走真入口（第 7 章地基包路 D，docs/ch07-foundation.md 4.2–4.4）：
//   · 自动播放：一句显示完停 autoAdvanceDelay 就自己翻；**有选项时停住，绝不自动选**；
//   · 整段跳过：取消键 → 确认框 → 后面的 talk 直接记进回看，途中 give / set_flag 照常，跳到选择就停、跳到脚本结束也停，
//     跳完弹「已跳过 N 句」，世界层上按菜单键翻得开回看；
//   · 插话气泡：说话人在画面上时不压对话框、脚本不等人；不在画面上时退回成普通对白，话不丢。
// 脚本写进临时资产根的 scripts/test_d/ 下，按键走 SDL 事件（与 SettingsWiring 同一个拍法）。
#include <gtest/gtest.h>

#include <SDL3/SDL.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

#include "TempDir.h"
#include "game/Application.h"
#include "game/DialogueExtras.h"
#include "game/DialogueScene.h"
#include "game/WorldScene.h"
#include "io/SettingsFile.h"

namespace fs = std::filesystem;
using fanren::game::Application;
using fanren::game::DialogueScene;

namespace {

std::string repoRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "data" / "text" / "ch01_main.json")) return candidate;
    }
    return ".";
}

void pushKey(SDL_Scancode scancode, bool down) {
    SDL_Event event{};
    event.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
    event.key.scancode = scancode;
    event.key.down = down;
    event.key.repeat = false;
    SDL_PushEvent(&event);
}

// 文案用 ui_dialogue.json 里现成的几条：key 认得出、正文也认得出。
constexpr const char* kA = "ui.dialogue.skip.ask";
constexpr const char* kB = "ui.dialogue.skip.yes";
constexpr const char* kC = "ui.dialogue.skip.no";
constexpr const char* kD = "ui.dialogue.auto";
constexpr const char* kOpt1 = "ui.settings.auto_advance";
constexpr const char* kOpt2 = "ui.settings.desc.auto_advance";

class DialogueFlow : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        root_ = std::make_unique<fanren::test::TempDir>("fanren_dialogue_flow_root");
        for (const char* dir : {"data", "scripts", "maps"}) {
            fs::copy(fs::path(repoRoot()) / dir, root_->path() / dir, fs::copy_options::recursive);
        }
        fs::create_directories(root_->path() / "scripts" / "test_d");
        const auto write = [](const char* name, const std::string& body) {
            std::ofstream(root_->path() / "scripts" / "test_d" / name, std::ios::binary) << body;
        };
        // 两句、一个选择、选完再一句。
        write("choose.lua", std::string("talk(\"\", \"") + kA + "\")\ntalk(\"\", \"" + kB + "\")\n" +
                                "local pick = choice{\"" + kOpt1 + "\", \"" + kOpt2 + "\"}\n" +
                                "flag.set(\"test_d.pick\", pick or 0)\ntalk(\"\", \"" + kC + "\")\n");
        // 一句、再两句夹一次给东西与置旗标、一个选择、选完一句：跳过从第一句起，跳到选择停。
        write("skip_to_choice.lua", std::string("talk(\"\", \"") + kA + "\")\ntalk(\"\", \"" + kB + "\")\n" +
                                        "give(\"material_lingshi\", 3)\nflag.set(\"test_d.mid\", 1)\nwait(800)\n" +
                                        "talk(\"\", \"" + kC + "\")\n" +
                                        "local pick = choice{\"" + kOpt1 + "\", \"" + kOpt2 + "\"}\n" +
                                        "flag.set(\"test_d.pick\", pick or 0)\ntalk(\"\", \"" + kD + "\")\n");
        // 四句，没有选择：跳过一路跳到脚本结束。
        write("skip_to_end.lua", std::string("talk(\"\", \"") + kA + "\")\ntalk(\"\", \"" + kB + "\")\n" +
                                     "talk(\"\", \"" + kC + "\")\ntalk(\"\", \"" + kD + "\")\nflag.set(\"test_d.end\", 1)\n");
        // 气泡：韩母站在韩家村出生点旁边；第二只气泡的说话人不在这张图上。
        write("bubble_here.lua", std::string("bubble(\"han_mu\", \"") + kA + "\", 2)\nflag.set(\"test_d.bubbled\", 1)\n");
        write("bubble_away.lua", std::string("bubble(\"baiyi_shaonv\", \"") + kB + "\", 2)\nflag.set(\"test_d.away\", 1)\n");
    }
    static void TearDownTestSuite() { root_.reset(); }

    void SetUp() override {
        auto ready = app_.init(root_->path().string(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
        ASSERT_TRUE(app_.loadMap(fanren::game::kNewGameMap, std::string{}).ok);
        app_.pushScene(std::make_unique<fanren::game::WorldScene>());
        tick();
        ASSERT_EQ(top(), "World");
    }
    void TearDown() override { app_.shutdown(); }

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
    DialogueScene* dialogue() { return dynamic_cast<DialogueScene*>(app_.topScene()); }
    void start(const char* name) {
        const auto started = app_.startEvent(std::string("test_d/") + name);
        ASSERT_TRUE(started.ok) << started.error;
        tick(2);
    }
    void setAuto(bool on) {
        fanren::io::Settings next = app_.settings();
        next.autoAdvance = on;
        app_.setSettings(next);
    }
    bool logged(const char* key) {
        const std::string body = app_.text(key);
        for (const auto& line : app_.dialogueLog()) {
            if (line.body == body) return true;
        }
        return false;
    }
    int spoken(const char* key) {
        int n = 0;
        for (const std::string& k : app_.spokenKeys()) n += k == key ? 1 : 0;
        return n;
    }

    static std::unique_ptr<fanren::test::TempDir> root_;
    Application app_;
};

std::unique_ptr<fanren::test::TempDir> DialogueFlow::root_;

}  // namespace

// ---------------------------------------------------------------------------
// 自动播放（4.2）
// ---------------------------------------------------------------------------

TEST(DialogueAutoAdvance, DelayIsPointEightPlusSixHundredthsPerGraphemeCappedAtFour) {
    EXPECT_DOUBLE_EQ(fanren::game::autoAdvanceDelay(0), 0.8);
    EXPECT_NEAR(fanren::game::autoAdvanceDelay(10), 1.4, 1e-9);
    EXPECT_NEAR(fanren::game::autoAdvanceDelay(30), 2.6, 1e-9);
    EXPECT_DOUBLE_EQ(fanren::game::autoAdvanceDelay(54), 4.0) << "0.8 + 54 × 0.06 = 4.04，封顶 4 秒";
    EXPECT_DOUBLE_EQ(fanren::game::autoAdvanceDelay(500), 4.0);
}

TEST(DialogueAutoAdvance, OffByDefaultAndRoundTripsThroughTheSettingsFile) {
    EXPECT_FALSE(fanren::io::Settings{}.autoAdvance) << "缺省关 = 改造前的行为";
    const auto read = fanren::io::parseSettings(R"({"version": 1, "auto_advance": true})");
    EXPECT_TRUE(read.settings.autoAdvance);
    EXPECT_TRUE(read.warnings.empty());
    const auto missing = fanren::io::parseSettings(R"({"version": 1})");
    EXPECT_FALSE(missing.settings.autoAdvance) << "缺字段 → 关";
}

TEST_F(DialogueFlow, WithAutoAdvanceOnTalksTurnThemselvesButAChoiceIsNeverPickedForThePlayer) {
    setAuto(true);
    start("choose.lua");
    ASSERT_EQ(top(), "Dialogue");
    EXPECT_EQ(spoken(kA), 1);
    // 一句至多 4 秒：12 秒里两句都自己翻过去，停在选择上。
    tick(12 * 60);
    ASSERT_EQ(top(), "Dialogue") << "选择框该一直等着";
    EXPECT_EQ(spoken(kB), 1) << "第二句自己翻到了";
    EXPECT_TRUE(app_.awaitingCommand()) << "选择还在等玩家";
    EXPECT_EQ(app_.state().flag("test_d.pick"), 0) << "选择没有被自动选";
    // 再等 30 秒也一样。
    tick(30 * 60);
    EXPECT_TRUE(app_.awaitingCommand());
    EXPECT_EQ(app_.state().flag("test_d.pick"), 0) << "**选择永远不自动选**";
    EXPECT_EQ(spoken(kC), 0) << "选完之后那一句还没说";
    // 玩家自己按：选第二项。
    press(SDL_SCANCODE_DOWN);
    press(SDL_SCANCODE_RETURN);
    tick(2);
    EXPECT_EQ(app_.state().flag("test_d.pick"), 2);
}

TEST_F(DialogueFlow, WithAutoAdvanceOffATalkWaitsForTheConfirmKey) {
    setAuto(false);
    start("choose.lua");
    ASSERT_EQ(top(), "Dialogue");
    tick(10 * 60);
    EXPECT_EQ(spoken(kB), 0) << "没开自动播放：十秒过去还停在第一句";
}

TEST_F(DialogueFlow, TheActionKeyTogglesAutoAdvanceFromInsideADialogue) {
    setAuto(false);
    start("choose.lua");
    ASSERT_EQ(top(), "Dialogue");
    press(SDL_SCANCODE_E);   // 路径行动键的默认键位
    EXPECT_TRUE(app_.settings().autoAdvance) << "对话里按路径行动键打开自动播放";
    tick(5 * 60);
    EXPECT_EQ(spoken(kB), 1) << "打开之后就自己翻";
    press(SDL_SCANCODE_E);
    EXPECT_FALSE(app_.settings().autoAdvance) << "再按一下关掉";
}

// ---------------------------------------------------------------------------
// 整段跳过（4.3）
// ---------------------------------------------------------------------------

TEST_F(DialogueFlow, SkippingStopsAtTheChoiceKeepsTheSideEffectsAndLogsWhatWasSkipped) {
    start("skip_to_choice.lua");
    ASSERT_EQ(top(), "Dialogue");
    const int stonesBefore = app_.state().itemCount("material_lingshi");
    press(SDL_SCANCODE_ESCAPE);
    ASSERT_NE(dialogue(), nullptr);
    ASSERT_TRUE(dialogue()->askingSkip()) << "取消键先弹确认框，不直接跳";
    // 确认框里先按「接着看」：什么都不跳。
    press(SDL_SCANCODE_DOWN);
    press(SDL_SCANCODE_RETURN);
    ASSERT_NE(dialogue(), nullptr);
    EXPECT_FALSE(dialogue()->askingSkip());
    EXPECT_EQ(spoken(kB), 0) << "选了接着看，第二句还没说";
    // 再来一次，这回跳。
    press(SDL_SCANCODE_ESCAPE);
    ASSERT_TRUE(dialogue()->askingSkip());
    press(SDL_SCANCODE_RETURN);
    tick(3);
    // 一路跳到选择：中间两句没压对话框，但记进了回看与 spokenKeys；给东西、置旗标照常。
    ASSERT_EQ(top(), "Dialogue") << "停在选择框上";
    EXPECT_TRUE(app_.awaitingCommand());
    EXPECT_EQ(app_.state().flag("test_d.pick"), 0) << "**选择永远不自动选**";
    EXPECT_EQ(spoken(kB), 1);
    EXPECT_EQ(spoken(kC), 1);
    EXPECT_TRUE(logged(kB));
    EXPECT_TRUE(logged(kC));
    EXPECT_EQ(app_.state().itemCount("material_lingshi"), stonesBefore + 3) << "途中的 give 照常";
    EXPECT_EQ(app_.state().flag("test_d.mid"), 1) << "途中的 flag.set 照常";
    EXPECT_FALSE(app_.skippingDialogue()) << "到了选择就收住";
    ASSERT_TRUE(app_.skipNotice().active());
    EXPECT_NE(app_.skipNotice().text().find("2"), std::string::npos) << app_.skipNotice().text();
    EXPECT_NE(app_.skipNotice().text().find("Tab"), std::string::npos) << "默认键位下写的是 Tab";
    // 选完之后照常逐框显示。
    press(SDL_SCANCODE_RETURN);
    tick(2);
    EXPECT_EQ(app_.state().flag("test_d.pick"), 1);
    ASSERT_EQ(top(), "Dialogue") << "选择之后那一句照常压对话框，不再跳";
    EXPECT_EQ(spoken(kD), 1);
}

TEST_F(DialogueFlow, SkippingRunsToTheEndOfTheScriptAndTabThenOpensTheLog) {
    start("skip_to_end.lua");
    ASSERT_EQ(top(), "Dialogue");
    press(SDL_SCANCODE_ESCAPE);
    press(SDL_SCANCODE_RETURN);
    tick(3);
    EXPECT_EQ(top(), "World") << "跳到脚本结束，人回到地图上";
    EXPECT_EQ(app_.state().flag("test_d.end"), 1);
    EXPECT_FALSE(app_.scripts().isRunning());
    ASSERT_TRUE(app_.skipNotice().holding());
    EXPECT_NE(app_.skipNotice().text().find("3"), std::string::npos) << "跳过了三句：" << app_.skipNotice().text();
    for (const char* key : {kB, kC, kD}) EXPECT_TRUE(logged(key)) << key;
    // 提示停着时按菜单键：翻开的是回看，不是主菜单。
    press(SDL_SCANCODE_TAB);
    ASSERT_EQ(top(), "DialogueLog");
    const auto* log = dynamic_cast<const fanren::game::DialogueLogScene*>(app_.topScene());
    ASSERT_NE(log, nullptr);
    EXPECT_GE(log->list().count(), 4) << "这一段四句都翻得到";
    press(SDL_SCANCODE_TAB);
    EXPECT_EQ(top(), "World");
    // 提示过去之后，菜单键照旧开主菜单。
    tick(6 * 60);
    EXPECT_FALSE(app_.skipNotice().active());
    press(SDL_SCANCODE_TAB);
    EXPECT_EQ(top(), "Menu");
}

TEST_F(DialogueFlow, EscOnAChoiceIsTheChoicesOwnCancelNotASkip) {
    start("choose.lua");
    press(SDL_SCANCODE_RETURN);
    press(SDL_SCANCODE_RETURN);
    tick(2);
    ASSERT_NE(dialogue(), nullptr);
    ASSERT_TRUE(app_.awaitingCommand());
    EXPECT_EQ(spoken(kB), 1);
    // 有选项的框：取消键归选项自己（取消 = nil），不弹跳过确认框。
    press(SDL_SCANCODE_ESCAPE);
    tick(2);
    EXPECT_FALSE(app_.skippingDialogue());
    EXPECT_EQ(app_.state().flag("test_d.pick"), 0) << "取消返回 nil，脚本记 0";
    EXPECT_EQ(spoken(kC), 1);
}

TEST_F(DialogueFlow, ThePromptsOutsideAScriptOfferNoSkip) {
    app_.showMessage(kA);
    tick();
    ASSERT_EQ(top(), "Dialogue");
    press(SDL_SCANCODE_ESCAPE);
    ASSERT_NE(dialogue(), nullptr);
    EXPECT_FALSE(dialogue()->askingSkip()) << "不经脚本的提示框没有「一段」可跳";
}

// ---------------------------------------------------------------------------
// 插话气泡（4.4）
// ---------------------------------------------------------------------------

TEST_F(DialogueFlow, ABubbleOverSomeoneOnScreenNeitherStopsTheScriptNorTakesInput) {
    start("bubble_here.lua");
    EXPECT_EQ(top(), "World") << "气泡不压对话框";
    EXPECT_EQ(app_.state().flag("test_d.bubbled"), 1) << "脚本不等人，接着往下跑完";
    ASSERT_TRUE(app_.speechBubbles().active());
    EXPECT_EQ(app_.speechBubbles().bubbles().front().roleId, "han_mu");
    EXPECT_EQ(spoken(kA), 1) << "气泡同样记进 spokenKeys";
    EXPECT_TRUE(logged(kA)) << "也记进回看";
    // 停 2 秒加淡入淡出，之后自己没了。
    tick(4 * 60);
    EXPECT_FALSE(app_.speechBubbles().active());
}

TEST_F(DialogueFlow, ABubbleForSomeoneNotOnScreenFallsBackToAnOrdinaryTalk) {
    start("bubble_away.lua");
    ASSERT_EQ(top(), "Dialogue") << "说话人不在画面上：退回成普通对白，话不丢";
    EXPECT_FALSE(app_.speechBubbles().active());
    EXPECT_EQ(spoken(kB), 1);
    EXPECT_EQ(app_.state().flag("test_d.away"), 0) << "像 talk 一样等玩家按确认";
    press(SDL_SCANCODE_RETURN);
    tick(2);
    EXPECT_EQ(app_.state().flag("test_d.away"), 1);
}

TEST(DialogueBubble, LayoutStaysOnScreenAndThePointerPointsAtTheSpeaker) {
    using fanren::game::bubbleLayout;
    const auto mid = bubbleLayout({640, 300}, 200, 22);
    EXPECT_LT(mid.box.y + mid.box.h, 300) << "框在头顶之上";
    EXPECT_EQ(mid.tip.x, 640);
    EXPECT_EQ(mid.tip.y, 300) << "尖角的尖落在说话人头顶";
    const auto edge = bubbleLayout({4, 300}, 200, 22);
    EXPECT_GE(edge.box.x, 0) << "贴着左边也不出屏";
    const auto right = bubbleLayout({1278, 300}, 200, 22);
    EXPECT_LE(right.box.x + right.box.w, 1280);
    const auto high = bubbleLayout({640, 10}, 200, 22);
    EXPECT_GE(high.box.y, 0) << "贴着顶边也不出屏";
}
