// 平台层单测，两台引擎：
//
//   · 无头引擎（HeadlessEngine）验「headless 契约」：不开窗口、不建渲染器，绘制全是
//     空操作，但字体必须真的加载、measureText 必须给出真实尺寸——无头 bot 与 CI
//     的对话框断行完全依赖它。八方旅人化追加的新接口同样守这条契约：建纹理、建渲染
//     目标、读后缓冲一律拿到无效句柄，textureSize 是 {0,0}，其余全是空操作。
//
//   · 软件渲染引擎（SoftwareEngine）验「画出来的像素对不对」：SDL 的 dummy 视频驱动 +
//     软件渲染器，不开窗口也有一张真的后缓冲。截图验收用的就是这一套，所以这里的像素
//     断言走的也是同一条路：captureFrame 存 PNG → 读回来 → 看像素。
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

#include <SDL3/SDL.h>
#include <gtest/gtest.h>

#include "TempDir.h"
#include "engine/Engine.h"
#include "engine/TextLayout.h"

namespace {

namespace fs = std::filesystem;

using fanren::engine::BlendMode;
using fanren::engine::Color;
using fanren::engine::DrawOptions;
using fanren::engine::Engine;
using fanren::engine::kInvalidTexture;
using fanren::engine::LayoutLine;
using fanren::engine::packRgba;
using fanren::engine::Point;
using fanren::engine::Rect;
using fanren::engine::RectF;
using fanren::engine::ScaleMode;
using fanren::engine::TextStyle;
using fanren::engine::TextureId;
using fanren::engine::Vertex;

// 每个用例自带一台引擎：init/shutdown 本身也是被测对象，
// 共享一台反而会把「重复 init 被拒」这类用例污染掉。
class HeadlessEngine : public ::testing::Test {
protected:
    void SetUp() override {
        const auto result = engine.init("fanren-tests", true);
        ASSERT_TRUE(result.ok) << result.error;
    }
    void TearDown() override { engine.shutdown(); }

    Engine engine;
};

// 无头模式收不到真实按键，所以直接往 SDL 事件队列里塞一个。
// 键位映射表与重复节流是玩家每一帧都在碰的东西，不能只靠肉眼走查。
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

// 写一张 w×h 的纯色 PNG。用 SDL 核心库自带的 PNG 编码（3.4 起有），
// 不经过被测的 Engine：被测物不能自己给自己出考题。
fs::path writePng(const fs::path& dir, const std::string& name, int w, int h, const Color& c) {
    SDL_Surface* surface = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_RGBA32);
    EXPECT_NE(surface, nullptr);
    if (surface == nullptr) return {};
    SDL_FillSurfaceRect(surface, nullptr, SDL_MapSurfaceRGBA(surface, c.r, c.g, c.b, c.a));
    const fs::path path = dir / name;
    EXPECT_TRUE(SDL_SavePNG(surface, path.string().c_str())) << SDL_GetError();
    SDL_DestroySurface(surface);
    return path;
}

// 读回来的一帧。
struct Frame {
    int w = 0;
    int h = 0;
    std::vector<Color> pixels;
    [[nodiscard]] Color at(int x, int y) const {
        return pixels[static_cast<std::size_t>(y) * static_cast<std::size_t>(w) +
                      static_cast<std::size_t>(x)];
    }
};

// 软件渲染器：dummy 视频驱动（不开窗口）+ 强制 software 渲染器 + dummy 音频。
// 提示在 SDL_Init 之前设、用例结束时撤掉，不留给同进程里后面的用例。
class SoftwareEngine : public ::testing::Test {
protected:
    void SetUp() override {
        SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
        SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
        SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "dummy");
        const auto result = engine.init("fanren-tests", false);
        ASSERT_TRUE(result.ok) << result.error;
        engine.beginFrame();
    }
    void TearDown() override {
        engine.shutdown();
        SDL_ResetHint(SDL_HINT_VIDEO_DRIVER);
        SDL_ResetHint(SDL_HINT_RENDER_DRIVER);
        SDL_ResetHint(SDL_HINT_AUDIO_DRIVER);
    }

    // 抓当前后缓冲：与截图验收同一条路（captureFrame → PNG → 读回）。
    Frame grab() {
        Frame frame;
        const fs::path png = dir.path() / ("frame" + std::to_string(shots++) + ".png");
        const auto shot = engine.captureFrame(png.string());
        EXPECT_TRUE(shot.ok) << shot.error;
        if (!shot.ok) return frame;
        SDL_Surface* loaded = SDL_LoadPNG(png.string().c_str());
        EXPECT_NE(loaded, nullptr) << SDL_GetError();
        if (loaded == nullptr) return frame;
        SDL_Surface* rgba = SDL_ConvertSurface(loaded, SDL_PIXELFORMAT_RGBA32);
        SDL_DestroySurface(loaded);
        EXPECT_NE(rgba, nullptr) << SDL_GetError();
        if (rgba == nullptr) return frame;
        frame.w = rgba->w;
        frame.h = rgba->h;
        frame.pixels.reserve(static_cast<std::size_t>(frame.w) * static_cast<std::size_t>(frame.h));
        for (int y = 0; y < frame.h; ++y) {
            const auto* row = static_cast<const std::uint8_t*>(rgba->pixels) + y * rgba->pitch;
            for (int x = 0; x < frame.w; ++x) {
                frame.pixels.push_back(Color{row[x * 4], row[x * 4 + 1], row[x * 4 + 2], row[x * 4 + 3]});
            }
        }
        SDL_DestroySurface(rgba);
        return frame;
    }

    Engine engine;
    fanren::test::TempDir dir{"fanren_engine_sw"};
    int shots = 0;
};

constexpr Color kWhite{255, 255, 255, 255};
constexpr Color kBlack{0, 0, 0, 255};
constexpr Color kRed{255, 0, 0, 255};
constexpr Color kBlue{0, 0, 255, 255};

// 两色是否在容差内相等（只比 RGB：后缓冲的 alpha 没有意义）。
::testing::AssertionResult sameRgb(const Color& got, const Color& want, int tolerance = 0) {
    const auto off = [&](std::uint8_t a, std::uint8_t b) { return std::abs(int{a} - int{b}) > tolerance; };
    if (off(got.r, want.r) || off(got.g, want.g) || off(got.b, want.b)) {
        return ::testing::AssertionFailure()
               << "得到 (" << int{got.r} << "," << int{got.g} << "," << int{got.b} << ")，应为 ("
               << int{want.r} << "," << int{want.g} << "," << int{want.b} << ") ±" << tolerance;
    }
    return ::testing::AssertionSuccess();
}

}  // namespace

