// 同道录 2a·路 K：三个加载器认新字段（docs/ch07-tongdaolu-k.md 第 1 节）。
//   * 伙伴表第二版：kind / line / bond_cap / roster_bonus / spar，roles[i] 下的 magics / passive / duo；
//     表外字段照旧报错，种类表外的被动报错；
//   * 法术：hits、amount、四种辅助效果；
//   * 编成：spar 与它的几条硬规矩；
//   * 真树：两份现有的表迁到新形状，行为不变（没有新法术、没有被动）。
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "TempDir.h"
#include "core/model/Types.h"
#include "core/rules/Tongdao.h"
#include "io/BattleLoader.h"
#include "io/CompanionLoader.h"
#include "io/DataLoader.h"

namespace {

namespace fs = std::filesystem;
using fanren::core::BondLine;
using fanren::core::CompanionKind;
using fanren::core::MagicEffect;
using fanren::core::PerkKind;
using fanren::test::TempDir;

fs::path repoRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "data" / "text" / "ch01_main.json")) return candidate;
    }
    return ".";
}

void writeFile(const fs::path& path, const std::string& text) {
    fs::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out << text;
}

// 接口文档里那份完整示例（docs/ch07-tongdaolu-k.md 1.2），一字不差。
constexpr const char* kFullExample = R"({
  "id": "chen_qiaoqian",
  "kind": "person",
  "line": "hongyan",
  "roles": [
    {
      "role": "chen_shimei",
      "until_flag": "ch08.some_flag",
      "magics": [
        {"tier": 3, "magic": "magic_shuangjian"},
        {"tier": 4, "magic": "magic_jianmu"}
      ],
      "passive": {"kind": "chase_power", "values": [20, 30]},
      "duo": "magic_duo_jianhe"
    },
    {"role": "chen_qiaoqian"}
  ],
  "met_flag": "ch07.fanhui",
  "bond_cap": {"until_flag": "ch07.done", "max_tier": 4},
  "field_skill": "see_through_illusion",
  "chase": true,
  "duo": "magic_duo_old",
  "gifts": {"like": ["material_huolong_cao"], "dislike": ["material_yaoshou_cai"]},
  "roster_bonus": {"kind": "ambush_bp", "value": 1},
  "spar": {
    "battle": "spar_chen_qiaoqian",
    "prizes": [
      {"tier": 2, "items": [{"item": "pill_huiqi_dan", "count": 2}]},
      {"tier": 3, "items": [{"item": "talisman_feixing_fu"}]}
    ]
  },
  "note": "示例"
})";

fanren::core::Result<fanren::core::Companion> loadText(const std::string& text) {
    TempDir dir{"fanren_tongdao_loader"};
    writeFile(dir.path() / "c.json", text);
    return fanren::io::loadCompanionFile((dir.path() / "c.json").string());
}

TEST(TongdaoCompanionLoader, ReadsEveryFieldOfTheFullExample) {
    const auto loaded = loadText(kFullExample);
    ASSERT_TRUE(loaded.ok) << loaded.error;
    const fanren::core::Companion& c = loaded.value;
    EXPECT_EQ(c.kind, CompanionKind::Person);
    EXPECT_EQ(c.line, BondLine::Hongyan);
    EXPECT_EQ(c.capFlag, "ch07.done");
    EXPECT_EQ(c.capTier, 4);
    ASSERT_EQ(c.roles.size(), 2u);
    const fanren::core::CompanionRole& r = c.roles[0];
    ASSERT_EQ(r.tierMagics.size(), 2u);
    EXPECT_EQ(r.tierMagics[0].tier, 3);
    EXPECT_EQ(r.tierMagics[0].magicId, "magic_shuangjian");
    EXPECT_EQ(r.tierMagics[1].tier, 4);
    EXPECT_EQ(r.passive.kind, PerkKind::ChasePower);
    EXPECT_EQ(r.passive.value, 20);
    EXPECT_EQ(r.passive.value5, 30);
    EXPECT_EQ(r.duo, "magic_duo_jianhe");
    EXPECT_TRUE(c.roles[1].tierMagics.empty()) << "第 8 章起的那一段不写";
    EXPECT_EQ(c.roles[1].passive.kind, PerkKind::None);
    EXPECT_EQ(c.duo, "magic_duo_old");
    EXPECT_EQ(c.rosterBonus.kind, PerkKind::AmbushBp);
    EXPECT_EQ(c.rosterBonus.value, 1);
    EXPECT_EQ(c.sparBattle, "spar_chen_qiaoqian");
    ASSERT_EQ(c.sparPrizes.size(), 2u);
    EXPECT_EQ(c.sparPrizes[0].tier, 2);
    ASSERT_EQ(c.sparPrizes[0].items.size(), 1u);
    EXPECT_EQ(c.sparPrizes[0].items[0].itemId, "pill_huiqi_dan");
    EXPECT_EQ(c.sparPrizes[0].items[0].count, 2);
    EXPECT_EQ(c.sparPrizes[1].items[0].count, 1) << "count 缺省 1";
}

