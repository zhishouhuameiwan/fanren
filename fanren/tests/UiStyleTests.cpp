// 八方旅人化改造·界面路（docs/interfaces-octo-ui.md）的单测：墨金主题、主菜单小像选外观的规则、
// 章节卡的排程与「无头下不挡路」、淡入淡出的浓度、主菜单的用词、名签的摆位。
//
// 判据的来处：主题色、章名、完成旗标抄的是施工图 docs/octopath-overhaul.md 1.6 节与界面路派工单
// 的原文，不从被测物推（docs/README.md「判据是从被测物推导出来的」那一节）。
#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "TempDir.h"
#include "core/model/Types.h"
#include "engine/Engine.h"
#include "game/Application.h"
#include "game/BoardScene.h"
#include "game/ChapterCardScene.h"
#include "game/DialogueScene.h"
#include "game/FadeScene.h"
#include "game/KeyConfigScene.h"
#include "game/MenuScene.h"
#include "game/SettingsScene.h"
#include "game/SpriteAtlas.h"
#include "game/TitleScene.h"
#include "game/Wording.h"
#include "game/WorldScene.h"
#include "io/DataLoader.h"
#include "io/VisualLoader.h"
#include "ui/Widgets.h"

namespace {

namespace fs = std::filesystem;
using fanren::engine::Color;
using fanren::game::CardKind;
using fanren::game::CardRequest;
using fanren::game::ChapterTable;

std::string assetRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "data" / "text" / "ch01_main.json")) return candidate;
    }
    return ".";
}

fanren::core::GameData shippedData() {
    auto loaded = fanren::io::loadGameData((fs::path(assetRoot()) / "data").string());
    EXPECT_TRUE(loaded.ok) << loaded.error;
    return loaded.value;
}

ChapterTable shippedChapters(const fanren::core::GameData& data) {
    auto table = fanren::game::loadChapterTable(
        (fs::path(assetRoot()) / "data" / "chapters.json").string(), data);
    EXPECT_TRUE(table.ok) << table.error;
    return table.value;
}

void expectColor(const Color& actual, int r, int g, int b, int a, const char* what) {
    EXPECT_EQ(actual.r, r) << what;
    EXPECT_EQ(actual.g, g) << what;
    EXPECT_EQ(actual.b, b) << what;
    EXPECT_EQ(actual.a, a) << what;
}

}  // namespace

// ---------------------------------------------------------------------------
// 墨金主题
// ---------------------------------------------------------------------------

TEST(InkGoldTheme, TheTokensAreTheConstructionDrawingsValues) {
    // 施工图 1.6 节那张表的原值：#0E1119 α .86（底部 α .72）、#07090E、#C9A45C、#F2D98B、
    // #F1EAD8、#9C9480、#B8412F、#5FAE8C、#5B8FD6。
    const fanren::ui::Theme theme;
    expectColor(theme.ink, 0x0E, 0x11, 0x19, 219, "ink");
    expectColor(theme.inkLow, 0x0E, 0x11, 0x19, 184, "ink 渐变下端 α .72");
    expectColor(theme.inkDeep, 0x07, 0x09, 0x0E, 255, "inkDeep");
    expectColor(theme.gold, 0xC9, 0xA4, 0x5C, 255, "gold");
    expectColor(theme.goldBright, 0xF2, 0xD9, 0x8B, 255, "goldBright");
    expectColor(theme.paper, 0xF1, 0xEA, 0xD8, 255, "paper");
    expectColor(theme.paperDim, 0x9C, 0x94, 0x80, 255, "paperDim");
    expectColor(theme.cinnabar, 0xB8, 0x41, 0x2F, 255, "cinnabar");
    expectColor(theme.jade, 0x5F, 0xAE, 0x8C, 255, "jade");
    expectColor(theme.azure, 0x5B, 0x8F, 0xD6, 255, "azure");
}

TEST(InkGoldTheme, TheSkinChangedButTheLayoutMetricsDidNot) {
    // 换皮不换摆位：各面板能显示几行、对话框不出屏，全建在这四个数上（ListLayoutTests）。
    const fanren::ui::Theme theme;
    EXPECT_EQ(theme.padding, 24);
    EXPECT_EQ(theme.lineSpacing, 8);
    EXPECT_EQ(theme.bodyFontSize, 22);
    EXPECT_EQ(theme.titleFontSize, 24);
    // 字要压得住花底：正文有投影，标题再加描边。
    EXPECT_TRUE(theme.bodyStyle.shadow);
    EXPECT_TRUE(theme.titleStyle.outline);
}

