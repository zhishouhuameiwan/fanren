// 过门不回弹：从每一道门按住方向键走进去，到了对面那张图接着按住同一个方向，
// 不许几步之内又踩上回程门、被送回原来那张图。
//
// ---------------------------------------------------------------------------
// 钉的是什么
// ---------------------------------------------------------------------------
// 玩家报：韩家村出口往下走，进青牛镇却落在最下面，多按一下「下」就在两张图之间
// 来回弹。病根是衔接「掉头」：从 A 图的南边出去，落在 B 图的南边、离回程门只有
// 一两格；而世界层按住方向键是一直走的（WorldScene::update：keyDown 加 0.12 秒一步
// 的冷却），落地之后再走一两步就踩回回程门。全仓扫出四对。
//
// 判据，对每道门 P（A → B）的每一个「进门方向」逐个走一遍：
//   · 进门方向 = P 的某一格 c 加一个方向 d：c-d 在图内、不属于 P，c-d 与 c 都按引擎
//     口径走得上去——walkable；在场的 NPC 挡路（在不在场由 WorldScene::npcVisible 判，
//     经 visibleNpcAt，与 tryStep 同一个函数）；设施挡路。站在 c-d 朝 d 迈一步就踩上 P。
//   · 迈这一步之后，当前图必须变成 B；
//   · 接着按住 d 再迈最多 8 步（0.12 秒一步，约一秒——远不止「多按一下」），每一步之后
//     当前图都不许是 A。脚本起来了（落点旁有踏入型剧情，那是设计）或迈不动了
//    （tryStep 返回 false）就停。
//
// ---------------------------------------------------------------------------
// 为什么这样钉
// ---------------------------------------------------------------------------
// 1. **走真入口。** 真的 Application（无头）、真的 maps/*.tmj、真的 WorldScene::tryStep，
//    与无头通关测试踩门是同一条路（tests/Ch03SliceTests.cpp 的 usePortal）。不另写一份
//    「门通到哪、落在哪、往前几格是什么」的推算：推算与引擎一旦分了家，绿灯只证明推算自洽。
//    门从哪里来也不抄表：枚举的就是 maps/ 下每张图上的每一个 portal 对象。
//
// 2. **每道门、每个进门方向都走。** 回弹只在某一个方向上发生——往下出韩家村会弹，
//    横着走进同一道门就不会。只挑一个方向走，一半的病看不见。
//
// 3. **两种存档状态各走一遍**（kFlagSettings）。
//    · 「只开这一道门」：只置 P 的 require_flag，别的旗标一个不置。
//    · 「门全开」：地图对象上用到的旗标全部置位（require / guard / set / visible /
//      hidden）——每道门都开着，每个一次性剧情都演过了。
//    只走前一种会漏：回程门自己也挂着 require_flag 时，在「只开这一道门」的存档里它是关着的，
//    踩上去只弹一句提示、不换图，回弹于是藏住了。可玩家真走到那里的时候它多半早开了——
//    密室回神手谷就是：进密室先得 ch03.mo_gui_gu，而神手谷那道回密室的门挂的正是它，
//    「人在密室里、那道门却关着」的存档根本不存在。后一种把这个洞补上，也补上另一种遮挡：
//    落点前面压着一个还没演的一次性剧情，在前一种存档里把人拦下了，演过之后就拦不住。
//    反过来，前一种也不能省：旗标全开时 NPC 该在的在、该走的走，挡路的人不一样。
//
// 4. **先验。** 两种状态下，实际走过的门数都必须等于枚举出来的门数且大于 0；每道门至少
//    一个进门方向，一个都没有就报出来——那是地图坏了（门四面是墙、或门格本身走不上去）。
//    没有这两条，地图目录找错了、或者进门方向一个都没算出来，这条用例照样是绿的。
//
// 与邻居的分工：
//   * tests/WorldViewTests.cpp 的 EveryShippedPortalOnAMapEdgePointsOutThroughThatEdge
//     —— 门上的箭头指哪边，纯画面；
//   * tools/validate.py —— 门的两头是否互通、spawn 是否存在，静态门禁；
//   * 本文件 —— 按住方向键真走一趟，过了门还会不会被原路送回去。
#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "core/model/Types.h"
#include "game/Application.h"
#include "game/WorldScene.h"
#include "io/DataLoader.h"

