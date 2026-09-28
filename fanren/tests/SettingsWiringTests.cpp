// 系统设置的接线（docs/settings.md 第 7 节第 3–5 条）：真 Application、真 data/、SDL 真事件（SDL_PushEvent，
// 与 EngineTests 的 pushKey 同一个做法），从玩家碰得到的入口进去——
//   标题画面第 3 项、主菜单「设置」、面板里的左右键与确认键、Esc 关面板、Alt+Enter、改键面板（二期）。
//
// 资产根是一份只属于这一批的临时拷贝（data/、scripts/、maps/，与 QuickSaveTests 同一个做法）：
// 设置文件的默认路径 <资产根>/saves/settings.json 落在临时根里，仓库里的 saves/ 一个字节不碰；
// 要写盘的用例一律 enable 到临时根里各自的文件名上。
#include <SDL3/SDL.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <optional>
#include <string>

#include "TempDir.h"
#include "core/battle/Battle.h"
#include "game/Application.h"
#include "game/BattleView.h"
#include "game/DialogueScene.h"
#include "game/KeyConfigScene.h"
#include "game/SettingsScene.h"
#include "game/TitleScene.h"
#include "game/WorldScene.h"
#include "io/SettingsFile.h"

namespace {

namespace fs = std::filesystem;

using fanren::game::Application;
using fanren::game::DialogueScene;
using fanren::game::SettingsScene;
using fanren::io::Settings;
using fanren::io::TextSpeed;

// 行的下标即契约第 1 节的 # 次序（「按键设置」一期不可选时也占着第 8 行，二期接通时没挪别的行）。
static_assert(SettingsScene::kBgmVolume == 0 && SettingsScene::kSfxVolume == 1 && SettingsScene::kFullscreen == 2 &&
              SettingsScene::kScale == 3 && SettingsScene::kVSync == 4 && SettingsScene::kEffects == 5 &&
              SettingsScene::kScreenShake == 6 && SettingsScene::kTextSpeed == 7 && SettingsScene::kKeys == 8 &&
              SettingsScene::kRestore == 9 && SettingsScene::kRowCount == 10);
// 标题画面：新的旅程 0 / 继续旅程 1 / 设置 2 / 离开 3（契约 5.1）。
static_assert(fanren::game::TitleScene::kNewJourney == 0 && fanren::game::TitleScene::kContinue == 1 &&
              fanren::game::TitleScene::kSettings == 2 && fanren::game::TitleScene::kQuit == 3);

std::string repoRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "data" / "text" / "ch01_main.json")) return candidate;
    }
    return ".";
}

// mod 是事件自带的修饰键状态（真键盘上 SDL 按键盘的实际状态填它；Alt+Enter 认的就是它）。
void pushKey(SDL_Scancode scancode, bool down, SDL_Keymod mod = SDL_KMOD_NONE) {
    SDL_Event event{};
    event.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
    event.key.scancode = scancode;
    event.key.mod = mod;
    event.key.down = down;
    event.key.repeat = false;
    SDL_PushEvent(&event);
}

class SettingsWiring : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        root_ = std::make_unique<fanren::test::TempDir>("fanren_settings_root");
        for (const char* dir : {"data", "scripts", "maps"}) {
            fs::copy(fs::path(repoRoot()) / dir, root_->path() / dir, fs::copy_options::recursive);
        }
    }
    static void TearDownTestSuite() { root_.reset(); }

    void SetUp() override {
        auto ready = app_.init(root_->path().string(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
    }
    void TearDown() override { app_.shutdown(); }

    void tick(int frames = 1) {
        for (int i = 0; i < frames; ++i) app_.tick(1.0 / 60.0);
    }
    // 按一下：按下那一帧收键，松开再过一帧。
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
    // 站到韩家村，世界层在底。
    void standInTheWorld() {
        ASSERT_TRUE(app_.loadMap(fanren::game::kNewGameMap, std::string{}).ok);
        app_.pushScene(std::make_unique<fanren::game::WorldScene>());
        tick();
        ASSERT_EQ(top(), "World");
    }
    // 直接压一块设置面板，返回它（面板归场景栈所有，弹出之前指针有效）。
    SettingsScene* openPanel() {
        auto scene = std::make_unique<SettingsScene>();
        SettingsScene* panel = scene.get();
        app_.pushScene(std::move(scene));
        tick();
        EXPECT_EQ(top(), "Settings");
        return panel;
    }
    [[nodiscard]] static std::string fileIn(const char* name) { return (root_->path() / name).string(); }

    static std::unique_ptr<fanren::test::TempDir> root_;
    Application app_;
};

std::unique_ptr<fanren::test::TempDir> SettingsWiring::root_;

}  // namespace

// ---------------------------------------------------------------------------
// 两处入口（契约 5.1）
// ---------------------------------------------------------------------------

TEST_F(SettingsWiring, TheTitleScreensThirdItemOpensTheSettingsPanel) {
    app_.pushScene(std::make_unique<fanren::game::TitleScene>());
    tick(40);   // 标题画面淡入的前半截（0.6 秒）不收键
    ASSERT_EQ(top(), "Title");
    // 从第一项往上绕：↑ 到「离开」，再 ↑ 到「设置」。「继续旅程」可不可选都不影响这两步。
    press(SDL_SCANCODE_UP);
    press(SDL_SCANCODE_UP);
    press(SDL_SCANCODE_RETURN);
    EXPECT_EQ(top(), "Settings") << "标题画面第 3 项确认，栈顶应是设置面板";

    // 面板一收，标题画面还在（面板压在它上面，不是换掉它）。
    press(SDL_SCANCODE_ESCAPE);
    EXPECT_EQ(top(), "Title");
}

