// 伙伴改版 · E 路美术打包（docs/interfaces-companions.md 2.5、3.9；格式见 docs/art-sprites.md 5.2–5.4）。
//
// 打包出来的东西对着真树读、走真实入口取：
//   · 手绘索引（assets/art/sprites/hd/index.json）经 io::loadSpriteIndex 与生成索引合并：四张人物表、三张敌人表、
//     三个接管的角色；韩立只在第 7 章换手绘（第 2、7、8 章各取到哪张表，章节判据用真章节表与存档旗标）；
//   · 八个新物件与历练入口在真索引的物件表里都取得到；
//   · 头像表（data/visual/portraits.json）按章生效：读取器认列表与 min / max_chapter；
//     软件渲染器上开一台真 Application，sayAs 压一句韩立的对白，第 7 章画出的是打包出来的那一格、第 2 / 8 章不带头像。
// 期望值照契约与 tools/artgen/hd_pack.py 的实测写死（走路格 64×60、脚底第 57 行；战斗格 132×84、脚底第 81 行；
// 头像格 128×136、×1），不取自被测代码。
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <regex>
#include <functional>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <SDL3/SDL.h>
#include <gtest/gtest.h>

#include "TempDir.h"
#include "engine/TextLayout.h"
#include "game/Application.h"
#include "game/DialogueScene.h"
#include "game/Portraits.h"
#include "game/SpriteAtlas.h"
#include "io/PortraitLoader.h"
#include "io/VisualLoader.h"
#include "ui/Widgets.h"

namespace {

namespace fs = std::filesystem;
using fanren::core::SpriteIndex;

std::string repoRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "maps" / "ch01_hanjiacun.tmj")) return candidate;
    }
    return ".";
}

SpriteIndex shippedIndex() {
    const auto index =
        fanren::io::loadSpriteIndex((fs::path(repoRoot()) / "assets" / "art" / "sprites" / "index.json").string());
    EXPECT_TRUE(index.ok) << index.error;
    return index.ok ? index.value : SpriteIndex{};
}

// 「正在演第 chapter 章」的存档：之前各章的 done_flag 都立起来（与 WorldView、MenuScene 同一个判据：章节表 + 存档旗标）。
void playChapter(fanren::game::Application& app, int chapter) {
    for (int n = 1; n <= 13; ++n) {
        const auto* entry = app.chapterTable().find(n);
        ASSERT_NE(entry, nullptr) << "章节表里没有第 " << n << " 章";
        app.state().flags[entry->doneFlag] = n < chapter ? 1 : 0;
    }
}

fanren::game::ChapterDone chapterDoneOf(fanren::game::Application& app) {
    return [&app](int n) { return app.chapterTable().chapterDone(app.state(), n); };
}

// ---------------------------------------------------------------------------
// 手绘索引
// ---------------------------------------------------------------------------

