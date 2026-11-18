// 伙伴改版 · 手绘索引带非人形敌人（docs/interfaces-companions.md 2.5 末条、第 6 节「四路交回之后」）。
//
//   · 敌人图多了按表的 scale / battle_scale，缺省是改版前写死的 ×3 / ×4（真树里生成的敌人图一张都不变）；
//   · hd/index.json 可带 enemies：同 id 覆盖、没有的新增，file 补 hd/ 前缀，必须自己写两个倍数；
//   · 世界层画明雷（WorldProps 的 enemyDest / enemyHeadTop）与战斗层取外观（BattleView 的 battleLookFor）都读表上的数。
// 读索引走 io::loadSpriteIndex——世界画面、主菜单、战斗画面三处读精灵索引的那一个入口。
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

#include "TempDir.h"
#include "core/model/Visual.h"
#include "game/BattleView.h"
#include "game/WorldProps.h"
#include "io/VisualLoader.h"

namespace {

namespace fs = std::filesystem;

fs::path repoRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "assets" / "art" / "sprites" / "index.json")) return candidate;
    }
    return ".";
}

void writeFile(const fs::path& path, const std::string& text) {
    fs::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out << text;
}

// 生成索引：一张人物表、一只狼（16×24 的旧美术）。
constexpr const char* kIndex = R"({"sheets": {"boy": {"file": "boy.png",
    "walk": {"down": [0,1,2], "left": [3,4,5], "right": [6,7,8], "up": [9,10,11]}}},
  "enemies": {"wild_wolf": {"file": "enemies/wild_wolf.png", "kind": "beast", "frame_w": 32, "frame_h": 24,
    "cols": 5, "foot_y": 22, "head_y": 3, "battle": {"idle": [0, 1], "attack": [2]}}}})";

// 手绘：墨蛟（新增，大图）、狼（覆盖）。
constexpr const char* kHd = R"({"sheets": {}, "files": {},
  "enemies": {
    "mo_jiao": {"file": "mo_jiao.png", "kind": "beast", "frame_w": 160, "frame_h": 96, "cols": 4,
                "foot_y": 90, "head_y": 6, "scale": 1, "battle_scale": 2, "battle": {"idle": [0, 1], "attack": [2]}},
    "wild_wolf": {"file": "wolf.png", "kind": "beast", "frame_w": 64, "frame_h": 48, "cols": 4,
                  "foot_y": 44, "head_y": 4, "scale": 2, "battle_scale": 2, "battle": {"idle": [0]}}}})";

fs::path spritesDir(const fanren::test::TempDir& dir, const std::string& hd) {
    const fs::path sprites = dir.path() / "art" / "sprites";
    writeFile(sprites / "index.json", kIndex);
    if (!hd.empty()) writeFile(sprites / "hd" / "index.json", hd);
    return sprites / "index.json";
}

const fanren::game::ChapterDone kNever = [](int) { return false; };

}  // namespace

TEST(HdEnemies, GeneratedEnemySheetsKeepTheOldTimesThreeAndTimesFour) {
    fanren::test::TempDir dir("fanren_hd_enemy_default");
    const auto index = fanren::io::loadSpriteIndex(spritesDir(dir, "").string());
    ASSERT_TRUE(index.ok) << index.error;
    const auto& wolf = index.value.enemies.at("wild_wolf");
    EXPECT_EQ(wolf.scale, 3);
    EXPECT_EQ(wolf.battleScale, 4);
    // 真树里生成的那些：一张都不变。
    const auto real = fanren::io::loadSpriteIndex((repoRoot() / "assets/art/sprites/index.json").string());
    ASSERT_TRUE(real.ok) << real.error;
    ASSERT_FALSE(real.value.enemies.empty());
    for (const auto& [id, enemy] : real.value.enemies) {
        if (enemy.file.rfind("hd/", 0) == 0) continue;   // 手绘的由美术路提交，自己写倍数
        EXPECT_EQ(enemy.scale, 3) << id;
        EXPECT_EQ(enemy.battleScale, 4) << id;
    }
}