// Save 当年的规矩：新键追加在 Count 之前、既有键之后，已有键的编号一个都不许动。
static_assert(static_cast<int>(Engine::Key::Action) == static_cast<int>(Engine::Key::Save) + 1,
              "Action 要紧跟在 Save 之后");
static_assert(static_cast<int>(Engine::Key::Count) == static_cast<int>(Engine::Key::Action) + 1,
              "Action 要在 Count 之前");

TEST(EngineLifecycle, InitialisesAndShutsDownInHeadlessMode) {
    Engine engine;
    const auto first = engine.init("fanren-tests", true);
    ASSERT_TRUE(first.ok) << first.error;
    engine.shutdown();

    // 关掉之后必须还能再开：场景重载与「回到标题」都走这条路。
    const auto second = engine.init("fanren-tests", true);
    EXPECT_TRUE(second.ok) << second.error;
    engine.shutdown();
}

TEST(EngineLifecycle, ShutdownIsIdempotent) {
    Engine engine;
    ASSERT_TRUE(engine.init("fanren-tests", true).ok);
    engine.shutdown();
    engine.shutdown();  // 第二次必须是空操作而不是二次释放
    SUCCEED();
}

TEST(EngineLifecycle, RejectsDoubleInit) {
    Engine engine;
    ASSERT_TRUE(engine.init("fanren-tests", true).ok);
    const auto again = engine.init("fanren-tests", true);
    EXPECT_FALSE(again.ok);
    EXPECT_FALSE(again.error.empty());
    engine.shutdown();
}

TEST_F(HeadlessEngine, MeasureTextReturnsRealSizes) {
    const Point one = engine.measureText("韩", 24);
    EXPECT_GT(one.x, 0);
    EXPECT_GT(one.y, 0);

    // 更长的串必须更宽，否则排版的贪心断行会退化成死循环或一字一行。
    const Point four = engine.measureText("韩立修仙", 24);
    EXPECT_GT(four.x, one.x);
    EXPECT_EQ(four.y, one.y);
}

TEST_F(HeadlessEngine, MeasureTextScalesWithFontSize) {
    const Point small = engine.measureText("韩立", 16);
    const Point large = engine.measureText("韩立", 32);
    EXPECT_GT(large.x, small.x);
    EXPECT_GT(large.y, small.y);
}

TEST_F(HeadlessEngine, MeasureTextOfEmptyStringHasZeroWidthButRealHeight) {
    const Point p = engine.measureText("", 24);
    EXPECT_EQ(p.x, 0);
    EXPECT_GT(p.y, 0);  // 空行仍要占一行高
}

TEST_F(HeadlessEngine, MeasuresLatinAndMixedTextToo) {
    EXPECT_GT(engine.measureText("Hello", 20).x, 0);
    EXPECT_GT(engine.measureText("韩立 said hello", 20).x, 0);
}

TEST_F(HeadlessEngine, LoadTextureYieldsInvalidHandleWithoutARenderer) {
    EXPECT_EQ(engine.loadTexture("assets/does_not_exist.png"), kInvalidTexture);
}

TEST_F(HeadlessEngine, DrawCallsAreLegalNoOps) {
    const Rect src{0, 0, 32, 32};
    const Rect dst{10, 10, 32, 32};
    const Color white{255, 255, 255, 255};

    engine.beginFrame();
    engine.drawRect(dst, white, true);
    engine.drawRect(dst, white, false);
    engine.drawText("韩立", 0, 0, 20, white);
    engine.drawTexture(kInvalidTexture, src, dst);
    engine.drawTexture(12345, src, dst);  // 伪造的句柄不许让游戏崩掉
    engine.endFrame();
    SUCCEED();
}

TEST_F(HeadlessEngine, TextureFactoriesYieldInvalidHandlesEvenForARealImage) {
    // 先验：这张图是真的、读得出来（软件渲染器那边同一个写法能载入，见
    // SoftwareEngine 的纹理用例）。拿一个根本不存在的路径去验「无头返回无效」，
    // 那条断言在有渲染器时也照样成立，等于什么都没验。
    const fanren::test::TempDir dir("fanren_engine_headless");
    const fs::path png = writePng(dir.path(), "real.png", 4, 4, kRed);
    ASSERT_TRUE(fs::exists(png));

    EXPECT_EQ(engine.loadTexture(png.string()), kInvalidTexture);
    EXPECT_EQ(engine.loadTexture(png.string(), ScaleMode::Linear), kInvalidTexture);
    EXPECT_EQ(engine.createRenderTarget(64, 32, ScaleMode::Linear), kInvalidTexture);
    EXPECT_EQ(engine.createTexture(1, 1, {packRgba(kRed)}, ScaleMode::Pixel), kInvalidTexture);
    EXPECT_EQ(engine.snapshotBackbuffer(), kInvalidTexture);
}

TEST_F(HeadlessEngine, TextureSizeIsZeroForEveryHandle) {
    for (const TextureId id : {kInvalidTexture, TextureId{1}, TextureId{12345}}) {
        const Point size = engine.textureSize(id);
        EXPECT_EQ(size.x, 0) << "句柄 " << id;
        EXPECT_EQ(size.y, 0) << "句柄 " << id;
    }
}

