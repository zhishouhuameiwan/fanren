// 第 7 章试玩返修·路 L：专属掉落、宝箱加码、丹方、采集物入丹（docs/ch07-playtest-loot.md）。
//
// 用户 2026-11-20 试玩第 7 章，原话「找东西没啥回报感」「打完怪物都是兽材，也不知道用来干啥，应该要区分开」。
// 拍板：每种妖兽掉专属材料（说明里写清用途）、地火屋加几张用得上它们的丹方、宝箱放更好的东西、寒烟草 / 火龙草 /
// 火灵石能炼成有用的丹。
//
// 期望值一律照 docs/ch07-playtest-loot.md 的表写死，不从被测数据推：
//   1. 每一场 be07 / bl07 掉的恰是这一种妖兽的专属材料（对照表），一种妖兽一样、七样两两不同；兽材不再掉；
//      支线 Z2 东坡那一场拆成编成一样、不掉东西的 be07_dongpo_tiebi_yuan；
//   2. 七样材料的品阶、价钱、收药摊收价、说明里写的用途；
//   3. 四张丹方在地火屋（facility_dihuo，炉品 4，ch07.feidan 起）用对应的料炼得出来，四味丹的药效；
//   4. 南宫婉、陈巧倩的喜好；
//   5. 正史 11 个、历练 10 个宝箱的内容。
#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "core/battle/Battle.h"
#include "core/model/Types.h"
#include "core/rules/Companions.h"
#include "core/rules/Crafting.h"
#include "core/rules/Economy.h"
#include "core/rules/FieldItems.h"
#include "game/AlchemyScene.h"
#include "game/Application.h"
#include "game/BattleScene.h"
#include "game/WorldObjects.h"
#include "io/DataLoader.h"

namespace {

namespace fs = std::filesystem;

using fanren::core::BattleSetup;
using fanren::core::BattleUnitSpec;
using fanren::core::GameState;
using fanren::core::Item;
using fanren::core::MapObject;
using fanren::core::battle::BattlePhase;
using fanren::game::AlchemyScene;
using fanren::game::Application;
using fanren::rules::CraftKind;
using fanren::rules::GiftTaste;

std::string assetRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "data" / "battles" / "be07_dongpo_tiebi_yuan.json")) return candidate;
    }
    return ".";
}

