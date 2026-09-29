// 手柄支持（docs/gamepad.md 第 10 节的最低清单）。
//
// 判据抄契约原文、写成字面量，不从被测物推：第 2 节的键位表与显示名、第 3 节摇杆 / 扳机的用例、第 5 节的连发规则、
// 第 6 节「默认展开成」那一列、第 8 节什么时候震。
//
// 五种台子：
//   · 纯函数：Engine 的四个静态函数直接喂；
//   · 无头引擎 + SDL_PushEvent 塞真手柄事件（与 EngineTests 的 pushKey 同一个做法）。which 不在引擎的表里 → 引擎现建一份
//     句柄为空的状态，走的是与真手柄同一段代码；
//   · 虚拟手柄（SDL_AttachVirtualJoystick）走 SDL 的全链路。无头引擎不开手柄子系统，测试自己 SDL_InitSubSystem；
//     开之前用 hint 关掉真手柄驱动，开发机上插着的真手柄不掺进来，用例结束复原；
//   · 真 Application（临时资产根，与 SettingsWiring 同一个做法）：从玩家碰得到的入口进去；
//   · 有画面的真 Application（dummy 视频 + 软件渲染器，临时资产根外加一场测试战斗）：真打一仗看破势震不震，拍手柄提示的截图。
//     非无头的引擎自己开手柄子系统（上线的那条路），开之前同样关掉真手柄驱动。
// 关真手柄驱动的 hint 用 OVERRIDE 优先级并断言设上了：环境变量里设了 SDL_JOYSTICK_HIDAPI=1 之类时普通优先级设不上。
//
// 连发按墙钟（SDL_GetTicks）算：「按住一秒」的用例真的等一秒，逐帧 tick。
#include <SDL3/SDL.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <numbers>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "TempDir.h"
#include "core/battle/Battle.h"
#include "engine/Engine.h"
#include "game/Application.h"
#include "game/BattleScene.h"
#include "game/BattleView.h"
#include "game/DialogueScene.h"
#include "game/KeyConfigScene.h"
#include "game/SettingsScene.h"
#include "game/WorldScene.h"
#include "io/SettingsFile.h"

namespace {

namespace fs = std::filesystem;

using fanren::engine::Engine;
using fanren::game::Application;
using fanren::game::SettingsScene;
using Key = Engine::Key;
using PadButton = Engine::PadButton;
using InputDevice = Engine::InputDevice;

// 塞事件用的两个手柄实例 id。SDL 真发的实例 id 从 1 往上数，这两个数不会撞上虚拟手柄。
constexpr SDL_JoystickID kPad = 4242;
constexpr SDL_JoystickID kOtherPad = 4343;

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
    EXPECT_TRUE(SDL_PushEvent(&event)) << SDL_GetError();
}

// 塞一个手柄按键事件（与真手柄的 SDL_EVENT_GAMEPAD_BUTTON_DOWN / UP 同一个结构）。
void pushPadButton(SDL_JoystickID which, SDL_GamepadButton button, bool down) {
    SDL_Event event{};
    event.type = down ? SDL_EVENT_GAMEPAD_BUTTON_DOWN : SDL_EVENT_GAMEPAD_BUTTON_UP;
    event.gbutton.which = which;
    event.gbutton.button = static_cast<Uint8>(button);
    event.gbutton.down = down;
    EXPECT_TRUE(SDL_PushEvent(&event)) << SDL_GetError();
}

// 塞一个手柄轴事件：摇杆 -32768…32767（y 向下为正），扳机 0…32767。
void pushPadAxis(SDL_JoystickID which, SDL_GamepadAxis axis, Sint16 value) {
    SDL_Event event{};
    event.type = SDL_EVENT_GAMEPAD_AXIS_MOTION;
    event.gaxis.which = which;
    event.gaxis.axis = static_cast<Uint8>(axis);
    event.gaxis.value = value;
    EXPECT_TRUE(SDL_PushEvent(&event)) << SDL_GetError();
}

void pushPadRemoved(SDL_JoystickID which) {
    SDL_Event event{};
    event.type = SDL_EVENT_GAMEPAD_REMOVED;
    event.gdevice.which = which;
    EXPECT_TRUE(SDL_PushEvent(&event)) << SDL_GetError();
}

// 归一的轴值 → SDL 的 -32768…32767。
Sint16 axisValue(float value) {
    return static_cast<Sint16>(std::lround(std::clamp(value, -1.f, 1.f) * 32767.f));
}

// 一份摇杆报告，照 SDL 的真实次序：先 LEFTX、再 LEFTY，complete 时最后跟一个 GAMEPAD_UPDATE_COMPLETE（docs/gamepad.md 第 4 节）。
void pushStickReport(SDL_JoystickID which, float x, float y, bool complete) {
    pushPadAxis(which, SDL_GAMEPAD_AXIS_LEFTX, axisValue(x));
    pushPadAxis(which, SDL_GAMEPAD_AXIS_LEFTY, axisValue(y));
    if (!complete) return;
    SDL_Event event{};
    event.type = SDL_EVENT_GAMEPAD_UPDATE_COMPLETE;
    event.gdevice.which = which;
    EXPECT_TRUE(SDL_PushEvent(&event)) << SDL_GetError();
}

// 本帧「刚按下」的、眼下「按住」的逻辑键（断言「只有它」用）。
std::vector<Key> pressedKeys(const Engine& engine) {
    std::vector<Key> keys;
    for (std::size_t k = 0; k < Engine::kKeyCount; ++k) {
        if (engine.keyPressed(static_cast<Key>(k))) keys.push_back(static_cast<Key>(k));
    }
    return keys;
}

std::vector<Key> heldKeys(const Engine& engine) {
    std::vector<Key> keys;
    for (std::size_t k = 0; k < Engine::kKeyCount; ++k) {
        if (engine.keyDown(static_cast<Key>(k))) keys.push_back(static_cast<Key>(k));
    }
    return keys;
}

// 给人看的键名（失败信息里用）。
std::string names(const std::vector<Key>& keys) {
    std::string out = "[";
    for (const Key key : keys) out += std::string(out.size() > 1 ? " " : "") + Engine::keyId(key);
    return out + "]";
}

class HeadlessPad : public ::testing::Test {
protected:
    void SetUp() override {
        const auto result = engine.init("fanren-tests", true);
        ASSERT_TRUE(result.ok) << result.error;
    }
    void TearDown() override { engine.shutdown(); }

    // 按一下手柄键：按下、收一帧、松开、再收一帧。返回按下那一帧「刚按下」的逻辑键。
    std::vector<Key> tapPad(SDL_GamepadButton button, SDL_JoystickID which = kPad) {
        pushPadButton(which, button, true);
        engine.pollEvents();
        const std::vector<Key> fired = pressedKeys(engine);
        pushPadButton(which, button, false);
        engine.pollEvents();
        return fired;
    }
    std::vector<Key> tapKey(SDL_Scancode scancode) {
        pushKey(scancode, true);
        engine.pollEvents();
        const std::vector<Key> fired = pressedKeys(engine);
        pushKey(scancode, false);
        engine.pollEvents();
        return fired;
    }

    // 按住 ms 毫秒、逐帧收，数 key「刚按下」出了几次（首按那一次也算）。按下、松开各由调用方塞。
    template <typename Down, typename Up>
    int holdAndCount(Key key, int ms, Down down, Up up) {
        down();
        int presses = 0;
        bool heldThroughout = true;
        const std::uint64_t until = SDL_GetTicks() + static_cast<std::uint64_t>(ms);
        while (SDL_GetTicks() < until) {
            engine.pollEvents();
            if (engine.keyPressed(key)) ++presses;
            heldThroughout = heldThroughout && engine.keyDown(key);
            SDL_Delay(5);
        }
        EXPECT_TRUE(heldThroughout) << Engine::keyId(key) << "：按住期间 keyDown 一直为真";
        up();
        engine.pollEvents();
        EXPECT_FALSE(engine.keyDown(key)) << Engine::keyId(key) << "：松开了";
        return presses;
    }

    Engine engine;
};

class KeyRepeat : public HeadlessPad {};

// 一只虚拟手柄（SDL_AttachVirtualJoystick）：15 键 6 轴（键的下标 = SDL_GamepadButton，轴的下标 = SDL_GamepadAxis，
// SDL 给 GAMEPAD 型的虚拟手柄自动配的映射就是这么对的）；Rumble 回调记下收到的每一次强度。
class TestPad {
public:
    // 真手柄的驱动。开手柄子系统之前全关掉：开发机上插着的真手柄不掺进来；用例结束复原（SDL_ResetHint）。
    // 用 OVERRIDE 优先级：环境变量里设了 SDL_JOYSTICK_HIDAPI=1 之类时，普通优先级的 SDL_SetHint 设不上、返回 false（审查方实测），
    // 真驱动就照开不误。设不上就当场红（调用方用 ASSERT_NO_FATAL_FAILURE 接住）。
    static void disableRealDrivers() {
        for (const char* hint : kRealDrivers) {
            ASSERT_TRUE(SDL_SetHintWithPriority(hint, "0", SDL_HINT_OVERRIDE)) << hint << "：" << SDL_GetError();
        }
    }
    static void restoreRealDrivers() {
        for (const char* hint : kRealDrivers) SDL_ResetHint(hint);
    }

    void attach() {
        SDL_VirtualJoystickDesc desc;
        SDL_INIT_INTERFACE(&desc);
        desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
        desc.naxes = SDL_GAMEPAD_AXIS_COUNT;
        desc.nbuttons = SDL_GAMEPAD_BUTTON_DPAD_RIGHT + 1;
        desc.name = "fanren test pad";
        desc.userdata = &rumbles_;
        desc.Rumble = &TestPad::recordRumble;
        id_ = SDL_AttachVirtualJoystick(&desc);
        ASSERT_NE(id_, 0u) << SDL_GetError();
        // 给虚拟手柄设键、设轴要一个它的句柄（引擎那边 SDL_OpenGamepad 自己另开一份，SDL 计引用数）。
        joystick_ = SDL_OpenJoystick(id_);
        ASSERT_NE(joystick_, nullptr) << SDL_GetError();
    }
    void detach() {
        SDL_CloseJoystick(joystick_);
        joystick_ = nullptr;
        ASSERT_TRUE(SDL_DetachVirtualJoystick(id_)) << SDL_GetError();
        id_ = 0;
    }
    // 用例收尾：还挂着就摘掉（在 SDL_Quit 之前调）。
    void release() {
        if (joystick_ != nullptr) SDL_CloseJoystick(joystick_);
        joystick_ = nullptr;
        if (id_ != 0) SDL_DetachVirtualJoystick(id_);
        id_ = 0;
    }

    void setButton(SDL_GamepadButton button, bool down) {
        ASSERT_TRUE(SDL_SetJoystickVirtualButton(joystick_, button, down)) << SDL_GetError();
    }
    // 虚拟手柄的扳机轴是 -32768…32767，对到手柄上是 0…32767（探针实测：-32768 → 0、0 → 16383、32767 → 32767）。
    void setAxis(SDL_GamepadAxis axis, Sint16 value) {
        ASSERT_TRUE(SDL_SetJoystickVirtualAxis(joystick_, axis, value)) << SDL_GetError();
    }

    // 收到的震动里强度不为 0 的那些（SDL 在时长到了之后自己补发一次 (0, 0) 停震，那一次不算「震了」）。
    [[nodiscard]] std::vector<std::pair<Uint16, Uint16>> strongRumbles() const {
        std::vector<std::pair<Uint16, Uint16>> out;
        for (const auto& call : rumbles_) {
            if (call.first != 0 || call.second != 0) out.push_back(call);
        }
        return out;
    }

private:
    static constexpr const char* kRealDrivers[] = {
        SDL_HINT_JOYSTICK_RAWINPUT,    SDL_HINT_JOYSTICK_WGI,    SDL_HINT_XINPUT_ENABLED,
        SDL_HINT_JOYSTICK_DIRECTINPUT, SDL_HINT_JOYSTICK_HIDAPI, SDL_HINT_JOYSTICK_GAMEINPUT,
    };

    static bool SDLCALL recordRumble(void* userdata, Uint16 low, Uint16 high) {
        static_cast<std::vector<std::pair<Uint16, Uint16>>*>(userdata)->emplace_back(low, high);
        return true;
    }

    SDL_JoystickID id_ = 0;
    SDL_Joystick* joystick_ = nullptr;
    std::vector<std::pair<Uint16, Uint16>> rumbles_;
};

// 两个强度（SDL 的 0…0xFFFF）是不是 (low, high)，差 1 以内算相等（浮点乘 0xFFFF 再取整）。
bool rumbled(const std::vector<std::pair<Uint16, Uint16>>& calls, int low, int high) {
    return std::any_of(calls.begin(), calls.end(), [low, high](const std::pair<Uint16, Uint16>& call) {
        return std::abs(call.first - low) <= 1 && std::abs(call.second - high) <= 1;
    });
}

