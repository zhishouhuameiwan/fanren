// 伙伴改版 · 对白头像（docs/interfaces-companions.md 2.5、3.9、第 5 节「头像」那条；表的形状见 docs/art-sprites.md 5.3）。
//
// 三组：
//   · 读取器（io/PortraitLoader）：四个表情都要写、认不出的键整份报错、空表合法，仓库里那份读得进来；
//   · 摆位（game/Portraits.h 的纯函数）：定稿头像 56×64 按 ×2 摆、正文让开；
//   · 截图：软件渲染器（dummy 视频驱动）上开一台真 Application，走 Application::sayAs（脚本 talk 与截图口
//     talk:<key>:<角色>:<表情> 落到的同一处）压对话框，空转帧数与截图口相同，抓帧比像素。
//     头像那一块必须与夹具图集里「那个表情的那一格」按 ×2 放大后逐像素相同；没登记的角色与旁白，整帧与
//     没有任何头像表时逐像素相同。夹具图集在用例里用 SDL 自己画（不经被测的引擎）。
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

#include <SDL3/SDL.h>
#include <gtest/gtest.h>

#include "TempDir.h"
#include "game/Application.h"
#include "game/DialogueScene.h"
#include "game/Portraits.h"
#include "io/PortraitLoader.h"

namespace {

namespace fs = std::filesystem;
using fanren::core::PortraitTable;

std::string repoRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "maps" / "ch01_hanjiacun.tmj")) return candidate;
    }
    return ".";
}

constexpr const char* kGoodEntry =
    R"({"file": "a.png", "frame_w": 56, "frame_h": 64, "cols": 4,
        "faces": {"normal": 0, "smile": 1, "angry": 2, "hurt": 3}})";

std::string tableWith(const std::string& entry) {
    return std::string(R"({"_doc": "x", "portraits": {"nangong_wan": )") + entry + "}}";
}

// ---------------------------------------------------------------------------
// 读取器
// ---------------------------------------------------------------------------

TEST(PortraitLoader, ReadsAnEntryWithItsFourFaces) {
    const auto table = fanren::io::parsePortraits(tableWith(kGoodEntry));
    ASSERT_TRUE(table.ok) << table.error;
    ASSERT_EQ(table.value.size(), 1u);
    const auto& sheet = table.value.at("nangong_wan");
    EXPECT_EQ(sheet.file, "a.png");
    EXPECT_EQ(sheet.frameW, 56);
    EXPECT_EQ(sheet.frameH, 64);
    EXPECT_EQ(sheet.cols, 4);
    EXPECT_EQ(sheet.faces[0], 0);   // normal
    EXPECT_EQ(sheet.faces[1], 1);   // smile
    EXPECT_EQ(sheet.faces[2], 2);   // angry
    EXPECT_EQ(sheet.faces[3], 3);   // hurt
}

TEST(PortraitLoader, AnEmptyTableIsValid) {
    const auto table = fanren::io::parsePortraits(R"({"portraits": {}})");
    ASSERT_TRUE(table.ok) << table.error;
    EXPECT_TRUE(table.value.empty());
}

TEST(PortraitLoader, AMissingFaceRejectsTheWholeFile) {
    const auto table = fanren::io::parsePortraits(tableWith(
        R"({"file": "a.png", "frame_w": 56, "frame_h": 64, "cols": 4, "faces": {"normal": 0, "smile": 1, "angry": 2}})"));
    ASSERT_FALSE(table.ok);
    EXPECT_NE(table.error.find("portraits.nangong_wan.faces"), std::string::npos) << table.error;
    EXPECT_NE(table.error.find("hurt"), std::string::npos) << table.error;
}

TEST(PortraitLoader, UnknownFacesFieldsAndTopLevelKeysAreRejected) {
    EXPECT_FALSE(fanren::io::parsePortraits(tableWith(
                     R"({"file": "a.png", "frame_w": 56, "frame_h": 64, "cols": 4,
                         "faces": {"normal": 0, "smile": 1, "angry": 2, "hurt": 3, "sad": 4}})"))
                     .ok)
        << "认不出的表情";
    EXPECT_FALSE(fanren::io::parsePortraits(tableWith(
                     R"({"file": "a.png", "frame_w": 56, "frame_h": 64, "cols": 4, "scale": 2,
                         "faces": {"normal": 0, "smile": 1, "angry": 2, "hurt": 3}})"))
                     .ok)
        << "认不出的字段（引擎不判的字段不许写）";
    EXPECT_FALSE(fanren::io::parsePortraits(R"({"portraits": {}, "faces": {}})").ok) << "认不出的顶层键";
    EXPECT_FALSE(fanren::io::parsePortraits(R"({})").ok) << "portraits 必填";
}

