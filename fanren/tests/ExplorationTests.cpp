// 第 7 章试玩返修·路 U（docs/ch07-playtest-ui.md）：得物卡片、每张图的收集进度、集齐奖励。
//
// 用户 2026-11-20 试玩第 7 章，原话「找东西没啥回报感」。这里钉的是选定的那三件事：
//   · 开箱、采集（以及脚本 give()）弹一张得物卡片：图标、按品阶上色的名字、×数量、说明第一句；不挡路，几秒后自己淡出，
//     世界层按确认提前收；对话记录照旧记那一行「打开箱子，得了……」；
//   · 地名下（目标框底下）「宝箱 2/3 · 采集 1/2」：只数在场的，读档后保持；历练里不显示；
//   · 一张图找齐给一份奖励（data/exploration）：说一句、给东西、上场的伙伴加默契，只给一次，读档后不重复。
//
// 世界层的用例挂在真的 Application 上，资产根是一份临时拷贝（与 CompanionsWorldTests 同一个做法）：maps/ 里放一张探针图
// probe_explore（三个宝箱、一处采集点，其中一个宝箱要 probe.show），data/exploration/ 里给它配一份奖励。
// 期望值照选定的设计写死（卡片上写什么、几阶算哪一档、奖励几块），不从被测代码推。
#include <gtest/gtest.h>
#include <SDL3/SDL.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "TempDir.h"
#include "core/model/Types.h"
#include "game/Application.h"
#include "game/BattleHud.h"
#include "game/DialogueScene.h"
#include "game/Exploration.h"
#include "game/ItemToasts.h"
#include "game/ShotScenes.h"
#include "game/WorldScene.h"
#include "game/WorldView.h"
#include "io/DataLoader.h"
#include "io/ExplorationLoader.h"

