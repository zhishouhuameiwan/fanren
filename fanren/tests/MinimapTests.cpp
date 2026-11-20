// 小地图与区域地图（第 7 章可玩性改版·第一期「地基包」路 M，docs/ch07-foundation.md 3.1、3.2、3.5 第 1、2 条）。
//
// 钉四件事：
//   1. 换算：格 → 小地图上的点、取景（整张放得下就整张，放不下跟着主角开窗、夹住图边）、贴边、平移；
//   2. 标记的显示规矩：没开的宝箱、没采的采集点、明雷只在 8 格以内画；开过的、采过的一直画成灰的；
//      目标在本图就是它，在别的图上就是那道通往它的门；
//   3. 第 7 章 12 张主线图、6 张历练图：都有高清烘焙图（尺寸对得上格数），图上每个门、设施、宝箱、采集点、明雷
//      在小地图与区域地图两种取景下都落在它自己那几格上；第 3、6 章没有烘焙图的图走剪影；
//   4. 走玩家碰得到的入口：世界层按 M 打开区域地图、再按 M 关上；主菜单有「地图」一项；对白压着时小地图不画；
//      设置里关掉小地图就不画。
#include <SDL3/SDL.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "TempDir.h"
#include "core/model/Types.h"
#include "game/Application.h"
#include "game/AreaMapScene.h"
#include "game/MenuScene.h"
#include "game/Minimap.h"
#include "game/WorldScene.h"
#include "io/DataLoader.h"

namespace {

namespace fs = std::filesystem;
namespace mm = fanren::game::minimap;
using fanren::core::MapObject;
using fanren::core::Point;
using fanren::engine::RectF;
using fanren::game::Vec2f;
using fanren::game::WorldFrame;

fs::path repoRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "data" / "text" / "ch01_main.json")) return candidate;
    }
    return ".";
}

// ===========================================================================
// 1. 换算
// ===========================================================================

TEST(MinimapLayout, ASmallMapFitsWholeAndCentred) {
    const RectF area{100.f, 50.f, 190.f, 140.f};
    const mm::Layout l = mm::fit(area, 20, 14, Vec2f{3.f, 3.f});
    EXPECT_TRUE(l.whole);
    EXPECT_FLOAT_EQ(l.cellPx, 9.5f) << "190 / 20 与 140 / 14 取小的那个";
    EXPECT_FLOAT_EQ(l.offsetX, 0.f);
    EXPECT_FLOAT_EQ(l.offsetY, (140.f - 14.f * 9.5f) / 2.f) << "竖向放不满就居中";
    const Vec2f origin = mm::toScreen(l, Vec2f{0.f, 0.f});
    EXPECT_FLOAT_EQ(origin.x, 100.f);
    const Vec2f corner = mm::toScreen(l, Vec2f{20.f, 14.f});
    EXPECT_FLOAT_EQ(corner.x, 290.f);
    EXPECT_LE(corner.y, 190.f);
    EXPECT_EQ(mm::toCell(l, mm::toScreen(l, Vec2f{7.5f, 4.5f})), (Point{7, 4})) << "格中心换回去落在那一格";
    const mm::Layout big = mm::fit(area, 4, 3, Vec2f{}, 3.f, 32.f);
    EXPECT_FLOAT_EQ(big.cellPx, 32.f) << "小屋子不放成一格一块砖：至多 maxCellPx";
}

