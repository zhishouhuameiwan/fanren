// 女角色的高清立绘（docs/heroine-art-build.md 第 2、3 节；施工记录 docs/heroine-art-build-e.md）。
//
// 四组：
//   · 读取器（io/PortraitLoader 的 parseBusts / loadBusts）：形状照 portraits.json、files 记 sha256、按章生效；
//     仓库里那份读得进来、陈巧倩登记了；
//   · 摆位（game/Busts.h 的纯函数）：框在下、框长高了、框挪到顶边、框挪到半中腰；队伍页取格子中间那块；
//   · 淡入淡出（BustStage，纯）：0.15 秒淡入、同一个人接着说不重新淡入、换人淡出、对白收起一阵子清空；
//   · 截图：软件渲染器（dummy 视频驱动）上开一台真 Application，游戏根是一份临时拷贝，立绘表与头像表换成夹具，
//     走 Application::sayAs（脚本 talk 落到的同一处）。有立绘的人：立绘画在右侧、框里不画小头像；没有立绘的人
//     照旧画小头像；缺图、坏表、认不出的表情三种退路；无头模式不读表；回看面板里不画；整段跳过时不画；
//     插话气泡退回成的对白不带立绘。
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <SDL3/SDL.h>
#include <gtest/gtest.h>

#include "TempDir.h"
#include "game/Application.h"
#include "game/Busts.h"
#include "game/DialogueScene.h"
#include "game/Portraits.h"
#include "io/PortraitLoader.h"

namespace {

namespace fs = std::filesystem;
using fanren::engine::Rect;
using fanren::engine::RectF;
using fanren::game::BustShot;
using fanren::game::BustStage;

std::string repoRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "maps" / "ch01_hanjiacun.tmj")) return candidate;
    }
    return ".";
}

constexpr const char* kEntry =
    R"({"file": "a.png", "frame_w": 840, "frame_h": 1260, "cols": 4,
        "faces": {"normal": 0, "smile": 1, "angry": 2, "hurt": 3}})";
const std::string kDigest(64, 'a');

// ---------------------------------------------------------------------------
// 读取器
// ---------------------------------------------------------------------------

TEST(BustLoader, ReadsTheSameShapeAsPortraitsPlusFileDigests) {
    const auto table = fanren::io::parseBusts(std::string(R"({"_doc": "x", "busts": {"chen_shimei": [)") + kEntry +
                                              R"(]}, "files": {"a.png": ")" + kDigest + R"("}})");
    ASSERT_TRUE(table.ok) << table.error;
    ASSERT_EQ(table.value.count("chen_shimei"), 1u);
    const auto& sheet = table.value.at("chen_shimei").front();
    EXPECT_EQ(sheet.file, "a.png");
    EXPECT_EQ(sheet.frameW, 840);
    EXPECT_EQ(sheet.frameH, 1260);
    EXPECT_EQ(sheet.faces[2], 2);
}

TEST(BustLoader, ErrorsNameBustsJsonAndRejectBadDigestsAndForeignKeys) {
    const auto badDigest = fanren::io::parseBusts(R"({"busts": {}, "files": {"a.png": "XYZ"}})");
    ASSERT_FALSE(badDigest.ok);
    EXPECT_EQ(badDigest.error.rfind("busts.json", 0), 0u) << badDigest.error;
    EXPECT_NE(badDigest.error.find("files.a.png"), std::string::npos) << badDigest.error;

    const auto portraitsKey = fanren::io::parseBusts(R"({"portraits": {}})");
    ASSERT_FALSE(portraitsKey.ok) << "顶层键叫 busts，写成 portraits 是认不出的字段";
    EXPECT_EQ(portraitsKey.error.rfind("busts.json", 0), 0u) << portraitsKey.error;

    const auto missingFace = fanren::io::parseBusts(
        R"({"busts": {"x": [{"file": "a.png", "frame_w": 8, "frame_h": 12, "cols": 1, "faces": {"normal": 0}}]}})");
    ASSERT_FALSE(missingFace.ok);
    EXPECT_NE(missingFace.error.find("busts.x[0].faces"), std::string::npos) << missingFace.error;

    // 头像表不认 files：两份表共用读取器，口径不串。
    const auto portraitFiles = fanren::io::parsePortraits(R"({"portraits": {}, "files": {}})");
    EXPECT_FALSE(portraitFiles.ok);
}

