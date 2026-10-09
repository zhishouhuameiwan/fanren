#include <gtest/gtest.h>
#include <map>
#include <set>
#include <string>
#include <vector>
#include "ChapterFixture.h"
#include "io/DataLoader.h"

namespace {
using namespace fanren;
core::MapObject object(const core::TileMap& map, const std::string& name) {
    for (const auto& o : map.objects) if (o.name == name) return o;
    return {};
}
bool matches(const core::MapObject& o, const char* mode, bool once, const char* guard, const char* done) {
    return o.type == "trigger" && o.property("mode") == mode && o.property("once") == (once ? "true" : "false") &&
           o.property("guard_flag") == guard && o.property("set_flag") == done && o.property("script").rfind("ch09/", 0) == 0;
}
TEST(Ch09TriggerMode, EveryObjectiveAndMainHookMatchesTheIndependentTwentyTwoStepTable) {
    const auto root = test::chapterNineAssetRoot();
    const auto data = io::loadGameData(root + "/data");
    ASSERT_TRUE(data.ok) << data.error;
    std::vector<core::Objective> objectives;
    for (const auto& o : data.value.objectives) if (o.chapter == 9) objectives.push_back(o);
    ASSERT_EQ(objectives.size(), test::kChapterNineSteps.size());
    std::map<std::string, core::TileMap> maps;
    for (std::size_t i = 0; i < test::kChapterNineSteps.size(); ++i) {
        const auto& s = test::kChapterNineSteps[i];
        SCOPED_TRACE(s.object);
        if (!maps.count(s.map)) {
            const auto map = io::loadTileMap(root + "/maps/" + s.map + ".tmj");
            ASSERT_TRUE(map.ok) << map.error;
            maps.emplace(s.map, map.value);
        }
        const char* guard = i == 0 ? "ch08.done" : test::kChapterNineSteps[i - 1].done;
        EXPECT_TRUE(matches(object(maps.at(s.map), s.object), s.enter ? "enter" : "interact", true, guard, s.done));
        EXPECT_EQ(objectives[i].doneFlag, s.done);
        EXPECT_EQ(objectives[i].targetMap, s.map);
        EXPECT_EQ(objectives[i].targetObject, s.object);
    }
}
TEST(Ch09TriggerMode, FourFlagsAndTheReusableTunnelWallHaveNoCompletionFlag) {
    const auto map = io::loadTileMap(test::chapterNineAssetRoot() + "/maps/ch08_lingkuang.tmj");
    ASSERT_TRUE(map.ok);
    for (const char* suffix : {"jin", "mu", "shui", "huo"}) {
        const auto o = object(map.value, std::string("trigger_zhenwei_jiu_") + suffix);
        EXPECT_TRUE(matches(o, "interact", false, "ch09.goucai", "")) << suffix;
        auto bad = o;
        bad.type = "portal";
        EXPECT_FALSE(matches(bad, "interact", false, "ch09.goucai", ""));
    }
    EXPECT_TRUE(matches(object(map.value, "trigger_suidao_hui"), "enter", false, "ch09.goucai", ""));
}
TEST(Ch09TriggerMode, DeparturesAndClosedRoadsHaveTheDesignPresenceFlags) {
    struct Spec { const char* map; const char* npc; const char* visible; const char* hidden; };
    const Spec specs[] = {
        {"ch08_dongfu", "npc_qu_hun", "ch08.huifu", "ch09.fengfu"},
        {"ch08_lingkuang", "npc_qu_hun_dongku", "ch09.goucai", "ch09.done"},
        {"ch08_jinmacheng", "npc_qi_yunxiao", "ch08.jinma", "ch08.yueding"},
        {"ch08_jinmacheng", "npc_xin_ruyin", "ch08.jiuren", "ch08.yueding"},
        {"ch09_wumingshan", "npc_xin_ruyin_jiu", "ch09.fujia", ""},
        {"ch08_tianxing_fangshi", "npc_tianxing_shouwei", "ch09.fengfu", ""},
    };
    for (const auto& s : specs) {
        const auto map = io::loadTileMap(test::chapterNineAssetRoot() + "/maps/" + s.map + ".tmj");
        ASSERT_TRUE(map.ok);
        const auto o = object(map.value, s.npc);
        ASSERT_FALSE(o.name.empty()) << s.npc;
        EXPECT_EQ(o.property("visible_flag"), s.visible) << s.npc;
        EXPECT_EQ(o.property("hidden_flag"), s.hidden) << s.npc;
    }
}
}  // namespace