// 无头引擎 + 虚拟手柄。无头引擎不开手柄子系统，由用例自己开（startGamepads）。
class VirtualPad : public HeadlessPad {
protected:
    void SetUp() override {
        ASSERT_NO_FATAL_FAILURE(TestPad::disableRealDrivers());
        HeadlessPad::SetUp();
    }
    void TearDown() override {
        pad.release();
        HeadlessPad::TearDown();   // SDL_Quit：连测试自己开的手柄子系统一起收
        TestPad::restoreRealDrivers();
    }

    void startGamepads() { ASSERT_TRUE(SDL_InitSubSystem(SDL_INIT_GAMEPAD)) << SDL_GetError(); }
    void attach() { pad.attach(); }
    void detach() { pad.detach(); }
    void setButton(SDL_GamepadButton button, bool down) { pad.setButton(button, down); }
    void setAxis(SDL_GamepadAxis axis, Sint16 value) { pad.setAxis(axis, value); }
    [[nodiscard]] std::vector<std::pair<Uint16, Uint16>> strongRumbles() const { return pad.strongRumbles(); }

    TestPad pad;
};

// 真 Application、临时资产根（data/、scripts/、maps/ 的一份拷贝，与 SettingsWiring 同一个做法）。
class PadWiring : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        root_ = std::make_unique<fanren::test::TempDir>("fanren_pad_root");
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
    // 按一下：按下那一帧收键，松开再过一帧。
    void pressPad(SDL_GamepadButton button) {
        pushPadButton(kPad, button, true);
        tick();
        pushPadButton(kPad, button, false);
        tick();
    }
    void pressKey(SDL_Scancode scancode) {
        pushKey(scancode, true);
        tick();
        pushKey(scancode, false);
        tick();
    }
    // 直接压一块设置面板，返回它（面板归场景栈所有，弹出之前指针有效）。
    SettingsScene* openPanel(bool openKeyConfig = false) {
        auto scene = std::make_unique<SettingsScene>(openKeyConfig);
        SettingsScene* panel = scene.get();
        app_.pushScene(std::move(scene));
        tick(3);
        return panel;
    }
    [[nodiscard]] static std::string fileIn(const char* name) { return (root_->path() / name).string(); }

    // 在世界层按住一秒：逐帧 tick，记下主菜单开过几次、合过几次、最后开没开着。
    struct Held {
        int opened = 0;
        int closed = 0;
        bool openAtEnd = false;
        int frames = 0;
    };
    template <typename Down, typename Up>
    Held holdInTheWorldForASecond(Down down, Up up) {
        Held held;
        bool open = false;
        down();
        const std::uint64_t until = SDL_GetTicks() + 1000;
        while (SDL_GetTicks() < until) {
            tick();
            ++held.frames;
            const bool now = top() == "Menu";
            if (now && !open) ++held.opened;
            if (!now && open) ++held.closed;
            open = now;
            SDL_Delay(5);
        }
        held.openAtEnd = open;
        up();
        tick();
        return held;
    }

    static std::unique_ptr<fanren::test::TempDir> root_;
    Application app_;
};

std::unique_ptr<fanren::test::TempDir> PadWiring::root_;

// 真 Application（无头）+ 虚拟手柄：无头引擎不开手柄子系统，这里自己开，挂上一只、收一帧让引擎打开它。
class PadRumbleWiring : public PadWiring {
protected:
    void SetUp() override {
        ASSERT_NO_FATAL_FAILURE(TestPad::disableRealDrivers());
        PadWiring::SetUp();
        ASSERT_TRUE(SDL_InitSubSystem(SDL_INIT_GAMEPAD)) << SDL_GetError();
        pad.attach();
        tick();
        ASSERT_EQ(app_.engine().gamepadCount(), 1);
    }
    void TearDown() override {
        pad.release();
        PadWiring::TearDown();
        TestPad::restoreRealDrivers();
    }
    // 按一下这只虚拟手柄上的键。
    void pressVirtual(SDL_GamepadButton button) {
        pad.setButton(button, true);
        tick();
        pad.setButton(button, false);
        tick();
    }
    // 等正在震的那一下到时（最长 280ms）、收一帧让 SDL 停震：SDL 对「与正在震的强度相同」的请求不再转给手柄。
    void letTheRumbleEnd() {
        SDL_Delay(320);
        tick();
    }

    TestPad pad;
};

// 读回一张 PNG（与 EngineTests 的 grab 同一个做法：SDL 核心库的 PNG 解码，不经被测的 Engine）。
struct Frame {
    int w = 0;
    int h = 0;
    std::vector<std::uint8_t> rgb;
};

Frame loadFrame(const fs::path& png) {
    Frame frame;
    SDL_Surface* loaded = SDL_LoadPNG(png.string().c_str());
    EXPECT_NE(loaded, nullptr) << SDL_GetError();
    if (loaded == nullptr) return frame;
    SDL_Surface* rgba = SDL_ConvertSurface(loaded, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(loaded);
    EXPECT_NE(rgba, nullptr) << SDL_GetError();
    if (rgba == nullptr) return frame;
    frame.w = rgba->w;
    frame.h = rgba->h;
    for (int y = 0; y < frame.h; ++y) {
        const auto* row = static_cast<const std::uint8_t*>(rgba->pixels) + y * rgba->pitch;
        for (int x = 0; x < frame.w; ++x) {
            frame.rgb.insert(frame.rgb.end(), {row[x * 4], row[x * 4 + 1], row[x * 4 + 2]});
        }
    }
    SDL_DestroySurface(rgba);
    return frame;
}

// 矩形里 RGB 不同的像素有几个（只比 RGB：后缓冲的 alpha 没有意义）。
int differingPixels(const Frame& a, const Frame& b, const fanren::engine::Rect& area) {
    if (a.w != b.w || a.h != b.h) return -1;
    int count = 0;
    for (int y = std::max(0, area.y); y < std::min(a.h, area.y + area.h); ++y) {
        for (int x = std::max(0, area.x); x < std::min(a.w, area.x + area.w); ++x) {
            const std::size_t at = (static_cast<std::size_t>(y) * static_cast<std::size_t>(a.w) + static_cast<std::size_t>(x)) * 3;
            if (a.rgb[at] != b.rgb[at] || a.rgb[at + 1] != b.rgb[at + 1] || a.rgb[at + 2] != b.rgb[at + 2]) ++count;
        }
    }
    return count;
}

// 专用的测试战斗（整改轮 LOW-3：不再绑第 3 章野猪的平衡）：一个木桩，架势 1、怕「拳」、气血厚、打人只掉 1 点、比韩立慢——
// 韩立空手一拳就破势，那一下之前谁也不会被重击。只写进 PadScreen 的临时资产根，仓库的 data/ 一个字节不动。
constexpr const char* kTestBattleId = "pad_test_break";
constexpr const char* kTestRoleJson = R"({
  "id": "pad_test_dummy",
  "name": "木桩",
  "realm": "Mortal",
  "maxHp": 999,
  "maxMp": 0,
  "attack": 1,
  "defence": 0,
  "speed": 1,
  "element": 0,
  "magics": [],
  "toughness": 1,
  "weaknesses": ["拳"]
}
)";
constexpr const char* kTestBattleJson = R"({
  "id": "pad_test_break",
  "name": "手柄震动测试",
  "chapter": 1,
  "terrain": "field",
  "can_escape": true,
  "defeat_is_fatal": false,
  "units": [{"role_id": "pad_test_dummy", "faction": "enemy"}]
}
)";

void writeFile(const fs::path& path, const char* text) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out << text;
    ASSERT_TRUE(out.good()) << path.string();
}

// 有画面的真 Application：dummy 视频驱动 + 软件渲染器 + dummy 音频（与 EngineTests 的 SoftwareEngine 同一个配法）。
// 资产根是一份只属于这一批的临时拷贝（data/、scripts/、maps/，与 PadWiring 同一个做法），外加上面那场测试战斗；
// 美术、字体仍由引擎从仓库的 assets/ 读。非无头的引擎自己开手柄子系统——走的就是上线的那条路；开之前照样关掉真手柄驱动。
class PadScreen : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        root_ = std::make_unique<fanren::test::TempDir>("fanren_pad_screen_root");
        for (const char* dir : {"data", "scripts", "maps"}) {
            fs::copy(fs::path(repoRoot()) / dir, root_->path() / dir, fs::copy_options::recursive);
        }
        writeFile(root_->path() / "data" / "roles" / "pad_test_dummy.json", kTestRoleJson);
        writeFile(root_->path() / "data" / "battles" / (std::string(kTestBattleId) + ".json"), kTestBattleJson);
    }
    static void TearDownTestSuite() { root_.reset(); }

    void SetUp() override {
        SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
        SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
        SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "dummy");
        ASSERT_NO_FATAL_FAILURE(TestPad::disableRealDrivers());
        auto ready = app_.init(root_->path().string(), /*headless=*/false);
        ASSERT_TRUE(ready.ok) << ready.error;
    }
    void TearDown() override {
        pad.release();
        app_.shutdown();
        SDL_ResetHint(SDL_HINT_VIDEO_DRIVER);
        SDL_ResetHint(SDL_HINT_RENDER_DRIVER);
        SDL_ResetHint(SDL_HINT_AUDIO_DRIVER);
        TestPad::restoreRealDrivers();
    }

    void tick(int frames = 1) {
        for (int i = 0; i < frames; ++i) app_.tick(1.0 / 60.0);
    }
    std::string top() {
        const fanren::game::Scene* scene = app_.topScene();
        return scene == nullptr ? std::string{} : scene->name();
    }
    void pressPad(SDL_GamepadButton button) {
        pushPadButton(kPad, button, true);
        tick();
        pushPadButton(kPad, button, false);
        tick();
    }
    void pressKey(SDL_Scancode scancode) {
        pushKey(scancode, true);
        tick();
        pushKey(scancode, false);
        tick();
    }
    // 与截图口（main.cpp 的 --screenshot）同一个拍法：beginFrame → drawScenes → captureFrame → endFrame。
    // 拍进本用例自己的临时目录、断言读的就是那一份（整改轮 LOW-2：同一构建目录里并发跑也不会互相踩）；
    // 再尽力拷一份到测试程序所在目录的 shots\（build-<槽>\shots\）给人看——拷不成不判红，找不到那个目录就不拷。
    Frame shoot(const std::string& name) {
        const fs::path png = shots_.path() / name;
        app_.engine().beginFrame();
        app_.drawScenes();
        const auto shot = app_.engine().captureFrame(png.string());
        app_.engine().endFrame();
        EXPECT_TRUE(shot.ok) << shot.error;
        if (!shot.ok) return Frame{};
        if (const char* base = SDL_GetBasePath(); base != nullptr && base[0] != '\0') {
            const fs::path forPeople = fs::path(reinterpret_cast<const char8_t*>(base)) / "shots";
            std::error_code ec;
            fs::create_directories(forPeople, ec);
            if (!ec) fs::copy_file(png, forPeople / name, fs::copy_options::overwrite_existing, ec);
        }
        return loadFrame(png);
    }

    static std::unique_ptr<fanren::test::TempDir> root_;
    Application app_;
    TestPad pad;
    fanren::test::TempDir shots_{"fanren_pad_shots"};
};

std::unique_ptr<fanren::test::TempDir> PadScreen::root_;

}  // namespace

// ---------------------------------------------------------------------------
// 1. 表与纯函数（第 2、3 节）
// ---------------------------------------------------------------------------

TEST(PadTable, EachActionsPadButtonsAreTheContractsTableInDisplayOrder) {
    const std::vector<std::pair<Key, std::vector<PadButton>>> kTable{
        {Key::Up, {PadButton::DpadUp, PadButton::StickUp}},
        {Key::Down, {PadButton::DpadDown, PadButton::StickDown}},
        {Key::Left, {PadButton::DpadLeft, PadButton::StickLeft}},
        {Key::Right, {PadButton::DpadRight, PadButton::StickRight}},
        {Key::Confirm, {PadButton::A}},
        {Key::Cancel, {PadButton::B}},
        {Key::Menu, {PadButton::Y, PadButton::Start}},
        {Key::Skip, {PadButton::RT, PadButton::RB}},
        {Key::Save, {PadButton::Back}},
        {Key::Action, {PadButton::X}},
    };
    ASSERT_EQ(kTable.size(), Engine::kKeyCount);
    for (const auto& [key, buttons] : kTable) {
        EXPECT_EQ(Engine::padButtons(key), buttons) << Engine::keyId(key);
    }
    EXPECT_TRUE(Engine::padButtons(Key::Count).empty());
}