TEST(BustLoader, TheShippedTableLoadsAndChenQiaoqianHasABust) {
    const fs::path path = fs::path(repoRoot()) / "data" / "visual" / "busts.json";
    const auto table = fanren::io::loadBusts(path.string());
    ASSERT_TRUE(table.ok) << table.error;
    ASSERT_EQ(table.value.count("chen_shimei"), 1u);
    const auto& sheet = table.value.at("chen_shimei").front();
    EXPECT_EQ(sheet.frameW, 840);
    EXPECT_EQ(sheet.frameH, 1260);
    EXPECT_TRUE(fs::exists(fs::path(repoRoot()) / "assets" / "art" / "busts" / sheet.file)) << sheet.file;
    EXPECT_EQ(table.value.count("hanli"), 0u) << "男角色不配立绘（先只画女角色）";
}

TEST(BustLoader, TheFirstEntryInEffectThisChapterWins) {
    const auto table = fanren::io::parseBusts(
        R"({"busts": {"x": [
            {"file": "seven.png", "frame_w": 8, "frame_h": 12, "cols": 1,
             "faces": {"normal": 0, "smile": 0, "angry": 0, "hurt": 0}, "min_chapter": 7, "max_chapter": 7},
            {"file": "later.png", "frame_w": 8, "frame_h": 12, "cols": 1,
             "faces": {"normal": 0, "smile": 0, "angry": 0, "hurt": 0}}]}})");
    ASSERT_TRUE(table.ok) << table.error;
    const auto& sheets = table.value.at("x");
    const auto upTo = [](int last) { return [last](int n) { return n <= last; }; };
    ASSERT_NE(fanren::game::portraitForChapter(sheets, upTo(6)), nullptr);
    EXPECT_EQ(fanren::game::portraitForChapter(sheets, upTo(6))->file, "seven.png") << "第 7 章：第一条";
    EXPECT_EQ(fanren::game::portraitForChapter(sheets, upTo(7))->file, "later.png") << "第 7 章演完：第二条";
    EXPECT_EQ(fanren::game::portraitForChapter(sheets, upTo(3))->file, "later.png")
        << "第 7 章之前第一条不生效，退到不限章的那一条";
}

// ---------------------------------------------------------------------------
// 摆位
// ---------------------------------------------------------------------------

constexpr Rect kBottomBox{48, 472, 1184, 200};   // DialogueScene::boxAreaFor(0, 缺省皮肤)

TEST(BustLayout, WithTheBoxAtTheBottomTheBustStandsRightAndItsFootSits40BelowTheBoxTop) {
    const Rect bust = fanren::game::bustDialogueArea(kBottomBox, kBottomBox, kBottomBox.y);
    EXPECT_EQ(bust.w, 420);
    EXPECT_EQ(bust.h, 630);
    EXPECT_EQ(bust.x + bust.w, 1280 - 60) << "右边距 60";
    EXPECT_EQ(bust.y + bust.h, kBottomBox.y + 40) << "底边落在框顶以下 40";
    EXPECT_GE(bust.y + fanren::game::kBustHeadLine, 40) << "头顶线在屏幕里";
    // 框是半透明的：框后面那一截不画，画到框顶金线下 3 像素。
    EXPECT_EQ(fanren::game::bustVisibleRows(bust, kBottomBox, false), kBottomBox.y + 3 - bust.y);
    // 不压左上角的地名与目标框（WorldHud 那一栏宽不到 560）。
    EXPECT_GT(bust.x, 560);
}

TEST(BustLayout, ATallerChoiceBoxDoesNotPushTheHeadOffScreen) {
    const Rect tall{48, 300, 1184, 372};
    const Rect bust = fanren::game::bustDialogueArea(tall, tall, kBottomBox.y);
    EXPECT_EQ(bust.y + bust.h, kBottomBox.y + 40) << "框因选项长高了，立绘不跟着往上抬";
    EXPECT_EQ(fanren::game::bustVisibleRows(bust, tall, false), tall.y + 3 - bust.y) << "多出来的那截由框压住";
}

