// game/SpriteAtlas：人物精灵与地图物件的图集（assets/art/sprites/index.json，A2 路生成）。
//
// 两类用例：
//   * 外观选择与帧选择的**规则**——用手写的小索引，不依赖仓库里的生成物；
//   * 与 A2 的**契约**——拿仓库里真的 index.json：韩立、张铁第 1–2 章用少年版（施工图 / 派工单
//     写死的口径：判据旗标 ch02.done），九种设施、四向传送箭头、交互闪光、四种路径行动气泡都有图。
//     这几条红了，说明两路对 docs/art-sprites.md 第 5 节的理解分了家。
#include <gtest/gtest.h>

#include <filesystem>
#include <string>

#include "core/model/Types.h"
#include "game/SpriteAtlas.h"
#include "game/WorldView.h"
#include "io/VisualLoader.h"

namespace {

namespace fs = std::filesystem;
using fanren::core::GameState;
using fanren::game::CharacterSheet;
using fanren::game::lookForRole;
using fanren::io::parseSpriteIndex;
using fanren::game::ChapterDone;
using fanren::game::SpriteIndex;

// 仓库根：测试可能从 build-xxx/ 或工程根启动。判据用一定存在的那张图。
std::string repoRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "maps" / "ch01_hanjiacun.tmj")) return candidate;
    }
    return ".";
}

std::string realIndexPath() {
    return (fs::path(repoRoot()) / "assets" / "art" / "sprites" / "index.json").string();
}

// 一张最小的索引：两个外观、一个带变体的角色、一个物件。
constexpr const char* kSmallIndex = R"({
 "version": 1, "frame_w": 16, "frame_h": 24,
 "walk_cycle": [0, 1, 0, 2], "walk_fps": 8,
 "sheets": {
  "boy": {"file": "chars/boy.png", "cols": 3, "foot_y": 23, "head_y": 4,
          "walk": {"down": [0,1,2], "left": [3,4,5], "right": [6,7,8], "up": [9,10,11]}},
  "man": {"file": "chars/man.png", "frame_w": 24, "frame_h": 32,
          "walk": {"down": [0,1,2], "left": [3,4,5], "right": [6,7,8], "up": [9,10,11]}}
 },
 "roles": {"hero": "man", "villager": "boy", "ghost": "no_such_look"},
 "role_variants": {"hero": [{"max_chapter": 2, "look": "boy"}],
                   "elder": [{"min_chapter": 5, "look": "man"}]},
 "objects": {"file": "objects.png", "emissive": "objects_emissive.png", "cell": 16,
  "items": {"portal.arrow_up": {"frames": [[0,96,16,16],[16,96,16,16]], "anchor": [8,16], "fps": 6.0, "blend": "add"},
            "facility.save": {"frames": [[0,64,32,32]]}}},
 "facilities": {"save": "facility.save"}
})";

// 「第 1 … last 章都演完了」。
ChapterDone doneThrough(int last) {
    return [last](int n) { return n >= 1 && n <= last; };
}

// 判据写死成设计原文：第 n 章演完 ⇔ 存档里 chNN.done 置位（ch02.done 就是第 2 章）。
ChapterDone doneFlagsOf(const GameState& state) {
    return [&state](int n) {
        const std::string flag = std::string("ch") + (n < 10 ? "0" : "") + std::to_string(n) + ".done";
        return state.flag(flag) != 0;
    };
}

SpriteIndex smallIndex() {
    auto parsed = parseSpriteIndex(kSmallIndex);
    EXPECT_TRUE(parsed.ok) << parsed.error;
    return parsed.value;
}

// ---------------------------------------------------------------------------
// 外观选择的规则
// ---------------------------------------------------------------------------