TEST(PadTable, EveryActionHasAPadButtonNoButtonServesTwoAndLbLtServeNone) {
    std::map<PadButton, Key> owner;
    for (std::size_t k = 0; k < Engine::kKeyCount; ++k) {
        const auto key = static_cast<Key>(k);
        const std::vector<PadButton> buttons = Engine::padButtons(key);
        EXPECT_FALSE(buttons.empty()) << Engine::keyId(key) << " 一个手柄键都没有";
        for (const PadButton button : buttons) {
            const auto [it, fresh] = owner.emplace(button, key);
            EXPECT_TRUE(fresh) << Engine::padLabel(button) << " 同时是 " << Engine::keyId(it->second) << " 与 "
                               << Engine::keyId(key) << " 的键";
        }
    }
    EXPECT_EQ(owner.count(PadButton::LB), 0u) << "LB 不配动作";
    EXPECT_EQ(owner.count(PadButton::LT), 0u) << "LT 不配动作";
    EXPECT_EQ(owner.size(), 16u) << "18 个手柄键里配了动作的是 16 个（LB、LT 不配）";
}

TEST(PadLabels, FollowTheContractsTable) {
    const std::vector<std::pair<PadButton, std::string>> kLabels{
        {PadButton::A, "A"},           {PadButton::B, "B"},
        {PadButton::X, "X"},           {PadButton::Y, "Y"},
        {PadButton::Back, "⧉"},        {PadButton::Start, "≡"},
        {PadButton::LB, "LB"},         {PadButton::RB, "RB"},
        {PadButton::LT, "LT"},         {PadButton::RT, "RT"},
        {PadButton::DpadUp, "十字↑"},  {PadButton::DpadDown, "十字↓"},
        {PadButton::DpadLeft, "十字←"}, {PadButton::DpadRight, "十字→"},
        {PadButton::StickUp, "摇杆↑"}, {PadButton::StickDown, "摇杆↓"},
        {PadButton::StickLeft, "摇杆←"}, {PadButton::StickRight, "摇杆→"},
    };
    ASSERT_EQ(kLabels.size(), static_cast<std::size_t>(PadButton::Count)) << "18 个手柄键一个不落";
    for (const auto& [button, label] : kLabels) {
        EXPECT_EQ(Engine::padLabel(button), label) << static_cast<int>(button);
    }
    EXPECT_EQ(Engine::padLabel(PadButton::Count), "");
}

namespace {

std::optional<Key> stick(float x, float y, std::optional<Key> held = std::nullopt) {
    return Engine::stickDirection(x, y, held);
}

// 从某个方向偏 degrees 度（往顺时针，即 y 变大的那一边）的单位向量。
std::pair<float, float> offRight(float degrees) {
    const float radians = degrees * std::numbers::pi_v<float> / 180.f;
    return {std::cos(radians), std::sin(radians)};
}

}  // namespace

TEST(StickDirection, TheCentreAndAnythingShortOfHalfwayIsNoDirection) {
    EXPECT_EQ(stick(0.f, 0.f), std::nullopt);
    EXPECT_EQ(stick(0.49f, 0.f), std::nullopt);
    EXPECT_EQ(stick(0.5f, 0.f), std::optional<Key>(Key::Right));
    EXPECT_EQ(stick(0.f, -0.49f), std::nullopt);
    EXPECT_EQ(stick(0.f, -0.5f), std::optional<Key>(Key::Up));
}

TEST(StickDirection, TheFourWaysAndYGrowsDownwards) {
    EXPECT_EQ(stick(1.f, 0.f), std::optional<Key>(Key::Right));
    EXPECT_EQ(stick(-1.f, 0.f), std::optional<Key>(Key::Left));
    EXPECT_EQ(stick(0.f, -1.f), std::optional<Key>(Key::Up)) << "SDL 的 y 轴向下为正：y < 0 是上";
    EXPECT_EQ(stick(0.f, 1.f), std::optional<Key>(Key::Down));
}

TEST(StickDirection, AHeldDirectionLastsDownToThirtyFivePercent) {
    EXPECT_EQ(stick(0.36f, 0.f, Key::Right), std::optional<Key>(Key::Right));
    EXPECT_EQ(stick(0.34f, 0.f, Key::Right), std::nullopt);
    // [配对] 同一个 0.36 没按住就不到位：迟滞只给按住的方向。
    EXPECT_EQ(stick(0.36f, 0.f), std::nullopt);
    // held 不是四个方向键之一，就当没按住。
    EXPECT_EQ(stick(0.36f, 0.f, Key::Confirm), std::nullopt);
}

TEST(StickDirection, TheExactDiagonalGoesSideways) {
    EXPECT_EQ(stick(0.6f, 0.6f), std::optional<Key>(Key::Right));
    EXPECT_EQ(stick(0.6f, -0.6f), std::optional<Key>(Key::Right));
    EXPECT_EQ(stick(-0.6f, 0.6f), std::optional<Key>(Key::Left));
    EXPECT_EQ(stick(-0.6f, -0.6f), std::optional<Key>(Key::Left));
    // 离对角线差一点就取最近的那条轴。
    EXPECT_EQ(stick(0.6f, 0.61f), std::optional<Key>(Key::Down));
    EXPECT_EQ(stick(0.61f, 0.6f), std::optional<Key>(Key::Right));
}

TEST(StickDirection, AHeldDirectionHoldsUpToFiftyFiveDegreesOffItsAxis) {
    const auto [x50, y50] = offRight(50.f);
    const auto [x60, y60] = offRight(60.f);
    EXPECT_EQ(stick(x50, y50, Key::Right), std::optional<Key>(Key::Right)) << "按住右、偏 50° 仍是右";
    EXPECT_EQ(stick(x60, y60, Key::Right), std::optional<Key>(Key::Down)) << "偏 60° 换成最近的轴：下";
    // [配对] 同样偏 50° 没按住：最近的轴是下——上面那个「仍是右」是迟滞给的，不是本来就近。
    EXPECT_EQ(stick(x50, y50), std::optional<Key>(Key::Down));
    EXPECT_EQ(stick(0.55f, -0.6f, Key::Up), std::optional<Key>(Key::Up));
    // 按住右、直接扳到左：偏 180°，换。
    EXPECT_EQ(stick(-1.f, 0.f, Key::Right), std::optional<Key>(Key::Left));
}

// 整改轮（docs/gamepad.md 第 3 节）：换向算一次新的推——偏出 55° 要换到新方向时，新方向与从零推起一样要 r ≥ 0.5，
// 不够就当松开；0.35 的迟滞只管原方向继续按住。
TEST(StickDirection, SwitchingToANewWayIsAFreshPushThatNeedsHalfway) {
    EXPECT_EQ(stick(0.1f, 0.45f, Key::Right), std::nullopt) << "偏约 77°、r≈0.46：换向不够 0.5，当松开";
    EXPECT_EQ(stick(0.1f, 0.6f, Key::Right), std::optional<Key>(Key::Down)) << "r≈0.61：够了，换成下";
    // [配对] 原方向在 0.35–0.5 之间照旧按住：迟滞还在，只是不给新方向。
    EXPECT_EQ(stick(0.4f, 0.1f, Key::Right), std::optional<Key>(Key::Right));
}

TEST(StickDirection, OutOfRangeInputStaysInRange) {
    EXPECT_EQ(stick(1.f, 1.f), std::optional<Key>(Key::Right)) << "r = √2 当 1 算；恰在对角线取右";
    EXPECT_EQ(stick(-1.f, -1.f), std::optional<Key>(Key::Left));
    EXPECT_EQ(stick(3.f, 0.f), std::optional<Key>(Key::Right));
    EXPECT_EQ(stick(0.f, -3.f, Key::Up), std::optional<Key>(Key::Up));
}

TEST(TriggerDown, HalfwayPressesAndThirtyPercentKeepsItPressed) {
    EXPECT_FALSE(Engine::triggerDown(0.f, false));
    EXPECT_FALSE(Engine::triggerDown(0.49f, false));
    EXPECT_TRUE(Engine::triggerDown(0.5f, false));
    EXPECT_TRUE(Engine::triggerDown(1.f, false));
    EXPECT_TRUE(Engine::triggerDown(0.3f, true)) << "按住时 0.3 仍按住";
    EXPECT_TRUE(Engine::triggerDown(0.4f, true));
    EXPECT_FALSE(Engine::triggerDown(0.29f, true));
    EXPECT_FALSE(Engine::triggerDown(0.f, true));
    // [配对] 同一个 0.4 没按住时不算按下：迟滞只给按住的。
    EXPECT_FALSE(Engine::triggerDown(0.4f, false));
}

// ---------------------------------------------------------------------------
// 2. 无头引擎 + SDL_PushEvent 塞真手柄事件（第 2、4、6、7 节）
// ---------------------------------------------------------------------------

TEST_F(HeadlessPad, AIsAConfirmThatIsPressedHeldAndLetGo) {
    pushPadButton(kPad, SDL_GAMEPAD_BUTTON_SOUTH, true);
    engine.pollEvents();
    EXPECT_EQ(pressedKeys(engine), std::vector<Key>{Key::Confirm}) << names(pressedKeys(engine));
    EXPECT_TRUE(engine.keyDown(Key::Confirm));
    engine.pollEvents();
    EXPECT_FALSE(engine.keyPressed(Key::Confirm)) << "下一帧不再是「刚按下」";
    EXPECT_TRUE(engine.keyDown(Key::Confirm)) << "还按着";
    pushPadButton(kPad, SDL_GAMEPAD_BUTTON_SOUTH, false);
    engine.pollEvents();
    EXPECT_FALSE(engine.keyDown(Key::Confirm));
    EXPECT_EQ(engine.gamepadCount(), 0) << "塞进来的事件建的状态句柄为空，不算接着的手柄";
}

TEST_F(HeadlessPad, EachButtonDrivesItsOwnActionAndNothingElse) {
    const struct {
        SDL_GamepadButton button;
        Key key;
    } kCases[] = {
        {SDL_GAMEPAD_BUTTON_SOUTH, Key::Confirm},      {SDL_GAMEPAD_BUTTON_EAST, Key::Cancel},
        {SDL_GAMEPAD_BUTTON_WEST, Key::Action},        {SDL_GAMEPAD_BUTTON_NORTH, Key::Menu},
        {SDL_GAMEPAD_BUTTON_START, Key::Menu},         {SDL_GAMEPAD_BUTTON_BACK, Key::Save},
        {SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, Key::Skip}, {SDL_GAMEPAD_BUTTON_DPAD_UP, Key::Up},
        {SDL_GAMEPAD_BUTTON_DPAD_DOWN, Key::Down},     {SDL_GAMEPAD_BUTTON_DPAD_LEFT, Key::Left},
        {SDL_GAMEPAD_BUTTON_DPAD_RIGHT, Key::Right},
    };
    for (const auto& c : kCases) {
        pushPadButton(kPad, c.button, true);
        engine.pollEvents();
        EXPECT_EQ(pressedKeys(engine), std::vector<Key>{c.key})
            << "手柄键 " << static_cast<int>(c.button) << "：" << names(pressedKeys(engine));
        EXPECT_EQ(heldKeys(engine), std::vector<Key>{c.key}) << "手柄键 " << static_cast<int>(c.button);
        pushPadButton(kPad, c.button, false);
        engine.pollEvents();
        EXPECT_TRUE(heldKeys(engine).empty()) << "手柄键 " << static_cast<int>(c.button) << " 松开了";
    }
}

TEST_F(HeadlessPad, TriggersPressPastHalfwayAndLetGoBelowThirtyPercent) {
    // RT：过半按下（16384 / 32767 ≥ 0.5）→ 快进；回落到 0.3 以上仍按着，0.3 以下才松。
    pushPadAxis(kPad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, 16000);   // 0.488：不到一半
    engine.pollEvents();
    EXPECT_FALSE(engine.keyDown(Key::Skip));
    pushPadAxis(kPad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, 16384);
    engine.pollEvents();
    EXPECT_TRUE(engine.keyPressed(Key::Skip));
    EXPECT_TRUE(engine.keyDown(Key::Skip));
    pushPadAxis(kPad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, 9831);    // 0.30003：还按着
    engine.pollEvents();
    EXPECT_TRUE(engine.keyDown(Key::Skip));
    EXPECT_FALSE(engine.keyPressed(Key::Skip)) << "没松开过，不是新的一下";
    pushPadAxis(kPad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, 9500);    // 0.29：松了
    engine.pollEvents();
    EXPECT_FALSE(engine.keyDown(Key::Skip));
    pushPadAxis(kPad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, 16000);   // 松开之后 0.488 不算按下
    engine.pollEvents();
    EXPECT_FALSE(engine.keyDown(Key::Skip));
    pushPadAxis(kPad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, 0);
    engine.pollEvents();

    // LT 扳到底：不出任何键，也不换设备（先按一下键盘，让设备回到键盘）。
    ASSERT_EQ(engine.lastInputDevice(), InputDevice::Gamepad) << "先验：RT 过半切到了手柄";
    tapKey(SDL_SCANCODE_RETURN);
    ASSERT_EQ(engine.lastInputDevice(), InputDevice::Keyboard);
    pushPadAxis(kPad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER, SDL_JOYSTICK_AXIS_MAX);
    engine.pollEvents();
    EXPECT_TRUE(pressedKeys(engine).empty()) << names(pressedKeys(engine));
    EXPECT_TRUE(heldKeys(engine).empty()) << names(heldKeys(engine));
    EXPECT_EQ(engine.lastInputDevice(), InputDevice::Keyboard);
}