TEST_F(SettingsWiring, TheMainMenusSettingsEntryOpensTheSamePanel) {
    standInTheWorld();
    app_.openMainMenu();
    tick();
    ASSERT_EQ(top(), "Menu");
    // 左栏七项：状态 · 物品 · 法门 · 记事 · 存盘 · 设置 · 返回。从「状态」往上绕两格就是「设置」。
    press(SDL_SCANCODE_UP);
    press(SDL_SCANCODE_UP);
    press(SDL_SCANCODE_RETURN);
    EXPECT_EQ(top(), "Settings") << "主菜单「设置」确认，栈顶应是设置面板";

    // 面板一收，主菜单重新成为顶层。
    press(SDL_SCANCODE_ESCAPE);
    EXPECT_EQ(top(), "Menu");
}

// ---------------------------------------------------------------------------
// 面板里调值：立刻生效；关面板写到 enable 过的路径、读回相等
// ---------------------------------------------------------------------------

TEST_F(SettingsWiring, LeftAndRightInThePanelMoveTheMusicVolumeInTheSettingsAndTheEngine) {
    const std::string path = fileIn("volume_settings.json");
    ASSERT_TRUE(app_.enableSettingsFile(path).ok);
    ASSERT_FALSE(fs::exists(path)) << "先验：第一次启动，文件还不在";
    const SettingsScene* panel = openPanel();
    ASSERT_EQ(panel->selection(), SettingsScene::kBgmVolume) << "光标一进来停在第 0 行";

    // 默认 10 档已经是顶：→ 按不动，← 才往下走。
    press(SDL_SCANCODE_RIGHT);
    EXPECT_EQ(app_.settings().bgmVolume, 10);
    press(SDL_SCANCODE_LEFT);
    EXPECT_EQ(app_.settings().bgmVolume, 9);
    EXPECT_FLOAT_EQ(app_.engine().bgmVolume(), 0.81f) << "引擎的增益同一帧就变（(9/10)²）";
    press(SDL_SCANCODE_LEFT);
    press(SDL_SCANCODE_RIGHT);
    EXPECT_EQ(app_.settings().bgmVolume, 9);
    EXPECT_FLOAT_EQ(app_.engine().bgmVolume(), fanren::io::volumeGain(9));
    // 调的是音乐那一路，音效那一路不动。
    EXPECT_EQ(app_.settings().sfxVolume, 10);
    EXPECT_FLOAT_EQ(app_.engine().sfxVolume(), 1.f);
    EXPECT_FALSE(fs::exists(path)) << "调的时候不写盘：关面板时才写";

    press(SDL_SCANCODE_ESCAPE);
    EXPECT_NE(top(), "Settings");
    ASSERT_TRUE(fs::exists(path)) << "有改动，关面板就写到 enable 过的那份";
    const fanren::io::SettingsRead read = fanren::io::loadSettings(path);
    EXPECT_TRUE(read.error.empty()) << read.error;
    EXPECT_EQ(read.settings, app_.settings());
    EXPECT_EQ(read.settings.bgmVolume, 9);
}

TEST_F(SettingsWiring, ClosingThePanelWithoutAnEnabledFileWritesNothing) {
    openPanel();
    press(SDL_SCANCODE_LEFT);
    ASSERT_EQ(app_.settings().bgmVolume, 9) << "先验：确实改了东西";
    press(SDL_SCANCODE_ESCAPE);
    EXPECT_NE(top(), "Settings");
    EXPECT_FALSE(fs::exists(app_.defaultSettingsPath())) << "没 enable 过就不碰文件";
    EXPECT_FALSE(fs::exists(root_->path() / "saves")) << "连目录都不该建";
    const auto saved = app_.saveSettings();
    EXPECT_TRUE(saved.ok);
    EXPECT_FALSE(saved.value) << "没 enable 过：成功的空操作";
}