namespace {

namespace fs = std::filesystem;
using fanren::core::Point;
using fanren::game::Application;
using fanren::game::BattleIcon;
using fanren::game::ItemToast;
using fanren::game::ItemToasts;
using fanren::game::WorldScene;

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

const fanren::core::GameData& shippedData() {
    static const fanren::core::GameData data = [] {
        auto loaded = fanren::io::loadGameData((repoRoot() / "data").string());
        EXPECT_TRUE(loaded.ok) << loaded.error;
        return loaded.value;
    }();
    return data;
}

// ===========================================================================
// 1. 卡片的几条纯规则
// ===========================================================================

TEST(ItemToastRules, GradesFallIntoFiveColourTiers) {
    // 0–1 凡品、2–3、4–5、6–7、8 以上（docs/ch07-playtest-ui.md 第 2 节那张表）。
    const int expected[] = {0, 0, 1, 1, 2, 2, 3, 3, 4, 4};
    for (int grade = 0; grade <= 9; ++grade) EXPECT_EQ(fanren::game::gradeTier(grade), expected[grade]) << grade;
    EXPECT_EQ(fanren::game::gradeTier(-1), 0);
    EXPECT_EQ(fanren::game::gradeTier(12), 4);
}

TEST(ItemToastRules, TheNoteIsTheFirstSentenceCutToFit) {
    using fanren::game::firstSentence;
    EXPECT_EQ(firstSentence("叶背泛黄的常见灵草，谷中阴湿处成片地长。年份越久，断口渗出的汁液越稠。", 21),
              "叶背泛黄的常见灵草，谷中阴湿处成片地长");
    EXPECT_EQ(firstSentence("画符用的丹砂，拿含灵气的兽血调过，落到符纸上还泛着一点光。", 21),
              "画符用的丹砂，拿含灵气的兽血调过，落到符…");
    EXPECT_EQ(firstSentence("真的吗？不是。", 21), "真的吗");
    EXPECT_EQ(firstSentence("一句没有句号的话", 21), "一句没有句号的话");
    EXPECT_EQ(firstSentence("", 21), "");
    EXPECT_EQ(firstSentence("恰好五个字。", 5), "恰好五个字") << "正好放得下不截";
    EXPECT_EQ(firstSentence("多出一个字了。", 5), "多出一个…");
}

TEST(ItemToastRules, TheIconFollowsTheKindOfThing) {
    const auto& data = shippedData();
    const auto icon = [&data](const char* id) { return fanren::game::itemIcon(data.findItem(id), id); };
    EXPECT_EQ(icon("pill_huiqi_dan"), BattleIcon::Medicine);
    EXPECT_EQ(icon("pill_qidu_shui"), BattleIcon::Poison) << "带毒的丹药是毒那一格";
    EXPECT_EQ(icon("herb_huangjing_cao"), BattleIcon::Wood);
    EXPECT_EQ(icon("material_dansha"), BattleIcon::Earth);
    EXPECT_EQ(icon("material_lingshi"), BattleIcon::Silver) << "钱是碎银那一格";
    EXPECT_EQ(icon("talisman_huoqiu_fu"), BattleIcon::Stance);
    EXPECT_EQ(icon("weapon_qingfeng_jian"), BattleIcon::Sword) << "兵器按它给的攻击类别";
    EXPECT_EQ(icon("weapon_zhishi_peidao"), BattleIcon::Blade);
    EXPECT_EQ(icon("formation_juling_zhen"), BattleIcon::BeadFull);
    EXPECT_EQ(icon("no_such_item"), BattleIcon::Chest) << "查不到的给宝箱那一格";
}

TEST(ItemToastRules, MoneyIsCalledByTheStageAndCarriesNoNote) {
    const auto& data = shippedData();
    fanren::core::GameState mortal;
    const ItemToast early = fanren::game::makeItemToast(data, mortal, "material_lingshi", 30);
    EXPECT_EQ(early.name, "碎银") << "凡人篇不许冒出「灵石」";
    EXPECT_EQ(early.count, 30);
    EXPECT_TRUE(early.note.empty()) << "灵石的说明里有「灵气」，不写";
    fanren::core::GameState immortal;
    immortal.setFlag("story.xiuxian_known");
    EXPECT_EQ(fanren::game::makeItemToast(data, immortal, "material_lingshi", 30).name, "灵石");

    const ItemToast pill = fanren::game::makeItemToast(data, immortal, "pill_zhuji_dan", 0);
    EXPECT_EQ(pill.name, "筑基丹");
    EXPECT_EQ(pill.count, 1) << "件数不到 1 按 1";
    EXPECT_EQ(pill.tier, 2) << "五阶";
    EXPECT_EQ(pill.note, "炼气修士冲击筑基所用的丹，能洗髓易筋");
    const ItemToast unknown = fanren::game::makeItemToast(data, immortal, "no_such_item", 2);
    EXPECT_EQ(unknown.name, "no_such_item") << "查不到回显 id，画面上一眼看出谁没进 data";
}

ItemToast card(const char* id) {
    ItemToast c;
    c.itemId = id;
    return c;
}

TEST(ItemToastRules, CardsStaggerInHoldThenFadeOnTheirOwn) {
    ItemToasts toasts;
    toasts.push(card("a"));
    toasts.push(card("b"));
    toasts.push(card("c"));
    ASSERT_EQ(toasts.cards().size(), 3u);
    EXPECT_FLOAT_EQ(toasts.cards()[0].age, 0.f);
    EXPECT_FLOAT_EQ(toasts.cards()[1].age, -0.18f) << "一次给几样：后一张晚 0.18 秒出场";
    EXPECT_FLOAT_EQ(toasts.cards()[2].age, -0.36f);
    EXPECT_FLOAT_EQ(ItemToasts::alpha(toasts.cards()[1]), 0.f) << "排队时看不见";

    toasts.advance(0.1f);
    EXPECT_NEAR(ItemToasts::alpha(toasts.cards()[0]), 0.5f, 1e-4f) << "淡入 0.2 秒";
    toasts.advance(1.0f);
    for (const ItemToast& c : toasts.cards()) EXPECT_FLOAT_EQ(ItemToasts::alpha(c), 1.f);
    // 第一张出场 3.4 秒后开始淡出，0.5 秒淡完。
    toasts.advance(2.55f);   // 第一张 3.65 秒
    EXPECT_NEAR(ItemToasts::alpha(toasts.cards()[0]), 0.5f, 1e-3f);
    toasts.advance(0.3f);    // 3.95：第一张淡完、移走
    ASSERT_EQ(toasts.cards().size(), 2u);
    EXPECT_EQ(toasts.cards()[0].itemId, "b");
    toasts.advance(1.0f);
    EXPECT_FALSE(toasts.active()) << "几秒之后自己没了，不用按键";
}

TEST(ItemToastRules, ConfirmPutsThemAwayQuicklyAndQueuedOnesNeverShow) {
    ItemToasts toasts;
    toasts.push(card("a"));
    toasts.push(card("b"));
    toasts.advance(0.1f);   // a 淡入一半，b 还在排队
    ASSERT_TRUE(toasts.dismissable());
    toasts.dismiss();
    EXPECT_FALSE(toasts.dismissable()) << "收起过的不再算可收";
    EXPECT_TRUE(toasts.active()) << "收起是一个很快的淡出，不是一下子没了";
    toasts.advance(0.09f);
    EXPECT_NEAR(ItemToasts::alpha(toasts.cards()[0]), 0.25f, 1e-3f) << "从收起那一刻的浓度，0.18 秒淡完";
    for (const ItemToast& c : toasts.cards()) {
        if (c.itemId == "b") EXPECT_FLOAT_EQ(ItemToasts::alpha(c), 0.f) << "排队的那张不再出场";
    }
    toasts.advance(0.1f);
    EXPECT_FALSE(toasts.active());
}

TEST(ItemToastRules, AtMostSixCardsTheOldestGoesFirst) {
    ItemToasts toasts;
    for (const char* id : {"1", "2", "3", "4", "5", "6", "7", "8"}) toasts.push(card(id));
    ASSERT_EQ(toasts.cards().size(), 6u);
    EXPECT_EQ(toasts.cards().front().itemId, "3");
    EXPECT_EQ(toasts.cards().back().itemId, "8");
}

TEST(ItemToastRules, CardsStackDownTheRightSideBelowTheSaveNotice) {
    const auto first = fanren::game::itemToastRect(0);
    const auto second = fanren::game::itemToastRect(1);
    EXPECT_FLOAT_EQ(first.x + first.w, 1280.f - 20.f) << "贴右边，留 20 像素";
    EXPECT_GE(first.y, 100.f) << "右上角的存盘提示之下";
    EXPECT_FLOAT_EQ(second.x, first.x);
    EXPECT_GT(second.y, first.y + first.h) << "往下排，不叠在一起";
    EXPECT_LE(fanren::game::itemToastRect(5).y + first.h, 720.f - 150.f) << "六张也压不到底下的对话框";
}

// ===========================================================================
// 2. 集齐奖励表的读取器（只查形状）
// ===========================================================================

class ExplorationLoader : public ::testing::Test {
protected:
    fanren::test::TempDir dir_{"fanren_exploration_loader"};
    void write(const char* name, const std::string& text) { writeFile(dir_.path() / name, text); }
    fanren::core::Result<std::map<std::string, fanren::core::ExplorationReward>> load() {
        return fanren::io::loadExplorationRewards(dir_.path().string());
    }
};

TEST_F(ExplorationLoader, AWellFormedFileReadsBackAsWritten) {
    write("ch99.json", R"({"note": "x", "maps": [
        {"map": "m_a", "done_flag": "f.a", "spirit_stones": 12, "items": [{"itemId": "i_one", "count": 2}], "bond": 1},
        {"map": "m_b", "done_flag": "f.b", "bond": 2}]})");
    const auto loaded = load();
    ASSERT_TRUE(loaded.ok) << loaded.error;
    ASSERT_EQ(loaded.value.size(), 2u);
    const auto& a = loaded.value.at("m_a");
    EXPECT_EQ(a.doneFlag, "f.a");
    EXPECT_EQ(a.spiritStones, 12);
    ASSERT_EQ(a.items.size(), 1u);
    EXPECT_EQ(a.items[0].itemId, "i_one");
    EXPECT_EQ(a.items[0].count, 2);
    EXPECT_EQ(a.bond, 1);
    const auto& b = loaded.value.at("m_b");
    EXPECT_EQ(b.spiritStones, 0) << "没写按 0";
    EXPECT_TRUE(b.items.empty());
    EXPECT_EQ(b.bond, 2);
}