TEST(HdArt, TheShippedHdIndexAddsTheFourPeopleWithTheirOwnScalesAndBattleSheets) {
    const SpriteIndex index = shippedIndex();
    const char* const kSheets[][3] = {
        {"hanli_hd", "hd/hanli_walk.png", "hd/hanli_battle.png"},
        {"nangong_wan_veil_hd", "hd/nangong_wan_veil_walk.png", "hd/nangong_wan_veil_battle.png"},
        {"nangong_wan_ch07_hd", "hd/nangong_wan_ch07_walk.png", "hd/nangong_wan_ch07_battle.png"},
        {"chen_shimei_hd", "hd/chen_shimei_walk.png", "hd/chen_shimei_battle.png"},
    };
    for (const auto& row : kSheets) {
        SCOPED_TRACE(row[0]);
        ASSERT_EQ(index.sheets.count(row[0]), 1u);
        const auto& sheet = index.sheets.at(row[0]);
        EXPECT_EQ(sheet.file, row[1]);
        EXPECT_EQ(sheet.frameW, 64);
        EXPECT_EQ(sheet.frameH, 60);
        EXPECT_EQ(sheet.cols, 4);
        EXPECT_EQ(sheet.footY, 57);
        EXPECT_EQ(sheet.headY, 10);
        EXPECT_EQ(sheet.scale, 2);
        EXPECT_EQ(sheet.battleScale, 2);
        EXPECT_EQ(sheet.walkCycle, (std::vector<int>{0, 1, 2, 3}));
        // walk 的下标是 GameState::facing：上、右、下、左；图上的行序是下、左、右、上。
        EXPECT_EQ(sheet.walk[0], (std::vector<int>{12, 13, 14, 15}));
        EXPECT_EQ(sheet.walk[1], (std::vector<int>{8, 9, 10, 11}));
        EXPECT_EQ(sheet.walk[2], (std::vector<int>{0, 1, 2, 3}));
        EXPECT_EQ(sheet.walk[3], (std::vector<int>{4, 5, 6, 7}));
        const char* const kActions[] = {"idle", "ready", "attack", "cast", "hurt", "down", "victory", "guard"};
        for (int i = 0; i < 8; ++i) EXPECT_EQ(sheet.battle.at(kActions[i]), (std::vector<int>{i})) << kActions[i];
        ASSERT_TRUE(sheet.battleSheet.has_value());
        EXPECT_EQ(sheet.battleSheet->file, row[2]);
        EXPECT_EQ(sheet.battleSheet->frameW, 132);
        EXPECT_EQ(sheet.battleSheet->frameH, 84);
        EXPECT_EQ(sheet.battleSheet->cols, 4);
        EXPECT_EQ(sheet.battleSheet->footY, 81);
        EXPECT_EQ(fanren::game::battleSheetFile(sheet), row[2]) << "战斗层经 SpriteAtlas 取到的是战斗图";
    }
    EXPECT_EQ(index.roles.at("baiyi_shaonv"), "nangong_wan_veil_hd");
    EXPECT_EQ(index.roles.at("nangong_wan_ch07"), "nangong_wan_ch07_hd");
    EXPECT_EQ(index.roles.at("chen_shimei"), "chen_shimei_hd");
    EXPECT_EQ(index.roles.at("hanli"), "hanli") << "韩立不进 roles：那样每一章都会换";
}

TEST(HdArt, HanliWearsTheHandDrawnSheetOnlyInChapterSeven) {
    // 判据走真的：一台 Application 的章节表（data/chapters.json）问存档里的 done_flag。
    fanren::game::Application app;
    ASSERT_TRUE(app.init(repoRoot(), /*headless=*/true).ok);
    const SpriteIndex index = shippedIndex();
    const auto lookIn = [&](int chapter) {
        playChapter(app, chapter);
        return fanren::game::lookForRole(index, "hanli", chapterDoneOf(app));
    };
    EXPECT_EQ(lookIn(2), "hanli_child") << "第 1–2 章仍是生成的少年相";
    EXPECT_EQ(lookIn(7), "hanli_hd") << "第 7 章（样板段）换手绘";
    EXPECT_EQ(lookIn(8), "hanli") << "第 8 章起落回生成索引的 roles";
    EXPECT_EQ(lookIn(3), "hanli");
    playChapter(app, 7);
    const auto* sheet = fanren::game::sheetForRole(index, "hanli", chapterDoneOf(app));
    ASSERT_NE(sheet, nullptr);
    EXPECT_EQ(sheet->file, "hd/hanli_walk.png");
    app.shutdown();
}

