// 第 11 章四张编成（只读数据那一半）。对应 docs/ch11-design.md 第 15 节验收第 4 条与 8.2「必须守住的」：
//   ① b11_ningcuidao_youyao  qianmu_yao ×1 ＋ siji_haishou ×2、无 sanji_haishou、可逃、致命、cultivation > 0、
//      drops 恰五级妖丹 ×1 rate 100；qianmu_yao 破绽含「土」、siji_haishou 含「暗器」
//   ② b11_honghu_jiguanjiao  jiguan_jiao ×1、可逃、不致命、drops 恰六级妖丹 ×1 rate 100、cultivation 0；破绽含「金」
//   ③ b11_yinsha_shayao      yinsha_shayao 恰 4、不可逃、致命、drops 空、cultivation 0；破绽含「剑」
//   ④ b11_tiandu_shi         tiandu_shi ×1、killable_by 恰 ["水","火"] 且 ⊆ weaknesses、weaknesses 恰 {水,火}、
//      友军恰 xueyu_zhizhu_ally ×2 ＋ kuilei_yuan ×2（各引水 / 火两门友军法术）、不可逃、致命、drops 空、cultivation 0
//   四张 hero_absent false、spirit_stones 0、chapter 11。
// 「② 三回合内逃得出、④ 只靠友军两技能打到 0」是 BattleHand 实测，归平衡 / 通关用例，这里不做。
#include <gtest/gtest.h>
#include "Ch11Data.h"
#include "core/model/Types.h"

