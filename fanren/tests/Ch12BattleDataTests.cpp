// 第 12 章四张编成（只读数据 / 扫脚本那一半）。对应 docs/ch12-design.md 第 15 节验收：
//   第 4 条  四张编成与 8.2 一致（波次、敌方、破绽、killable_by、蓄势、actions、友军；③ 两名友军的 magics 不含金类；
//            ④ 无友军）；四张 hero_absent false、defeat_is_fatal true、can_escape false、spirit_stones 0、drops 空、
//            cultivation > 0；killable_by 非空且 ⊆ weaknesses；b12_xutian_hudian_kuilei 不存在。
//   第 7 条  xuan_gu.json 只一份、③ 里 ally、④ 里 enemy；wu_chou.json 与第 11 章集成版逐字节相同；
//            xutian_hudian_kuilei、duoding_xiushi 不存在（契约 E2 引擎用例之外的读数据那一半）。
//   第 14 条 四编成各只被一个脚本调用（guiwang / tihun / ruyi / xuangu），本章没有别的 battle(；set_flag 在目标链里且是
//            下一步的 guard。通关两侧实打的编成归通关用例；BattleHand 实测归平衡路。
//   第 26 条 视觉数据走真实入口：四场背景由开战画面问的 battleBackdropFor 答出 8.2 的字面量；十张图的主题 / 时辰 /
//            backdrop 由 WorldView 读的那份烘焙 meta（同一资产根、同一 loadMapMeta → mapVisualFrom）答出第 4 节的字面量。
// 每条判据都有「在内存里改坏 → 必须报」的负向。
#include <gtest/gtest.h>
#include <algorithm>
#include "Ch12Data.h"
#include "core/model/Types.h"
#include "core/model/Visual.h"
#include "game/Application.h"
#include "game/BattleScene.h"
#include "game/MapVisual.h"
#include "io/VisualLoader.h"