namespace {

namespace fs = std::filesystem;

using fanren::core::GameState;
using fanren::core::MapObject;
using fanren::core::Point;
using fanren::core::TileMap;
using fanren::game::Application;
using fanren::game::WorldScene;

// 过了门之后接着按住几步。0.12 秒一步（WorldScene 的 kStepCooldownSeconds），8 步约一秒。
constexpr int kHoldSteps = 8;

struct Direction {
    Point step;
    const char* name;   // 给人看的：上 / 右 / 下 / 左
    int facing;         // GameState::facing 那一套：0 上 1 右 2 下 3 左
};

constexpr Direction kDirections[] = {
    {{0, -1}, "上", 0},
    {{1, 0}, "右", 1},
    {{0, 1}, "下", 2},
    {{-1, 0}, "左", 3},
};

// 两种存档状态，理由见文件头第 3 条。
struct FlagSetting {
    const char* name;
    bool everyDoorOpen;
};

constexpr FlagSetting kFlagSettings[] = {
    {"只开这一道门", false},
    {"门全开", true},
};

// 与 tests/WorldViewTests.cpp 同一个找法。
std::string repoRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "maps" / "ch01_hanjiacun.tmj")) return candidate;
    }
    return ".";
}

std::string cellText(Point cell) {
    return "(" + std::to_string(cell.x) + "," + std::to_string(cell.y) + ")";
}

// 与 core::TileMap::objectAt 逐字相同的矩形命中：门「占哪几格」必须与引擎认的是同一组格子。
bool covers(const MapObject& object, Point cell) {
    return cell.x >= object.position.x && cell.x < object.position.x + object.width &&
           cell.y >= object.position.y && cell.y < object.position.y + object.height;
}

// 这一格踩不踩得上去：WorldScene::tryStep 的规则，一条不多一条不少。
bool enterable(const TileMap& map, const GameState& state, Point cell) {
    if (!map.walkable(cell)) return false;
    if (WorldScene::visibleNpcAt(state, map, cell) != nullptr) return false;   // 在场的人挡路
    if (map.objectAt(cell, "facility") != nullptr) return false;
    return true;
}

struct Approach {
    Point stand;          // c-d：站在这里……
    Direction direction;  // ……朝 d 迈一步，就踩上门格 c
};

// 一道门的全部进门方向（文件头「判据」那一段）。
std::vector<Approach> approachesOf(const TileMap& map, const MapObject& portal, const GameState& state) {
    std::vector<Approach> out;
    for (int y = portal.position.y; y < portal.position.y + portal.height; ++y) {
        for (int x = portal.position.x; x < portal.position.x + portal.width; ++x) {
            const Point cell{x, y};
            if (!enterable(map, state, cell)) continue;   // 门格自己都迈不上去，从哪边来都一样
            for (const Direction& direction : kDirections) {
                const Point stand{x - direction.step.x, y - direction.step.y};
                if (!map.inBounds(stand) || covers(portal, stand)) continue;
                if (!enterable(map, state, stand)) continue;
                out.push_back(Approach{stand, direction});
            }
        }
    }
    return out;
}

// 地图对象上出现过的全部旗标，一律置 1：「门全开」那一种存档。
std::map<std::string, int> flagsOnMaps(const std::vector<TileMap>& maps) {
    static constexpr const char* kFlagKeys[] = {"require_flag", "guard_flag", "set_flag",
                                                "visible_flag", "hidden_flag"};
    std::map<std::string, int> flags;
    for (const TileMap& map : maps) {
        for (const MapObject& object : map.objects) {
            for (const char* key : kFlagKeys) {
                const std::string name = object.property(key);
                if (!name.empty()) flags[name] = 1;
            }
        }
    }
    return flags;
}