TEST(InkGoldTheme, MixingColoursStaysBetweenTheTwoEnds) {
    const Color a{0, 100, 200, 77};
    const Color b{200, 100, 0, 255};
    const Color mid = fanren::ui::mixColor(a, b, 0.5f);
    EXPECT_EQ(mid.r, 100);
    EXPECT_EQ(mid.g, 100);
    EXPECT_EQ(mid.b, 100);
    EXPECT_EQ(mid.a, 77) << "透明度取第一个颜色的";
    const Color low = fanren::ui::mixColor(a, b, -3.f);
    const Color high = fanren::ui::mixColor(a, b, 9.f);
    EXPECT_EQ(low.r, 0);
    EXPECT_EQ(high.r, 200) << "比例夹在 [0,1]";
}

// ---------------------------------------------------------------------------
// 精灵索引（主菜单小像）
// ---------------------------------------------------------------------------
// 读 index.json 在 io 层（语法与字段的严格性由 VisualLoaderTests / SpriteAtlasTests 管），
// 选外观走 game/SpriteAtlas——与世界画面同一张表、同一条规则。这里量主菜单要的那几样。

TEST(SpriteIndexLookup, AVariantHoldsUntilItsChapterIsDone) {
    const auto index = fanren::io::parseSpriteIndex(R"({
        "sheets": {
          "boy": {"file": "chars/boy.png", "frame_w": 16, "frame_h": 24, "cols": 3,
                  "walk": {"down": [0, 1, 2], "left": [3, 4, 5], "right": [6, 7, 8], "up": [9, 10, 11]}},
          "man": {"file": "chars/man.png", "frame_w": 16, "frame_h": 24, "cols": 3,
                  "walk": {"down": [4, 5, 6], "left": [3, 4, 5], "right": [6, 7, 8], "up": [9, 10, 11]}}
        },
        "roles": {"hero": "man", "ghost": "nowhere"},
        "role_variants": {"hero": [{"look": "boy", "max_chapter": 2}]}
    })");
    ASSERT_TRUE(index.ok) << index.error;

    int doneThrough = 0;   // 演完到第几章
    const fanren::game::ChapterDone done = [&](int n) { return n <= doneThrough; };
    EXPECT_EQ(fanren::game::lookForRole(index.value, "hero", done), "boy") << "第 1 章：少年";
    doneThrough = 1;
    EXPECT_EQ(fanren::game::lookForRole(index.value, "hero", done), "boy") << "第 2 章还没完：仍是少年";
    doneThrough = 2;
    EXPECT_EQ(fanren::game::lookForRole(index.value, "hero", done), "man") << "第 2 章演完：换成大人";
    EXPECT_EQ(fanren::game::lookForRole(index.value, "stranger", done), "")
        << "没进索引的角色给空串，调用方退回旧画法";
    EXPECT_EQ(fanren::game::sheetForRole(index.value, "ghost", done), nullptr) << "外观不在 sheets 里：没有图";
    const fanren::game::CharacterSheet* man = fanren::game::sheetForRole(index.value, "hero", done);
    ASSERT_NE(man, nullptr);
    EXPECT_EQ(man->walk[2].front(), 4) << "小像取「行走·下」的站立帧";

    const fanren::engine::RectF frame = fanren::game::sheetFrameRect(*man, 4);
    EXPECT_FLOAT_EQ(frame.x, 16.f);
    EXPECT_FLOAT_EQ(frame.y, 24.f);
    EXPECT_FLOAT_EQ(frame.w, 16.f);
    EXPECT_FLOAT_EQ(frame.h, 24.f);

    // 帧宽为 0 的表画不出来。原先界面层那份索引把它悄悄跳过；统一到 io 层后整份报错——
    // 生成器写不出这种表，写出来了就是坏了，该响。
    const auto broken = fanren::io::parseSpriteIndex(R"({"sheets": {
        "broken": {"file": "chars/x.png", "frame_w": 0, "frame_h": 24, "cols": 3,
                   "walk": {"down": [0, 1, 2], "left": [3, 4, 5], "right": [6, 7, 8], "up": [9, 10, 11]}}}})");
    EXPECT_FALSE(broken.ok);
    EXPECT_NE(broken.error.find("sheets.broken"), std::string::npos) << broken.error;
}