TEST(TongdaoCompanionLoader, AnOldStyleFileReadsWithTheOldBehaviour) {
    const auto loaded = loadText(R"({"id": "a", "met_flag": "f", "roles": [{"role": "r"}], "duo": "d"})");
    ASSERT_TRUE(loaded.ok) << loaded.error;
    EXPECT_EQ(loaded.value.kind, CompanionKind::Person);
    EXPECT_EQ(loaded.value.line, BondLine::General);
    EXPECT_EQ(loaded.value.capTier, 0);
    EXPECT_EQ(loaded.value.rosterBonus.kind, PerkKind::None);
    EXPECT_TRUE(loaded.value.sparBattle.empty());
    EXPECT_TRUE(loaded.value.roles[0].tierMagics.empty());
    EXPECT_TRUE(loaded.value.roles[0].duo.empty());
}

TEST(TongdaoCompanionLoader, ABeastDefaultsToTheBeastLine) {
    const auto loaded = loadText(R"({"id": "b", "kind": "beast", "met_flag": "f", "roles": [{"role": "r"}],
        "roster_bonus": {"kind": "talisman_power", "value": 10}})");
    ASSERT_TRUE(loaded.ok) << loaded.error;
    EXPECT_EQ(loaded.value.kind, CompanionKind::Beast);
    EXPECT_EQ(loaded.value.line, BondLine::Beast);
}

TEST(TongdaoCompanionLoader, RejectsBadShapesAndPerksOutsideTheTable) {
    const std::string head = R"({"id": "a", "met_flag": "f", )";
    const std::string roles = R"("roles": [{"role": "r"}])";
    struct Bad {
        std::string text;
        const char* needle;
    };
    const std::vector<Bad> bads = {
        {head + roles + R"(, "kind": "spirit"})", "kind"},
        {head + roles + R"(, "line": "romance"})", "line"},
        {head + roles + R"(, "kind": "beast", "line": "hongyan"})", "line"},
        {head + roles + R"(, "line": "beast"})", "line"},
        {head + roles + R"(, "bond_cap": {"until_flag": "ch07.done", "max_tier": 5}})", "bond_cap"},
        {head + roles + R"(, "bond_cap": {"max_tier": 4}})", "bond_cap"},
        {head + roles + R"(, "bond_cap": {"until_flag": "x", "max_tier": 4, "why": 1}})", "bond_cap"},
        {head + roles + R"(, "roster_bonus": {"kind": "stun_all", "value": 1}})", "stun_all"},
        {head + roles + R"(, "roster_bonus": {"kind": "chase_power", "value": 10}})", "在册加成"},
        {head + roles + R"(, "roster_bonus": {"kind": "ambush_bp", "value": 3}})", "value"},
        {head + roles + R"(, "roster_bonus": {"kind": "ambush_bp"}})", "value"},
        {head + roles + R"(, "roster_bonus": {"kind": "element_stagger", "value": 1}})", "element"},
        {head + roles + R"(, "roster_bonus": {"kind": "element_stagger", "value": 1, "element": "ice"}})", "element"},
        {head + roles + R"(, "roster_bonus": {"kind": "ambush_bp", "value": 1, "element": "fire"}})", "element"},
        {head + roles + R"(, "roster_bonus": {"kind": "ambush_bp", "values": [1]}})", "values"},
        {head + R"("roles": [{"role": "r", "passive": {"kind": "ambush_bp", "values": [1]}}]})", "伙伴被动"},
        {head + R"("roles": [{"role": "r", "passive": {"kind": "chase_power", "values": [30, 20]}}]})", "五档"},
        {head + R"("roles": [{"role": "r", "passive": {"kind": "chase_power", "values": []}}]})", "values"},
        {head + R"("roles": [{"role": "r", "passive": {"kind": "chase_power", "values": [40]}}]})", "values"},
        {head + R"("roles": [{"role": "r", "passive": {"kind": "chase_power", "value": 20}}]})", "value"},
        {head + R"("roles": [{"role": "r", "magics": [{"tier": 1, "magic": "m"}]}]})", "tier"},
        {head + R"("roles": [{"role": "r", "magics": [{"tier": 3}]}]})", "magic"},
        {head + R"("roles": [{"role": "r", "magics": "m"}]})", "magics"},
        {head + R"("roles": [{"role": "r", "skills": []}]})", "skills"},
        {head + roles + R"(, "spar": {"prizes": []}})", "battle"},
        {head + roles + R"(, "spar": {"battle": "s", "prizes": [{"tier": 3, "items": [{"item": "x"}]},
                                                               {"tier": 2, "items": [{"item": "y"}]}]}})", "升序"},
        {head + roles + R"(, "spar": {"battle": "s", "prizes": [{"tier": 2, "items": []}]}})", "items"},
        {head + roles + R"(, "spar": {"battle": "s", "prizes": [{"tier": 2, "items": [{"item": "x", "count": 0}]}]}})",
         "count"},
        {head + roles + R"(, "kind": "beast", "spar": {"battle": "s"}})", "灵兽"},
        {head + roles + R"(, "romance": true})", "romance"},
    };
    for (const Bad& bad : bads) {
        const auto loaded = loadText(bad.text);
        EXPECT_FALSE(loaded.ok) << bad.text;
        EXPECT_NE(loaded.error.find(bad.needle), std::string::npos) << bad.needle << " ← " << loaded.error;
    }
}