TEST_F(HeadlessPad, AHeldDpadDownRepeatsAfterAQuarterSecond) {
    pushPadButton(kPad, SDL_GAMEPAD_BUTTON_DPAD_DOWN, true);
    engine.pollEvents();
    ASSERT_TRUE(engine.keyPressed(Key::Down));
    engine.pollEvents();
    EXPECT_FALSE(engine.keyPressed(Key::Down)) << "紧接着的一帧不许再报";
    SDL_Delay(350);
    engine.pollEvents();
    EXPECT_TRUE(engine.keyPressed(Key::Down)) << "按住过了 250ms：连发";
    pushPadButton(kPad, SDL_GAMEPAD_BUTTON_DPAD_DOWN, false);
    engine.pollEvents();
    EXPECT_FALSE(engine.keyDown(Key::Down));
}

TEST_F(HeadlessPad, TheLeftStickPressesADirectionAndLettingGoReleasesIt) {
    const struct {
        SDL_GamepadAxis axis;
        Sint16 value;
        Key key;
    } kCases[] = {
        {SDL_GAMEPAD_AXIS_LEFTX, SDL_JOYSTICK_AXIS_MAX, Key::Right},
        {SDL_GAMEPAD_AXIS_LEFTX, SDL_JOYSTICK_AXIS_MIN, Key::Left},
        {SDL_GAMEPAD_AXIS_LEFTY, SDL_JOYSTICK_AXIS_MIN, Key::Up},
        {SDL_GAMEPAD_AXIS_LEFTY, SDL_JOYSTICK_AXIS_MAX, Key::Down},
    };
    for (const auto& c : kCases) {
        pushPadAxis(kPad, c.axis, c.value);
        engine.pollEvents();
        EXPECT_EQ(pressedKeys(engine), std::vector<Key>{c.key}) << Engine::keyId(c.key) << "：" << names(pressedKeys(engine));
        engine.pollEvents();
        EXPECT_TRUE(engine.keyDown(c.key)) << Engine::keyId(c.key) << " 推着不放";
        EXPECT_FALSE(engine.keyPressed(c.key));
        pushPadAxis(kPad, c.axis, 0);
        engine.pollEvents();
        EXPECT_TRUE(heldKeys(engine).empty()) << Engine::keyId(c.key) << " 回中：" << names(heldKeys(engine));
    }
    // 右摇杆不配动作。
    pushPadAxis(kPad, SDL_GAMEPAD_AXIS_RIGHTX, SDL_JOYSTICK_AXIS_MAX);
    pushPadAxis(kPad, SDL_GAMEPAD_AXIS_RIGHTY, SDL_JOYSTICK_AXIS_MIN);
    engine.pollEvents();
    EXPECT_TRUE(pressedKeys(engine).empty()) << names(pressedKeys(engine));
    EXPECT_TRUE(heldKeys(engine).empty()) << names(heldKeys(engine));
}

TEST_F(HeadlessPad, SwingingTheStickRoundWithoutCentringPressesTheNewWayAndLetsGoOfTheOld) {
    pushPadAxis(kPad, SDL_GAMEPAD_AXIS_LEFTX, SDL_JOYSTICK_AXIS_MAX);
    engine.pollEvents();
    ASSERT_TRUE(engine.keyPressed(Key::Right));
    // 往下扳到右下角（正好 45°）：离右不到 55°，仍是右。
    pushPadAxis(kPad, SDL_GAMEPAD_AXIS_LEFTY, SDL_JOYSTICK_AXIS_MAX);
    engine.pollEvents();
    EXPECT_EQ(heldKeys(engine), std::vector<Key>{Key::Right}) << names(heldKeys(engine));
    EXPECT_TRUE(pressedKeys(engine).empty()) << names(pressedKeys(engine));
    // 再扳到正下：换成下，右松开——一路没经过中心。
    pushPadAxis(kPad, SDL_GAMEPAD_AXIS_LEFTX, 0);
    engine.pollEvents();
    EXPECT_EQ(pressedKeys(engine), std::vector<Key>{Key::Down}) << names(pressedKeys(engine));
    EXPECT_EQ(heldKeys(engine), std::vector<Key>{Key::Down}) << names(heldKeys(engine));
    pushPadAxis(kPad, SDL_GAMEPAD_AXIS_LEFTY, 0);
    engine.pollEvents();
    EXPECT_TRUE(heldKeys(engine).empty());
}

// 整改轮 HIGH-1（docs/gamepad.md 第 4 节、第 10 节第 9 条）：SDL 一份报告里先发 LEFTX、再发 LEFTY。逐个轴事件判方向的话，
// 斜推后松手，X 先归零那一刻是「新 X + 旧 Y」的半截状态（右偏下 30° 松手 → (0, 0.5)，偏离右轴 90°），凭空判出一次「下」。
// 同一帧塞一份报告的两个轴（一种照真实次序跟 UPDATE_COMPLETE，一种不跟）：斜推之后回中，不许出任何新的刚按下，
// 也不许留一帧上 / 下的按住。(1, 0) 与右偏下 20° 两条是对照——改之前它们就不出。
TEST_F(HeadlessPad, LettingGoOfADiagonalInOneReportPressesNothingNew) {
    struct Trajectory {
        const char* name;
        float x;
        float y;
        Key way;                                         // 推下去那一刻该出的方向
        std::vector<std::pair<float, float>> letGo;      // 松手时一份一份的报告
    };
    const Trajectory kTrajectories[] = {
        {"(1, 0) 松手〔对照〕", 1.f, 0.f, Key::Right, {{0.f, 0.f}}},
        {"右偏下 20° 松手〔对照〕", 0.94f, 0.34f, Key::Right, {{0.f, 0.f}}},
        {"右偏下 30° 一份回中", 0.87f, 0.5f, Key::Right, {{0.f, 0.f}}},
        {"右偏下 30° 两份回中", 0.87f, 0.5f, Key::Right, {{0.3f, 0.15f}, {0.f, 0.f}}},
        {"右偏下 45° 两份回中", 0.7f, 0.7f, Key::Right, {{0.35f, 0.35f}, {0.f, 0.f}}},
        {"右偏上 30° 一份回中", 0.87f, -0.5f, Key::Right, {{0.f, 0.f}}},
        {"左偏下 30° 一份回中", -0.87f, 0.5f, Key::Left, {{0.f, 0.f}}},
    };
    SDL_JoystickID which = kPad;
    for (const bool complete : {true, false}) {
        for (const Trajectory& t : kTrajectories) {
            const std::string what = std::string(t.name) + (complete ? "（带 UPDATE_COMPLETE）" : "（不带）");
            ++which;   // 一条轨迹一个手柄，互不串
            pushStickReport(which, t.x, t.y, complete);
            engine.pollEvents();
            EXPECT_EQ(pressedKeys(engine), std::vector<Key>{t.way}) << what << "，推下去：" << names(pressedKeys(engine));
            engine.pollEvents();
            for (const auto& step : t.letGo) {
                pushStickReport(which, step.first, step.second, complete);
                engine.pollEvents();
                EXPECT_TRUE(pressedKeys(engine).empty())
                    << what << "，松到 (" << step.first << ", " << step.second << ")：" << names(pressedKeys(engine));
                const std::vector<Key> held = heldKeys(engine);
                EXPECT_TRUE(held.empty() || held == std::vector<Key>{t.way}) << what << "：留着 " << names(held);
            }
            EXPECT_TRUE(heldKeys(engine).empty()) << what << "，回中之后：" << names(heldKeys(engine));
        }
    }
}

// SDL 第一次设轴时把六个轴的初值各补发一遍、同一个值连发两次（第 0 节探针）：同值再来不许算新的一下。
TEST_F(HeadlessPad, TheSameValueTwiceIsStillOnePress) {
    pushPadAxis(kPad, SDL_GAMEPAD_AXIS_LEFTX, SDL_JOYSTICK_AXIS_MAX);
    engine.pollEvents();
    ASSERT_TRUE(engine.keyPressed(Key::Right));
    pushPadAxis(kPad, SDL_GAMEPAD_AXIS_LEFTX, SDL_JOYSTICK_AXIS_MAX);
    engine.pollEvents();
    EXPECT_FALSE(engine.keyPressed(Key::Right)) << "同值的轴事件又来一次：不是新的按下";
    EXPECT_TRUE(engine.keyDown(Key::Right));

    pushPadAxis(kPad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, SDL_JOYSTICK_AXIS_MAX);
    engine.pollEvents();
    ASSERT_TRUE(engine.keyPressed(Key::Skip));
    pushPadAxis(kPad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, SDL_JOYSTICK_AXIS_MAX);
    engine.pollEvents();
    EXPECT_FALSE(engine.keyPressed(Key::Skip));

    pushPadButton(kPad, SDL_GAMEPAD_BUTTON_SOUTH, true);
    engine.pollEvents();
    ASSERT_TRUE(engine.keyPressed(Key::Confirm));
    pushPadButton(kPad, SDL_GAMEPAD_BUTTON_SOUTH, true);
    engine.pollEvents();
    EXPECT_FALSE(engine.keyPressed(Key::Confirm)) << "按着的键又来一次按下：不是新的一下";
}

TEST_F(HeadlessPad, TwoPadsHoldingAKeepConfirmDownUntilBothLetGo) {
    pushPadButton(kPad, SDL_GAMEPAD_BUTTON_SOUTH, true);
    pushPadButton(kOtherPad, SDL_GAMEPAD_BUTTON_SOUTH, true);
    engine.pollEvents();
    ASSERT_TRUE(engine.keyDown(Key::Confirm));
    pushPadButton(kPad, SDL_GAMEPAD_BUTTON_SOUTH, false);
    engine.pollEvents();
    EXPECT_TRUE(engine.keyDown(Key::Confirm)) << "另一个手柄还按着";
    pushPadButton(kOtherPad, SDL_GAMEPAD_BUTTON_SOUTH, false);
    engine.pollEvents();
    EXPECT_FALSE(engine.keyDown(Key::Confirm));
}

TEST_F(HeadlessPad, ARemovedPadLetsGoOfEverythingItHeld) {
    pushPadButton(kPad, SDL_GAMEPAD_BUTTON_SOUTH, true);
    pushPadAxis(kPad, SDL_GAMEPAD_AXIS_LEFTX, SDL_JOYSTICK_AXIS_MIN);
    pushPadAxis(kPad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, SDL_JOYSTICK_AXIS_MAX);
    pushPadButton(kOtherPad, SDL_GAMEPAD_BUTTON_NORTH, true);
    engine.pollEvents();
    ASSERT_EQ(heldKeys(engine), (std::vector<Key>{Key::Left, Key::Confirm, Key::Menu, Key::Skip}))
        << names(heldKeys(engine));
    // 拔掉的那个没发松开就走了（SDL 真拔时先补发松开，这里连那一步都省掉，只剩 REMOVED）。
    pushPadRemoved(kPad);
    engine.pollEvents();
    EXPECT_EQ(heldKeys(engine), std::vector<Key>{Key::Menu}) << "它按着的立刻松开，另一个手柄按着的不动："
                                                              << names(heldKeys(engine));
    pushPadButton(kOtherPad, SDL_GAMEPAD_BUTTON_NORTH, false);
    engine.pollEvents();
    EXPECT_TRUE(heldKeys(engine).empty());
}

TEST_F(HeadlessPad, LbTheGuideAndTheStickClicksPressNothingAndLeaveTheDeviceAlone) {
    ASSERT_EQ(engine.lastInputDevice(), InputDevice::Keyboard) << "缺省是键盘";
    for (const SDL_GamepadButton button :
         {SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, SDL_GAMEPAD_BUTTON_GUIDE, SDL_GAMEPAD_BUTTON_LEFT_STICK,
          SDL_GAMEPAD_BUTTON_RIGHT_STICK, SDL_GAMEPAD_BUTTON_MISC1, SDL_GAMEPAD_BUTTON_TOUCHPAD,
          SDL_GAMEPAD_BUTTON_RIGHT_PADDLE1, SDL_GAMEPAD_BUTTON_LEFT_PADDLE2}) {
        pushPadButton(kPad, button, true);
        engine.pollEvents();
        EXPECT_TRUE(pressedKeys(engine).empty()) << static_cast<int>(button) << "：" << names(pressedKeys(engine));
        EXPECT_TRUE(heldKeys(engine).empty()) << static_cast<int>(button) << "：" << names(heldKeys(engine));
        EXPECT_EQ(engine.lastInputDevice(), InputDevice::Keyboard) << static_cast<int>(button) << " 不切设备";
        pushPadButton(kPad, button, false);
        engine.pollEvents();
    }
    // [配对] 同一个手柄上按一下配了动作的键就切：上面那个「不切」不是因为这条路根本不切。
    EXPECT_EQ(tapPad(SDL_GAMEPAD_BUTTON_SOUTH), std::vector<Key>{Key::Confirm});
    EXPECT_EQ(engine.lastInputDevice(), InputDevice::Gamepad);
}