TEST_F(HeadlessEngine, NewDrawAndStateCallsAreLegalNoOps) {
    const RectF box{10.f, 10.f, 40.f, 30.f};
    const std::vector<Vertex> triangle{
        {0.f, 0.f, kWhite, 0.f, 0.f}, {10.f, 0.f, kRed, 1.f, 0.f}, {0.f, 10.f, kBlue, 0.f, 1.f}};

    engine.beginFrame();
    engine.setRenderTarget(kInvalidTexture);
    engine.setRenderTarget(4242);   // 伪造的目标：无头模式下同样什么都不做
    engine.clear(Color{1, 2, 3, 4});
    DrawOptions options;
    options.tint = Color{10, 20, 30, 40};
    options.blend = BlendMode::Add;
    options.flipX = true;
    options.flipY = true;
    options.angle = 33.f;
    engine.drawTexture(kInvalidTexture, RectF{}, box, options);
    engine.drawTexture(777, RectF{0.f, 0.f, 4.f, 4.f}, box, options);
    for (const BlendMode mode :
         {BlendMode::None, BlendMode::Alpha, BlendMode::Add, BlendMode::Mod, BlendMode::Mul}) {
        engine.fillRect(box, kRed, mode);
        engine.drawGradient(box, kRed, kBlue, mode);
        engine.drawGradientH(box, kRed, kBlue, mode);
        engine.drawGeometry(kInvalidTexture, triangle, {0, 1, 2}, mode);
    }
    engine.drawGeometry(kInvalidTexture, triangle, {}, BlendMode::Alpha);
    engine.drawGeometry(999, triangle, {0, 1, 7}, BlendMode::Alpha);   // 过期纹理 + 越界下标
    engine.drawGeometry(kInvalidTexture, {}, {}, BlendMode::Alpha);
    engine.drawLine(0.f, 0.f, 100.f, 50.f, kWhite);
    const Rect clip{5, 5, 10, 10};
    engine.setClipRect(&clip);
    engine.setClipRect(nullptr);
    TextStyle style;
    style.shadow = true;
    style.outline = true;
    style.outlineWidth = 2;
    engine.drawText("韩立", 0, 0, 20, kWhite, style);
    engine.setTextureScaleMode(kInvalidTexture, ScaleMode::Linear);
    engine.setTextureScaleMode(31337, ScaleMode::Pixel);
    engine.destroyTexture(kInvalidTexture);
    engine.destroyTexture(31337);
    engine.endFrame();

    // 「什么都不做」的可观察部分：目标仍是后缓冲，抓帧仍然如实失败。
    EXPECT_EQ(engine.renderTarget(), kInvalidTexture);
    EXPECT_FALSE(engine.captureFrame("unused.png").ok);
}

TEST_F(HeadlessEngine, ElapsedSecondsIsTheSumOfTheFrameDeltas) {
    EXPECT_EQ(engine.elapsedSeconds(), 0.0);
    double sum = 0.0;
    for (int i = 0; i < 5; ++i) {
        SDL_Delay(2);   // 保证每一帧都真的过了一点时间
        engine.beginFrame();
        sum += engine.deltaSeconds();
        engine.endFrame();
    }
    EXPECT_GT(engine.elapsedSeconds(), 0.0);
    EXPECT_NEAR(engine.elapsedSeconds(), sum, 1e-9);

    // 重新 init 从零算起：它是「这一次运行」的时钟。
    engine.shutdown();
    ASSERT_TRUE(engine.init("fanren-tests", true).ok);
    EXPECT_EQ(engine.elapsedSeconds(), 0.0);
}

TEST_F(HeadlessEngine, FindAssetsListsMatchingFilesSortedAndNothingForAMissingDir) {
    // assets/fonts 里至少有一套字体（init 就是靠它起来的）。
    const std::vector<std::string> fonts = engine.findAssets("fonts", "*.ttf");
    ASSERT_FALSE(fonts.empty());
    EXPECT_TRUE(std::is_sorted(fonts.begin(), fonts.end()));
    for (const std::string& path : fonts) {
        EXPECT_TRUE(fs::exists(path)) << path;
    }
    EXPECT_TRUE(engine.findAssets("fonts", "*.definitely_not_here").empty());
    EXPECT_TRUE(engine.findAssets("no/such/dir", "*").empty());
}

TEST_F(HeadlessEngine, FrameLoopRunsAndReportsSaneDelta) {
    for (int i = 0; i < 3; ++i) {
        engine.pollEvents();
        engine.beginFrame();
        engine.endFrame();
        EXPECT_GE(engine.deltaSeconds(), 0.0);
        EXPECT_LE(engine.deltaSeconds(), 0.5);  // 上限由内部钳位保证
    }
    EXPECT_FALSE(engine.shouldQuit());
}

TEST_F(HeadlessEngine, AllKeysAreUpWithoutInput) {
    engine.pollEvents();
    for (int k = 0; k < static_cast<int>(Engine::Key::Count); ++k) {
        const auto key = static_cast<Engine::Key>(k);
        EXPECT_FALSE(engine.keyDown(key)) << "键位 " << k;
        EXPECT_FALSE(engine.keyPressed(key)) << "键位 " << k;
    }
}

TEST_F(HeadlessEngine, AudioCallsAreSilentNoOps) {
    engine.playBgm("does_not_exist");
    engine.playSfx("does_not_exist");
    engine.stopBgm();
    SUCCEED();
}

TEST_F(HeadlessEngine, LayoutWorksOnTopOfTheRealFontMetrics) {
    // 排版模块的单测用假测量函数；这里补上「真字体也能跑通」的那一环。
    const int size = 24;
    const auto measure = [this, size](const std::string& s) {
        return engine.measureText(s, size).x;
    };
    const std::string speech =
        "韩立站在院中，望着三叔递来的那封信，半晌没有说话。他知道这一去，"
        "便再难回头。";
    const std::vector<LayoutLine> lines = fanren::engine::layoutText(speech, 400, measure);

    ASSERT_GE(lines.size(), 2u);
    for (const LayoutLine& line : lines) {
        EXPECT_FALSE(line.text.empty());
        EXPECT_GT(line.width, 0);
        // 悬挂标点允许略微超出，但不该超过一整个汉字的两倍。
        EXPECT_LE(line.width, 400 + 2 * size);
    }
}

TEST_F(HeadlessEngine, MapsScancodesToLogicalKeys) {
    // 表驱动：任何一个键位被接错都会在这里露出来。
    const struct {
        SDL_Scancode scancode;
        Engine::Key key;
    } kCases[] = {
        {SDL_SCANCODE_UP, Engine::Key::Up},         {SDL_SCANCODE_W, Engine::Key::Up},
        {SDL_SCANCODE_DOWN, Engine::Key::Down},     {SDL_SCANCODE_S, Engine::Key::Down},
        {SDL_SCANCODE_LEFT, Engine::Key::Left},     {SDL_SCANCODE_A, Engine::Key::Left},
        {SDL_SCANCODE_RIGHT, Engine::Key::Right},   {SDL_SCANCODE_D, Engine::Key::Right},
        {SDL_SCANCODE_RETURN, Engine::Key::Confirm}, {SDL_SCANCODE_Z, Engine::Key::Confirm},
        {SDL_SCANCODE_SPACE, Engine::Key::Confirm}, {SDL_SCANCODE_ESCAPE, Engine::Key::Cancel},
        {SDL_SCANCODE_X, Engine::Key::Cancel},      {SDL_SCANCODE_TAB, Engine::Key::Menu},
        {SDL_SCANCODE_LCTRL, Engine::Key::Skip},    {SDL_SCANCODE_F5, Engine::Key::Save},
        {SDL_SCANCODE_E, Engine::Key::Action},      {SDL_SCANCODE_Q, Engine::Key::Action},
    };
    for (const auto& c : kCases) {
        pushKey(c.scancode, true);
        engine.pollEvents();
        EXPECT_TRUE(engine.keyDown(c.key)) << "扫描码 " << static_cast<int>(c.scancode);
        EXPECT_TRUE(engine.keyPressed(c.key)) << "扫描码 " << static_cast<int>(c.scancode);

        pushKey(c.scancode, false);
        engine.pollEvents();
        EXPECT_FALSE(engine.keyDown(c.key)) << "扫描码 " << static_cast<int>(c.scancode);
    }
}