TEST(TongdaoRepoData, TheTwoCompanionsMigratedWithoutNewSpellsOrPassives) {
    const auto loaded = fanren::io::loadCompanions((repoRoot() / "data" / "companions").string());
    ASSERT_TRUE(loaded.ok) << loaded.error;
    const auto& companions = loaded.value;
    ASSERT_TRUE(companions.count("chen_qiaoqian") && companions.count("nangong_wan"));
    const auto& chen = companions.at("chen_qiaoqian");
    EXPECT_EQ(chen.kind, CompanionKind::Person);
    EXPECT_EQ(chen.line, BondLine::Hongyan) << "红颜线（施工图 3.2）";
    EXPECT_EQ(chen.capTier, 0);
    const auto& wan = companions.at("nangong_wan");
    EXPECT_EQ(wan.line, BondLine::General) << "第 7 章是少女：通用档名";
    EXPECT_EQ(wan.capFlag, "ch07.done");
    EXPECT_EQ(wan.capTier, 4) << "用户 2026-11-21 定：封在四档";
    EXPECT_EQ(wan.duo, "magic_duo_huanhuo") << "合招照旧挂在顶层：第 8 章起也一样";
    // 路 N 填数之前：不加法术、不加被动、没有在册加成与切磋——战斗行为不变。路 N 填了之后这一条要跟着改。
    for (const auto* c : {&chen, &wan}) {
        for (const auto& role : c->roles) {
            EXPECT_TRUE(role.tierMagics.empty()) << c->id << " / " << role.roleId;
            EXPECT_EQ(role.passive.kind, PerkKind::None) << c->id << " / " << role.roleId;
            EXPECT_TRUE(role.duo.empty()) << c->id << " / " << role.roleId;
        }
        EXPECT_EQ(c->rosterBonus.kind, PerkKind::None) << c->id;
        EXPECT_TRUE(c->sparBattle.empty()) << c->id;
    }
}

TEST(TongdaoRepoData, EveryTongdaoFlagForTheThreePeopleIsRegistered) {
    std::ifstream in(repoRoot() / "data" / "flags.json", std::ios::binary);
    const std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    for (const char* id : {"chen_qiaoqian", "nangong_wan", "han_yunzhi"}) {
        for (const std::string& flag :
             {fanren::rules::qingjiaoFlag(id), fanren::rules::xuhuaFlag(id), fanren::rules::sparDayFlag(id),
              fanren::rules::sparPrizeFlag(id), fanren::rules::bonusOnFlag(id)}) {
            EXPECT_NE(text.find("\"" + flag + "\""), std::string::npos) << flag;
        }
    }
    for (const char* flag : {fanren::rules::kTongxiuDayFlag, fanren::rules::kTongxiuBonusDayFlag}) {
        EXPECT_NE(text.find("\"" + std::string(flag) + "\""), std::string::npos) << flag;
    }
}

// ---------------------------------------------------------------------------
// 法术
// ---------------------------------------------------------------------------

fanren::core::Result<fanren::core::GameData> loadMagic(const std::string& body) {
    TempDir tmp{"fanren_tongdao_magic"};
    writeFile(tmp.path() / "data" / "magics" / "m.json",
              R"({"id":"m","name":"探针","element":0,"needMp":5,)" + body + "}");
    return fanren::io::loadGameData((tmp.path() / "data").string());
}