TEST(BustLayout, WithTheBoxRaisedToTheTopTheBustSitsOnTheScreenBottom) {
    const Rect top{48, 48, 1184, 200};
    const Rect bust = fanren::game::bustDialogueArea(top, kBottomBox, kBottomBox.y);
    EXPECT_EQ(bust.y + bust.h, 720) << "框贴到顶边：立绘底边贴屏幕底边";
    EXPECT_EQ(bust.x + bust.w, 1220);
    EXPECT_GT(bust.y + fanren::game::kBustHeadLine, top.y + top.h) << "头顶在框下面";
    EXPECT_EQ(fanren::game::bustVisibleRows(bust, top, true), 630) << "整幅都画";
}

TEST(BustLayout, WithTheBoxRaisedOnlyHalfwayTheFaceIsNotUnderTheBox) {
    const Rect mid{48, 235, 1184, 200};
    const Rect bust = fanren::game::bustDialogueArea(mid, kBottomBox, kBottomBox.y);
    EXPECT_EQ(bust.y + fanren::game::kBustHeadLine, mid.y + mid.h + fanren::game::kBustHeadGap)
        << "头顶落在框底以下 12";
    EXPECT_GT(bust.y + bust.h, 720) << "下半截出屏";
}

TEST(BustLayout, TheRosterTakesTheMiddleOfTheCellAt560High) {
    const RectF src = fanren::game::bustRosterSource(RectF{0.f, 0.f, 840.f, 1260.f});
    EXPECT_FLOAT_EQ(src.x, 120.f);
    EXPECT_FLOAT_EQ(src.y, 336.f) << "头顶线 376 以上留 40";
    EXPECT_FLOAT_EQ(src.w, 600.f);
    EXPECT_FLOAT_EQ(src.h, 924.f);
    const RectF dst = fanren::game::bustRosterArea(Rect{100, 50, 380, 600}, src);
    EXPECT_FLOAT_EQ(dst.h, 560.f);
    EXPECT_NEAR(dst.w, 560.f * 600.f / 924.f, 0.01f);
    EXPECT_FLOAT_EQ(dst.y + dst.h, 650.f) << "底边贴那一栏的底";
    EXPECT_NEAR(dst.x + dst.w / 2.f, 290.f, 0.01f) << "水平居中";
    // 那一栏不够高：按栏高缩。
    EXPECT_FLOAT_EQ(fanren::game::bustRosterArea(Rect{0, 0, 380, 400}, src).h, 400.f);
}

// ---------------------------------------------------------------------------
// 淡入淡出
// ---------------------------------------------------------------------------

BustShot shot(const std::string& role, float x = 0.f) {
    return BustShot{role, 7u, RectF{x, 0.f, 840.f, 1260.f}};
}

constexpr double kFrame = 1.0 / 60.0;

TEST(BustStage, HerFirstLineFadesInOverAFifteenthOfASecondTimesTwoAndAHalf) {
    BustStage stage;
    stage.speak(shot("chen"));
    EXPECT_TRUE(stage.layers().empty()) << "刚开口那一刻透明度为 0";
    stage.advance(0.075, true);
    ASSERT_EQ(stage.layers().size(), 1u);
    EXPECT_NEAR(stage.layers()[0].alpha, 0.5f, 1e-4f);
    stage.advance(0.075, true);
    EXPECT_FLOAT_EQ(stage.layers()[0].alpha, 1.f) << "0.15 秒淡满";
    stage.advance(1.0, true);
    EXPECT_FLOAT_EQ(stage.layers()[0].alpha, 1.f);
}

TEST(BustStage, HerNextLineDoesNotFadeInAgainAndANewFaceSwapsAtOnce) {
    BustStage stage;
    stage.speak(shot("chen", 0.f));
    stage.advance(0.2, true);
    stage.advance(kFrame, false);   // 换句之间那一帧空档：不清
    stage.speak(shot("chen", 840.f));
    ASSERT_EQ(stage.layers().size(), 1u);
    EXPECT_FLOAT_EQ(stage.layers()[0].alpha, 1.f) << "接着说：不重新淡入";
    EXPECT_FLOAT_EQ(stage.layers()[0].shot.src.x, 840.f) << "表情变了直接换格";
}