TEST(SpriteIndexLookup, HanLiIsTheBoyUntilTheSecondChapterEnds) {
    // 派工单的原话：「韩立第 1–2 章用少年版——判据用旗标 ch02.done」。读的是真的索引与真的章节表。
    const fs::path indexPath = fs::path(assetRoot()) / "assets" / "art" / "sprites" / "index.json";
    if (!fs::exists(indexPath)) GTEST_SKIP() << "人物美术还没生成：" << indexPath.string();
    const auto index = fanren::io::loadSpriteIndex(indexPath.string());
    ASSERT_TRUE(index.ok) << index.error;

    const fanren::core::GameData data = shippedData();
    const ChapterTable chapters = shippedChapters(data);
    fanren::core::GameState state;
    const fanren::game::ChapterDone done = [&](int n) { return chapters.chapterDone(state, n); };

    EXPECT_EQ(fanren::game::lookForRole(index.value, "hanli", done), "hanli_child");
    state.setFlag("ch01.done");
    EXPECT_EQ(fanren::game::lookForRole(index.value, "hanli", done), "hanli_child") << "第 2 章里还是少年";
    state.setFlag("ch02.done");
    EXPECT_EQ(fanren::game::lookForRole(index.value, "hanli", done), "hanli") << "ch02.done 一置就换成青年";
    EXPECT_NE(index.value.sheets.count("hanli"), 0u);
    EXPECT_NE(index.value.sheets.count("hanli_child"), 0u);
}

// ---------------------------------------------------------------------------
// 章节表与排程
// ---------------------------------------------------------------------------

TEST(ChapterTableData, TheShippedTableIsWhatTheWorkOrderAsked) {
    // 派工单原文：第 1–5 章，章名「山村·七玄门 / 绿瓶四年 / 神手谷惊变 / 落日峰 / 嘉元城·墨府」，
    // 完成旗标 ch01.done…ch05.done，最后一章之后一张「未完待续」。
    // 2026-09-29 第 6 章开工（docs/ch06-design.md 10.1 E8）：表加了第六章，ch06.done。卡上的字是「太南谷·黄枫谷」
    //（不是大纲的「太南谷·升仙令」：卡片在 ch05.done 那一刻就上屏，会把识破牌子那一幕剧透掉，第 6 章校对 MEDIUM-1）。
    const fanren::core::GameData data = shippedData();
    const ChapterTable table = shippedChapters(data);
    const char* titles[] = {"山村·七玄门", "绿瓶四年", "神手谷惊变", "落日峰", "嘉元城·墨府", "太南谷·黄枫谷"};
    const char* numerals[] = {"第一章", "第二章", "第三章", "第四章", "第五章", "第六章"};
    const char* flags[] = {"ch01.done", "ch02.done", "ch03.done", "ch04.done", "ch05.done", "ch06.done"};
    ASSERT_EQ(table.chapters.size(), 6u);
    for (int i = 0; i < 6; ++i) {
        const auto& entry = table.chapters[static_cast<std::size_t>(i)];
        EXPECT_EQ(entry.number, i + 1);
        EXPECT_EQ(entry.title, titles[i]);
        EXPECT_EQ(entry.numeral, numerals[i]);
        EXPECT_EQ(entry.doneFlag, flags[i]);
    }
    EXPECT_EQ(table.closingWord, "终");
    EXPECT_EQ(table.toBeContinued, "未完待续");
}