namespace {
using namespace fanren::test::ch12;
using Errors = std::vector<std::string>;
using Units = std::multiset<std::string>;
using Words = std::set<std::string>;

const std::string kMetal = "金", kWood = "木", kWater = "水", kFire = "火", kSword = "剑";

std::string joined(const Errors& errors) {
    std::string out;
    for (const auto& e : errors) out += "\n  " + e;
    return out;
}

struct Data {
    std::array<Json, 4> battles;
    std::map<std::string, Json> roles;   // 四场里出现的全部 role
    std::map<std::string, Json> magics;  // 按 id（data/magics 的文件名不带 magic_ 前缀）
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

// 某一波的敌方（按 wave 字段；友军不写 wave）。
Units foesOf(const Json& battle, int wave) {
    Units out;
    for (const auto& unit : battle.at("units"))
        if (unit.at("faction") == "enemy" && unit.value("wave", 0) == wave) out.insert(unit.at("role_id").get<std::string>());
    return out;
}

Units alliesOf(const Json& battle) {
    Units out;
    for (const auto& unit : battle.at("units"))
        if (unit.at("faction") == "ally") out.insert(unit.at("role_id").get<std::string>());
    return out;
}

int waveCount(const Json& battle) {
    std::set<int> waves;
    for (const auto& unit : battle.at("units"))
        if (unit.at("faction") == "enemy") waves.insert(unit.value("wave", 0));
    return static_cast<int>(waves.size());
}

Words words(const Json& role, const char* key) { return role.value(key, Json::array()).get<Words>(); }

Errors errors(const Data& data) {
    Errors out;
    const auto need = [&](bool ok, const std::string& why) { if (!ok) out.push_back(why); };
    const auto role = [&](const char* id) -> const Json& { return data.roles.at(id); };
    for (std::size_t i = 0; i < kBattles.size(); ++i) {
        const Json& b = data.battles[i];
        const std::string id = kBattles[i];
        need(b.at("id") == id && b.at("chapter") == 12, id + " id/chapter");
        need(b.value("hero_absent", true) == false, id + " hero_absent");
        need(b.value("can_escape", true) == false && b.value("defeat_is_fatal", false) == true, id + " escape/fatal");
        const Json& r = b.at("rewards");
        need(r.at("spirit_stones") == 0 && r.at("drops").is_array() && r.at("drops").empty(), id + " stones/drops");
        need(r.at("cultivation").get<int>() > 0, id + " cultivation");
        for (const auto& unit : b.at("units")) {
            if (unit.at("faction") != "enemy") continue;
            const Json& foe = data.roles.at(unit.at("role_id").get<std::string>());
            if (!foe.contains("killable_by")) continue;
            const Words kill = words(foe, "killable_by"), weak = words(foe, "weaknesses");
            need(!kill.empty() && std::includes(weak.begin(), weak.end(), kill.begin(), kill.end()),
                 unit.at("role_id").get<std::string>() + " killable_by not a non-empty subset of weaknesses");
        }
    }
    // ①
    const Json& one = data.battles[0];
    need(waveCount(one) == 3, "1 waves");
    need(foesOf(one, 0) == Units{"weiwu_hanggui", "weiwu_hanggui", "gui_yecha"}, "1 wave 0");
    need(foesOf(one, 1) == Units{"hui_ying", "shuangtou_guihu"}, "1 wave 1");
    need(foesOf(one, 2) == Units{"sanshou_guiwu"}, "1 wave 2");
    need(alliesOf(one) == Units{"zi_ling_xutian", "yuan_yao_heipao"}, "1 allies");
    need(words(role("weiwu_hanggui"), "weaknesses") == Words{kFire, kSword}, "weiwu weaknesses");
    need(words(role("gui_yecha"), "weaknesses") == Words{kWood, kFire}, "gui_yecha weaknesses");
    need(words(role("hui_ying"), "weaknesses") == Words{kMetal, kFire}, "hui_ying weaknesses");
    need(words(role("shuangtou_guihu"), "weaknesses") == Words{kSword, kWood}, "guihu weaknesses");
    need(words(role("sanshou_guiwu"), "weaknesses") == Words{kWood, kFire}, "sanshou weaknesses");
    need(role("sanshou_guiwu").value("killable_by", Json()) == Json::array({kWood, kFire}), "sanshou killable_by");
    need(role("sanshou_guiwu").value("charge", Json::object()).value("all", false) == true, "sanshou charge all");
    // ②
    const Json& two = data.battles[1];
    need(waveCount(two) == 3, "2 waves");
    need(foesOf(two, 0) == Units{"tiehuo_yiqun", "tiehuo_yiqun"}, "2 wave 0");
    need(foesOf(two, 1) == Units{"tiehuo_yiqun", "tiehuo_yiqun", "tiehuo_yiqun"}, "2 wave 1");
    need(foesOf(two, 2) == Units{"tiehuo_yiqun_da"}, "2 wave 2");
    need(alliesOf(two) == Units{"yuan_yao_jiedan"}, "2 allies");
    for (const char* id : {"tiehuo_yiqun", "tiehuo_yiqun_da"}) {
        need(role(id).value("killable_by", Json()) == Json::array({kWater, kMetal}), std::string(id) + " killable_by");
        need(words(role(id), "weaknesses") == Words{kWater, kMetal}, std::string(id) + " weaknesses");
    }
    need(role("tiehuo_yiqun_da").value("actions", 1) == 2, "swarm actions");
    need(role("tiehuo_yiqun_da").value("charge", Json::object()).value("all", false) == true, "swarm charge all");
    // ③
    const Json& three = data.battles[2];
    need(waveCount(three) == 2, "3 waves");
    need(foesOf(three, 0) == Units{"langxing_qiling"}, "3 wave 0");
    need(foesOf(three, 1) == Units{"langxing_qiling_yin"}, "3 wave 1");
    need(alliesOf(three) == Units{"wu_chou", "xuan_gu"}, "3 allies");
    need(words(role("langxing_qiling"), "weaknesses") == Words{kWater, kMetal} && !role("langxing_qiling").contains("killable_by"),
         "fire wolf");
    need(words(role("langxing_qiling_yin"), "weaknesses") == Words{kMetal}, "silver wolf weaknesses");
    need(role("langxing_qiling_yin").value("killable_by", Json()) == Json::array({kMetal}), "silver wolf killable_by");
    need(role("langxing_qiling_yin").value("charge", Json::object()).value("all", false) == true, "silver wolf charge all");
    for (const char* ally : {"wu_chou", "xuan_gu"})
        for (const auto& magic : role(ally).value("magics", Json::array())) {
            const auto found = data.magics.find(magic.get<std::string>());
            need(found != data.magics.end(), std::string(ally) + " magic missing " + magic.get<std::string>());
            if (found != data.magics.end())
                need((found->second.value("element", 0) & fanren::core::kElementMetal) == 0,
                     std::string(ally) + " carries a metal magic " + magic.get<std::string>());
        }
    // ④
    const Json& four = data.battles[3];
    need(foesOf(four, 0) == Units{"xuan_gu"} && waveCount(four) == 1 && alliesOf(four).empty(), "4 units");
    need(words(role("xuan_gu"), "weaknesses") == Words{kWood}, "xuan_gu weaknesses");
    need(role("xuan_gu").value("killable_by", Json()) == Json::array({kWood}), "xuan_gu killable_by");
    need(role("xuan_gu").value("charge", Json::object()).value("every", 0) == 3, "xuan_gu charge every");
    need(role("xuan_gu").value("actions", 1) == 2, "xuan_gu actions");
    need(role("xuan_gu").value("magics", Json::array()) == Json::array({"magic_moyan_huo"}), "xuan_gu magics (16.1 #40)");
    return out;
}

TEST(Ch12BattleData, TheFourMandatorySetupsMatchDesignEightPointTwo) {
    const auto found = errors(shipped());
    EXPECT_TRUE(found.empty()) << joined(found);
    EXPECT_FALSE(fs::exists(root() / "data" / "battles" / "b12_xutian_hudian_kuilei.json")) << "验收 4：占位编成已删";
}
TEST(Ch12BattleData, InMemoryMutationsOfWavesAlliesWeaknessesAndRewardsAreCaught) {
    const Data original = shipped();
    ASSERT_TRUE(errors(original).empty()) << joined(errors(original));
    for (int variant = 0; variant < 16; ++variant) {
        Data bad = original;
        switch (variant) {
        case 0: bad.battles[0]["units"].push_back({{"role_id", "hui_ying"}, {"faction", "enemy"}, {"wave", 2}}); break;
        case 1: bad.battles[0]["units"].erase(std::size_t{0}); break;                               // 少了紫灵
        case 2: bad.roles["sanshou_guiwu"]["killable_by"] = Json::array({kWood}); break;
        case 3: bad.roles["sanshou_guiwu"]["charge"]["all"] = false; break;
        case 4: bad.roles["tiehuo_yiqun"]["killable_by"] = Json::array({kWater, kMetal, kFire}); break;  // 不再 ⊆ 破绽
        case 5: bad.roles["tiehuo_yiqun_da"]["actions"] = 1; break;
        case 6: bad.battles[1]["units"][3]["wave"] = 0; break;                                     // 第 1 波少一群
        case 7: bad.roles["langxing_qiling"]["killable_by"] = Json::array({kWater}); break;
        case 8: bad.roles["wu_chou"]["magics"].push_back("magic_shijin_chongqun"); break;          // 友军配了金
        case 9: bad.roles["xuan_gu"]["magics"].push_back("magic_xiuluo_shenghuo"); break;          // 16.1 第 40 条
        case 10: bad.roles["xuan_gu"]["charge"]["every"] = 4; break;
        case 11: bad.battles[3]["units"].push_back({{"role_id", "wu_chou"}, {"faction", "ally"}}); break;
        case 12: bad.battles[3]["rewards"]["spirit_stones"] = 30; break;
        case 13: bad.battles[2]["rewards"]["drops"].push_back({{"item_id", "story_yu_ruyi"}, {"count", 1}, {"rate", 100}}); break;
        case 14: bad.battles[1]["can_escape"] = true; break;
        case 15: bad.roles["hui_ying"]["weaknesses"] = Json::array({kMetal}); break;
        }
        EXPECT_FALSE(errors(bad).empty()) << "mutation=" << variant;
    }
}

// ---- 第 7 条：同一 role 先友后敌（读数据那一半）----
// 第 11 章集成版 data/roles/wu_chou.json（HEAD 6ec8073）：726 字节，FNV-1a 64 = 0x55a3df0583774db6。
constexpr std::size_t kWuChouBytes = 726;
constexpr std::uint64_t kWuChouHash = 0x55a3df0583774db6ull;

Errors sameRoleErrors(const std::array<Json, 4>& battles, const std::vector<std::string>& roleIds, const std::string& wuChou) {
    Errors out;
    if (std::count(roleIds.begin(), roleIds.end(), "xuan_gu") != 1) out.push_back("xuan_gu is not exactly one role file");
    for (const auto& id : roleIds)
        if (id.rfind("xuan_gu_", 0) == 0) out.push_back("second Xuan Gu role " + id);
    for (const char* gone : {"xutian_hudian_kuilei", "duoding_xiushi"})
        if (std::count(roleIds.begin(), roleIds.end(), gone)) out.push_back(std::string("placeholder role still there: ") + gone);
    const auto factionOf = [](const Json& battle, const char* id) {
        std::set<std::string> factions;
        for (const auto& unit : battle.at("units"))
            if (unit.at("role_id") == id) factions.insert(unit.at("faction").get<std::string>());
        return factions;
    };
    if (factionOf(battles[2], "xuan_gu") != std::set<std::string>{"ally"}) out.push_back("xuan_gu is not an ally in 3");
    if (factionOf(battles[2], "wu_chou") != std::set<std::string>{"ally"}) out.push_back("wu_chou is not an ally in 3");
    if (factionOf(battles[3], "xuan_gu") != std::set<std::string>{"enemy"}) out.push_back("xuan_gu is not the enemy in 4");
    if (wuChou.size() != kWuChouBytes || fnv1a(wuChou) != kWuChouHash) out.push_back("wu_chou.json differs from chapter 11");
    return out;
}

std::vector<std::string> roleIds() {
    std::vector<std::string> ids;
    for (const auto& file : fs::recursive_directory_iterator(root() / "data" / "roles"))
        if (file.path().extension() == ".json") ids.push_back(json(file.path()).value("id", file.path().stem().string()));
    return ids;
}

TEST(Ch12BattleData, XuanGuIsOneRoleThatFightsBesideThenAgainstAndWuChouIsUntouched) {
    const auto found = sameRoleErrors(shipped().battles, roleIds(), read(root() / "data" / "roles" / "wu_chou.json"));
    EXPECT_TRUE(found.empty()) << joined(found);
}

TEST(Ch12BattleData, MutationsOfTheSameRoleCaseAreCaught) {
    const auto battles = shipped().battles;
    const auto ids = roleIds();
    const std::string wuChou = read(root() / "data" / "roles" / "wu_chou.json");
    ASSERT_TRUE(sameRoleErrors(battles, ids, wuChou).empty());
    auto twin = ids;
    twin.push_back("xuan_gu_enemy");
    EXPECT_FALSE(sameRoleErrors(battles, twin, wuChou).empty());
    auto placeholder = ids;
    placeholder.push_back("duoding_xiushi");
    EXPECT_FALSE(sameRoleErrors(battles, placeholder, wuChou).empty());
    auto turned = battles;
    for (auto& unit : turned[2]["units"]) if (unit.at("role_id") == "xuan_gu") unit["faction"] = "enemy";
    EXPECT_FALSE(sameRoleErrors(turned, ids, wuChou).empty());
    auto edited = wuChou;
    edited[edited.size() / 2] = edited[edited.size() / 2] == '1' ? '2' : '1';
    EXPECT_FALSE(sameRoleErrors(battles, ids, edited).empty());
}

// ---- 第 14 条：必打 4 场绕不过去（扫脚本 ＋ 读目标链）----
Errors battleCallErrors(const std::map<std::string, std::string>& sources) {
    Errors out;
    const std::map<std::string, std::string> owner{
        {"b12_guiwu_guiwang", "guiwang"}, {"b12_heishamo_tiehuoyi", "tihun"},
        {"b12_neidian_duoding", "ruyi"}, {"b12_hanlitai_xuangu", "xuangu"}};
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
    if (total != 4) out.push_back("battle( occurs " + std::to_string(total) + " times in chapter 12");
    return out;
}

TEST(Ch12BattleData, TheFourMandatoryBattlesHaveOneCallSiteEachAndGateTheNextStep) {
    const auto found = battleCallErrors(scripts());
    EXPECT_TRUE(found.empty()) << joined(found);
    // n13 / n21 / n29 / n31 的 set_flag 在目标链里，且是下一步挂点的 guard（3.1 表下第 2 条）。
    const auto steps = json(root() / "data" / "objectives" / "ch12.json").at("steps");
    ASSERT_EQ(steps.size(), 33u) << "施工图 3.4：目标链 33 步";
    struct Gate { const char* done; const char* object; const char* map; const char* next; };
    for (const auto& gate : {Gate{"ch12.guiwang", "trigger_guiwang", "ch12_guiwu", "trigger_huayuan"},
                             Gate{"ch12.tihun", "trigger_shamo", "ch12_binghuo", "trigger_jingzhu"},
                             Gate{"ch12.ruyi", "trigger_taijie", "ch12_hanlitai", "trigger_jitan"},
                             Gate{"ch12.xuangu", "trigger_dongkou", "ch12_hanlitai", "trigger_xiaojian"}}) {
        std::size_t index = steps.size();
        for (std::size_t i = 0; i < steps.size(); ++i) if (steps[i].at("done_flag") == gate.done) index = i;
        ASSERT_LT(index + 1, steps.size()) << gate.done;
        EXPECT_EQ(steps[index].at("target_object"), gate.object);
        EXPECT_EQ(steps[index + 1].at("target_object"), gate.next) << gate.done;
        const auto all = objects(json(root() / "maps" / (std::string(gate.map) + ".tmj")));
        EXPECT_EQ(properties(all.at(gate.object)).value("set_flag", ""), gate.done);
        EXPECT_EQ(properties(all.at(gate.next)).value("guard_flag", ""), gate.done) << gate.next;
    }
}

TEST(Ch12BattleData, AnExtraOrMovedBattleCallIsCaught) {
    const auto original = scripts();
    ASSERT_TRUE(battleCallErrors(original).empty());
    auto extra = original;
    extra["butian"] += "\nbattle(\"b12_neidian_duoding\")\n";
    EXPECT_FALSE(battleCallErrors(extra).empty());
    auto stranger = original;
    stranger["guairen"] += "\nlocal won = battle(\"b11_tiandu_shi\")\n";
    EXPECT_FALSE(battleCallErrors(stranger).empty());
    auto moved = original;
    const auto at = moved["xuangu"].find("battle(\"b12_hanlitai_xuangu\")");
    ASSERT_NE(at, std::string::npos);
    moved["xuangu"].replace(at, 6, "fight");
    moved["canhun"] += "\nbattle(\"b12_hanlitai_xuangu\")\n";
    EXPECT_FALSE(battleCallErrors(moved).empty());
    auto commented = original;
    commented["lingquan"] += "\n-- battle(\"b12_hanlitai_xuangu\")\n";
    EXPECT_TRUE(battleCallErrors(commented).empty()) << "注释里的不算";
}

// ---- 第 26 条：视觉数据走真实入口（handoff 第 6 节第 5 条）----
// 战斗：开战画面问的就是 battleBackdropFor（BattleScene.cpp 建 BattleView 那一行）；地图：WorldView::ensureMap 用
// engine().findAssets("art/maps/<id>", "meta.json") → io::loadMapMeta → mapVisualFrom，这里走同一个资产根、同一个函数。
// 判据抄施工图 8.2「战斗背景」与第 4 节表的视觉预设，不从 data/visual/*.json 读。
struct Backdrop { const char* battle; const char* map; const char* backdrop; };
constexpr Backdrop kBackdrops[] = {
    {"b12_guiwu_guiwang", "ch12_guiwu", "mountain_forest_dusk"},
    {"b12_heishamo_tiehuoyi", "ch12_binghuo", "cliff_top"},
    {"b12_neidian_duoding", "ch12_hanlitai", "secret_room"},
    {"b12_hanlitai_xuangu", "ch12_hanlitai", "secret_room"}};

struct Preset { const char* map; const char* theme; fanren::core::TimeOfDay time; const char* backdrop; };
constexpr fanren::core::TimeOfDay kDusk = fanren::core::TimeOfDay::Dusk, kIndoor = fanren::core::TimeOfDay::Indoor,
                                  kNight = fanren::core::TimeOfDay::Night, kDay = fanren::core::TimeOfDay::Day;
constexpr Preset kPresets[] = {
    {"ch12_tushan", "wild", kDusk, "mountain_forest_dusk"}, {"ch12_dixue", "cave", kIndoor, "cave_tunnel"},
    {"ch12_dadian", "indoor_stone", kIndoor, "secret_room"}, {"ch12_guiwu", "mountain_path", kNight, "mountain_forest"},
    {"ch12_xiaojie", "valley", kDay, "herb_valley"}, {"ch12_binghuo", "cliff", kDusk, "cliff_top_dusk"},
    {"ch12_baoguang", "indoor_stone", kIndoor, "secret_room"}, {"ch12_taqian", "indoor_stone", kIndoor, "secret_room"},
    {"ch12_hanlitai", "indoor_stone", kIndoor, "secret_room"}, {"ch12_lingquan", "indoor_stone", kIndoor, "secret_room"}};

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
    }
    // 第 4 节表：鬼雾加 mist（与 ch09_milin 同一组合）。
    if (const auto fog = metas.find("ch12_guiwu"); fog != metas.end()) {
        const auto layers = fanren::game::mapVisualFrom(fog->second).particles;
        if (std::none_of(layers.begin(), layers.end(), [](const auto& l) { return l.kind == fanren::engine::ParticleKind::Mist; }))
            out.push_back("ch12_guiwu has no mist");
    }
    return out;
}