TEST(HdArt, TheThreeHandDrawnBeastsOverrideOrJoinTheEnemyTable) {
    const SpriteIndex index = shippedIndex();
    for (const char* id : {"mo_jiao", "mo_jiao_tuipi", "bishui_e"}) {
        SCOPED_TRACE(id);
        ASSERT_EQ(index.enemies.count(id), 1u);
        const auto& enemy = index.enemies.at(id);
        EXPECT_EQ(enemy.file, std::string("hd/") + id + ".png") << "墨蛟覆盖生成索引里同 id 的那一只";
        EXPECT_EQ(enemy.scale, 1);
        EXPECT_EQ(enemy.battleScale, 2);
        EXPECT_EQ(enemy.cols, 4);
        EXPECT_EQ(enemy.battleFacing, fanren::core::BattleFacing::Right);
        EXPECT_EQ(enemy.battle.at("idle"), (std::vector<int>{0}));
        EXPECT_EQ(enemy.battle.at("attack"), (std::vector<int>{1}));
        EXPECT_EQ(enemy.battle.at("hurt"), (std::vector<int>{2}));
        EXPECT_EQ(enemy.battle.at("down"), (std::vector<int>{3}));

        // 四个动作底边对齐：每一格的脚底行上都有不透明像素，再往下一行全透明。
        const fs::path png = fs::path(repoRoot()) / "assets" / "art" / "sprites" / enemy.file;
        SDL_Surface* loaded = SDL_LoadPNG(png.string().c_str());
        ASSERT_NE(loaded, nullptr) << SDL_GetError();
        SDL_Surface* rgba = SDL_ConvertSurface(loaded, SDL_PIXELFORMAT_RGBA32);
        SDL_DestroySurface(loaded);
        ASSERT_NE(rgba, nullptr);
        EXPECT_EQ(rgba->w, enemy.frameW * 4);
        EXPECT_EQ(rgba->h, enemy.frameH);
        const auto alphaAt = [rgba](int x, int y) {
            return static_cast<const std::uint8_t*>(rgba->pixels)[y * rgba->pitch + x * 4 + 3];
        };
        for (int cell = 0; cell < 4; ++cell) {
            bool onFoot = false, below = false;
            for (int x = cell * enemy.frameW; x < (cell + 1) * enemy.frameW; ++x) {
                onFoot = onFoot || alphaAt(x, enemy.footY) != 0;
                if (enemy.footY + 1 < rgba->h) below = below || alphaAt(x, enemy.footY + 1) != 0;
            }
            EXPECT_TRUE(onFoot) << "第 " << cell << " 格脚底没落在 foot_y";
            EXPECT_FALSE(below) << "第 " << cell << " 格脚底以下还有像素";
        }
        SDL_DestroySurface(rgba);
    }
}

TEST(HdArt, TheNewMapObjectsAndTheLilianFacilityAreInTheShippedObjectTable) {
    const SpriteIndex index = shippedIndex();
    for (const char* id : {"chest_closed", "chest_open", "gather_ready", "gather_spent", "gate_closed", "gate_open",
                           "foe_mark", "foe_mark_elite", "facility.lilian"}) {
        EXPECT_EQ(index.objects.count(id), 1u) << id;
    }
    EXPECT_EQ(index.facilities.at("lilian"), "facility.lilian");
    // 落地点口径：箱子、采集点、机关都对到对象格底边，锚点落在帧的下沿附近而不是正中。
    EXPECT_GE(index.objects.at("chest_closed").anchorY, 12.f);
    EXPECT_EQ(index.objects.at("chest_closed").anchorY, index.objects.at("chest_open").anchorY)
        << "开箱不让箱子跳一下";
    EXPECT_EQ(index.objects.at("gather_ready").anchorY, index.objects.at("gather_spent").anchorY);
}

// ---------------------------------------------------------------------------
// 头像按章生效
// ---------------------------------------------------------------------------

constexpr const char* kFaces = R"("faces": {"normal": 0, "smile": 1, "angry": 2, "hurt": 3})";

std::string entry(const std::string& extra) {
    return std::string(R"({"file": "a.png", "frame_w": 128, "frame_h": 136, "cols": 4, )") + kFaces + extra + "}";
}

TEST(PortraitChapters, TheLoaderReadsAListWithChapterWindows) {
    const auto table = fanren::io::parsePortraits(std::string(R"({"portraits": {"hanli": [)") +
                                                  entry(R"(, "min_chapter": 7, "max_chapter": 7)") + ", " + entry("") +
                                                  "]}}");
    ASSERT_TRUE(table.ok) << table.error;
    const auto& list = table.value.at("hanli");
    ASSERT_EQ(list.size(), 2u);
    EXPECT_EQ(list[0].minChapter, 7);
    EXPECT_EQ(list[0].maxChapter, 7);
    EXPECT_EQ(list[1].minChapter, 0);
    EXPECT_EQ(list[1].maxChapter, 0);
}