TEST_F(SettingsWiring, AnUnchangedPanelDoesNotWriteAndAFailedWriteKeepsThePanelOpen) {
    // 没改动就关：不写（第一次启动的玩家开关一下面板，不该凭空多出一个文件）。
    const std::string untouched = fileIn("untouched_settings.json");
    ASSERT_TRUE(app_.enableSettingsFile(untouched).ok);
    openPanel();
    press(SDL_SCANCODE_DOWN);
    press(SDL_SCANCODE_ESCAPE);
    EXPECT_NE(top(), "Settings");
    EXPECT_FALSE(fs::exists(untouched));

    // 写不进去（那个路径本身是一个目录）：留在面板上，原因写在说明行；再按一次才关，这一局照样按改过的跑。
    const fs::path blocked = root_->path() / "blocked_settings.json";
    fs::create_directories(blocked);
    static_cast<void>(app_.enableSettingsFile(blocked.string()));   // 读也读不了：已按默认，这里不关心
    SettingsScene* panel = openPanel();
    press(SDL_SCANCODE_LEFT);
    press(SDL_SCANCODE_ESCAPE);
    EXPECT_EQ(top(), "Settings") << "写盘失败不许静默关掉";
    // 说明行只有两行：先写「再按一次…」，再写短原因（点名写不进的那个文件），整条路径只进标准错误。
    EXPECT_EQ(panel->note().rfind(app_.text("ui.settings.save_failed.again"), 0), 0u) << panel->note();
    EXPECT_NE(panel->note().find("blocked_settings.json"), std::string::npos) << "原因要说出来：" << panel->note();
    EXPECT_EQ(panel->note().find(root_->path().string()), std::string::npos) << "整条路径放不下，不进说明行";
    EXPECT_EQ(app_.settings().bgmVolume, 9);
    press(SDL_SCANCODE_ESCAPE);
    EXPECT_NE(top(), "Settings") << "再按一次才关";
    EXPECT_EQ(app_.settings().bgmVolume, 9) << "这一局照样按改过的跑";
}

// 曾用名：TheKeysRowIsSkippedAndRestoreBringsEveryDefaultBack（一期「按键设置」不可选，光标跳过它；二期接通了）。
TEST_F(SettingsWiring, TheKeysRowOpensTheKeyPanelAndRestoreBringsEveryDefaultBack) {
    SettingsScene* panel = openPanel();
    for (int i = 0; i < SettingsScene::kTextSpeed; ++i) press(SDL_SCANCODE_DOWN);
    ASSERT_EQ(panel->selection(), SettingsScene::kTextSpeed);
    press(SDL_SCANCODE_RIGHT);
    EXPECT_EQ(app_.settings().textSpeed, TextSpeed::Fast);
    EXPECT_EQ(DialogueScene::revealRate(app_), std::optional<double>(120.0)) << "对话框用的速率跟着变";

    // 「按键设置」（第 8 行）可选了：确认 → 栈顶是改键面板；Esc 收回来，设置面板还在原处。
    press(SDL_SCANCODE_DOWN);
    ASSERT_EQ(panel->selection(), SettingsScene::kKeys);
    press(SDL_SCANCODE_RETURN);
    EXPECT_EQ(top(), "KeyConfig");
    press(SDL_SCANCODE_ESCAPE);
    EXPECT_EQ(top(), "Settings");
    EXPECT_EQ(panel->selection(), SettingsScene::kKeys);
    press(SDL_SCANCODE_DOWN);
    ASSERT_EQ(panel->selection(), SettingsScene::kRestore);

    // 恢复默认：全部（含键位）回到默认，说明行写「已恢复默认」。
    Settings custom = app_.settings();
    custom.fullscreen = true;
    custom.effects = fanren::io::EffectsLevel::Lite;
    custom.keys = {{"menu", {SDL_SCANCODE_M, 0}}};
    app_.setSettings(custom);
    ASSERT_EQ(app_.engine().customKeys(fanren::engine::Engine::Key::Menu)[0], SDL_SCANCODE_M) << "先验：键位推到了引擎";
    press(SDL_SCANCODE_RETURN);
    EXPECT_EQ(app_.settings(), Settings{});
    EXPECT_FALSE(app_.engine().fullscreen());
    EXPECT_EQ(app_.engine().effectsLevel(), fanren::engine::EffectsLevel::Full);
    EXPECT_EQ(app_.engine().customKeys(fanren::engine::Engine::Key::Menu)[0], SDL_SCANCODE_TAB) << "键位也回到默认";
    EXPECT_EQ(panel->note(), "已恢复默认");
}

TEST_F(SettingsWiring, EveryValueRowReachesTheEngineOrTheSettingsThroughThePanel) {
    const SettingsScene* panel = openPanel();
    // 第 1–6 行各按一下 →：音效音量顶着不动（10 档），其余五项都翻到另一档。
    press(SDL_SCANCODE_DOWN);
    ASSERT_EQ(panel->selection(), SettingsScene::kSfxVolume);
    press(SDL_SCANCODE_LEFT);
    EXPECT_EQ(app_.settings().sfxVolume, 9);
    EXPECT_FLOAT_EQ(app_.engine().sfxVolume(), 0.81f);
    for (int row = SettingsScene::kFullscreen; row <= SettingsScene::kScreenShake; ++row) {
        press(SDL_SCANCODE_DOWN);
        ASSERT_EQ(panel->selection(), row);
        press(SDL_SCANCODE_RIGHT);
    }
    EXPECT_TRUE(app_.settings().fullscreen);
    EXPECT_TRUE(app_.engine().fullscreen());
    EXPECT_EQ(app_.settings().scale, fanren::io::DisplayScale::Fit);
    EXPECT_FALSE(app_.engine().integerScale());
    EXPECT_FALSE(app_.settings().vsync);
    EXPECT_FALSE(app_.engine().vsync());
    EXPECT_EQ(app_.settings().effects, fanren::io::EffectsLevel::Lite);
    EXPECT_EQ(app_.engine().effectsLevel(), fanren::engine::EffectsLevel::Lite);
    EXPECT_FALSE(app_.settings().screenShake);
}

// ---------------------------------------------------------------------------
// Alt+Enter：引擎自己切，Application 每帧对账、写盘（契约第 3 节）
// ---------------------------------------------------------------------------