TEST(BustStage, WhenSomeoneElseSpeaksSheFadesOut) {
    BustStage stage;
    stage.speak(shot("chen"));
    stage.advance(0.2, true);
    stage.speak(std::nullopt);   // 韩立（没有立绘）
    stage.advance(0.075, true);
    ASSERT_EQ(stage.layers().size(), 1u);
    EXPECT_EQ(stage.layers()[0].shot.role, "chen");
    EXPECT_NEAR(stage.layers()[0].alpha, 0.5f, 1e-4f) << "淡出到一半";
    EXPECT_TRUE(stage.currentRole().empty());
    stage.advance(0.075, true);
    EXPECT_TRUE(stage.layers().empty()) << "0.15 秒淡完";
    EXPECT_TRUE(stage.empty());
}

TEST(BustStage, TwoWomenCrossFadeAndAHalfFadedOneComesBackFromWhereSheWas) {
    BustStage stage;
    stage.speak(shot("chen"));
    stage.advance(0.2, true);
    stage.speak(shot("nangong"));
    stage.advance(0.06, true);
    auto layers = stage.layers();
    ASSERT_EQ(layers.size(), 2u);
    EXPECT_EQ(layers[0].shot.role, "chen") << "淡出的在下";
    EXPECT_EQ(layers[1].shot.role, "nangong") << "淡入的在上";
    EXPECT_NEAR(layers[0].alpha + layers[1].alpha, 1.f, 1e-4f);
    const float chenAlpha = layers[0].alpha;
    stage.speak(shot("chen"));
    layers = stage.layers();
    ASSERT_EQ(layers.size(), 2u);
    EXPECT_EQ(layers[1].shot.role, "chen");
    EXPECT_FLOAT_EQ(layers[1].alpha, chenAlpha) << "从此刻的透明度接着淡入，不从零开始";
}

TEST(BustStage, WhenTheDialogueIsGoneForAWhileTheStageIsCleared) {
    BustStage stage;
    stage.speak(shot("chen"));
    stage.advance(0.2, true);
    stage.advance(kFrame, false);
    EXPECT_FALSE(stage.empty()) << "一帧空档不清";
    stage.advance(fanren::game::kBustLingerSeconds, false);
    EXPECT_TRUE(stage.empty()) << "对白收起一阵子：清空，下一段戏她再开口重新淡入";
    stage.speak(shot("chen"));
    EXPECT_TRUE(stage.layers().empty());
}

// ---------------------------------------------------------------------------
// 截图
// ---------------------------------------------------------------------------

struct Rgb {
    std::uint8_t r = 0, g = 0, b = 0;
};

struct Frame {
    int w = 0, h = 0;
    std::vector<Rgb> pixels;
    [[nodiscard]] Rgb at(int x, int y) const { return pixels[static_cast<std::size_t>(y * w + x)]; }
};

// 夹具立绘：每格 168×252（2:3），四个表情四种纯色、不透明。画成 420×630 后格子中间仍是纯色（线性缩放，内部不混色）。
constexpr int kCellW = 168;
constexpr int kCellH = 252;
constexpr Rgb kFaceColors[4] = {{200, 40, 40}, {40, 200, 40}, {40, 40, 200}, {200, 200, 40}};
// 夹具小头像：56×64 纯品红，画成 ×2。
constexpr Rgb kPortraitColor{255, 0, 255};

bool near(const Rgb& a, const Rgb& b, int tolerance = 3) {
    return std::abs(a.r - b.r) <= tolerance && std::abs(a.g - b.g) <= tolerance && std::abs(a.b - b.b) <= tolerance;
}

void writeSolid(const fs::path& path, int cellW, int cellH, const std::vector<Rgb>& colors) {
    SDL_Surface* surface = SDL_CreateSurface(cellW * static_cast<int>(colors.size()), cellH, SDL_PIXELFORMAT_RGBA32);
    ASSERT_NE(surface, nullptr) << SDL_GetError();
    for (std::size_t c = 0; c < colors.size(); ++c) {
        for (int y = 0; y < cellH; ++y) {
            auto* row = static_cast<std::uint8_t*>(surface->pixels) + y * surface->pitch;
            for (int x = 0; x < cellW; ++x) {
                std::uint8_t* px = row + (static_cast<int>(c) * cellW + x) * 4;
                px[0] = colors[c].r;
                px[1] = colors[c].g;
                px[2] = colors[c].b;
                px[3] = 255;
            }
        }
    }
    ASSERT_TRUE(SDL_SavePNG(surface, path.string().c_str())) << SDL_GetError();
    SDL_DestroySurface(surface);
}

