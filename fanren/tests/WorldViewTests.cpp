// game/WorldView：世界画面的纯视觉那一半——镜头、插值滑动、深度排序、树冠渐隐、
// 换图淡入、地名横幅、传送箭头的朝向。
//
// 最要紧的一条契约在 WorldViewHeadless 那一组里：**画面从不延迟逻辑**。tryStep 一调，
// GameState::position 立刻变；画面上的人要 0.12 秒才滑到。无头测试驱动整章剧情靠的是前一半，
// 这一组钉住「后一半没有反过来拖住前一半」。
#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <string>
#include <vector>

#include "core/model/Types.h"
#include "game/Application.h"
#include "game/WorldScene.h"
#include "game/SpriteAtlas.h"
#include "game/WorldView.h"
#include "io/DataLoader.h"
#include "io/VisualLoader.h"

namespace {

namespace fs = std::filesystem;
using fanren::core::MapObject;
using fanren::core::Point;
using fanren::core::TileMap;
using fanren::game::cameraAxis;
using fanren::game::cameraTarget;
using fanren::game::kWorldTilePx;
using fanren::game::Vec2f;

constexpr float kCell = static_cast<float>(kWorldTilePx);

std::string repoRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "maps" / "ch01_hanjiacun.tmj")) return candidate;
    }
    return ".";
}

// ---------------------------------------------------------------------------
// 像素尺度
// ---------------------------------------------------------------------------

TEST(WorldView, OneCellIsFortyEightScreenPixels) {
    // 施工图 1.1：16px 美术 ×3。逻辑格仍是 32，两者不许混用。
    EXPECT_EQ(fanren::game::kArtTilePx, 16);
    EXPECT_EQ(fanren::game::kArtScale, 3);
    EXPECT_EQ(kWorldTilePx, 48);
    EXPECT_EQ(fanren::engine::kTileSize, 32) << "逻辑格不动：地图、碰撞、测试全都按它";
}

// ---------------------------------------------------------------------------
// 镜头
// ---------------------------------------------------------------------------

TEST(WorldView, AMapNarrowerThanTheScreenIsCentred) {
    // 24 格宽 = 1152 像素，比 1280 窄 128：两边各留 64。
    EXPECT_FLOAT_EQ(cameraAxis(300.f, 24.f * kCell, 1280.f), -64.f);
    EXPECT_FLOAT_EQ(cameraAxis(900.f, 24.f * kCell, 1280.f), -64.f) << "居中与主角站在哪无关";
    // 恰好一样大：不动。
    EXPECT_FLOAT_EQ(cameraAxis(640.f, 1280.f, 1280.f), 0.f);
}

TEST(WorldView, ALargerMapFollowsTheHeroAndStaysInsideItsEdges) {
    const float map = 40.f * kCell;   // 1920
    EXPECT_FLOAT_EQ(cameraAxis(100.f, map, 1280.f), 0.f) << "左边缘：不越过地图";
    EXPECT_FLOAT_EQ(cameraAxis(1900.f, map, 1280.f), 640.f) << "右边缘：地图 − 视口";
    EXPECT_FLOAT_EQ(cameraAxis(1000.f, map, 1280.f), 360.f) << "中间：主角在视口正中";
}

TEST(WorldView, CameraTargetWorksInFortyEightPixelCells) {
    // 40×30 的图：1920×1440。
    const Vec2f corner = cameraTarget(40, 30, Vec2f{0.5f, 0.5f});
    EXPECT_FLOAT_EQ(corner.x, 0.f);
    EXPECT_FLOAT_EQ(corner.y, 0.f);
    const Vec2f far = cameraTarget(40, 30, Vec2f{39.5f, 29.5f});
    EXPECT_FLOAT_EQ(far.x, 640.f);
    EXPECT_FLOAT_EQ(far.y, 720.f);
    const Vec2f mid = cameraTarget(40, 30, Vec2f{20.f, 15.f});
    EXPECT_FLOAT_EQ(mid.x, 20.f * kCell - 640.f);
    EXPECT_FLOAT_EQ(mid.y, 15.f * kCell - 360.f);
    // 24×18（居所、密室）：横向居中、纵向跟随。
    const Vec2f small = cameraTarget(24, 18, Vec2f{12.f, 9.f});
    EXPECT_FLOAT_EQ(small.x, -64.f);
    EXPECT_FLOAT_EQ(small.y, 9.f * kCell - 360.f);
}