class Ch12VisualWiring : public ::testing::Test {
protected:
    void SetUp() override {
        const auto ready = app_.init(root().string(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
    }
    void TearDown() override { app_.shutdown(); }
    fanren::game::Application app_;
};

TEST_F(Ch12VisualWiring, TheFourBattlesOpenOnTheBackdropsOfDesignEightPointTwo) {
    std::map<std::string, std::string> actual;
    for (const auto& row : kBackdrops) {
        const auto loaded = app_.loadMap(row.map, std::string{});
        ASSERT_TRUE(loaded.ok) << loaded.error;
        actual[row.battle] = fanren::game::battleBackdropFor(app_, row.battle);
    }
    const auto found = backdropErrors(actual);
    EXPECT_TRUE(found.empty()) << joined(found);
    auto wrong = actual;
    wrong["b12_heishamo_tiehuoyi"] = "cliff_top_dusk";
    EXPECT_FALSE(backdropErrors(wrong).empty()) << "负向：② 退回地图背景必须报";
    // 配对的负向：表里没有的一场在同一张图上取地图自己的背景——①② 的字面量与所在图不同，证明答话的是指派表
    // （③④ 与寒骊台同为 secret_room，分不出来，只靠上面那条字面量）。
    ASSERT_TRUE(app_.loadMap("ch12_guiwu", std::string{}).ok);
    EXPECT_EQ(fanren::game::battleBackdropFor(app_, "b12_no_such_battle"), "mountain_forest");
    ASSERT_TRUE(app_.loadMap("ch12_binghuo", std::string{}).ok);
    EXPECT_EQ(fanren::game::battleBackdropFor(app_, "b12_no_such_battle"), "cliff_top_dusk");
}

TEST_F(Ch12VisualWiring, TheTenMapsLoadThePresetsOfDesignSectionFour) {
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
        case 0: bad["ch12_guiwu"].backdrop = "mountain_forest_dusk"; break;
        case 1: bad["ch12_lingquan"].time = kDay; break;
        case 2: bad["ch12_xiaojie"].theme = "wild"; break;
        case 3: bad["ch12_guiwu"].particles.clear(); break;
        }
        EXPECT_FALSE(presetErrors(bad).empty()) << "mutation=" << variant;
    }
}
}  // namespace