TEST_F(ExplorationLoader, AMissingFolderMeansNothingIsConfigured) {
    const auto loaded = fanren::io::loadExplorationRewards((dir_.path() / "nowhere").string());
    ASSERT_TRUE(loaded.ok) << loaded.error;
    EXPECT_TRUE(loaded.value.empty());
}

TEST_F(ExplorationLoader, BadShapesAreRefusedWithTheReason) {
    const std::vector<std::pair<std::string, std::string>> cases = {
        {R"({"maps": []})", "maps"},
        {R"({"maps": [{"map": "m", "done_flag": "f", "bond": 1, "flag": "x"}]})", "表外字段 flag"},
        {R"({"maps": [{"map": "m", "bond": 1}]})", "done_flag"},
        {R"({"maps": [{"done_flag": "f", "bond": 1}]})", "map"},
        {R"({"maps": [{"map": "m", "done_flag": "f", "bond": 6}]})", "bond"},
        {R"({"maps": [{"map": "m", "done_flag": "f", "spirit_stones": -1}]})", "spirit_stones"},
        {R"({"maps": [{"map": "m", "done_flag": "f", "items": [{"itemId": "i", "count": 0}]}]})", "count"},
        {R"({"maps": [{"map": "m", "done_flag": "f", "items": [{"item": "i", "count": 1}]}]})", "表外字段 item"},
        {R"({"maps": [{"map": "m", "done_flag": "f"}]})", "一样奖励都没写"},
        {R"({"maps": [{"map": "m", "done_flag": "f", "bond": 1}], "extra": 1})", "表外字段 extra"},
    };
    for (const auto& [text, why] : cases) {
        write("bad.json", text);
        const auto loaded = load();
        EXPECT_FALSE(loaded.ok) << text;
        EXPECT_NE(loaded.error.find(why), std::string::npos) << text << " → " << loaded.error;
    }
}

TEST_F(ExplorationLoader, OneMapOrOneFlagConfiguredTwiceIsRefused) {
    write("a.json", R"({"maps": [{"map": "m", "done_flag": "f.a", "bond": 1}]})");
    write("b.json", R"({"maps": [{"map": "m", "done_flag": "f.b", "bond": 1}]})");
    auto loaded = load();
    EXPECT_FALSE(loaded.ok);
    EXPECT_NE(loaded.error.find("配了两遍"), std::string::npos) << loaded.error;
    write("b.json", R"({"maps": [{"map": "n", "done_flag": "f.a", "bond": 1}]})");
    loaded = load();
    EXPECT_FALSE(loaded.ok);
    EXPECT_NE(loaded.error.find("给了两张图"), std::string::npos) << loaded.error;
}

// ===========================================================================
// 3. 真树上的那一份：先只给第 7 章禁地六张图配（docs/ch07-playtest-ui.md 第 4 节的表，抄在这里）
// ===========================================================================