TEST(SpriteAtlas, VariantsHoldWhileTheirChapterIsNotDoneThenRolesTakeOver) {
    const SpriteIndex index = smallIndex();
    EXPECT_EQ(lookForRole(index, "hero", doneThrough(0)), "boy") << "一章都没演完";
    EXPECT_EQ(lookForRole(index, "hero", doneThrough(1)), "boy") << "max_chapter 2：第 2 章还没完就还用它";
    EXPECT_EQ(lookForRole(index, "hero", doneThrough(2)), "man") << "第 2 章演完了，回到 roles 那一条";
    EXPECT_EQ(lookForRole(index, "elder", doneThrough(3)), "") << "min_chapter 5 要第 4 章演完；roles 里也没有";
    EXPECT_EQ(lookForRole(index, "elder", doneThrough(4)), "man");
    EXPECT_EQ(lookForRole(index, "villager", doneThrough(0)), "boy");
    EXPECT_EQ(lookForRole(index, "nobody", doneThrough(0)), "") << "查不到的角色没有外观（调用方退回旧画法）";
    // 判据是「那一章演完没有」这一个旗标，不是推出来的章号：存档里有第 3 章的章末旗标、
    // 却没有第 2 章的，照样按第 2 章没完算——主菜单小像（MenuScene）走的也是这一条。
    const ChapterDone gap = [](int n) { return n == 3; };
    EXPECT_EQ(lookForRole(index, "hero", gap), "boy");
}

TEST(SpriteAtlas, SheetForRoleIsNullWhenTheLookHasNoSheet) {
    const SpriteIndex index = smallIndex();
    ASSERT_NE(fanren::game::sheetForRole(index, "hero", doneThrough(2)), nullptr);
    EXPECT_EQ(fanren::game::sheetForRole(index, "hero", doneThrough(2))->file, "chars/man.png");
    EXPECT_EQ(lookForRole(index, "ghost", doneThrough(0)), "no_such_look");
    EXPECT_EQ(fanren::game::sheetForRole(index, "ghost", doneThrough(0)), nullptr)
        << "外观 id 写了、人物表却没有：当作没有图，而不是拿一张空表去画";
}

TEST(SpriteAtlas, SheetFieldsFallBackToTheDocumentedDefaults) {
    const SpriteIndex index = smallIndex();
    const CharacterSheet& boy = index.sheets.at("boy");
    EXPECT_EQ(boy.frameW, 16) << "帧宽缺省取顶层的 frame_w";
    EXPECT_EQ(boy.headY, 4);
    const CharacterSheet& man = index.sheets.at("man");
    EXPECT_EQ(man.frameW, 24);
    EXPECT_EQ(man.frameH, 32);
    EXPECT_EQ(man.cols, 3) << "列数缺省 3";
    EXPECT_EQ(man.footY, 31) << "foot_y 缺省是最后一行（脚底描边压在最后一行）";
    EXPECT_EQ(man.headY, 0);
}

// ---------------------------------------------------------------------------
// 帧
// ---------------------------------------------------------------------------

TEST(SpriteAtlas, StandingUsesTheFirstFrameOfEachDirection) {
    const SpriteIndex index = smallIndex();
    const CharacterSheet& boy = index.sheets.at("boy");
    // facing 与 GameState 同一套：0 上 / 1 右 / 2 下 / 3 左。
    EXPECT_EQ(fanren::game::walkFrame(index, boy, 0, false, 5.f), 9);
    EXPECT_EQ(fanren::game::walkFrame(index, boy, 1, false, 5.f), 6);
    EXPECT_EQ(fanren::game::walkFrame(index, boy, 2, false, 5.f), 0);
    EXPECT_EQ(fanren::game::walkFrame(index, boy, 3, false, 5.f), 3);
}

TEST(SpriteAtlas, WalkingPlaysTheCycleAtWalkFps) {
    const SpriteIndex index = smallIndex();
    const CharacterSheet& boy = index.sheets.at("boy");
    // walk_cycle [0,1,0,2] 以 8 帧/秒：站、迈甲、站、迈乙，每帧 0.125 秒。
    const float step = 1.f / 8.f;
    EXPECT_EQ(fanren::game::walkFrame(index, boy, 2, true, 0.f), 0);
    EXPECT_EQ(fanren::game::walkFrame(index, boy, 2, true, step * 1.5f), 1);
    EXPECT_EQ(fanren::game::walkFrame(index, boy, 2, true, step * 2.5f), 0);
    EXPECT_EQ(fanren::game::walkFrame(index, boy, 2, true, step * 3.5f), 2);
    EXPECT_EQ(fanren::game::walkFrame(index, boy, 2, true, step * 4.5f), 0) << "播完一轮从头来";
    EXPECT_EQ(fanren::game::walkFrame(index, boy, 1, true, step * 1.5f), 7) << "朝右的迈甲";
}