TEST_F(HeadlessEngine, ActionKeyIsNotTriggeredByMovementOrConfirm) {
    // E / Q 紧挨着 WASD：反过来也要成立——走路、确认不许顺带触发路径行动。
    for (const SDL_Scancode sc : {SDL_SCANCODE_W, SDL_SCANCODE_A, SDL_SCANCODE_S, SDL_SCANCODE_D,
                                  SDL_SCANCODE_Z, SDL_SCANCODE_RETURN, SDL_SCANCODE_SPACE}) {
        pushKey(sc, true);
        engine.pollEvents();
        EXPECT_FALSE(engine.keyPressed(Engine::Key::Action)) << "扫描码 " << static_cast<int>(sc);
        pushKey(sc, false);
        engine.pollEvents();
    }
}

TEST_F(HeadlessEngine, KeepsALogicalKeyDownWhileAnyBoundScancodeIsHeld) {
    // 同时按住方向键和 WASD，松开其中一个不该让方向「断掉」。
    pushKey(SDL_SCANCODE_LEFT, true);
    pushKey(SDL_SCANCODE_A, true);
    engine.pollEvents();
    ASSERT_TRUE(engine.keyDown(Engine::Key::Left));

    pushKey(SDL_SCANCODE_A, false);
    engine.pollEvents();
    EXPECT_TRUE(engine.keyDown(Engine::Key::Left));

    pushKey(SDL_SCANCODE_LEFT, false);
    engine.pollEvents();
    EXPECT_FALSE(engine.keyDown(Engine::Key::Left));
}

TEST_F(HeadlessEngine, ThrottlesKeyRepeatAfterTheFirstTrigger) {
    pushKey(SDL_SCANCODE_DOWN, true);
    engine.pollEvents();
    ASSERT_TRUE(engine.keyPressed(Engine::Key::Down));

    // 紧接着的一帧不许再报「刚按下」，否则菜单一按就窜好几格。
    engine.pollEvents();
    EXPECT_FALSE(engine.keyPressed(Engine::Key::Down));
    EXPECT_TRUE(engine.keyDown(Engine::Key::Down));

    // 过了首次重复延迟（约 250ms）之后才允许再次触发。
    SDL_Delay(350);
    engine.pollEvents();
    EXPECT_TRUE(engine.keyPressed(Engine::Key::Down));

    pushKey(SDL_SCANCODE_DOWN, false);
    engine.pollEvents();
    EXPECT_FALSE(engine.keyDown(Engine::Key::Down));
    EXPECT_FALSE(engine.keyPressed(Engine::Key::Down));
}

// ---------------------------------------------------------------------------
// 系统设置（docs/settings.md 第 4 节）：无头也存值；Alt+Enter 切全屏、不出 Confirm
// ---------------------------------------------------------------------------

TEST_F(HeadlessEngine, SettingsDefaultToTheBehaviourBeforeTheyExisted) {
    EXPECT_FLOAT_EQ(engine.bgmVolume(), 1.f);
    EXPECT_FLOAT_EQ(engine.sfxVolume(), 1.f);
    EXPECT_FALSE(engine.fullscreen());
    EXPECT_TRUE(engine.integerScale());
    EXPECT_TRUE(engine.vsync());
    EXPECT_EQ(engine.effectsLevel(), fanren::engine::EffectsLevel::Full);
}

TEST_F(HeadlessEngine, SettingsAreStoredAndReadBackWithoutAWindow) {
    engine.setBgmVolume(0.49f);
    engine.setSfxVolume(0.f);
    engine.setFullscreen(true);
    engine.setIntegerScale(false);
    engine.setVSync(false);
    engine.setEffectsLevel(fanren::engine::EffectsLevel::Lite);
    EXPECT_FLOAT_EQ(engine.bgmVolume(), 0.49f);
    EXPECT_FLOAT_EQ(engine.sfxVolume(), 0.f);
    EXPECT_TRUE(engine.fullscreen());
    EXPECT_FALSE(engine.integerScale());
    EXPECT_FALSE(engine.vsync());
    EXPECT_EQ(engine.effectsLevel(), fanren::engine::EffectsLevel::Lite);

    // 增益夹在 [0, 1]：负数混音器不收，大于 1 设置里没有这一档。
    engine.setBgmVolume(-0.5f);
    engine.setSfxVolume(3.f);
    EXPECT_FLOAT_EQ(engine.bgmVolume(), 0.f);
    EXPECT_FLOAT_EQ(engine.sfxVolume(), 1.f);

    // 切得回来。
    engine.setFullscreen(false);
    engine.setIntegerScale(true);
    engine.setVSync(true);
    engine.setEffectsLevel(fanren::engine::EffectsLevel::Full);
    EXPECT_FALSE(engine.fullscreen());
    EXPECT_TRUE(engine.integerScale());
    EXPECT_TRUE(engine.vsync());
    EXPECT_EQ(engine.effectsLevel(), fanren::engine::EffectsLevel::Full);
    // 音效还没轨道池（无头不开混音器）：放一声照样是安全的空操作。
    engine.playSfx("ui_confirm");
}

