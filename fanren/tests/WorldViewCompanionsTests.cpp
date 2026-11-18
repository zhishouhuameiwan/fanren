// 伙伴改版 · 世界画面（A 路，docs/interfaces-companions.md 2.4–2.5、3.4–3.7、第 6 节）：
//   · 人物按每张表自己的倍数落地、按每张表自己的播放序走路（旧表 ×3 与改版前写死的落点逐像素相同）；
//   · 跟随者：WorldView 自己给每位留一个 StepSlide，照主角的做法判滑还是瞬移，换图 / 主角被挪 / 名单变了整列重来；
//   · 四种新对象：挑哪张物件图、开了的机关不再立着、明雷本体用哪张表哪一帧、谁先画谁后画；
//   · 缺图不崩：软件渲染器上真画一遍，新对象与跟随者取不到图也画得出看得见的东西。
//
// 期望值照契约与任务书写死（物件 id 是契约第 6 节定死的那八个），不从被测代码里取。
// 跟随者那一组驱动真的 WorldScene::tryStep 挪主角，WorldView::trackFollowers 就是 render 每帧开头调的那一个。
#include <gtest/gtest.h>

#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "TempDir.h"
#include "core/model/Types.h"
#include "game/Application.h"
#include "game/SpriteAtlas.h"
#include "game/WorldProps.h"
#include "game/WorldScene.h"
#include "game/WorldView.h"
#include "io/VisualLoader.h"

namespace {

namespace fs = std::filesystem;
using fanren::core::CharacterSheet;
using fanren::core::EnemySheet;
using fanren::core::MapObject;
using fanren::core::Point;
using fanren::core::SpriteIndex;
using fanren::core::TileMap;
using fanren::game::Vec2f;
using fanren::game::WorldDrawItem;
using fanren::game::WorldFrame;
using Kind = fanren::game::WorldDrawItem::Kind;

constexpr float kCell = 48.f;   // 一格在屏幕上 48 像素（施工图 1.1）

std::string repoRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "maps" / "ch01_hanjiacun.tmj")) return candidate;
    }
    return ".";
}

const fanren::game::ChapterDone kNoChapterDone = [](int) { return false; };

// ---------------------------------------------------------------------------
// 按表的倍数与播放序
// ---------------------------------------------------------------------------

TEST(WorldViewCompanions, AnOldSheetLandsExactlyWhereItDidAtTheHardWiredTimesThree) {
    // 生成的旧表：16×24、脚底第 23 行、不写 scale（缺省 3）。改版前 drawCharacter 的公式：
    // w = 16·3、h = 24·3、左 = round(脚底x − w/2)、上 = round(脚底y − (23+1)·3)。
    CharacterSheet sheet;
    ASSERT_EQ(sheet.scale, 3) << "缺省倍数就是改版前写死的 kArtScale";
    const auto dst = fanren::game::characterDest(sheet, 100.f, 200.f);
    EXPECT_FLOAT_EQ(dst.x, 76.f);
    EXPECT_FLOAT_EQ(dst.y, 128.f);
    EXPECT_FLOAT_EQ(dst.w, 48.f);
    EXPECT_FLOAT_EQ(dst.h, 72.f);
    EXPECT_FLOAT_EQ(fanren::game::characterHeadTop(sheet, 200.f), 128.f);
    // 半像素的脚底照旧取整到屏幕像素。
    const auto half = fanren::game::characterDest(sheet, 100.5f, 200.f);
    EXPECT_FLOAT_EQ(half.x, std::round(100.5f - 24.f));
}

TEST(WorldViewCompanions, AHighResolutionSheetIsDrawnAtItsOwnTimesTwoAndOverhangsItsCell) {
    // 定稿手绘表：每帧 64×60、脚底第 57 行、头顶第 2 行、scale 2。
    CharacterSheet hd;
    hd.frameW = 64;
    hd.frameH = 60;
    hd.footY = 57;
    hd.headY = 2;
    hd.scale = 2;
    const auto dst = fanren::game::characterDest(hd, 100.f, 200.f);
    EXPECT_FLOAT_EQ(dst.w, 128.f);
    EXPECT_FLOAT_EQ(dst.h, 120.f);
    EXPECT_FLOAT_EQ(dst.x, 36.f);
    EXPECT_FLOAT_EQ(dst.y, 200.f - 58.f * 2.f);
    EXPECT_GT(dst.w, kCell * 2.f) << "比一格宽得多：左右叠住相邻格，排序只看脚底";
    EXPECT_FLOAT_EQ(fanren::game::characterHeadTop(hd, 200.f), 200.f - 56.f * 2.f);
}

