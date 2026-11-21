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
#include "core/rules/PathActions.h"
#include "engine/TextLayout.h"
#include "game/Application.h"
#include "game/Busts.h"
#include "game/DialogueScene.h"
#include "game/IntroCardScene.h"
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

TEST(BustLayout, WithTheBoxAtTheBottomTheCellTopSitsAt18AndOnlyTheRowsAboveTheBoxAreDrawn) {
    const Rect bust = fanren::game::bustDialogueArea(kBottomBox, kBottomBox);
    EXPECT_EQ(bust.w, 420);
    EXPECT_EQ(bust.h, 630);
    EXPECT_EQ(bust.x + bust.w, 1280 - 60) << "右边距 60";
    EXPECT_EQ(bust.y, 18) << "施工图 3.5.1：格子顶边 y = 18";
    EXPECT_EQ(bust.y + fanren::game::kBustHeadLine, 48) << "头顶约在 y = 48";
    EXPECT_EQ(bust.h - fanren::game::kBustHeadLine, 600) << "人物从头顶到格底在屏幕上高 600";
    // 框是半透明的：框后面那一截不画，画到框顶金线下 3 像素。
    EXPECT_EQ(fanren::game::bustVisibleRows(bust, kBottomBox), kBottomBox.y + 3 - bust.y);
}

TEST(BustLayout, ATallerChoiceBoxDoesNotLiftTheBust) {
    const Rect tall{48, 300, 1184, 372};
    const Rect bust = fanren::game::bustDialogueArea(tall, tall);
    EXPECT_EQ(bust.y, 18) << "框因选项长高了，立绘不跟着往上抬";
    EXPECT_EQ(fanren::game::bustVisibleRows(bust, tall), tall.y + 3 - bust.y) << "多出来的那截由框压住";
}

TEST(BustLayout, WithTheBoxRaisedTheBustStandsOnWhicheverSideHasMoreRoom) {
    // 框贴到顶边（e02b 那样，框 184–382）：上方余地 136 < 下方余地 326 → 站框下，头顶在框底以下 12，整幅画、下半截出屏。
    const Rect top{48, 184, 1184, 198};
    const Rect atTop = fanren::game::bustDialogueArea(top, kBottomBox);
    EXPECT_EQ(atTop.y + fanren::game::kBustHeadLine, top.y + top.h + fanren::game::kBustHeadGap);
    EXPECT_GT(atTop.y + atTop.h, 720);
    EXPECT_EQ(atTop.x + atTop.w, 1220);
    EXPECT_EQ(fanren::game::bustVisibleRows(atTop, top), 630) << "站框下：整幅都画";
    // 框只抬到偏低的半中腰（e02c 那样，框 378–574）：上方余地 330 ≥ 下方余地 134 → 照常态摆，只画到框顶金线下 3。
    const Rect low{48, 378, 1184, 196};
    const Rect atLow = fanren::game::bustDialogueArea(low, kBottomBox);
    EXPECT_EQ(atLow.y, 18);
    EXPECT_EQ(fanren::game::bustVisibleRows(atLow, low), low.y + 3 - 18) << "露头到胸";
    // 两边一样大：站上方（「上方不小于下方」）。框高 200：框顶 y 使 y − 48 = 720 − (y + 212) → y = 278。
    const Rect even{48, 278, 1184, 200};
    EXPECT_EQ(fanren::game::bustDialogueArea(even, kBottomBox).y, 18);
    const Rect justBelow{48, 277, 1184, 200};
    EXPECT_EQ(fanren::game::bustDialogueArea(justBelow, kBottomBox).y + fanren::game::kBustHeadLine, 277 + 200 + 12);
}

TEST(BustLayout, WhenTheHeroWouldBeCoveredTheBustStandsOnTheLeft) {
    const Rect bust = fanren::game::bustDialogueArea(kBottomBox, kBottomBox);
    const int rows = fanren::game::bustVisibleRows(bust, kBottomBox);
    // 主角站在右边（镜头贴地图右沿）：与画出来的那一块相交 → 改站左侧，格子左边 x = 160，大小不变。
    const Rect heroRight{900, 150, 144, 144};
    const Rect moved = fanren::game::bustAvoidHero(bust, rows, heroRight);
    EXPECT_EQ(moved.x, 160);
    EXPECT_EQ(moved.y, bust.y);
    EXPECT_EQ(moved.w, bust.w);
    EXPECT_EQ(moved.h, bust.h);
    // 主角在中间、或在右边但在框后那截（不画的地方）：不动。
    EXPECT_EQ(fanren::game::bustAvoidHero(bust, rows, Rect{568, 200, 144, 144}).x, 800);
    EXPECT_EQ(fanren::game::bustAvoidHero(bust, rows, Rect{900, 480, 144, 144}).x, 800) << "框后那截不画，不算挡";
    // 没有世界层：主角矩形为空，不动。
    EXPECT_EQ(fanren::game::bustAvoidHero(bust, rows, Rect{}).x, 800);
}

