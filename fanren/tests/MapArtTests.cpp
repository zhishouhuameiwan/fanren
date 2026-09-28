// 占位美术的那张对照表，与全部 18 张真地图对账。
//
// 这一批修的 bug 不是「哪一行写错了」，而是**两份约定分了家**：
// tools/mapgen/genmaps*.py 的文件头写着 gid 1-8 各是什么，渲染器里的色板却对着
// 另一套语义，且用 `gid % 8` 兜底。于是 gid 8（篱笆 / 摊架 / 兵器架，实心挡路）
// 落到 `8 % 8 == 0` 上被兜成草地绿——全 18 张图 390 格实心障碍长得和可走的草地
// 一模一样；gid 4（树冠 / 屋檐）画成不透明的水蓝盖在人物头上，1081 格。
//
// 没有任何测试能在「两份约定分家」这件事上报警，因为两份各自都自洽。这个文件
// 就是那个缺掉的对账：语义由 tileRole 给出，实际用到的编号由真地图给出，两边
// 逐层核对。任何一侧单方面改动都会在这里变红。
//
// 每条断言都写成**两个方向**：不只要求「现在这些编号认得出」，还要求「约定之外
// 的编号必须落在 Unknown 上」——少了后半条，一个「什么都返回 Ground」的实现
// 也能全绿，而那正是旧色板的病。
#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "core/model/Types.h"
#include "game/MapArt.h"
#include "io/DataLoader.h"

namespace {

namespace fs = std::filesystem;

using fanren::game::TileRole;
using fanren::game::facilityColor;
using fanren::game::facilityGlyph;
using fanren::game::npcStyle;
using fanren::game::playerStyle;
using fanren::game::tilePalette;
using fanren::game::tileRole;

// 仓库根：测试可能从 build/ 或工程根启动。判据用本文件真正要读的东西
// （maps/ 里那张最早的图），找错根目录时报的是「找不到地图」而不是一串空断言。
std::string assetRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        const fs::path root(candidate);
        if (fs::exists(root / "maps" / "ch01_hanjiacun.tmj")) return candidate;
    }
    return ".";
}

std::vector<fs::path> allMaps() {
    std::vector<fs::path> maps;
    for (const fs::directory_entry& entry : fs::directory_iterator(fs::path(assetRoot()) / "maps")) {
        if (entry.is_regular_file() && entry.path().extension() == ".tmj") {
            maps.push_back(entry.path());
        }
    }
    std::sort(maps.begin(), maps.end());
    return maps;
}