TEST_F(HeadlessPad, TheDeviceFollowsTheLastPressThatMeantSomething) {
    EXPECT_EQ(engine.lastInputDevice(), InputDevice::Keyboard);
    tapPad(SDL_GAMEPAD_BUTTON_SOUTH);
    EXPECT_EQ(engine.lastInputDevice(), InputDevice::Gamepad) << "手柄 A";
    tapKey(SDL_SCANCODE_LALT);
    tapKey(SDL_SCANCODE_LGUI);
    tapKey(SDL_SCANCODE_MUTE);
    EXPECT_EQ(engine.lastInputDevice(), InputDevice::Gamepad) << "Alt、Win、媒体键没配动作：不切";
    EXPECT_EQ(tapKey(SDL_SCANCODE_F5), std::vector<Key>{Key::Save});
    EXPECT_EQ(engine.lastInputDevice(), InputDevice::Keyboard) << "F5（自定义键）";
    tapPad(SDL_GAMEPAD_BUTTON_LEFT_SHOULDER);
    EXPECT_EQ(engine.lastInputDevice(), InputDevice::Keyboard) << "LB 没配动作：不切";
    pushPadAxis(kPad, SDL_GAMEPAD_AXIS_LEFTY, SDL_JOYSTICK_AXIS_MIN);
    engine.pollEvents();
    EXPECT_EQ(engine.lastInputDevice(), InputDevice::Gamepad) << "摇杆推到位";
    pushPadAxis(kPad, SDL_GAMEPAD_AXIS_LEFTY, 0);
    engine.pollEvents();
    EXPECT_EQ(tapKey(SDL_SCANCODE_RETURN), std::vector<Key>{Key::Confirm});
    EXPECT_EQ(engine.lastInputDevice(), InputDevice::Keyboard) << "Enter（固定键）";
    pushPadAxis(kPad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, SDL_JOYSTICK_AXIS_MAX);
    engine.pollEvents();
    EXPECT_EQ(engine.lastInputDevice(), InputDevice::Gamepad) << "扳机过半";
}

// 第 7 节：抓键（等玩家在键盘上按新键）时，手柄 B = 作罢（交出 kEscapeCode），别的手柄键一律不理、抓键继续；
// 手柄键按下的物理状态照记（松开时对得上），只是不出逻辑键。
TEST_F(HeadlessPad, WhileCapturingPadButtonsFireNothingBCancelsAndTheRestAreIgnored) {
    engine.beginKeyCapture();
    pushPadButton(kPad, SDL_GAMEPAD_BUTTON_SOUTH, true);
    engine.pollEvents();
    EXPECT_TRUE(pressedKeys(engine).empty()) << names(pressedKeys(engine));
    EXPECT_TRUE(engine.capturingKey()) << "A 不是被抓的那一下";
    EXPECT_FALSE(engine.takeCapturedKey().has_value());
    pushPadAxis(kPad, SDL_GAMEPAD_AXIS_LEFTX, SDL_JOYSTICK_AXIS_MAX);
    pushPadButton(kPad, SDL_GAMEPAD_BUTTON_NORTH, true);
    pushPadAxis(kPad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, SDL_JOYSTICK_AXIS_MAX);
    engine.pollEvents();
    EXPECT_TRUE(pressedKeys(engine).empty()) << names(pressedKeys(engine));
    EXPECT_TRUE(engine.capturingKey()) << "摇杆、Y、RT 也不理，抓键继续";
    EXPECT_EQ(engine.lastInputDevice(), InputDevice::Keyboard) << "不理的键不切设备";
    pushPadAxis(kPad, SDL_GAMEPAD_AXIS_LEFTX, 0);
    pushPadButton(kPad, SDL_GAMEPAD_BUTTON_NORTH, false);
    pushPadAxis(kPad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, 0);
    engine.pollEvents();

    // B：作罢。交出的是 Esc 的扫描码，面板照「作罢」那条路走；也不出 Cancel。
    pushPadButton(kPad, SDL_GAMEPAD_BUTTON_EAST, true);
    engine.pollEvents();
    EXPECT_FALSE(engine.capturingKey());
    EXPECT_EQ(engine.takeCapturedKey(), std::optional<fanren::engine::ScanCode>(Engine::kEscapeCode));
    EXPECT_FALSE(engine.keyPressed(Key::Cancel));
    EXPECT_EQ(engine.lastInputDevice(), InputDevice::Gamepad) << "手柄 B 作罢：设备是手柄";
    // A 在抓键期间按下、一直没松：物理状态照记，抓键完了它算按着，但不是「刚按下」。
    engine.pollEvents();
    EXPECT_TRUE(engine.keyDown(Key::Confirm)) << "A 还按着";
    EXPECT_FALSE(engine.keyPressed(Key::Confirm));
    pushPadButton(kPad, SDL_GAMEPAD_BUTTON_SOUTH, false);
    pushPadButton(kPad, SDL_GAMEPAD_BUTTON_EAST, false);
    engine.pollEvents();
    EXPECT_TRUE(heldKeys(engine).empty()) << "松开时对得上：" << names(heldKeys(engine));

    // 抓完回到常态：手柄 A 又是确认。
    EXPECT_EQ(tapPad(SDL_GAMEPAD_BUTTON_SOUTH), std::vector<Key>{Key::Confirm});
    // 键盘上被抓走的那一下：设备回键盘。
    engine.beginKeyCapture();
    pushKey(SDL_SCANCODE_J, true);
    engine.pollEvents();
    EXPECT_EQ(engine.takeCapturedKey(), std::optional<fanren::engine::ScanCode>(SDL_SCANCODE_J));
    EXPECT_EQ(engine.lastInputDevice(), InputDevice::Keyboard);
    pushKey(SDL_SCANCODE_J, false);
    engine.pollEvents();
}

// 收尾轮 LOW-A：「一帧末尾补判摇杆」排在抓键那道闸之前，这个次序决定行为。抓键期间把摇杆推到位：物理状态照记、不出逻辑键、
// 不切设备；再用键盘键结束抓键、摇杆还推着——结束那一帧与之后 320ms 都不许出「右」的刚按下、不许连发，设备仍是键盘。
// 两种报告各走一遍：不带 UPDATE_COMPLETE（SDL_PushEvent 塞的事件就这样）时只有一帧末尾那一次补判，那一次若挪到闸后，
// 抓键期间推的那一下就攒到抓键结束那一帧才判，凭空出一次「右」、设备切成手柄、按住还连发（变异验证实测红过）；
// 带 UPDATE_COMPLETE（SDL 的真实次序）时报告末尾当场就判了，与那个次序无关，一并钉住。
TEST_F(HeadlessPad, AStickPushedDuringACaptureDoesNotFireWhenAKeyEndsIt) {
    SDL_JoystickID which = kPad;
    for (const bool complete : {false, true}) {
        const std::string what = complete ? "带 UPDATE_COMPLETE" : "不带 UPDATE_COMPLETE";
        ++which;
        tapKey(SDL_SCANCODE_RETURN);   // 两遍互不牵连：每一遍都从「设备是键盘」起
        ASSERT_EQ(engine.lastInputDevice(), InputDevice::Keyboard) << what << "：先验";
        engine.beginKeyCapture();
        pushStickReport(which, 1.f, 0.f, complete);
        engine.pollEvents();
        EXPECT_TRUE(pressedKeys(engine).empty()) << what << "，抓键期间推摇杆：" << names(pressedKeys(engine));
        EXPECT_TRUE(engine.capturingKey()) << what << "：摇杆不是被抓的那一下";
        EXPECT_EQ(engine.lastInputDevice(), InputDevice::Keyboard) << what << "：抓键期间推摇杆不切设备";

        // 用键盘键结束抓键，摇杆还推着。
        pushKey(SDL_SCANCODE_J, true);
        engine.pollEvents();
        EXPECT_EQ(engine.takeCapturedKey(), std::optional<fanren::engine::ScanCode>(SDL_SCANCODE_J)) << what;
        EXPECT_FALSE(engine.keyPressed(Key::Right)) << what << "：结束抓键那一帧不许出「右」";
        EXPECT_TRUE(engine.keyDown(Key::Right)) << what << "：物理状态照记——摇杆还推着";
        EXPECT_EQ(engine.lastInputDevice(), InputDevice::Keyboard) << what << "：结束抓键的是键盘";
        int presses = 0;
        const std::uint64_t until = SDL_GetTicks() + 320;
        while (SDL_GetTicks() < until) {
            engine.pollEvents();
            if (engine.keyPressed(Key::Right)) ++presses;
            SDL_Delay(5);
        }
        EXPECT_EQ(presses, 0) << what << "：之后 320ms 不出、不连发";
        EXPECT_EQ(engine.lastInputDevice(), InputDevice::Keyboard) << what;

        pushKey(SDL_SCANCODE_J, false);
        pushStickReport(which, 0.f, 0.f, complete);
        engine.pollEvents();
        EXPECT_TRUE(heldKeys(engine).empty()) << what << "，松开：" << names(heldKeys(engine));
    }
}

TEST_F(HeadlessPad, TheRumbleSwitchIsStoredWithoutAWindowAndStartsOn) {
    EXPECT_TRUE(engine.rumbleEnabled()) << "缺省开（docs/gamepad.md 第 8 节）";
    engine.setRumbleEnabled(false);
    EXPECT_FALSE(engine.rumbleEnabled());
    engine.setRumbleEnabled(true);
    EXPECT_TRUE(engine.rumbleEnabled());
    // 没有手柄、设备是键盘：震一下是安全的空操作。
    engine.rumble(1.f, 1.f, 5000);
    engine.rumble(-1.f, std::nanf(""), -5);
}

// ---------------------------------------------------------------------------
// 3. 虚拟手柄走 SDL 全链路（第 4、8 节）
// ---------------------------------------------------------------------------

TEST_F(VirtualPad, AnAttachedPadIsOpenedAndItsButtonsStickAndTriggerReachTheKeys) {
    startGamepads();
    attach();
    engine.pollEvents();   // GAMEPAD_ADDED → SDL_OpenGamepad
    EXPECT_EQ(engine.gamepadCount(), 1);

    setButton(SDL_GAMEPAD_BUTTON_SOUTH, true);
    engine.pollEvents();
    EXPECT_EQ(pressedKeys(engine), std::vector<Key>{Key::Confirm}) << names(pressedKeys(engine));
    EXPECT_EQ(engine.lastInputDevice(), InputDevice::Gamepad);
    setButton(SDL_GAMEPAD_BUTTON_SOUTH, false);
    engine.pollEvents();
    EXPECT_FALSE(engine.keyDown(Key::Confirm));

    // 第一次设轴：SDL 把六个轴的初值各补发两遍（扳机的初值是 0），只该出一个「左」。
    setAxis(SDL_GAMEPAD_AXIS_LEFTX, SDL_JOYSTICK_AXIS_MIN);
    engine.pollEvents();
    EXPECT_EQ(pressedKeys(engine), std::vector<Key>{Key::Left}) << names(pressedKeys(engine));
    EXPECT_EQ(heldKeys(engine), std::vector<Key>{Key::Left}) << "扳机初值 0：不按下；" << names(heldKeys(engine));
    setAxis(SDL_GAMEPAD_AXIS_LEFTX, 0);
    engine.pollEvents();
    EXPECT_FALSE(engine.keyDown(Key::Left));

    setAxis(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, SDL_JOYSTICK_AXIS_MAX);
    engine.pollEvents();
    EXPECT_TRUE(engine.keyPressed(Key::Skip));
    EXPECT_TRUE(engine.keyDown(Key::Skip)) << "RT 扣住 = 按住快进";
    setAxis(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, SDL_JOYSTICK_AXIS_MIN);   // 手柄上读到 0
    engine.pollEvents();
    EXPECT_FALSE(engine.keyDown(Key::Skip));
}