TEST(WorldViewCompanions, AHighResolutionSheetWalksThroughAllFourOfItsFrames) {
    SpriteIndex index;   // walk_fps 8
    CharacterSheet hd;
    hd.walk[2] = {10, 11, 12, 13};
    hd.walkCycle = {0, 1, 2, 3};
    const float step = 1.f / index.walkFps;
    std::vector<int> played;
    for (int i = 0; i < 5; ++i) {
        played.push_back(fanren::game::walkFrame(index, hd, 2, true, static_cast<float>(i) * step + step / 2.f));
    }
    EXPECT_EQ(played, (std::vector<int>{10, 11, 12, 13, 10})) << "第 4 帧播得到";
    CharacterSheet old;
    old.walk[2] = {0, 1, 2};
    played.clear();
    for (int i = 0; i < 4; ++i) {
        played.push_back(fanren::game::walkFrame(index, old, 2, true, static_cast<float>(i) * step + step / 2.f));
    }
    EXPECT_EQ(played, (std::vector<int>{0, 1, 0, 2})) << "旧表照旧 站、迈甲、站、迈乙";
}

// ---------------------------------------------------------------------------
// 四种新对象：挑图与占位
// ---------------------------------------------------------------------------

TEST(WorldViewCompanions, EachNewObjectAsksForTheSpriteIdFixedByTheContract) {
    using namespace fanren::game;
    EXPECT_STREQ(chestSprite(false), "chest_closed");
    EXPECT_STREQ(chestSprite(true), "chest_open");
    EXPECT_STREQ(gatherSprite(false), "gather_ready");
    EXPECT_STREQ(gatherSprite(true), "gather_spent");
    EXPECT_STREQ(gateSprite(false), "gate_closed");
    EXPECT_STREQ(gateSprite(true), "gate_open");
    EXPECT_STREQ(foeMarkSprite(false), "foe_mark");
    EXPECT_STREQ(foeMarkSprite(true), "foe_mark_elite");
}

TEST(WorldViewCompanions, AnOpenGateNoLongerStandsAndItsPlaceholderIsNotASolidBlock) {
    using namespace fanren::game;
    EXPECT_TRUE(gateStands(false));
    EXPECT_FALSE(gateStands(true)) << "开了的机关画进地面一层，不像还挡着路";
    EXPECT_TRUE(gatePlaceholder(false).solid);
    EXPECT_FALSE(gatePlaceholder(true).solid);
    EXPECT_TRUE(chestPlaceholder(true).solid) << "开过的宝箱还在原地，只是空了";
    EXPECT_STRNE(chestPlaceholder(true).glyph, chestPlaceholder(false).glyph) << "开没开看得出来";
    EXPECT_STRNE(gatherPlaceholder(true).glyph, gatherPlaceholder(false).glyph) << "采没采看得出来";
    EXPECT_STRNE(foeMarkPlaceholder(true).glyph, foeMarkPlaceholder(false).glyph) << "精英与寻常明雷分得开";
}

// 明雷本体用的一份小索引：一个人形劫修（角色 → 外观）、一张只按外观 id 查得到的表、一只面朝右的狼、
// 一个面朝左的人形敌人。
SpriteIndex foeIndex() {
    SpriteIndex index;
    CharacterSheet bandit;
    bandit.file = "chars/arch_bandit.png";
    for (auto& dir : bandit.walk) dir = {0, 1, 2};
    index.sheets["arch_bandit"] = bandit;
    CharacterSheet lone = bandit;
    lone.file = "chars/lone.png";
    index.sheets["lone_look"] = lone;
    index.roles["jie_xiu"] = "arch_bandit";
    EnemySheet wolf;
    wolf.file = "enemies/wild_wolf.png";
    wolf.frameW = 32;
    wolf.frameH = 24;
    wolf.cols = 5;
    wolf.footY = 22;
    wolf.headY = 3;
    wolf.battle["idle"] = {3, 4};
    wolf.battleFacing = fanren::core::BattleFacing::Right;
    index.enemies["wild_wolf"] = wolf;
    EnemySheet ghoul = wolf;
    ghoul.battle.clear();
    ghoul.battleFacing = fanren::core::BattleFacing::Left;
    index.enemies["ghoul"] = ghoul;
    return index;
}