TEST(MinimapLayout, ABigMapOpensAWindowOnTheHeroAndStopsAtTheEdges) {
    const RectF area{0.f, 0.f, 190.f, 140.f};
    // 80 × 56：整张缩进来一格只有 2.375 像素，小于 3：开窗。
    const mm::Layout mid = mm::fit(area, 80, 56, Vec2f{40.f, 28.f});
    EXPECT_FALSE(mid.whole);
    EXPECT_FLOAT_EQ(mid.cellPx, mm::kMinCellPx);
    const Vec2f hero = mm::toScreen(mid, Vec2f{40.f, 28.f});
    EXPECT_FLOAT_EQ(hero.x, 95.f) << "主角在窗正中";
    EXPECT_FLOAT_EQ(hero.y, 70.f);

    const mm::Layout corner = mm::fit(area, 80, 56, Vec2f{1.f, 1.f});
    EXPECT_FLOAT_EQ(corner.offsetX, 0.f) << "贴左上角时窗夹住，不露出图外";
    EXPECT_FLOAT_EQ(corner.offsetY, 0.f);
    const mm::Layout far = mm::fit(area, 80, 56, Vec2f{79.f, 55.f});
    EXPECT_FLOAT_EQ(far.offsetX, 190.f - 80.f * 3.f);
    EXPECT_FLOAT_EQ(far.offsetY, 140.f - 56.f * 3.f);
    const mm::CellRange cells = mm::visibleCells(far);
    EXPECT_FLOAT_EQ(cells.x1, 80.f);
    EXPECT_FLOAT_EQ(cells.y1, 56.f);
    EXPECT_NEAR(cells.x1 - cells.x0, 190.f / 3.f, 1e-3f);

    // 贴边：窗外的目标夹到边上、离边 inset。
    const Vec2f outside = mm::toScreen(corner, Vec2f{70.f, 2.f});
    EXPECT_FALSE(mm::contains(corner, outside));
    const Vec2f clamped = mm::clampInside(corner, outside, 6.f);
    EXPECT_FLOAT_EQ(clamped.x, 190.f - 6.f);
    EXPECT_FLOAT_EQ(clamped.y, outside.y);

    // 平移：往右挪三格，到头就停。
    const mm::Layout moved = mm::panned(corner, 3.f, 0.f);
    EXPECT_FLOAT_EQ(moved.offsetX, -9.f);
    EXPECT_FLOAT_EQ(mm::panned(corner, -3.f, 0.f).offsetX, 0.f) << "左边已经到头";
    EXPECT_FLOAT_EQ(mm::panned(far, 50.f, 0.f).offsetX, far.offsetX) << "右边已经到头";
    const mm::Layout whole = mm::fit(area, 20, 14, Vec2f{});
    EXPECT_FLOAT_EQ(mm::panned(whole, 5.f, 5.f).offsetX, whole.offsetX) << "整张放得下时不平移";
}

TEST(MinimapLayout, TheMinimapSitsTopRightAndTheSaveNoticeGoesBelowIt) {
    const RectF panel = mm::panelRect();
    EXPECT_FLOAT_EQ(panel.x + panel.w, 1280.f - mm::kRight);
    EXPECT_FLOAT_EQ(panel.y, mm::kTop);
    EXPECT_NEAR(panel.w, 200.f, 0.5f);
    EXPECT_NEAR(panel.h, 150.f, 0.5f);
    EXPECT_GT(mm::kSaveNoticeTop, panel.y + panel.h);
}

// ===========================================================================
// 2. 标记的显示规矩
// ===========================================================================

MapObject object(const char* type, int x, int y, const char* id = "") {
    MapObject o;
    o.type = type;
    o.position = Point{x, y};
    if (*id != '\0') o.properties["id"] = id;
    return o;
}

int countKind(const std::vector<mm::Mark>& marks, mm::MarkKind kind) {
    int n = 0;
    for (const mm::Mark& m : marks) n += m.kind == kind ? 1 : 0;
    return n;
}

TEST(MinimapMarks, NearOnlyForUnopenedThingsAndFoesAlwaysForOpenedOnes) {
    const MapObject nearChest = object("chest", 12, 10, "c_near");
    const MapObject farChest = object("chest", 30, 10, "c_far");
    const MapObject farOpened = object("chest", 30, 20, "c_open");
    const MapObject nearGather = object("gather", 10, 4, "g_near");
    const MapObject farGathered = object("gather", 40, 40, "g_done");
    const MapObject nearFoe = object("foe", 17, 10, "f_near");
    const MapObject farFoe = object("foe", 19, 10, "f_far");
    WorldFrame frame;
    frame.chests = {{&nearChest, false}, {&farChest, false}, {&farOpened, true}};
    frame.gathers = {{&nearGather, false}, {&farGathered, false}};
    WorldFrame::Foe f1;
    f1.object = &nearFoe;
    WorldFrame::Foe f2;
    f2.object = &farFoe;
    frame.foes = {f1, f2};
    fanren::core::WorldObjectState objects;
    objects.gatheredDay["g_done"] = 3;

    const auto marks = mm::collectMarks(frame, objects, Vec2f{10.f, 10.f}, 1);
    EXPECT_EQ(countKind(marks, mm::MarkKind::Chest), 1) << "8 格以内那只没开的画，30 格外的不画";
    EXPECT_EQ(countKind(marks, mm::MarkKind::ChestOpened), 1) << "开过的远也画（灰）";
    EXPECT_EQ(countKind(marks, mm::MarkKind::Gather), 1);
    EXPECT_EQ(countKind(marks, mm::MarkKind::GatherDone), 1) << "采过的远也画（灰）";
    EXPECT_EQ(countKind(marks, mm::MarkKind::Foe), 1) << "明雷只画 8 格以内的：17 在、19 不在";
    ASSERT_FALSE(marks.empty());
    EXPECT_EQ(marks.back().kind, mm::MarkKind::Hero) << "主角最后画，压在最上面";
    EXPECT_EQ(marks.back().facing, 1);
    EXPECT_FLOAT_EQ(marks.back().cell.x, 10.5f);
}