TEST(ExplorationData, TheForbiddenGroundsSixMapsCarryTheChosenSmallRewards) {
    struct Want {
        const char* map;
        const char* flag;
        int stones;
        const char* item;
    };
    const std::vector<Want> wants = {
        {"ch07_jindi_waiwei", "ch07.xunbian_waiwei", 10, "material_hanyan_cao"},
        {"ch07_jindi_zhongxin", "ch07.xunbian_zhongxin", 10, "material_huolong_cao"},
        {"ch07_huanxingshan", "ch07.xunbian_huanxingshan", 15, "material_huolong_cao"},
        {"ch07_dixia_zhaoze", "ch07.xunbian_zhaoze", 8, "material_hanyan_cao"},
        {"ch07_zhaoze_shenchu", "ch07.xunbian_shenchu", 12, "material_huolingshi"},
        {"ch07_dixia_shiku", "ch07.xunbian_shiku", 12, "material_huolingshi"},
    };
    const auto loaded = fanren::io::loadExplorationRewards((repoRoot() / "data" / "exploration").string());
    ASSERT_TRUE(loaded.ok) << loaded.error;
    ASSERT_EQ(loaded.value.size(), wants.size()) << "先只给第 7 章禁地那几张图配";
    std::ifstream flagsIn(repoRoot() / "data" / "flags.json", std::ios::binary);
    const std::string flags((std::istreambuf_iterator<char>(flagsIn)), std::istreambuf_iterator<char>());
    int stones = 0;
    for (const Want& w : wants) {
        ASSERT_EQ(loaded.value.count(w.map), 1u) << w.map;
        const auto& reward = loaded.value.at(w.map);
        EXPECT_EQ(reward.doneFlag, w.flag) << w.map;
        EXPECT_NE(flags.find(std::string("\"") + w.flag + "\""), std::string::npos) << w.flag << " 没在 data/flags.json 登记";
        EXPECT_EQ(reward.spiritStones, w.stones) << w.map;
        ASSERT_EQ(reward.items.size(), 1u) << w.map;
        EXPECT_EQ(reward.items[0].itemId, w.item) << w.map;
        EXPECT_EQ(reward.items[0].count, 1) << w.map;
        EXPECT_NE(shippedData().findItem(w.item), nullptr) << w.item;
        EXPECT_EQ(reward.bond, 1) << w.map << "：上场伙伴默契 +1";
        stones += reward.spiritStones;
        // 那张图在、上面有可数的东西（不然永远集不齐）。
        const auto map = fanren::io::loadTileMap((repoRoot() / "maps" / (std::string(w.map) + ".tmj")).string());
        ASSERT_TRUE(map.ok) << map.error;
        int countable = 0;
        for (const auto& object : map.value.objects) countable += object.type == "chest" || object.type == "gather" ? 1 : 0;
        EXPECT_GT(countable, 0) << w.map << " 上没有宝箱、采集点";
    }
    EXPECT_EQ(stones, 67) << "六张合计 67 块：章末约 450 块的一成半（docs/ch07-balance.md 第 7 节）";
}

// ===========================================================================
// 4. 世界层：卡片、进度、集齐奖励（探针图）
// ===========================================================================

constexpr int kW = 16;
constexpr int kH = 12;

std::string prop(const std::string& name, const std::string& value) {
    return R"({"name":")" + name + R"(","type":"string","value":")" + value + R"("})";
}
std::string propInt(const std::string& name, int value) {
    return R"({"name":")" + name + R"(","type":"int","value":)" + std::to_string(value) + "}";
}
std::string propBool(const std::string& name, bool value) {
    return R"({"name":")" + name + R"(","type":"bool","value":)" + (value ? "true" : "false") + "}";
}
std::string object(const std::string& name, const std::string& type, int x, int y, const std::vector<std::string>& props) {
    std::string list;
    for (const std::string& p : props) list += (list.empty() ? "" : ",") + p;
    return R"({"id":0,"name":")" + name + R"(","type":")" + type + R"(","x":)" + std::to_string(x * 32) +
           R"(,"y":)" + std::to_string(y * 32) + R"(,"width":32,"height":32,"rotation":0,"visible":true,"properties":[)" +
           list + "]}";
}