TEST(HdEnemies, TheHdIndexOverridesAndAddsEnemiesWithTheirOwnScales) {
    fanren::test::TempDir dir("fanren_hd_enemy");
    const auto index = fanren::io::loadSpriteIndex(spritesDir(dir, kHd).string());
    ASSERT_TRUE(index.ok) << index.error;
    const auto& jiao = index.value.enemies.at("mo_jiao");
    EXPECT_EQ(jiao.file, "hd/mo_jiao.png") << "新增的，file 补 hd/ 前缀";
    EXPECT_EQ(jiao.scale, 1);
    EXPECT_EQ(jiao.battleScale, 2);
    const auto& wolf = index.value.enemies.at("wild_wolf");
    EXPECT_EQ(wolf.file, "hd/wolf.png") << "同 id 的覆盖生成的那一张";
    EXPECT_EQ(wolf.frameW, 64);
    EXPECT_EQ(wolf.scale, 2);
}

TEST(HdEnemies, AnHdEnemyMustWriteBothScales) {
    for (const char* missing : {"scale", "battle_scale"}) {
        std::string hd = kHd;
        const std::string key = std::string("\"") + missing + "\": ";
        // 把墨蛟那一条里的这一项改名成认不得的键（等于没写）。
        const auto at = hd.find(key);
        ASSERT_NE(at, std::string::npos);
        hd.replace(at, key.size(), std::string("\"x_") + missing + "\": ");
        fanren::test::TempDir dir("fanren_hd_enemy_bad");
        const auto index = fanren::io::loadSpriteIndex(spritesDir(dir, hd).string());
        EXPECT_FALSE(index.ok) << missing;
        if (!index.ok) EXPECT_NE(index.error.find(missing), std::string::npos) << index.error;
    }
}

TEST(HdEnemies, TheWorldLayerDrawsAFoeAtItsSheetsScale) {
    fanren::test::TempDir dir("fanren_hd_enemy_world");
    const auto index = fanren::io::loadSpriteIndex(spritesDir(dir, kHd).string());
    ASSERT_TRUE(index.ok) << index.error;
    // 明雷本体：look 当敌人 id 查到那张图。
    const auto body = fanren::game::foeBodyFor(index.value, "mo_jiao", kNever);
    ASSERT_NE(body.enemy, nullptr);
    const auto dest = fanren::game::enemyDest(*body.enemy, 100.f, 200.f);
    EXPECT_EQ(dest.w, 160.f) << "×1";
    EXPECT_EQ(dest.h, 96.f);
    EXPECT_EQ(dest.y, 200.f - 91.f) << "脚底（第 90 行的下沿）落在 footY";
    EXPECT_EQ(fanren::game::enemyHeadTop(*body.enemy, 200.f), 200.f - (91.f - 6.f));

    fanren::test::TempDir plain("fanren_hd_enemy_world_plain");
    const auto old = fanren::io::loadSpriteIndex(spritesDir(plain, "").string());
    ASSERT_TRUE(old.ok) << old.error;
    const auto& wolf = old.value.enemies.at("wild_wolf");
    EXPECT_EQ(fanren::game::enemyDest(wolf, 100.f, 200.f).w, 32.f * 3.f) << "生成的照旧 ×3";
}

TEST(HdEnemies, TheBattleLayerTakesTheEnemySheetsBattleScale) {
    fanren::test::TempDir dir("fanren_hd_enemy_battle");
    const auto index = fanren::io::loadSpriteIndex(spritesDir(dir, kHd).string());
    ASSERT_TRUE(index.ok) << index.error;
    const auto look = fanren::game::battleLookFor(index.value, "b_probe", "mo_jiao", kNever);
    EXPECT_EQ(look.file, "hd/mo_jiao.png");
    EXPECT_EQ(look.scale, 2);
    EXPECT_EQ(fanren::game::battleScale(look.scale, false), 2.f);

    fanren::test::TempDir plain("fanren_hd_enemy_battle_plain");
    const auto old = fanren::io::loadSpriteIndex(spritesDir(plain, "").string());
    ASSERT_TRUE(old.ok) << old.error;
    const auto wolf = fanren::game::battleLookFor(old.value, "b_probe", "wild_wolf", kNever);
    EXPECT_EQ(wolf.scale, 4);
    EXPECT_EQ(fanren::game::battleScale(wolf.scale, false), 4.f) << "生成的照旧 ×4";
    EXPECT_EQ(fanren::game::battleScale(wolf.scale, true), 5.f) << "首领照旧 ×5";
}