TEST(MinimapMarks, TheObjectiveIsTheTargetHereOrTheDoorThatLeadsToIt) {
    const MapObject npc = object("npc", 5, 6);
    const MapObject door = object("portal", 0, 3);
    const MapObject other = object("portal", 9, 0);
    MapObject save = object("facility", 4, 4);
    save.properties["kind"] = "save";
    MapObject field = object("facility", 6, 4);
    field.properties["kind"] = "field";
    WorldFrame frame;
    frame.portals = {{&door, true, true}, {&other, false, false}};
    frame.facilities = {{&save, "save"}, {&field, "field"}};
    fanren::core::WorldObjectState objects;

    auto marks = mm::collectMarks(frame, objects, Vec2f{2.f, 2.f}, 2);
    EXPECT_EQ(countKind(marks, mm::MarkKind::DoorGuide), 1);
    EXPECT_EQ(countKind(marks, mm::MarkKind::DoorLocked), 1);
    EXPECT_EQ(countKind(marks, mm::MarkKind::Save), 1);
    EXPECT_EQ(countKind(marks, mm::MarkKind::Facility), 1);
    ASSERT_EQ(countKind(marks, mm::MarkKind::Objective), 1);
    for (const mm::Mark& m : marks) {
        if (m.kind == mm::MarkKind::Objective) EXPECT_FLOAT_EQ(m.cell.x, 0.5f) << "目标在别的图：标在那道门上";
    }
    frame.objectiveTarget = &npc;
    marks = mm::collectMarks(frame, objects, Vec2f{2.f, 2.f}, 2);
    for (const mm::Mark& m : marks) {
        if (m.kind == mm::MarkKind::Objective) {
            EXPECT_FLOAT_EQ(m.cell.x, 5.5f) << "目标在本图：标在它身上";
            EXPECT_FLOAT_EQ(m.cell.y, 6.5f);
        }
    }
}

// ===========================================================================
// 3. 第 7 章 18 张图与第 3、6 章的剪影
// ===========================================================================

// PNG 的宽高（IHDR 紧跟在 8 字节签名与 8 字节块头之后，大端）。
bool pngSize(const fs::path& file, int& w, int& h) {
    std::ifstream in(file, std::ios::binary);
    unsigned char head[24] = {};
    if (!in.read(reinterpret_cast<char*>(head), sizeof(head))) return false;
    w = (head[16] << 24) | (head[17] << 16) | (head[18] << 8) | head[19];
    h = (head[20] << 24) | (head[21] << 16) | (head[22] << 8) | head[23];
    return true;
}

const std::vector<std::string>& ch07Maps() {
    static const std::vector<std::string> maps{
        "ch07_dihuo",          "ch07_dixia_shiku",  "ch07_dixia_zhaoze",      "ch07_fangshi",
        "ch07_huanxingshan",   "ch07_jindi_wai",    "ch07_jindi_waiwei",      "ch07_jindi_zhongxin",
        "ch07_shandong",       "ch07_yuelu_dian",   "ch07_zhaoze_shenchu",    "ch06_baiyaoyuan",
        "lilian_jindi_huanxingshan", "lilian_jindi_shenchu", "lilian_jindi_shiku",
        "lilian_jindi_waiwei", "lilian_jindi_zhaoze", "lilian_jindi_zhongxin",
    };
    return maps;
}