TEST(TongdaoMagicLoader, ReadsHitsAmountAndTheFourSupportEffects) {
    auto twin = loadMagic(R"("power": 12, "hits": 2)");
    ASSERT_TRUE(twin.ok) << twin.error;
    EXPECT_EQ(twin.value.findMagic("m")->hits, 2);
    auto plain = loadMagic(R"("power": 12)");
    ASSERT_TRUE(plain.ok) << plain.error;
    EXPECT_EQ(plain.value.findMagic("m")->hits, 1) << "缺省 1";
    EXPECT_EQ(plain.value.findMagic("m")->amount, 0);

    const std::vector<std::pair<std::string, MagicEffect>> effects = {
        {R"("power": 0, "effect": "heal", "amount": 60)", MagicEffect::Heal},
        {R"("power": 0, "effect": "ward")", MagicEffect::Ward},
        {R"("power": 0, "effect": "bulwark", "amount": 30)", MagicEffect::Bulwark},
        {R"("power": 0, "effect": "bind")", MagicEffect::Bind},
    };
    for (const auto& [body, effect] : effects) {
        const auto loaded = loadMagic(body);
        ASSERT_TRUE(loaded.ok) << body << " ← " << loaded.error;
        EXPECT_EQ(loaded.value.findMagic("m")->effect, effect) << body;
    }
    EXPECT_EQ(loadMagic(effects[0].first).value.findMagic("m")->amount, 60);
}

TEST(TongdaoMagicLoader, RejectsAmountsAndHitsWhereTheyMeanNothing) {
    for (const char* body : {
             R"("power": 0, "effect": "heal")",                       // 回血不写量
             R"("power": 0, "effect": "bulwark", "amount": 95)",      // 减伤超过九成
             R"("power": 0, "effect": "ward", "amount": 5)",          // 护持不收量
             R"("power": 12, "amount": 5)",                           // 伤人法术不收量
             R"("power": 12, "effect": "heal", "amount": 5)",         // 辅助效果不能同时伤人
             R"("power": 12, "hits": 6)",                             // 段数超过 5
             R"("power": 12, "hits": 0)",
             R"("power": 0, "effect": "ward", "hits": 2)",            // 辅助法术没有段
             R"("power": 0, "effect": "freeze")",
         }) {
        const auto loaded = loadMagic(body);
        EXPECT_FALSE(loaded.ok) << body;
    }
}

// ---------------------------------------------------------------------------
// 编成
// ---------------------------------------------------------------------------

fanren::core::Result<fanren::core::BattleSetup> loadBattleText(const std::string& extra, bool fatal = false,
                                                               bool escape = false, const std::string& rewards = "") {
    TempDir tmp{"fanren_tongdao_battle"};
    const std::string text = std::string(R"({"id": "spar_x", "name": "切磋", "chapter": 7, "terrain": "field", )") +
                             R"("can_escape": )" + (escape ? "true" : "false") + R"(, "defeat_is_fatal": )" +
                             (fatal ? "true" : "false") +
                             R"(, "units": [{"role_id": "chen_shimei", "faction": "enemy"}])" +
                             (rewards.empty() ? std::string(R"(, "rewards": {"cultivation": 0, "spirit_stones": 0, "drops": []})")
                                              : rewards) +
                             extra + "}";
    writeFile(tmp.path() / "b.json", text);
    return fanren::io::loadBattle((tmp.path() / "b.json").string());
}

TEST(TongdaoBattleLoader, SparDefaultsOffAndWhenOnItMustBeSafe) {
    const auto off = loadBattleText("");
    ASSERT_TRUE(off.ok) << off.error;
    EXPECT_FALSE(off.value.spar);
    const auto on = loadBattleText(R"(, "spar": true)");
    ASSERT_TRUE(on.ok) << on.error;
    EXPECT_TRUE(on.value.spar);

    EXPECT_FALSE(loadBattleText(R"(, "spar": "yes")").ok);
    EXPECT_FALSE(loadBattleText(R"(, "spar": true)", /*fatal=*/true).ok) << "切磋输了不致命";
    EXPECT_FALSE(loadBattleText(R"(, "spar": true)", false, /*escape=*/true).ok) << "切磋不能逃";
    EXPECT_FALSE(loadBattleText(R"(, "spar": true)", false, false,
                                R"(, "rewards": {"cultivation": 5, "spirit_stones": 0, "drops": []})")
                     .ok)
        << "切磋不发修为";
    EXPECT_FALSE(loadBattleText(R"(, "spar": true, "hero_absent": true)").ok);
}

TEST(TongdaoBattleLoader, NoExistingFormationIsASpar) {
    const auto loaded = fanren::io::loadBattles((repoRoot() / "data" / "battles").string());
    ASSERT_TRUE(loaded.ok) << loaded.error;
    for (const auto& [id, setup] : loaded.value) EXPECT_FALSE(setup.spar) << id << "（切磋编成归路 N）";
}

}  // namespace