TEST_F(HeadlessEngine, AltEnterTogglesFullscreenAndIsNeverAConfirm) {
    // 真键盘上按住左 Alt 之后的每个按键事件，SDL 都把 SDL_KMOD_LALT 填进 mod：这里照样填。
    pushKey(SDL_SCANCODE_LALT, true, SDL_KMOD_LALT);
    engine.pollEvents();
    pushKey(SDL_SCANCODE_RETURN, true, SDL_KMOD_LALT);
    engine.pollEvents();
    EXPECT_TRUE(engine.fullscreen()) << "Alt+Enter 切全屏";
    EXPECT_FALSE(engine.keyPressed(Engine::Key::Confirm)) << "这次按键整个吞掉";
    EXPECT_FALSE(engine.keyDown(Engine::Key::Confirm));

    // 按住：过了首次重复延迟（250ms）也不许自动重复出 Confirm。
    SDL_Delay(300);
    engine.pollEvents();
    EXPECT_FALSE(engine.keyPressed(Engine::Key::Confirm)) << "按住 300ms 之后冒出了 Confirm";
    EXPECT_FALSE(engine.keyDown(Engine::Key::Confirm));
    pushKey(SDL_SCANCODE_RETURN, false, SDL_KMOD_LALT);
    engine.pollEvents();
    EXPECT_TRUE(engine.fullscreen()) << "松开不切回去";

    // 小键盘的 Enter 同样认：再按一下切回窗口。
    pushKey(SDL_SCANCODE_KP_ENTER, true, SDL_KMOD_LALT);
    engine.pollEvents();
    EXPECT_FALSE(engine.fullscreen());
    EXPECT_FALSE(engine.keyPressed(Engine::Key::Confirm));
    pushKey(SDL_SCANCODE_KP_ENTER, false, SDL_KMOD_LALT);
    pushKey(SDL_SCANCODE_LALT, false);
    engine.pollEvents();

    // [配对的负向] 不带 Alt 的 Enter 照旧是 Confirm，也不碰全屏。
    pushKey(SDL_SCANCODE_RETURN, true);
    engine.pollEvents();
    EXPECT_TRUE(engine.keyPressed(Engine::Key::Confirm));
    EXPECT_FALSE(engine.fullscreen());
    pushKey(SDL_SCANCODE_RETURN, false);
    engine.pollEvents();
}

TEST_F(HeadlessEngine, RightAltEnterCountsTooEvenWithoutSeeingTheAltGoDown) {
    // 认的是事件自带的修饰键：Alt 在窗口拿到焦点之前就按下了（我们没收到它的按下事件）也算，右 Alt 也算。
    pushKey(SDL_SCANCODE_RETURN, true, SDL_KMOD_RALT);
    engine.pollEvents();
    EXPECT_TRUE(engine.fullscreen());
    EXPECT_FALSE(engine.keyPressed(Engine::Key::Confirm));
    pushKey(SDL_SCANCODE_RETURN, false, SDL_KMOD_RALT);
    engine.pollEvents();
}

// ---------------------------------------------------------------------------
// 软件渲染器：像素级
// ---------------------------------------------------------------------------

TEST_F(SoftwareEngine, CreatesTexturesAndTargetsOfTheRequestedSize) {
    const TextureId target = engine.createRenderTarget(64, 32, ScaleMode::Linear);
    ASSERT_NE(target, kInvalidTexture);
    EXPECT_EQ(engine.textureSize(target).x, 64);
    EXPECT_EQ(engine.textureSize(target).y, 32);

    const TextureId made = engine.createTexture(3, 2, std::vector<std::uint32_t>(6, packRgba(kRed)),
                                                ScaleMode::Pixel);
    ASSERT_NE(made, kInvalidTexture);
    EXPECT_EQ(engine.textureSize(made).x, 3);
    EXPECT_EQ(engine.textureSize(made).y, 2);

    // 同一张图按路径缓存；无头那条用例的先验也在这里：这张图确实载得进来。
    const fs::path png = writePng(dir.path(), "real.png", 4, 4, kRed);
    const TextureId loaded = engine.loadTexture(png.string());
    ASSERT_NE(loaded, kInvalidTexture);
    EXPECT_EQ(engine.loadTexture(png.string()), loaded);
    EXPECT_EQ(engine.textureSize(loaded).x, 4);
}

TEST_F(SoftwareEngine, CreateTextureRejectsAPixelCountThatDoesNotMatch) {
    EXPECT_EQ(engine.createTexture(2, 2, std::vector<std::uint32_t>(3, packRgba(kRed)), ScaleMode::Pixel),
              kInvalidTexture);
    EXPECT_EQ(engine.createTexture(0, 2, {}, ScaleMode::Pixel), kInvalidTexture);
}

TEST_F(SoftwareEngine, CreateTexturePixelsAreRrggbbaa) {
    // 0xRRGGBBAA：R 在最高字节。四个像素四种颜色，布局一错就对不上。
    const std::vector<std::uint32_t> pixels{0xFF0000FFu, 0x00FF00FFu, 0x0000FFFFu, 0xFFFFFFFFu};
    const TextureId tex = engine.createTexture(2, 2, pixels, ScaleMode::Pixel);
    ASSERT_NE(tex, kInvalidTexture);
    engine.drawTexture(tex, RectF{}, RectF{0.f, 0.f, 20.f, 20.f}, DrawOptions{});
    const Frame f = grab();
    EXPECT_TRUE(sameRgb(f.at(5, 5), {255, 0, 0, 255}));
    EXPECT_TRUE(sameRgb(f.at(15, 5), {0, 255, 0, 255}));
    EXPECT_TRUE(sameRgb(f.at(5, 15), {0, 0, 255, 255}));
    EXPECT_TRUE(sameRgb(f.at(15, 15), kWhite));
}

TEST_F(SoftwareEngine, DrawingIntoATargetLandsInTheTargetNotOnScreen) {
    const TextureId target = engine.createRenderTarget(100, 50, ScaleMode::Pixel);
    ASSERT_NE(target, kInvalidTexture);
    engine.setRenderTarget(target);
    EXPECT_EQ(engine.renderTarget(), target);
    engine.clear(kRed);
    engine.setRenderTarget(kInvalidTexture);
    EXPECT_EQ(engine.renderTarget(), kInvalidTexture);

    // 还没贴上去：屏幕上不该有红。
    const Frame before = grab();
    EXPECT_TRUE(sameRgb(before.at(20, 30), kBlack));

    DrawOptions copy;
    copy.blend = BlendMode::None;
    engine.drawTexture(target, RectF{}, RectF{10.f, 20.f, 100.f, 50.f}, copy);
    const Frame after = grab();
    EXPECT_TRUE(sameRgb(after.at(15, 25), kRed));
    EXPECT_TRUE(sameRgb(after.at(105, 65), kRed));
    EXPECT_TRUE(sameRgb(after.at(5, 5), kBlack));
    EXPECT_TRUE(sameRgb(after.at(115, 25), kBlack));
}

TEST_F(SoftwareEngine, NewTargetsStartTransparent) {
    const TextureId target = engine.createRenderTarget(40, 40, ScaleMode::Pixel);
    ASSERT_NE(target, kInvalidTexture);
    engine.clear(kBlue);
    engine.drawTexture(target, RectF{}, RectF{0.f, 0.f, 40.f, 40.f}, DrawOptions{});
    EXPECT_TRUE(sameRgb(grab().at(20, 20), kBlue));   // 贴了一张全透明的图：底色原样
}