// 探针图：chest_a (2,4)、chest_b (6,4)、gather_a (4,4) 在场；chest_r (13,9) 要 probe.show；按键调查点 (8,8) 给东西。
std::string probeMap(const std::string& mapId) {
    std::string zeros;
    for (int i = 0; i < kW * kH; ++i) zeros += (i == 0 ? "0" : ",0");
    std::string layers;
    for (const char* name : {"ground", "overlay", "building", "front", "collision"}) {
        layers += R"({"name":")" + std::string(name) + R"(","type":"tilelayer","width":)" + std::to_string(kW) +
                  R"(,"height":)" + std::to_string(kH) + R"(,"data":[)" + zeros + "]},";
    }
    const std::vector<std::string> objects{
        object("spawn_main", "spawn", 1, 1, {prop("id", "spawn_main"), propBool("default", true), prop("facing", "down")}),
        object("chest_a", "chest", 2, 4, {prop("id", mapId + ".chest_a"), prop("loot", "material_dansha:2,material_fuzhi")}),
        object("chest_b", "chest", 6, 4, {prop("id", mapId + ".chest_b"), prop("loot", "pill_zhuji_dan,material_lianjing:3")}),
        object("gather_a", "gather", 4, 4,
               {prop("id", mapId + ".gather_a"), prop("item", "herb_huangjing_cao"), propInt("respawn_days", 3)}),
        object("chest_r", "chest", 13, 9,
               {prop("id", mapId + ".chest_r"), prop("loot", "material_fuzhi"), prop("require_flag", "probe.show")}),
        object("trigger_give", "trigger", 8, 8, {prop("script", "t/give.lua"), prop("mode", "interact"), propBool("once", false)}),
    };
    std::string list;
    for (const std::string& o : objects) list += (list.empty() ? "" : ",") + o;
    layers += R"({"name":"objects","type":"objectgroup","objects":[)" + list + "]}";
    return R"({"width":)" + std::to_string(kW) + R"(,"height":)" + std::to_string(kH) +
           R"(,"tilewidth":32,"tileheight":32,"orientation":"orthogonal","layers":[)" + layers + R"(],"properties":[)" +
           prop("map_id", mapId) + "," + prop("display_name_key", "probe.map.explore") + "," + prop("region", "probe") +
           "," + prop("bgm", "none") + "," + propInt("chapter", 7) + "," + propBool("outdoor", true) + "," +
           propBool("can_leave_edge", false) + "]}";
}

constexpr const char* kCompanionA = R"({"id": "probe_a", "met_flag": "probe.met_a", "roles": [{"role": "zhang_tie"}]})";
constexpr const char* kCompanionB = R"({"id": "probe_b", "met_flag": "probe.met_b", "roles": [{"role": "mo_daifu"}]})";