TEST_F(SettingsWiring, AltEnterFlipsFullscreenAndTheSettingsFollowTheWindow) {
    const std::string path = fileIn("altenter_settings.json");
    ASSERT_TRUE(app_.enableSettingsFile(path).ok);
    standInTheWorld();
    ASSERT_FALSE(app_.settings().fullscreen);

    pushKey(SDL_SCANCODE_LALT, true, SDL_KMOD_LALT);
    pushKey(SDL_SCANCODE_RETURN, true, SDL_KMOD_LALT);
    tick();
    EXPECT_TRUE(app_.engine().fullscreen());
    EXPECT_TRUE(app_.settings().fullscreen) << "以引擎（窗口的真实状态）为准改设置";
    ASSERT_TRUE(fs::exists(path)) << "Alt+Enter 切了全屏就写盘";
    EXPECT_TRUE(fanren::io::loadSettings(path).settings.fullscreen);
    // 这一帧的按键状态还留在引擎里（下一次 pollEvents 才清）：直接问 Confirm 出没出。
    EXPECT_FALSE(app_.engine().keyPressed(fanren::engine::Engine::Key::Confirm)) << "Alt+Enter 不是确认键";
    EXPECT_FALSE(app_.engine().keyDown(fanren::engine::Engine::Key::Confirm));
    pushKey(SDL_SCANCODE_RETURN, false, SDL_KMOD_LALT);
    pushKey(SDL_SCANCODE_LALT, false);
    tick();
    EXPECT_TRUE(app_.settings().fullscreen);
}

// 设置面板开着时 Alt+Enter 那一次写盘失败：原因写在面板的说明行上，不只进日志。
TEST_F(SettingsWiring, AnAltEnterWriteFailureShowsOnTheOpenPanel) {
    const fs::path blocked = root_->path() / "blocked_altenter.json";
    fs::create_directories(blocked);
    static_cast<void>(app_.enableSettingsFile(blocked.string()));
    SettingsScene* panel = openPanel();
    ASSERT_TRUE(panel->note().empty());
    pushKey(SDL_SCANCODE_RETURN, true, SDL_KMOD_LALT);
    tick();
    pushKey(SDL_SCANCODE_RETURN, false, SDL_KMOD_LALT);
    tick();
    EXPECT_TRUE(app_.settings().fullscreen) << "写不进也照样按全屏跑";
    EXPECT_EQ(top(), "Settings");
    EXPECT_NE(panel->note().find("blocked_altenter.json"), std::string::npos) << panel->note();
}

// ---------------------------------------------------------------------------
// 审查 HIGH：两档的行按住 → 不许来回闪切（左右键带自动重复，250ms 之后每 60ms 一下）
// ---------------------------------------------------------------------------

TEST_F(SettingsWiring, HoldingRightOnATwoStateRowFlipsItOnceNotOnEveryRepeat) {
    const SettingsScene* panel = openPanel();
    press(SDL_SCANCODE_DOWN);
    press(SDL_SCANCODE_DOWN);
    ASSERT_EQ(panel->selection(), SettingsScene::kFullscreen);

    const auto hold = [&](SDL_Scancode key, bool& fullscreenAfter) {
        int presses = 0;
        int flips = 0;
        bool last = app_.settings().fullscreen;
        pushKey(key, true);
        const std::uint64_t until = SDL_GetTicks() + 400;
        while (SDL_GetTicks() < until) {
            tick();
            const auto logical = key == SDL_SCANCODE_RIGHT ? fanren::engine::Engine::Key::Right
                                                           : fanren::engine::Engine::Key::Left;
            if (app_.engine().keyPressed(logical)) ++presses;
            if (app_.settings().fullscreen != last) {
                ++flips;
                last = app_.settings().fullscreen;
            }
            SDL_Delay(10);
        }
        pushKey(key, false);
        tick();
        fullscreenAfter = app_.settings().fullscreen;
        EXPECT_GE(presses, 3) << "先验：按住 400ms，首按之后确实自动重复了（否则这条判不出闪切）";
        return flips;
    };
    bool after = false;
    EXPECT_EQ(hold(SDL_SCANCODE_RIGHT, after), 1) << "按住 → 只切一次";
    EXPECT_TRUE(after) << "→ 是契约表里的第二档：全屏";
    EXPECT_TRUE(app_.engine().fullscreen());
    EXPECT_EQ(hold(SDL_SCANCODE_LEFT, after), 1) << "按住 ← 也只切一次";
    EXPECT_FALSE(after) << "← 是第一档：窗口";
    // 到头就停：已经是窗口再按 ←，不动。
    press(SDL_SCANCODE_LEFT);
    EXPECT_FALSE(app_.settings().fullscreen);
}

// ---------------------------------------------------------------------------
// 审查 MEDIUM：面板开着就关窗口（X / Alt+F4 → run 退出 → shutdown），改动不丢
// ---------------------------------------------------------------------------