TEST(ChapterCards, OnlyAChapterEndingForTheFirstTimeQueuesCards) {
    const fanren::core::GameData data = shippedData();
    const ChapterTable table = shippedChapters(data);
    ASSERT_FALSE(table.chapters.empty()) << "先验：表里有章，否则下面每一条「不排」都是白绿";

    const std::vector<CardRequest> ending1 = fanren::game::cardsForFlagChange(table, "ch01.done", 0, 1);
    const std::vector<CardRequest> expected1{{CardKind::Closing, 1}, {CardKind::Opening, 2}};
    EXPECT_EQ(ending1, expected1) << "第一章　终，接第二章开篇";
    const std::vector<CardRequest> expected3{{CardKind::Closing, 3}, {CardKind::Opening, 4}};
    EXPECT_EQ(fanren::game::cardsForFlagChange(table, "ch03.done", 0, 1), expected3);

    // 不是「从没演完到演完」的一律不排：读档、脚本重复置、清回 0、别的旗标。
    EXPECT_TRUE(fanren::game::cardsForFlagChange(table, "ch01.done", 1, 1).empty());
    EXPECT_TRUE(fanren::game::cardsForFlagChange(table, "ch01.done", 1, 0).empty());
    EXPECT_TRUE(fanren::game::cardsForFlagChange(table, "ch01.done", 0, 0).empty());
    EXPECT_TRUE(fanren::game::cardsForFlagChange(table, "ch01.koujue_received", 0, 1).empty());
    EXPECT_TRUE(fanren::game::cardsForFlagChange(table, "ch07.done", 0, 1).empty())
        << "表里没有的章不排（第 6 章 2026-09-29 进了表，这里换成第 7 章）";
    const std::vector<CardRequest> anyValue{{CardKind::Closing, 2}, {CardKind::Opening, 3}};
    EXPECT_EQ(fanren::game::cardsForFlagChange(table, "ch02.done", 0, 5), anyValue)
        << "置成任何非 0 值都算演完";
}

TEST(ChapterCards, TheLastChapterIsFollowedByToBeContinued) {
    const ChapterTable table = shippedChapters(shippedData());
    const std::vector<CardRequest> expected{{CardKind::Closing, 6}, {CardKind::ToBeContinued, 0}};
    EXPECT_EQ(fanren::game::cardsForFlagChange(table, "ch06.done", 0, 1), expected);
    const fanren::game::Card more = fanren::game::resolveCard(table, expected[1]);
    EXPECT_EQ(more.headline, "未完待续");
    const fanren::game::Card closing = fanren::game::resolveCard(table, expected[0]);
    EXPECT_EQ(closing.headline, "第六章　终");
    EXPECT_EQ(closing.kicker, "太南谷·黄枫谷");
    // 第 5 章不再是最后一章：它「终」之后接第六章开篇，不接「未完待续」。
    const std::vector<CardRequest> fifth{{CardKind::Closing, 5}, {CardKind::Opening, 6}};
    EXPECT_EQ(fanren::game::cardsForFlagChange(table, "ch05.done", 0, 1), fifth);
}

TEST(ChapterCards, ANewJourneyOpensWithChapterOne) {
    const ChapterTable table = shippedChapters(shippedData());
    const std::vector<CardRequest> expected{{CardKind::Opening, 1}};
    EXPECT_EQ(fanren::game::openingCards(table), expected);
    const fanren::game::Card card = fanren::game::resolveCard(table, expected[0]);
    EXPECT_EQ(card.kicker, "第一章");
    EXPECT_EQ(card.headline, "山村·七玄门");
    EXPECT_TRUE(fanren::game::openingCards(ChapterTable{}).empty());
}

TEST(ChapterCards, AMissingTableIsEmptyButABrokenOneIsAnError) {
    const fanren::core::GameData data;
    const auto missing = fanren::game::loadChapterTable("no/such/chapters.json", data);
    ASSERT_TRUE(missing.ok) << "没有章节表是合法的：临时资产根多半只拷了一部分 data";
    EXPECT_TRUE(missing.value.chapters.empty());

    const fanren::test::TempDir temp("fanren_ui_chapters");   // 唯一目录名：并行跑的 ctest 不互删
    const fs::path& dir = temp.path();
    const auto write = [&](const char* body) {
        std::FILE* file = std::fopen((dir / "chapters.json").string().c_str(), "wb");
        std::fputs(body, file);
        std::fclose(file);
        return fanren::game::loadChapterTable((dir / "chapters.json").string(), data);
    };
    EXPECT_FALSE(write("{\"chapters\": [").ok) << "语法错";
    EXPECT_FALSE(write(R"({"closing_key": "a", "to_be_continued_key": "b",
                          "chapters": [{"number": 1, "numeral_key": "x", "title_key": "y"}]})")
                     .ok)
        << "缺 done_flag";
    EXPECT_FALSE(write(R"({"closing_key": "a", "to_be_continued_key": "b", "chapters": [
                          {"number": 2, "numeral_key": "x", "title_key": "y", "done_flag": "f2"},
                          {"number": 1, "numeral_key": "x", "title_key": "y", "done_flag": "f1"}]})")
                     .ok)
        << "章号倒序：「下一章」就会排错";
    const auto good = write(R"({"closing_key": "a", "to_be_continued_key": "b", "chapters": [
                          {"number": 1, "numeral_key": "x", "title_key": "y", "done_flag": "f1"}]})");
    EXPECT_TRUE(good.ok) << good.error;
}