TEST(PortraitLoader, BadNumbersAndMissingFieldsAreRejected) {
    EXPECT_FALSE(fanren::io::parsePortraits(tableWith(
                     R"({"file": "a.png", "frame_w": 0, "frame_h": 64, "cols": 4,
                         "faces": {"normal": 0, "smile": 1, "angry": 2, "hurt": 3}})"))
                     .ok);
    EXPECT_FALSE(fanren::io::parsePortraits(tableWith(
                     R"({"file": "a.png", "frame_w": 56, "frame_h": 64, "cols": 4,
                         "faces": {"normal": -1, "smile": 1, "angry": 2, "hurt": 3}})"))
                     .ok);
    EXPECT_FALSE(fanren::io::parsePortraits(tableWith(
                     R"({"file": "a.png", "frame_w": 56, "frame_h": 64,
                         "faces": {"normal": 0, "smile": 1, "angry": 2, "hurt": 3}})"))
                     .ok)
        << "cols 必填";
    EXPECT_FALSE(fanren::io::parsePortraits(tableWith(
                     R"({"file": "", "frame_w": 56, "frame_h": 64, "cols": 4,
                         "faces": {"normal": 0, "smile": 1, "angry": 2, "hurt": 3}})"))
                     .ok);
}

TEST(PortraitLoader, TheShippedTableLoads) {
    const fs::path path = fs::path(repoRoot()) / "data" / "visual" / "portraits.json";
    ASSERT_TRUE(fs::exists(path)) << path.string();
    const auto table = fanren::io::loadPortraits(path.string());
    EXPECT_TRUE(table.ok) << table.error;
}

// ---------------------------------------------------------------------------
// 摆位
// ---------------------------------------------------------------------------

TEST(PortraitLayout, TheFinalPortraitSizeIsDrawnAtTwiceAndTheTextMovesAside) {
    // 定稿样本的头像原生约 56×64（A 路任务书）：×2 = 112×128。
    EXPECT_EQ(fanren::game::portraitScale(56, 64), 2);
    EXPECT_EQ(fanren::game::portraitScale(64, 72), 2);
    EXPECT_EQ(fanren::game::portraitScale(32, 32), 4);
    EXPECT_EQ(fanren::game::portraitScale(200, 200), 1) << "放不下也至少 ×1，不缩小像素美术";
    // 对白框（无选项）：x 48、宽 1184、高 200，正文那一块四周再缩 24。
    const fanren::engine::Rect text{48, 472, 1184, 200};
    const auto layout = fanren::game::portraitLayout(text, 24, 56, 64);
    EXPECT_EQ(layout.portrait.x, 72) << "贴着缩进后的左边，不压框线";
    EXPECT_EQ(layout.portrait.y, 472 + 24 + 12) << "缩进后高 152，竖直居中：(152 − 128) / 2 = 12";
    EXPECT_EQ(layout.portrait.w, 112);
    EXPECT_EQ(layout.portrait.h, 128);
    EXPECT_EQ(layout.text.x, 48 + 112 + 20) << "正文第一个字落在 72 + 112 + 20";
    EXPECT_EQ(layout.text.w, 1184 - 132);
    EXPECT_EQ(layout.text.y, 472);
    EXPECT_EQ(layout.text.h, 200);
}

// ---------------------------------------------------------------------------
// 截图
// ---------------------------------------------------------------------------

struct Rgb {
    std::uint8_t r = 0, g = 0, b = 0;
    bool operator==(const Rgb&) const = default;
};

struct Frame {
    int w = 0;
    int h = 0;
    std::vector<Rgb> pixels;
    [[nodiscard]] Rgb at(int x, int y) const {
        return pixels[static_cast<std::size_t>(y) * static_cast<std::size_t>(w) + static_cast<std::size_t>(x)];
    }
};

// 夹具图集：4 列 1 行、每格 56×64，四个表情四格。每个像素的颜色由（表情, x, y）算出来，四格彼此全不相同，
// 一格里的像素也几乎各不相同——取错格、错位、缩放走了样都比得出来。全不透明：比的是图集像素本身。
constexpr int kFixtureW = 56;
constexpr int kFixtureH = 64;

Rgb fixturePixel(int face, int x, int y) {
    return Rgb{static_cast<std::uint8_t>(30 + face * 55), static_cast<std::uint8_t>((x * 4) % 256),
               static_cast<std::uint8_t>((y * 3 + face * 17) % 256)};
}