void writeText(const fs::path& path, const std::string& text) {
    std::ofstream out(path, std::ios::binary);
    out << text;
}

constexpr const char* kLine = "ch01.zhenshang.arrive";

class BustShotTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        root_ = std::make_unique<fanren::test::TempDir>("fanren_bust_root");
        for (const char* dir : {"data", "scripts", "maps"}) {
            fs::copy(fs::path(repoRoot()) / dir, root_->path() / dir, fs::copy_options::recursive);
        }
        fs::create_directories(root_->path() / "scripts" / "test_bust");
        writeText(root_->path() / "scripts" / "test_bust" / "three.lua",
                  std::string("talk(\"chen_shimei\", \"") + kLine + "\")\ntalk(\"chen_shimei\", \"" + kLine +
                      "\")\ntalk(\"chen_shimei\", \"" + kLine + "\")\nflag.set(\"test_bust.end\", 1)\n");
        writeText(root_->path() / "scripts" / "test_bust" / "bubble.lua",
                  std::string("bubble(\"chen_shimei\", \"") + kLine + "\", 2)\n");
    }
    static void TearDownTestSuite() { root_.reset(); }

    void SetUp() override {
        SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
        SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
        SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "dummy");
        dir_ = fanren::test::uniqueTempPath("fanren_busts");
        fs::create_directories(dir_);
        const fs::path art = root_->path() / "assets" / "art";
        fs::create_directories(art / "busts");
        fs::create_directories(art / "portraits");
        writeSolid(art / "busts" / "fixture.png", kCellW, kCellH,
                   {kFaceColors[0], kFaceColors[1], kFaceColors[2], kFaceColors[3]});
        writeSolid(art / "portraits" / "fixture.png", 56, 64, {kPortraitColor});
        // 小头像：陈巧倩与韩立都有（同一格品红）；立绘只有陈巧倩。
        writeText(root_->path() / "data" / "visual" / "portraits.json",
                  R"({"portraits": {
                      "hanli": [{"file": "fixture.png", "frame_w": 56, "frame_h": 64, "cols": 1,
                                 "faces": {"normal": 0, "smile": 0, "angry": 0, "hurt": 0}}],
                      "chen_shimei": [{"file": "fixture.png", "frame_w": 56, "frame_h": 64, "cols": 1,
                                 "faces": {"normal": 0, "smile": 0, "angry": 0, "hurt": 0}}]}})");
        useBusts("fixture.png");
    }
    void TearDown() override {
        SDL_ResetHint(SDL_HINT_VIDEO_DRIVER);
        SDL_ResetHint(SDL_HINT_RENDER_DRIVER);
        SDL_ResetHint(SDL_HINT_AUDIO_DRIVER);
        if (std::getenv("FANREN_KEEP_SHOTS") != nullptr) {
            std::printf("[bust shots] %s\n", dir_.string().c_str());
            return;
        }
        std::error_code ignored;
        fs::remove_all(dir_, ignored);
    }

    static void useBusts(const std::string& file) {
        writeText(root_->path() / "data" / "visual" / "busts.json",
                  R"({"busts": {"chen_shimei": [{"file": ")" + file + R"(", "frame_w": 168, "frame_h": 252, "cols": 4,
                      "faces": {"normal": 0, "smile": 1, "angry": 2, "hurt": 3}}]}})");
    }
    static void useBustText(const std::string& text) {
        writeText(root_->path() / "data" / "visual" / "busts.json", text);
    }

    bool open(fanren::game::Application& app, bool headless = false) {
        const auto ready = app.init(root_->path().string(), headless);
        EXPECT_TRUE(ready.ok) << ready.error;
        return ready.ok;
    }
    void tick(fanren::game::Application& app, int frames) {
        for (int i = 0; i < frames; ++i) app.tick(kFrame);
    }
    Frame capture(fanren::game::Application& app) {
        Frame frame;
        app.engine().beginFrame();
        app.drawScenes();
        const fs::path png = dir_ / ("shot" + std::to_string(shots_++) + ".png");
        const auto shot = app.engine().captureFrame(png.string());
        app.engine().endFrame();
        EXPECT_TRUE(shot.ok) << shot.error;
        if (!shot.ok) return frame;
        SDL_Surface* loaded = SDL_LoadPNG(png.string().c_str());
        if (loaded == nullptr) return frame;
        SDL_Surface* rgba = SDL_ConvertSurface(loaded, SDL_PIXELFORMAT_RGBA32);
        SDL_DestroySurface(loaded);
        if (rgba == nullptr) return frame;
        frame.w = rgba->w;
        frame.h = rgba->h;
        for (int y = 0; y < frame.h; ++y) {
            const auto* row = static_cast<const std::uint8_t*>(rgba->pixels) + y * rgba->pitch;
            for (int x = 0; x < frame.w; ++x) frame.pixels.push_back(Rgb{row[x * 4], row[x * 4 + 1], row[x * 4 + 2]});
        }
        SDL_DestroySurface(rgba);
        return frame;
    }
    // 一句话拍一张：sayAs 压对话框，空转到淡入走完、字全显。
    Frame say(const std::string& role, const std::string& face, std::string* err = nullptr) {
        fanren::game::Application app;
        if (!open(app)) return {};
        if (err != nullptr) ::testing::internal::CaptureStderr();
        app.sayAs(role, kLine, face);
        tick(app, 120);
        Frame frame = capture(app);
        if (err != nullptr) *err = ::testing::internal::GetCapturedStderr();
        app.shutdown();
        return frame;
    }

    // 立绘中间那一点（框在底部时）：x 在立绘正中，y 在框顶以上。
    static constexpr int kProbeX = 1010;
    static constexpr int kProbeY = 250;

    static int portraitPixels(const Frame& frame) {
        int n = 0;
        for (int y = 472; y < 672 && y < frame.h; ++y) {
            for (int x = 48; x < 400 && x < frame.w; ++x) n += near(frame.at(x, y), kPortraitColor, 0) ? 1 : 0;
        }
        return n;
    }

    static inline std::unique_ptr<fanren::test::TempDir> root_;
    fs::path dir_;
    int shots_ = 0;
};