TEST(WorldViewCompanions, AFoeLooksUpARoleThenALookThenAnEnemy) {
    const SpriteIndex index = foeIndex();
    const auto bandit = fanren::game::foeBodyFor(index, "jie_xiu", kNoChapterDone);
    ASSERT_NE(bandit.sheet, nullptr);
    EXPECT_EQ(bandit.sheet->file, "chars/arch_bandit.png") << "角色 id 走 roles，与 NPC 同一条";
    const auto lone = fanren::game::foeBodyFor(index, "lone_look", kNoChapterDone);
    ASSERT_NE(lone.sheet, nullptr);
    EXPECT_EQ(lone.sheet->file, "chars/lone.png") << "外观 id 直接查 sheets";
    const auto wolf = fanren::game::foeBodyFor(index, "wild_wolf", kNoChapterDone);
    EXPECT_EQ(wolf.sheet, nullptr);
    ASSERT_NE(wolf.enemy, nullptr);
    EXPECT_EQ(wolf.enemy->file, "enemies/wild_wolf.png");
    const auto nobody = fanren::game::foeBodyFor(index, "no_such_thing", kNoChapterDone);
    EXPECT_EQ(nobody.sheet, nullptr);
    EXPECT_EQ(nobody.enemy, nullptr);
}

TEST(WorldViewCompanions, AnEnemyFoeStandsOnItsIdleFrameAndTurnsToItsFacing) {
    const SpriteIndex index = foeIndex();
    const EnemySheet& wolf = index.enemies.at("wild_wolf");
    EXPECT_EQ(fanren::game::enemyStandFrame(wolf), 3) << "idle 的第一帧";
    EXPECT_EQ(fanren::game::enemyStandFrame(index.enemies.at("ghoul")), 0) << "没有 idle 画第 0 帧";
    const auto rect = fanren::game::enemyFrameRect(wolf, 3);
    EXPECT_FLOAT_EQ(rect.x, 96.f);
    EXPECT_FLOAT_EQ(rect.y, 0.f);
    EXPECT_FLOAT_EQ(rect.w, 32.f);
    // 狼的帧面朝右：明雷朝左（3）才翻，朝右、朝上下不翻。
    EXPECT_TRUE(fanren::game::enemyFlipped(wolf, 3));
    EXPECT_FALSE(fanren::game::enemyFlipped(wolf, 1));
    EXPECT_FALSE(fanren::game::enemyFlipped(wolf, 2));
    EXPECT_FALSE(fanren::game::enemyFlipped(wolf, 0));
    // 面朝左的帧：反过来。
    EXPECT_TRUE(fanren::game::enemyFlipped(index.enemies.at("ghoul"), 1));
    EXPECT_FALSE(fanren::game::enemyFlipped(index.enemies.at("ghoul"), 3));
    // 世界层 ×3：32×24、脚底第 22 行。
    const auto dst = fanren::game::enemyDest(wolf, 100.f, 200.f);
    EXPECT_FLOAT_EQ(dst.w, 96.f);
    EXPECT_FLOAT_EQ(dst.h, 72.f);
    EXPECT_FLOAT_EQ(dst.x, 52.f);
    EXPECT_FLOAT_EQ(dst.y, 200.f - 23.f * 3.f);
}

// ---------------------------------------------------------------------------
// 谁先画谁后画
// ---------------------------------------------------------------------------

MapObject objectAt(const std::string& type, int x, int y) {
    MapObject object;
    object.type = type;
    object.name = type + "_" + std::to_string(x) + "_" + std::to_string(y);
    object.position = Point{x, y};
    return object;
}

std::vector<Kind> kindsOf(const std::vector<WorldDrawItem>& items) {
    std::vector<Kind> kinds;
    for (const WorldDrawItem& item : items) kinds.push_back(item.kind);
    return kinds;
}

const fanren::engine::RectF kWholeScreen{0.f, 0.f, 1280.f, 720.f};

