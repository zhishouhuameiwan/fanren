#include <gtest/gtest.h>
#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <vector>
#include "ChapterFixture.h"
#include "io/DataLoader.h"
#include "game/WorldScene.h"

namespace {
using namespace fanren;
using core::MapObject;
using core::TileMap;

MapObject find(const TileMap& map, const std::string& name) {
    for (const auto& o : map.objects) if (o.name == name) return o;
    return {};
}
std::vector<std::string> hookProblems(const MapObject& o, const char* mode, bool once,
                                    const std::string& guard, const std::string& done) {
    std::vector<std::string> problems;
    if (o.type != "trigger") problems.push_back("type");
    if (o.property("mode") != mode) problems.push_back("mode");
    if (o.property("once") != (once ? "true" : "false")) problems.push_back("once");
    if (o.property("guard_flag") != guard) problems.push_back("guard");
    if (o.property("set_flag") != done) problems.push_back("done");
    if (o.property("script").rfind("ch08/", 0) != 0) problems.push_back("script");
    return problems;
}

TEST(Ch08TriggerMode, EveryMainHookAndObjectiveMatchesTheDesignTable) {
    const auto root = test::chapterEightAssetRoot();
    std::map<std::string, TileMap> maps;
    const auto data = io::loadGameData(root + "/data");
    ASSERT_TRUE(data.ok) << data.error;
    std::vector<core::Objective> objectives;
    for (const auto& o : data.value.objectives) if (o.chapter == 8) objectives.push_back(o);
    ASSERT_EQ(objectives.size(), test::kChapterEightSteps.size());
    for (std::size_t i = 0; i < test::kChapterEightSteps.size(); ++i) {
        const auto& s = test::kChapterEightSteps[i];
        SCOPED_TRACE(s.object);
        if (!maps.count(s.map)) {
            const auto loaded = io::loadTileMap(root + "/maps/" + s.map + ".tmj");
            ASSERT_TRUE(loaded.ok) << loaded.error;
            maps.emplace(s.map, loaded.value);
        }
        const auto o = find(maps.at(s.map), s.object);
        ASSERT_FALSE(o.name.empty());
        const std::string guard = i == 0 ? "ch07.done" : i == 42 ? "ch08.fengwu" : test::kChapterEightSteps[i - 1].done;
        const bool once = i != 42;
        EXPECT_TRUE(hookProblems(o, s.enter ? "enter" : "interact", once, guard, once ? s.done : "").empty());
        const auto& objective = objectives[i];
        EXPECT_EQ(objective.doneFlag, s.done);
        EXPECT_EQ(objective.targetMap, s.map);
        EXPECT_EQ(objective.targetObject, s.object);
    }
}

TEST(Ch08TriggerMode, AllTwelveArrayPositionsAndReusableBoardsHaveNoCompletionFlag) {
    const auto root = test::chapterEightAssetRoot();
    for (int group = 1; group <= 3; ++group) {
        const std::string mapId = group == 1 ? "ch08_dongfu" : group == 2 ? "ch08_yuejing" : "ch08_jiayuan_shanlin";
        const auto map = io::loadTileMap(root + "/maps/" + mapId + ".tmj");
        ASSERT_TRUE(map.ok) << map.error;
        const std::string guard = group == 1 ? "ch08.quqi" : group == 2 ? "ch08.qb_yuanbing" : "ch08.quhun";
        const char* first[] = {"jin", "mu", "shui", "huo"};
        for (int slot = 0; slot < 4; ++slot) {
            const std::string name = group == 1 ? std::string("trigger_zhenwei_") + first[slot]
                : std::string(group == 2 ? "trigger_zhulin_" : "trigger_milin_") + std::to_string(slot + 1);
            SCOPED_TRACE(name);
            EXPECT_TRUE(hookProblems(find(map.value, name), "interact", false, guard, "").empty());
        }
    }
    const auto map = io::loadTileMap(root + "/maps/ch08_jinguyuan.tmj");
    ASSERT_TRUE(map.ok);
    for (const char* name : {"trigger_zhanbao", "trigger_liesha"})
        EXPECT_TRUE(hookProblems(find(map.value, name), "interact", false, "ch08.yinian", "").empty()) << name;
}

TEST(Ch08TriggerMode, ContractProbeCatchesABrokenRealHook) {
    const auto map = io::loadTileMap(test::chapterEightAssetRoot() + "/maps/ch08_dongfu.tmj");
    ASSERT_TRUE(map.ok);
    auto o = find(map.value, "trigger_zhenwei_jin");
    ASSERT_TRUE(hookProblems(o, "interact", false, "ch08.quqi", "").empty());
    o.type = "portal";
    EXPECT_FALSE(hookProblems(o, "interact", false, "ch08.quqi", "").empty());
}
}  // namespace