TEST_F(BustShotTest, AWomanWithABustStandsRightOfTheBoxAndHerPixelPortraitIsNotDrawn) {
    const Frame smile = say("chen_shimei", "smile");
    ASSERT_EQ(smile.w, 1280);
    EXPECT_TRUE(near(smile.at(kProbeX, kProbeY), kFaceColors[1])) << "立绘画的是 smile 那一格";
    EXPECT_TRUE(near(smile.at(kProbeX, 40), kFaceColors[1])) << "立绘一直画到屏幕顶上那一截";
    EXPECT_FALSE(near(smile.at(780, kProbeY), kFaceColors[1])) << "立绘左边沿在 800";
    EXPECT_FALSE(near(smile.at(kProbeX, 560), kFaceColors[1])) << "框后面那截不画（框半透明，不透出来）";
    EXPECT_EQ(portraitPixels(smile), 0) << "有立绘：框里的像素小头像不画";

    const Frame hurt = say("chen_shimei", "hurt");
    EXPECT_TRUE(near(hurt.at(kProbeX, kProbeY), kFaceColors[3])) << "表情跟着 talk 的第三个参数走";
}

TEST_F(BustShotTest, AManWithoutABustKeepsHisPixelPortraitAndNothingStandsOnTheRight) {
    const Frame hanli = say("hanli", "normal");
    ASSERT_EQ(hanli.w, 1280);
    EXPECT_GT(portraitPixels(hanli), 112 * 128 - 10) << "没有立绘：小头像照旧";
    for (const Rgb& color : kFaceColors) EXPECT_FALSE(near(hanli.at(kProbeX, kProbeY), color));
}

