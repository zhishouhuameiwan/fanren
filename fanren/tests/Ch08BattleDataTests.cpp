#include <gtest/gtest.h>
#include <set>
#include <string>
#include <vector>
#include "ChapterFixture.h"
#include "core/model/AttackCategory.h"
#include "io/BattleLoader.h"
#include "io/DataLoader.h"

namespace {
using namespace fanren;
struct Spec {
    const char* id;
    std::multiset<std::pair<std::string, int>> enemies;
    std::set<std::string> allies;
    bool fatal;
};
const std::vector<Spec> kSpecs = {
    {"b08_guiling_shaozhu", {{"xue_gui", 0}, {"xue_gui", 0}, {"xue_gui", 0}, {"guiling_shaozhu", 1}}, {}, true},
    {"b08_lingkuang_shouzhen", {{"tianshazong_xiushi", 0}, {"tianshazong_xiushi", 0}, {"moyanmen_xiushi", 0},
        {"tianshazong_toumu", 1}, {"moyanmen_xiushi", 1}}, {"xuan_le", "lv_tianmeng"}, true},
    {"b08_zhongrudong", {{"xuan_le", 0}, {"xue_zhizhu", 1}}, {}, true},
    {"b08_jinguyuan_juezhan", {{"guilingmen_xiushi", 0}, {"guilingmen_xiushi", 0}, {"moyanmen_xiushi", 0},
        {"tianshazong_toumu", 1}, {"tianshazong_xiushi", 1}, {"tianshazong_xiushi", 1}},
        {"song_meng", "huangfenggu_zhuji_xiushi"}, false},
    {"b08_yuehuang", {{"yue_huang", 0}}, {"chen_qiaoqian", "zhong_weiniang"}, true},
    {"b08_quhun", {{"yulingzong_canhun", 0}}, {}, true},
};
std::vector<std::string> problems(const core::BattleSetup& b, const Spec& s) {
    std::multiset<std::pair<std::string, int>> enemies;
    std::set<std::string> allies;
    for (const auto& u : b.units) {
        if (u.ally) allies.insert(u.roleId); else enemies.emplace(u.roleId, u.wave);
    }
    std::vector<std::string> out;
    if (b.canEscape) out.push_back("escape");
    if (b.defeatIsFatal != s.fatal) out.push_back("fatal");
    if (b.reward.spiritStones || !b.reward.drops.empty()) out.push_back("reward");
    if (enemies != s.enemies) out.push_back("enemies");
    if (allies != s.allies) out.push_back("allies");
    return out;
}
TEST(Ch08BattleData, SixRequiredCompositionsHaveTheDesignWavesAlliesAndSettlement) {
    const auto root = test::chapterEightAssetRoot();
    for (const auto& s : kSpecs) {
        SCOPED_TRACE(s.id);
        const auto b = io::loadBattle(root + "/data/battles/" + s.id + ".json");
        ASSERT_TRUE(b.ok) << b.error;
        EXPECT_TRUE(problems(b.value, s).empty());
        auto bad = b.value;
        bad.canEscape = true;
        EXPECT_FALSE(problems(bad, s).empty());
        bad = b.value;
        bad.reward.spiritStones = 1;
        EXPECT_FALSE(problems(bad, s).empty());
        bad = b.value;
        bad.units.pop_back();
        EXPECT_FALSE(problems(bad, s).empty());
    }
}
TEST(Ch08BattleData, FormationExposesExactlyFiveElementsAndRestrictsTheEmperorToOneAction) {
    const auto data = io::loadGameData(test::chapterEightAssetRoot() + "/data");
    ASSERT_TRUE(data.ok) << data.error;
    ASSERT_TRUE(data.value.roles.count("yue_huang"));
    const auto& emperor = data.value.roles.at("yue_huang");
    int five = 0;
    for (const char* category : {"金", "木", "水", "火", "土"}) five |= core::categoryFromName(category);
    EXPECT_EQ(emperor.weaknesses, five);
    EXPECT_EQ(emperor.actions, 1);
    EXPECT_EQ(emperor.toughness, 7);
    for (const char* id : {"xuan_le", "xue_zhizhu", "yulingzong_canhun"}) {
        ASSERT_TRUE(data.value.roles.count(id)) << id;
        EXPECT_EQ(data.value.roles.at(id).killableBy, 0) << id << " no consumable-only kill gate";
    }
}
}  // namespace
