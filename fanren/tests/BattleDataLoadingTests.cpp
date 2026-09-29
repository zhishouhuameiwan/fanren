// 「破势与蓄劲」的数据字段怎么读进来（docs/octopath-battle.md 第 5 节）。
//
// 加载器的口径：字段都可选，**写了就得写对**，写错了报出文件与字段——
// 悄悄按缺省收下的话，一个拼错的「剑剑」会让那一格破绽永远亮不起来，而症状只是
// 「这一仗怎么打都破不了势」。每一条负向用例都配一条「同一份夹具只改回那一处就读得进来」
// 的先验：少了它，EXPECT_FALSE 可能只是因为夹具本身写坏了。
//
// 门禁（tools/validate.py 规则 24）查的是同一批字段，但它在编译之前跑、只看数据形状；
// 这里查的是 C++ 加载器自己的那一道——两道闸各管一段，谁也不替谁。
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

#include "TempDir.h"
#include "core/model/Types.h"
#include "io/BattleLoader.h"
#include "io/DataLoader.h"

namespace {

namespace fs = std::filesystem;

using fanren::core::GameData;
using fanren::test::TempDir;

void writeFile(const fs::path& path, const std::string& content) {
    fs::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out << content;
}

// 一个角色：基础字段写全，extra 拼进对象里。
fanren::core::Result<GameData> loadRole(const TempDir& tmp, const std::string& extra) {
    std::string json = R"({"id":"probe_role","name":"探针","maxHp":20,"attack":5,"defence":2,"speed":4)";
    if (!extra.empty()) json += "," + extra;
    writeFile(tmp.path() / "data" / "roles" / "probe_role.json", json + "}");
    return fanren::io::loadGameData((tmp.path() / "data").string());
}

fanren::core::Result<GameData> loadMagic(const TempDir& tmp, const std::string& extra) {
    std::string json = R"({"id":"probe_magic","name":"探针术","element":8,"needMp":4,"power":6)";
    if (!extra.empty()) json += "," + extra;
    writeFile(tmp.path() / "data" / "magics" / "probe_magic.json", json + "}");
    return fanren::io::loadGameData((tmp.path() / "data").string());
}

fanren::core::Result<GameData> loadItem(const TempDir& tmp, const std::string& extra) {
    std::string json = R"({"id":"probe_blade","name":"探针刀","kind":"artifact")";
    if (!extra.empty()) json += "," + extra;
    writeFile(tmp.path() / "data" / "items" / "probe_blade.json", json + "}");
    return fanren::io::loadGameData((tmp.path() / "data").string());
}

// ---------------------------------------------------------------------------
// 角色
// ---------------------------------------------------------------------------

TEST(BattleDataLoading, ARoleWithoutTheNewFieldsIsBareHandedWithoutToughness) {
    TempDir tmp{"fanren_break_role"};
    const auto loaded = loadRole(tmp, "");
    ASSERT_TRUE(loaded.ok) << loaded.error;
    const auto* role = loaded.value.findRole("probe_role");
    ASSERT_NE(role, nullptr);
    EXPECT_EQ(role->weapons, fanren::core::kCategoryFist) << "缺省空手：谁都打得出一拳";
    EXPECT_EQ(role->toughness, 0);
    EXPECT_EQ(role->weaknesses, 0);
    EXPECT_EQ(role->actions, 1);
    EXPECT_EQ(role->charge.every, 0) << "缺省不会蓄势";
}

TEST(BattleDataLoading, TheFieldsAreReadByTheirChineseNames) {
    TempDir tmp{"fanren_break_role"};
    const auto loaded = loadRole(
        tmp, R"("weapons":["剑","拳"],"toughness":6,"weaknesses":["毒","火"],"actions":2,)"
             R"("charge":{"every":3,"mult":2.5,"all":true,"text_key":"probe.key"})");
    ASSERT_TRUE(loaded.ok) << loaded.error;
    const auto* role = loaded.value.findRole("probe_role");
    ASSERT_NE(role, nullptr);
    EXPECT_EQ(role->weapons, fanren::core::kCategorySword | fanren::core::kCategoryFist);
    EXPECT_EQ(role->toughness, 6);
    EXPECT_EQ(role->weaknesses, fanren::core::kCategoryPoison | fanren::core::kCategoryFire);
    EXPECT_EQ(role->actions, 2);
    EXPECT_EQ(role->charge.every, 3);
    EXPECT_DOUBLE_EQ(role->charge.mult, 2.5);
    EXPECT_TRUE(role->charge.all);
    EXPECT_EQ(role->charge.textKey, "probe.key");
}

// 一份写坏的角色：必须读不进来，而且报错里写出是哪个字段、哪个文件。
void expectRoleRefused(const std::string& extra, const std::string& field) {
    TempDir tmp{"fanren_break_role_bad"};
    const auto loaded = loadRole(tmp, extra);
    EXPECT_FALSE(loaded.ok) << extra << " 被收下了";
    EXPECT_NE(loaded.error.find(field), std::string::npos) << "报错没点名字段 " << field << "：" << loaded.error;
    EXPECT_NE(loaded.error.find("probe_role.json"), std::string::npos) << "报错没点名文件：" << loaded.error;
}

TEST(BattleDataLoading, ABrokenCategoryListIsRefusedWithFileAndField) {
    // 先验：同一份夹具写对就读得进来。
    {
        TempDir sane{"fanren_break_role_sane"};
        ASSERT_TRUE(loadRole(sane, R"("toughness":2,"weaknesses":["剑"])").ok);
    }
    expectRoleRefused(R"("toughness":2,"weaknesses":["剑剑"])", "weaknesses");
    expectRoleRefused(R"("toughness":2,"weaknesses":["剑","剑"])", "weaknesses");
    expectRoleRefused(R"("toughness":2,"weaknesses":"剑")", "weaknesses");
    expectRoleRefused(R"("weapons":["火"])", "weapons");      // 兵刃只许兵刃五类
    expectRoleRefused(R"("weapons":[])", "weapons");          // 空手就不写
}

// 只有这几类要得了他的命（欧阳飞天：只有祭剑符的金）。缺省 0 = 什么都要得了命。
TEST(BattleDataLoading, KillableByIsReadAndAnEmptyOrMisspelledListIsRefused) {
    {
        TempDir tmp{"fanren_break_role_killable"};
        const auto loaded = loadRole(tmp, R"("toughness":2,"weaknesses":["金"],"killable_by":["金"])");
        ASSERT_TRUE(loaded.ok) << loaded.error;
        const auto* role = loaded.value.findRole("probe_role");
        ASSERT_NE(role, nullptr);
        EXPECT_EQ(role->killableBy, fanren::core::kCategoryMetal);
    }
    {
        TempDir tmp{"fanren_break_role_killable_default"};
        const auto loaded = loadRole(tmp, "");
        ASSERT_TRUE(loaded.ok) << loaded.error;
        EXPECT_EQ(loaded.value.findRole("probe_role")->killableBy, 0) << "不写 = 谁都杀得死";
    }
    expectRoleRefused(R"("killable_by":[])", "killable_by");      // 空数组 = 打不死
    expectRoleRefused(R"("killable_by":["金子"])", "killable_by");
}

TEST(BattleDataLoading, ToughnessAndActionsMustBeAtLeastOne) {
    expectRoleRefused(R"("toughness":0,"weaknesses":["剑"])", "toughness");
    expectRoleRefused(R"("toughness":"3","weaknesses":["剑"])", "toughness");
    expectRoleRefused(R"("toughness":3)", "破绽");            // 有架势却没有破绽
    expectRoleRefused(R"("actions":0)", "actions");
}

TEST(BattleDataLoading, AChargeMustBeWholeAndSensible) {
    {
        TempDir sane{"fanren_break_role_sane"};
        ASSERT_TRUE(loadRole(sane, R"("charge":{"every":2,"mult":1.5,"text_key":"k"})").ok)
            << "every 2、不写 all 也合法";
    }
    expectRoleRefused(R"("charge":{"every":1,"mult":2,"text_key":"k"})", "charge.every");
    expectRoleRefused(R"("charge":{"every":3,"mult":0,"text_key":"k"})", "charge.mult");
    expectRoleRefused(R"("charge":{"every":3,"mult":2})", "charge.text_key");
    expectRoleRefused(R"("charge":{"every":3,"mult":2,"all":"yes","text_key":"k"})", "charge.all");
}

// ---------------------------------------------------------------------------
// 法术与物品
// ---------------------------------------------------------------------------

TEST(BattleDataLoading, AMagicBoostsByPowerUnlessItSaysHits) {
    TempDir plain{"fanren_break_magic"};
    const auto a = loadMagic(plain, "");
    ASSERT_TRUE(a.ok) << a.error;
    EXPECT_EQ(a.value.findMagic("probe_magic")->boost, fanren::core::MagicBoost::Power);

    TempDir hits{"fanren_break_magic"};
    const auto b = loadMagic(hits, R"("boost":"hits")");
    ASSERT_TRUE(b.ok) << b.error;
    EXPECT_EQ(b.value.findMagic("probe_magic")->boost, fanren::core::MagicBoost::Hits);

    TempDir typo{"fanren_break_magic"};
    const auto c = loadMagic(typo, R"("boost":"hit")");
    EXPECT_FALSE(c.ok) << "拼错成 hit 的人本意是连发，悄悄按 power 收下，火弹术蓄满劲就只剩一发";
    EXPECT_NE(c.error.find("boost"), std::string::npos) << c.error;
    EXPECT_NE(c.error.find("probe_magic.json"), std::string::npos) << c.error;
}

TEST(BattleDataLoading, AWeaponItemGivesOneWeaponCategory) {
    TempDir ok{"fanren_break_item"};
    const auto loaded = loadItem(ok, R"("weapon":"刀")");
    ASSERT_TRUE(loaded.ok) << loaded.error;
    EXPECT_EQ(loaded.value.findItem("probe_blade")->weapon, fanren::core::kCategoryBlade);

    TempDir none{"fanren_break_item"};
    const auto plain = loadItem(none, "");
    ASSERT_TRUE(plain.ok) << plain.error;
    EXPECT_EQ(plain.value.findItem("probe_blade")->weapon, 0) << "没写就不是兵器";

    for (const char* bad : {R"("weapon":"火")", R"("weapon":["刀"])", R"("weapon":"大刀")"}) {
        TempDir tmp{"fanren_break_item_bad"};
        const auto refused = loadItem(tmp, bad);
        EXPECT_FALSE(refused.ok) << bad;
        EXPECT_NE(refused.error.find("weapon"), std::string::npos) << refused.error;
    }
}

// ---------------------------------------------------------------------------
// 战斗编成
// ---------------------------------------------------------------------------

fanren::core::Result<fanren::core::BattleSetup> loadBattleJson(const TempDir& tmp, const std::string& extra) {
    std::string json = R"({"id":"b_probe","name":"探针","chapter":4,"terrain":"field",)"
                       R"("units":[{"role_id":"wild_wolf","faction":"enemy"}])";
    if (!extra.empty()) json += "," + extra;
    const fs::path file = tmp.path() / "b_probe.json";
    writeFile(file, json + "}");
    return fanren::io::loadBattle(file.string());
}

TEST(BattleDataLoading, TheBackdropIsOptionalButMustBeAString) {
    TempDir none{"fanren_break_battle"};
    const auto plain = loadBattleJson(none, "");
    ASSERT_TRUE(plain.ok) << plain.error;
    EXPECT_TRUE(plain.value.backdrop.empty()) << "没写就按地形推（BattleScene::backdropOf）";

    TempDir named{"fanren_break_battle"};
    const auto set = loadBattleJson(named, R"("backdrop":"cave_dark")");
    ASSERT_TRUE(set.ok) << set.error;
    EXPECT_EQ(set.value.backdrop, "cave_dark");

    TempDir bad{"fanren_break_battle"};
    const auto refused = loadBattleJson(bad, R"("backdrop":3)");
    EXPECT_FALSE(refused.ok);
    EXPECT_NE(refused.error.find("backdrop"), std::string::npos) << refused.error;
}

}  // namespace