TEST(SpriteAtlas, AFacingFromACorruptSaveDoesNotIndexOutOfTheTable) {
    const SpriteIndex index = smallIndex();
    const CharacterSheet& boy = index.sheets.at("boy");
    EXPECT_EQ(fanren::game::walkFrame(index, boy, 6, false, 0.f), 0) << "6 取模 4 是 2（朝下）";
    EXPECT_EQ(fanren::game::walkFrame(index, boy, -1, false, 0.f), 3) << "-1 取模 4 是 3（朝左）";
}

TEST(SpriteAtlas, FrameNumbersMapToSheetRectangles) {
    const SpriteIndex index = smallIndex();
    const auto r = fanren::game::sheetFrameRect(index.sheets.at("boy"), 7);   // 第 2 行第 1 列
    EXPECT_FLOAT_EQ(r.x, 16.f);
    EXPECT_FLOAT_EQ(r.y, 48.f);
    EXPECT_FLOAT_EQ(r.w, 16.f);
    EXPECT_FLOAT_EQ(r.h, 24.f);
    const auto big = fanren::game::sheetFrameRect(index.sheets.at("man"), 4);
    EXPECT_FLOAT_EQ(big.x, 24.f);
    EXPECT_FLOAT_EQ(big.y, 32.f);
}

TEST(SpriteAtlas, ObjectsAnimateAtTheirFpsAndStillOnesStayOnFrameZero) {
    const SpriteIndex index = smallIndex();
    const auto& arrow = index.objects.at("portal.arrow_up");
    EXPECT_TRUE(arrow.additive);
    EXPECT_FLOAT_EQ(arrow.anchorX, 8.f);
    EXPECT_EQ(fanren::game::objectFrame(arrow, 0.f), 0);
    EXPECT_EQ(fanren::game::objectFrame(arrow, 1.f / 6.f + 0.01f), 1);
    EXPECT_EQ(fanren::game::objectFrame(arrow, 2.f / 6.f + 0.01f), 0) << "两帧轮播";
    const auto& save = index.objects.at("facility.save");
    EXPECT_FALSE(save.additive);
    EXPECT_EQ(fanren::game::objectFrame(save, 99.f), 0);
    // 锚点缺省：第 0 帧的底边中点。
    EXPECT_FLOAT_EQ(save.anchorX, 16.f);
    EXPECT_FLOAT_EQ(save.anchorY, 32.f);
}

// ---------------------------------------------------------------------------
// 坏文件
// ---------------------------------------------------------------------------

TEST(SpriteAtlas, BrokenIndexesAreErrorsNotSilentDefaults) {
    const char* broken[] = {
        R"({"sheets": {"a": {"walk": {"down":[0,1,2],"left":[3,4,5],"right":[6,7,8],"up":[9,10,11]}}}})",   // 缺 file
        R"({"sheets": {"a": {"file": "a.png", "walk": {"down":[0,1,2],"left":[3,4,5],"right":[6,7,8]}}}})",   // 缺一向
        R"({"sheets": {"a": {"file": "a.png", "walk": {"down":[0],"left":[3],"right":[6],"up":[9]}}}})",      // 帧比播放序短
        R"({"sheets": {"a": {"file": "a.png", "walk": {"down":[0,1,2],"left":[3,4,5],"right":[6,7,8],"up":[9,10,1.5]}}}})",
        R"({"sheets": {}, "objects": {"file": "o.png", "items": {"x": {"frames": []}}}})",                   // 物件没有帧
        R"({"sheets": {}, "objects": {"file": "o.png", "items": {"x": {"frames": [[0,0,16,16]], "blend": "screen"}}}})",
        R"({"sheets": {}, "roles": {"a": 3}})",
        R"({"sheets": {}, "role_variants": {"a": [{"max_chapter": 2}]}})",                                   // 变体缺 look
        R"({"sheets": {}, "walk_cycle": []})",
        R"({"version": 1})",                                                                                // 没有 sheets
        R"([1, 2, 3])",
        R"({"sheets": {},})",
    };
    for (const char* text : broken) {
        auto parsed = parseSpriteIndex(text);
        EXPECT_FALSE(parsed.ok) << "竟然读成功了：" << text;
        EXPECT_NE(parsed.error.find("index.json"), std::string::npos) << parsed.error;
    }
}