TEST_F(VirtualPad, UnpluggingLetsGoOfEverythingAndTheDeviceFallsBackToTheKeyboard) {
    startGamepads();
    attach();
    engine.pollEvents();
    ASSERT_EQ(engine.gamepadCount(), 1);
    setButton(SDL_GAMEPAD_BUTTON_DPAD_LEFT, true);
    setAxis(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, SDL_JOYSTICK_AXIS_MAX);
    engine.pollEvents();
    ASSERT_EQ(heldKeys(engine), (std::vector<Key>{Key::Left, Key::Skip})) << names(heldKeys(engine));
    ASSERT_EQ(engine.lastInputDevice(), InputDevice::Gamepad);

    detach();
    engine.pollEvents();
    EXPECT_EQ(engine.gamepadCount(), 0);
    EXPECT_TRUE(heldKeys(engine).empty()) << "拔掉时按着的方向不许卡住：" << names(heldKeys(engine));
    EXPECT_EQ(engine.lastInputDevice(), InputDevice::Keyboard) << "最后一个手柄拔掉：设备回键盘";
}

// 整改轮 HIGH-1，SDL 真链路：一次更新推到右偏下 30°、一次更新松手回中。SDL 先发 LEFTX = 0、再发 LEFTY = 0、最后 UPDATE_COMPLETE。
TEST_F(VirtualPad, LettingGoOfADiagonalInOneUpdatePressesNothingNew) {
    startGamepads();
    attach();
    engine.pollEvents();
    ASSERT_EQ(engine.gamepadCount(), 1);
    setAxis(SDL_GAMEPAD_AXIS_LEFTX, 0);   // 第一次设轴 SDL 补发六个轴的初值：先收掉
    engine.pollEvents();
    engine.pollEvents();
    setAxis(SDL_GAMEPAD_AXIS_LEFTX, axisValue(0.87f));
    setAxis(SDL_GAMEPAD_AXIS_LEFTY, axisValue(0.5f));
    engine.pollEvents();
    EXPECT_EQ(pressedKeys(engine), std::vector<Key>{Key::Right}) << "推下去只出「右」：" << names(pressedKeys(engine));
    engine.pollEvents();
    setAxis(SDL_GAMEPAD_AXIS_LEFTX, 0);
    setAxis(SDL_GAMEPAD_AXIS_LEFTY, 0);
    engine.pollEvents();
    EXPECT_TRUE(pressedKeys(engine).empty()) << "松手回中：" << names(pressedKeys(engine));
    EXPECT_TRUE(heldKeys(engine).empty()) << names(heldKeys(engine));
}

// 整改轮 HIGH-1：斜推着拔线。SDL 拔线时先逐轴补发回中、再报 REMOVED——补发的那几个轴值不许判出方向。
TEST_F(VirtualPad, UnpluggingWhileHoldingADiagonalPressesNothing) {
    startGamepads();
    attach();
    engine.pollEvents();
    ASSERT_EQ(engine.gamepadCount(), 1);
    setAxis(SDL_GAMEPAD_AXIS_LEFTX, axisValue(0.87f));
    setAxis(SDL_GAMEPAD_AXIS_LEFTY, axisValue(0.5f));
    engine.pollEvents();
    engine.pollEvents();
    ASSERT_EQ(heldKeys(engine), std::vector<Key>{Key::Right}) << names(heldKeys(engine));
    detach();
    engine.pollEvents();
    EXPECT_TRUE(pressedKeys(engine).empty()) << "拔线那一帧：" << names(pressedKeys(engine));
    EXPECT_TRUE(heldKeys(engine).empty()) << names(heldKeys(engine));
    EXPECT_EQ(engine.gamepadCount(), 0);
}

// 游戏启动前就插着的手柄：只开 SDL_INIT_JOYSTICK 时挂上，再开手柄子系统——SDL 补发 GAMEPAD_ADDED，引擎照样打开它，
// 不另外枚举（两处都开会把同一个手柄开两次）。
TEST_F(VirtualPad, APadPluggedInBeforeTheSubsystemStartsIsStillOpened) {
    ASSERT_TRUE(SDL_InitSubSystem(SDL_INIT_JOYSTICK)) << SDL_GetError();
    attach();
    engine.pollEvents();
    EXPECT_EQ(engine.gamepadCount(), 0) << "手柄子系统还没开：没有 GAMEPAD_ADDED";
    startGamepads();
    engine.pollEvents();
    EXPECT_EQ(engine.gamepadCount(), 1) << "补发的 ADDED 到了，打开了";
    setButton(SDL_GAMEPAD_BUTTON_SOUTH, true);
    engine.pollEvents();
    EXPECT_TRUE(engine.keyPressed(Key::Confirm)) << "真的开着：按键到得了";
    setButton(SDL_GAMEPAD_BUTTON_SOUTH, false);
    engine.pollEvents();
    EXPECT_EQ(engine.gamepadCount(), 1) << "没有开第二次";
}

// 第 8 节：rumble 是唯一的闸——开关开着、最后按的是这个手柄才震到它；开关关、最后按的是键盘都收不到。
// SDL 对「与正在震的强度相同」的请求不再转给手柄（探针实测），所以每一次阳性都换一个强度。
TEST_F(VirtualPad, RumbleReachesThePadOnlyWhenItWasLastUsedAndTheSwitchIsOn) {
    startGamepads();
    attach();
    engine.pollEvents();
    ASSERT_EQ(engine.gamepadCount(), 1);

    engine.rumble(0.8f, 0.6f, 280);
    EXPECT_TRUE(strongRumbles().empty()) << "还没按过手柄（设备是键盘）：不震";

    setButton(SDL_GAMEPAD_BUTTON_SOUTH, true);
    engine.pollEvents();
    setButton(SDL_GAMEPAD_BUTTON_SOUTH, false);
    engine.pollEvents();
    ASSERT_EQ(engine.lastInputDevice(), InputDevice::Gamepad);
    engine.rumble(0.8f, 0.6f, 280);
    ASSERT_EQ(strongRumbles().size(), 1u) << "按过这个手柄、开关开着：震到了";
    EXPECT_NEAR(strongRumbles()[0].first, 52428, 1) << "0.8 × 0xFFFF";
    EXPECT_NEAR(strongRumbles()[0].second, 39321, 1) << "0.6 × 0xFFFF";

    // 按一下键盘：设备换回键盘，不震。
    pushKey(SDL_SCANCODE_RETURN, true);
    engine.pollEvents();
    pushKey(SDL_SCANCODE_RETURN, false);
    engine.pollEvents();
    engine.rumble(0.5f, 0.35f, 180);
    EXPECT_EQ(strongRumbles().size(), 1u) << "最后按的是键盘：收不到";

    // 再按一下手柄，但开关关了：不震。
    setButton(SDL_GAMEPAD_BUTTON_SOUTH, true);
    engine.pollEvents();
    setButton(SDL_GAMEPAD_BUTTON_SOUTH, false);
    engine.pollEvents();
    engine.setRumbleEnabled(false);
    engine.rumble(0.5f, 0.35f, 180);
    EXPECT_EQ(strongRumbles().size(), 1u) << "开关关着：收不到";
    // [配对] 开关一开，同样一下就到了。
    engine.setRumbleEnabled(true);
    engine.rumble(0.5f, 0.35f, 180);
    ASSERT_EQ(strongRumbles().size(), 2u);
    EXPECT_NEAR(strongRumbles()[1].first, 32768, 1) << "0.5 × 0xFFFF";
    EXPECT_NEAR(strongRumbles()[1].second, 22937, 1) << "0.35 × 0xFFFF";
}

// ---------------------------------------------------------------------------
// 4. 连发：只有方向键（第 5 节）
// ---------------------------------------------------------------------------

TEST_F(KeyRepeat, OnTheKeyboardOnlyTheFourArrowsRepeat) {
    const std::pair<SDL_Scancode, Key> kOnce[] = {
        {SDL_SCANCODE_RETURN, Key::Confirm}, {SDL_SCANCODE_ESCAPE, Key::Cancel}, {SDL_SCANCODE_TAB, Key::Menu},
        {SDL_SCANCODE_LCTRL, Key::Skip},     {SDL_SCANCODE_F5, Key::Save},       {SDL_SCANCODE_E, Key::Action},
    };
    for (const auto& entry : kOnce) {
        const SDL_Scancode code = entry.first;
        const int presses =
            holdAndCount(entry.second, 400, [code] { pushKey(code, true); }, [code] { pushKey(code, false); });
        EXPECT_EQ(presses, 1) << Engine::keyId(entry.second) << "：按住 400ms 只出一次";
    }
    const std::pair<SDL_Scancode, Key> kRepeat[] = {
        {SDL_SCANCODE_UP, Key::Up}, {SDL_SCANCODE_DOWN, Key::Down},
        {SDL_SCANCODE_LEFT, Key::Left}, {SDL_SCANCODE_RIGHT, Key::Right},
    };
    for (const auto& entry : kRepeat) {
        const SDL_Scancode code = entry.first;
        const int presses =
            holdAndCount(entry.second, 400, [code] { pushKey(code, true); }, [code] { pushKey(code, false); });
        EXPECT_GE(presses, 3) << Engine::keyId(entry.second) << "：按住 400ms 照旧连发（250ms 起每 60ms 一次）";
    }
}

TEST_F(KeyRepeat, OnAPadOnlyTheFourDirectionsRepeat) {
    const std::pair<SDL_GamepadButton, Key> kOnce[] = {
        {SDL_GAMEPAD_BUTTON_SOUTH, Key::Confirm}, {SDL_GAMEPAD_BUTTON_EAST, Key::Cancel},
        {SDL_GAMEPAD_BUTTON_NORTH, Key::Menu},    {SDL_GAMEPAD_BUTTON_BACK, Key::Save},
        {SDL_GAMEPAD_BUTTON_WEST, Key::Action},
    };
    for (const auto& entry : kOnce) {
        const SDL_GamepadButton button = entry.first;
        const int presses = holdAndCount(entry.second, 400, [button] { pushPadButton(kPad, button, true); },
                                         [button] { pushPadButton(kPad, button, false); });
        EXPECT_EQ(presses, 1) << Engine::keyId(entry.second) << "：手柄键按住 400ms 只出一次";
    }
    const int skip = holdAndCount(
        Key::Skip, 400, [] { pushPadAxis(kPad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, SDL_JOYSTICK_AXIS_MAX); },
        [] { pushPadAxis(kPad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, 0); });
    EXPECT_EQ(skip, 1) << "RT 扣住 400ms 只出一次（快进靠 keyDown）";

    const std::pair<SDL_GamepadButton, Key> kRepeat[] = {
        {SDL_GAMEPAD_BUTTON_DPAD_UP, Key::Up}, {SDL_GAMEPAD_BUTTON_DPAD_DOWN, Key::Down},
        {SDL_GAMEPAD_BUTTON_DPAD_LEFT, Key::Left}, {SDL_GAMEPAD_BUTTON_DPAD_RIGHT, Key::Right},
    };
    for (const auto& entry : kRepeat) {
        const SDL_GamepadButton button = entry.first;
        const int presses = holdAndCount(entry.second, 400, [button] { pushPadButton(kPad, button, true); },
                                         [button] { pushPadButton(kPad, button, false); });
        EXPECT_GE(presses, 3) << Engine::keyId(entry.second) << "：十字键按住 400ms 照旧连发";
    }
    const int stickUp = holdAndCount(
        Key::Up, 400, [] { pushPadAxis(kPad, SDL_GAMEPAD_AXIS_LEFTY, SDL_JOYSTICK_AXIS_MIN); },
        [] { pushPadAxis(kPad, SDL_GAMEPAD_AXIS_LEFTY, 0); });
    EXPECT_GE(stickUp, 3) << "摇杆推住 400ms 照旧连发";
}

// RED 先行（第 5 节）：按住 Tab / Esc / 手柄 Y / 手柄 B 一秒，世界层的主菜单只开一次、一直开着。
// 改之前：世界层见「刚按下」开菜单，菜单见「刚按下」合上，而这几个键按住 250ms 起每 60ms 连发一次——菜单每 60ms 开合一次。
TEST_F(PadWiring, HoldingTabForASecondOpensTheMainMenuOnceAndItStaysOpen) {
    standInTheWorld();
    const Held held = holdInTheWorldForASecond([] { pushKey(SDL_SCANCODE_TAB, true); },
                                               [] { pushKey(SDL_SCANCODE_TAB, false); });
    ASSERT_GE(held.frames, 30) << "先验：真的逐帧走了一秒";
    EXPECT_EQ(held.opened, 1) << "只开一次";
    EXPECT_EQ(held.closed, 0) << "开了就一直开着";
    EXPECT_TRUE(held.openAtEnd);
}

TEST_F(PadWiring, HoldingEscForASecondOpensTheMainMenuOnceAndItStaysOpen) {
    standInTheWorld();
    const Held held = holdInTheWorldForASecond([] { pushKey(SDL_SCANCODE_ESCAPE, true); },
                                               [] { pushKey(SDL_SCANCODE_ESCAPE, false); });
    ASSERT_GE(held.frames, 30);
    EXPECT_EQ(held.opened, 1);
    EXPECT_EQ(held.closed, 0);
    EXPECT_TRUE(held.openAtEnd);
}