TEST(WorldViewCompanions, NewObjectsSortByTheirFeetWithEveryoneElse) {
    const MapObject npc = objectAt("npc", 2, 5);
    const MapObject chest = objectAt("chest", 4, 3);
    const MapObject gather = objectAt("gather", 6, 7);
    const MapObject gate = objectAt("gate", 8, 4);
    const MapObject foe = objectAt("foe", 3, 6);
    WorldFrame frame;
    frame.npcs.push_back(WorldFrame::Npc{&npc, "zhang_tie", "张铁", 2, fanren::game::PathBubble::None});
    frame.chests.push_back(WorldFrame::Chest{&chest, false});
    frame.gathers.push_back(WorldFrame::Gather{&gather, true});
    frame.gates.push_back(WorldFrame::Gate{&gate, false});
    frame.foes.push_back(WorldFrame::Foe{&foe, "jie_xiu", 1, false});
    // 主角在第 5 行（脚底与 NPC 同一行、偏右），一位跟随者在第 4 行。
    const auto items = fanren::game::depthOrder(frame, Vec2f{5.f, 5.f}, {Vec2f{5.f, 4.f}}, kWholeScreen);
    // 脚底（格底边）：宝箱 4 → 跟随者、机关 5 → NPC、主角 6 → 明雷 7 → 采集点 8；同一行从左到右。
    EXPECT_EQ(kindsOf(items), (std::vector<Kind>{Kind::Chest, Kind::Follower, Kind::Gate, Kind::Npc, Kind::Player,
                                                 Kind::Foe, Kind::Gather}));
}

TEST(WorldViewCompanions, AnOpenGateIsNotAmongTheStandingThings) {
    const MapObject shut = objectAt("gate", 2, 2);
    const MapObject open = objectAt("gate", 6, 2);
    WorldFrame frame;
    frame.gates.push_back(WorldFrame::Gate{&shut, false});
    frame.gates.push_back(WorldFrame::Gate{&open, true});
    const auto items = fanren::game::depthOrder(frame, Vec2f{4.f, 6.f}, {}, kWholeScreen);
    ASSERT_EQ(items.size(), 2u);
    EXPECT_EQ(items[0].kind, Kind::Gate);
    EXPECT_EQ(items[0].index, 0u) << "只有关着的那一道";
    EXPECT_EQ(items[1].kind, Kind::Player);
}

TEST(WorldViewCompanions, FollowersStackedOnTheHeroAreDrawnBehindHim) {
    // 换图之后伙伴叠在主角格上（契约 3.7）：同一个脚底，跟随者先画，被主角挡住。
    WorldFrame frame;
    const auto items = fanren::game::depthOrder(frame, Vec2f{3.f, 3.f}, {Vec2f{3.f, 3.f}, Vec2f{3.f, 3.f}}, kWholeScreen);
    EXPECT_EQ(kindsOf(items), (std::vector<Kind>{Kind::Follower, Kind::Follower, Kind::Player}));
}

TEST(WorldViewCompanions, ThingsWellOffScreenAreNotSorted) {
    const MapObject far = objectAt("foe", 60, 3);
    const MapObject chest = objectAt("chest", 1, 40);
    WorldFrame frame;
    frame.foes.push_back(WorldFrame::Foe{&far, "jie_xiu", 2, true});
    frame.chests.push_back(WorldFrame::Chest{&chest, true});
    const auto items = fanren::game::depthOrder(frame, Vec2f{3.f, 3.f}, {Vec2f{70.f, 3.f}}, kWholeScreen);
    EXPECT_EQ(kindsOf(items), (std::vector<Kind>{Kind::Player}));
}

TEST(WorldViewCompanions, WithoutTheNewListsTheOrderIsTheOldOne) {
    // 默认设置、不带伙伴、不碰新对象：与改版前一样只有设施、NPC、主角，按脚底排。
    const MapObject facility = objectAt("facility", 1, 1);
    const MapObject npc = objectAt("npc", 4, 8);
    WorldFrame frame;
    frame.facilities.push_back(WorldFrame::Facility{&facility, "save"});
    frame.npcs.push_back(WorldFrame::Npc{&npc, "zhang_tie", "张铁", 2, fanren::game::PathBubble::None});
    const auto items = fanren::game::depthOrder(frame, Vec2f{4.f, 5.f}, {}, kWholeScreen);
    EXPECT_EQ(kindsOf(items), (std::vector<Kind>{Kind::Facility, Kind::Player, Kind::Npc}));
}

// ---------------------------------------------------------------------------
// 跟随者的滑步：驱动真的 WorldScene::tryStep
// ---------------------------------------------------------------------------