TEST(WorldView, ApproachEasesWithoutOvershootAndIndependentOfFrameRate) {
    using fanren::game::approach;
    float x = 0.f;
    float previous = 0.f;
    for (int i = 0; i < 120; ++i) {
        x = approach(x, 100.f, 1.f / 60.f, 10.f);
        // 离目标还远时每一帧都得往前走；贴到目标之后浮点已经分辨不出下一步，只要求不往回退。
        if (100.f - previous > 0.01f) {
            EXPECT_GT(x, previous) << "一直在往目标走，第 " << i << " 帧";
        } else {
            EXPECT_GE(x, previous) << "不往回退，第 " << i << " 帧";
        }
        EXPECT_LE(x, 100.f) << "不越过目标";
        previous = x;
    }
    EXPECT_NEAR(x, 100.f, 0.01f) << "两秒后已经对正";
    // 60 帧与 30 帧跑同样的秒数，停在同一处（不然帧率一掉镜头就追不上）。
    float at60 = 0.f;
    float at30 = 0.f;
    for (int i = 0; i < 6; ++i) at60 = approach(at60, 100.f, 1.f / 60.f, 10.f);
    for (int i = 0; i < 3; ++i) at30 = approach(at30, 100.f, 1.f / 30.f, 10.f);
    EXPECT_NEAR(at60, at30, 1e-3f);
    EXPECT_FLOAT_EQ(approach(42.f, 100.f, 0.f, 10.f), 42.f) << "dt 为 0 不动";
}

// ---------------------------------------------------------------------------
// 插值滑动
// ---------------------------------------------------------------------------

TEST(WorldView, AStepSlidesLinearlyOverTwelveHundredthsOfASecond) {
    fanren::game::StepSlide slide;
    slide.snap(Vec2f{3.f, 4.f});
    EXPECT_FALSE(slide.moving());
    slide.moveTo(Vec2f{4.f, 4.f});
    EXPECT_TRUE(slide.moving());
    EXPECT_FLOAT_EQ(slide.position().x, 3.f) << "刚起步：画面上还在原格";
    slide.advance(0.06f);
    EXPECT_NEAR(slide.position().x, 3.5f, 1e-5f) << "匀速：一半时间走一半路";
    slide.advance(0.03f);
    EXPECT_NEAR(slide.position().x, 3.75f, 1e-5f);
    slide.advance(0.1f);
    EXPECT_FLOAT_EQ(slide.position().x, 4.f);
    EXPECT_FALSE(slide.moving());
    EXPECT_FLOAT_EQ(slide.position().y, 4.f);
}

TEST(WorldView, ANewStepMidSlideStartsFromWhereThePictureIs) {
    fanren::game::StepSlide slide;
    slide.snap(Vec2f{3.f, 4.f});
    slide.moveTo(Vec2f{4.f, 4.f});
    slide.advance(0.06f);                      // 画面在 3.5
    slide.moveTo(Vec2f{4.f, 5.f});             // 又迈了一步（向下）
    EXPECT_NEAR(slide.position().x, 3.5f, 1e-5f) << "不跳回 4，也不跳回 3";
    EXPECT_NEAR(slide.position().y, 4.f, 1e-5f);
    slide.advance(1.f);
    EXPECT_FLOAT_EQ(slide.position().x, 4.f);
    EXPECT_FLOAT_EQ(slide.position().y, 5.f);
    slide.snap(Vec2f{10.f, 10.f});
    EXPECT_FALSE(slide.moving()) << "瞬移不滑";
    EXPECT_FLOAT_EQ(slide.position().x, 10.f);
}

// ---------------------------------------------------------------------------
// 深度排序
// ---------------------------------------------------------------------------

