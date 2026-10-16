// 第 13 章两张编成（只读数据 / 扫脚本那一半）。对应 docs/ch13-design.md 第 15 节验收：
//   第 4 条  两张编成与 8.2 一致：① 一波、白面儒生与黑壮大汉各 1、killable_by 恰 ["金","剑"] / ["木","剑"]、大汉
//            charge.every == 3 且 all == false、无友军、can_escape false、drops 空、cultivation > 0；② 两波（第 0 波五级海兽 ×2、
//            第 1 波青色巨虾 ×1）、巨虾 killable_by 恰 ["木","剑"]、actions 2、charge.every 3 且 all true、can_escape true、
//            drops 恰六级 1 ＋ 五级 2（rate 100）、0 < cultivation <= 60；两张 hero_absent false、defeat_is_fatal true、terrain sea、
//            spirit_stones 0、killable_by ⊆ weaknesses、无空数组、charge 四键齐（16.1 第 59 条）；wuji_haishou 的破绽含土、剑且
//            全仓只 ② 一张编成含它；magic_ji_wuxinghuan 金、effect stagger、stagger 3、power 0。另：8.2 的破绽、架势、法术、
//            境界逐个对表；蜂云合剑是木、无 effect（收尾要得了命）；黑雾怪鸟是水、全体；b13_* 恰两张。
//            「最低合法起点两场都打得过」是平衡路的实测（BattleHand），不在这里。
//   第 15 条 battle( 全章只在 zhuibing.lua、youyao.lua 各一处；ch13.zhuibing 是 n06 的前置；ch13.chongmo 只在 lieyao >= 2
//            之后置（扫脚本 ＋ 读目标链）。通关两侧实打的编成归通关用例；遭遇区 0 处归 Ch13TriggerMode。
//   第 5 节  wen_siyue.json、fan_furen.json 与第 12 章集成版逐字节相同（「一个字不动 2」，契约 6.3）。
//   8.2 战斗背景 / 第 4 节视觉预设走真实入口（handoff 第 6 节第 5 条）：两场背景由开战画面问的 battleBackdropFor 答出，
//            四张图的主题 / 时辰 / backdrop / 粒子由 WorldView 读的那份烘焙 meta 答出。
// 每条判据都有「在内存里改坏 → 必须报」的负向。
#include <gtest/gtest.h>
#include <algorithm>
#include <tuple>
#include "Ch13Data.h"
#include "core/model/Types.h"
#include "core/model/Visual.h"
#include "game/Application.h"
#include "game/BattleScene.h"
#include "game/MapVisual.h"
#include "io/VisualLoader.h"