TEST_F(SettingsWiring, ChangesSurviveClosingTheWindowWithThePanelStillOpen) {
    const std::string path = fileIn("closed_window_settings.json");
    ASSERT_TRUE(app_.enableSettingsFile(path).ok);
    openPanel();
    press(SDL_SCANCODE_LEFT);
    ASSERT_EQ(app_.settings().bgmVolume, 9);
    ASSERT_FALSE(fs::exists(path)) << "先验：面板还开着，没写过";
    app_.shutdown();
    ASSERT_TRUE(fs::exists(path)) << "退出时补写了面板里的改动";
    EXPECT_EQ(fanren::io::loadSettings(path).settings.bgmVolume, 9);
}

TEST_F(SettingsWiring, ShuttingDownWithoutChangesLeavesABrokenFileAlone) {
    const fs::path path = root_->path() / "broken_settings.json";
    {
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        out << "{ \"bgm_volume\": 3,";
    }
    EXPECT_FALSE(app_.enableSettingsFile(path.string()).ok) << "先验：坏文件";
    openPanel();
    press(SDL_SCANCODE_DOWN);   // 只挪光标，不改值
    app_.shutdown();
    std::ifstream in(path, std::ios::binary);
    const std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    EXPECT_EQ(text, "{ \"bgm_volume\": 3,") << "没人改过设置：退出时不许覆盖那个坏文件";
}

// ---------------------------------------------------------------------------
// 文字速度、战斗震屏：由 DialogueScene / BattleView 读 settings()
// ---------------------------------------------------------------------------

TEST_F(SettingsWiring, TheDialogueRevealsAtTheRateTheSettingsName) {
    EXPECT_EQ(DialogueScene::revealRate(app_), std::optional<double>(60.0)) << "默认 = 改造前的 60 字/秒";
    Settings s;
    s.textSpeed = TextSpeed::Slow;
    app_.setSettings(s);
    EXPECT_EQ(DialogueScene::revealRate(app_), std::optional<double>(30.0));
    s.textSpeed = TextSpeed::Instant;
    app_.setSettings(s);
    EXPECT_FALSE(DialogueScene::revealRate(app_).has_value()) << "瞬显：一上来就全显，没有速率";
}

TEST_F(SettingsWiring, ScreenShakeOffKeepsTheBattleStagesAmplitudeAtZero) {
    using fanren::core::battle::BattleState;
    using fanren::core::battle::Unit;
    Unit hero;
    hero.id = hero.name = "hanli";
    hero.hp = hero.maxHp = 100;
    hero.attack = 10;
    hero.speed = 10;
    hero.ally = true;
    Unit wolf = hero;
    wolf.id = wolf.name = "wild_wolf";
    wolf.ally = false;
    wolf.speed = 8;
    BattleState battle;
    battle.setup({hero, wolf}, 7u);

    // BattleView 在无头下也建得起来（贴图全是无效句柄，画是空操作）；震屏的开关就在它那一处。
    Settings off;
    off.screenShake = false;
    app_.setSettings(off);
    {
        fanren::game::BattleView view(app_, battle, "settings_probe", "drill_ground");
        view.onBreak(1);
        view.onChargeDeclare(1);
        EXPECT_FLOAT_EQ(view.shakeAmplitude(), 0.f) << "战斗震屏关了，破势那一下也不许震";
    }
    // [配对的负向] 开着（默认）同一下就震：证明上面那个 0 不是因为无头下根本没震。
    app_.setSettings(Settings{});
    {
        fanren::game::BattleView view(app_, battle, "settings_probe", "drill_ground");
        view.onBreak(1);
        EXPECT_GT(view.shakeAmplitude(), 0.f);
    }
}

// ---------------------------------------------------------------------------
// 开着设置面板的时间记进 playSecondsSystem（契约第 3 节，与主菜单同口径）
// ---------------------------------------------------------------------------

TEST_F(SettingsWiring, TimeSpentInThePanelIsSystemTimeNotPlayTime) {
    standInTheWorld();
    openPanel();
    const double gameplay = app_.state().playSecondsGameplay;
    const double system = app_.state().playSecondsSystem;
    tick(30);
    EXPECT_DOUBLE_EQ(app_.state().playSecondsGameplay, gameplay) << "开着设置面板不算玩的时间";
    EXPECT_NEAR(app_.state().playSecondsSystem - system, 0.5, 1e-9);

    // [配对的负向] 面板一收，时间又记回玩法那一栏。
    press(SDL_SCANCODE_ESCAPE);
    ASSERT_EQ(top(), "World");
    const double after = app_.state().playSecondsGameplay;
    tick(30);
    EXPECT_NEAR(app_.state().playSecondsGameplay - after, 0.5, 1e-9);
}

// ---------------------------------------------------------------------------
// 二期：改键（契约第 7 节第 3–5 条二期那几条）
// ---------------------------------------------------------------------------

// 默认键位下，四处提示展开出来与改造前写死的字面逐字节相同（对白框那句从前写死在 DialogueScene.cpp 里）。
TEST_F(SettingsWiring, DefaultKeyPromptsAreByteForByteTheOldText) {
    EXPECT_EQ(app_.text("ui.menu.keys"), "Tab 关闭　Esc 返回");
    EXPECT_EQ(app_.text("ui.battle.hint.menu"), "↑↓ 选择 · ←→ 蓄劲 · 回车 确定 · Esc / Tab 返回 · 按住 Ctrl 加速");
    EXPECT_EQ(app_.text("ui.battle.hint.watch"), "按住 Ctrl 加速");
    EXPECT_EQ(app_.text("ui.dialogue.keys"), "Tab 回看　长按 Ctrl 快进");
    // 文案本身存的是占位：展开是 Application::text 做的，不是文案里还写着旧键名。
    EXPECT_NE(app_.data().lookupText("ui.menu.keys").find("{key.menu}"), std::string::npos);
}