TEST(WorldView, WhoeverStandsLowerOnScreenIsDrawnLater) {
    using fanren::game::DepthKey;
    using fanren::game::drawsBefore;
    const DepthKey behind{5.f * kCell, 10.f * kCell, 7};
    const DepthKey front{6.f * kCell, 2.f * kCell, 0};
    EXPECT_TRUE(drawsBefore(behind, front)) << "脚底靠上的先画（被挡在后面）";
    EXPECT_FALSE(drawsBefore(front, behind));
    // 同一行：从左到右。
    const DepthKey left{6.f * kCell, 2.f * kCell, 9};
    const DepthKey right{6.f * kCell, 3.f * kCell, 0};
    EXPECT_TRUE(drawsBefore(left, right));
    // 完全重叠：按登记次序，保证同一帧画两遍一模一样。
    const DepthKey first{6.f * kCell, 2.f * kCell, 1};
    const DepthKey second{6.f * kCell, 2.f * kCell, 2};
    EXPECT_TRUE(drawsBefore(first, second));
    EXPECT_FALSE(drawsBefore(second, first));
    EXPECT_FALSE(drawsBefore(first, first)) << "严格弱序：不比自己靠前";

    // 主角从上一行往下滑的半路上：脚底在两行之间，仍排在下面那一行的人前面画（被他挡着），
    // 滑到了才轮到自己挡别人。
    const DepthKey npcBelow{6.f * kCell, 4.f * kCell, 0};
    const DepthKey heroHalfway{5.5f * kCell, 4.f * kCell, 99};
    EXPECT_TRUE(drawsBefore(heroHalfway, npcBelow));
}

// ---------------------------------------------------------------------------
// 树冠 / 屋檐渐隐
// ---------------------------------------------------------------------------

TEST(WorldView, CanopyThinsOutAroundTheHeroOnlyWithinAboutTwoCells) {
    using fanren::game::canopyAlpha;
    EXPECT_FLOAT_EQ(canopyAlpha(0.f), 0.3f) << "正头顶：透得出人";
    EXPECT_FLOAT_EQ(canopyAlpha(1.f), 0.3f);
    EXPECT_GT(canopyAlpha(2.f), 0.3f);
    EXPECT_LT(canopyAlpha(2.f), 1.f) << "约 2 格处在过渡带里";
    EXPECT_FLOAT_EQ(canopyAlpha(2.75f), 1.f);
    EXPECT_FLOAT_EQ(canopyAlpha(3.f), 1.f) << "逐顶点算透明度的窗口边上（±3 格）恒为不透明，与窗口外接得上";
    EXPECT_FLOAT_EQ(canopyAlpha(10.f), 1.f);
    float previous = 0.f;
    for (float d = 0.f; d < 4.f; d += 0.05f) {
        const float a = canopyAlpha(d);
        EXPECT_GE(a, previous) << "离得越远越不透明，d=" << d;
        previous = a;
    }
}

// ---------------------------------------------------------------------------
// 换图淡入与地名横幅
// ---------------------------------------------------------------------------

TEST(WorldView, MapFadeGoesFromBlackToClearInPointThreeSeconds) {
    using fanren::game::mapFadeAlpha;
    EXPECT_FLOAT_EQ(mapFadeAlpha(0.f), 1.f);
    EXPECT_NEAR(mapFadeAlpha(0.15f), 0.5f, 1e-5f);
    EXPECT_FLOAT_EQ(mapFadeAlpha(0.3f), 0.f);
    EXPECT_FLOAT_EQ(mapFadeAlpha(5.f), 0.f);
}