namespace {
using namespace fanren::test::ch13;
using Errors = std::vector<std::string>;
using Units = std::multiset<std::string>;
using Words = std::set<std::string>;

const std::string kMetal = "金", kWood = "木", kEarth = "土", kSword = "剑";

struct Data {
    std::array<Json, 2> battles;
    std::map<std::string, Json> roles;   // 两场里出现的全部 role
    std::map<std::string, Json> magics;  // 按 id
};

Data shipped() {
    Data data;
    for (std::size_t i = 0; i < kBattles.size(); ++i) {
        data.battles[i] = json(root() / "data" / "battles" / (std::string(kBattles[i]) + ".json"));
        for (const auto& unit : data.battles[i].at("units")) {
            const std::string id = unit.at("role_id").get<std::string>();
            if (!data.roles.count(id)) data.roles[id] = json(root() / "data" / "roles" / (id + ".json"));
        }
    }
    for (const auto& file : fs::directory_iterator(root() / "data" / "magics")) {
        if (file.path().extension() != ".json") continue;
        const Json magic = json(file.path());
        if (magic.contains("id")) data.magics[magic.at("id").get<std::string>()] = magic;
    }
    return data;
}

Units foesOf(const Json& battle, int wave) {
    Units out;
    for (const auto& unit : battle.at("units"))
        if (unit.at("faction") == "enemy" && unit.value("wave", 0) == wave) out.insert(unit.at("role_id").get<std::string>());
    return out;
}

int allyCount(const Json& battle) {
    return static_cast<int>(std::count_if(battle.at("units").begin(), battle.at("units").end(),
                                          [](const Json& unit) { return unit.at("faction") != "enemy"; }));
}

int waveCount(const Json& battle) {
    std::set<int> waves;
    for (const auto& unit : battle.at("units"))
        if (unit.at("faction") == "enemy") waves.insert(unit.value("wave", 0));
    return static_cast<int>(waves.size());
}

Words words(const Json& role, const char* key) { return role.value(key, Json::array()).get<Words>(); }

// 8.2 必须守住的：charge 四键都写、mult > 0（加载器缺了整份拒，16.1 第 59 条），text_key 是本场的蓄势文案。
bool chargeIs(const Json& role, int every, bool all, const std::string& textKey) {
    const Json charge = role.value("charge", Json::object());
    return charge.contains("every") && charge.contains("mult") && charge.contains("all") && charge.contains("text_key") &&
           charge.at("every") == every && charge.at("all") == all && charge.at("text_key") == textKey &&
           charge.at("mult").is_number() && charge.at("mult").get<double>() > 0.0;
}

Errors errors(const Data& data) {
    Errors out;
    const auto need = [&](bool ok, const std::string& why) { if (!ok) out.push_back(why); };
    const auto role = [&](const char* id) -> const Json& { return data.roles.at(id); };
    for (std::size_t i = 0; i < kBattles.size(); ++i) {
        const Json& b = data.battles[i];
        const std::string id = kBattles[i];
        need(b.at("id") == id && b.at("chapter") == 13, id + " id/chapter");
        need(b.value("hero_absent", true) == false && b.value("defeat_is_fatal", false) == true, id + " hero_absent/fatal");
        need(b.value("terrain", "") == "sea", id + " terrain");
        need(allyCount(b) == 0, id + " has allies (8.2: 韩立一人)");
        need(b.value("intro_key", "") == "ch13.battle." + id.substr(4) + ".intro", id + " intro_key");
        const Json& r = b.at("rewards");
        need(r.at("spirit_stones") == 0, id + " spirit_stones");
        for (const auto& unit : b.at("units")) {
            const std::string foe = unit.at("role_id").get<std::string>();
            const auto known = data.roles.find(foe);
            if (known == data.roles.end()) { out.push_back(foe + " is not a designed role of this battle"); continue; }
            const Json& f = known->second;
            need(!words(f, "weaknesses").empty(), foe + " weaknesses empty");
            if (!f.contains("killable_by")) continue;
            const Words kill = words(f, "killable_by"), weak = words(f, "weaknesses");
            need(!kill.empty() && std::includes(weak.begin(), weak.end(), kill.begin(), kill.end()),
                 foe + " killable_by not a non-empty subset of weaknesses");
        }
    }
    // ①
    const Json& one = data.battles[0];
    need(waveCount(one) == 1 && foesOf(one, 0) == Units{"nixing_rusheng", "nixing_dahan"}, "1 units");
    need(one.value("can_escape", true) == false, "1 can_escape");
    need(one.at("rewards").at("drops").is_array() && one.at("rewards").at("drops").empty(), "1 drops");
    need(one.at("rewards").at("cultivation").get<int>() > 0, "1 cultivation");
    need(words(role("nixing_rusheng"), "weaknesses") == Words{kMetal, kSword}, "rusheng weaknesses");
    need(role("nixing_rusheng").value("killable_by", Json()) == Json::array({kMetal, kSword}), "rusheng killable_by");
    need(role("nixing_rusheng").value("toughness", 0) == 3 && role("nixing_rusheng").value("realm", "") == "CoreEarly", "rusheng toughness/realm");
    need(role("nixing_rusheng").value("magics", Json()) == Json::array({"magic_mihuan_guniao"}), "rusheng magics");
    need(words(role("nixing_dahan"), "weaknesses") == Words{kMetal, kWood, kSword}, "dahan weaknesses");
    need(role("nixing_dahan").value("killable_by", Json()) == Json::array({kWood, kSword}), "dahan killable_by");
    need(role("nixing_dahan").value("toughness", 0) == 4 && role("nixing_dahan").value("realm", "") == "CoreEarly", "dahan toughness/realm");
    need(chargeIs(role("nixing_dahan"), 3, false, "ch13.battle.nanming_zhuibing.charge"), "dahan charge");
    // ②
    const Json& two = data.battles[1];
    need(waveCount(two) == 2 && foesOf(two, 0) == Units{"wuji_haishou", "wuji_haishou"} && foesOf(two, 1) == Units{"qingse_juxia"},
         "2 waves");
    need(two.value("can_escape", false) == true, "2 can_escape");
    std::multiset<std::tuple<std::string, int, int>> drops;
    for (const auto& d : two.at("rewards").at("drops"))
        drops.insert({d.value("item_id", ""), d.value("count", 0), d.value("rate", 0)});
    need(drops == std::multiset<std::tuple<std::string, int, int>>{{"material_yaodan_liuji", 1, 100}, {"material_yaodan_wuji", 2, 100}},
         "2 drops");
    const int cultivation = two.at("rewards").at("cultivation").get<int>();
    need(cultivation > 0 && cultivation <= 60, "2 cultivation (O-10: 0 < c <= 60)");
    need(words(role("wuji_haishou"), "weaknesses") == Words{kEarth, kSword} && !role("wuji_haishou").contains("killable_by"),
         "wuji_haishou weaknesses / killable_by (B25)");
    need(role("wuji_haishou").value("toughness", 0) == 3 && role("wuji_haishou").value("realm", "") == "CoreEarly", "wuji toughness/realm");
    need(words(role("qingse_juxia"), "weaknesses") == Words{kMetal, kWood, kSword}, "juxia weaknesses");
    need(role("qingse_juxia").value("killable_by", Json()) == Json::array({kWood, kSword}), "juxia killable_by");
    need(role("qingse_juxia").value("actions", 1) == 2, "juxia actions");
    need(role("qingse_juxia").value("toughness", 0) == 5 && role("qingse_juxia").value("realm", "") == "CoreMid", "juxia toughness/realm");
    need(role("qingse_juxia").value("magics", Json()) == Json::array({"magic_haichao_jue"}), "juxia magics");
    need(chargeIs(role("qingse_juxia"), 3, true, "ch13.battle.waixinghai_lieyao.charge"), "juxia charge");
    // 两门新手段与儒生的怪鸟（第 7 节「两样新手段」、8.2、18.3 第 6 条）。
    const auto magic = [&](const char* id) { const auto f = data.magics.find(id); return f == data.magics.end() ? Json() : f->second; };
    const Json ring = magic("magic_ji_wuxinghuan"), sword = magic("magic_fengyun_heji"), bird = magic("magic_mihuan_guniao");
    need(ring.value("element", 0) == fanren::core::kElementMetal && ring.value("effect", "") == "stagger" &&
         ring.value("stagger", 0) == 3 && ring.value("power", -1) == 0, "five-element ring");
    need(sword.value("element", 0) == fanren::core::kElementWood && !sword.contains("effect") && sword.value("power", 0) > 0,
         "swarm sword is not a killing wood spell");
    need(bird.value("element", 0) == fanren::core::kElementWater && bird.value("target", "") == "all", "black-fog bird");
    // 门禁复核 G-2：敌人的法力 ≥ 它每一门法术的耗蓝（G-34 下有法术合法就只放法术，法力定放几发；低于耗蓝时它永远
    // 放不出、也不报错，平衡路量的「一发 / 三发」就静悄悄变成零发）。没有法术的（大汉）法力 0：只打拳脚与蓄势重招。
    // 规则 25 只核友军的 maxMp，敌方这一条只有这里钉着。
    for (const auto& [id, f] : data.roles) {
        const int mp = f.value("maxMp", -1);
        const Json spells = f.value("magics", Json::array());
        if (spells.empty()) need(mp == 0, id + " has no spell but maxMp " + std::to_string(mp));
        for (const auto& spell : spells) {
            const Json m = magic(spell.get<std::string>().c_str());
            need(!m.is_null() && mp >= m.value("needMp", 1 << 20),
                 id + " maxMp " + std::to_string(mp) + " cannot pay " + spell.get<std::string>());
        }
    }
    return out;
}

TEST(Ch13BattleData, TheTwoMandatorySetupsMatchDesignEightPointTwo) {
    const auto found = errors(shipped());
    EXPECT_TRUE(found.empty()) << joined(found);
}

TEST(Ch13BattleData, InMemoryMutationsOfWavesWeaknessesChargesAndRewardsAreCaught) {
    const Data original = shipped();
    ASSERT_TRUE(errors(original).empty()) << joined(errors(original));
    for (int variant = 0; variant < 25; ++variant) {
        Data bad = original;
        switch (variant) {
        case 0: bad.battles[0]["units"].push_back({{"role_id", "nixing_dahan"}, {"faction", "enemy"}, {"wave", 1}}); break;
        case 1: bad.battles[0]["units"].push_back({{"role_id", "ling_yuling"}, {"faction", "ally"}}); break;     // 有了援手
        case 2: bad.roles["nixing_rusheng"]["killable_by"] = Json::array({kMetal}); break;
        case 3: bad.roles["nixing_dahan"]["charge"]["all"] = true; break;
        case 4: bad.roles["nixing_dahan"]["charge"].erase("mult"); break;                                     // 16.1 第 59 条
        case 5: bad.battles[0]["can_escape"] = true; break;
        case 6: bad.battles[0]["rewards"]["drops"].push_back({{"item_id", "story_mihuan_yan"}, {"count", 1}, {"rate", 100}}); break;
        case 7: bad.battles[1]["units"][2]["wave"] = 0; break;                                                // 巨虾混进第 0 波
        case 8: bad.roles["qingse_juxia"]["killable_by"] = Json::array({kMetal, kWood, kSword}); break;      // 金能要它的命
        case 9: bad.roles["qingse_juxia"]["actions"] = 1; break;
        case 10: bad.battles[1]["rewards"]["cultivation"] = 61; break;
        case 11: bad.battles[1]["rewards"]["drops"][0]["rate"] = 50; break;
        case 12: bad.battles[1]["can_escape"] = false; break;
        case 13: bad.roles["wuji_haishou"]["weaknesses"] = Json::array({kEarth}); break;                      // B25 补的剑
        case 14: bad.battles[1]["rewards"]["spirit_stones"] = 320; break;                                     // 旧占位的灵石
        case 15: bad.magics["magic_ji_wuxinghuan"]["power"] = 20; break;
        case 16: bad.magics["magic_fengyun_heji"]["effect"] = "stagger"; break;                              // 蜂云合剑成了削势
        case 17: bad.battles[0]["terrain"] = "dock"; break;
        case 18: bad.roles["qingse_juxia"]["charge"]["text_key"] = "ch13.battle.nanming_zhuibing.charge"; break;
        case 19: bad.roles["nixing_rusheng"]["killable_by"] = Json::array(); break;                           // 空数组
        // G-2：敌方法力 ≥ 法术耗蓝（门禁复核 B3 / B4 那两种，外加儒生、大汉、法术涨价）。
        case 20: bad.roles["wuji_haishou"]["maxMp"] = 17; break;                                             // 海潮诀 18
        case 21: bad.roles["qingse_juxia"]["maxMp"] = 0; break;
        case 22: bad.roles["nixing_rusheng"]["maxMp"] = 23; break;                                           // 黑雾怪鸟 24
        case 23: bad.roles["nixing_dahan"]["maxMp"] = 10; break;                                             // 无法术却有法力
        case 24: bad.magics["magic_haichao_jue"]["needMp"] = 19; break;                                      // 海兽只有 18
        }
        EXPECT_FALSE(errors(bad).empty()) << "mutation=" << variant;
    }
    // 边界对照（G-2）：法力恰等于耗蓝不报——巨虾压到 18（＝海潮诀）、儒生压到 24（＝黑雾怪鸟）。
    Data edge = original;
    edge.roles["qingse_juxia"]["maxMp"] = 18;
    edge.roles["nixing_rusheng"]["maxMp"] = 24;
    EXPECT_TRUE(errors(edge).empty()) << joined(errors(edge));
}

// ---- 第 4 条后半：wuji_haishou 全仓只 ② 一张编成含它；b13_* 恰两张；两个「一个字不动」的 role ----
// 第 12 章集成版（HEAD 46f6d37）：wen_siyue.json 476 字节、fan_furen.json 445 字节。
constexpr std::size_t kWenSiyueBytes = 476;
constexpr std::uint64_t kWenSiyueHash = 0xe7e1ca020ea7a8f8ull;
constexpr std::size_t kFanFurenBytes = 445;
constexpr std::uint64_t kFanFurenHash = 0x0d7312696bcdedb9ull;

Errors catalogueErrors(const std::map<std::string, Json>& battles, const std::string& wenSiyue, const std::string& fanFuren) {
    Errors out;
    std::vector<std::string> holders, ours;
    for (const auto& [id, battle] : battles) {
        if (id.rfind("b13_", 0) == 0) ours.push_back(id);
        for (const auto& unit : battle.value("units", Json::array()))
            if (unit.value("role_id", "") == "wuji_haishou") { holders.push_back(id); break; }
    }
    if (holders != std::vector<std::string>{"b13_waixinghai_lieyao"}) out.push_back("wuji_haishou is not in exactly battle 2");
    if (ours != std::vector<std::string>{"b13_nanming_zhuibing", "b13_waixinghai_lieyao"}) out.push_back("b13_* battles are not the two of 8.2");
    if (wenSiyue.size() != kWenSiyueBytes || fnv1a(wenSiyue) != kWenSiyueHash) out.push_back("wen_siyue.json differs from chapter 12");
    if (fanFuren.size() != kFanFurenBytes || fnv1a(fanFuren) != kFanFurenHash) out.push_back("fan_furen.json differs from chapter 12");
    return out;
}

std::map<std::string, Json> allBattles() {
    std::map<std::string, Json> out;
    for (const auto& file : fs::recursive_directory_iterator(root() / "data" / "battles"))
        if (file.path().extension() == ".json") out[file.path().stem().string()] = json(file.path());
    return out;
}

TEST(Ch13BattleData, TheSeaBeastFightsOnlyInBattleTwoAndTheTwoUntouchedRolesAreByteForByteChapterTwelve) {
    const auto battles = allBattles();
    ASSERT_GT(battles.size(), 40u) << "先验：读得到 data/battles（第 1–12 章合计 45 场）";
    const std::string wen = read(root() / "data" / "roles" / "wen_siyue.json");
    const std::string fan = read(root() / "data" / "roles" / "fan_furen.json");
    const auto found = catalogueErrors(battles, wen, fan);
    EXPECT_TRUE(found.empty()) << joined(found);
    auto borrowed = battles;
    borrowed["b14_fengxi_xiezhi"]["units"].push_back({{"role_id", "wuji_haishou"}, {"faction", "enemy"}, {"wave", 0}});
    EXPECT_FALSE(catalogueErrors(borrowed, wen, fan).empty()) << "第二张编成借五级海兽";
    auto reserved = battles;
    reserved["b13_mishi_yuntianxiao"] = Json::object();
    EXPECT_FALSE(catalogueErrors(reserved, wen, fan).empty()) << "O-1 的预留编成被建了";
    auto edited = wen;
    edited[edited.size() / 2] = edited[edited.size() / 2] == '1' ? '2' : '1';
    EXPECT_FALSE(catalogueErrors(battles, edited, fan).empty());
    EXPECT_FALSE(catalogueErrors(battles, wen, fan + "\n").empty());
}

// ---- 第 15 条：必打 2 场绕不过去（扫脚本 ＋ 读目标链 ＋ 读地图）----
Errors battleCallErrors(const std::map<std::string, std::string>& sources) {
    Errors out;
    const std::map<std::string, std::string> owner{{"b13_nanming_zhuibing", "zhuibing"}, {"b13_waixinghai_lieyao", "youyao"}};
    const std::regex anyCall(R"lua(\bbattle\s*\()lua");
    const std::regex named(R"lua(\bbattle\s*\(\s*"([^"]+)")lua");
    int total = 0;
    std::map<std::string, int> calls;
    for (const auto& [name, source] : sources) {
        const std::string code = executableLines(source);
        total += countMatches(code, anyCall);
        for (std::sregex_iterator it(code.begin(), code.end(), named), end; it != end; ++it) {
            const std::string id = (*it)[1].str();
            ++calls[id];
            const auto expected = owner.find(id);
            if (expected == owner.end()) out.push_back(name + " calls undesigned battle " + id);
            else if (expected->second != name) out.push_back(id + " called from " + name);
        }
    }
    for (const auto& [id, script] : owner)
        if (calls[id] != 1) out.push_back(id + " called " + std::to_string(calls[id]) + " times");
    if (total != 2) out.push_back("battle( occurs " + std::to_string(total) + " times in chapter 13");
    // ① 是 n06 的前置：yinjian.lua 开头判 ch13.zhuibing；② 的胜场是 n25 的门槛：chongmo.lua 第一句判 lieyao >= 2，
    // 置 ch13.chongmo 在它之后（3.2 n25、验收 15）。
    const auto lines = [&](const char* name) {
        const auto found = sources.find(name);
        return found == sources.end() ? std::vector<std::string>{} : liveLines(found->second);
    };
    const auto yinjian = lines("yinjian");
    if (std::none_of(yinjian.begin(), yinjian.begin() + std::min<std::ptrdiff_t>(2, static_cast<std::ptrdiff_t>(yinjian.size())),
                     [](const std::string& l) { return l.rfind("if flag.get(\"ch13.zhuibing\") == 0 then", 0) == 0 && l.find("return") != std::string::npos; }))
        out.push_back("yinjian.lua does not gate on ch13.zhuibing");
    const auto chongmo = lines("chongmo");
    if (chongmo.empty() || chongmo[0].rfind("if flag.get(\"ch13.lieyao\") < 2 then", 0) != 0 || chongmo[0].find("return") == std::string::npos)
        out.push_back("chongmo.lua does not open on ch13.lieyao < 2");
    if (std::count(chongmo.begin(), chongmo.end(), "flag.set(\"ch13.chongmo\")") != 1) out.push_back("chongmo.lua does not set ch13.chongmo once");
    return out;
}

TEST(Ch13BattleData, TheTwoMandatoryBattlesHaveOneCallSiteEachAndGateTheNextStep) {
    const auto found = battleCallErrors(scripts());
    EXPECT_TRUE(found.empty()) << joined(found);
    // ① 的 set_flag 在目标链里，是下一步 NPC 的出场旗标；② 的计数旗是第 24 步、第 25 步挂点的 guard 是 buzhen（3.0 表）。
    const auto steps = json(root() / "data" / "objectives" / "ch13.json").at("steps");
    ASSERT_EQ(steps.size(), 34u) << "施工图 3.4：目标链 34 步";
    EXPECT_EQ(steps[4].at("done_flag"), "ch13.zhuibing");
    EXPECT_EQ(steps[4].at("target_object"), "trigger_chugang");
    EXPECT_EQ(steps[5].at("target_object"), "npc_ling_yuling");
    EXPECT_EQ(steps[23].at("done_flag"), "ch13.lieyao");
    EXPECT_EQ(steps[23].at("target_object"), "trigger_youyao");
    EXPECT_EQ(steps[24].at("target_object"), "trigger_shoushou");
    const auto isle = objects(json(root() / "maps" / "ch13_nanming.tmj"));
    EXPECT_EQ(properties(isle.at("trigger_chugang")).value("set_flag", ""), "ch13.zhuibing");
    EXPECT_EQ(properties(isle.at("npc_ling_yuling")).value("visible_flag", ""), "ch13.zhuibing");
    const auto fog = objects(json(root() / "maps" / "ch13_wuzhong.tmj"));
    EXPECT_EQ(properties(fog.at("trigger_shoushou")).value("guard_flag", ""), "ch13.buzhen");
}

TEST(Ch13BattleData, AnExtraOrMovedBattleCallOrAnOpenGateIsCaught) {
    const auto original = scripts();
    ASSERT_TRUE(battleCallErrors(original).empty());
    auto extra = original;
    extra["chongmo"] += "\nbattle(\"b13_waixinghai_lieyao\")\n";
    EXPECT_FALSE(battleCallErrors(extra).empty());
    auto stranger = original;
    stranger["mishi"] += "\nlocal won = battle(\"b13_mishi_yuntianxiao\")\n";
    EXPECT_FALSE(battleCallErrors(stranger).empty()) << "O-1：云天啸不升格为仗";
    auto moved = original;
    const auto at = findInCode(moved["zhuibing"], "battle(\"b13_nanming_zhuibing\")");
    ASSERT_NE(at, std::string::npos);
    moved["zhuibing"].replace(at, 6, "fight");
    moved["yinjian"] += "\nbattle(\"b13_nanming_zhuibing\")\n";
    EXPECT_FALSE(battleCallErrors(moved).empty());
    auto commented = original;
    commented["banyao"] += "\n-- battle(\"b13_waixinghai_lieyao\")\n";
    EXPECT_TRUE(battleCallErrors(commented).empty()) << "注释里的不算";
    auto early = original;
    early["chongmo"] = std::regex_replace(early["chongmo"], std::regex(R"lua(flag\.get\("ch13\.lieyao"\) < 2)lua"), "flag.get(\"ch13.lieyao\") < 1");
    EXPECT_FALSE(battleCallErrors(early).empty()) << "收手门槛降到 1 胜";
    auto open = original;
    open["yinjian"] = std::regex_replace(open["yinjian"], std::regex(R"lua(if flag\.get\("ch13\.zhuibing"\) == 0 then[^\n]*\n)lua"), "");
    EXPECT_FALSE(battleCallErrors(open).empty()) << "n06 不再判 ① 的旗";
}

// ---- 8.2「战斗背景」与第 4 节视觉预设走真实入口（handoff 第 6 节第 5 条）----
// 战斗：开战画面问的就是 battleBackdropFor；地图：WorldView::ensureMap 用 findAssets("art/maps/<id>", "meta.json") →
// io::loadMapMeta → mapVisualFrom，这里走同一个资产根、同一个函数。判据抄施工图，不从 data/visual/*.json 读。
struct Backdrop { const char* battle; const char* map; const char* backdrop; };
constexpr Backdrop kBackdrops[] = {
    {"b13_nanming_zhuibing", "ch13_nanming", "dock_river"}, {"b13_waixinghai_lieyao", "ch13_wuzhong", "cliff_top"}};

using fanren::engine::ParticleKind;
struct Preset { const char* map; const char* theme; fanren::core::TimeOfDay time; const char* backdrop; ParticleKind particle; };
constexpr fanren::core::TimeOfDay kDay = fanren::core::TimeOfDay::Day, kDusk = fanren::core::TimeOfDay::Dusk;
constexpr Preset kPresets[] = {
    {"ch13_nanming", "dock", kDay, "dock_river", ParticleKind::Mist}, {"ch13_tianxing", "town", kDusk, "dock_river", ParticleKind::Dust},
    {"ch13_waihai", "dock", kDay, "dock_river", ParticleKind::Mist}, {"ch13_wuzhong", "valley", kDay, "herb_valley", ParticleKind::Mist}};

Errors backdropErrors(const std::map<std::string, std::string>& actual) {
    Errors out;
    for (const auto& row : kBackdrops) {
        const auto found = actual.find(row.battle);
        if (found == actual.end() || found->second != row.backdrop) out.push_back(std::string(row.battle) + " backdrop");
    }
    return out;
}

Errors presetErrors(const std::map<std::string, fanren::core::MapMeta>& metas) {
    Errors out;
    for (const auto& row : kPresets) {
        const auto found = metas.find(row.map);
        if (found == metas.end()) { out.push_back(std::string(row.map) + " has no baked meta"); continue; }
        const fanren::game::MapVisual visual = fanren::game::mapVisualFrom(found->second);
        if (visual.theme != row.theme) out.push_back(std::string(row.map) + " theme " + visual.theme);
        if (visual.time != row.time) out.push_back(std::string(row.map) + " time");
        if (found->second.backdrop != row.backdrop) out.push_back(std::string(row.map) + " backdrop " + found->second.backdrop);
        const auto& layers = visual.particles;
        if (std::none_of(layers.begin(), layers.end(), [&](const auto& l) { return l.kind == row.particle; }))
            out.push_back(std::string(row.map) + " lacks its particle");
    }
    return out;
}

class Ch13VisualWiring : public ::testing::Test {
protected:
    void SetUp() override {
        const auto ready = app_.init(root().string(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
    }
    void TearDown() override { app_.shutdown(); }
    fanren::game::Application app_;
};

TEST_F(Ch13VisualWiring, TheTwoBattlesOpenOnTheBackdropsOfDesignEightPointTwo) {
    std::map<std::string, std::string> actual;
    for (const auto& row : kBackdrops) {
        const auto loaded = app_.loadMap(row.map, std::string{});
        ASSERT_TRUE(loaded.ok) << loaded.error;
        actual[row.battle] = fanren::game::battleBackdropFor(app_, row.battle);
    }
    const auto found = backdropErrors(actual);
    EXPECT_TRUE(found.empty()) << joined(found);
    auto wrong = actual;
    wrong["b13_waixinghai_lieyao"] = "herb_valley";
    EXPECT_FALSE(backdropErrors(wrong).empty()) << "负向：② 退回地图背景必须报";
    // 配对的负向：表里没有的一场在雾中岛上取地图自己的背景——② 的字面量与所在图不同，证明答话的是指派表
    //（① 与南明岛同为 dock_river，分不出来，只靠上面那条字面量）。
    ASSERT_TRUE(app_.loadMap("ch13_wuzhong", std::string{}).ok);
    EXPECT_EQ(fanren::game::battleBackdropFor(app_, "b13_no_such_battle"), "herb_valley");
}

TEST_F(Ch13VisualWiring, TheFourMapsLoadThePresetsOfDesignSectionFour) {
    std::map<std::string, fanren::core::MapMeta> metas;
    for (const auto& row : kPresets) {
        const auto loaded = app_.loadMap(row.map, std::string{});
        ASSERT_TRUE(loaded.ok) << loaded.error;
        const auto files = app_.engine().findAssets("art/maps/" + std::string(row.map), "meta.json");
        ASSERT_FALSE(files.empty()) << row.map;
        const auto meta = fanren::io::loadMapMeta(files.front());
        ASSERT_TRUE(meta.ok) << meta.error;
        metas[row.map] = meta.value;
    }
    const auto found = presetErrors(metas);
    EXPECT_TRUE(found.empty()) << joined(found);
    for (int variant = 0; variant < 4; ++variant) {
        auto bad = metas;
        switch (variant) {
        case 0: bad["ch13_wuzhong"].backdrop = "dock_river"; break;
        case 1: bad["ch13_tianxing"].time = kDay; break;
        case 2: bad["ch13_waihai"].theme = "town"; break;
        case 3: bad["ch13_tianxing"].particles.clear(); break;
        }
        EXPECT_FALSE(presetErrors(bad).empty()) << "mutation=" << variant;
    }
}
}  // namespace