TEST(BustLayout, TheRosterShowsTheWholePersonAt560High) {
    const RectF src = fanren::game::bustRosterSource(RectF{0.f, 0.f, 840.f, 1260.f});
    EXPECT_FLOAT_EQ(src.x, 0.f) << "左右不裁：最宽的一位约 827";
    EXPECT_FLOAT_EQ(src.y, 20.f) << "头顶线 60 以上留 20";
    EXPECT_FLOAT_EQ(src.w, 840.f);
    EXPECT_FLOAT_EQ(src.h, 1240.f);
    const RectF dst = fanren::game::bustRosterArea(Rect{100, 50, 380, 600}, src);
    EXPECT_FLOAT_EQ(dst.h, 560.f);
    EXPECT_NEAR(dst.w, 560.f * 840.f / 1240.f, 0.01f);
    EXPECT_LE(dst.w, 380.f) << "放得进那一栏";
    EXPECT_FLOAT_EQ(dst.y + dst.h, 650.f) << "底边贴那一栏的底";
    EXPECT_NEAR(dst.x + dst.w / 2.f, 290.f, 0.01f) << "水平居中";
    EXPECT_FLOAT_EQ(fanren::game::bustRosterArea(Rect{0, 0, 380, 400}, src).h, 400.f) << "那一栏不够高：按栏高缩";
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
        writeText(root_->path() / "scripts" / "test_bust" / "card.lua",
                  "intro_card(\"chen_shimei\")\nflag.set(\"test_bust.card\", 1)\n");
        writeText(root_->path() / "scripts" / "test_bust" / "card_nobody.lua",
                  "intro_card(\"hanli\")\nflag.set(\"test_bust.card\", 1)\n");
        writeText(root_->path() / "scripts" / "test_bust" / "card_skip.lua",
                  std::string("talk(\"\", \"") + kLine + "\")\nintro_card(\"chen_shimei\")\ntalk(\"\", \"" + kLine +
                      "\")\nflag.set(\"test_bust.card\", 1)\n");
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

// ---------------------------------------------------------------------------
// 登场卡（3.5.3）
// ---------------------------------------------------------------------------

TEST(IntroCard, TheTimelineSlidesInOverAQuarterSecondAndFadesOutInAFifth) {
    using fanren::game::IntroCardScene;
    EXPECT_FLOAT_EQ(IntroCardScene::opacityAt(0.0, false, 0.0), 0.f);
    EXPECT_NEAR(IntroCardScene::opacityAt(0.125, false, 0.0), 0.5f, 1e-4f);
    EXPECT_FLOAT_EQ(IntroCardScene::opacityAt(0.25, false, 0.0), 1.f);
    EXPECT_NEAR(IntroCardScene::opacityAt(1.0, true, 0.1), 0.5f, 1e-4f);
    EXPECT_FLOAT_EQ(IntroCardScene::opacityAt(1.0, true, 0.2), 0.f);
    EXPECT_FLOAT_EQ(IntroCardScene::slideOffsetAt(0.0), static_cast<float>(IntroCardScene::kSlideFrom))
        << "从右边滑进来";
    EXPECT_FLOAT_EQ(IntroCardScene::slideOffsetAt(0.25), 0.f);
    EXPECT_DOUBLE_EQ(IntroCardScene::kArmSeconds, 0.6);
    EXPECT_DOUBLE_EQ(IntroCardScene::kAutoSeconds, 2.5);
    const RectF bust = IntroCardScene::bustRect(fanren::game::bustRosterSource(RectF{0.f, 0.f, 840.f, 1260.f}));
    EXPECT_FLOAT_EQ(bust.h, 660.f) << "高约 660";
    EXPECT_GT(bust.x + bust.w / 2.f, 640.f) << "停在中间偏右";
    EXPECT_LE(bust.x + bust.w, 1280.f);
    EXPECT_FLOAT_EQ(bust.y + bust.h, 720.f);
}

TEST_F(BustShotTest, HeadlessCardsFinishAtOnceAndChangeNothing) {
    fanren::game::Application app;
    ASSERT_TRUE(open(app, /*headless=*/true));
    const auto spokenBefore = app.spokenKeys().size();
    const auto logBefore = app.dialogueLog().size();
    ASSERT_TRUE(app.startEvent("test_bust/card.lua").ok);
    tick(app, 3);
    EXPECT_EQ(app.state().flag("test_bust.card"), 1) << "卡片立即完成，脚本照常往下走";
    EXPECT_EQ(dynamic_cast<fanren::game::IntroCardScene*>(app.topScene()), nullptr);
    EXPECT_EQ(app.spokenKeys().size(), spokenBefore) << "不记进已说的话";
    EXPECT_EQ(app.dialogueLog().size(), logBefore) << "不进回看";
    EXPECT_FALSE(app.busts().tableLoaded()) << "无头不读表";
    app.shutdown();
}

TEST_F(BustShotTest, ACardWaitsSixTenthsBeforeConfirmThenFadesAndTheScriptGoesOn) {
    fanren::game::Application app;
    ASSERT_TRUE(open(app));
    ASSERT_TRUE(app.startEvent("test_bust/card.lua").ok);
    tick(app, 3);
    auto* card = dynamic_cast<fanren::game::IntroCardScene*>(app.topScene());
    ASSERT_NE(card, nullptr);
    EXPECT_EQ(card->role(), "chen_shimei");
    const auto press = [&](SDL_Scancode code) {
        SDL_Event e{};
        e.type = SDL_EVENT_KEY_DOWN;
        e.key.scancode = code;
        e.key.down = true;
        SDL_PushEvent(&e);
        tick(app, 1);
        e.type = SDL_EVENT_KEY_UP;
        e.key.down = false;
        SDL_PushEvent(&e);
        tick(app, 1);
    };
    press(SDL_SCANCODE_RETURN);   // 0.6 秒之内：不收
    ASSERT_EQ(app.topScene(), card);
    EXPECT_FALSE(card->closing());
    tick(app, 40);
    press(SDL_SCANCODE_RETURN);
    ASSERT_EQ(app.topScene(), card);
    EXPECT_TRUE(card->closing()) << "按确认：开始淡出";
    EXPECT_EQ(app.state().flag("test_bust.card"), 0) << "淡完之前脚本还在等";
    tick(app, 20);
    EXPECT_EQ(dynamic_cast<fanren::game::IntroCardScene*>(app.topScene()), nullptr);
    EXPECT_EQ(app.state().flag("test_bust.card"), 1);
    app.shutdown();
}

TEST_F(BustShotTest, WithAutoAdvanceACardClosesItself) {
    fanren::game::Application app;
    ASSERT_TRUE(open(app));
    fanren::io::Settings next = app.settings();
    next.autoAdvance = true;
    app.setSettings(next);
    ASSERT_TRUE(app.startEvent("test_bust/card.lua").ok);
    tick(app, 3);
    ASSERT_NE(dynamic_cast<fanren::game::IntroCardScene*>(app.topScene()), nullptr);
    tick(app, 140);   // 约 2.4 秒：还在
    EXPECT_NE(dynamic_cast<fanren::game::IntroCardScene*>(app.topScene()), nullptr);
    tick(app, 30);    // 过了 2.5 秒、再淡 0.2 秒
    EXPECT_EQ(dynamic_cast<fanren::game::IntroCardScene*>(app.topScene()), nullptr);
    EXPECT_EQ(app.state().flag("test_bust.card"), 1);
    app.shutdown();
}

TEST_F(BustShotTest, SomeoneWithoutABustGetsNoCardAndOneWarning) {
    fanren::game::Application app;
    ASSERT_TRUE(open(app));
    ::testing::internal::CaptureStderr();
    ASSERT_TRUE(app.startEvent("test_bust/card_nobody.lua").ok);
    tick(app, 3);
    ASSERT_TRUE(app.startEvent("test_bust/card_nobody.lua").ok);
    tick(app, 3);
    const std::string err = ::testing::internal::GetCapturedStderr();
    EXPECT_EQ(dynamic_cast<fanren::game::IntroCardScene*>(app.topScene()), nullptr);
    EXPECT_EQ(app.state().flag("test_bust.card"), 1) << "脚本照常往下走";
    const auto first = err.find("hanli");
    ASSERT_NE(first, std::string::npos) << err;
    EXPECT_EQ(err.find("hanli", first + 1), std::string::npos) << "一个角色只警告一次：" << err;
    app.shutdown();
}

TEST(IntroCard, TheTourScriptRunsAndEveryCardHasItsText) {
    // 「看立绘-七位巡览」用 --event 演 scripts/dev/heroine_tour.lua：七张卡、每人一句。无头下卡片立即完成，停在第一句。
    fanren::game::Application app;
    ASSERT_TRUE(app.init(repoRoot(), /*headless=*/true).ok);
    ASSERT_TRUE(app.startEvent("dev/heroine_tour.lua").ok);
    for (int i = 0; i < 3; ++i) app.tick(kFrame);
    auto* dialogue = dynamic_cast<fanren::game::DialogueScene*>(app.topScene());
    ASSERT_NE(dialogue, nullptr);
    EXPECT_EQ(dialogue->speakerRole(), "chen_shimei");
    // 七位的身份与短评都在文本表里；短评 16 字以内（3.5.3）。
    for (const char* role : {"chen_shimei", "nichang_xianzi", "duobao_nv", "huangshan_shijie", "baiyi_shaonv",
                             "nangong_wan_ch07", "han_yunzhi"}) {
        const std::string base = std::string("card.") + role + ".";
        EXPECT_NE(app.text(base + "title"), base + "title") << role;
        const std::string line = app.text(base + "line");
        ASSERT_NE(line, base + "line") << role;
        EXPECT_LE(fanren::engine::splitGraphemes(line).size(), 17u) << role << "：" << line;   // 16 字 + 句号
    }
    EXPECT_EQ(app.text("card.baiyi_shaonv.line").find("南宫"), std::string::npos) << "问名之前的那张不点名";
    app.shutdown();
}

// ---------------------------------------------------------------------------
// 路径行动带表情（第三轮：禁地荒山陈巧倩那句打探配冷脸）
// ---------------------------------------------------------------------------

TEST(PathActionFace, TheFaceRidesOnTheTextKeyLineOnly) {
    fanren::core::PathAction ask;
    ask.id = "x_dating";
    ask.kind = fanren::core::PathActionKind::Inquire;
    ask.textKey = "t.x";
    ask.doneFlag = "ch07.path.x_dating";
    fanren::core::GameState state;
    std::vector<fanren::rules::PathEffect> effects = fanren::rules::pathActionEffects(ask, state);
    ASSERT_FALSE(effects.empty());
    EXPECT_TRUE(effects[0].face.empty()) << "不写 face：照旧（对话框按 normal）";
    ask.face = "angry";
    effects = fanren::rules::pathActionEffects(ask, state);
    ASSERT_FALSE(effects.empty());
    EXPECT_EQ(effects[0].kind, fanren::rules::PathEffect::Kind::Say);
    EXPECT_EQ(effects[0].face, "angry");
}

TEST(PathActionFace, ChenDatingIsSaidWithTheColdFaceAndOthersStayNormal) {
    fanren::game::Application app;
    ASSERT_TRUE(app.init(repoRoot(), /*headless=*/true).ok);
    const fanren::core::PathAction* chen = nullptr;
    const fanren::core::PathAction* plain = nullptr;
    for (const fanren::core::PathAction& action : app.data().pathActions) {
        if (action.id == "chen_dating") chen = &action;
        if (plain == nullptr && action.kind == fanren::core::PathActionKind::Inquire && action.face.empty()) {
            plain = &action;
        }
    }
    ASSERT_NE(chen, nullptr);
    ASSERT_NE(plain, nullptr);
    EXPECT_EQ(chen->face, "angry") << "data/pathactions/ch07.json 的 chen_dating 写了 face";
    const auto spoken = [&app](const fanren::core::PathAction& action, const std::string& role) {
        app.applyPathEffects(action, fanren::rules::pathActionEffects(action, app.state()), role);
        app.tick(kFrame);
        auto* dialogue = dynamic_cast<fanren::game::DialogueScene*>(app.topScene());
        std::string face = dialogue == nullptr ? std::string("(没有对话框)") : dialogue->face();
        while (dynamic_cast<fanren::game::DialogueScene*>(app.topScene()) != nullptr) {
            app.popScene();
            app.tick(kFrame);
        }
        return face;
    };
    EXPECT_EQ(spoken(*chen, "chen_shimei"), "angry") << "表情一路送到对话框";
    EXPECT_EQ(spoken(*plain, "hanli"), "normal") << "不带表情的照旧是 normal";
    app.shutdown();
}

TEST_F(BustShotTest, SkippingTheWholeSceneSkipsTheCardToo) {
    fanren::game::Application app;
    ASSERT_TRUE(open(app));
    ASSERT_TRUE(app.startEvent("test_bust/card_skip.lua").ok);
    tick(app, 30);
    ASSERT_NE(dynamic_cast<fanren::game::DialogueScene*>(app.topScene()), nullptr);
    ASSERT_TRUE(app.canSkipDialogue());
    app.beginDialogueSkip();
    app.popScene();
    app.completeCommand(fanren::script::CommandResult{true});
    tick(app, 5);
    EXPECT_EQ(app.state().flag("test_bust.card"), 1) << "跳到了脚本结束，卡片没拦住";
    EXPECT_EQ(dynamic_cast<fanren::game::IntroCardScene*>(app.topScene()), nullptr);
    app.shutdown();
}

}  // namespace