TEST_F(PadWiring, HoldingPadYForASecondOpensTheMainMenuOnceAndItStaysOpen) {
    standInTheWorld();
    const Held held = holdInTheWorldForASecond([] { pushPadButton(kPad, SDL_GAMEPAD_BUTTON_NORTH, true); },
                                               [] { pushPadButton(kPad, SDL_GAMEPAD_BUTTON_NORTH, false); });
    ASSERT_GE(held.frames, 30);
    EXPECT_EQ(held.opened, 1);
    EXPECT_EQ(held.closed, 0);
    EXPECT_TRUE(held.openAtEnd);
}

TEST_F(PadWiring, HoldingPadBForASecondOpensTheMainMenuOnceAndItStaysOpen) {
    standInTheWorld();
    const Held held = holdInTheWorldForASecond([] { pushPadButton(kPad, SDL_GAMEPAD_BUTTON_EAST, true); },
                                               [] { pushPadButton(kPad, SDL_GAMEPAD_BUTTON_EAST, false); });
    ASSERT_GE(held.frames, 30);
    EXPECT_EQ(held.opened, 1);
    EXPECT_EQ(held.closed, 0);
    EXPECT_TRUE(held.openAtEnd);
}

// 改键面板最右那一列（第 7 节）：每行写那个动作的手柄键显示名，多个用「 / 」隔开。期望照抄契约原文。
TEST(KeyPanelPadColumn, EachRowNamesThatActionsPadButtonsSeparatedBySlashes) {
    const std::pair<Key, std::string> kColumn[] = {
        {Key::Up, "十字↑ / 摇杆↑"},    {Key::Down, "十字↓ / 摇杆↓"}, {Key::Left, "十字← / 摇杆←"},
        {Key::Right, "十字→ / 摇杆→"}, {Key::Confirm, "A"},           {Key::Cancel, "B"},
        {Key::Menu, "Y / ≡"},          {Key::Skip, "RT / RB"},        {Key::Save, "⧉"},
        {Key::Action, "X"},
    };
    for (const auto& entry : kColumn) {
        EXPECT_EQ(fanren::game::KeyConfigScene::padKeysText(entry.first), entry.second) << Engine::keyId(entry.first);
    }
}

// ---------------------------------------------------------------------------
// 5. 接线：真 Application，从玩家碰得到的入口进去（第 6、7、8 节）
// ---------------------------------------------------------------------------

TEST_F(PadWiring, PadYOpensTheMainMenuAndBClosesItStartOpensItToo) {
    standInTheWorld();
    pressPad(SDL_GAMEPAD_BUTTON_NORTH);
    EXPECT_EQ(top(), "Menu") << "世界层手柄 Y → 主菜单";
    pressPad(SDL_GAMEPAD_BUTTON_EAST);
    EXPECT_EQ(top(), "World") << "主菜单里手柄 B → 合上";
    pressPad(SDL_GAMEPAD_BUTTON_START);
    EXPECT_EQ(top(), "Menu") << "≡（Start）也开主菜单";
    pressPad(SDL_GAMEPAD_BUTTON_NORTH);
    EXPECT_EQ(top(), "World") << "Y 在主菜单里合上它（与 Tab 同一个口径）";
}

TEST_F(PadWiring, PadAPagesThroughTheDialogue) {
    standInTheWorld();
    app_.pushScene(std::make_unique<fanren::game::DialogueScene>(std::string{}, "第一句。"));
    app_.pushScene(std::make_unique<fanren::game::DialogueScene>(std::string{}, "第二句。"));
    tick();
    ASSERT_EQ(top(), "Dialogue");
    pressPad(SDL_GAMEPAD_BUTTON_SOUTH);
    EXPECT_EQ(top(), "Dialogue") << "翻过一页，还有一页";
    pressPad(SDL_GAMEPAD_BUTTON_SOUTH);
    EXPECT_EQ(top(), "World") << "两页都翻完了";
}

// 第 6 节：11 条提示在键盘模式下与今天逐字节相同；按过手柄之后等于那张表「默认展开成」一列；按一下键盘又换回来。
// 期望串写死在这里（照抄契约），不从 ui.json 读。
TEST_F(PadWiring, PromptsFollowTheDeviceThatWasPressedLast) {
    const std::pair<const char*, const char*> kKeyboard[] = {
        {"ui.title.keys", "↑↓ 选择　Enter 确认"},
        {"ui.menu.keys", "Tab 关闭　Esc 返回"},
        {"ui.dialogue.keys", "Tab 回看　长按 Ctrl 快进"},
        {"ui.path.keys", "确认 选定　Esc 作罢"},
        {"ui.settings.hint", "↑↓ 选择　←→ 调整　Esc 返回"},
        {"ui.keys.hint", "↑↓ 选择　←→ 选格　Enter 改键　Esc 返回"},
        {"ui.keys.capture", "按下新键（Esc 作罢，Backspace 清空）"},
        {"ui.keys.desc", "选好一格按 Enter，再按下要配的键。方向键、Enter、Esc 是固定键，始终可用。"},
        {"ui.keys.desc.restore", "全部动作回到默认键位。按 Enter 执行。"},
        {"ui.battle.hint.menu", "↑↓ 选择 · ←→ 蓄劲 · 回车 确定 · Esc / Tab 返回 · 按住 Ctrl 加速"},
        {"ui.battle.hint.watch", "按住 Ctrl 加速"},
    };
    const std::pair<const char*, const char*> kPadText[] = {
        {"ui.title.keys", "↑↓ 选择　A 确认"},
        {"ui.menu.keys", "Y 关闭　B 返回"},
        {"ui.dialogue.keys", "Y 回看　长按 RT 快进"},
        {"ui.path.keys", "A 选定　B 作罢"},
        {"ui.settings.hint", "↑↓ 选择　←→ 调整　B 返回"},
        {"ui.keys.hint", "↑↓ 选择　←→ 选格　A 改键　B 返回"},
        {"ui.keys.capture", "在键盘上按下新键（B 作罢）"},
        {"ui.keys.desc", "这里改的是键盘键位，手柄键位固定（见最右一列）。选好一格按 A，再在键盘上按下要配的键。"},
        {"ui.keys.desc.restore", "全部动作回到默认键位。按 A 执行。"},
        {"ui.battle.hint.menu", "↑↓ 选择 · ←→ 蓄劲 · A 确定 · B / Y 返回 · 按住 RT 加速"},
        {"ui.battle.hint.watch", "按住 RT 加速"},
    };
    const auto expectAll = [this](const auto& table, const char* when) {
        for (const auto& entry : table) EXPECT_EQ(app_.text(entry.first), entry.second) << when << "：" << entry.first;
    };
    ASSERT_EQ(app_.engine().lastInputDevice(), InputDevice::Keyboard);
    expectAll(kKeyboard, "键盘模式（缺省）");
    pressPad(SDL_GAMEPAD_BUTTON_LEFT_SHOULDER);
    expectAll(kKeyboard, "按 LB（没配动作）不切");
    pressPad(SDL_GAMEPAD_BUTTON_DPAD_DOWN);
    expectAll(kPadText, "按过手柄之后");
    pressKey(SDL_SCANCODE_LALT);
    expectAll(kPadText, "按 Alt（没配动作）不切回");
    pressKey(SDL_SCANCODE_DOWN);
    expectAll(kKeyboard, "按一下键盘就切回");
    // 不在那 11 条里的文案没有 .pad 版：手柄模式下照旧。
    pressPad(SDL_GAMEPAD_BUTTON_SOUTH);
    EXPECT_EQ(app_.text("ui.ending.continue_hint"), "按确认键继续");
    EXPECT_EQ(app_.text("ui.settings.desc.fullscreen"), "窗口还是全屏。走路、打仗时也可以随时按 Alt+Enter 切换。");
}

// 整改轮 HIGH-1，玩家碰得到的入口：设置面板光标在第 0 行，摇杆往右偏下 30° 推一下、一份报告松手——光标不许动
// （整改前审查方实测跳到第 1 行：松手时那半截状态判出了一次「下」）。
TEST_F(PadWiring, ADiagonalStickPushAndLetGoLeavesTheSettingsCursorWhereItWas) {
    SettingsScene* panel = openPanel();
    ASSERT_EQ(top(), "Settings");
    ASSERT_EQ(panel->selection(), SettingsScene::kBgmVolume) << "先验：光标一进来停在第 0 行";
    pushStickReport(kPad, 0.87f, 0.5f, true);
    tick();
    tick(5);
    pushStickReport(kPad, 0.f, 0.f, true);
    tick();
    EXPECT_EQ(panel->selection(), SettingsScene::kBgmVolume) << "松手不许凭空出一次「下」";
    EXPECT_EQ(app_.settings().bgmVolume, 10) << "推的是 →：音乐音量已在顶上，不动";
    // [配对] 同一根摇杆往下推一下再松手：光标确实下移一行——上面那个「没动」不是因为摇杆在这里根本不管用。
    pushStickReport(kPad, 0.f, 1.f, true);
    tick();
    pushStickReport(kPad, 0.f, 0.f, true);
    tick();
    EXPECT_EQ(panel->selection(), SettingsScene::kSfxVolume);
}

// 第 7 节：只拿手柄的玩家在改键面板里按 A 进了「按下新键」，要出得来——手柄 B 作罢；别的手柄键不理、抓键继续。
TEST_F(PadWiring, InTheKeyPanelPadAStartsACaptureWithThePadNoteAndBCancelsIt) {
    openPanel(/*openKeyConfig=*/true);
    ASSERT_EQ(top(), "KeyConfig") << "截图口 settings:keys 走的就是这条路";
    const auto* keys = dynamic_cast<const fanren::game::KeyConfigScene*>(app_.topScene());
    ASSERT_NE(keys, nullptr);
    pressPad(SDL_GAMEPAD_BUTTON_DPAD_DOWN);
    ASSERT_EQ(keys->row(), static_cast<int>(Key::Down));
    pressPad(SDL_GAMEPAD_BUTTON_SOUTH);
    ASSERT_TRUE(keys->capturing()) << "手柄 A 在键位格上 → 进抓键";
    EXPECT_EQ(keys->note(), "在键盘上按下新键（B 作罢）") << "说明行是 .pad 版";
    pressPad(SDL_GAMEPAD_BUTTON_NORTH);
    pressPad(SDL_GAMEPAD_BUTTON_DPAD_UP);
    EXPECT_TRUE(keys->capturing()) << "Y、十字键不理，抓键继续";
    EXPECT_EQ(keys->row(), static_cast<int>(Key::Down)) << "十字键↑ 不是「光标上移」";
    pressPad(SDL_GAMEPAD_BUTTON_EAST);
    EXPECT_FALSE(keys->capturing());
    EXPECT_EQ(keys->note(), "作罢，没有改动。") << "手柄 B = 作罢，照「作罢」那条路走";
    EXPECT_EQ(top(), "KeyConfig") << "作罢不是关面板";
    EXPECT_TRUE(app_.settings().keys.empty()) << "键位一格没动";
    EXPECT_FALSE(app_.engine().capturingKey());

    // [配对] 用键盘进抓键：说明行是键盘版。
    pressKey(SDL_SCANCODE_RETURN);
    ASSERT_TRUE(keys->capturing());
    EXPECT_EQ(keys->note(), "按下新键（Esc 作罢，Backspace 清空）");
    pressKey(SDL_SCANCODE_ESCAPE);
    EXPECT_FALSE(keys->capturing());
    // 抓键之外手柄 B 就是「取消」：关面板。
    pressPad(SDL_GAMEPAD_BUTTON_EAST);
    EXPECT_EQ(top(), "Settings");
}