TEST(PortraitChapters, BadListsAndWindowsRejectTheWholeFile) {
    EXPECT_FALSE(fanren::io::parsePortraits(std::string(R"({"portraits": {"hanli": )") + entry("") + "}}").ok)
        << "改版前那种一个角色一个对象的写法不再认";
    EXPECT_FALSE(fanren::io::parsePortraits(R"({"portraits": {"hanli": []}})").ok) << "空列表";
    EXPECT_FALSE(fanren::io::parsePortraits(std::string(R"({"portraits": {"hanli": [)") +
                                            entry(R"(, "min_chapter": 8, "max_chapter": 7)") + "]}}")
                     .ok)
        << "min 大于 max：哪一章都不生效";
    EXPECT_FALSE(fanren::io::parsePortraits(std::string(R"({"portraits": {"hanli": [)") +
                                            entry(R"(, "max_chapter": -1)") + "]}}")
                     .ok);
    const auto bad = fanren::io::parsePortraits(std::string(R"({"portraits": {"hanli": [)") + entry("") + ", " +
                                                entry(R"(, "chapter": 3)") + "]}}");
    ASSERT_FALSE(bad.ok);
    EXPECT_NE(bad.error.find("portraits.hanli[1].chapter"), std::string::npos) << bad.error;
}

TEST(PortraitChapters, TheShippedTableGivesHanliAFaceOnlyInChapterSeven) {
    const auto table = fanren::io::loadPortraits((fs::path(repoRoot()) / "data" / "visual" / "portraits.json").string());
    ASSERT_TRUE(table.ok) << table.error;
    for (const char* role : {"hanli", "baiyi_shaonv", "nangong_wan_ch07", "chen_shimei"}) {
        SCOPED_TRACE(role);
        ASSERT_EQ(table.value.count(role), 1u);
        const auto& sheet = table.value.at(role).front();
        EXPECT_EQ(sheet.frameW, 128);
        EXPECT_EQ(sheet.frameH, 136);
        EXPECT_EQ(fanren::game::portraitScale(sheet.frameW, sheet.frameH), 1) << "放得进 128×144，×1 画";
    }
    EXPECT_EQ(table.value.at("baiyi_shaonv").front().faces, (std::array<int, 4>{0, 0, 0, 0}))
        << "面纱版只有「平常」一张，四个表情都指它";

    fanren::game::Application app;
    ASSERT_TRUE(app.init(repoRoot(), /*headless=*/true).ok);
    const auto faceIn = [&](const char* role, int chapter) {
        playChapter(app, chapter);
        return fanren::game::portraitForChapter(table.value.at(role), chapterDoneOf(app)) != nullptr;
    };
    EXPECT_FALSE(faceIn("hanli", 2)) << "第 1–2 章那个十岁的孩子说话不出青年的脸";
    EXPECT_TRUE(faceIn("hanli", 7));
    EXPECT_FALSE(faceIn("hanli", 8));
    EXPECT_TRUE(faceIn("chen_shimei", 7));
    EXPECT_TRUE(faceIn("chen_shimei", 2)) << "没写章的那一条每章都生效";
    app.shutdown();
}

// ---------------------------------------------------------------------------
// 带真头像时正文还装不装得下（只报告，不判红；与 tests/PortraitTextFitTests.cpp 同一套量法）
// ---------------------------------------------------------------------------
// A 路那条按「定稿 56×64 → ×2 = 112 宽」量；打包出来的头像是 128×136、×1 = 128 宽，正文又窄 16 像素。
// 这里取仓库头像表里真的格子尺寸再量一遍：全部写死说话人的 talk 按带头像的宽度排，另单列已登记头像的角色。