// 从玩家碰得到的入口改键：设置面板「按键设置」→ 改键面板 →「菜单」第一格 → 按 M。
// 引擎的键位、提示文案、写盘，三处都跟着变。
TEST_F(SettingsWiring, RebindingThroughTheKeyPanelMovesTheKeyThePromptsAndTheFile) {
    using Key = fanren::engine::Engine::Key;
    const std::string path = fileIn("rebind_settings.json");
    ASSERT_TRUE(app_.enableSettingsFile(path).ok);
    SettingsScene* panel = openPanel();
    for (int i = 0; i < SettingsScene::kKeys; ++i) press(SDL_SCANCODE_DOWN);
    ASSERT_EQ(panel->selection(), SettingsScene::kKeys);
    press(SDL_SCANCODE_RETURN);
    ASSERT_EQ(top(), "KeyConfig");
    const auto* keys = dynamic_cast<const fanren::game::KeyConfigScene*>(app_.topScene());
    ASSERT_NE(keys, nullptr);
    for (int i = 0; i < static_cast<int>(Key::Menu); ++i) press(SDL_SCANCODE_DOWN);
    ASSERT_EQ(keys->row(), static_cast<int>(Key::Menu));
    ASSERT_EQ(keys->slot(), 0);

    press(SDL_SCANCODE_RETURN);
    ASSERT_TRUE(keys->capturing());
    EXPECT_EQ(keys->note(), "按下新键（Esc 作罢，Backspace 清空）");
    press(SDL_SCANCODE_M);
    EXPECT_FALSE(keys->capturing());
    EXPECT_EQ(app_.engine().customKeys(Key::Menu), (fanren::engine::Engine::KeySlots{SDL_SCANCODE_M, 0}));
    EXPECT_EQ(app_.settings().keys.at("menu"), (fanren::io::KeySlots{SDL_SCANCODE_M, 0}));
    EXPECT_EQ(app_.settings().keys.size(), 1u) << "只存与默认不同的动作";
    EXPECT_EQ(keys->note(), "改好了。");
    EXPECT_EQ(app_.text("ui.menu.keys"), "M 关闭　Esc 返回") << "提示跟着键位走";
    EXPECT_EQ(app_.text("ui.dialogue.keys"), "M 回看　长按 Ctrl 快进");
    EXPECT_EQ(app_.text("ui.battle.hint.menu"), "↑↓ 选择 · ←→ 蓄劲 · 回车 确定 · Esc / M 返回 · 按住 Ctrl 加速");

    // 新键立刻生效：在改键面板里按 M 就是「菜单键」——关面板（与 Esc 同一个口径）。Tab 不再是菜单。
    press(SDL_SCANCODE_M);
    EXPECT_EQ(top(), "Settings");
    EXPECT_FALSE(fs::exists(path)) << "改键面板自己不写盘：随设置面板关闭时一起写";
    press(SDL_SCANCODE_TAB);
    EXPECT_EQ(top(), "Settings") << "Tab 让出来了，不再关面板";
    press(SDL_SCANCODE_ESCAPE);
    EXPECT_NE(top(), "Settings");
    ASSERT_TRUE(fs::exists(path));
    const fanren::io::SettingsRead read = fanren::io::loadSettings(path);
    EXPECT_TRUE(read.warnings.empty());
    EXPECT_EQ(read.settings.keys, app_.settings().keys);
}