TEST_F(SoftwareEngine, CaptureFrameReadsTheBackbufferEvenWhileATargetIsBound) {
    engine.clear(kBlue);
    const TextureId target = engine.createRenderTarget(64, 64, ScaleMode::Pixel);
    ASSERT_NE(target, kInvalidTexture);
    engine.setRenderTarget(target);
    engine.clear(Color{0, 255, 0, 255});

    const Frame f = grab();
    ASSERT_EQ(f.w, fanren::engine::kLogicalWidth);
    EXPECT_TRUE(sameRgb(f.at(10, 10), kBlue));        // 屏幕，不是那张绿的离屏图
    EXPECT_EQ(engine.renderTarget(), target);         // 抓完原样恢复
}

TEST_F(SoftwareEngine, BlendModesFollowTheirFormulas) {
    engine.clear(Color{100, 100, 100, 255});
    engine.fillRect({0.f, 0.f, 20.f, 20.f}, Color{50, 0, 0, 255}, BlendMode::Add);
    engine.fillRect({20.f, 0.f, 20.f, 20.f}, Color{128, 255, 255, 255}, BlendMode::Mod);
    engine.fillRect({40.f, 0.f, 20.f, 20.f}, Color{0, 0, 0, 0}, BlendMode::Mul);        // a=0：不变
    engine.fillRect({60.f, 0.f, 20.f, 20.f}, Color{0, 0, 0, 255}, BlendMode::Mul);      // a=1：乘黑
    engine.fillRect({80.f, 0.f, 20.f, 20.f}, Color{200, 0, 0, 128}, BlendMode::Alpha);
    engine.fillRect({100.f, 0.f, 20.f, 20.f}, Color{10, 20, 30, 0}, BlendMode::None);
    const Frame f = grab();
    EXPECT_TRUE(sameRgb(f.at(10, 10), {150, 100, 100, 255}, 2));
    EXPECT_TRUE(sameRgb(f.at(30, 10), {50, 100, 100, 255}, 2));
    EXPECT_TRUE(sameRgb(f.at(50, 10), {100, 100, 100, 255}, 2));
    EXPECT_TRUE(sameRgb(f.at(70, 10), {0, 0, 0, 255}, 2));
    EXPECT_TRUE(sameRgb(f.at(90, 10), {150, 50, 50, 255}, 3));
    EXPECT_TRUE(sameRgb(f.at(110, 10), {10, 20, 30, 255}, 1));
}

TEST_F(SoftwareEngine, TintFlipAndRotationAreHonoured) {
    const TextureId white = engine.createTexture(1, 1, {packRgba(kWhite)}, ScaleMode::Pixel);
    const TextureId redBlue =
        engine.createTexture(2, 1, {packRgba(kRed), packRgba(kBlue)}, ScaleMode::Pixel);
    ASSERT_NE(white, kInvalidTexture);
    ASSERT_NE(redBlue, kInvalidTexture);

    DrawOptions tinted;
    tinted.tint = Color{0, 255, 0, 255};
    engine.drawTexture(white, RectF{}, RectF{0.f, 0.f, 10.f, 10.f}, tinted);

    engine.drawTexture(redBlue, RectF{}, RectF{100.f, 0.f, 40.f, 20.f}, DrawOptions{});
    DrawOptions flipped;
    flipped.flipX = true;
    engine.drawTexture(redBlue, RectF{}, RectF{200.f, 0.f, 40.f, 20.f}, flipped);

    // 顺时针转 90°：原来在左边的红转到上面。
    DrawOptions turned;
    turned.angle = 90.f;
    engine.drawTexture(redBlue, RectF{}, RectF{400.f, 100.f, 40.f, 20.f}, turned);

    // 同一张纹理换回旧接口：tint、翻转、旋转一样都不许漏过来。
    engine.drawTexture(white, Rect{0, 0, 0, 0}, Rect{20, 0, 10, 10});

    // 取点都离交界处几个像素：这里验的是「哪一头朝哪」，不是取样算法的舍入。
    const Frame f = grab();
    EXPECT_TRUE(sameRgb(f.at(5, 5), {0, 255, 0, 255}));
    EXPECT_TRUE(sameRgb(f.at(105, 10), kRed, 8));
    EXPECT_TRUE(sameRgb(f.at(135, 10), kBlue, 8));
    EXPECT_TRUE(sameRgb(f.at(205, 10), kBlue, 8));
    EXPECT_TRUE(sameRgb(f.at(235, 10), kRed, 8));
    EXPECT_TRUE(sameRgb(f.at(420, 95), kRed, 8));
    EXPECT_TRUE(sameRgb(f.at(420, 125), kBlue, 8));
    EXPECT_TRUE(sameRgb(f.at(25, 5), kWhite));
}

TEST_F(SoftwareEngine, PixelScalingStaysCrispAndLinearScalingBlends) {
    const std::vector<std::uint32_t> blackWhite{packRgba(kBlack), packRgba(kWhite)};
    const TextureId pixel = engine.createTexture(2, 1, blackWhite, ScaleMode::Pixel);
    const TextureId smooth = engine.createTexture(2, 1, blackWhite, ScaleMode::Linear);
    ASSERT_NE(pixel, kInvalidTexture);
    ASSERT_NE(smooth, kInvalidTexture);
    engine.drawTexture(pixel, RectF{}, RectF{0.f, 0.f, 200.f, 10.f}, DrawOptions{});
    engine.drawTexture(smooth, RectF{}, RectF{0.f, 20.f, 200.f, 10.f}, DrawOptions{});
    const Frame f = grab();

    // 像素：左边纯黑、右边纯白，一路上没有一个灰点（交界落在哪一列是取样的舍入，不验）。
    for (int x = 0; x < 200; ++x) {
        const std::uint8_t v = f.at(x, 5).r;
        EXPECT_TRUE(v == 0 || v == 255) << "x=" << x << " 出现了灰 " << int{v};
    }
    EXPECT_EQ(f.at(90, 5).r, 0);
    EXPECT_EQ(f.at(110, 5).r, 255);
    // 线性：正中间是灰。
    const int mid = f.at(100, 25).r;
    EXPECT_GT(mid, 60);
    EXPECT_LT(mid, 196);
}