TEST(WorldView, BannerFadesInAfterAMapChangeAndIsThereAtOnceOnFirstEntry) {
    using fanren::game::bannerAlpha;
    // 换图进来：黑幕还厚的时候不出来，然后淡入、停住、淡出。
    EXPECT_FLOAT_EQ(bannerAlpha(0.f, true), 0.f);
    EXPECT_FLOAT_EQ(bannerAlpha(0.1f, true), 0.f);
    const float rising = bannerAlpha(0.4f, true);
    EXPECT_GT(rising, 0.f);
    EXPECT_LT(rising, 1.f);
    EXPECT_FLOAT_EQ(bannerAlpha(2.f, true), 1.f);
    EXPECT_FLOAT_EQ(bannerAlpha(30.f, true), 0.f) << "几秒后淡出，不常驻";
    // 开场：一上来就在（外层的开场转场自己在淡入）。
    EXPECT_FLOAT_EQ(bannerAlpha(0.f, false), 1.f);
    EXPECT_FLOAT_EQ(bannerAlpha(2.f, false), 1.f);
    EXPECT_FLOAT_EQ(bannerAlpha(30.f, false), 0.f);
    // 淡出是平滑的：中途有一段半透明。
    bool sawHalf = false;
    for (float t = 3.f; t < 6.f; t += 0.05f) {
        const float a = bannerAlpha(t, false);
        if (a > 0.1f && a < 0.9f) sawHalf = true;
    }
    EXPECT_TRUE(sawHalf);
}

// ---------------------------------------------------------------------------
// 传送箭头朝哪
// ---------------------------------------------------------------------------

// 用字符画拼一张图：'#' 挡路，其余可走；'P' 是门格（可走）。
TileMap sketch(const std::vector<std::string>& rows) {
    TileMap map;
    map.id = "sketch";
    map.height = static_cast<int>(rows.size());
    map.width = static_cast<int>(rows.front().size());
    MapObject portal;
    portal.type = "portal";
    portal.name = "portal_sketch";
    int minX = map.width, minY = map.height, maxX = -1, maxY = -1;
    for (int y = 0; y < map.height; ++y) {
        for (int x = 0; x < map.width; ++x) {
            const char c = rows[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)];
            map.collision.push_back(c == '#' ? 1 : 0);
            if (c == 'P') {
                minX = std::min(minX, x);
                minY = std::min(minY, y);
                maxX = std::max(maxX, x);
                maxY = std::max(maxY, y);
            }
        }
    }
    portal.position = Point{minX, minY};
    portal.width = maxX - minX + 1;
    portal.height = maxY - minY + 1;
    map.objects.push_back(portal);
    return map;
}

TEST(WorldView, PortalArrowsPointTheWayYouStepIntoThem) {
    using fanren::game::portalArrowFacing;
    using fanren::game::portalIsDoorway;
    // 墙上的门：从下面走上去。
    const TileMap door = sketch({"#####", "##P##", "#...#", "#...#"});
    EXPECT_EQ(portalArrowFacing(door, door.objects.front()), 0);
    EXPECT_TRUE(portalIsDoorway(door, door.objects.front())) << "朝上、上面是墙不是地图边：门洞";
    // 下边缘的路口：往下走出去。
    const TileMap south = sketch({"#...#", "#...#", "##PP#"});
    EXPECT_EQ(portalArrowFacing(south, south.objects.front()), 2);
    EXPECT_FALSE(portalIsDoorway(south, south.objects.front()));
    // 右墙上的侧门（密室那种 1×2）：往右迈进去。
    const TileMap east = sketch({"#####", "#..P#", "#..P#", "#####"});
    EXPECT_EQ(portalArrowFacing(east, east.objects.front()), 1);
    // 左墙上的。
    const TileMap west = sketch({"#####", "#P..#", "#P..#", "#####"});
    EXPECT_EQ(portalArrowFacing(west, west.objects.front()), 3);
    // 上边缘的路口：朝上，但不是门洞（上面是地图外，不是墙）。
    const TileMap north = sketch({"#PP#", "#..#", "#..#"});
    EXPECT_EQ(portalArrowFacing(north, north.objects.front()), 0);
    EXPECT_FALSE(portalIsDoorway(north, north.objects.front()));
    // 摆在空地中间、四面都走得通：没有「来路」可言，朝下。
    const TileMap open = sketch({".....", "..P..", "....."});
    EXPECT_EQ(portalArrowFacing(open, open.objects.front()), 2);
}