TEST(MinimapMaps, EveryChapter7MapHasAHdBaseAndEveryMarkLandsOnItsOwnCells) {
    const fs::path root = repoRoot();
    int checkedMaps = 0;
    int checkedObjects = 0;
    for (const std::string& id : ch07Maps()) {
        const fs::path tmj = root / "maps" / (id + ".tmj");
        if (!fs::exists(tmj)) {
            ADD_FAILURE() << id << " 不在 maps/ 里";
            continue;
        }
        auto loaded = fanren::io::loadTileMap(tmj.string());
        ASSERT_TRUE(loaded.ok) << loaded.error;
        const fanren::core::TileMap& map = loaded.value;
        ++checkedMaps;
        const fs::path hd = root / "assets" / "art" / "maps_hd" / id / "below.png";
        int w = 0;
        int h = 0;
        if (!fs::exists(hd)) {
            // 没有高清图的就走剪影——也画得出；只记下来，不当失败。
            EXPECT_EQ(mm::silhouettePixels(map).size(), static_cast<std::size_t>(map.width * map.height)) << id;
        } else {
            ASSERT_TRUE(pngSize(hd, w, h)) << hd;
            EXPECT_EQ(w, map.width * fanren::game::kHdArtTilePx) << id << " 烘焙图与格数对不上，底图会退成剪影";
            EXPECT_EQ(h, map.height * fanren::game::kHdArtTilePx) << id;
        }
        const RectF small = mm::mapArea(mm::panelRect());
        const RectF large = fanren::game::AreaMapScene::mapRect();
        for (const MapObject& o : map.objects) {
            if (o.type != "portal" && o.type != "facility" && o.type != "chest" && o.type != "gather" && o.type != "foe")
                continue;
            ++checkedObjects;
            const Vec2f c = mm::objectCenter(o);
            // 两种取景：小地图（对准这个对象，窗开着也看得见它）与区域地图（缺省最小 8 像素一格）。
            for (const mm::Layout& l : {mm::fit(small, map.width, map.height, c),
                                        mm::fit(large, map.width, map.height, c, 8.f, 32.f)}) {
                const Vec2f at = mm::toScreen(l, c);
                EXPECT_TRUE(mm::contains(l, at)) << id << " " << o.name << " 对准了却不在取景里";
                const Point cell = mm::toCell(l, at);
                EXPECT_GE(cell.x, o.position.x) << id << " " << o.name;
                EXPECT_LT(cell.x, o.position.x + std::max(1, o.width)) << id << " " << o.name;
                EXPECT_GE(cell.y, o.position.y) << id << " " << o.name;
                EXPECT_LT(cell.y, o.position.y + std::max(1, o.height)) << id << " " << o.name;
            }
        }
    }
    EXPECT_EQ(checkedMaps, 18);
    EXPECT_GT(checkedObjects, 100);
}

TEST(MinimapMaps, Chapter3And6MapsWithoutHdArtGetASilhouette) {
    const fs::path root = repoRoot();
    for (const char* id : {"ch03_guwai", "ch06_huangfenggu"}) {
        const fs::path tmj = root / "maps" / (std::string(id) + ".tmj");
        if (!fs::exists(tmj)) {
            // 换一张同章的图：只要求第 3、6 章各有一张没有高清图的。
            continue;
        }
        EXPECT_FALSE(fs::exists(root / "assets" / "art" / "maps_hd" / id)) << id << " 本该没有高清烘焙图";
        auto loaded = fanren::io::loadTileMap(tmj.string());
        ASSERT_TRUE(loaded.ok) << loaded.error;
        const auto pixels = mm::silhouettePixels(loaded.value);
        ASSERT_EQ(pixels.size(), static_cast<std::size_t>(loaded.value.width * loaded.value.height));
        int walkable = 0;
        for (int y = 0; y < loaded.value.height; ++y) {
            for (int x = 0; x < loaded.value.width; ++x) {
                const bool open = loaded.value.walkable(Point{x, y});
                walkable += open ? 1 : 0;
                EXPECT_EQ(pixels[static_cast<std::size_t>(y * loaded.value.width + x)],
                          open ? mm::kSilhouetteWalkable : mm::kSilhouetteBlocked);
            }
        }
        EXPECT_GT(walkable, 0) << id;
    }
    // 两章各至少一张没有高清图的图（随便哪张）。
    for (const char* prefix : {"ch03_", "ch06_"}) {
        int without = 0;
        for (const auto& entry : fs::directory_iterator(root / "maps")) {
            const std::string name = entry.path().stem().string();
            if (name.rfind(prefix, 0) == 0 && !fs::exists(root / "assets" / "art" / "maps_hd" / name)) ++without;
        }
        EXPECT_GT(without, 0) << prefix;
    }
}

// ===========================================================================
// 4. 真入口
// ===========================================================================

