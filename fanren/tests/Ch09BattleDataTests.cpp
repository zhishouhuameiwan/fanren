#include <gtest/gtest.h>
#include <set>
#include <string>
#include "ChapterFixture.h"
#include "core/model/AttackCategory.h"
#include "io/BattleLoader.h"
#include "io/DataLoader.h"

namespace {
using namespace fanren;
TEST(Ch09BattleData, ThreeRequiredFightsHaveTheDesignEnemiesAndNoEscapeOrItemRewards) {
    struct Spec { const char* id; std::multiset<std::string> enemies; };
    const Spec specs[] = {
        {"b09_tuwei", {"tianshazong_xiushi", "moyanmen_xiushi", "modao_guitou"}},
        {"b09_fujia", {"fujia_laozhe", "fujia_qingnian"}},
        {"b09_dong_xuaner", {"dong_xuaner_hehuan"}},
    };
    for (const auto& s : specs) {
        const auto loaded = io::loadBattle(test::chapterNineAssetRoot() + "/data/battles/" + s.id + ".json");
        ASSERT_TRUE(loaded.ok) << loaded.error;
        const auto accepts = [&s](const core::BattleSetup& b) {
            std::multiset<std::string> enemies;
            for (const auto& u : b.units) {
                if (u.ally || u.wave != 0) return false;
                enemies.insert(u.roleId);
            }
            return !b.canEscape && b.defeatIsFatal && !b.heroAbsent && b.reward.spiritStones == 0 &&
                   b.reward.drops.empty() && enemies == s.enemies;
        };
        EXPECT_TRUE(accepts(loaded.value)) << s.id;
        auto bad = loaded.value;
        bad.canEscape = true;
        EXPECT_FALSE(accepts(bad));
        bad = loaded.value;
        bad.reward.spiritStones = 1;
        EXPECT_FALSE(accepts(bad));
        bad = loaded.value;
        bad.units.pop_back();
        EXPECT_FALSE(accepts(bad));
    }
}
TEST(Ch09BattleData, TheCoinsHaveOnlyAHiddenWeaponWeaknessAndTheNewBarrierHasWaterAndMetal) {
    const auto data = io::loadGameData(test::chapterNineAssetRoot() + "/data");
    ASSERT_TRUE(data.ok) << data.error;
    ASSERT_TRUE(data.value.roles.count("fujia_laozhe"));
    const auto& elder = data.value.roles.at("fujia_laozhe");
    EXPECT_EQ(elder.weaknesses, core::categoryFromName("暗器"));
    EXPECT_EQ(elder.toughness, 6);
    EXPECT_EQ(elder.actions, 1);
    EXPECT_EQ(elder.charge.every, 0);
    ASSERT_TRUE(data.value.roles.count("dong_xuaner_hehuan"));
    const auto& barrier = data.value.roles.at("dong_xuaner_hehuan");
    const int elements = core::categoryFromName("水") | core::categoryFromName("金");
    EXPECT_EQ(barrier.weaknesses & elements, elements);
    EXPECT_EQ(barrier.toughness, 7);
    EXPECT_EQ(barrier.actions, 1);
    EXPECT_EQ(barrier.killableBy, 0) << "new artifacts are useful, never mandatory";
    ASSERT_TRUE(data.value.magics.count("magic_qingyuan_jianmang"));
    EXPECT_EQ(data.value.magics.at("magic_qingyuan_jianmang").needRealm, rules::Realm::FoundationEarly);
    for (const char* id : {"magic_ji_hongxianzhen", "magic_ji_baizhudao"}) {
        ASSERT_TRUE(data.value.magics.count(id));
        EXPECT_EQ(data.value.magics.at(id).needRealm, rules::Realm::QiRefining1);
    }
}
}  // namespace