TEST(WorldView, EveryShippedPortalOnAMapEdgePointsOutThroughThatEdge) {
    // 规则，不是普查：贴着地图边、那一侧又是出口的门，箭头一律指向地图外。
    int checked = 0;
    for (const fs::directory_entry& entry : fs::directory_iterator(fs::path(repoRoot()) / "maps")) {
        if (entry.path().extension() != ".tmj") continue;
        auto loaded = fanren::io::loadTileMap(entry.path().string());
        ASSERT_TRUE(loaded.ok) << loaded.error;
        const TileMap& map = loaded.value;
        for (const MapObject& object : map.objects) {
            if (object.type != "portal") continue;
            const int w = std::max(1, object.width);
            const int h = std::max(1, object.height);
            int edge = -1;
            if (object.position.y == 0) edge = 0;
            else if (object.position.x + w == map.width) edge = 1;
            else if (object.position.y + h == map.height) edge = 2;
            else if (object.position.x == 0) edge = 3;
            if (edge < 0) continue;
            ++checked;
            EXPECT_EQ(fanren::game::portalArrowFacing(map, object), edge)
                << entry.path().filename().string() << " 的 " << object.name;
        }
    }
    EXPECT_GT(checked, 0) << "一道贴边的门都没查到，这条用例什么也没验";
}

TEST(WorldView, EachFacilityKindHasExactlyOneOwnerForItsStandingObject) {
    using fanren::game::facilitySpriteAtRuntime;
    // 烘焙图在用：告示板、蒲团整件在烘焙里，运行时再叠一份精灵就是两块板、两个蒲团。
    EXPECT_FALSE(facilitySpriteAtRuntime("board", true));
    EXPECT_FALSE(facilitySpriteAtRuntime("meditate", true));
    // 其余几种烘焙里只有地面那一截（地火口、存档石、幌子），立着的物件与它的帧动画、发光帧归精灵。
    for (const char* kind : {"alchemy", "forge", "talisman", "formation", "field", "shop", "save"}) {
        EXPECT_TRUE(facilitySpriteAtRuntime(kind, true)) << kind;
    }
    // 烘焙图缺了（退回旧图元）：九种一律运行时画，不然告示板与蒲团就从画面上消失了。
    for (const char* kind : {"board", "meditate", "alchemy", "save", "shop"}) {
        EXPECT_TRUE(facilitySpriteAtRuntime(kind, false)) << kind;
    }
}

// ---------------------------------------------------------------------------
// 无头：画面从不延迟逻辑
// ---------------------------------------------------------------------------

// 这一格走得上去、而且上面什么对象都没有（踩上去不会起剧情、不会换图）。
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