bool plainCell(const TileMap& map, Point cell) {
    if (!map.walkable(cell)) return false;
    for (const MapObject& object : map.objects) {
        if (object.type == "spawn") continue;
        if (cell.x >= object.position.x && cell.x < object.position.x + std::max(1, object.width) &&
            cell.y >= object.position.y && cell.y < object.position.y + std::max(1, object.height)) {
            return false;
        }
    }
    return true;
}

class FollowerSlide : public ::testing::Test {
protected:
    void SetUp() override {
        auto ready = app_.init(repoRoot(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
        ASSERT_TRUE(app_.loadMap("ch01_qingniuzhen", std::string{}).ok);
    }
    void TearDown() override { app_.shutdown(); }

    // 一排三格空地（往右走两步不碰任何对象）。
    bool placeInTheOpen(Point& from) {
        const TileMap& map = *app_.currentMap();
        for (int y = 10; y < map.height - 10; ++y) {
            for (int x = 12; x < map.width - 12; ++x) {
                if (plainCell(map, {x, y}) && plainCell(map, {x + 1, y}) && plainCell(map, {x + 2, y})) {
                    from = Point{x, y};
                    return true;
                }
            }
        }
        return false;
    }

    static WorldFrame withFollowers(std::vector<WorldFrame::Follower> followers) {
        WorldFrame frame;
        frame.followers = std::move(followers);
        return frame;
    }

    fanren::game::Application app_;
};

TEST_F(FollowerSlide, AFollowerGivenTheHerosLastCellSlidesThereInStepWithHim) {
    Point from;
    ASSERT_TRUE(placeInTheOpen(from)) << "青牛镇中间一带找不到一排三格空地";
    app_.state().position = from;
    fanren::game::WorldView view;
    fanren::game::WorldScene world;
    view.advance(app_, 0.0);
    // 开场：伙伴叠在主角格上。
    view.trackFollowers(app_, withFollowers({{"nangong_wan", from, 2}}));
    ASSERT_EQ(view.followers().size(), 1u);
    EXPECT_FLOAT_EQ(view.followers()[0].slide.position().x, static_cast<float>(from.x));
    EXPECT_FALSE(view.followers()[0].slide.moving());

    // 第一步：主角走开，伙伴的格还是原地（足迹里主角上一格就是这一格），不动。
    ASSERT_TRUE(world.tryStep(app_, 1, 0));
    view.advance(app_, 0.12);
    view.trackFollowers(app_, withFollowers({{"nangong_wan", from, 1}}));
    EXPECT_FALSE(view.followers()[0].slide.moving());

    // 第二步：帧里把伙伴挪到主角上一格。像 WorldScene::update → render 那样先 advance 再 track：
    // 伙伴与主角同一帧起步、同一帧到——这一帧已经推过的 0.06 秒补给伙伴。
    ASSERT_TRUE(world.tryStep(app_, 1, 0));
    view.advance(app_, 0.06);
    view.trackFollowers(app_, withFollowers({{"nangong_wan", Point{from.x + 1, from.y}, 1}}));
    const auto& slot = view.followers()[0];
    EXPECT_TRUE(slot.slide.moving());
    EXPECT_NEAR(slot.slide.position().x, static_cast<float>(from.x) + 0.5f, 1e-4f) << "滑到一半";
    EXPECT_NEAR(view.playerCell().x - slot.slide.position().x, 1.f, 1e-4f) << "与主角隔一格、同一个进度";
    EXPECT_TRUE(slot.pose.walking) << "滑的时候在播走路帧";
    EXPECT_EQ(slot.facing, 1);
    view.advance(app_, 0.06);
    EXPECT_NEAR(view.followers()[0].slide.position().x, static_cast<float>(from.x + 1), 1e-4f);
    EXPECT_FALSE(view.followers()[0].slide.moving());
    view.advance(app_, 0.2);
    EXPECT_FALSE(view.followers()[0].pose.walking) << "停下之后回站立帧";
}

TEST_F(FollowerSlide, AFollowerMovedMoreThanOneCellSnaps) {
    Point from;
    ASSERT_TRUE(placeInTheOpen(from));
    app_.state().position = from;
    fanren::game::WorldView view;
    view.advance(app_, 0.0);
    view.trackFollowers(app_, withFollowers({{"nangong_wan", from, 2}}));
    view.advance(app_, 0.0);
    view.trackFollowers(app_, withFollowers({{"nangong_wan", Point{from.x + 2, from.y + 1}, 2}}));
    EXPECT_FALSE(view.followers()[0].slide.moving());
    EXPECT_FLOAT_EQ(view.followers()[0].slide.position().x, static_cast<float>(from.x + 2));
    EXPECT_FLOAT_EQ(view.followers()[0].slide.position().y, static_cast<float>(from.y + 1));
}

TEST_F(FollowerSlide, ChangingMapsSnapsTheLineOntoTheHeroEvenFromANeighbouringCell) {
    // 先认一下韩家村的出生点，再回青牛镇，把伙伴摆在「韩家村出生点右边一格」那个坐标上：
    // 换图之后帧里给她出生点那一格——只差一格，但换了图，必须瞬移，不许从旧图滑过来。
    ASSERT_TRUE(app_.loadMap("ch01_hanjiacun", std::string{}).ok);
    const Point spawn = app_.state().position;
    ASSERT_TRUE(app_.loadMap("ch01_qingniuzhen", std::string{}).ok);
    fanren::game::WorldView view;
    view.advance(app_, 0.0);
    view.trackFollowers(app_, withFollowers({{"nangong_wan", Point{spawn.x + 1, spawn.y}, 2}}));
    ASSERT_TRUE(app_.loadMap("ch01_hanjiacun", std::string{}).ok);
    view.advance(app_, 0.016);
    view.trackFollowers(app_, withFollowers({{"nangong_wan", spawn, 2}}));
    EXPECT_FALSE(view.followers()[0].slide.moving()) << "换图：叠在主角格上，走一步才排开";
    EXPECT_FLOAT_EQ(view.followers()[0].slide.position().x, static_cast<float>(spawn.x));
    EXPECT_FALSE(view.followers()[0].pose.walking);
}

TEST_F(FollowerSlide, AScriptJumpOfTheHeroSnapsTheFollowersToo) {
    Point from;
    ASSERT_TRUE(placeInTheOpen(from));
    app_.state().position = from;
    fanren::game::WorldView view;
    view.advance(app_, 0.0);
    view.trackFollowers(app_, withFollowers({{"nangong_wan", from, 2}}));
    // 脚本在同一张图上把人挪走（不相邻）：伙伴的新格虽然只差一格，也跟着整列瞬移。
    app_.state().position = Point{from.x + 5, from.y + 3};
    view.advance(app_, 0.0);
    view.trackFollowers(app_, withFollowers({{"nangong_wan", Point{from.x + 1, from.y}, 2}}));
    EXPECT_FALSE(view.playerMoving());
    EXPECT_FALSE(view.followers()[0].slide.moving());
    EXPECT_FLOAT_EQ(view.followers()[0].slide.position().x, static_cast<float>(from.x + 1));
}

TEST_F(FollowerSlide, AChangedRosterStartsTheLineAfresh) {
    Point from;
    ASSERT_TRUE(placeInTheOpen(from));
    app_.state().position = from;
    fanren::game::WorldView view;
    view.advance(app_, 0.0);
    view.trackFollowers(app_, withFollowers({{"nangong_wan", from, 2}}));
    // 换了一个人（同样只差一格）：不是她滑过去，是名单变了，整列重来。
    view.advance(app_, 0.0);
    view.trackFollowers(app_, withFollowers({{"baiyi_shaonv", Point{from.x + 1, from.y}, 2}}));
    ASSERT_EQ(view.followers().size(), 1u);
    EXPECT_EQ(view.followers()[0].roleId, "baiyi_shaonv");
    EXPECT_FALSE(view.followers()[0].slide.moving());
    // 多了一个人：同样整列重来。
    view.advance(app_, 0.0);
    view.trackFollowers(app_, withFollowers({{"baiyi_shaonv", Point{from.x, from.y}, 2},
                                             {"zhang_tie", Point{from.x + 1, from.y}, 2}}));
    ASSERT_EQ(view.followers().size(), 2u);
    EXPECT_FALSE(view.followers()[0].slide.moving());
    EXPECT_FALSE(view.followers()[1].slide.moving());
    // 没人跟了：列表清空。
    view.trackFollowers(app_, withFollowers({}));
    EXPECT_TRUE(view.followers().empty());
}

// ---------------------------------------------------------------------------
// 缺图不崩：软件渲染器上真画一遍
// ---------------------------------------------------------------------------

struct Shot {
    int w = 0;
    int h = 0;
    std::vector<std::uint32_t> pixels;   // RGB 打包，比较用
    [[nodiscard]] std::uint32_t at(int x, int y) const {
        return pixels[static_cast<std::size_t>(y) * static_cast<std::size_t>(w) + static_cast<std::size_t>(x)];
    }
};

class WorldPropsDrawn : public ::testing::Test {
protected:
    void SetUp() override {
        SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
        SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
        SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "dummy");
        dir_ = fanren::test::uniqueTempPath("fanren_worldprops");
        fs::create_directories(dir_);
        const auto ready = app_.init(repoRoot(), /*headless=*/false);
        ASSERT_TRUE(ready.ok) << ready.error;
        ASSERT_TRUE(app_.loadMap("ch01_qingniuzhen", std::string{}).ok);
        ASSERT_TRUE(pickHeroCell()) << "青牛镇里找不到一处四周几格都没有树冠、屋檐的空地";
        app_.state().position = hero_;
    }

    // 主角站哪：镜头不被地图边夹住，而且下面要摆东西的那几格（连同人物头顶的上一格）上面都没有
    // front 层（树冠、屋檐烘进 above.png 的那一层）——被屋檐盖住的东西比不出差别。
    bool pickHeroCell() {
        const TileMap& map = *app_.currentMap();
        const auto clear = [&map](Point p) {
            if (!map.inBounds(p)) return false;
            const auto i = static_cast<std::size_t>(p.y) * static_cast<std::size_t>(map.width) +
                           static_cast<std::size_t>(p.x);
            return map.front.empty() || i >= map.front.size() || map.front[i] <= 0;
        };
        for (int y = 9; y < map.height - 9; ++y) {
            for (int x = 15; x < map.width - 15; ++x) {
                bool ok = true;
                for (const Point d : {Point{0, 0}, Point{0, -1}, Point{3, 0}, Point{-3, 0}, Point{0, 3}, Point{0, 2},
                                      Point{2, -2}, Point{2, -3}}) {
                    ok = ok && clear(Point{x + d.x, y + d.y});
                }
                if (ok) {
                    hero_ = Point{x, y};
                    return true;
                }
            }
        }
        return false;
    }
    void TearDown() override {
        app_.shutdown();
        SDL_ResetHint(SDL_HINT_VIDEO_DRIVER);
        SDL_ResetHint(SDL_HINT_RENDER_DRIVER);
        SDL_ResetHint(SDL_HINT_AUDIO_DRIVER);
        std::error_code ignored;
        fs::remove_all(dir_, ignored);
    }

    // 一台新的 WorldView 画一帧（开场：镜头就位、不淡入），抓下来。
    Shot draw(const WorldFrame& frame) {
        Shot shot;
        fanren::game::WorldView view;
        view.advance(app_, 0.0);
        app_.engine().beginFrame();
        view.render(app_, frame);
        const fs::path png = dir_ / ("frame" + std::to_string(count_++) + ".png");
        const auto saved = app_.engine().captureFrame(png.string());
        app_.engine().endFrame();
        EXPECT_TRUE(saved.ok) << saved.error;
        if (!saved.ok) return shot;
        SDL_Surface* loaded = SDL_LoadPNG(png.string().c_str());
        if (loaded == nullptr) return shot;
        SDL_Surface* rgba = SDL_ConvertSurface(loaded, SDL_PIXELFORMAT_RGBA32);
        SDL_DestroySurface(loaded);
        if (rgba == nullptr) return shot;
        shot.w = rgba->w;
        shot.h = rgba->h;
        for (int y = 0; y < shot.h; ++y) {
            const auto* row = static_cast<const std::uint8_t*>(rgba->pixels) + y * rgba->pitch;
            for (int x = 0; x < shot.w; ++x) {
                shot.pixels.push_back((static_cast<std::uint32_t>(row[x * 4]) << 16) |
                                      (static_cast<std::uint32_t>(row[x * 4 + 1]) << 8) | row[x * 4 + 2]);
            }
        }
        SDL_DestroySurface(rgba);
        return shot;
    }

    // 主角所在格在屏幕上的左上角（与 WorldView 开场时镜头就位的算法同一个契约：cameraTarget）。
    [[nodiscard]] fanren::engine::Point cellOnScreen(Point cell) const {
        const TileMap& map = *app_.currentMap();
        const Vec2f cam = fanren::game::cameraTarget(map.width, map.height,
                                                     Vec2f{static_cast<float>(hero_.x) + 0.5f,
                                                           static_cast<float>(hero_.y) + 0.5f});
        return {static_cast<int>(std::round(static_cast<float>(cell.x) * kCell - std::round(cam.x))),
                static_cast<int>(std::round(static_cast<float>(cell.y) * kCell - std::round(cam.y)))};
    }

    // 两帧在一块矩形里有几个像素不同。
    static int differing(const Shot& a, const Shot& b, int x0, int y0, int w, int h) {
        int n = 0;
        for (int y = std::max(0, y0); y < std::min(a.h, y0 + h); ++y) {
            for (int x = std::max(0, x0); x < std::min(a.w, x0 + w); ++x) n += a.at(x, y) != b.at(x, y) ? 1 : 0;
        }
        return n;
    }

    fanren::game::Application app_;
    Point hero_{};
    fs::path dir_;
    int count_ = 0;
};

TEST_F(WorldPropsDrawn, TheSameFrameDrawnTwiceIsIdentical) {
    // 下面几条比的都是「多了一样东西」的差别：先钉住同一帧画两遍逐像素相同，差别才归得到那样东西头上。
    const WorldFrame empty;
    const Shot a = draw(empty);
    const Shot b = draw(empty);
    ASSERT_GT(a.w, 0);
    EXPECT_EQ(differing(a, b, 0, 0, a.w, a.h), 0);
}

TEST_F(WorldPropsDrawn, NewObjectsAndAFollowerAreVisibleEvenWithoutTheirArt) {
    const Point chestCell{hero_.x + 3, hero_.y};
    const Point gateCell{hero_.x - 3, hero_.y};
    const Point foeCell{hero_.x, hero_.y + 3};
    const Point followerCell{hero_.x + 2, hero_.y - 2};
    const MapObject chest = objectAt("chest", chestCell.x, chestCell.y);
    const MapObject gate = objectAt("gate", gateCell.x, gateCell.y);
    const MapObject foe = objectAt("foe", foeCell.x, foeCell.y);

    const WorldFrame empty;
    const Shot base = draw(empty);
    ASSERT_GT(base.w, 0);

    WorldFrame withChest;
    withChest.chests.push_back(WorldFrame::Chest{&chest, false});
    const auto c = cellOnScreen(chestCell);
    EXPECT_GT(differing(base, draw(withChest), c.x, c.y, 48, 48), 48 * 48 / 3) << "宝箱看得见";

    WorldFrame shut;
    shut.gates.push_back(WorldFrame::Gate{&gate, false});
    WorldFrame open;
    open.gates.push_back(WorldFrame::Gate{&gate, true});
    const auto g = cellOnScreen(gateCell);
    const int shutDiff = differing(base, draw(shut), g.x, g.y, 48, 48);
    const int openDiff = differing(base, draw(open), g.x, g.y, 48, 48);
    EXPECT_GT(shutDiff, 48 * 48 / 3) << "关着的机关是一整块";
    EXPECT_GT(openDiff, 0) << "开了的机关也看得出这里有过一道关";
    EXPECT_LT(openDiff * 4, shutDiff) << "开了的机关不再画成一整块挡路的样子";

    WorldFrame withFoe;
    withFoe.foes.push_back(WorldFrame::Foe{&foe, "zhang_tie", 2, false});
    const Shot foeShot = draw(withFoe);
    const auto f = cellOnScreen(foeCell);
    EXPECT_GT(differing(base, foeShot, f.x, f.y - 48, 48, 96), 200) << "明雷本体看得见";
    // 头顶标记：本体（×3 的 16×24 人，顶在脚底上 72 像素）再往上那一截。
    EXPECT_GT(differing(base, foeShot, f.x, f.y + 48 - 72 - 30, 48, 28), 50) << "头顶有明雷标记";

    WorldFrame withFollower;
    withFollower.followers.push_back(WorldFrame::Follower{"zhang_tie", followerCell, 2});
    const auto p = cellOnScreen(followerCell);
    EXPECT_GT(differing(base, draw(withFollower), p.x, p.y - 24, 48, 72), 200) << "跟随者画出来了";
}

}  // namespace