TEST(ChapterCards, TheCardFadesInHoldsAndFadesOut) {
    using fanren::game::ChapterCardScene;
    const double in = ChapterCardScene::kFadeInSeconds;
    const double hold = ChapterCardScene::kHoldSeconds;
    const double out = ChapterCardScene::kFadeOutSeconds;
    EXPECT_NEAR(hold, 2.5, 1e-9) << "派工单：停约 2.5 秒";
    EXPECT_FLOAT_EQ(ChapterCardScene::visibilityAt(0.0, hold), 0.f);
    EXPECT_NEAR(ChapterCardScene::visibilityAt(in / 2.0, hold), 0.5f, 1e-4);
    EXPECT_FLOAT_EQ(ChapterCardScene::visibilityAt(in + hold / 2.0, hold), 1.f);
    EXPECT_NEAR(ChapterCardScene::visibilityAt(in + hold + out / 2.0, hold), 0.5f, 1e-4);
    // 浮点：in + hold + out 再减回去不一定分毫不差，量到百万分之一即可。
    EXPECT_NEAR(ChapterCardScene::visibilityAt(in + hold + out, hold), 0.f, 1e-6);
    EXPECT_FLOAT_EQ(ChapterCardScene::visibilityAt(in + hold + out + 0.01, hold), 0.f);
    EXPECT_FLOAT_EQ(ChapterCardScene::visibilityAt(1000.0, -1.0), 1.f) << "结局卡一直停着等确认";
}

// ---------------------------------------------------------------------------
// 无头下，章节卡不挡脚本、不留在场景栈上
// ---------------------------------------------------------------------------