std::string readFile(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

// ---------------------------------------------------------------------------
// 表一：七种妖兽各一样专属材料（docs/ch07-playtest-loot.md 第 1 节）
// ---------------------------------------------------------------------------
struct MaterialSpec {
    const char* species;    // 角色 id（历练分档的 _t2 / _t3 去掉后缀）
    const char* itemId;
    const char* name;
    int grade;
    int price;
    int yaotanPays;         // 坊市收药摊（sellRate 0.3）收一件给几块
    const char* useWord;    // 说明里必须写到的用途
};

const std::vector<MaterialSpec>& materials() {
    static const std::vector<MaterialSpec> kTable = {
        {"hanbing_chan", "material_hanchan_su", "寒蟾酥", 1, 16, 5, "寒蟾丹"},
        {"cai_e", "material_caie_linfen", "彩蛾鳞粉", 1, 24, 7, "女修"},
        {"fei_she", "material_feishe_dan", "飞蛇胆", 1, 18, 5, "调元丸"},
        {"huoyan_shu", "material_huoshu_rong", "火鼠绒", 1, 16, 5, "火元丹"},
        {"tiebi_yuan", "material_tieyuan_gu", "铁猿臂骨", 2, 40, 12, "坊市"},
        {"tuishan_shou", "material_tuishan_ya", "推山兽獠牙", 2, 46, 14, "坊市"},
        {"bishui_e", "material_bie_lin", "碧鳄鳞", 3, 90, 27, "碧鳞丹"},
    };
    return kTable;
}

// ---------------------------------------------------------------------------
// 表二：每一场 be07 / bl07 掉什么、几份（第 2 节：明雷几只掉几份，推山兽一对獠牙，精英 3 片；历练第几档掉几份）
// ---------------------------------------------------------------------------
struct DropSpec {
    const char* battle;
    const char* itemId;
    int count;
};

const std::vector<DropSpec>& drops() {
    static const std::vector<DropSpec> kTable = {
        {"be07_cai_e", "material_caie_linfen", 3},
        {"be07_hanbing_chan", "material_hanchan_su", 2},
        {"be07_zhaoze_chan", "material_hanchan_su", 3},
        {"be07_fei_she", "material_feishe_dan", 2},
        {"be07_shenchu_she", "material_feishe_dan", 3},
        {"be07_huoyan_shu", "material_huoshu_rong", 3},
        {"be07_shenchu_shu", "material_huoshu_rong", 4},
        {"be07_tiebi_yuan", "material_tieyuan_gu", 2},
        {"be07_tuishan_shou", "material_tuishan_ya", 2},
        {"be07_bishui_e", "material_bie_lin", 3},
        {"bl07_hanbing_chan_t1", "material_hanchan_su", 1},
        {"bl07_hanbing_chan_t2", "material_hanchan_su", 2},
        {"bl07_hanbing_chan_t3", "material_hanchan_su", 3},
        {"bl07_fei_she_t1", "material_feishe_dan", 1},
        {"bl07_fei_she_t2", "material_feishe_dan", 2},
        {"bl07_fei_she_t3", "material_feishe_dan", 3},
        {"bl07_huoyan_shu_t1", "material_huoshu_rong", 1},
        {"bl07_huoyan_shu_t2", "material_huoshu_rong", 2},
        {"bl07_huoyan_shu_t3", "material_huoshu_rong", 3},
        {"bl07_bishui_e_t1", "material_bie_lin", 1},
        {"bl07_bishui_e_t2", "material_bie_lin", 2},
        {"bl07_bishui_e_t3", "material_bie_lin", 3},
    };
    return kTable;
}

// 历练分档的角色（bishui_e_t2、fei_she_t3……）归到本体那一种。
std::string speciesOf(const std::string& roleId) {
    for (const char* suffix : {"_t2", "_t3"}) {
        const std::string tail = suffix;
        if (roleId.size() > tail.size() && roleId.compare(roleId.size() - tail.size(), tail.size(), tail) == 0) {
            return roleId.substr(0, roleId.size() - tail.size());
        }
    }
    return roleId;
}

// ---------------------------------------------------------------------------
// 表三：四张丹方（第 3 节）
// ---------------------------------------------------------------------------
struct RecipeSpec {
    const char* recipe;
    const char* pill;
    const char* pillName;
    int productCount;
    std::vector<std::pair<const char*, int>> inputs;
    int restoreHp;
    int restoreMp;
};

const std::vector<RecipeSpec>& recipes() {
    static const std::vector<RecipeSpec> kTable = {
        {"recipe_hanchan_dan", "pill_hanchan_dan", "寒蟾丹", 2,
         {{"material_hanchan_su", 2}, {"material_hanyan_cao", 1}}, 70, 0},
        {"recipe_huoyuan_dan", "pill_huoyuan_dan", "火元丹", 2,
         {{"material_huoshu_rong", 2}, {"material_huolong_cao", 1}}, 0, 45},
        {"recipe_tiaoyuan_wan", "pill_tiaoyuan_wan", "调元丸", 2,
         {{"material_feishe_dan", 1}, {"material_huolong_cao", 1}}, 35, 30},
        {"recipe_bilin_dan", "pill_bilin_dan", "碧鳞丹", 1,
         {{"material_bie_lin", 1}, {"material_huolingshi", 1}, {"material_hanyan_cao", 1}}, 100, 40},
    };
    return kTable;
}

class Ch07Loot : public ::testing::Test {
protected:
    void SetUp() override {
        auto ready = app_.init(assetRoot(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
    }
    void TearDown() override { app_.shutdown(); }

    std::vector<MapObject> objectsOn(const std::string& mapId) {
        auto loaded = fanren::io::loadTileMap((fs::path(assetRoot()) / "maps" / (mapId + ".tmj")).string());
        EXPECT_TRUE(loaded.ok) << mapId << "：" << loaded.error;
        return loaded.ok ? loaded.value.objects : std::vector<MapObject>{};
    }

    Application app_;
};

// ===========================================================================
// 1. 每一场 be07 / bl07 掉的恰是这一种妖兽的专属材料
// ===========================================================================
TEST_F(Ch07Loot, EveryBeastFightDropsThatBeastsOwnMaterial) {
    std::map<std::string, std::string> materialOf;
    std::set<std::string> distinct;
    for (const MaterialSpec& m : materials()) {
        materialOf[m.species] = m.itemId;
        distinct.insert(m.itemId);
    }
    ASSERT_EQ(materialOf.size(), 7u) << "先验：第 7 章明雷与历练里一共七种妖兽";
    EXPECT_EQ(distinct.size(), 7u) << "一种妖兽一样，七样两两不同（用户：「应该要区分开」）";

    for (const DropSpec& d : drops()) {
        const BattleSetup* setup = app_.battleSetup(d.battle);
        ASSERT_NE(setup, nullptr) << d.battle;
        ASSERT_EQ(setup->reward.drops.size(), 1u) << d.battle << "：只掉一样";
        EXPECT_EQ(setup->reward.drops[0].itemId, d.itemId) << d.battle;
        EXPECT_EQ(setup->reward.drops[0].count, d.count) << d.battle;
        // 这一场的敌方都是同一种妖兽，掉的正是那一种的材料。
        for (const BattleUnitSpec& unit : setup->units) {
            if (unit.ally) continue;
            const auto it = materialOf.find(speciesOf(unit.roleId));
            ASSERT_NE(it, materialOf.end()) << d.battle << " 的 " << unit.roleId << " 不在七种妖兽的表里";
            EXPECT_EQ(it->second, d.itemId) << d.battle << "：" << unit.roleId << " 的专属材料不是这一样";
        }
    }

    // data/battles 里每一份 be07 / bl07 都在上表里——漏一份就可能还在掉兽材；只有 Z2 东坡那一场例外（不掉）。
    std::set<std::string> listed;
    for (const DropSpec& d : drops()) listed.insert(d.battle);
    int seen = 0;
    for (const auto& entry : fs::directory_iterator(fs::path(assetRoot()) / "data" / "battles")) {
        const std::string stem = entry.path().stem().string();
        if (stem.rfind("be07_", 0) != 0 && stem.rfind("bl07_", 0) != 0) continue;
        ++seen;
        const BattleSetup* setup = app_.battleSetup(stem);
        ASSERT_NE(setup, nullptr) << stem;
        for (const auto& drop : setup->reward.drops) {
            EXPECT_NE(drop.itemId, "material_yaoshou_cai") << stem << "：兽材不再掉（第 2 节）";
        }
        if (stem == "be07_dongpo_tiebi_yuan") continue;
        EXPECT_EQ(listed.count(stem), 1u) << stem << " 不在掉落对照表里";
    }
    EXPECT_EQ(seen, 23) << "先验：be07 十一份（含 Z2 东坡）＋ bl07 十二份";

    // 兽材那个物品留着：旧存档里可能还有，读档不能因为它不见了而坏。
    const Item* old = app_.data().findItem("material_yaoshou_cai");
    ASSERT_NE(old, nullptr) << "兽材的物品文件保留（旧存档）";
    EXPECT_TRUE(old->tradeable);
}

// 打赢了真的进背包：发奖励走的是 BattleScene 结算用的同一个函数（只有赢了才发）。
TEST_F(Ch07Loot, WinningTheThreeOnceEmptyHandedFightsNowPaysOut) {
    const struct {
        const char* battle;
        const char* itemId;
        int count;
    } kThree[] = {{"be07_huoyan_shu", "material_huoshu_rong", 3},
                  {"be07_tiebi_yuan", "material_tieyuan_gu", 2},
                  {"be07_tuishan_shou", "material_tuishan_ya", 2}};
    for (const auto& row : kThree) {
        const BattleSetup* setup = app_.battleSetup(row.battle);
        ASSERT_NE(setup, nullptr) << row.battle;
        GameState state;
        const auto lost = fanren::game::grantBattleReward(state, setup->reward, BattlePhase::Escaped);
        EXPECT_TRUE(lost.empty()) << row.battle << "：逃了不发";
        EXPECT_EQ(state.itemCount(row.itemId), 0);
        const auto won = fanren::game::grantBattleReward(state, setup->reward, BattlePhase::Won);
        ASSERT_EQ(won.drops.size(), 1u) << row.battle << "：从前 drops 空，打赢什么也不给";
        EXPECT_EQ(state.itemCount(row.itemId), row.count) << row.battle;
        EXPECT_EQ(state.itemCount("material_lingshi"), 0) << row.battle << "：灵石照旧 0（第 9 节第 3 条）";
    }
}

// 支线 Z2 东坡那一场：编成与环形山明雷 be07_tiebi_yuan 一模一样，不掉东西（第一侧最小路线打它，交接存档的背包不许变）。
// 它不在 Ch07BattleData 那张 8.2 表里——两份的敌方相同，「认得出」那一条会把它当撞车；通关测试的 identifyBattle
// 认出来是 be07_tiebi_yuan，那一步（Z2a）的背包照施工图另核，不读掉落，认错不碍事。
TEST_F(Ch07Loot, TheDongpoApesAreTheRingMountainApesWithoutTheLoot) {
    const BattleSetup* dongpo = app_.battleSetup("be07_dongpo_tiebi_yuan");
    const BattleSetup* ring = app_.battleSetup("be07_tiebi_yuan");
    ASSERT_NE(dongpo, nullptr);
    ASSERT_NE(ring, nullptr);
    const auto foes = [](const BattleSetup& s) {
        std::multiset<std::string> out;
        for (const BattleUnitSpec& u : s.units) out.insert((u.ally ? "ally:" : "") + u.roleId + "@" + std::to_string(u.wave));
        return out;
    };
    EXPECT_EQ(foes(*dongpo), (std::multiset<std::string>{"tiebi_yuan@0", "tiebi_yuan@0"})) << "3.1 表 Z2a：门口两只铁臂猿";
    EXPECT_EQ(foes(*dongpo), foes(*ring));
    EXPECT_TRUE(dongpo->reward.drops.empty()) << "Z2 东坡那一场不掉东西";
    EXPECT_EQ(dongpo->reward.cultivation, 5) << "修为与拆开之前一样";
    EXPECT_EQ(ring->reward.cultivation, 5);
    EXPECT_EQ(dongpo->reward.spiritStones, 0);
    EXPECT_TRUE(dongpo->canEscape) << "3.1 表 Z2a：可逃";
    EXPECT_FALSE(dongpo->defeatIsFatal);
    EXPECT_EQ(dongpo->chapter, 7);
    EXPECT_EQ(app_.battleBackdrop("be07_dongpo_tiebi_yuan"), "hd07_mountain_forest") << "环形山，白天";

    const std::string script = readFile(fs::path(assetRoot()) / "scripts" / "ch07" / "yujian_dongpo.lua");
    EXPECT_NE(script.find("battle(\"be07_dongpo_tiebi_yuan\")"), std::string::npos) << "东坡的脚本打拆出来的那一份";
    EXPECT_EQ(script.find("battle(\"be07_tiebi_yuan\")"), std::string::npos);
    int foeTiebi = 0;
    for (const MapObject& object : objectsOn("ch07_huanxingshan")) {
        if (object.type != "foe" || object.name != "foe_tiebi") continue;
        ++foeTiebi;
        EXPECT_EQ(object.property("battle"), "be07_tiebi_yuan") << "环形山的明雷还是掉臂骨的那一份";
    }
    EXPECT_EQ(foeTiebi, 1);
}

// ===========================================================================
// 2. 七样材料：品阶、价钱、收药摊收价、说明里写的用途
// ===========================================================================
TEST_F(Ch07Loot, TheSevenMaterialsAreGradedPricedAndSayWhatTheyAreFor) {
    const fanren::rules::Shop* yaotan = app_.shop("ch07_fangshi_yaotan");
    ASSERT_NE(yaotan, nullptr);
    for (const MaterialSpec& m : materials()) {
        const Item* item = app_.data().findItem(m.itemId);
        ASSERT_NE(item, nullptr) << m.itemId;
        EXPECT_EQ(item->name, m.name) << m.itemId;
        EXPECT_EQ(item->kind, fanren::core::ItemKind::Material) << m.itemId;
        EXPECT_EQ(item->grade, m.grade) << m.itemId;
        EXPECT_EQ(item->price, m.price) << m.itemId;
        EXPECT_TRUE(item->tradeable) << m.itemId << "：照常能卖";
        EXPECT_FALSE(fanren::core::battleUsable(*item)) << m.itemId << "：材料不是药";
        EXPECT_EQ(fanren::rules::sellPrice(*yaotan, *item, 0), m.yaotanPays) << m.itemId << " 在收药摊卖几块";
        const std::string desc = app_.data().lookupText(item->descKey);
        ASSERT_NE(desc, item->descKey) << m.itemId << " 的说明查不到";
        EXPECT_NE(desc.find(m.useWord), std::string::npos) << m.itemId << " 的说明没写到用途「" << m.useWord << "」：" << desc;
    }
    // 拉开档次：兽材收 1 块；小兽 5–7；臂骨、獠牙十几；碧鳄鳞最贵。
    const Item* old = app_.data().findItem("material_yaoshou_cai");
    ASSERT_NE(old, nullptr);
    EXPECT_EQ(fanren::rules::sellPrice(*yaotan, *old, 0), 1);
}

// ===========================================================================
// 3. 四张丹方：地火屋（炉品 4，ch07.feidan 起）用对应的料炼得出来
// ===========================================================================
TEST_F(Ch07Loot, TheFourRecipesBrewAtTheEarthFireFurnaceFromTheirMaterials) {
    // 第 7 章唯一的丹炉：地火屋那只圆墩。
    int furnaces = 0;
    int grade = 0;
    for (const MapObject& object : objectsOn("ch07_dihuo")) {
        if (object.type != "facility" || object.name != "facility_dihuo") continue;
        ++furnaces;
        EXPECT_EQ(object.property("kind"), "alchemy");
        EXPECT_EQ(object.property("require_flag"), "ch07.feidan");
        grade = std::stoi(object.property("grade"));
    }
    ASSERT_EQ(furnaces, 1);
    ASSERT_EQ(grade, 4) << "地火屋的炉品";

    GameState& state = app_.state();
    state = GameState{};
    state.alchemyProficiency = 100;   // 第 7 章的炼丹熟练度（Ch07AcceptanceTests：施工图第 2 节）
    const auto listed = [&]() {
        AlchemyScene panel(CraftKind::Alchemy, grade);
        panel.onEnter(app_);
        return panel.recipeIds();
    };
    for (const std::string& id : listed()) {
        for (const RecipeSpec& r : recipes()) EXPECT_NE(id, r.recipe) << "ch07.feidan 之前地火屋还没开炉，面板上不该有";
    }
    state.setFlag("ch07.feidan");
    const std::vector<std::string> open = listed();
    for (const RecipeSpec& r : recipes()) {
        EXPECT_EQ(std::count(open.begin(), open.end(), std::string(r.recipe)), 1) << r.recipe << "：推门进十九号就列出来";
    }

    for (const RecipeSpec& r : recipes()) {
        // 方子本身：产物、几份、几味料。
        const auto found = app_.recipes().find(r.recipe);
        ASSERT_NE(found, app_.recipes().end()) << r.recipe;
        EXPECT_EQ(found->second.productId, r.pill);
        EXPECT_EQ(found->second.productCount, r.productCount) << r.recipe;
        EXPECT_EQ(found->second.requireFlag, "ch07.feidan") << r.recipe;
        std::multiset<std::pair<std::string, int>> want;
        std::multiset<std::pair<std::string, int>> have;
        for (const auto& [id, n] : r.inputs) want.insert({id, n});
        for (const auto& in : found->second.inputs) have.insert({in.itemId, in.count});
        EXPECT_EQ(have, want) << r.recipe << " 的料";

        // 真开炉：料各备五炉份，炼到成一炉为止。
        state.bag.clear();
        for (const auto& [id, n] : r.inputs) state.addItem(id, n * 5, 0);
        AlchemyScene panel(CraftKind::Alchemy, grade);
        panel.onEnter(app_);
        const auto& ids = panel.recipeIds();
        const auto row = std::find(ids.begin(), ids.end(), std::string(r.recipe));
        ASSERT_NE(row, ids.end()) << r.recipe;
        int tries = 0;
        while (state.itemCount(r.pill) == 0 && tries < 5) {
            ASSERT_TRUE(panel.craftAt(app_, static_cast<int>(row - ids.begin()))) << r.recipe << "：" << panel.feedback();
            ++tries;
        }
        EXPECT_EQ(state.itemCount(r.pill), r.productCount) << r.recipe << " 五炉内该成一炉：" << panel.feedback();
        for (const auto& [id, n] : r.inputs) {
            EXPECT_EQ(state.itemCount(id), n * (5 - tries)) << r.recipe << "：每炉扣 " << id << " ×" << n;
        }

        // 炼出来的丹：药效照第 3 节，战斗里、战斗外都用得上，能卖。
        const Item* pill = app_.data().findItem(r.pill);
        ASSERT_NE(pill, nullptr) << r.pill;
        EXPECT_EQ(pill->name, r.pillName);
        EXPECT_EQ(pill->kind, fanren::core::ItemKind::Pill);
        EXPECT_EQ(pill->restoreHp, r.restoreHp) << r.pill;
        EXPECT_EQ(pill->restoreMp, r.restoreMp) << r.pill;
        EXPECT_TRUE(pill->tradeable) << r.pill;
        EXPECT_TRUE(fanren::core::battleUsable(*pill)) << r.pill;
        EXPECT_TRUE(fanren::rules::fieldUsable(*pill)) << r.pill;
        EXPECT_NE(app_.data().lookupText(pill->descKey), pill->descKey) << r.pill << " 的说明查不到";
    }

    // 三样采集物都接上了炼丹（用户：「寒烟草、火龙草、火灵石能在地火屋炼成有用的丹」）。
    std::set<std::string> gathered;
    for (const RecipeSpec& r : recipes()) {
        for (const auto& [id, n] : r.inputs) gathered.insert(id);
    }
    for (const char* herb : {"material_hanyan_cao", "material_huolong_cao", "material_huolingshi"}) {
        EXPECT_EQ(gathered.count(herb), 1u) << herb << " 没有一张方子用它";
    }
}

// ===========================================================================
// 4. 南宫婉、陈巧倩的喜好
// ===========================================================================
TEST_F(Ch07Loot, TheTwoCompanionsLikeTheMaterialsThatSuitThem) {
    const auto& companions = app_.data().companions;
    ASSERT_EQ(companions.count("nangong_wan"), 1u);
    ASSERT_EQ(companions.count("chen_qiaoqian"), 1u);
    const fanren::core::Companion& nangong = companions.at("nangong_wan");
    const fanren::core::Companion& chen = companions.at("chen_qiaoqian");

    // 南宫婉：原有的火灵石、寒烟草 ＋ 火鼠绒（她修火法）、彩蛾鳞粉。
    for (const char* id : {"material_huolingshi", "material_hanyan_cao", "material_huoshu_rong", "material_caie_linfen"}) {
        EXPECT_EQ(fanren::rules::giftTaste(nangong, id), GiftTaste::Liked) << "南宫婉喜欢 " << id;
    }
    // 陈巧倩：原有的火龙草、养精丹 ＋ 彩蛾鳞粉。
    for (const char* id : {"material_huolong_cao", "pill_yangjing_dan", "material_caie_linfen"}) {
        EXPECT_EQ(fanren::rules::giftTaste(chen, id), GiftTaste::Liked) << "陈巧倩喜欢 " << id;
    }
    EXPECT_EQ(fanren::rules::giftTaste(chen, "material_huoshu_rong"), GiftTaste::Plain) << "火鼠绒只合南宫婉";
    for (const char* id : {"material_hanchan_su", "material_feishe_dan", "material_tieyuan_gu", "material_tuishan_ya",
                           "material_bie_lin"}) {
        EXPECT_EQ(fanren::rules::giftTaste(nangong, id), GiftTaste::Plain) << id;
        EXPECT_EQ(fanren::rules::giftTaste(chen, id), GiftTaste::Plain) << id;
    }
    // 兽材照旧不喜（旧存档里可能还有）。
    EXPECT_EQ(fanren::rules::giftTaste(nangong, "material_yaoshou_cai"), GiftTaste::Disliked);
    EXPECT_EQ(fanren::rules::giftTaste(chen, "material_yaoshou_cai"), GiftTaste::Disliked);

    // 送出去真的按合意记账：+5（契约 2.2），寻常 +2。
    fanren::core::BondRecord a;
    auto out = fanren::rules::giveGift(a, nangong, "material_huoshu_rong", 40);
    EXPECT_TRUE(out.given);
    EXPECT_EQ(out.gained, 5);
    fanren::core::BondRecord b;
    out = fanren::rules::giveGift(b, chen, "material_caie_linfen", 40);
    EXPECT_EQ(out.gained, 5);
    fanren::core::BondRecord c;
    out = fanren::rules::giveGift(c, chen, "material_tuishan_ya", 40);
    EXPECT_EQ(out.gained, 2);
}

// ===========================================================================
// 5. 宝箱：正史 11 个、历练 10 个（第 4 节那张表）
// ===========================================================================
struct ChestSpec {
    const char* map;
    const char* name;
    std::vector<std::pair<const char*, int>> loot;
};

const std::vector<ChestSpec>& storyChests() {
    static const std::vector<ChestSpec> kTable = {
        {"ch07_jindi_waiwei", "chest_yaxia", {{"talisman_huoqiu_fu", 1}, {"material_lingshi", 15}, {"pill_jinchuang_yao", 1}}},
        {"ch07_jindi_waiwei", "chest_tanbei", {{"pill_huiqi_dan", 1}, {"talisman_tulao_fu", 1}, {"material_lingshi", 25}}},
        {"ch07_jindi_zhongxin", "chest_linbian", {{"pill_yangjing_dan", 1}, {"material_lingshi", 10}}},
        {"ch07_jindi_zhongxin", "chest_huayuan", {{"material_huolong_cao", 2}, {"pill_huiqi_dan", 1}, {"material_lingshi", 15}}},
        {"ch07_huanxingshan", "chest_shanjiao", {{"pill_yangjing_dan", 1}, {"talisman_huoqiu_fu", 2}}},
        {"ch07_huanxingshan", "chest_beipo", {{"material_huolong_cao", 1}, {"material_lingshi", 20}, {"pill_jinchuang_yao", 1}}},
        {"ch07_huanxingshan", "chest_dongpo", {{"pill_huiqi_dan", 1}, {"material_lingshi_zhong", 1}}},
        {"ch07_dixia_zhaoze", "chest_dongan", {{"pill_huiqi_dan", 1}, {"material_hanyan_cao", 2}, {"material_lingshi", 15}}},
        {"ch07_zhaoze_shenchu", "chest_ewan",
         {{"material_lingshi", 50}, {"material_lingshi_zhong", 2}, {"talisman_tulao_fu", 1}, {"material_huolingshi", 2}}},
        {"ch07_dixia_shiku", "chest_xiaoshi",
         {{"material_lingshi", 40}, {"material_lingshi_zhong", 2}, {"talisman_huoqiu_fu", 2}, {"pill_yangjing_dan", 1}}},
        {"ch07_dixia_shiku", "chest_shishi", {{"pill_yangjing_dan", 1}, {"material_lingshi", 15}, {"pill_jinchuang_yao", 1}}},
    };
    return kTable;
}

const std::vector<ChestSpec>& lilianChests() {
    static const std::vector<ChestSpec> kTable = {
        {"lilian_jindi_waiwei", "chest_yaxia", {{"material_lingshi", 8}, {"material_hanyan_cao", 1}}},
        {"lilian_jindi_waiwei", "chest_tanbei", {{"pill_huiqi_dan", 1}, {"talisman_huoqiu_fu", 1}}},
        {"lilian_jindi_zhongxin", "chest_linbian", {{"pill_jinchuang_yao", 1}, {"material_lingshi", 6}}},
        {"lilian_jindi_zhongxin", "chest_huayuan", {{"material_lingshi", 10}, {"material_huolong_cao", 1}}},
        {"lilian_jindi_huanxingshan", "chest_shanjiao", {{"pill_yangjing_dan", 1}, {"material_lingshi", 6}}},
        {"lilian_jindi_huanxingshan", "chest_beipo", {{"material_lingshi", 8}, {"material_huolong_cao", 1}}},
        {"lilian_jindi_zhaoze", "chest_dongan", {{"material_lingshi", 8}, {"pill_huiqi_dan", 1}}},
        {"lilian_jindi_shenchu", "chest_ewan", {{"material_lingshi", 25}, {"material_huolingshi", 1}, {"talisman_huoqiu_fu", 1}}},
        {"lilian_jindi_shiku", "chest_xiaoshi", {{"material_lingshi", 30}, {"pill_yangjing_dan", 1}, {"talisman_huoqiu_fu", 1}}},
        {"lilian_jindi_shiku", "chest_shishi", {{"pill_jinchuang_yao", 1}, {"material_lingshi", 8}}},
    };
    return kTable;
}

TEST_F(Ch07Loot, TheChestsHoldTheUpgradedLoot) {
    for (const auto* table : {&storyChests(), &lilianChests()}) {
        std::map<std::string, int> chestsPerMap;
        for (const ChestSpec& c : *table) ++chestsPerMap[c.map];
        for (const auto& [mapId, count] : chestsPerMap) {
            int onMap = 0;
            for (const MapObject& object : objectsOn(mapId)) {
                if (object.type == "chest") ++onMap;
            }
            EXPECT_EQ(onMap, count) << mapId << "：图上的宝箱都在表里";
        }
        for (const ChestSpec& c : *table) {
            const std::string tag = std::string(c.map) + "/" + c.name;
            int found = 0;
            for (const MapObject& object : objectsOn(c.map)) {
                if (object.type != "chest" || object.name != c.name) continue;
                ++found;
                bool ok = false;
                std::map<std::string, int> have;
                for (const auto& stack : fanren::game::worldobj::parseItemList(object.property("loot"), ok)) {
                    have[stack.itemId] += stack.count;
                    EXPECT_NE(app_.data().findItem(stack.itemId), nullptr) << tag << "：" << stack.itemId << " 不存在";
                }
                ASSERT_TRUE(ok) << tag << " 的 loot 写坏了";
                std::map<std::string, int> want;
                for (const auto& [id, n] : c.loot) want[id] += n;
                EXPECT_EQ(have, want) << tag;
                // 普通宝箱至少一样不是金疮药（用户：「找东西没啥回报感」）。
                const bool onlySalve = std::all_of(have.begin(), have.end(),
                                                   [](const auto& kv) { return kv.first == "pill_jinchuang_yao"; });
                EXPECT_FALSE(onlySalve) << tag << " 里只有金疮药";
            }
            EXPECT_EQ(found, 1) << tag;
        }
    }
    // 精英守着的、藏得深的：放了符箓或中阶灵石，灵石也是成堆的（第 4 节）。
    for (const ChestSpec& c : storyChests()) {
        const std::string name = c.name;
        if (name != "chest_tanbei" && name != "chest_dongpo" && name != "chest_ewan" && name != "chest_xiaoshi") continue;
        const bool better = std::any_of(c.loot.begin(), c.loot.end(), [](const auto& kv) {
            const std::string id = kv.first;
            return id.rfind("talisman_", 0) == 0 || id == "material_lingshi_zhong";
        });
        EXPECT_TRUE(better) << c.map << "/" << name << "：精英守着的、藏得深的要放符箓或中阶灵石";
    }
    // 正史十一个箱子一共给多少灵石、中阶灵石（docs/ch07-playtest-loot.md 第 5 节的估算按这两个数）。
    int lingshi = 0;
    int zhong = 0;
    for (const ChestSpec& c : storyChests()) {
        for (const auto& [id, n] : c.loot) {
            if (std::string(id) == "material_lingshi") lingshi += n;
            if (std::string(id) == "material_lingshi_zhong") zhong += n;
        }
    }
    EXPECT_EQ(lingshi, 205);
    EXPECT_EQ(zhong, 5);
}

}  // namespace