// ---------------------------------------------------------------------------
// 与 A2 的契约：仓库里那一份 index.json
// ---------------------------------------------------------------------------

class RealSpriteIndex : public ::testing::Test {
protected:
    void SetUp() override {
        if (!fs::exists(realIndexPath())) GTEST_SKIP() << "仓库里还没有 " << realIndexPath();
        auto loaded = fanren::io::loadSpriteIndex(realIndexPath());
        ASSERT_TRUE(loaded.ok) << loaded.error;
        index_ = std::move(loaded.value);
    }
    SpriteIndex index_;
};

TEST_F(RealSpriteIndex, HanliAndZhangTieAreBoysUntilChapterTwoIsDone) {
    // 派工单的原话：韩立用 role_variants，第 1–2 章少年版，判据旗标 ch02.done；张铁同理。
    // 判据写成「旗标 → 章 → 外观」这一整条，不从 index.json 里推——从被测物推出来的判据
    // 只发现得了「它变了」。
    GameState state;
    const ChapterDone done = doneFlagsOf(state);
    EXPECT_EQ(lookForRole(index_, "hanli", done), "hanli_child");
    EXPECT_EQ(lookForRole(index_, "zhang_tie", done), "zhang_tie_child");
    state.setFlag("ch01.done", 1);
    EXPECT_EQ(lookForRole(index_, "hanli", done), "hanli_child");
    EXPECT_EQ(lookForRole(index_, "zhang_tie", done), "zhang_tie_child");
    state.setFlag("ch02.done", 1);
    EXPECT_EQ(lookForRole(index_, "hanli", done), "hanli");
    EXPECT_EQ(lookForRole(index_, "zhang_tie", done), "zhang_tie");
    // 两套都真有人物表（有外观 id、没有表，等于没有图）。
    for (const char* look : {"hanli", "hanli_child", "zhang_tie", "zhang_tie_child"}) {
        EXPECT_EQ(index_.sheets.count(look), 1u) << look;
    }
}

TEST_F(RealSpriteIndex, EveryFacilityKindHasAnObjectSprite) {
    // docs/map_spec.md 4.6 的九种 kind。缺一种，那种设施就退回旧画法的底板。
    for (const char* kind : {"alchemy", "forge", "talisman", "formation", "field", "meditate", "shop",
                             "board", "save"}) {
        const auto it = index_.facilities.find(kind);
        ASSERT_NE(it, index_.facilities.end()) << kind;
        EXPECT_EQ(index_.objects.count(it->second), 1u) << kind << " → " << it->second;
    }
}

TEST_F(RealSpriteIndex, PortalsSparklesAndBubblesHaveTheSpritesTheWorldAsksFor) {
    // 世界画面按名字要这些物件（WorldView.cpp）。名字对不上时画面静默退回旧画法——
    // 这一条让「对不上」在门禁上响。
    for (const char* id : {"portal.arrow_up", "portal.arrow_right", "portal.arrow_down",
                           "portal.arrow_left", "portal.door", "mark.sparkle"}) {
        EXPECT_EQ(index_.objects.count(id), 1u) << id;
    }
    using fanren::game::PathBubble;
    for (const PathBubble bubble : {PathBubble::Generic, PathBubble::Inquire, PathBubble::Purchase,
                                    PathBubble::Challenge}) {
        const char* id = fanren::game::pathBubbleSprite(bubble);
        ASSERT_NE(id, nullptr);
        EXPECT_EQ(index_.objects.count(id), 1u) << id;
    }
    EXPECT_EQ(fanren::game::pathBubbleSprite(PathBubble::None), nullptr);
    // 闪光与传送箭头是纯光：加色画，不是贴一张不透明的图。
    EXPECT_TRUE(index_.objects.at("mark.sparkle").additive);
    EXPECT_TRUE(index_.objects.at("portal.arrow_down").additive);
    EXPECT_GT(index_.objects.at("mark.sparkle").fps, 0.f) << "闪光要能播，fps 为 0 就只剩第一帧";
}

}  // namespace