class MapWiring : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        root_ = std::make_unique<fanren::test::TempDir>("fanren_map_wiring");
        for (const char* dir : {"data", "scripts", "maps"}) {
            fs::copy(repoRoot() / dir, root_->path() / dir, fs::copy_options::recursive);
        }
    }
    static void TearDownTestSuite() { root_.reset(); }

    void SetUp() override {
        const auto ready = app_.init(root_->path().string(), true);
        ASSERT_TRUE(ready.ok) << ready.error;
        const auto started = app_.startNewJourney();
        ASSERT_TRUE(started.ok) << started.error;
        tick();
        ASSERT_EQ(top(), "World");
    }
    void TearDown() override { app_.shutdown(); }
    void tick() { app_.tick(1.0 / 60.0); }
    std::string top() { return app_.topScene() == nullptr ? "" : app_.topScene()->name(); }
    void press(SDL_Scancode key) {
        for (const bool down : {true, false}) {
            SDL_Event event{};
            event.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
            event.key.scancode = key;
            event.key.down = down;
            ASSERT_TRUE(SDL_PushEvent(&event)) << SDL_GetError();
            tick();
        }
    }
    void pressPad(Uint8 button) {
        for (const bool down : {true, false}) {
            SDL_Event event{};
            event.type = down ? SDL_EVENT_GAMEPAD_BUTTON_DOWN : SDL_EVENT_GAMEPAD_BUTTON_UP;
            event.gbutton.which = 902;
            event.gbutton.button = button;
            event.gbutton.down = down;
            ASSERT_TRUE(SDL_PushEvent(&event)) << SDL_GetError();
            tick();
        }
    }
    const WorldFrame& frame() {
        fanren::game::WorldScene* world = app_.worldScene();
        EXPECT_NE(world, nullptr);
        return world->describeFrame(app_);
    }

    fanren::game::Application app_;
    inline static std::unique_ptr<fanren::test::TempDir> root_;
};

TEST_F(MapWiring, MOpensTheAreaMapAndMClosesIt) {
    EXPECT_TRUE(frame().minimap) << "世界层在最上面、设置缺省开：画小地图";
    press(SDL_SCANCODE_M);
    ASSERT_EQ(top(), "AreaMap");
    auto* map = dynamic_cast<fanren::game::AreaMapScene*>(app_.topScene());
    ASSERT_NE(map, nullptr);
    EXPECT_FALSE(map->exits().empty()) << "韩家村有出口";
    EXPECT_EQ(map->marks().back().kind, mm::MarkKind::Hero);
    EXPECT_FALSE(frame().minimap) << "区域地图压着时小地图不画";
    press(SDL_SCANCODE_M);
    EXPECT_EQ(top(), "World");
    pressPad(SDL_GAMEPAD_BUTTON_LEFT_SHOULDER);
    EXPECT_EQ(top(), "AreaMap") << "手柄 LB 同样打开";
    press(SDL_SCANCODE_ESCAPE);
    EXPECT_EQ(top(), "World") << "Esc 也关得上";
}

TEST_F(MapWiring, TheMainMenuHasAMapEntry) {
    press(SDL_SCANCODE_TAB);
    ASSERT_EQ(top(), "Menu");
    auto* menu = dynamic_cast<fanren::game::MenuScene*>(app_.topScene());
    ASSERT_NE(menu, nullptr);
    const auto& pages = menu->pages();
    const auto at = std::find(pages.begin(), pages.end(), fanren::game::MenuScene::Page::Map);
    ASSERT_NE(at, pages.end()) << "主菜单有「地图」";
    EXPECT_EQ(fanren::game::MenuScene::pageLabel(app_.data(), app_.state(), fanren::game::MenuScene::Page::Map), "地图");
    for (auto it = pages.begin(); it != at; ++it) press(SDL_SCANCODE_DOWN);
    press(SDL_SCANCODE_RETURN);
    EXPECT_EQ(top(), "AreaMap") << "确认：菜单合上、区域地图打开";
    press(SDL_SCANCODE_ESCAPE);
    EXPECT_EQ(top(), "World");
}

TEST_F(MapWiring, TheSettingTurnsTheMinimapOffAndTheKeyIsInTheKeyTable) {
    EXPECT_TRUE(frame().minimap);
    fanren::io::Settings off = app_.settings();
    off.minimap = false;
    app_.setSettings(off);
    EXPECT_FALSE(frame().minimap) << "设置里关掉就不画";
    press(SDL_SCANCODE_M);
    EXPECT_EQ(top(), "AreaMap") << "关了小地图，地图键照样能看整张图";
    EXPECT_STREQ(fanren::engine::Engine::keyId(fanren::engine::Engine::Key::Map), "map");
    EXPECT_EQ(app_.engine().customKeys(fanren::engine::Engine::Key::Map)[0], SDL_SCANCODE_M);
    EXPECT_EQ(app_.text("ui.map.minimap.key"), "M 地图") << "提示跟着键位走";
}

}  // namespace