TEST_F(SettingsWiring, InTheKeyPanelEscCancelsBackspaceClearsAndTheLastKeyStays) {
    using Key = fanren::engine::Engine::Key;
    openPanel();
    for (int i = 0; i < SettingsScene::kKeys; ++i) press(SDL_SCANCODE_DOWN);
    press(SDL_SCANCODE_RETURN);
    const auto* keys = dynamic_cast<const fanren::game::KeyConfigScene*>(app_.topScene());
    ASSERT_NE(keys, nullptr);
    press(SDL_SCANCODE_UP);   // 从「向上」往上绕一格 = 最后一行「恢复默认键位」
    press(SDL_SCANCODE_UP);   // 再一格 =「路径行动」
    ASSERT_EQ(keys->row(), static_cast<int>(Key::Action));
    press(SDL_SCANCODE_RIGHT);
    ASSERT_EQ(keys->slot(), 1);

    // Esc 作罢：面板还开着（抓键期间 Esc 不是关面板），键位一格没动。
    press(SDL_SCANCODE_RETURN);
    press(SDL_SCANCODE_ESCAPE);
    EXPECT_EQ(top(), "KeyConfig");
    EXPECT_EQ(keys->note(), "作罢，没有改动。");
    EXPECT_EQ(app_.engine().customKeys(Key::Action), (fanren::engine::Engine::KeySlots{SDL_SCANCODE_E, SDL_SCANCODE_Q}));

    // Backspace 清空第二格（Q）：还有 E，可以。
    press(SDL_SCANCODE_RETURN);
    press(SDL_SCANCODE_BACKSPACE);
    EXPECT_EQ(app_.engine().customKeys(Key::Action), (fanren::engine::Engine::KeySlots{SDL_SCANCODE_E, 0}));
    // 再清第一格（E）：「路径行动」没有固定键，这是最后一个键——拒绝，说明行点名。
    press(SDL_SCANCODE_LEFT);
    press(SDL_SCANCODE_RETURN);
    press(SDL_SCANCODE_DELETE);
    EXPECT_EQ(app_.engine().customKeys(Key::Action), (fanren::engine::Engine::KeySlots{SDL_SCANCODE_E, 0}));
    EXPECT_EQ(keys->note(), "「路径行动」只剩这一个键，先给它另配一个。");
    // 固定键不能配：把 ↑ 配给「路径行动」，说明是谁的固定键。
    press(SDL_SCANCODE_RETURN);
    press(SDL_SCANCODE_UP);
    EXPECT_EQ(keys->note(), "这是「向上」的固定键，不能挪作他用。");
    EXPECT_EQ(keys->row(), static_cast<int>(Key::Action)) << "抓走的 ↑ 不是「光标上移」";

    // 恢复默认键位（最后一行）。
    press(SDL_SCANCODE_DOWN);
    ASSERT_EQ(keys->row(), fanren::game::KeyConfigScene::kRestoreRow);
    press(SDL_SCANCODE_RETURN);
    EXPECT_TRUE(app_.settings().keys.empty());
    EXPECT_EQ(app_.engine().customKeys(Key::Action), (fanren::engine::Engine::KeySlots{SDL_SCANCODE_E, SDL_SCANCODE_Q}));
    EXPECT_EQ(keys->note(), "已恢复默认键位");
}

// 设置文件里键位表整张校验不过（这里是「菜单」配了「向上」的 W，重复）→ 键位整张回默认 + 警告；别的字段照读；
// 没人改过设置，退出时也不去覆盖那个文件。
TEST_F(SettingsWiring, AnInvalidKeyTableInTheFileResetsEveryKeyAndLeavesTheFileAlone) {
    using Key = fanren::engine::Engine::Key;
    const fs::path path = root_->path() / "dup_keys.json";
    const std::string text = R"({"version": 1, "bgm_volume": 6, "keys": {"menu": [26, 0], "action": [8, 0]}})";
    {
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        out << text;
    }
    const auto read = app_.enableSettingsFile(path.string());
    EXPECT_TRUE(read.ok) << "形状是对的，只是整表校验不过：不算坏文件";
    EXPECT_TRUE(app_.settings().keys.empty()) << "整张回默认，不是只回滚「菜单」那一个";
    EXPECT_EQ(app_.engine().customKeys(Key::Menu), (fanren::engine::Engine::KeySlots{SDL_SCANCODE_TAB, 0}));
    EXPECT_EQ(app_.engine().customKeys(Key::Action), (fanren::engine::Engine::KeySlots{SDL_SCANCODE_E, SDL_SCANCODE_Q}));
    EXPECT_EQ(app_.settings().bgmVolume, 6) << "别的字段照读";
    app_.shutdown();
    std::ifstream in(path, std::ios::binary);
    EXPECT_EQ(std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>()), text);
}

TEST_F(SettingsWiring, TimeSpentInTheKeyPanelIsSystemTimeToo) {
    standInTheWorld();
    app_.pushScene(std::make_unique<SettingsScene>(/*openKeyConfig=*/true));
    tick(3);
    ASSERT_EQ(top(), "KeyConfig") << "截图口 settings:keys 走的就是这条路";
    const double gameplay = app_.state().playSecondsGameplay;
    const double system = app_.state().playSecondsSystem;
    tick(30);
    EXPECT_DOUBLE_EQ(app_.state().playSecondsGameplay, gameplay);
    EXPECT_NEAR(app_.state().playSecondsSystem - system, 0.5, 1e-9);
}

// 改键面板在等按键，引擎那边的抓键却被收回了（Engine::cancelKeyCapture）：面板不许一直卡在「等按键」上，
// 下一帧回到常态，方向键照常挪光标。
TEST_F(SettingsWiring, AKeyPanelWhoseCaptureWasWithdrawnStopsWaiting) {
    app_.pushScene(std::make_unique<SettingsScene>(/*openKeyConfig=*/true));
    tick(3);
    const auto* keys = dynamic_cast<const fanren::game::KeyConfigScene*>(app_.topScene());
    ASSERT_NE(keys, nullptr);
    press(SDL_SCANCODE_RETURN);
    ASSERT_TRUE(keys->capturing());
    app_.engine().cancelKeyCapture();
    tick();
    EXPECT_FALSE(keys->capturing());
    press(SDL_SCANCODE_DOWN);
    EXPECT_EQ(keys->row(), 1) << "回到常态：↓ 是挪光标，不是被抓走的键";
}