void writeFixtureAtlas(const fs::path& path) {
    SDL_Surface* surface = SDL_CreateSurface(kFixtureW * 4, kFixtureH, SDL_PIXELFORMAT_RGBA32);
    ASSERT_NE(surface, nullptr) << SDL_GetError();
    for (int face = 0; face < 4; ++face) {
        for (int y = 0; y < kFixtureH; ++y) {
            for (int x = 0; x < kFixtureW; ++x) {
                const Rgb c = fixturePixel(face, x, y);
                auto* row = static_cast<std::uint8_t*>(surface->pixels) + y * surface->pitch;
                std::uint8_t* px = row + (face * kFixtureW + x) * 4;
                px[0] = c.r;
                px[1] = c.g;
                px[2] = c.b;
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

// 用例结束时把头像来源还给游戏缺省，不留给同进程里后面的用例。
struct SourceGuard {
    explicit SourceGuard(fanren::game::PortraitSource source) { fanren::game::setPortraitSource(std::move(source)); }
    ~SourceGuard() { fanren::game::setPortraitSource({}); }
    SourceGuard(const SourceGuard&) = delete;
    SourceGuard& operator=(const SourceGuard&) = delete;
};

class PortraitShot : public ::testing::Test {
protected:
    void SetUp() override {
        SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
        SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
        SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "dummy");
        dir_ = fanren::test::uniqueTempPath("fanren_portraits");
        fs::create_directories(dir_);
        writeFixtureAtlas(dir_ / "fixture.png");
        writeText(dir_ / "portraits.json",
                  R"({"portraits": {"hanli": {"file": "fixture.png", "frame_w": 56, "frame_h": 64, "cols": 4,
                      "faces": {"normal": 0, "smile": 1, "angry": 2, "hurt": 3}}}})");
        writeText(dir_ / "empty.json", R"({"portraits": {}})");
    }
    void TearDown() override {
        SDL_ResetHint(SDL_HINT_VIDEO_DRIVER);
        SDL_ResetHint(SDL_HINT_RENDER_DRIVER);
        SDL_ResetHint(SDL_HINT_AUDIO_DRIVER);
        if (std::getenv("FANREN_KEEP_SHOTS") != nullptr) {
            std::printf("[portrait shots] %s\n", dir_.string().c_str());   // 排查时留着截图看
            return;
        }
        std::error_code ignored;
        fs::remove_all(dir_, ignored);
    }

    fanren::game::PortraitSource fixture() const {
        return {(dir_ / "portraits.json").string(), dir_.string()};
    }
    fanren::game::PortraitSource emptyTable() const { return {(dir_ / "empty.json").string(), dir_.string()}; }

    // 一句对白拍一张：新开一台 Application（软件渲染），sayAs 压对话框，空转 240 帧（截图口 talk 的
    // kTalkShotFrames），再像截图口那样 beginFrame → drawScenes → captureFrame。
    Frame shoot(const std::string& role, const std::string& face, int& boxX) {
        Frame frame;
        fanren::game::Application app;
        const auto ready = app.init(repoRoot(), /*headless=*/false);
        EXPECT_TRUE(ready.ok) << ready.error;
        if (!ready.ok) return frame;
        app.sayAs(role, "ch01.zhenshang.arrive", face);
        for (int i = 0; i < 240; ++i) app.tick(1.0 / 60.0);
        EXPECT_NE(app.topScene(), nullptr);
        if (app.topScene() != nullptr) EXPECT_EQ(app.topScene()->name(), "Dialogue");
        boxX = fanren::game::DialogueScene::boxAreaFor(0, app.theme()).x;
        app.engine().beginFrame();
        app.drawScenes();
        const fs::path png = dir_ / ("shot" + std::to_string(shots_++) + ".png");
        const auto shot = app.engine().captureFrame(png.string());
        app.engine().endFrame();
        app.shutdown();
        EXPECT_TRUE(shot.ok) << shot.error;
        if (!shot.ok) return frame;
        SDL_Surface* loaded = SDL_LoadPNG(png.string().c_str());
        EXPECT_NE(loaded, nullptr) << SDL_GetError();
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

    fs::path dir_;
    int shots_ = 0;
};

// 夹具第 face 格按 ×2 放大后，在画面上哪里与它逐像素相同。找不到返回 {-1, -1}。
fanren::engine::Point findScaledFace(const Frame& frame, int face) {
    constexpr int kScale = 2;
    const Rgb corner = fixturePixel(face, 0, 0);
    for (int y = 0; y + kFixtureH * kScale <= frame.h; ++y) {
        for (int x = 0; x + kFixtureW * kScale <= frame.w; ++x) {
            if (!(frame.at(x, y) == corner)) continue;
            bool same = true;
            for (int fy = 0; same && fy < kFixtureH * kScale; ++fy) {
                for (int fx = 0; same && fx < kFixtureW * kScale; ++fx) {
                    same = frame.at(x + fx, y + fy) == fixturePixel(face, fx / kScale, fy / kScale);
                }
            }
            if (same) return {x, y};
        }
    }
    return {-1, -1};
}

std::size_t differingPixels(const Frame& a, const Frame& b) {
    if (a.w != b.w || a.h != b.h) return static_cast<std::size_t>(-1);
    std::size_t n = 0;
    for (std::size_t i = 0; i < a.pixels.size(); ++i) n += a.pixels[i] == b.pixels[i] ? 0u : 1u;
    return n;
}

TEST_F(PortraitShot, ASpeakerWithAPortraitShowsExactlyThatFaceAtTheLeftOfTheBox) {
    SourceGuard guard(fixture());
    int boxX = 0;
    const Frame smile = shoot("hanli", "smile", boxX);
    ASSERT_GT(smile.w, 0);
    const auto at = findScaledFace(smile, 1);
    ASSERT_GE(at.x, 0) << "画面上找不到夹具 smile 那一格按 ×2 放大后的样子";
    EXPECT_GT(at.x, boxX) << "头像在对白框里、不压左框线";
    EXPECT_LT(at.x, boxX + 60) << "头像贴在对白框左侧";
    EXPECT_GT(at.y, smile.h / 2) << "对白框在屏幕下半";
    EXPECT_EQ(findScaledFace(smile, 0).x, -1) << "取对了表情：normal 那一格不该出现";
    EXPECT_EQ(findScaledFace(smile, 2).x, -1);

    // 表情真的跟着 talk 的第三个参数走：换 hurt，同一个位置换成 hurt 那一格。
    const Frame hurt = shoot("hanli", "hurt", boxX);
    EXPECT_EQ(findScaledFace(hurt, 3).x, at.x);
    EXPECT_EQ(findScaledFace(hurt, 3).y, at.y);
    EXPECT_EQ(findScaledFace(hurt, 1).x, -1);
}

TEST_F(PortraitShot, NarrationAndUnregisteredSpeakersLookExactlyAsWithoutAnyPortraits) {
    int boxX = 0;
    // 暖机：同一进程里第一台 Application 用软件渲染器拍的那一张，对白框的底色与字形边缘比之后几台差 1–2 级
    // （与头像无关：头像表全空时一样；第 2 台起两两逐像素相同）。比整帧之前先空拍一张，扔掉。
    {
        SourceGuard guard(emptyTable());
        (void)shoot("", "normal", boxX);
    }
    Frame narrationWith, narrationWithout, otherWith, otherWithout;
    {
        SourceGuard guard(fixture());
        narrationWith = shoot("", "normal", boxX);
        otherWith = shoot("zhang_tie", "normal", boxX);
    }
    {
        SourceGuard guard(emptyTable());
        narrationWithout = shoot("", "normal", boxX);
        otherWithout = shoot("zhang_tie", "normal", boxX);
    }
    ASSERT_GT(narrationWith.w, 0);
    ASSERT_GT(otherWith.w, 0);
    EXPECT_EQ(differingPixels(narrationWith, narrationWithout), 0u) << "旁白：头像表在不在，画面一模一样";
    EXPECT_EQ(differingPixels(otherWith, otherWithout), 0u) << "没登记头像的角色：画面一模一样";
    EXPECT_EQ(findScaledFace(narrationWith, 0).x, -1);
    // 阳性对照：同一套比法对「有头像」的那一句要报得出差别，否则上面两个 0 什么也没证明。
    Frame hanliWith, hanliWithout;
    {
        SourceGuard guard(fixture());
        hanliWith = shoot("hanli", "normal", boxX);
    }
    {
        SourceGuard guard(emptyTable());
        hanliWithout = shoot("hanli", "normal", boxX);
    }
    EXPECT_GT(differingPixels(hanliWith, hanliWithout), static_cast<std::size_t>(kFixtureW * kFixtureH * 4))
        << "有头像时至少头像那 112×128 一块不同";
}

TEST_F(PortraitShot, TheDefaultSourceIsTheShippedTableBesideTheAssets) {
    SourceGuard guard(fanren::game::PortraitSource{});
    fanren::game::Application app;
    ASSERT_TRUE(app.init(repoRoot(), /*headless=*/false).ok);
    const std::string path = fanren::game::portraitTablePath(app.engine());
    ASSERT_FALSE(path.empty()) << "缺省来源找不到 data/visual/portraits.json";
    EXPECT_TRUE(fs::equivalent(path, fs::path(repoRoot()) / "data" / "visual" / "portraits.json")) << path;
    app.shutdown();
}

}  // namespace