namespace {
using namespace fanren::test::ch11;
using Errors = std::vector<std::string>;
using Units = std::multiset<std::string>;

const std::string kEarth = "土", kDart = "暗器", kMetal = "金", kSword = "剑";
const std::string kWater = "水", kFire = "火";

std::string joined(const Errors& errors) {
    std::string out;
    for (const auto& e : errors) out += "\n  " + e;
    return out;
}

struct Data {
    std::array<Json, 4> battles;
    std::map<std::string, Json> roles;
    std::map<std::string, Json> magics;
};

Data shipped() {
    Data data;
    for (std::size_t i = 0; i < kBattles.size(); ++i)
        data.battles[i] = json(root() / "data" / "battles" / (std::string(kBattles[i]) + ".json"));
    for (const char* id : {"qianmu_yao", "siji_haishou", "jiguan_jiao", "yinsha_shayao", "tiandu_shi",
                           "xueyu_zhizhu_ally", "kuilei_yuan"})
        data.roles[id] = json(root() / "data" / "roles" / (std::string(id) + ".json"));
    for (const char* id : {"magic_zhizhu_tuwang", "magic_kuilei_zihuo"})
        data.magics[id] = json(root() / "data" / "magics" / (std::string(id) + ".json"));
    return data;
}

bool exactDrop(const Json& drops, const char* item) {
    return drops.is_array() && drops.size() == 1 && drops[0].value("item_id", "") == item &&
           drops[0].value("count", 0) == 1 && drops[0].value("rate", 0) == 100;
}

std::set<std::string> weak(const Data& data, const char* role) {
    return data.roles.at(role).value("weaknesses", Json::array()).get<std::set<std::string>>();
}

Errors errors(const Data& data) {
    Errors out;
    const auto need = [&](bool ok, const std::string& why) { if (!ok) out.push_back(why); };
    std::array<Units, 4> foes, allies;
    for (std::size_t i = 0; i < kBattles.size(); ++i) {
        const Json& b = data.battles[i];
        const std::string id = kBattles[i];
        need(b.at("id") == id, id + " id");
        need(b.at("chapter") == 11, id + " chapter");
        need(b.value("hero_absent", true) == false, id + " hero_absent");
        need(b.at("rewards").at("spirit_stones") == 0, id + " spirit_stones");
        for (const auto& unit : b.at("units"))
            (unit.at("faction") == "ally" ? allies[i] : foes[i]).insert(unit.at("role_id").get<std::string>());
    }
    const auto& r1 = data.battles[0].at("rewards");
    need(foes[0] == Units{"qianmu_yao", "siji_haishou", "siji_haishou"} && allies[0].empty(), "1 units");
    need(data.battles[0].at("can_escape") == true && data.battles[0].at("defeat_is_fatal") == true, "1 escape/fatal");
    need(r1.at("cultivation").get<int>() > 0, "1 cultivation");
    need(exactDrop(r1.at("drops"), "material_yaodan_wuji"), "1 drops");
    need(weak(data, "qianmu_yao").count(kEarth) == 1, "qianmu_yao earth");
    need(weak(data, "siji_haishou").count(kDart) == 1, "siji_haishou dart");
    const auto& r2 = data.battles[1].at("rewards");
    need(foes[1] == Units{"jiguan_jiao"} && allies[1].empty(), "2 units");
    need(data.battles[1].at("can_escape") == true && data.battles[1].at("defeat_is_fatal") == false, "2 escape/fatal");
    need(r2.at("cultivation") == 0, "2 cultivation");
    need(exactDrop(r2.at("drops"), "material_yaodan_liuji"), "2 drops");
    need(weak(data, "jiguan_jiao").count(kMetal) == 1, "jiguan_jiao metal");
    for (std::size_t i : {2u, 3u}) {
        const Json& b = data.battles[i];
        need(b.at("can_escape") == false && b.at("defeat_is_fatal") == true, std::string(kBattles[i]) + " escape/fatal");
        need(b.at("rewards").at("drops").empty() && b.at("rewards").at("cultivation") == 0, std::string(kBattles[i]) + " rewards");
    }
    need(foes[2] == Units{"yinsha_shayao", "yinsha_shayao", "yinsha_shayao", "yinsha_shayao"} && allies[2].empty(), "3 units");
    need(weak(data, "yinsha_shayao").count(kSword) == 1, "yinsha_shayao sword");
    need(foes[3] == Units{"tiandu_shi"}, "4 foe");
    need(allies[3] == Units{"xueyu_zhizhu_ally", "xueyu_zhizhu_ally", "kuilei_yuan", "kuilei_yuan"}, "4 allies");
    const Json& corpse = data.roles.at("tiandu_shi");
    need(corpse.value("killable_by", Json()) == Json::array({kWater, kFire}), "4 killable_by");
    need(weak(data, "tiandu_shi") == std::set<std::string>{kWater, kFire}, "4 weaknesses");
    // 友军法术：蛛网是水、紫火是火（core::Element），各挂在对应友军的 magics 上。
    need(data.roles.at("xueyu_zhizhu_ally").value("magics", Json::array()) == Json::array({"magic_zhizhu_tuwang"}), "spider magic");
    need(data.roles.at("kuilei_yuan").value("magics", Json::array()) == Json::array({"magic_kuilei_zihuo"}), "ape magic");
    need(data.magics.at("magic_zhizhu_tuwang").at("element") == static_cast<int>(fanren::core::kElementWater), "web is water");
    need(data.magics.at("magic_kuilei_zihuo").at("element") == static_cast<int>(fanren::core::kElementFire), "purple fire is fire");
    return out;
}

TEST(Ch11BattleData, TheFourMandatorySetupsMatchDesignEightPointTwo) {
    const auto found = errors(shipped());
    EXPECT_TRUE(found.empty()) << joined(found);
}

TEST(Ch11BattleData, InMemoryMutationsOfUnitsRewardsEscapeAndWeaknessesAreCaught) {
    const Data original = shipped();
    ASSERT_TRUE(errors(original).empty()) << joined(errors(original));
    for (int variant = 0; variant < 14; ++variant) {
        Data bad = original;
        switch (variant) {
        case 0: bad.battles[0]["units"].push_back({{"role_id", "sanji_haishou"}, {"faction", "enemy"}, {"wave", 1}}); break;
        case 1: bad.battles[0]["rewards"]["drops"][0]["rate"] = 50; break;
        case 2: bad.battles[0]["rewards"]["cultivation"] = 0; break;
        case 3: bad.battles[1]["defeat_is_fatal"] = true; break;
        case 4: bad.battles[1]["rewards"]["drops"] = Json::array(); break;
        case 5: bad.battles[2]["units"].erase(bad.battles[2]["units"].begin()); break;
        case 6: bad.battles[2]["can_escape"] = true; break;
        case 7: bad.roles["tiandu_shi"]["killable_by"] = Json::array({kWater}); break;
        case 8: bad.roles["tiandu_shi"]["weaknesses"].push_back(kSword); break;
        case 9: bad.battles[3]["units"][2]["role_id"] = "xueyu_zhizhu_ally"; break;
        case 10: bad.battles[3]["hero_absent"] = true; break;
        case 11: bad.roles["jiguan_jiao"]["weaknesses"] = Json::array({kFire}); break;
        case 12: bad.magics["magic_kuilei_zihuo"]["element"] = static_cast<int>(fanren::core::kElementEarth); break;
        case 13: bad.battles[3]["rewards"]["spirit_stones"] = 10; break;
        }
        EXPECT_FALSE(errors(bad).empty()) << "mutation=" << variant;
    }
}
} // namespace