std::string describePortal(const TileMap& map, const MapObject& portal) {
    return map.id + " 的 " + portal.name + "（→ " + portal.property("target_map") + " / " +
           portal.property("target_spawn") + "）";
}

class MapLinkWalk : public ::testing::Test {
protected:
    void SetUp() override {
        auto ready = app_.init(repoRoot(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
    }
    void TearDown() override { app_.shutdown(); }

    // 从 from 图站到 approach.stand，朝 approach.direction 迈进 portal，再按住同一个方向接着走。
    // 返回空串 = 这一趟没事；否则是一句写明经过的话。
    std::string walkThrough(const std::string& from, const MapObject& portal, const Approach& approach,
                            const std::map<std::string, int>& flags) {
        GameState& state = app_.state();
        state.flags = flags;
        // 任一 spawn 都行：人马上就被挪到门前。
        const auto loaded = app_.loadMap(from, std::string{});
        if (!loaded.ok) return "载不进 " + from + "：" + loaded.error;
        state.position = approach.stand;
        state.facing = approach.direction.facing;

        const std::string target = portal.property("target_map");
        WorldScene world;
        world.tryStep(app_, approach.direction.step.x, approach.direction.step.y);
        std::string verdict;
        if (app_.scripts().isRunning()) {
            verdict = "踩上门格却起了剧情脚本（门格上压着一个此刻备着的踏入型触发），没有换图";
        } else if (state.mapId != target) {
            verdict = "迈进门之后当前图是 " + state.mapId + cellText(state.position) + "，不是 " + target;
        } else {
            verdict = holdOn(world, from, approach.direction);
        }
        // 这一趟起了的剧情就地丢掉：tryStep 在脚本跑着时一步都不受理，不丢的话下一趟迈不动。
        // 这里从不 tick，脚本的命令一条也没演，丢掉它不会在存档上留下半截剧情。
        app_.scripts().abortEvent("MapLinkWalk：这一趟走完了");
        return verdict;
    }

    // 已经进了门：按住同一个方向接着走，每一步之后当前图都不许是 from。
    std::string holdOn(WorldScene& world, const std::string& from, const Direction& direction) {
        const GameState& state = app_.state();
        const std::string landedMap = state.mapId;
        const Point landed = state.position;
        for (int held = 1; held <= kHoldSteps; ++held) {
            // 落点旁的踏入型剧情起来了：那是设计，玩家此刻被剧情接走，不会接着走。
            if (app_.scripts().isRunning() || app_.awaitingCommand()) break;
            const Point next{state.position.x + direction.step.x, state.position.y + direction.step.y};
            const std::string mapBefore = state.mapId;
            // 门的名字先抄下来：一换图，这张图连同它上面的对象就整个没了。
            const MapObject* door = app_.currentMap()->objectAt(next, "portal");
            const std::string doorName = door != nullptr ? door->name : std::string{};
            if (!world.tryStep(app_, direction.step.x, direction.step.y)) break;   // 迈不动了
            if (state.mapId == from) {
                return "落在 " + landedMap + cellText(landed) + "；按住「" + direction.name + "」第 " +
                       std::to_string(held) + " 步踩上 " + mapBefore + " 的 " + doorName + cellText(next) +
                       "，又回到了 " + from + cellText(state.position) + "——两张图之间来回弹";
            }
        }
        return {};
    }

    Application app_;
};

TEST_F(MapLinkWalk, HoldingTheKeyThroughEveryPortalNeverBouncesBack) {
    const fs::path mapsDir = fs::path(repoRoot()) / "maps";
    std::vector<fs::path> files;
    for (const fs::directory_entry& entry : fs::directory_iterator(mapsDir)) {
        if (entry.path().extension() == ".tmj") files.push_back(entry.path());
    }
    std::sort(files.begin(), files.end());   // 报错的次序与文件系统无关
    std::vector<TileMap> maps;
    for (const fs::path& file : files) {
        auto loaded = fanren::io::loadTileMap(file.string());
        ASSERT_TRUE(loaded.ok) << loaded.error;
        maps.push_back(std::move(loaded.value));
    }
    ASSERT_FALSE(maps.empty()) << mapsDir.string() << " 下一张地图都没有，下面的循环全是空的";

    int enumerated = 0;
    for (const TileMap& map : maps) {
        enumerated += static_cast<int>(std::count_if(map.objects.begin(), map.objects.end(),
                                                     [](const MapObject& o) { return o.type == "portal"; }));
    }
    ASSERT_GT(enumerated, 0) << "全部地图上一道门都没枚举到，这条用例什么也没验";

    const std::map<std::string, int> everyFlag = flagsOnMaps(maps);
    for (const FlagSetting& setting : kFlagSettings) {
        int walkedPortals = 0;
        int walks = 0;
        for (const TileMap& map : maps) {
            for (const MapObject& portal : map.objects) {
                if (portal.type != "portal") continue;
                std::map<std::string, int> flags =
                    setting.everyDoorOpen ? everyFlag : std::map<std::string, int>{};
                const std::string need = portal.property("require_flag");
                if (!need.empty()) flags[need] = 1;

                // Chapter 9 design 4 closes this existing Chapter 8 door after the cave is destroyed.
                // Verify the closure, then retain every portal/direction check at its earlier open phase.
                if (map.id == "ch08_tianxing_fangshi" && portal.name == "portal_to_dongfu") {
                    GameState closed;
                    closed.flags = flags;
                    closed.setFlag("ch09.fengfu", 1);
                    const MapObject* guard = nullptr;
                    for (const auto& o : map.objects) if (o.name == "npc_tianxing_shouwei") guard = &o;
                    ASSERT_NE(guard, nullptr);
                    EXPECT_EQ(guard->property("visible_flag"), "ch09.fengfu");
                    EXPECT_TRUE(WorldScene::npcVisible(closed, *guard));
                    EXPECT_TRUE(approachesOf(map, portal, closed).empty());
                    flags["ch09.fengfu"] = 0;
                    GameState beforeSealing;
                    beforeSealing.flags = flags;
                    EXPECT_FALSE(WorldScene::npcVisible(beforeSealing, *guard));
                }

                GameState probe;
                probe.flags = flags;
                const std::vector<Approach> approaches = approachesOf(map, portal, probe);
                if (approaches.empty()) {
                    ADD_FAILURE() << "[" << setting.name << "] " << describePortal(map, portal)
                                  << " 一个进门方向都没有：门格四邻没有一格站得住，或门格本身走不上去"
                                     "——这道门玩家根本踩不上，是地图坏了";
                    continue;
                }
                ++walkedPortals;
                for (const Approach& approach : approaches) {
                    ++walks;
                    const std::string problem = walkThrough(map.id, portal, approach, flags);
                    if (problem.empty()) continue;
                    ADD_FAILURE() << "[" << setting.name << "] " << describePortal(map, portal) << "：站在 "
                                  << cellText(approach.stand) << " 朝" << approach.direction.name
                                  << "迈进门，" << problem;
                }
            }
        }
        EXPECT_EQ(walkedPortals, enumerated)
            << "[" << setting.name << "] 枚举到 " << enumerated << " 道门，实际走过的只有 " << walkedPortals
            << " 道（缺的那几道见上面「一个进门方向都没有」）";
        EXPECT_GT(walks, 0) << "[" << setting.name << "] 一趟都没走";
    }
}

}  // namespace