[[nodiscard]] bool sameColor(const fanren::engine::Color& a, const fanren::engine::Color& b) {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

// ---------------------------------------------------------------------------
// gid → 语义
// ---------------------------------------------------------------------------

TEST(MapArt, GidMapsToTheConventionInGenmaps) {
    // 这八条就是 tools/mapgen/genmaps.py 文件头那张表，一条不多一条不少。
    EXPECT_EQ(tileRole(0), TileRole::Empty);
    EXPECT_EQ(tileRole(1), TileRole::Ground);
    EXPECT_EQ(tileRole(2), TileRole::Road);
    EXPECT_EQ(tileRole(3), TileRole::Special);
    EXPECT_EQ(tileRole(4), TileRole::Canopy);
    EXPECT_EQ(tileRole(5), TileRole::Wall);
    EXPECT_EQ(tileRole(6), TileRole::Flora);
    EXPECT_EQ(tileRole(7), TileRole::Rubble);
    EXPECT_EQ(tileRole(8), TileRole::Fence);
}

TEST(MapArt, UnknownGidsAreNotQuietlyFoldedIntoSomethingElse) {
    // 旧实现是 `gid % 8`，于是 9 变成 1（草地）、16 变成 0 再兜成草地。
    // 这几条钉的就是「不许再有任何就近兜底」：认不出来就得说认不出来。
    EXPECT_EQ(tileRole(9), TileRole::Unknown);
    EXPECT_EQ(tileRole(16), TileRole::Unknown);
    EXPECT_EQ(tileRole(64), TileRole::Unknown);
    // Tiled 用高位存翻转标志，出现负 gid 说明导出时开了翻转——同样是约定之外。
    EXPECT_EQ(tileRole(-1), TileRole::Unknown);
}

// ---------------------------------------------------------------------------
// 与真地图对账
// ---------------------------------------------------------------------------

TEST(MapArt, EveryTileInEveryShippedMapHasAKnownRole) {
    const std::vector<fs::path> maps = allMaps();
    // 空目录会让下面那个循环一条断言也不跑，而那看起来和全绿一模一样。
    ASSERT_GE(maps.size(), 18u) << "maps/ 里没找到应有的那些图，测试的根目录找错了";

    for (const fs::path& path : maps) {
        auto loaded = fanren::io::loadTileMap(path.string());
        ASSERT_TRUE(loaded.ok) << path.string() << "：" << loaded.error;
        const fanren::core::TileMap& map = loaded.value;

        // 每一层只许出现属于它的那几种语义。这一条比「不是 Unknown 就行」严得多：
        // 它同时钉住「front 层画的是遮挡物」「building 层画的是实心物」——而遮挡
        // 画成不透明、实心画成可走的地表，正是这次要修的那两个 bug。
        const auto check = [&path](const std::vector<int>& layer, const char* layerName,
                                   const std::set<TileRole>& allowed) {
            for (const int gid : layer) {
                const TileRole role = tileRole(gid);
                EXPECT_TRUE(allowed.count(role) != 0)
                    << path.filename().string() << " 的 " << layerName << " 层出现了 gid " << gid
                    << "，它不属于这一层该有的语义";
            }
        };

        check(map.ground, "ground",
              {TileRole::Empty, TileRole::Ground, TileRole::Road, TileRole::Special});
        check(map.overlay, "overlay", {TileRole::Empty, TileRole::Flora, TileRole::Rubble});
        check(map.building, "building", {TileRole::Empty, TileRole::Wall, TileRole::Fence});
        check(map.front, "front", {TileRole::Empty, TileRole::Canopy});
    }
}

TEST(MapArt, TheShippedMapsUseGid3BothAsWaterAndAsWalkableGround) {
    // 这一条钉的是「水与土的判据」本身还站得住。
    //
    // 渲染器靠碰撞层区分同一个 gid 3 是水面还是翻土，依据是生成器那条固定写法
    // 「水面不画墙，只挡路」。万一哪天有人给水面补上 building、或者把某片翻土
    // 设成挡路，这条判据就悄悄失效了——画面上的表现是一潭水变成一片田，而那
    // 种错没人会在门禁上看见。
    //
    // 两个方向都要有样本：只有挡路的那一半时，把判据写成「一律画水」也能全绿。
    const std::vector<fs::path> maps = allMaps();
    ASSERT_GE(maps.size(), 18u) << "maps/ 里没找到应有的那些图，测试的根目录找错了";

    int blockedCells = 0;
    int walkableCells = 0;
    for (const fs::path& path : maps) {
        auto loaded = fanren::io::loadTileMap(path.string());
        ASSERT_TRUE(loaded.ok) << path.string() << "：" << loaded.error;
        const fanren::core::TileMap& map = loaded.value;
        for (std::size_t i = 0; i < map.ground.size(); ++i) {
            if (tileRole(map.ground[i]) != TileRole::Special) continue;
            const bool blocked = i < map.collision.size() && map.collision[i] != 0;
            (blocked ? blockedCells : walkableCells) += 1;
            // 水面不画墙——它只靠碰撞层挡路。这一格要是有了 building，
            // 上面那条判据就不成立了。
            if (blocked) {
                EXPECT_TRUE(i >= map.building.size() || map.building[i] == 0)
                    << path.filename().string() << " 的第 " << i
                    << " 格：挡路的特殊地表上画了 building，水与墙的分工乱了";
            }
        }
    }
    EXPECT_GT(blockedCells, 0) << "没有一格挡路的特殊地表，水面那条分支从没被走到过";
    EXPECT_GT(walkableCells, 0) << "没有一格可踩的特殊地表，翻土那条分支从没被走到过";
}

TEST(MapArt, WaterAndTilledDoNotShareAColour) {
    // 两种地表用同一个颜色，等于这条区分没做。两套配色都要查：
    // 室内那一套（暗道、密室）同样有可踩与挡路两种特殊地表。
    for (const bool outdoor : {true, false}) {
        const fanren::game::TilePalette& palette = tilePalette(outdoor);
        EXPECT_FALSE(sameColor(palette.special, palette.tilled))
            << (outdoor ? "室外" : "室内") << "：水面与翻土同色";
        EXPECT_FALSE(sameColor(palette.tilled, palette.road))
            << (outdoor ? "室外" : "室内") << "：翻土与道路同色，田埂与路分不开";
        EXPECT_FALSE(sameColor(palette.tilled, palette.ground))
            << (outdoor ? "室外" : "室内") << "：翻土与主地表同色";
    }
}

TEST(MapArt, EveryFacilityKindOnTheMapsHasItsOwnGlyph) {
    const std::vector<fs::path> maps = allMaps();
    ASSERT_GE(maps.size(), 18u) << "maps/ 里没找到应有的那些图，测试的根目录找错了";

    std::map<std::string, std::string> seen;   // kind -> glyph
    for (const fs::path& path : maps) {
        auto loaded = fanren::io::loadTileMap(path.string());
        ASSERT_TRUE(loaded.ok) << path.string() << "：" << loaded.error;
        for (const fanren::core::MapObject& object : loaded.value.objects) {
            if (object.type != "facility") continue;
            const std::string kind = object.property("kind");
            const std::string glyph = facilityGlyph(kind);
            // 「?」是给没登记的 kind 留的报错字形。地图上真在用的 kind 落到它
            // 上面，等于这一类设施在画面上没有身份。
            EXPECT_NE(glyph, "?") << path.filename().string() << " 用了没登记字形的 kind："
                                  << kind;
            seen[kind] = glyph;
        }
    }
    ASSERT_FALSE(seen.empty()) << "一张图上都没有设施，这个测试什么也没验到";

    // 字形必须两两不同：两种设施共用一个字，与共用一个绿框没有区别。
    std::set<std::string> glyphs;
    for (const auto& [kind, glyph] : seen) {
        EXPECT_TRUE(glyphs.insert(glyph).second) << "kind " << kind << " 的字形 " << glyph
                                                 << " 与别的设施重了";
    }
}

TEST(MapArt, EveryDeclaredFacilityKindIsDistinct) {
    // docs/map_spec.md 第 4.6 节声明的九种。地图上眼下只用到六种，
    // 另外三种（forge / talisman / formation）要到后面的章节才出现——
    // 等它们出现时字形与配色必须已经就位，而不是到那时才发现是两个一样的框。
    const std::vector<std::string> kinds{"alchemy", "forge",    "talisman", "formation", "field",
                                         "meditate", "shop",    "board",    "save"};
    std::set<std::string> glyphs;
    for (const std::string& kind : kinds) {
        const std::string glyph = facilityGlyph(kind);
        EXPECT_NE(glyph, "?") << kind << " 没有字形";
        EXPECT_TRUE(glyphs.insert(glyph).second) << kind << " 的字形与别人重了";
    }
    EXPECT_EQ(facilityGlyph("no_such_kind"), "?");

    // 配色同样要两两不同，且表外的 kind 给洋红——那是一眼看得出的报错色。
    for (std::size_t i = 0; i < kinds.size(); ++i) {
        for (std::size_t j = i + 1; j < kinds.size(); ++j) {
            EXPECT_FALSE(sameColor(facilityColor(kinds[i]), facilityColor(kinds[j])))
                << kinds[i] << " 与 " << kinds[j] << " 配色相同";
        }
    }
    EXPECT_TRUE(sameColor(facilityColor("no_such_kind"), fanren::engine::Color{200, 0, 160, 255}));
}

// ---------------------------------------------------------------------------
// 人物
// ---------------------------------------------------------------------------

TEST(MapArt, NpcColoursAreStableAndNeverTheProtagonists) {
    const std::vector<fs::path> maps = allMaps();
    ASSERT_GE(maps.size(), 18u) << "maps/ 里没找到应有的那些图，测试的根目录找错了";

    std::set<std::string> roles;
    for (const fs::path& path : maps) {
        auto loaded = fanren::io::loadTileMap(path.string());
        ASSERT_TRUE(loaded.ok) << path.string() << "：" << loaded.error;
        for (const fanren::core::MapObject& object : loaded.value.objects) {
            if (object.type != "npc") continue;
            roles.insert(object.property("role_id"));
        }
    }
    ASSERT_FALSE(roles.empty()) << "一张图上都没有 NPC，这个测试什么也没验到";

    for (const std::string& roleId : roles) {
        // 同一个人每次进图都得是同一身衣服，否则玩家认不住人。
        const fanren::game::FigureStyle first = npcStyle(roleId);
        const fanren::game::FigureStyle again = npcStyle(roleId);
        EXPECT_TRUE(sameColor(first.robe, again.robe)) << roleId << " 的衣色每次都不一样";
        EXPECT_TRUE(sameColor(first.head, again.head)) << roleId << " 的肤色每次都不一样";

        // 主角那身亮白是保留色：屏幕上那个白衣的人永远只有一个，
        // 玩家靠它认自己站在哪。
        EXPECT_FALSE(sameColor(first.robe, playerStyle().robe))
            << roleId << " 与主角撞了衣色，玩家分不出哪个是自己";
    }
}

TEST(MapArt, IndoorAndOutdoorPalettesDiffer) {
    // 同一个 gid 1 在村口是草地、在居所里是木地板。两套配色只要有一处相同，
    // 就说明 outdoor 这个属性又被无视了——它此前在 TileMap 里躺了整整四章
    // 没有任何渲染代码看过。
    const fanren::game::TilePalette& outdoor = tilePalette(true);
    const fanren::game::TilePalette& indoor = tilePalette(false);
    EXPECT_FALSE(outdoor.indoor);
    EXPECT_TRUE(indoor.indoor);
    EXPECT_FALSE(sameColor(outdoor.ground, indoor.ground));
    EXPECT_FALSE(sameColor(outdoor.special, indoor.special));
    EXPECT_FALSE(sameColor(outdoor.canopy, indoor.canopy));

    // 遮挡层必须是半透明的：不透明的树冠会把树下的人整个盖掉，
    // 而那正是旧实现的样子（gid 4 画成不透明水蓝）。
    EXPECT_LT(outdoor.canopy.a, 255);
    EXPECT_LT(indoor.canopy.a, 255);
}

}  // namespace