TEST_F(BustShotTest, AMissingImageFallsBackToThePixelPortraitWithOneWarning) {
    useBusts("missing.png");
    std::string err;
    const Frame frame = say("chen_shimei", "normal", &err);
    EXPECT_NE(err.find("[bust]"), std::string::npos) << err;
    EXPECT_NE(err.find("missing.png"), std::string::npos) << err;
    EXPECT_GT(portraitPixels(frame), 112 * 128 - 10) << "缺图当作没有立绘：小头像照旧";
    EXPECT_FALSE(near(frame.at(kProbeX, kProbeY), kFaceColors[0]));
}

TEST_F(BustShotTest, ABrokenTableFallsBackToThePixelPortraitWithOneWarning) {
    useBustText("{\"busts\": {\"chen_shimei\": ");
    std::string err;
    const Frame frame = say("chen_shimei", "normal", &err);
    EXPECT_NE(err.find("[bust]"), std::string::npos) << err;
    EXPECT_NE(err.find("busts.json"), std::string::npos) << err;
    EXPECT_GT(portraitPixels(frame), 112 * 128 - 10);
}

TEST_F(BustShotTest, AnUnknownFaceIsDrawnAsNormalWithAWarning) {
    std::string err;
    const Frame frame = say("chen_shimei", "sad", &err);
    EXPECT_TRUE(near(frame.at(kProbeX, kProbeY), kFaceColors[0])) << "认不出的表情按 normal 画";
    EXPECT_NE(err.find("sad"), std::string::npos) << err;
    EXPECT_EQ(portraitPixels(frame), 0);
}

TEST_F(BustShotTest, HeadlessNeverReadsTheTableNorWarns) {
    useBusts("missing.png");
    fanren::game::Application app;
    ASSERT_TRUE(open(app, /*headless=*/true));
    ::testing::internal::CaptureStderr();
    app.sayAs("chen_shimei", kLine, "smile");
    tick(app, 3);
    app.drawScenes();
    const std::string err = ::testing::internal::GetCapturedStderr();
    auto* dialogue = dynamic_cast<fanren::game::DialogueScene*>(app.topScene());
    ASSERT_NE(dialogue, nullptr);
    EXPECT_FALSE(dialogue->showsBust());
    EXPECT_FALSE(app.busts().tableLoaded()) << "无头模式不读立绘表";
    EXPECT_EQ(err.find("[bust]"), std::string::npos) << err;
    app.shutdown();
}

TEST_F(BustShotTest, HanLiTakingOverMakesHerFadeOutWhileHisPortraitShows) {
    fanren::game::Application app;
    ASSERT_TRUE(open(app));
    app.sayAs("chen_shimei", kLine, "normal");
    tick(app, 30);
    ASSERT_EQ(app.bustStage().layers().size(), 1u);
    EXPECT_FLOAT_EQ(app.bustStage().layers()[0].alpha, 1.f);
    app.popScene();   // 这一句收起（游戏里是按确认）
    tick(app, 1);     // 换句之间那一帧空档
    EXPECT_FALSE(app.bustStage().empty());
    app.sayAs("hanli", kLine, "normal");
    tick(app, 3);
    ASSERT_EQ(app.bustStage().layers().size(), 1u);
    EXPECT_EQ(app.bustStage().layers()[0].shot.role, "chen_shimei");
    EXPECT_LT(app.bustStage().layers()[0].alpha, 1.f) << "她在淡出";
    EXPECT_GT(app.bustStage().layers()[0].alpha, 0.f);
    auto* dialogue = dynamic_cast<fanren::game::DialogueScene*>(app.topScene());
    ASSERT_NE(dialogue, nullptr);
    EXPECT_FALSE(dialogue->showsBust());
    tick(app, 120);
    EXPECT_TRUE(app.bustStage().layers().empty()) << "0.15 秒后淡完";
    const Frame frame = capture(app);
    EXPECT_GT(portraitPixels(frame), 112 * 128 - 10) << "韩立的小头像照旧";
    app.shutdown();
}