class WorldViewHeadless : public ::testing::Test {
protected:
    void SetUp() override {
        auto ready = app_.init(repoRoot(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
        // 青牛镇 48×36：镜头在中间一带不被地图边夹住，缓动看得出来。
        auto loaded = app_.loadMap("ch01_qingniuzhen", std::string{});
        ASSERT_TRUE(loaded.ok) << loaded.error;
    }
    void TearDown() override { app_.shutdown(); }

    // 找一格「站得住、往右一格也站得住、镜头在这两格之间会动」的起点。
    bool placeInTheOpen(Point& from) {
        const TileMap& map = *app_.currentMap();
        for (int y = 12; y < map.height - 12; ++y) {
            for (int x = 16; x < map.width - 16; ++x) {
                const Point here{x, y};
                const Point right{x + 1, y};
                if (plainCell(map, here) && plainCell(map, right)) {
                    from = here;
                    return true;
                }
            }
        }
        return false;
    }

    fanren::game::Application app_;
};

TEST_F(WorldViewHeadless, FirstEntryIsInPlaceWithoutAFadeAndWithTheBanner) {
    fanren::game::WorldView view;
    view.advance(app_, 0.0);
    EXPECT_FLOAT_EQ(view.fadeAlpha(), 0.f) << "开场不淡入（外层的开场转场自己淡）";
    EXPECT_FLOAT_EQ(view.bannerOpacity(), 1.f);
    const Point at = app_.state().position;
    EXPECT_FLOAT_EQ(view.playerCell().x, static_cast<float>(at.x));
    const Vec2f target = cameraTarget(app_.currentMap()->width, app_.currentMap()->height,
                                      Vec2f{static_cast<float>(at.x) + 0.5f, static_cast<float>(at.y) + 0.5f});
    EXPECT_FLOAT_EQ(view.camera().x, target.x) << "镜头直接就位，不从左上角一路追过来";
    EXPECT_FLOAT_EQ(view.camera().y, target.y);
}

TEST_F(WorldViewHeadless, TheLogicMovesAtOnceWhileThePictureSlides) {
    Point from;
    ASSERT_TRUE(placeInTheOpen(from)) << "青牛镇中间一带找不到两格连着的空地";
    app_.state().position = from;
    fanren::game::WorldView view;
    fanren::game::WorldScene world;
    view.advance(app_, 0.0);

    ASSERT_TRUE(world.tryStep(app_, 1, 0));
    // 契约：tryStep 一返回，逻辑位置已经是新的一格——不等画面。
    EXPECT_EQ(app_.state().position, (Point{from.x + 1, from.y}));

    view.advance(app_, 0.0);
    EXPECT_FLOAT_EQ(view.playerCell().x, static_cast<float>(from.x)) << "画面刚起步";
    EXPECT_TRUE(view.playerMoving());
    view.advance(app_, 0.06);
    EXPECT_NEAR(view.playerCell().x, static_cast<float>(from.x) + 0.5f, 1e-4f);
    EXPECT_TRUE(view.walking()) << "滑的时候在播走路帧";
    view.advance(app_, 0.06);
    EXPECT_NEAR(view.playerCell().x, static_cast<float>(from.x + 1), 1e-4f);
    EXPECT_FALSE(view.playerMoving());
    view.advance(app_, 0.2);
    EXPECT_FALSE(view.walking()) << "停下之后回站立帧";
    // 画面走完了，逻辑一格都没多走、没少走。
    EXPECT_EQ(app_.state().position, (Point{from.x + 1, from.y}));
}

TEST_F(WorldViewHeadless, TheCameraEasesAfterTheHeroInsteadOfJumping) {
    Point from;
    ASSERT_TRUE(placeInTheOpen(from));
    app_.state().position = from;
    fanren::game::WorldView view;
    fanren::game::WorldScene world;
    view.advance(app_, 0.0);
    const float before = view.camera().x;
    ASSERT_TRUE(world.tryStep(app_, 1, 0));
    const float after =
        cameraTarget(app_.currentMap()->width, app_.currentMap()->height,
                     Vec2f{static_cast<float>(from.x + 1) + 0.5f, static_cast<float>(from.y) + 0.5f})
            .x;
    ASSERT_GT(after, before) << "这一步本该让镜头往右挪一格（选的起点不对，用例什么也没验）";
    view.advance(app_, 0.05);
    EXPECT_GT(view.camera().x, before) << "镜头在追";
    EXPECT_LT(view.camera().x, after) << "但不是一下子跳过去";
    for (int i = 0; i < 120; ++i) view.advance(app_, 1.0 / 60.0);
    EXPECT_NEAR(view.camera().x, after, 0.5f) << "停下来之后对正";
}

TEST_F(WorldViewHeadless, ChangingMapsIsInstantForTheLogicAndFadesInForThePicture) {
    fanren::game::WorldView view;
    view.advance(app_, 0.0);
    view.advance(app_, 5.0);   // 开场横幅早已淡出
    auto loaded = app_.loadMap("ch01_hanjiacun", std::string{});
    ASSERT_TRUE(loaded.ok) << loaded.error;
    EXPECT_EQ(app_.state().mapId, "ch01_hanjiacun") << "逻辑上换图是瞬时的";

    view.advance(app_, 0.0);
    EXPECT_FLOAT_EQ(view.fadeAlpha(), 1.f) << "从黑里淡进来";
    EXPECT_FLOAT_EQ(view.bannerOpacity(), 0.f) << "黑幕还厚的时候横幅不出来";
    EXPECT_FALSE(view.playerMoving()) << "换图不滑：人直接站在出生点";
    const Point at = app_.state().position;
    EXPECT_FLOAT_EQ(view.playerCell().x, static_cast<float>(at.x));
    const Vec2f target = cameraTarget(app_.currentMap()->width, app_.currentMap()->height,
                                      Vec2f{static_cast<float>(at.x) + 0.5f, static_cast<float>(at.y) + 0.5f});
    EXPECT_FLOAT_EQ(view.camera().x, target.x) << "镜头跟着换图直接就位";
    view.advance(app_, 0.15);
    EXPECT_NEAR(view.fadeAlpha(), 0.5f, 1e-4f);
    view.advance(app_, 0.2);
    EXPECT_FLOAT_EQ(view.fadeAlpha(), 0.f);
    view.advance(app_, 1.0);
    EXPECT_FLOAT_EQ(view.bannerOpacity(), 1.f) << "淡入之后地名横幅出来";
}

TEST_F(WorldViewHeadless, AJumpWithinTheSameMapSnapsInsteadOfSliding) {
    Point from;
    ASSERT_TRUE(placeInTheOpen(from));
    app_.state().position = from;
    fanren::game::WorldView view;
    view.advance(app_, 0.0);
    // 脚本在同一张图上把人挪走（不相邻）：人与镜头一起跳过去，不滑、不淡。
    app_.state().position = Point{from.x + 5, from.y + 3};
    view.advance(app_, 0.0);
    EXPECT_FALSE(view.playerMoving());
    EXPECT_FLOAT_EQ(view.playerCell().x, static_cast<float>(from.x + 5));
    EXPECT_FLOAT_EQ(view.playerCell().y, static_cast<float>(from.y + 3));
    EXPECT_FLOAT_EQ(view.fadeAlpha(), 0.f);
}

TEST_F(WorldViewHeadless, HanliGrowsUpWhenChapterTwoIsDoneThroughTheRealChapterTable) {
    // 世界画面选外观走的那一条线，整条接起来量：真的章节表（data/chapters.json）→ 真的存档旗标
    // → 真的 index.json。判据写死成派工原文：第 1–2 章少年版，ch02.done 置位后成年。
    const fs::path indexPath = fs::path(repoRoot()) / "assets" / "art" / "sprites" / "index.json";
    if (!fs::exists(indexPath)) GTEST_SKIP() << "仓库里还没有 " << indexPath.string();
    auto index = fanren::io::loadSpriteIndex(indexPath.string());
    ASSERT_TRUE(index.ok) << index.error;
    const auto& chapters = app_.chapterTable();
    auto& state = app_.state();
    const fanren::game::ChapterDone done = [&](int n) { return chapters.chapterDone(state, n); };
    EXPECT_EQ(fanren::game::lookForRole(index.value, "hanli", done), "hanli_child");
    state.setFlag("ch01.done", 1);
    EXPECT_EQ(fanren::game::lookForRole(index.value, "hanli", done), "hanli_child");
    state.setFlag("ch02.done", 1);
    EXPECT_EQ(fanren::game::lookForRole(index.value, "hanli", done), "hanli");
    EXPECT_EQ(fanren::game::lookForRole(index.value, "zhang_tie", done), "zhang_tie");
}

TEST_F(WorldViewHeadless, ResetMakesTheNextMapAFirstEntryAgain) {
    fanren::game::WorldView view;
    view.advance(app_, 0.0);
    ASSERT_TRUE(app_.loadMap("ch01_hanjiacun", std::string{}).ok);
    view.advance(app_, 0.0);
    ASSERT_FLOAT_EQ(view.fadeAlpha(), 1.f);
    view.reset();   // 场景重新进栈
    view.advance(app_, 0.0);
    EXPECT_FLOAT_EQ(view.fadeAlpha(), 0.f);
    EXPECT_FLOAT_EQ(view.bannerOpacity(), 1.f);
}

}  // namespace