TEST_F(SoftwareEngine, ClipRectConfinesDrawingAndNullLiftsIt) {
    const Rect clip{300, 0, 10, 10};
    engine.setClipRect(&clip);
    engine.fillRect({290.f, 0.f, 40.f, 20.f}, kWhite, BlendMode::Alpha);
    engine.setClipRect(nullptr);
    engine.fillRect({400.f, 0.f, 10.f, 10.f}, kWhite, BlendMode::Alpha);
    const Frame f = grab();
    EXPECT_TRUE(sameRgb(f.at(305, 5), kWhite));
    EXPECT_TRUE(sameRgb(f.at(295, 5), kBlack));
    EXPECT_TRUE(sameRgb(f.at(320, 5), kBlack));
    EXPECT_TRUE(sameRgb(f.at(305, 15), kBlack));
    EXPECT_TRUE(sameRgb(f.at(405, 5), kWhite));
}

TEST_F(SoftwareEngine, GradientsAndVertexColoursInterpolate) {
    engine.drawGradient({0.f, 0.f, 100.f, 200.f}, kBlack, kWhite, BlendMode::Alpha);
    engine.drawGradientH({200.f, 0.f, 200.f, 50.f}, kBlack, kWhite, BlendMode::Alpha);
    // 带纹理的几何 + 顶点 alpha：左边透明、右边不透明（景深带就是这么画的）。
    const TextureId white = engine.createTexture(1, 1, {packRgba(kWhite)}, ScaleMode::Linear);
    ASSERT_NE(white, kInvalidTexture);
    const std::vector<Vertex> quad{{500.f, 0.f, Color{255, 255, 255, 0}, 0.f, 0.f},
                                   {700.f, 0.f, kWhite, 1.f, 0.f},
                                   {700.f, 50.f, kWhite, 1.f, 1.f},
                                   {500.f, 50.f, Color{255, 255, 255, 0}, 0.f, 1.f}};
    engine.drawGeometry(white, quad, {0, 1, 2, 0, 2, 3}, BlendMode::Alpha);
    const Frame f = grab();

    EXPECT_LT(f.at(50, 3).r, 20);
    EXPECT_GT(f.at(50, 196).r, 235);
    EXPECT_NEAR(f.at(50, 100).r, 128, 20);
    EXPECT_LT(f.at(203, 25).r, 20);
    EXPECT_GT(f.at(396, 25).r, 235);
    EXPECT_NEAR(f.at(300, 25).r, 128, 20);
    EXPECT_LT(f.at(503, 25).r, 20);
    EXPECT_GT(f.at(696, 25).r, 235);
    EXPECT_NEAR(f.at(600, 25).r, 128, 20);
}

TEST_F(SoftwareEngine, LinesAreDrawn) {
    engine.drawLine(0.f, 300.f, 1279.f, 300.f, kWhite);
    const Frame f = grab();
    EXPECT_TRUE(sameRgb(f.at(640, 300), kWhite));
    EXPECT_TRUE(sameRgb(f.at(640, 302), kBlack));
}

namespace {

struct Ink {
    int count = 0;
    int minX = 1 << 30, minY = 1 << 30, maxX = -1, maxY = -1;
    double sumX = 0.0, sumY = 0.0;
};

// region 里满足 pred 的像素有多少、落在哪儿。
template <typename Pred>
Ink inkIn(const Frame& f, const Rect& region, Pred pred) {
    Ink ink;
    for (int y = region.y; y < region.y + region.h; ++y) {
        for (int x = region.x; x < region.x + region.w; ++x) {
            if (pred(f.at(x, y))) {
                ++ink.count;
                ink.minX = std::min(ink.minX, x - region.x);
                ink.minY = std::min(ink.minY, y - region.y);
                ink.maxX = std::max(ink.maxX, x - region.x);
                ink.maxY = std::max(ink.maxY, y - region.y);
                ink.sumX += x - region.x;
                ink.sumY += y - region.y;
            }
        }
    }
    return ink;
}

}  // namespace

TEST_F(SoftwareEngine, DefaultTextStyleDrawsExactlyLikeTheOldInterface) {
    engine.drawText("韩立修仙", 100, 100, 32, kWhite);
    engine.drawText("韩立修仙", 100, 300, 32, kWhite, TextStyle{});
    const Frame f = grab();
    int differing = 0;
    int inked = 0;
    for (int y = 0; y < 60; ++y) {
        for (int x = 0; x < 200; ++x) {
            const Color a = f.at(100 + x, 100 + y);
            const Color b = f.at(100 + x, 300 + y);
            differing += (a.r != b.r || a.g != b.g || a.b != b.b) ? 1 : 0;
            inked += a.r > 128 ? 1 : 0;
        }
    }
    EXPECT_GT(inked, 50);         // 先验：真的画出了字
    EXPECT_EQ(differing, 0);
}

TEST_F(SoftwareEngine, OutlineSurroundsTheGlyphsAndShadowFallsDownRight) {
    const Rect plainBox{80, 80, 120, 90};
    const Rect edgedBox{380, 80, 120, 90};
    const Rect shadowBox{680, 80, 120, 90};
    engine.drawText("韩", 100, 100, 48, kWhite);
    TextStyle edged;
    edged.outline = true;
    edged.outlineWidth = 2;
    edged.outlineColor = kRed;
    engine.drawText("韩", 400, 100, 48, kWhite, edged);
    TextStyle shadowed;
    shadowed.shadow = true;
    shadowed.shadowOffset = 4;
    shadowed.shadowColor = Color{0, 255, 0, 255};
    engine.drawText("韩", 700, 100, 48, kWhite, shadowed);
    const Frame f = grab();

    const auto white = [](const Color& c) { return c.r > 200 && c.g > 200 && c.b > 200; };
    const auto red = [](const Color& c) { return c.r > 150 && c.g < 80 && c.b < 80; };
    const auto green = [](const Color& c) { return c.g > 150 && c.r < 80 && c.b < 80; };
    const auto any = [](const Color& c) { return c.r > 40 || c.g > 40 || c.b > 40; };

    const Ink plain = inkIn(f, plainBox, white);
    ASSERT_GT(plain.count, 50);
    // 描边：出现了红，且有墨的范围比正文每边都宽出去。
    EXPECT_GT(inkIn(f, edgedBox, red).count, 50);
    const Ink edgedAll = inkIn(f, edgedBox, any);
    const Ink plainAll = inkIn(f, plainBox, any);
    EXPECT_LT(edgedAll.minX, plainAll.minX);
    EXPECT_LT(edgedAll.minY, plainAll.minY);
    EXPECT_GT(edgedAll.maxX, plainAll.maxX);
    EXPECT_GT(edgedAll.maxY, plainAll.maxY);
    // 正文仍在原处：白字的重心与不描边时一致（描边往外扩，不挪正文）。
    const Ink edgedWhite = inkIn(f, edgedBox, white);
    ASSERT_GT(edgedWhite.count, 0);
    EXPECT_NEAR(edgedWhite.sumX / edgedWhite.count, plain.sumX / plain.count, 1.0);
    EXPECT_NEAR(edgedWhite.sumY / edgedWhite.count, plain.sumY / plain.count, 1.0);
    // 描边一圈是围着正文的：红圈的重心与白字的重心重合。描边层贴歪了
    // （忘了往左上挪 outlineWidth），红圈会整体偏向右下。
    const Ink edgedRed = inkIn(f, edgedBox, red);
    EXPECT_NEAR(edgedRed.sumX / edgedRed.count, edgedWhite.sumX / edgedWhite.count, 1.5);
    EXPECT_NEAR(edgedRed.sumY / edgedRed.count, edgedWhite.sumY / edgedWhite.count, 1.5);
    // 投影：绿色的重心在白字重心的右下方约 shadowOffset 处。
    const Ink shade = inkIn(f, shadowBox, green);
    ASSERT_GT(shade.count, 20);
    const Ink shadowWhite = inkIn(f, shadowBox, white);
    EXPECT_GT(shade.sumX / shade.count, shadowWhite.sumX / shadowWhite.count + 1.0);
    EXPECT_GT(shade.sumY / shade.count, shadowWhite.sumY / shadowWhite.count + 1.0);
}