TEST_F(BustShotTest, StackedLinesHandTheBustOverWhenTheTopOneCloses) {
    // 路径行动一次压好几层对话框（后进先出，最上面那层先演）：立绘跟着「露在最上面的那一层」走，不跟 onEnter 的次序。
    fanren::game::Application app;
    ASSERT_TRUE(open(app));
    auto narration = std::make_unique<fanren::game::DialogueScene>(std::string{}, app.text(kLine));
    auto chen = std::make_unique<fanren::game::DialogueScene>(app.speakerName("chen_shimei"), app.text(kLine));
    chen->setPortrait("chen_shimei", "normal");
    const std::uint64_t chenSerial = chen->serial();
    app.pushScene(std::move(narration));
    app.pushScene(std::move(chen));
    tick(app, 30);
    EXPECT_EQ(app.bustOwner(), chenSerial) << "只有最上面那一层画立绘";
    EXPECT_EQ(app.bustStage().currentRole(), "chen_shimei");
    app.popScene();   // 她那句说完，露出下面的旁白
    tick(app, 3);
    EXPECT_NE(app.bustOwner(), chenSerial);
    EXPECT_TRUE(app.bustStage().currentRole().empty()) << "旁白那一层接手：她淡出";
    ASSERT_EQ(app.bustStage().layers().size(), 1u);
    EXPECT_LT(app.bustStage().layers()[0].alpha, 1.f);
    app.shutdown();
}

TEST_F(BustShotTest, TheLogPanelShowsNoBust) {
    fanren::game::Application app;
    ASSERT_TRUE(open(app));
    app.sayAs("chen_shimei", kLine, "smile");
    tick(app, 120);
    ASSERT_TRUE(near(capture(app).at(kProbeX, kProbeY), kFaceColors[1]));
    SDL_Event down{};
    down.type = SDL_EVENT_KEY_DOWN;
    down.key.scancode = SDL_SCANCODE_TAB;
    down.key.down = true;
    SDL_PushEvent(&down);
    tick(app, 1);
    SDL_Event up = down;
    up.type = SDL_EVENT_KEY_UP;
    up.key.down = false;
    SDL_PushEvent(&up);
    tick(app, 2);
    const Frame log = capture(app);
    for (const Rgb& color : kFaceColors) EXPECT_FALSE(near(log.at(kProbeX, kProbeY), color)) << "回看面板里不画立绘";
    app.shutdown();
}

TEST_F(BustShotTest, SkippingTheWholeSceneLeavesNoBustBehind) {
    fanren::game::Application app;
    ASSERT_TRUE(open(app));
    ASSERT_TRUE(app.startEvent("test_bust/three.lua").ok);
    tick(app, 30);
    auto* dialogue = dynamic_cast<fanren::game::DialogueScene*>(app.topScene());
    ASSERT_NE(dialogue, nullptr);
    EXPECT_TRUE(dialogue->showsBust());
    ASSERT_TRUE(app.canSkipDialogue());
    dialogue->openSkipAsk(app);
    app.beginDialogueSkip();   // 确认框里选「跳过」走的就是它（DialogueScene::update），之后这一句收起
    app.popScene();
    app.completeCommand(fanren::script::CommandResult{true});
    tick(app, 5);
    EXPECT_EQ(app.state().flag("test_bust.end"), 1) << "跳到了脚本结束";
    EXPECT_EQ(dynamic_cast<fanren::game::DialogueScene*>(app.topScene()), nullptr);
    tick(app, 30);
    EXPECT_TRUE(app.bustStage().empty()) << "跳过之后不留立绘";
    app.shutdown();
}

TEST_F(BustShotTest, ABubbleThatFallsBackToADialogueCarriesNoBust) {
    fanren::game::Application app;
    ASSERT_TRUE(open(app));
    ASSERT_TRUE(app.startEvent("test_bust/bubble.lua").ok);   // 没有世界层：她不在画面上，气泡退回成对白
    tick(app, 30);
    auto* dialogue = dynamic_cast<fanren::game::DialogueScene*>(app.topScene());
    ASSERT_NE(dialogue, nullptr);
    EXPECT_EQ(dialogue->speakerRole(), "chen_shimei");
    EXPECT_FALSE(dialogue->showsBust()) << "插话不出大图";
    const Frame frame = capture(app);
    EXPECT_GT(portraitPixels(frame), 112 * 128 - 10) << "小头像照旧";
    app.shutdown();
}

}  // namespace