// 审查 M1：写盘失败之后转去改键，回来按 Esc——从前面板直接关，不重试也不提示（失败的标记只在调值、恢复默认时清）。
// 现在记的是「失败那一刻的设置」：之后改过什么（含键位）就重新试着写，又失败就留在面板上说出来。
TEST_F(SettingsWiring, AfterAFailedWriteAKeyChangeMakesTheNextCloseTryAgain) {
    using Key = fanren::engine::Engine::Key;
    const fs::path blocked = root_->path() / "blocked_after_keys.json";
    fs::create_directories(blocked);
    static_cast<void>(app_.enableSettingsFile(blocked.string()));
    SettingsScene* panel = openPanel();
    press(SDL_SCANCODE_LEFT);
    press(SDL_SCANCODE_ESCAPE);
    ASSERT_EQ(top(), "Settings") << "先验：第一次关面板写盘失败，留在面板上";

    // 转去改键：清掉「路径行动」的第二格（Q）。
    for (int i = 0; i < SettingsScene::kKeys; ++i) press(SDL_SCANCODE_DOWN);
    press(SDL_SCANCODE_RETURN);
    ASSERT_EQ(top(), "KeyConfig");
    press(SDL_SCANCODE_UP);
    press(SDL_SCANCODE_UP);
    press(SDL_SCANCODE_RIGHT);
    press(SDL_SCANCODE_RETURN);
    press(SDL_SCANCODE_BACKSPACE);
    ASSERT_EQ(app_.engine().customKeys(Key::Action)[1], 0) << "先验：键位确实改了";
    press(SDL_SCANCODE_ESCAPE);
    ASSERT_EQ(top(), "Settings");

    press(SDL_SCANCODE_ESCAPE);
    EXPECT_EQ(top(), "Settings") << "失败之后改过键位：这一次关面板要重新写，又写不进就留在面板上";
    EXPECT_EQ(panel->note().rfind(app_.text("ui.settings.save_failed.again"), 0), 0u) << panel->note();
    EXPECT_NE(panel->note().find("blocked_after_keys.json"), std::string::npos) << panel->note();
    // 原样不动再按一次：才关（这一局照样按改过的跑）。
    press(SDL_SCANCODE_ESCAPE);
    EXPECT_NE(top(), "Settings");
    EXPECT_EQ(app_.engine().customKeys(Key::Action)[1], 0);
}

// 审查 L1：Alt+Enter 那次写盘失败时，改键面板压在设置面板上面——原因照样写给设置面板（不限栈顶），收起改键面板就看得见。
TEST_F(SettingsWiring, AnAltEnterWriteFailureUnderTheKeyPanelLandsOnTheSettingsPanel) {
    const fs::path blocked = root_->path() / "blocked_under_keys.json";
    fs::create_directories(blocked);
    static_cast<void>(app_.enableSettingsFile(blocked.string()));
    SettingsScene* panel = openPanel();
    for (int i = 0; i < SettingsScene::kKeys; ++i) press(SDL_SCANCODE_DOWN);
    press(SDL_SCANCODE_RETURN);
    ASSERT_EQ(top(), "KeyConfig");
    pushKey(SDL_SCANCODE_RETURN, true, SDL_KMOD_LALT);
    tick();
    pushKey(SDL_SCANCODE_RETURN, false, SDL_KMOD_LALT);
    tick();
    EXPECT_TRUE(app_.settings().fullscreen);
    EXPECT_NE(panel->note().find("blocked_under_keys.json"), std::string::npos)
        << "改键面板压在上面时也写给设置面板：" << panel->note();
    press(SDL_SCANCODE_ESCAPE);
    ASSERT_EQ(top(), "Settings");
    EXPECT_NE(panel->note().find("blocked_under_keys.json"), std::string::npos) << "收起改键面板，还看得见";
}

// 复查 N2：写盘失败之后，把某个值调走再调回失败那一刻的值——调值时说明已经撤了，若只比值，此刻的设置又恰好等于
// 失败那一刻，Esc 就会直接关、屏上什么提示都没有。现在在本面板调过值就重新试着写，又失败就留下再报一次。
TEST_F(SettingsWiring, AfterAFailedWriteAdjustingAwayAndBackStillRetriesOnClose) {
    const fs::path blocked = root_->path() / "blocked_away_and_back.json";
    fs::create_directories(blocked);
    static_cast<void>(app_.enableSettingsFile(blocked.string()));
    SettingsScene* panel = openPanel();
    press(SDL_SCANCODE_LEFT);
    press(SDL_SCANCODE_ESCAPE);
    ASSERT_EQ(top(), "Settings") << "先验：第一次关面板写盘失败，留在面板上";
    ASSERT_EQ(app_.settings().bgmVolume, 9);

    press(SDL_SCANCODE_LEFT);    // 调走：9 → 8，说明撤了
    press(SDL_SCANCODE_RIGHT);   // 调回：8 → 9，恰好是失败那一刻的值
    ASSERT_EQ(app_.settings().bgmVolume, 9);
    ASSERT_TRUE(panel->note().empty()) << "先验：调值时说明已撤";

    press(SDL_SCANCODE_ESCAPE);
    EXPECT_EQ(top(), "Settings") << "调过值之后再关，要重新写一次，又失败就留下";
    EXPECT_EQ(panel->note().rfind(app_.text("ui.settings.save_failed.again"), 0), 0u) << panel->note();
    EXPECT_NE(panel->note().find("blocked_away_and_back.json"), std::string::npos) << panel->note();
    press(SDL_SCANCODE_ESCAPE);
    EXPECT_NE(top(), "Settings") << "原样不动再按一次才关";
}