TEST_F(SoftwareEngine, DestroyedTexturesDrawNothingAndFreeTheirPathSlot) {
    const TextureId target = engine.createRenderTarget(32, 32, ScaleMode::Pixel);
    ASSERT_NE(target, kInvalidTexture);
    engine.setRenderTarget(target);
    engine.destroyTexture(target);
    EXPECT_EQ(engine.renderTarget(), kInvalidTexture);   // 销毁当前目标 → 回到后缓冲
    EXPECT_EQ(engine.textureSize(target).x, 0);
    engine.setRenderTarget(target);                       // 过期句柄：按后缓冲处理
    EXPECT_EQ(engine.renderTarget(), kInvalidTexture);

    const fs::path png = writePng(dir.path(), "slot.png", 2, 2, kWhite);
    const TextureId first = engine.loadTexture(png.string());
    ASSERT_NE(first, kInvalidTexture);
    engine.destroyTexture(first);
    engine.drawTexture(first, RectF{}, RectF{0.f, 0.f, 50.f, 50.f}, DrawOptions{});
    EXPECT_TRUE(sameRgb(grab().at(25, 25), kBlack));
    const TextureId second = engine.loadTexture(png.string());
    ASSERT_NE(second, kInvalidTexture);
    EXPECT_NE(second, first);   // 路径缓存已摘掉：重新读盘，发新句柄
}

TEST_F(SoftwareEngine, OwnedTextureReleasesItsTextureAndMovesOwnership) {
    TextureId id = kInvalidTexture;
    {
        fanren::engine::OwnedTexture owned(engine, engine.createRenderTarget(8, 8, ScaleMode::Pixel));
        id = owned.get();
        ASSERT_NE(id, kInvalidTexture);
        fanren::engine::OwnedTexture moved(std::move(owned));
        EXPECT_FALSE(owned.valid());
        EXPECT_EQ(moved.get(), id);
        EXPECT_EQ(engine.textureSize(id).x, 8);
    }
    EXPECT_EQ(engine.textureSize(id).x, 0);
}

TEST_F(SoftwareEngine, SnapshotCopiesTheFinalBackbuffer) {
    engine.clear(Color{10, 200, 30, 255});
    const TextureId shot = engine.snapshotBackbuffer();
    ASSERT_NE(shot, kInvalidTexture);
    EXPECT_EQ(engine.textureSize(shot).x, fanren::engine::kLogicalWidth);
    EXPECT_EQ(engine.textureSize(shot).y, fanren::engine::kLogicalHeight);

    engine.clear(kBlack);
    DrawOptions copy;
    copy.blend = BlendMode::None;
    engine.drawTexture(shot, RectF{}, RectF{0.f, 0.f, 1280.f, 720.f}, copy);
    EXPECT_TRUE(sameRgb(grab().at(640, 360), {10, 200, 30, 255}));
    engine.destroyTexture(shot);
}

TEST_F(SoftwareEngine, BeginFrameResetsTheTargetAndTheClip) {
    const TextureId target = engine.createRenderTarget(16, 16, ScaleMode::Pixel);
    ASSERT_NE(target, kInvalidTexture);
    const Rect tiny{0, 0, 1, 1};
    engine.setClipRect(&tiny);
    engine.setRenderTarget(target);
    engine.endFrame();
    engine.beginFrame();
    EXPECT_EQ(engine.renderTarget(), kInvalidTexture);
    engine.fillRect({0.f, 0.f, 100.f, 100.f}, kWhite, BlendMode::Alpha);
    EXPECT_TRUE(sameRgb(grab().at(50, 50), kWhite));   // 上一帧的裁剪没有漏过来
}

// 关了垂直同步也封顶 240 帧/秒（docs/settings.md 4.2）：关它是为了少一帧延迟，不是为了让一个核空转。
// 软件渲染器 present 几乎不花时间，没有封顶的话十帧转眼就过去了。
TEST_F(SoftwareEngine, WithVSyncOffAFrameStillTakesAtLeastA240thOfASecond) {
    engine.setVSync(false);
    ASSERT_FALSE(engine.vsync());
    engine.endFrame();   // 起点：记下这一帧 present 完的时刻
    const std::uint64_t start = SDL_GetTicksNS();
    constexpr int kFrames = 10;
    for (int i = 0; i < kFrames; ++i) {
        engine.beginFrame();
        engine.endFrame();
    }
    const double ms = static_cast<double>(SDL_GetTicksNS() - start) / 1.0e6;
    EXPECT_GE(ms, kFrames * 1000.0 / 240.0 - 1.0) << "十帧只用了 " << ms << " 毫秒";
    // 宽松的上界（十帧约 42 毫秒，给调度留足余量）：起作用的是 240 的封顶，不是别的什么把帧拖慢了——
    // 比如又被当成垂直同步开着、按 60Hz 等（那是 167 毫秒）。
    EXPECT_LT(ms, 120.0) << "十帧用了 " << ms << " 毫秒：这不是 240 帧的封顶";
    engine.beginFrame();   // 与 TearDown 之前的状态对齐（夹具开场就在一帧里）
}