class HeadlessChapterCards : public ::testing::Test {
protected:
    void SetUp() override {
        auto ready = app_.init(assetRoot(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
        auto map = app_.loadMap("ch01_shenshougu", std::string{});
        ASSERT_TRUE(map.ok) << map.error;
        app_.pushScene(std::make_unique<fanren::game::WorldScene>());
        app_.tick(kFrame);
        ASSERT_NE(app_.topScene(), nullptr);
        ASSERT_EQ(app_.topScene()->name(), "World");
    }
    void TearDown() override { app_.shutdown(); }

    std::string top() {
        return app_.topScene() == nullptr ? std::string("<空>") : app_.topScene()->name();
    }

    // 每一帧之后都看一眼栈顶：无头下章节卡**一帧也不许**出现在栈上。只在最后看一眼的话，
    // 一张「压上去、下一帧自己退场」的卡会从两次检查之间溜过去。
    void tick() {
        app_.tick(kFrame);
        if (top() == "ChapterCard") ++cardFrames_;
    }

    // 像主循环那样把脚本推到结束，替玩家按确认；对话框演完就收掉（与各章通关测试同一个做法）。
    void pumpScript() {
        for (int frame = 0; frame < 4000 && app_.scripts().isRunning(); ++frame) {
            tick();
            if (!app_.awaitingCommand()) continue;
            fanren::script::CommandResult result;
            result.ok = true;
            result.choiceIndex = 0;
            app_.completeCommand(result);
            app_.popScene();
        }
        ASSERT_FALSE(app_.scripts().isRunning()) << "脚本没能跑到结束";
        tick();
    }

    static constexpr double kFrame = 1.0 / 60.0;
    fanren::game::Application app_;
    int cardFrames_ = 0;
};

TEST_F(HeadlessChapterCards, TheCardsNeverBlockTheScriptOrTheSceneStack) {
    // 第一章章末那一场（授口诀）：最后一句是 flag.set("ch01.done")。
    // 开演前在世界层上面压一块告示板——它挡着的时候，卡片只许排、不许播。
    app_.pushScene(std::make_unique<fanren::game::BoardScene>());
    tick();
    ASSERT_EQ(top(), "Board");
    app_.state().setFlag("ch01.dazuo_done", 1);
    ASSERT_TRUE(app_.startEvent("ch01/shenshougu_koujue.lua").ok);
    pumpScript();

    ASSERT_EQ(app_.state().flag("ch01.done"), 1) << "先验：这一场真的演到了章末";
    const std::vector<CardRequest> expected{{CardKind::Closing, 1}, {CardKind::Opening, 2}};
    EXPECT_EQ(app_.pendingCards(), expected) << "章末旗标一置就该排上「第一章　终」接第二章";
    EXPECT_TRUE(app_.cardLog().empty()) << "顶上是告示板、不是行走：不许播";
    EXPECT_EQ(top(), "Board");

    // 告示板收掉之前先开演另一场：脚本在跑，还是不许播。
    app_.state().setFlag("ch01.dazuo_done", 0);   // 于是这一场只说一句「还不到时候」
    ASSERT_TRUE(app_.startEvent("ch01/shenshougu_koujue.lua").ok);
    app_.popScene();
    tick();
    EXPECT_TRUE(app_.awaitingCommand()) << "先验：第二场正停在一句台词上";
    EXPECT_TRUE(app_.cardLog().empty()) << "脚本还在跑：不许播";
    pumpScript();
    tick();

    // 回到行走：播了（无头下是「立即播完」），而且场景栈上一张卡也没留下。
    EXPECT_EQ(app_.cardLog(), expected);
    EXPECT_TRUE(app_.pendingCards().empty());
    for (int frame = 0; frame < 3; ++frame) tick();
    EXPECT_EQ(top(), "World") << "无头下章节卡不停留：顶上必须还是行走";
    EXPECT_EQ(cardFrames_, 0) << "无头下章节卡在栈顶出现过 " << cardFrames_ << " 帧";

    // 也不挡下一场：紧接着开演，第一帧就停在它的第一句台词上。
    ASSERT_TRUE(app_.startEvent("ch01/shenshougu_koujue.lua").ok);
    tick();
    EXPECT_TRUE(app_.awaitingCommand());
    EXPECT_EQ(top(), "Dialogue");
}

// ---------------------------------------------------------------------------
// 淡入淡出与等待
// ---------------------------------------------------------------------------

TEST(FadeLevels, OutInAndWaitLandWhereTheyShould) {
    using fanren::game::FadeScene;
    EXPECT_FLOAT_EQ(FadeScene::levelAt(FadeScene::Kind::Out, 0.f, 0.0, 0.3), 0.f);
    EXPECT_FLOAT_EQ(FadeScene::levelAt(FadeScene::Kind::Out, 0.f, 0.15, 0.3), 0.5f);
    EXPECT_FLOAT_EQ(FadeScene::levelAt(FadeScene::Kind::Out, 0.f, 0.3, 0.3), 1.f);
    EXPECT_FLOAT_EQ(FadeScene::levelAt(FadeScene::Kind::In, 1.f, 0.15, 0.3), 0.5f);
    EXPECT_FLOAT_EQ(FadeScene::levelAt(FadeScene::Kind::In, 1.f, 9.0, 0.3), 0.f);
    EXPECT_FLOAT_EQ(FadeScene::levelAt(FadeScene::Kind::Wait, 0.7f, 0.1, 0.3), 0.7f)
        << "等待不动黑幕";
    EXPECT_FLOAT_EQ(FadeScene::levelAt(FadeScene::Kind::Out, 0.4f, 0.0, 0.0), 1.f)
        << "时长为 0 直接到终值";
    EXPECT_FLOAT_EQ(FadeScene::levelAt(FadeScene::Kind::Out, 0.6f, 0.15, 0.3), 0.8f)
        << "从当前浓度接着推，不先亮一下";
}

// ---------------------------------------------------------------------------
// 主菜单的用词
// ---------------------------------------------------------------------------

namespace {

// 禁词：第 2 章硬约束 #8 的原文 + 第 2 章复审 HIGH-2 点名的（与 PanelTests 那张同源），
// 再加 LexiconTests 表一里首见晚于第 5 章的面板常用词。
const std::vector<std::string>& menuForbiddenWords() {
    static const std::vector<std::string> kWords{
        "修仙", "灵根", "法术", "灵气", "气感", "炼气", "筑基", "真元", "走火入魔", "法力",
        "境界", "灵石", "元神", "金丹", "神通", "法宝", "丹田", "真气", "储物袋", "灵符", "坊市",
    };
    return kWords;
}

std::string firstForbidden(const std::string& text) {
    for (const std::string& word : menuForbiddenWords()) {
        if (text.find(word) != std::string::npos) return word;
    }
    return {};
}

}  // namespace

TEST(MenuWording, TheMortalMenuSaysNothingAnElevenYearOldCouldNotSay) {
    const fanren::core::GameData data = shippedData();
    using fanren::game::MenuScene;
    using fanren::game::PanelStage;

    // 扫描器先有牙。
    ASSERT_EQ(firstForbidden("囊中灵石不足"), "灵石");
    ASSERT_TRUE(firstForbidden("碎银 28 块").empty());

    const std::vector<std::string> mortal = MenuScene::menuStrings(data, PanelStage::Mortal);
    ASSERT_GE(mortal.size(), 20u) << "用词表缩水了，这条扫描就扫了个寂寞";
    for (const std::string& text : mortal) {
        EXPECT_FALSE(text.empty()) << "有一条菜单字查出来是空的";
        EXPECT_TRUE(firstForbidden(text).empty())
            << "凡人阶段的菜单上出现了「" << firstForbidden(text) << "」：" << text;
        EXPECT_EQ(text.rfind("ui.", 0), std::string::npos) << "文案 key 没查到正文：" << text;
    }
    EXPECT_EQ(MenuScene::pageLabel(data, PanelStage::Mortal, MenuScene::Page::Magic), "法门");

    // 负向对照：修仙阶段那一套本来就该说这些词——两套若是同一套，上面那条不可能转红。
    int hits = 0;
    for (const std::string& text : MenuScene::menuStrings(data, PanelStage::Immortal)) {
        if (!firstForbidden(text).empty()) ++hits;
    }
    EXPECT_GE(hits, 3) << "修仙阶段的菜单里扫不出修仙词，两张表多半是同一张";
    EXPECT_EQ(MenuScene::pageLabel(data, PanelStage::Immortal, MenuScene::Page::Magic), "法术");
}

// 系统设置（docs/settings.md 5.4、第 7 节第 6 条）：设置面板与两处入口（标题画面、主菜单）的字过同一张禁词表。
// 设置是给这台机器调的，第 1 章的玩家就会打开它——面板上一个修仙词都不许有。
TEST(SettingsWording, ThePanelAndBothEntrancesPassTheForbiddenWordScan) {
    const fanren::core::GameData data = shippedData();
    using fanren::game::MenuScene;
    using fanren::game::PanelStage;
    using fanren::game::SettingsScene;
    using fanren::game::TitleScene;

    const std::vector<std::string> panel = SettingsScene::settingsStrings(data);
    // 11 行 × (名字 + 说明) + 3 个分组 + 12 个档位的叫法 + 标题、恢复默认、两句写盘失败、按键提示（键盘版与手柄版）。
    ASSERT_GE(panel.size(), 43u) << "用词表缩水了，这条扫描就扫了个寂寞";
    // 改键面板（二期）：10 个动作名 + 14 句结果 + 标题、表头五栏（docs/gamepad.md 第 7 节加了「手柄」）、恢复默认键位、空格位、
    // 抓键提示、两句说明、按键提示，再加最后四条的手柄版。
    const std::vector<std::string> keyPanel = fanren::game::KeyConfigScene::keyConfigStrings(data);
    ASSERT_GE(keyPanel.size(), 40u) << "改键面板的用词表缩水了";
    std::vector<std::string> all = panel;
    for (const std::string& text : keyPanel) all.push_back(text);
    for (const std::string& text : TitleScene::menuStrings(data)) all.push_back(text);
    for (const std::string& text : MenuScene::menuStrings(data, PanelStage::Mortal)) all.push_back(text);
    // 手柄模式下换上的全部 .pad 文案（docs/gamepad.md 第 6 节那 11 条；id 写死在这里，不从被测的表里推）。
    for (const char* id : {"ui.title.keys.pad", "ui.menu.keys.pad", "ui.dialogue.keys.pad", "ui.path.keys.pad",
                           "ui.settings.hint.pad", "ui.keys.hint.pad", "ui.keys.capture.pad", "ui.keys.desc.pad",
                           "ui.keys.desc.restore.pad", "ui.battle.hint.menu.pad", "ui.battle.hint.watch.pad"}) {
        all.push_back(data.lookupText(id));
    }
    for (const std::string& text : all) {
        EXPECT_FALSE(text.empty()) << "有一条固有字查出来是空的";
        EXPECT_TRUE(firstForbidden(text).empty()) << "出现了「" << firstForbidden(text) << "」：" << text;
        EXPECT_EQ(text.rfind("ui.", 0), std::string::npos) << "文案 key 没查到正文：" << text;
    }
    // 两处入口的字：标题画面第 3 项、主菜单「存盘」与「返回」之间那一项。
    EXPECT_EQ(data.lookupText("ui.title.settings"), "设置");
    EXPECT_EQ(MenuScene::pageLabel(data, PanelStage::Mortal, MenuScene::Page::Settings), "设置");
    // 契约第 1 节的行名，逐字抄（面板的行序即这张表的 # 次序）。docs/gamepad.md 第 8 节在「文字速度」之后插了
    // 「手柄震动」（新真值：11 行，行号照那一节写死的次序）。
    const std::vector<std::string> kRowNames{"音乐音量", "音效音量", "显示模式", "画面缩放", "垂直同步", "画面特效",
                                             "战斗震屏", "文字速度", "手柄震动", "按键设置", "恢复默认"};
    ASSERT_EQ(kRowNames.size(), static_cast<std::size_t>(SettingsScene::kRowCount));
    for (int row = 0; row < SettingsScene::kRowCount; ++row) {
        EXPECT_EQ(SettingsScene::rowLabel(data, row), kRowNames[static_cast<std::size_t>(row)]) << "第 " << row << " 行";
    }
    // 新行的说明与列头（docs/gamepad.md 第 7、8 节原文）。
    EXPECT_EQ(data.lookupText("ui.settings.desc.pad_rumble"), "破势、挨重击时手柄震一下。用键盘时不震。");
    EXPECT_EQ(data.lookupText("ui.keys.col.gamepad"), "手柄");
}

TEST(MenuItems, TheMoneyIsNotListedAsAnItemAndHerbsCarryTheirAge) {
    const fanren::core::GameData data = shippedData();
    fanren::core::GameState state;
    state.addItem("material_lingshi", 28);
    state.addItem("herb_huangjing_cao", 2, 44);
    const auto rows = fanren::game::MenuScene::itemRows(data, state);
    ASSERT_EQ(rows.size(), 1u) << "钱（物品名是「灵石」）不进物品页";
    EXPECT_NE(rows[0].label.find("44 年"), std::string::npos) << rows[0].label;
    EXPECT_EQ(rows[0].detail, "×2");
    for (const auto& row : rows) EXPECT_TRUE(firstForbidden(row.label).empty()) << row.label;
}

// ---------------------------------------------------------------------------
// 对话框的名签、标题画面的视差
// ---------------------------------------------------------------------------

TEST(DialogueNameTag, TheTagStaysOnScreenAndOffTheFirstLine) {
    using fanren::game::DialogueScene;
    const fanren::ui::Theme theme;
    for (const int rows : {0, 1, 3, DialogueScene::maxOptionRows(theme), 1000}) {
        const fanren::engine::Rect box = DialogueScene::boxAreaFor(rows, theme);
        const fanren::engine::Rect tag = DialogueScene::nameTagArea(box, 120);
        EXPECT_GE(tag.y, 0) << rows << " 个选项时名签被顶出了屏幕";
        EXPECT_LE(tag.x + tag.w, fanren::engine::kLogicalWidth);
        EXPECT_LT(tag.y, box.y) << "名签要骑在框的上沿";
        EXPECT_LT(tag.y + tag.h - box.y, theme.padding) << "伸进框里的那一截压到了正文第一行";
    }
}

TEST(TitleParallax, TheLayerOffsetStaysWithinOnePeriod) {
    using fanren::game::TitleScene;
    for (double t = 0.0; t < 2000.0; t += 13.37) {
        const float offset = TitleScene::layerOffset(12.f, t, 1280.f);
        EXPECT_GE(offset, 0.f);
        EXPECT_LT(offset, 1280.f);
    }
    EXPECT_FLOAT_EQ(TitleScene::layerOffset(0.f, 99.0, 1280.f), 0.f) << "不漂的那一层就不漂";
    EXPECT_NEAR(TitleScene::layerOffset(10.f, 3.0, 1280.f), 30.f, 1e-4);
}
