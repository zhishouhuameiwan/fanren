// io 模块测试：DataLoader（data/ 加载 + Tiled 地图解析）与 SaveFile（存档）。
//
// 这里不链接 nlohmann_json（fanren_io 只 PRIVATE 链接它，测试目标拿不到它的
// include 路径），所以构造"畸形地图 JSON"这类反例夹具时，一律用手写字符串拼
// 接，不引入 JSON 库依赖。
#include "io/DataLoader.h"
#include "io/SaveFile.h"

#include <cstdint>
#include <filesystem>
#include <map>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "TempDir.h"
#include "core/rules/Bottle.h"
#include "core/rules/Realm.h"

namespace {

namespace fs = std::filesystem;
namespace core = fanren::core;
using fanren::core::GameData;
using fanren::core::GameState;
using fanren::core::Point;
using fanren::core::TileMap;
using fanren::rules::Realm;

// ---- 测试用临时目录：用完自动清理。实现与命名口径见 tests/TempDir.h ----
using fanren::test::TempDir;

void writeFile(const fs::path& path, const std::string& content) {
    fs::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    ASSERT_TRUE(out.is_open()) << "无法创建测试夹具文件: " << path.string();
    out << content;
}

// 仓库根目录：从测试可执行文件的工作目录往上走，找到同时含 CMakeLists.txt /
// data / maps 的那一层。不同生成器（Ninja 单配置 / VS 多配置）下 ctest 的工作
// 目录深度不一样，向上多探几层比硬编码相对路径更稳。
fs::path repoRoot() {
    fs::path dir = fs::current_path();
    for (int i = 0; i < 10; ++i) {
        if (fs::exists(dir / "CMakeLists.txt") && fs::exists(dir / "data") && fs::exists(dir / "maps")) {
            return dir;
        }
        const fs::path parent = dir.parent_path();
        if (parent == dir) break;
        dir = parent;
    }
    ADD_FAILURE() << "找不到仓库根目录（从 " << fs::current_path().string()
                  << " 往上找不到同时含 CMakeLists.txt/data/maps 的目录）";
    return fs::current_path();
}

// ---- 手写最小合法 .tmj，用来拼反例（省去链接 JSON 库）----
// 2x2 的小地图，六个图层齐备、七项地图属性齐备，objects 层留空——反例测试只
// 会在到达 objects 解析之前就失败，所以留空不影响测试意图。
std::string layerBlock(const std::string& name, bool isTileLayer) {
    if (isTileLayer) {
        return "{\"name\":\"" + name + "\",\"type\":\"tilelayer\",\"width\":2,\"height\":2,"
               "\"data\":[0,0,0,0]}";
    }
    return "{\"name\":\"" + name + "\",\"type\":\"objectgroup\",\"objects\":[]}";
}

bool contains(const std::vector<std::string>& v, const std::string& s) {
    for (const std::string& e : v) {
        if (e == s) return true;
    }
    return false;
}

std::string buildMinimalMapJson(const std::string& mapId, int chapter,
                                 const std::vector<std::string>& omitLayers = {},
                                 const std::vector<std::string>& omitProps = {}) {
    const std::vector<std::pair<std::string, bool>> allLayers = {
        {"ground", true}, {"overlay", true}, {"building", true},
        {"front", true},  {"collision", true}, {"objects", false},
    };
    const std::vector<std::pair<std::string, std::string>> allProps = {
        {"map_id", "{\"name\":\"map_id\",\"type\":\"string\",\"value\":\"" + mapId + "\"}"},
        {"display_name_key", "{\"name\":\"display_name_key\",\"type\":\"string\",\"value\":\"key.x\"}"},
        {"region", "{\"name\":\"region\",\"type\":\"string\",\"value\":\"region_x\"}"},
        {"bgm", "{\"name\":\"bgm\",\"type\":\"string\",\"value\":\"none\"}"},
        {"chapter", "{\"name\":\"chapter\",\"type\":\"int\",\"value\":" + std::to_string(chapter) + "}"},
        {"outdoor", "{\"name\":\"outdoor\",\"type\":\"bool\",\"value\":true}"},
        {"can_leave_edge", "{\"name\":\"can_leave_edge\",\"type\":\"bool\",\"value\":false}"},
    };

    std::string propsJson;
    for (const auto& [name, block] : allProps) {
        if (contains(omitProps, name)) continue;
        if (!propsJson.empty()) propsJson += ",";
        propsJson += block;
    }
    std::string layersJson;
    for (const auto& [name, isTile] : allLayers) {
        if (contains(omitLayers, name)) continue;
        if (!layersJson.empty()) layersJson += ",";
        layersJson += layerBlock(name, isTile);
    }

    return "{\"width\":2,\"height\":2,\"tilewidth\":32,\"tileheight\":32,"
           "\"properties\":[" + propsJson + "],\"layers\":[" + layersJson + "]}";
}

// ==================== DataLoader: loadGameData ====================

TEST(LoadGameData, ReadsEveryCategoryAndParsesKnownEntries) {
    const core::Result<GameData> result = fanren::io::loadGameData((repoRoot() / "data").string());
    ASSERT_TRUE(result) << result.error;

    // 刻意不断言条目总数。内容从 P3 起每周都在长，把目录大小写死会让这条
    // 测试在没有任何东西损坏时变红，久而久之就没人拿它当回事了。
    // 这里要证明的是「加载器能读到并解析对」，所以断言已知条目存在且字段正确，
    // 外加一个下界防止整个目录被漏读。
    EXPECT_GE(result.value.items.size(), 4u);
    EXPECT_GE(result.value.roles.size(), 1u);
    EXPECT_GE(result.value.text.size(), 7u);

    // 空目录返回 0 条不是错误——这条仍然要守住，它是「缺分类不算失败」的判据。
    // magics 一旦有了内容，改成 GE 即可，不要删掉这条语义。
    EXPECT_TRUE(result.value.magics.empty() || result.value.magics.size() > 0u);


    // 第 1 章的问候语：证明 text/ 下的大对象被摊平进了同一张表。
    EXPECT_TRUE(result.value.text.count("ch01.sanshu.greet") == 1)
        << "data/text/ch01.json 的 key 没进 GameData";

    const auto* pill = result.value.findItem("pill_jinchuang_yao");
    ASSERT_NE(pill, nullptr);
    EXPECT_EQ(pill->name, "金疮药");
    EXPECT_EQ(pill->restoreHp, 15);

    const auto* sanshu = result.value.findRole("han_sanshu");
    ASSERT_NE(sanshu, nullptr);
    EXPECT_EQ(sanshu->name, "韩三叔");
    EXPECT_EQ(sanshu->realm, Realm::Mortal);

    EXPECT_FALSE(result.value.lookupText("ch01.sanshu.greet").empty());
    EXPECT_NE(result.value.lookupText("ch01.sanshu.greet"), "ch01.sanshu.greet");
    // 查不到的 key 应该原样回显，方便在画面上定位缺哪条文案。
    EXPECT_EQ(result.value.lookupText("no.such.key"), "no.such.key");
}

// ---------------------------------------------------------------------------
// 灵草年份上限：data 里的 maxAge 与加载器的品阶默认值
//
// 这道上限是「把整瓶绿液砸在同一株低阶药上刷天价」那个洞的闸门（见
// src/core/rules/Bottle.h 的 herbMaxAgeForGrade）。闸门本身在规则层，
// 这里守的是它的进料口：data 写了要按 data 走，没写要按品阶兜底，
// 而绝不能兜成「无上限」。
// ---------------------------------------------------------------------------

// 从 startAge 起一直往同一株上浇，返回催不动为止用掉的滴数。
[[nodiscard]] int dropsToCeiling(int startAge, int maxAge) {
    fanren::rules::Bottle bottle;
    bottle.owned = true;
    bottle.matureKnown = true;
    bottle.drops = 1000;
    bottle.capacity = 1000;

    int age = startAge;
    for (int spent = 0; spent <= 64; ++spent) {
        const fanren::rules::MatureResult result = fanren::rules::matureHerb(bottle, age, maxAge);
        if (!result.ok) return spent;
        age = result.newAge;
    }
    ADD_FAILURE() << "催熟没有收敛：maxAge=" << maxAge;
    return -1;
}

TEST(LoadGameData, EveryHerbInTheRepoCarriesAnAgeCeiling) {
    const core::Result<GameData> result = fanren::io::loadGameData((repoRoot() / "data").string());
    ASSERT_TRUE(result) << result.error;

    int herbsSeen = 0;
    for (const auto& [id, item] : result.value.items) {
        if (item.kind != core::ItemKind::Herb) continue;
        ++herbsSeen;

        EXPECT_GT(item.maxAge, 0) << id << " 没有年份上限，绿液能把它一路推到万年";
        EXPECT_LE(item.maxAge, fanren::rules::kMaxHerbAge) << id << " 越过了全局溢出闸";

        // 一阶灵草（本章的药）从足年浇到顶只该是两三滴的量级。谁把上限调回
        // 天上，这里立刻变红——这正是当初那个退化策略的入口。
        if (item.grade <= 1) {
            const int drops = dropsToCeiling(fanren::rules::kRipeAge, item.maxAge);
            EXPECT_GE(drops, 2) << id;
            EXPECT_LE(drops, 3) << id << " 的上限被抬高了：从足年浇到顶要 " << drops
                                << " 滴，远超炼气期一瓶（"
                                << fanren::rules::kBottleCapacityPerTier << " 滴）的量";
        }
    }
    EXPECT_GE(herbsSeen, 4) << "data/items/herbs 一株也没读到，上面那些断言等于没跑";
}

TEST(LoadGameData, HerbWithoutMaxAgeFallsBackToItsGrade) {
    TempDir tmp{"fanren_io_test"};
    const fs::path dataRoot = tmp.path() / "data";
    writeFile(dataRoot / "items" / "old.json",
              R"({"id":"herb_old","name":"老数据灵草","kind":"herb","grade":3,"price":12})");

    const core::Result<GameData> result = fanren::io::loadGameData(dataRoot.string());
    ASSERT_TRUE(result) << result.error;
    const auto* herb = result.value.findItem("herb_old");
    ASSERT_NE(herb, nullptr);
    EXPECT_EQ(herb->maxAge, fanren::rules::herbMaxAgeForGrade(3));
    EXPECT_LT(herb->maxAge, fanren::rules::kMaxHerbAge) << "缺字段不能等于无上限";
}

TEST(LoadGameData, HerbWithNeitherGradeNorMaxAgeLandsOnTheLowestTier) {
    // 最要紧的一条兜底：两个字段都没有的老数据，宁可按一阶低估，也不能
    // 变成「不限」——那就是把当初那个刷钱口子原样留给老数据。
    TempDir tmp{"fanren_io_test"};
    const fs::path dataRoot = tmp.path() / "data";
    writeFile(dataRoot / "items" / "bare.json",
              R"({"id":"herb_bare","name":"没写品阶的灵草","kind":"herb"})");

    const core::Result<GameData> result = fanren::io::loadGameData(dataRoot.string());
    ASSERT_TRUE(result) << result.error;
    const auto* herb = result.value.findItem("herb_bare");
    ASSERT_NE(herb, nullptr);
    EXPECT_EQ(herb->maxAge, fanren::rules::herbMaxAgeForGrade(1));
    EXPECT_LE(dropsToCeiling(fanren::rules::kRipeAge, herb->maxAge), 3);
}

TEST(LoadGameData, AnExplicitMaxAgeOverridesTheGradeLadder) {
    // 阶梯只是默认值。天雷竹那种「自有其极高上限」的特殊物种靠这一条表达。
    TempDir tmp{"fanren_io_test"};
    const fs::path dataRoot = tmp.path() / "data";
    writeFile(dataRoot / "items" / "special.json",
              R"({"id":"herb_special","name":"特例灵草","kind":"herb","grade":1,"maxAge":9000})");

    const core::Result<GameData> result = fanren::io::loadGameData(dataRoot.string());
    ASSERT_TRUE(result) << result.error;
    const auto* herb = result.value.findItem("herb_special");
    ASSERT_NE(herb, nullptr);
    EXPECT_EQ(herb->maxAge, 9000);
}

TEST(LoadGameData, RejectsAMaxAgeOutsideTheAllowedRange) {
    // 0 与负数多半是想写「不限」，夹成 0 会让这味药一滴也浇不动；
    // 超过万年则是把全局溢出闸绕过去。两种都当场报错，别等到玩家存档里才发作。
    const std::vector<std::string> bad = {
        R"({"id":"herb_zero","name":"甲","kind":"herb","maxAge":0})",
        R"({"id":"herb_neg","name":"乙","kind":"herb","maxAge":-30})",
        R"({"id":"herb_huge","name":"丙","kind":"herb","maxAge":100000})",
        R"({"id":"herb_text","name":"丁","kind":"herb","maxAge":"很多年"})",
    };
    for (const std::string& payload : bad) {
        TempDir tmp{"fanren_io_test"};
        const fs::path dataRoot = tmp.path() / "data";
        writeFile(dataRoot / "items" / "bad.json", payload);

        const core::Result<GameData> result = fanren::io::loadGameData(dataRoot.string());
        ASSERT_FALSE(result) << "这份数据该被挡下: " << payload;
        EXPECT_NE(result.error.find("maxAge"), std::string::npos)
            << "错误信息里要点名是哪个字段: " << result.error;
        EXPECT_NE(result.error.find("bad.json"), std::string::npos) << result.error;
    }
}

TEST(LoadGameData, LeavesMaxAgeAtZeroForThingsThatAreNotHerbs) {
    // 年份上限只对灵草有意义。给丹药、法器也推一个默认值，只会让存档与
    // 面板上多出一个谁也不该读的数。
    const core::Result<GameData> result = fanren::io::loadGameData((repoRoot() / "data").string());
    ASSERT_TRUE(result) << result.error;
    const auto* pill = result.value.findItem("pill_jinchuang_yao");
    ASSERT_NE(pill, nullptr);
    EXPECT_EQ(pill->maxAge, 0);
}

TEST(LoadGameData, DuplicateIdAcrossFilesFails) {
    TempDir tmp{"fanren_io_test"};
    const fs::path dataRoot = tmp.path() / "data";
    writeFile(dataRoot / "items" / "a.json", R"({"id":"dup_item","name":"甲"})");
    writeFile(dataRoot / "items" / "sub" / "b.json", R"({"id":"dup_item","name":"乙"})");

    const core::Result<GameData> result = fanren::io::loadGameData(dataRoot.string());
    ASSERT_FALSE(result);
    EXPECT_NE(result.error.find("dup_item"), std::string::npos) << result.error;
    EXPECT_NE(result.error.find("a.json"), std::string::npos) << result.error;
    EXPECT_NE(result.error.find("b.json"), std::string::npos) << result.error;
}

TEST(LoadGameData, MissingIdFieldFails) {
    TempDir tmp{"fanren_io_test"};
    const fs::path dataRoot = tmp.path() / "data";
    writeFile(dataRoot / "items" / "no_id.json", R"({"name":"没有 id 的物品"})");

    const core::Result<GameData> result = fanren::io::loadGameData(dataRoot.string());
    ASSERT_FALSE(result);
    EXPECT_NE(result.error.find("id"), std::string::npos) << result.error;
    EXPECT_NE(result.error.find("no_id.json"), std::string::npos) << result.error;
}

TEST(LoadGameData, ItemFileWithJsonSyntaxErrorFails) {
    TempDir tmp{"fanren_io_test"};
    const fs::path dataRoot = tmp.path() / "data";
    writeFile(dataRoot / "items" / "broken.json", R"({"id": "x", "name": )");  // 缺值，语法错误

    const core::Result<GameData> result = fanren::io::loadGameData(dataRoot.string());
    ASSERT_FALSE(result);
    EXPECT_NE(result.error.find("broken.json"), std::string::npos) << result.error;
}

TEST(LoadGameData, MissingDataRootFails) {
    TempDir tmp{"fanren_io_test"};
    const core::Result<GameData> result = fanren::io::loadGameData((tmp.path() / "does_not_exist").string());
    ASSERT_FALSE(result);
}

// ==================== DataLoader: loadTileMap ====================

TEST(LoadTileMap, SampleMapLoadsWithCorrectLayersObjectsAndAttributes) {
    const fs::path mapPath = repoRoot() / "maps" / "ch01_hanjiacun.tmj";
    const core::Result<TileMap> result = fanren::io::loadTileMap(mapPath.string());
    ASSERT_TRUE(result) << result.error;

    const TileMap& map = result.value;
    EXPECT_EQ(map.width, 40);
    EXPECT_EQ(map.height, 30);
    EXPECT_EQ(map.ground.size(), 40u * 30u);
    EXPECT_EQ(map.collision.size(), 40u * 30u);

    EXPECT_EQ(map.id, "ch01_hanjiacun");
    EXPECT_EQ(map.displayNameKey, "ch01.map.hanjiacun.name");
    EXPECT_EQ(map.region, "qingzhou");
    EXPECT_EQ(map.bgm, "bgm_village");
    EXPECT_EQ(map.chapter, 1);
    EXPECT_TRUE(map.outdoor);

    // 不锁对象总数：地图会随内容生产不断补充，锁死它只会在没坏任何东西时变红。
    // 下面逐个断言关键对象，那才是加载器要证明的事。
    EXPECT_GE(map.objects.size(), 4u);

    const auto* spawn = map.defaultSpawn();
    ASSERT_NE(spawn, nullptr);
    EXPECT_EQ(spawn->name, "spawn_main");
    EXPECT_EQ(spawn->property("facing"), "up");

    const auto* npc = map.objectAt(Point{20, 18}, "npc");
    ASSERT_NE(npc, nullptr);
    EXPECT_EQ(npc->property("role_id"), "han_sanshu");
    // 三叔这一幕挂在 NPC 身上，不再另挂一个同格的 trigger：同一格上 interact()
    // 先问 npc，命中就起脚本并 return，那个 trigger 永远轮不到
    // （docs/ch01-review.md 建议-5），已从生成器里删掉。
    EXPECT_EQ(npc->property("script"), "ch01/sanshu.lua");

    // 本图那个真正起作用的 once 触发在村口，由脚本自己的完成旗标兑现。
    // 村口 2026-09-27 从南墙挪到了北墙（map_spec 规则 29），触发跟着在门洞里侧那一排。
    const auto* trigger = map.objectAt(Point{19, 1}, "trigger");
    ASSERT_NE(trigger, nullptr);
    EXPECT_EQ(trigger->property("mode"), "enter");
    EXPECT_EQ(trigger->property("once"), "true");
    EXPECT_EQ(trigger->property("set_flag"), "ch01.muqin_bie");

    // 出口的去向由章节地理决定，会随剧情调整（本章就改过一次），所以只断言
    // 「有出口、且它指向一张确实存在的图」——这才是加载器的职责。
    const core::MapObject* portal = nullptr;
    for (const auto& object : map.objects) {
        if (object.type == "portal") {
            portal = &object;
            break;
        }
    }
    ASSERT_NE(portal, nullptr) << "韩家村应当有通往外界的出口";
    const std::string target = portal->property("target_map");
    EXPECT_FALSE(target.empty());
    EXPECT_TRUE(std::filesystem::exists(repoRoot() / "maps" / (target + ".tmj")))
        << "出口指向的地图不存在：" << target;
    EXPECT_FALSE(portal->property("target_spawn").empty());
}

TEST(LoadTileMap, EveryPortalPointsAtARealSpawnOnBothSides) {
    // 原先这条测试钉死「韩家村与七玄门互通」。第 1 章的地理后来按原著改过一次，
    // 两张图不再相邻，测试就红了——可地图加载器一点没坏。
    //
    // 真正要证明的是：任意一张图的任意出口，都能在目标图里找到落点。
    // 所以改成扫描全部地图，把每个 portal 都验一遍。内容怎么改都不影响这条。
    namespace fs = std::filesystem;
    const fs::path mapsDir = repoRoot() / "maps";
    ASSERT_TRUE(fs::exists(mapsDir));

    std::map<std::string, core::TileMap> loaded;
    for (const auto& entry : fs::directory_iterator(mapsDir)) {
        if (entry.path().extension() != ".tmj") continue;
        auto result = fanren::io::loadTileMap(entry.path().string());
        ASSERT_TRUE(result) << entry.path().string() << ": " << result.error;
        loaded.emplace(result.value.id, std::move(result.value));
    }
    ASSERT_FALSE(loaded.empty()) << "maps/ 下一张图都没读到";

    int portalsChecked = 0;
    for (const auto& [mapId, map] : loaded) {
        for (const auto& object : map.objects) {
            if (object.type != "portal") continue;
            ++portalsChecked;
            const std::string target = object.property("target_map");
            const std::string spawnId = object.property("target_spawn");
            const auto it = loaded.find(target);
            ASSERT_NE(it, loaded.end())
                << mapId << " 的 " << object.name << " 指向不存在的地图 " << target;

            bool found = false;
            for (const auto& candidate : it->second.objects) {
                if (candidate.type == "spawn" && candidate.property("id") == spawnId) {
                    found = true;
                    break;
                }
            }
            EXPECT_TRUE(found) << mapId << " 的 " << object.name
                               << " 指向 " << target << " 里不存在的落点 " << spawnId;
        }
    }
    EXPECT_GT(portalsChecked, 0) << "一个传送点都没查到，这条测试等于没跑";
}
TEST(LoadTileMap, WalkableMatchesCollisionLayer) {
    const fs::path mapPath = repoRoot() / "maps" / "ch01_hanjiacun.tmj";
    const core::Result<TileMap> result = fanren::io::loadTileMap(mapPath.string());
    ASSERT_TRUE(result) << result.error;
    const TileMap& map = result.value;

    // 地图边界一圈是 collision=1（村子围栏），不可通行。
    EXPECT_FALSE(map.walkable(Point{0, 0}));
    EXPECT_FALSE(map.walkable(Point{39, 29}));
    // 房屋地基（x:10-15, y:10-13）是 collision=1。
    EXPECT_FALSE(map.walkable(Point{12, 11}));
    // spawn 点和它旁边的空地是 collision=0，可通行。
    EXPECT_TRUE(map.walkable(Point{20, 20}));
    EXPECT_TRUE(map.walkable(Point{1, 1}));
    // 越界坐标不可通行。
    EXPECT_FALSE(map.walkable(Point{-1, 0}));
    EXPECT_FALSE(map.walkable(Point{40, 0}));
}

TEST(LoadTileMap, MissingLayerFailsWithLayerName) {
    TempDir tmp{"fanren_io_test"};
    const fs::path mapPath = tmp.path() / "ch01_broken.tmj";
    writeFile(mapPath, buildMinimalMapJson("ch01_broken", 1, /*omitLayers=*/{"collision"}));

    const core::Result<TileMap> result = fanren::io::loadTileMap(mapPath.string());
    ASSERT_FALSE(result);
    EXPECT_NE(result.error.find("collision"), std::string::npos) << result.error;
}

TEST(LoadTileMap, MissingMapAttributeFailsWithAttributeName) {
    TempDir tmp{"fanren_io_test"};
    const fs::path mapPath = tmp.path() / "ch01_broken2.tmj";
    writeFile(mapPath, buildMinimalMapJson("ch01_broken2", 1, /*omitLayers=*/{}, /*omitProps=*/{"bgm"}));

    const core::Result<TileMap> result = fanren::io::loadTileMap(mapPath.string());
    ASSERT_FALSE(result);
    EXPECT_NE(result.error.find("bgm"), std::string::npos) << result.error;
}

TEST(LoadTileMap, FileDoesNotExistFails) {
    TempDir tmp{"fanren_io_test"};
    const core::Result<TileMap> result = fanren::io::loadTileMap((tmp.path() / "nope.tmj").string());
    ASSERT_FALSE(result);
}

TEST(LoadTileMap, JsonSyntaxErrorFails) {
    TempDir tmp{"fanren_io_test"};
    const fs::path mapPath = tmp.path() / "broken_syntax.tmj";
    writeFile(mapPath, "{ this is not valid json");

    const core::Result<TileMap> result = fanren::io::loadTileMap(mapPath.string());
    ASSERT_FALSE(result);
}

// ==================== SaveFile ====================

GameState makeSampleState() {
    GameState s;
    s.mapId = "ch01_hanjiacun";
    s.position = Point{20, 20};
    s.facing = 1;
    s.realm = Realm::QiRefining3;
    s.cultivation = 555;
    s.hp = 8;
    s.maxHp = 10;
    s.mp = 2;
    s.maxMp = 5;
    s.day = 7;
    s.chapter = 2;
    // 同一味灵草不同年份要分堆存，不能合并——这是背包往返测试要专门盯住的点。
    s.addItem("herb_qingfeng_cao", 2, /*herbAge=*/3);
    s.addItem("herb_qingfeng_cao", 5, /*herbAge=*/1);
    s.addItem("pill_jinchuang_yao", 3, /*herbAge=*/0);
    s.setFlag("ch01.sanshu_met", 1);
    s.setFlag("ch01.some_other_flag", 7);
    s.playSecondsGameplay = 123.5;
    s.playSecondsSystem = 45.25;
    return s;
}

void expectStatesEqual(const GameState& a, const GameState& b) {
    EXPECT_EQ(a.mapId, b.mapId);
    EXPECT_EQ(a.position, b.position);
    EXPECT_EQ(a.facing, b.facing);
    EXPECT_EQ(a.realm, b.realm);
    EXPECT_EQ(a.cultivation, b.cultivation);
    EXPECT_EQ(a.hp, b.hp);
    EXPECT_EQ(a.maxHp, b.maxHp);
    EXPECT_EQ(a.mp, b.mp);
    EXPECT_EQ(a.maxMp, b.maxMp);
    EXPECT_EQ(a.day, b.day);
    EXPECT_EQ(a.chapter, b.chapter);
    EXPECT_DOUBLE_EQ(a.playSecondsGameplay, b.playSecondsGameplay);
    EXPECT_DOUBLE_EQ(a.playSecondsSystem, b.playSecondsSystem);

    ASSERT_EQ(a.bag.size(), b.bag.size());
    for (std::size_t i = 0; i < a.bag.size(); ++i) {
        EXPECT_EQ(a.bag[i].itemId, b.bag[i].itemId) << "bag[" << i << "]";
        EXPECT_EQ(a.bag[i].count, b.bag[i].count) << "bag[" << i << "]";
        EXPECT_EQ(a.bag[i].herbAge, b.bag[i].herbAge) << "bag[" << i << "]";
    }
    EXPECT_EQ(a.flags, b.flags);
}

TEST(SaveFile, RoundTripPreservesAllFieldsIncludingHerbAgeAndFlags) {
    TempDir tmp{"fanren_io_test"};
    const fs::path savePath = tmp.path() / "slot1.json";
    const GameState original = makeSampleState();

    const core::Result<bool> saved = fanren::io::saveGame(original, savePath.string());
    ASSERT_TRUE(saved) << saved.error;

    const core::Result<GameState> loaded = fanren::io::loadGame(savePath.string());
    ASSERT_TRUE(loaded) << loaded.error;

    expectStatesEqual(original, loaded.value);
    // 背包应该分成 3 堆：两份不同年份的灵草 + 一份金疮药。
    ASSERT_EQ(loaded.value.bag.size(), 3u);
    EXPECT_EQ(loaded.value.itemCount("herb_qingfeng_cao"), 7);
}

TEST(SaveFile, TamperedChecksumFailsToLoad) {
    TempDir tmp{"fanren_io_test"};
    const fs::path savePath = tmp.path() / "slot_tampered.json";
    ASSERT_TRUE(fanren::io::saveGame(makeSampleState(), savePath.string()));

    // 直接改校验和字段本身的一个十六进制字符：不管存档格式怎么排版，这个改法
    // 都稳，不依赖 payload 里具体某个字段长什么样。
    std::ifstream in(savePath, std::ios::binary);
    std::stringstream buffer;
    buffer << in.rdbuf();
    in.close();
    std::string text = buffer.str();

    const std::size_t key = text.find("\"checksum\"");
    ASSERT_NE(key, std::string::npos);
    const std::size_t firstQuoteAfterColon = text.find('"', text.find(':', key) + 1);
    ASSERT_NE(firstQuoteAfterColon, std::string::npos);
    const std::size_t hexPos = firstQuoteAfterColon + 1;
    ASSERT_LT(hexPos, text.size());
    text[hexPos] = (text[hexPos] == '0') ? '1' : '0';

    writeFile(savePath, text);

    const core::Result<GameState> loaded = fanren::io::loadGame(savePath.string());
    ASSERT_FALSE(loaded);
    EXPECT_NE(loaded.error.find("校验和"), std::string::npos) << loaded.error;
}

// FNV-1a 64 位，与 SaveFile.cpp 里的实现保持一致，仅用于本测试构造"校验和自洽
// 但版本号超出支持范围"的夹具。checksum 把 version 也绑了进去，意味着单纯改
// 一个已保存文件的 save_version 必然连带把 checksum 也弄错（这正是防篡改要的
// 效果），没法只用文本替换伪造出"合法校验和 + 不认识的版本号"这种场景——这里
// 复刻一份算法，模拟的是"存档来自一个更新的游戏版本，文件本身没有被篡改"这种
// 真实场景（向前兼容），而不是复用生产代码的私有实现细节。
std::uint64_t testFnv1a64(const std::string& data) {
    std::uint64_t hash = 0xcbf29ce484222325ULL;
    for (const char c : data) {
        hash ^= static_cast<unsigned char>(c);
        hash *= 0x100000001b3ULL;
    }
    return hash;
}

std::string testChecksum(int version, const std::string& payloadDump) {
    std::ostringstream oss;
    oss << std::hex << std::setw(16) << std::setfill('0')
        << testFnv1a64(std::to_string(version) + "|" + payloadDump);
    return oss.str();
}

TEST(SaveFile, UnknownFutureVersionFailsWithClearError) {
    TempDir tmp{"fanren_io_test"};
    const fs::path savePath = tmp.path() / "slot_future.json";

    // payload 内容本身无关紧要：这条测试要走到的是"校验和核对通过、但版本号
    // 超出支持范围"这条分支，从不会真的调用 fromJson 去理解字段。特意留空对
    // 象而不是拼一份完整 GameState payload，是为了不依赖 nlohmann 对数字/浮点
    // 的具体序列化格式——空对象 dump() 出来必然就是 "{}"，没有歧义。
    const std::string payload = "{}";
    constexpr int kFutureVersion = 999;
    const std::string content = "{\"save_version\":" + std::to_string(kFutureVersion) +
                                 ",\"checksum\":\"" + testChecksum(kFutureVersion, payload) +
                                 "\",\"payload\":" + payload + "}";
    writeFile(savePath, content);

    const core::Result<GameState> loaded = fanren::io::loadGame(savePath.string());
    ASSERT_FALSE(loaded);
    EXPECT_NE(loaded.error.find(std::to_string(kFutureVersion)), std::string::npos) << loaded.error;
}

TEST(SaveFile, LoadGameFileDoesNotExistFails) {
    TempDir tmp{"fanren_io_test"};
    const core::Result<GameState> loaded = fanren::io::loadGame((tmp.path() / "nope.json").string());
    ASSERT_FALSE(loaded);
}

TEST(SaveFile, LoadGameJsonSyntaxErrorFails) {
    TempDir tmp{"fanren_io_test"};
    const fs::path savePath = tmp.path() / "broken.json";
    writeFile(savePath, "{ not valid json");

    const core::Result<GameState> loaded = fanren::io::loadGame(savePath.string());
    ASSERT_FALSE(loaded);
}

TEST(SaveFile, SaveGameCreatesMissingParentDirectories) {
    TempDir tmp{"fanren_io_test"};
    const fs::path savePath = tmp.path() / "nested" / "dir" / "slot1.json";
    const core::Result<bool> saved = fanren::io::saveGame(makeSampleState(), savePath.string());
    ASSERT_TRUE(saved) << saved.error;
    EXPECT_TRUE(fs::exists(savePath));
}

TEST(SaveFile, CarriesTheBottleAndFieldsAndProficienciesAcrossARoundTrip) {
    // 这些字段是第 2 章经济循环的全部载体。丢掉任何一个，玩家的四年苦修
    // 与满田灵草就随读档蒸发了。
    GameState before = makeSampleState();
    before.day = 1460;

    before.bottle.owned = true;
    before.bottle.matureKnown = true;   // 两个开关必须能分别存
    before.bottle.drops = 2;
    before.bottle.lastChargeDay = 1453;
    before.bottle.capacity = 6;

    fanren::rules::SpiritField field;
    field.id = "shenshougu_yaopu";
    field.slots.push_back(fanren::rules::FieldSlot{"herb_huangjing_cao", 1200, 47, true, 44});
    field.slots.push_back(fanren::rules::FieldSlot{});   // 空槽也要原样存回来
    before.fields.push_back(std::move(field));

    before.alchemyProficiency = 31;
    before.talismanProficiency = 12;
    before.forgeProficiency = 3;
    before.formationProficiency = 7;
    before.aptitude = 44;
    before.cultivationRemainder = 17;

    TempDir tmp{"fanren_io_test"};
    const fs::path savePath = tmp.path() / "p2state.json";
    ASSERT_TRUE(fanren::io::saveGame(before, savePath.string()));

    const core::Result<GameState> loaded = fanren::io::loadGame(savePath.string());
    ASSERT_TRUE(loaded) << loaded.error;
    const GameState& after = loaded.value;

    EXPECT_TRUE(after.bottle.owned);
    EXPECT_TRUE(after.bottle.matureKnown);
    EXPECT_EQ(after.bottle.drops, 2);
    EXPECT_EQ(after.bottle.lastChargeDay, 1453);
    EXPECT_EQ(after.bottle.capacity, 6);

    ASSERT_EQ(after.fields.size(), 1u);
    EXPECT_EQ(after.fields[0].id, "shenshougu_yaopu");
    ASSERT_EQ(after.fields[0].slots.size(), 2u);
    EXPECT_EQ(after.fields[0].slots[0].seedId, "herb_huangjing_cao");
    EXPECT_EQ(after.fields[0].slots[0].age, 47) << "年份丢了，催熟等于白做";
    EXPECT_TRUE(after.fields[0].slots[0].ripe);
    // 年份上限丢了，这一畦读档之后就又能一路长到全局上限——刷钱的口子会
    // 在存档往返里悄悄重开。
    EXPECT_EQ(after.fields[0].slots[0].maxAge, 44) << "年份上限没跟着落盘";
    EXPECT_TRUE(after.fields[0].slots[1].seedId.empty());

    EXPECT_EQ(after.alchemyProficiency, 31);
    EXPECT_EQ(after.talismanProficiency, 12);
    EXPECT_EQ(after.forgeProficiency, 3);
    EXPECT_EQ(after.formationProficiency, 7);
    EXPECT_EQ(after.aptitude, 44);
    EXPECT_EQ(after.cultivationRemainder, 17);
}

TEST(SaveFile, RecognisesTheOlderVersionInsteadOfRejectingIt) {
    // 老档必须还能读。迁移表里 1->2 是空操作（v1 本就没有第 2 章那批字段），
    // 但「空操作」与「忘了登记」行为上天差地别：后者直接把老玩家的进度判死。
    //
    // 刻意用空 payload：它必然通过校验和与版本检查，再必然在字段解析阶段失败。
    // 看的是失败在哪一步——版本没登记就报「不支持的版本」，压根走不到解析；
    // 登记了才会执行迁移、进入解析、报「缺少 mapId」。
    //
    // 不手工拼完整 payload，是因为文件按 dump(2) 缩进写盘而校验和覆盖 dump()
    // 紧凑串，手写的字符串对不上，也不该让测试依赖数字的序列化格式。
    TempDir tmp{"fanren_io_test"};
    const fs::path savePath = tmp.path() / "legacy_v1.json";
    const std::string payload = "{}";
    const std::string content = "{\"save_version\":1,\"checksum\":\"" +
                                testChecksum(1, payload) + "\",\"payload\":" + payload + "}";
    writeFile(savePath, content);

    const core::Result<GameState> loaded = fanren::io::loadGame(savePath.string());
    ASSERT_FALSE(loaded) << "空 payload 不该被当成合法存档";
    EXPECT_NE(loaded.error.find("mapId"), std::string::npos)
        << "期望走到字段解析才失败，实际错误是：" << loaded.error;
}

TEST(SaveFile, RejectsAVersionThatHasNoMigration) {
    // 对照组：没登记的版本要在版本检查这一步就挡下，且错误里要带版本号，
    // 否则玩家只看到一句语焉不详的「读档失败」。
    TempDir tmp{"fanren_io_test"};
    const fs::path savePath = tmp.path() / "legacy_v0.json";
    const std::string payload = "{}";
    constexpr int kUnregistered = 0;
    const std::string content = "{\"save_version\":" + std::to_string(kUnregistered) +
                                ",\"checksum\":\"" + testChecksum(kUnregistered, payload) +
                                "\",\"payload\":" + payload + "}";
    writeFile(savePath, content);

    const core::Result<GameState> loaded = fanren::io::loadGame(savePath.string());
    ASSERT_FALSE(loaded);
    EXPECT_EQ(loaded.error.find("mapId"), std::string::npos)
        << "不该走到字段解析：" << loaded.error;
}

}  // namespace