class ExplorationWorld : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        root_ = std::make_unique<fanren::test::TempDir>("fanren_exploration_world");
        for (const char* dir : {"data", "scripts", "maps"}) {
            fs::copy(repoRoot() / dir, root_->path() / dir, fs::copy_options::recursive);
        }
        // 真伙伴的角色与探针伙伴不重叠也无妨，但默契要量得干净：只留两位探针伙伴。
        fs::remove_all(root_->path() / "data" / "companions");
        writeFile(root_->path() / "data" / "companions" / "probe_a.json", kCompanionA);
        writeFile(root_->path() / "data" / "companions" / "probe_b.json", kCompanionB);
        writeFile(root_->path() / "maps" / "probe_explore.tmj", probeMap("probe_explore"));
        writeFile(root_->path() / "maps" / "lilian_probe_explore.tmj", probeMap("lilian_probe_explore"));
        writeFile(root_->path() / "data" / "text" / "probe_explore.json", R"({"probe.map.explore": "试炼小院"})");
        // 历练图也配一份：配了也不该发（历练里不显示、不发奖）。
        writeFile(root_->path() / "data" / "exploration" / "probe.json", R"({"maps": [
            {"map": "probe_explore", "done_flag": "probe.explored", "spirit_stones": 7,
             "items": [{"itemId": "material_huolong_cao", "count": 1}], "bond": 2},
            {"map": "lilian_probe_explore", "done_flag": "probe.explored_lilian", "spirit_stones": 7}]})");
        writeFile(root_->path() / "scripts" / "t" / "give.lua",
                  "give(\"material_huolong_cao\", 2)\ntalk(\"\", \"ui.world.gate.unknown\")\n");
    }
    static void TearDownTestSuite() { root_.reset(); }

    void SetUp() override {
        auto ready = app_.init(root_->path().string(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
        std::error_code ec;
        fs::remove(app_.quickSavePath(), ec);
        app_.state().setFlag("story.xiuxian_known");   // 修仙阶段：钱叫「灵石」
        ASSERT_TRUE(app_.loadMap("probe_explore", std::string{}).ok);
        app_.pushScene(std::make_unique<WorldScene>());
        tick();
        ASSERT_NE(world(), nullptr);
    }
    void TearDown() override {
        app_.shutdown();
        std::error_code ec;
        fs::remove(app_.quickSavePath(), ec);
    }

    void tick(int frames = 1) {
        for (int i = 0; i < frames; ++i) app_.tick(1.0 / 60.0);
    }
    void press(SDL_Scancode scancode) {
        SDL_Event event{};
        event.type = SDL_EVENT_KEY_DOWN;
        event.key.scancode = scancode;
        event.key.down = true;
        SDL_PushEvent(&event);
        tick();
        event.type = SDL_EVENT_KEY_UP;
        event.key.down = false;
        SDL_PushEvent(&event);
        tick();
    }
    WorldScene* world() { return dynamic_cast<WorldScene*>(app_.topScene()); }
    std::string top() {
        const auto* scene = app_.topScene();
        return scene == nullptr ? std::string{} : scene->name();
    }
    void stand(Point at, int facing) {
        app_.state().position = at;
        app_.state().facing = facing;
    }
    // 面朝那一格按确认（与玩家同一条路：WorldScene::interact），返回按了有没有反应。
    bool open(Point stand, int facing) {
        this->stand(stand, facing);
        EXPECT_NE(world(), nullptr) << "栈顶应是世界层，实际是 " << top();
        return world() != nullptr && world()->interact(app_);
    }
    std::string lastLine() const {
        return app_.dialogueLog().empty() ? std::string{} : app_.dialogueLog().back().body;
    }
    std::string progress() {
        EXPECT_NE(world(), nullptr);
        return world() == nullptr ? std::string{} : world()->describeFrame(app_).exploreLine;
    }
    std::vector<std::string> cardIds() const {
        std::vector<std::string> out;
        for (const ItemToast& c : app_.itemToasts().cards()) out.push_back(c.itemId);
        return out;
    }
    const ItemToast* cardOf(const std::string& id) const {
        for (const ItemToast& c : app_.itemToasts().cards()) {
            if (c.itemId == id) return &c;
        }
        return nullptr;
    }
    void saveAndReload() {
        const auto saved = app_.quickSave();
        ASSERT_TRUE(saved.ok) << saved.error;
        const auto loaded = app_.continueJourney(app_.quickSavePath());
        ASSERT_TRUE(loaded.ok) << loaded.error;
        tick();
        ASSERT_NE(world(), nullptr);
    }
    int count(const std::string& item) { return app_.state().itemCount(item); }
    void collectAll() {
        ASSERT_TRUE(open({2, 3}, 2));   // chest_a
        ASSERT_TRUE(open({6, 3}, 2));   // chest_b
        ASSERT_TRUE(open({4, 3}, 2));   // gather_a
    }

    static std::unique_ptr<fanren::test::TempDir> root_;
    Application app_;
};

std::unique_ptr<fanren::test::TempDir> ExplorationWorld::root_;

TEST_F(ExplorationWorld, OpeningAChestPopsACardForEachThingAndKeepsTheLogLine) {
    ASSERT_TRUE(open({6, 3}, 2));   // chest_b：筑基丹、炼晶×3
    EXPECT_EQ(top(), "World") << "不压对话框：拿到东西不用按键关";
    EXPECT_EQ(lastLine(), "打开箱子，得了筑基丹、炼晶×3。") << "对话记录里照旧记那一行";
    ASSERT_EQ(cardIds(), (std::vector<std::string>{"pill_zhuji_dan", "material_lianjing"}));
    const ItemToast* pill = cardOf("pill_zhuji_dan");
    EXPECT_EQ(pill->name, "筑基丹");
    EXPECT_EQ(pill->count, 1);
    EXPECT_EQ(pill->tier, 2) << "五阶 → 第 2 档（蓝）";
    EXPECT_EQ(pill->icon, BattleIcon::Medicine);
    EXPECT_EQ(pill->note, "炼气修士冲击筑基所用的丹，能洗髓易筋");
    const ItemToast* crystal = cardOf("material_lianjing");
    EXPECT_EQ(crystal->name, "炼晶");
    EXPECT_EQ(crystal->count, 3);
    EXPECT_EQ(crystal->tier, 4) << "八阶 → 第 4 档（橙金）";
    EXPECT_EQ(crystal->icon, BattleIcon::Earth);
    EXPECT_EQ(app_.theme().gradeColors[4].r, 255) << "第 4 档是橙金";

    ASSERT_TRUE(open({2, 3}, 2));   // chest_a：丹砂×2、符纸——凡品
    EXPECT_EQ(lastLine(), "打开箱子，得了丹砂×2、符纸。");
    ASSERT_NE(cardOf("material_dansha"), nullptr);
    EXPECT_EQ(cardOf("material_dansha")->count, 2);
    EXPECT_EQ(cardOf("material_dansha")->tier, 0);
    EXPECT_EQ(cardOf("material_dansha")->note, "画符用的丹砂，拿含灵气的兽血调过，落到符…") << "说明第一句，过长截断";
}

TEST_F(ExplorationWorld, GatheringPopsACardToo) {
    ASSERT_TRUE(open({4, 3}, 2));
    EXPECT_EQ(top(), "World");
    EXPECT_EQ(lastLine(), "采得黄精。");
    ASSERT_EQ(cardIds(), (std::vector<std::string>{"herb_huangjing_cao"}));
    EXPECT_EQ(cardOf("herb_huangjing_cao")->icon, BattleIcon::Wood);
    EXPECT_EQ(cardOf("herb_huangjing_cao")->note, "叶背泛黄的常见灵草，谷中阴湿处成片地长");
    // 采过的那一处再按：照旧说一句（那一句仍是对话框——只有得物改成了卡片）。
    tick(300);
    ASSERT_TRUE(open({4, 3}, 2));
    EXPECT_EQ(lastLine(), "这里刚采过，过些日子再来。");
    tick();
    EXPECT_EQ(top(), "Dialogue");
}

TEST_F(ExplorationWorld, TheCardsNeverBlockWalkingAndFadeOnTheirOwn) {
    ASSERT_TRUE(open({2, 3}, 2));
    tick();
    ASSERT_TRUE(app_.itemToasts().active());
    ASSERT_NE(world(), nullptr);
    EXPECT_TRUE(world()->tryStep(app_, 1, 0)) << "卡片亮着照样走";
    EXPECT_EQ(app_.state().position, (Point{3, 3}));
    tick(60 * 3);
    EXPECT_TRUE(app_.itemToasts().active()) << "三秒时还在（停 3.2 秒）";
    tick(60 * 2);
    EXPECT_FALSE(app_.itemToasts().active()) << "五秒内自己淡完，不用按任何键";
    EXPECT_EQ(top(), "World");
}

TEST_F(ExplorationWorld, ConfirmPutsTheCardsAwayWhenThereIsNothingElseToPress) {
    ASSERT_TRUE(open({2, 3}, 2));   // 人还面朝开空了的 chest_a
    tick();
    ASSERT_TRUE(app_.itemToasts().dismissable());
    const std::size_t lines = app_.dialogueLog().size();
    press(SDL_SCANCODE_RETURN);
    EXPECT_FALSE(app_.itemToasts().dismissable()) << "这一下收起了卡片";
    EXPECT_EQ(top(), "World") << "没有弹「箱子是空的」";
    EXPECT_EQ(app_.dialogueLog().size(), lines);
    tick(15);
    EXPECT_FALSE(app_.itemToasts().active()) << "收起是很快的一下";

    // 卡片亮着、面前却是另一个没开的箱子：照常开箱，不先吃掉这一下。
    ASSERT_TRUE(open({4, 3}, 2));   // gather_a
    tick();
    stand({6, 3}, 2);               // 面朝 chest_b
    press(SDL_SCANCODE_RETURN);
    EXPECT_EQ(app_.state().worldObjects.openedChests.count("probe_explore.chest_b"), 1u) << "面前有能按的就照常交互";
}

TEST_F(ExplorationWorld, AScriptGiveAlsoPopsACardWhileTheTalkGoesOn) {
    stand({8, 7}, 2);   // 面朝 (8,8) 的调查点
    ASSERT_TRUE(world()->interact(app_));
    tick(3);
    EXPECT_EQ(count("material_huolong_cao"), 2);
    ASSERT_EQ(cardIds(), (std::vector<std::string>{"material_huolong_cao"}));
    EXPECT_EQ(cardOf("material_huolong_cao")->count, 2);
    EXPECT_EQ(top(), "Dialogue") << "卡片不挡脚本：那一句照常压上来";
    for (int i = 0; i < 4 && app_.scripts().isRunning(); ++i) press(SDL_SCANCODE_RETURN);
    EXPECT_FALSE(app_.scripts().isRunning()) << "那一句按掉，脚本演完";
}

TEST_F(ExplorationWorld, ProgressCountsOnlyWhatIsHereAndSurvivesASave) {
    EXPECT_EQ(progress(), "宝箱 0/2 · 采集 0/1") << "chest_r 要 probe.show，还不在，不进分母";
    ASSERT_TRUE(open({2, 3}, 2));
    EXPECT_EQ(progress(), "宝箱 1/2 · 采集 0/1") << "开一个箱子 +1";
    app_.state().setFlag("probe.show");
    EXPECT_EQ(progress(), "宝箱 1/3 · 采集 0/1") << "置了旗标，它在了，就数进来";
    ASSERT_TRUE(open({4, 3}, 2));
    EXPECT_EQ(progress(), "宝箱 1/3 · 采集 1/1");

    saveAndReload();
    EXPECT_EQ(progress(), "宝箱 1/3 · 采集 1/1") << "读档后保持（「采过」从采集点的记录推，不另存）";
    app_.advanceDays(5);
    EXPECT_EQ(progress(), "宝箱 1/3 · 采集 1/1") << "冷却好了又能采，也不把「采过」退回去";
}

TEST_F(ExplorationWorld, FindingEverythingPaysOnceAndNeverAgain) {
    app_.state().party = {{"zhang_tie", -1, true}, {"mo_daifu", -1, false}};   // 张铁上场，墨大夫在队不上场
    const int stones = count("material_lingshi");
    ASSERT_TRUE(open({2, 3}, 2));
    ASSERT_TRUE(open({6, 3}, 2));
    EXPECT_EQ(app_.state().flag("probe.explored"), 0) << "还差一处采集点";
    EXPECT_EQ(top(), "World");
    ASSERT_TRUE(open({4, 3}, 2));   // 最后一处
    EXPECT_EQ(app_.state().flag("probe.explored"), 1);
    EXPECT_EQ(count("material_lingshi"), stones + 7);
    EXPECT_EQ(count("material_huolong_cao"), 1);
    EXPECT_EQ(app_.state().bondOf("probe_a"), 2) << "上场的伙伴默契 +2";
    EXPECT_EQ(app_.state().bondOf("probe_b"), 0) << "没上场的不加";
    EXPECT_EQ(lastLine(), "试炼小院能找的都找到了，又在角落里拾得灵石×7、火龙草。张铁与你更有默契了。");
    tick();
    EXPECT_EQ(top(), "Dialogue") << "集齐那一句压对话框（一张图只有一次）";
    // 两个箱子四张、采集一张、奖励两张，一共七张：至多六张，最早那张（丹砂）挤掉。奖励的两张排在最后。
    EXPECT_EQ(cardIds(), (std::vector<std::string>{"material_fuzhi", "pill_zhuji_dan", "material_lianjing",
                                                   "herb_huangjing_cao", "material_lingshi", "material_huolong_cao"}));
    ASSERT_NE(cardOf("material_lingshi"), nullptr) << "奖励也弹卡片";
    EXPECT_EQ(cardOf("material_lingshi")->count, 7);
    EXPECT_EQ(cardOf("material_lingshi")->name, "灵石");
    app_.popScene();
    tick();
    EXPECT_EQ(progress(), "宝箱 2/2 · 采集 1/1 · 已寻遍");
    EXPECT_TRUE(world()->describeFrame(app_).exploreComplete);

    // 新冒出一个箱子（旗标置了）：又不齐了；再开齐，不再发。
    app_.state().setFlag("probe.show");
    EXPECT_EQ(progress(), "宝箱 2/3 · 采集 1/1");
    ASSERT_TRUE(open({13, 8}, 2));
    EXPECT_EQ(progress(), "宝箱 3/3 · 采集 1/1 · 已寻遍");
    EXPECT_EQ(count("material_lingshi"), stones + 7) << "只奖一次";
    EXPECT_EQ(app_.state().bondOf("probe_a"), 2);
    EXPECT_EQ(top(), "World");

    // 读档后也不重复：旗标进存档。冷却好了再采一遍，也不再发。
    saveAndReload();
    EXPECT_EQ(app_.state().flag("probe.explored"), 1);
    app_.advanceDays(5);
    ASSERT_TRUE(open({4, 3}, 2));
    EXPECT_EQ(count("material_lingshi"), stones + 7);
    EXPECT_EQ(app_.state().bondOf("probe_a"), 2);
    EXPECT_EQ(top(), "World");
}

TEST_F(ExplorationWorld, InsideATrialThereIsNoProgressLineAndNoReward) {
    fanren::game::LilianEntry entry;
    entry.siteId = "probe_site";
    entry.mapId = "lilian_probe_explore";
    const auto entered = app_.enterLilian(entry);
    ASSERT_TRUE(entered.ok) << entered.error;
    tick();
    ASSERT_TRUE(app_.inLilian());
    EXPECT_EQ(progress(), "") << "历练里不显示（没抽中的宝箱记成已开，数出来是假的）";
    const int stones = count("material_lingshi");
    ASSERT_TRUE(open({2, 3}, 2));
    ASSERT_TRUE(open({6, 3}, 2));
    ASSERT_TRUE(open({4, 3}, 2));
    EXPECT_FALSE(app_.itemToasts().cards().empty()) << "卡片照弹";
    EXPECT_EQ(app_.state().flag("probe.explored_lilian"), 0) << "历练里不发集齐奖励（每趟都能再集齐一次）";
    EXPECT_EQ(count("material_lingshi"), stones);
    EXPECT_EQ(top(), "World");
    ASSERT_TRUE(app_.leaveLilian().ok);
}

TEST_F(ExplorationWorld, AMapWithoutChestsOrGatherPointsShowsNoLine) {
    ASSERT_TRUE(app_.loadMap("ch01_hanjiacun", std::string{}).ok);
    app_.replaceScene(std::make_unique<WorldScene>());
    tick();
    EXPECT_EQ(progress(), "");
}

TEST_F(ExplorationWorld, TheShotWalkCanPressConfirmAndWaitForTheCards) {
    // 截图口 walk:<串> 的 Z（按确认）与 .（等 0.25 秒）：自查卡片样子的截图就是这么拍的。
    stand({2, 3}, 2);
    int warmup = 0;
    ASSERT_TRUE(fanren::game::pushCompanionShot(app_, "walk", "Z.", warmup));
    EXPECT_EQ(app_.state().worldObjects.openedChests.count("probe_explore.chest_a"), 1u) << "Z 就是面朝前方按确认";
    ASSERT_EQ(cardIds(), (std::vector<std::string>{"material_dansha", "material_fuzhi"}));
    // Z 走 1 帧、「.」走 15 帧：第二张晚 0.18 秒出场，此刻出场 16/60 − 0.18 秒，还在淡入。
    EXPECT_LT(ItemToasts::alpha(app_.itemToasts().cards()[1]), 1.f);
    ASSERT_TRUE(fanren::game::pushCompanionShot(app_, "walk", ".", warmup));
    EXPECT_FLOAT_EQ(ItemToasts::alpha(app_.itemToasts().cards()[1]), 1.f) << "再等一刻，排队的那张也已淡入";
    EXPECT_FALSE(fanren::game::pushCompanionShot(app_, "walk", "Q", warmup)) << "认不出的字照旧报错";
}

TEST_F(ExplorationWorld, ANewJourneyOrALoadStartsWithNoCardsLeftOver) {
    ASSERT_TRUE(open({2, 3}, 2));
    ASSERT_TRUE(app_.itemToasts().active());
    saveAndReload();
    EXPECT_FALSE(app_.itemToasts().active()) << "读档前那一局拿到的，不在读回来的这一局里弹";
}

}  // namespace