// 第 8 节：设置面板第 8 行「手柄震动」←→ → settings().padRumble 与 engine.rumbleEnabled() 同变；关面板写盘、读回相等；
// 「恢复默认」连它一起回到开。
TEST_F(PadWiring, ThePadRumbleRowFlipsTheSettingAndTheEngineAndIsSavedOnClose) {
    const std::string path = fileIn("rumble_settings.json");
    ASSERT_TRUE(app_.enableSettingsFile(path).ok);
    SettingsScene* panel = openPanel();
    ASSERT_EQ(top(), "Settings");
    for (int i = 0; i < 8; ++i) pressPad(SDL_GAMEPAD_BUTTON_DPAD_DOWN);
    ASSERT_EQ(panel->selection(), 8) << "第 8 行";
    EXPECT_EQ(SettingsScene::rowLabel(app_.data(), panel->selection()), "手柄震动");
    ASSERT_TRUE(app_.settings().padRumble) << "缺省开";
    ASSERT_TRUE(app_.engine().rumbleEnabled());
    EXPECT_EQ(SettingsScene::valueText(app_.data(), app_.settings(), 8), "开");

    pressPad(SDL_GAMEPAD_BUTTON_DPAD_LEFT);
    EXPECT_TRUE(app_.settings().padRumble) << "← 取「开」：已经是开，到头不动";
    pressPad(SDL_GAMEPAD_BUTTON_DPAD_RIGHT);
    EXPECT_FALSE(app_.settings().padRumble) << "→ 取「关」";
    EXPECT_FALSE(app_.engine().rumbleEnabled()) << "引擎同一帧就变";
    EXPECT_EQ(SettingsScene::valueText(app_.data(), app_.settings(), 8), "关");
    pressPad(SDL_GAMEPAD_BUTTON_DPAD_RIGHT);
    EXPECT_FALSE(app_.settings().padRumble) << "到头就停，不回绕";
    pressKey(SDL_SCANCODE_LEFT);
    EXPECT_TRUE(app_.settings().padRumble);
    EXPECT_TRUE(app_.engine().rumbleEnabled());
    pressKey(SDL_SCANCODE_RIGHT);
    ASSERT_FALSE(app_.settings().padRumble);
    EXPECT_FALSE(fs::exists(path)) << "调的时候不写盘";

    pressPad(SDL_GAMEPAD_BUTTON_EAST);
    EXPECT_NE(top(), "Settings") << "手柄 B 关面板";
    ASSERT_TRUE(fs::exists(path)) << "有改动：关面板写到 enable 过的那份";
    const fanren::io::SettingsRead read = fanren::io::loadSettings(path);
    EXPECT_TRUE(read.error.empty()) << read.error;
    EXPECT_TRUE(read.warnings.empty());
    EXPECT_EQ(read.settings, app_.settings());
    EXPECT_FALSE(read.settings.padRumble);

    // 恢复默认：连它一起回到开。
    panel = openPanel();
    for (int i = 0; i < SettingsScene::kRestore; ++i) pressKey(SDL_SCANCODE_DOWN);
    ASSERT_EQ(panel->selection(), SettingsScene::kRestore);
    pressPad(SDL_GAMEPAD_BUTTON_SOUTH);
    EXPECT_TRUE(app_.settings().padRumble);
    EXPECT_TRUE(app_.engine().rumbleEnabled());
    EXPECT_EQ(app_.settings(), fanren::io::Settings{});
}

// 第 8 节：战斗里只在两处震——破势（谁被破都算），我方挨了敌人的重击（≥ 气血上限的四分之一，或被打倒）；
// 普通命中不震；与「战斗震屏」开关互不相干；设置里关了「手柄震动」就不震。事件走 BattleView 自己的入口（onHit / onBreak），
// 不直接调 Engine::rumble。
TEST_F(PadRumbleWiring, OnlyABreakOrAHeavyEnemyHitOnOurSideRumbles) {
    using fanren::core::battle::BattleEvent;
    using fanren::core::battle::BattleEventKind;
    using fanren::core::battle::BattleState;
    using fanren::core::battle::Unit;
    pressVirtual(SDL_GAMEPAD_BUTTON_SOUTH);
    ASSERT_EQ(app_.engine().lastInputDevice(), InputDevice::Gamepad) << "先验：最后按的是这只手柄";

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
    const auto hit = [](fanren::game::BattleView& view, int actor, int target, int value, int hpAfter) {
        BattleEvent e;
        e.kind = BattleEventKind::Hit;
        e.actor = actor;
        e.target = target;
        e.value = value;
        e.hp = hpAfter;
        e.hit = 1;
        e.hits = 1;
        view.onHit(e, false, 0);
    };

    {
        fanren::game::BattleView view(app_, battle, "pad_probe", "drill_ground");
        hit(view, 1, 0, 24, 76);
        EXPECT_TRUE(pad.strongRumbles().empty()) << "敌人打我方 24 点（不到上限 100 的四分之一）：普通命中，不震";
        hit(view, 0, 1, 100, 0);
        EXPECT_TRUE(pad.strongRumbles().empty()) << "我方打敌人，打得再重也不震";
        hit(view, 1, 0, 25, 75);
        ASSERT_EQ(pad.strongRumbles().size(), 1u) << "敌人打我方 25 点（= 上限的四分之一）：震";
        EXPECT_TRUE(rumbled(pad.strongRumbles(), 32768, 22937)) << "(0.5, 0.35)";
        letTheRumbleEnd();
        hit(view, 1, 0, 3, 0);
        ASSERT_EQ(pad.strongRumbles().size(), 2u) << "这一下把人打倒（hp == 0），伤害再小也震";
        view.onBreak(1);
        ASSERT_EQ(pad.strongRumbles().size(), 3u) << "破势：震";
        EXPECT_TRUE(rumbled({pad.strongRumbles().back()}, 52428, 39321)) << "(0.8, 0.6)";
    }
    letTheRumbleEnd();

    // 与「战斗震屏」互不相干：关了震屏，破势照样震。
    fanren::io::Settings noShake;
    noShake.screenShake = false;
    app_.setSettings(noShake);
    {
        fanren::game::BattleView view(app_, battle, "pad_probe", "drill_ground");
        view.onBreak(1);
        EXPECT_FLOAT_EQ(view.shakeAmplitude(), 0.f) << "先验：震屏确实关了";
        EXPECT_EQ(pad.strongRumbles().size(), 4u) << "震屏关着，手柄照样震";
    }
    letTheRumbleEnd();

    // 设置里关了「手柄震动」：经 Application::applySettings 推给引擎，破势、重击都不震。
    fanren::io::Settings quiet;
    quiet.padRumble = false;
    app_.setSettings(quiet);
    ASSERT_FALSE(app_.engine().rumbleEnabled());
    {
        fanren::game::BattleView view(app_, battle, "pad_probe", "drill_ground");
        view.onBreak(1);
        hit(view, 1, 0, 50, 50);
        EXPECT_EQ(pad.strongRumbles().size(), 4u) << "开关关着：收不到";
    }
}

// 第 10 节第 5 条：挂虚拟手柄，用它真打一仗、打出破势 → 虚拟手柄收到震动。玩家那一手是手柄按出来的（A「攻击」→ A 打第一个
// 敌人，走战斗菜单的真实入口），规则层结算，事件由 BattleScene 逐个演给 BattleView——震动是 BattleView::onBreak 发的，
// 测试不直接调 Engine::rumble。要有画面才建 BattleView（无头下 BattleScene 不建台），所以用软件渲染器的真 Application；
// 手柄子系统由引擎自己开。
// 局面：只写进临时资产根的测试战斗 kTestBattleId（一个木桩：架势 1、怕「拳」、气血 999、打人只掉 1 点、比韩立慢）。
// 韩立照新开局空手出拳，头一拳就破势；那之前谁也不会被重击（1 点不到上限 10 的四分之一），不会先震出「重击」那一下。
// 不借正式的战斗数据：那些编成的平衡一调，这条就会在先验上红（整改轮 LOW-3）。
TEST_F(PadScreen, ABreakInARealFightRumblesThePadThatWasPressedLast) {
    using fanren::game::BattleMenuMode;
    using fanren::game::BattleScene;
    pad.attach();
    tick();
    ASSERT_EQ(app_.engine().gamepadCount(), 1) << "非无头的引擎自己开了手柄子系统，挂上的手柄被打开了";
    ASSERT_NE(app_.battleSetup(kTestBattleId), nullptr) << "先验：临时资产根里的测试战斗读进来了";
    app_.pushScene(std::make_unique<BattleScene>(kTestBattleId));
    tick();
    auto* scene = dynamic_cast<BattleScene*>(app_.topScene());
    ASSERT_NE(scene, nullptr);
    // 开战碎屏要先拍下这一帧的画面才开始碎：照游戏里的第一帧画一次。
    app_.engine().beginFrame();
    app_.drawScenes();
    app_.engine().endFrame();

    const auto breaks = [scene] {
        const auto& events = scene->battle().events();
        return std::count_if(events.begin(), events.end(), [](const fanren::core::battle::BattleEvent& e) {
            return e.kind == fanren::core::battle::BattleEventKind::Break;
        });
    };
    // 轮到我方、菜单开着、上一手演完了，就按手柄 A（根菜单「攻击」→（兵刃不止一样时）第一样 → 第一个敌人）；
    // 否则走一帧。打出破势、而且那一下演完了就停。
    bool quietUntilTheBreak = true;
    for (int frame = 0; frame < 60 * 90 && !(breaks() > 0 && !scene->playing()); ++frame) {
        if (breaks() == 0) quietUntilTheBreak = quietUntilTheBreak && pad.strongRumbles().empty();
        if (scene->menuMode() != BattleMenuMode::Closed && !scene->playing()) {
            pad.setButton(SDL_GAMEPAD_BUTTON_SOUTH, true);
            tick();
            pad.setButton(SDL_GAMEPAD_BUTTON_SOUTH, false);
        }
        tick();
    }
    ASSERT_GT(breaks(), 0) << "先验：用手柄真的打出了破势（韩立气血 " << scene->battle().units()[0].hp << "）";
    ASSERT_EQ(app_.engine().lastInputDevice(), InputDevice::Gamepad);
    EXPECT_TRUE(quietUntilTheBreak) << "破势之前的普通命中、木桩打的那几下都不震";
    EXPECT_TRUE(rumbled(pad.strongRumbles(), 52428, 39321)) << "破势那一下演给 BattleView 时震到了这只手柄（0.8, 0.6）";
}

// 第 11 节：用软件渲染器拍「按过手柄之后」的对白框与主菜单（拍法同截图口）。断言读的是拍进临时目录的那一份，
// 另尽力拷一份到构建目录的 shots\ 给人看：
//   pad_talk_keyboard.png / pad_talk.png：对白框右上那块牌（键盘版 / 手柄版）；
//   pad_menu.png / pad_menu_keyboard.png：主菜单右下那一行提示（手柄版 / 键盘版）。
TEST_F(PadScreen, AfterAPadPressTheDialogueAndTheMainMenuNameThePadsButtons) {
    ASSERT_TRUE(app_.loadMap(fanren::game::kNewGameMap, std::string{}).ok);
    app_.pushScene(std::make_unique<fanren::game::WorldScene>());
    tick(3);
    // 与截图口 talk:ui.ending.continue_hint:han_li 同一句。
    app_.pushScene(std::make_unique<fanren::game::DialogueScene>(app_.speakerName("han_li"),
                                                                 app_.text("ui.ending.continue_hint")));
    tick(240);   // 截图口的对白也空转 240 帧：等逐字走完
    ASSERT_EQ(top(), "Dialogue");
    ASSERT_EQ(app_.engine().lastInputDevice(), InputDevice::Keyboard);
    EXPECT_EQ(app_.text("ui.dialogue.keys"), "Tab 回看　长按 Ctrl 快进");
    const Frame talkKeyboard = shoot("pad_talk_keyboard.png");
    ASSERT_EQ(talkKeyboard.w, 1280);
    ASSERT_EQ(talkKeyboard.h, 720);

    pressPad(SDL_GAMEPAD_BUTTON_DPAD_DOWN);   // 对白框里十字键没有用处：只换设备
    ASSERT_EQ(top(), "Dialogue");
    ASSERT_EQ(app_.engine().lastInputDevice(), InputDevice::Gamepad);
    EXPECT_EQ(app_.text("ui.dialogue.keys"), "Y 回看　长按 RT 快进");
    const Frame talkPad = shoot("pad_talk.png");
    ASSERT_EQ(talkPad.w, 1280);

    pressPad(SDL_GAMEPAD_BUTTON_SOUTH);   // A 翻过这一句
    ASSERT_EQ(top(), "World");
    pressPad(SDL_GAMEPAD_BUTTON_NORTH);   // Y 开主菜单
    tick(2);
    ASSERT_EQ(top(), "Menu");
    EXPECT_EQ(app_.text("ui.menu.keys"), "Y 关闭　B 返回");
    const Frame menuPad = shoot("pad_menu.png");
    pressKey(SDL_SCANCODE_LCTRL);   // 快进键：主菜单里没有用处，只换回键盘
    ASSERT_EQ(top(), "Menu");
    ASSERT_EQ(app_.engine().lastInputDevice(), InputDevice::Keyboard);
    EXPECT_EQ(app_.text("ui.menu.keys"), "Tab 关闭　Esc 返回");
    const Frame menuKeyboard = shoot("pad_menu_keyboard.png");
    // 右下角那一行提示换了字：两张图在那一块不一样（底图是菜单打开那一帧拍的定格，这一块别的东西不动）。
    EXPECT_GT(differingPixels(menuPad, menuKeyboard, fanren::engine::Rect{900, 680, 340, 36}), 0);
    // [对照] 左下角盘缠那块下面只有底图：一个像素都不动——上面那个「不一样」确实是提示换了字，不是整张图在变。
    EXPECT_EQ(differingPixels(menuPad, menuKeyboard, fanren::engine::Rect{48, 684, 300, 30}), 0);
}