std::vector<std::pair<std::string, std::string>> spokenLines(const fs::path& scripts) {
    const std::regex call(R"re(talk\(\s*"([a-z0-9_]+)"\s*,\s*"([^"]+)")re");
    std::set<std::pair<std::string, std::string>> seen;
    for (const auto& entry : fs::recursive_directory_iterator(scripts)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".lua") continue;
        std::ifstream in(entry.path(), std::ios::binary);
        std::stringstream buffer;
        buffer << in.rdbuf();
        const std::string text = buffer.str();
        for (auto it = std::sregex_iterator(text.begin(), text.end(), call); it != std::sregex_iterator(); ++it) {
            seen.emplace((*it)[1].str(), (*it)[2].str());
        }
    }
    return {seen.begin(), seen.end()};
}

TEST(PortraitChapters, ReportsSpokenLinesThatNoLongerFitBesideTheShippedPortraits) {
    const auto table = fanren::io::loadPortraits((fs::path(repoRoot()) / "data" / "visual" / "portraits.json").string());
    ASSERT_TRUE(table.ok) << table.error;
    ASSERT_FALSE(table.value.empty());
    int frameW = 0, frameH = 0;
    for (const auto& [role, list] : table.value) {
        for (const auto& sheet : list) {
            frameW = std::max(frameW, sheet.frameW * fanren::game::portraitScale(sheet.frameW, sheet.frameH));
            frameH = std::max(frameH, sheet.frameH * fanren::game::portraitScale(sheet.frameW, sheet.frameH));
        }
    }
    EXPECT_EQ(frameW, 128) << "打包的头像格 128×136、×1";

    fanren::game::Application app;
    ASSERT_TRUE(app.init(repoRoot(), /*headless=*/true).ok);
    const fanren::ui::Theme& theme = app.theme();
    const fanren::engine::Rect box = fanren::game::DialogueScene::boxAreaFor(0, theme);
    const fanren::engine::Rect area = fanren::ui::panelContentArea(box, std::string{}, theme);
    const auto layout = fanren::game::portraitLayout(area, theme.padding, frameW, frameH);
    const int width = layout.text.w - theme.padding * 2;
    int rows = 0;
    for (int y = theme.padding; y + theme.bodyFontSize <= area.h - theme.padding;
         y += theme.bodyFontSize + theme.lineSpacing) {
        ++rows;
    }
    ASSERT_GT(rows, 0);
    fanren::engine::Engine& engine = app.engine();
    const auto measure = [&engine, &theme](const std::string& sample) {
        return engine.measureText(sample, theme.bodyFontSize).x;
    };
    ASSERT_GT(measure("韩立"), 0) << "无头引擎量不出字宽：字体没载进来";
    std::string tooLong;
    for (int i = 0; i < 6; ++i) tooLong += app.text("ch01.zhenshang.arrive");
    ASSERT_GT(static_cast<int>(fanren::engine::layoutText(tooLong, width, measure).size()), rows) << "阳性对照";

    const auto lines = spokenLines(fs::path(repoRoot()) / "scripts");
    std::size_t over = 0, registered = 0, registeredOver = 0, longest = 0;
    std::string longestKey;
    for (const auto& [role, key] : lines) {
        const auto n = fanren::engine::layoutText(app.text(key), width, measure).size();
        const bool hasPortrait = table.value.count(role) != 0;
        registered += hasPortrait ? 1u : 0u;
        if (n > longest) {
            longest = n;
            longestKey = key;
        }
        if (static_cast<int>(n) > rows) {
            ++over;
            if (hasPortrait) ++registeredOver;
            std::printf("    %s %s（%zu 行）\n", role.c_str(), key.c_str(), n);
        }
    }
    std::printf("[portrait fit · 真头像] 头像 %d×%d；正文宽 %d 像素；框里放得下 %d 行；量了 %zu 句写死说话人的 talk\n",
                frameW, frameH, width, rows, lines.size());
    std::printf("[portrait fit · 真头像] 全按带头像排：装不下 %zu 句；最长一句 %zu 行（%s）\n", over, longest,
                longestKey.c_str());
    std::printf("[portrait fit · 真头像] 已登记头像的角色说的 %zu 句里装不下 %zu 句\n", registered, registeredOver);
    RecordProperty("portrait_over", static_cast<int>(over));
    RecordProperty("registered_over", static_cast<int>(registeredOver));
    EXPECT_GT(lines.size(), 1000u) << "正则没认出来就什么也没量";
    app.shutdown();
}

// 软件渲染器上拍一句对白（与 tests/PortraitTests.cpp 同一个做法：dummy 视频驱动 + 软件渲染，空转 240 帧再抓帧）。
class PortraitChapterShot : public ::testing::Test {
protected:
    struct Shot {
        int w = 0, h = 0;
        std::vector<std::uint8_t> rgba;   // 抓帧是不透明的，第 4 个字节不看
    };

    void SetUp() override {
        SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
        SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
        SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "dummy");
        dir_ = fanren::test::uniqueTempPath("fanren_portrait_chapters");
        fs::create_directories(dir_);
    }
    void TearDown() override {
        SDL_ResetHint(SDL_HINT_VIDEO_DRIVER);
        SDL_ResetHint(SDL_HINT_RENDER_DRIVER);
        SDL_ResetHint(SDL_HINT_AUDIO_DRIVER);
        if (std::getenv("FANREN_KEEP_SHOTS") != nullptr) return;
        std::error_code ignored;
        fs::remove_all(dir_, ignored);
    }

    static Shot load(const fs::path& png) {
        Shot shot;
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
            shot.rgba.insert(shot.rgba.end(), row, row + shot.w * 4);
        }
        SDL_DestroySurface(rgba);
        return shot;
    }

    // 仓库真树上开一台 Application，存档摆到第 chapter 章，韩立说一句 normal。
    Shot shoot(int chapter) {
        fanren::game::Application app;
        const auto ready = app.init(repoRoot(), /*headless=*/false);
        EXPECT_TRUE(ready.ok) << ready.error;
        if (!ready.ok) return {};
        playChapter(app, chapter);
        app.sayAs("hanli", "ch01.zhenshang.arrive", "normal");
        for (int i = 0; i < 240; ++i) app.tick(1.0 / 60.0);
        app.engine().beginFrame();
        app.drawScenes();
        const fs::path png = dir_ / ("ch" + std::to_string(chapter) + "_" + std::to_string(shots_++) + ".png");
        const auto captured = app.engine().captureFrame(png.string());
        app.engine().endFrame();
        app.shutdown();
        EXPECT_TRUE(captured.ok) << captured.error;
        return captured.ok ? load(png) : Shot{};
    }

    // 打包出来的 hanli.png 第 cell 格（128×136）里不透明的像素，在画面上 ×1 原样出现在哪里；找不到返回 false。
    static bool findCell(const Shot& frame, const Shot& atlas, int cell) {
        constexpr int kW = 128, kH = 136;
        std::vector<std::pair<int, int>> solid;
        for (int y = 0; y < kH; ++y) {
            for (int x = 0; x < kW; ++x) {
                if (atlas.rgba[(y * atlas.w + cell * kW + x) * 4 + 3] == 255) solid.emplace_back(x, y);
            }
        }
        if (solid.empty() || frame.w < kW || frame.h < kH) return false;
        const auto same = [&](int ox, int oy, int x, int y) {
            const std::size_t f = (static_cast<std::size_t>(oy + y) * frame.w + (ox + x)) * 4;
            const std::size_t a = (static_cast<std::size_t>(y) * atlas.w + cell * kW + x) * 4;
            return frame.rgba[f] == atlas.rgba[a] && frame.rgba[f + 1] == atlas.rgba[a + 1] &&
                   frame.rgba[f + 2] == atlas.rgba[a + 2];
        };
        for (int oy = 0; oy + kH <= frame.h; ++oy) {
            for (int ox = 0; ox + kW <= frame.w; ++ox) {
                bool all = true;
                for (const auto& [x, y] : solid) {
                    if (!same(ox, oy, x, y)) {
                        all = false;
                        break;
                    }
                }
                if (all) return true;
            }
        }
        return false;
    }

    fs::path dir_;
    int shots_ = 0;
};

TEST_F(PortraitChapterShot, HanliSpeaksWithTheHandDrawnFaceInChapterSevenOnly) {
    const Shot atlas = load(fs::path(repoRoot()) / "assets" / "art" / "portraits" / "hanli.png");
    ASSERT_EQ(atlas.w, 128 * 4);
    ASSERT_EQ(atlas.h, 136);
    (void)shoot(1);   // 暖机：进程里第一台软件渲染的 Application 拍的图与之后几台差 1–2 级（docs/handoff.md 的教训）
    const Shot seven = shoot(7);
    const Shot two = shoot(2);
    const Shot eight = shoot(8);
    ASSERT_GT(seven.w, 0);
    EXPECT_TRUE(findCell(seven, atlas, 0)) << "第 7 章：画面上有打包出来的韩立 normal 那一格（×1）";
    EXPECT_FALSE(findCell(seven, atlas, 1)) << "取对了表情";
    EXPECT_FALSE(findCell(two, atlas, 0)) << "第 2 章：韩立说话不带头像";
    EXPECT_FALSE(findCell(eight, atlas, 0)) << "第 8 章：同上";
}

}  // namespace
