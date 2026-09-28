// 第 5 章的经济账（docs/ch05-design.md 第 9 节、验收 12）——直接读 scripts/ch05/*.lua 与
// data/battles/b05_*.json、data/shops/，判据是第 9 节那两张表的原文数字。
//
// ---------------------------------------------------------------------------
// 为什么要有这个文件（第 4 章复验 N-1 / 二次复验 R-4）
// ---------------------------------------------------------------------------
// 第 4 章两次栽在药上：一次是测试替玩家把料补齐（药从哪来没人看着），一次是账只看药材、
// 脚本直接发成药没人发现。施工图第 9 节因此写死三条：
//   1. 本章脚本**不直接 give 养精丹或清灵散**；成药只有炼出来一条路；
//   2. 碎银与修为的进出只有上两表那几处；每一处 take 都看返回值；十一张编成的 rewards 与 8.2 字面量相符；
//   3. 从 P0 = 0 出发的自产能力写进测试打印，**不设门槛**。
// 形状与通关测试不同：不走位、不起脚本、不开炉，只读字。
//
// 这个文件看不见什么：
//   · 药铺卖的草药几年份——商店条目没有年份字段（施工偏差 18.2 第 3 条），第 9 节「20 年」这一格判不了；
//   · 玩家自己去药铺买卖（那是玩家的动作，不是脚本的进出）。
#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "core/rules/Crafting.h"
#include "game/Application.h"

namespace {

namespace fs = std::filesystem;

std::string assetRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        const fs::path root(candidate);
        if (fs::exists(root / "scripts" / "ch05" / "huanyu.lua") &&
            fs::exists(root / "data" / "shops" / "ch05_nancheng_yaopu.json")) {
            return candidate;
        }
    }
    return ".";
}

std::string readFile(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

// 一次物品进出：哪个脚本、第几行、是 give 还是 take、物品、数量（字面量；写成变量的记 -1）、那一行是不是 if 的条件。
struct Flow {
    std::string script;
    int line = 0;
    bool give = false;
    std::string item;
    int count = 0;
    int age = 0;
    bool inIf = false;
};

// 读一个脚本里所有的 give / take（注释不算）。读取器自己另有一条自检用例。
std::vector<Flow> flowsIn(const std::string& script, const std::string& source) {
    std::vector<Flow> out;
    static const std::regex kCall("\\b(give|take)\\(\"([a-z0-9_]+)\"(?:,\\s*([A-Za-z0-9_]+))?(?:,\\s*([0-9]+))?\\)");
    std::istringstream in(source);
    std::string line;
    int number = 0;
    while (std::getline(in, line)) {
        ++number;
        const std::size_t dash = line.find("--");
        const std::string code = dash == std::string::npos ? line : line.substr(0, dash);
        for (auto it = std::sregex_iterator(code.begin(), code.end(), kCall); it != std::sregex_iterator(); ++it) {
            Flow f;
            f.script = script;
            f.line = number;
            f.give = (*it)[1] == "give";
            f.item = (*it)[2];
            const std::string count = (*it)[3].matched ? std::string((*it)[3]) : std::string("1");
            f.count = std::regex_match(count, std::regex("[0-9]+")) ? std::stoi(count) : -1;
            f.age = (*it)[4].matched ? std::stoi((*it)[4]) : 0;
            const std::size_t first = code.find_first_not_of(" \t");
            const std::string head = first == std::string::npos ? std::string{} : code.substr(first);
            f.inIf = head.rfind("if ", 0) == 0 || head.rfind("elseif ", 0) == 0;
            out.push_back(f);
        }
    }
    return out;
}

std::vector<Flow> chapterFlows(const std::string& root, int& scripts) {
    std::vector<Flow> all;
    scripts = 0;
    for (const auto& entry : fs::directory_iterator(fs::path(root) / "scripts" / "ch05")) {
        if (entry.path().extension() != ".lua") continue;
        ++scripts;
        const auto flows = flowsIn(entry.path().filename().string(), readFile(entry.path()));
        all.insert(all.end(), flows.begin(), flows.end());
    }
    return all;
}

class Ch05Ledger : public ::testing::Test {
protected:
    void SetUp() override {
        root_ = assetRoot();
        auto ready = app_.init(root_, /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
        flows_ = chapterFlows(root_, scripts_);
        ASSERT_GE(scripts_, 25) << "先验：scripts/ch05/ 读得到";
        ASSERT_GE(flows_.size(), 10u) << "先验：本章的 give / take 读得到（分母不能塌）";
    }
    void TearDown() override { app_.shutdown(); }

    fanren::game::Application app_;
    std::string root_;
    int scripts_ = 0;
    std::vector<Flow> flows_;
};

// ---------------------------------------------------------------------------
// 第 9 节第 1 条：本章脚本不直接 give 养精丹或清灵散；十一张编成不掉药；药铺不卖成药
// ---------------------------------------------------------------------------
TEST_F(Ch05Ledger, No1_PillsComeOnlyFromTheFurnaceNeverFromAScriptAFightOrTheShop) {
    for (const Flow& f : flows_) {
        if (!f.give) continue;
        EXPECT_NE(f.item, "pill_yangjing_dan") << f.script << " 第 " << f.line << " 行直接发养精丹（施工图第 9 节第 1 条）";
        EXPECT_NE(f.item, "pill_qingling_san") << f.script << " 第 " << f.line << " 行直接发清灵散（施工图第 9 节第 1 条）";
    }
    for (const char* id : {"b05_yesu_yelang", "b05_heishuixiang", "b05_matou_zhuishao", "b05_tiequanhui_jieren",
                           "b05_duobang", "b05_yange_qiecuo", "b05_mofu_shigui", "b05_wu_jianming",
                           "b05_xunzhuang", "b05_ouyang_feitian", "b05_xiaoxiangyuan"}) {
        const fanren::core::BattleSetup* setup = app_.battleSetup(id);
        ASSERT_NE(setup, nullptr) << id;
        EXPECT_TRUE(setup->reward.drops.empty()) << id << " 掉东西（施工图 8.2：掉落一律不写）";
    }
    // 新商店 ch05_nancheng_yaopu：卖黄精、紫参与金疮药，**不卖清灵散**（第 9 节）。
    const fanren::rules::Shop* shop = app_.shop("ch05_nancheng_yaopu");
    ASSERT_NE(shop, nullptr) << "施工图第 9 节：新商店 ch05_nancheng_yaopu";
    std::set<std::string> sold;
    for (const auto& entry : shop->entries) sold.insert(entry.itemId);
    const std::set<std::string> design = {"herb_huangjing_cao", "herb_zishen_cao", "pill_jinchuang_yao"};
    EXPECT_EQ(sold, design) << "施工图第 9 节：药铺卖黄精、紫参与金疮药（不卖清灵散，也就不卖养精丹）";
}

// ---------------------------------------------------------------------------
// 第 9 节第 2 条：碎银只有那三处去向；每一处 take 都在 if 里
// ---------------------------------------------------------------------------
// 去向表（第 9 节原文）：3a 给孙二狗一袋 −20；4a 再赏一小袋 −10；7b 见面礼·碎银 −20。
// 3a 另有「不够一袋时改说把身上剩的都给了」（施工图 3.2），那一处扣的是变量，记成「剩下的全部」。
// 来源表：碎银只从十场必打里的 ③④⑦ 与潇湘院条件战来——脚本不 give 碎银。
TEST_F(Ch05Ledger, No2_SilverLeavesOnlyAtTheThreePlacesOfTheTableAndEveryTakeIsChecked) {
    std::multiset<int> taken;
    int remainder = 0;
    for (const Flow& f : flows_) {
        if (f.item != "material_lingshi") continue;
        EXPECT_FALSE(f.give) << f.script << " 第 " << f.line << " 行 give 碎银：第 9 节的来源表里脚本不发钱";
        if (f.give) continue;
        if (f.count < 0) {
            ++remainder;
        } else {
            taken.insert(f.count);
        }
    }
    const std::multiset<int> design = {20, 10, 20};
    EXPECT_EQ(taken, design) << "施工图第 9 节去向表：碎银只有 −20（3a）、−10（4a）、−20（7b）三处";
    EXPECT_EQ(remainder, 1) << "3a「不够一袋把剩的都给了」那一处（施工图 3.2）";
    // 每一处 take 都在 if 里（第 9 节第 2 条、验收 12）：扣不下时脚本得知道。
    int takes = 0;
    for (const Flow& f : flows_) {
        if (f.give) continue;
        ++takes;
        EXPECT_TRUE(f.inIf) << f.script << " 第 " << f.line << " 行的 take(\"" << f.item
                            << "\") 没放在 if 里：扣不下时照样往下演（施工图第 9 节第 2 条）";
    }
    EXPECT_GE(takes, 10) << "先验：本章的 take 数得出来";
}

// 修为与碎银的来源（第 9 节来源表）：十场必打 ＋10（③）＋15（④）＋15（⑦）＝ ＋40 碎银，
// 修为 10＋40＋30＋40＋0＋30＋90＋60＋40＋100 ＝ ＋440；潇湘院条件战 ＋30 / ＋60。
TEST_F(Ch05Ledger, No2_TheTenMustFightsPayFortySilverAndFourHundredFortyCultivation) {
    int silver = 0;
    int cultivation = 0;
    for (const char* id : {"b05_yesu_yelang", "b05_heishuixiang", "b05_matou_zhuishao", "b05_tiequanhui_jieren",
                           "b05_duobang", "b05_yange_qiecuo", "b05_mofu_shigui", "b05_wu_jianming",
                           "b05_xunzhuang", "b05_ouyang_feitian"}) {
        const fanren::core::BattleSetup* setup = app_.battleSetup(id);
        ASSERT_NE(setup, nullptr) << id;
        silver += setup->reward.spiritStones;
        cultivation += setup->reward.cultivation;
    }
    EXPECT_EQ(silver, 40) << "施工图第 9 节：十场必打共 ＋40 碎银";
    EXPECT_EQ(cultivation, 440) << "施工图第 9 节：十场必打共 ＋440 修为";
    const fanren::core::BattleSetup* brothel = app_.battleSetup("b05_xiaoxiangyuan");
    ASSERT_NE(brothel, nullptr);
    EXPECT_EQ(brothel->reward.spiritStones, 30) << "施工图第 9 节：潇湘院条件战 ＋30";
    EXPECT_EQ(brothel->reward.cultivation, 60) << "施工图第 9 节：潇湘院条件战 ＋60";
}

// 年份药材的来源（第 9 节来源表：Z1 医书 黄精 6 / 紫参 3，40 年）。
// 从前这一条只扫脚本、断言「只有 Z1 那一处」；路径求购 fengwu_qiugou 另卖两株二十年紫参，
// 那句话已经不真（复验 LOW-d）。口径改成两处都进账：脚本里只有 Z1；路径行动里只有那两株紫参。
// 账的结论不变：养精丹一炉黄精 2、紫参 1（都要 20 年以上，recipe_yangjing_dan），黄精只有 Z1 的 6 株，
// 于是至多 3 炉——多出来的紫参添不了炉，No3 那里的 kFurnaces = 3 照旧成立。
TEST_F(Ch05Ledger, No2_YearHerbsComeFromTheBookForFengwuAndHerTwoRootsOnly) {
    using Herbs = std::multiset<std::tuple<std::string, int, int>>;
    Herbs fromScripts;
    for (const Flow& f : flows_) {
        if (!f.give || f.item.rfind("herb_", 0) != 0) continue;
        fromScripts.insert({f.item, f.count, f.age});
    }
    const Herbs z1 = {{"herb_huangjing_cao", 6, 40}, {"herb_zishen_cao", 3, 40}};
    EXPECT_EQ(fromScripts, z1) << "施工图第 9 节：脚本里的年份药材只有 Z1 那一处（黄精 6 / 紫参 3，40 年）";

    Herbs fromPaths;
    int actions = 0;
    for (const fanren::core::PathAction& action : app_.data().pathActions) {
        if (action.chapter != 5) continue;
        ++actions;
        std::vector<fanren::core::BagEntry> entries = action.gives;
        entries.insert(entries.end(), action.rewardItems.begin(), action.rewardItems.end());
        if (action.kind == fanren::core::PathActionKind::Purchase) entries.push_back(action.goods);
        for (const fanren::core::BagEntry& e : entries) {
            if (e.itemId.rfind("herb_", 0) == 0) fromPaths.insert({e.itemId, e.count, e.herbAge});
        }
    }
    ASSERT_GE(actions, 10) << "先验：第 5 章的路径行动读得到（data/pathactions/ch05.json）";
    const Herbs purchase = {{"herb_zishen_cao", 2, 20}};
    EXPECT_EQ(fromPaths, purchase) << "路径行动里的年份药材只有墨凤舞求购那两株二十年紫参（fengwu_qiugou）";

    int huangjing = 0;
    int zishen = 0;
    for (const Herbs* herbs : {&fromScripts, &fromPaths}) {
        for (const auto& [item, count, age] : *herbs) {
            if (age < 20) continue;
            if (item == "herb_huangjing_cao") huangjing += count;
            if (item == "herb_zishen_cao") zishen += count;
        }
    }
    EXPECT_EQ(huangjing, 6) << "黄精只有 Z1 那 6 株";
    EXPECT_EQ(std::min(huangjing / 2, zishen), 3) << "全章年份药材至多够 3 炉养精丹（黄精是瓶颈）";
}

// ---------------------------------------------------------------------------
// 第 9 节第 3 条：从 P0 = 0 出发的自产能力（写进打印，不设门槛）
// ---------------------------------------------------------------------------
// 施工图原话：「M0 − 50 ＋ 40 块买料、Z1 三炉，按 Crafting.cpp 算期望与 5% 分位」。
// 实情（施工偏差 18.2 第 3 条）：药铺卖的是零年药材，养精丹方要 20 年、清灵散方要 10 年，
// 零年的买来也下不了炉（掌天瓶也催不熟未足年的苗）——买料那一半在今天的内容上是 0 炉。
// 剩下 Z1 那三炉（黄精 6、紫参 3，40 年；养精丹每炉黄精 2、紫参 1）。
// 熟练度按第 4 章终局 fixture 的 100、资质 50、客栈药炉 1 品（施工图第 4 节）算。
TEST_F(Ch05Ledger, No3_SelfSufficiencyFromZeroPillsIsPrintedNotGated) {
    const auto found = app_.recipes().find("recipe_yangjing_dan");
    ASSERT_NE(found, app_.recipes().end());
    const fanren::rules::Recipe& pill = found->second;
    constexpr int kFurnaces = 3;        // Z1：黄精 6 / 2、紫参 3 / 1
    constexpr int kProficiency = 100;   // 第 4 章终局 fixture 两侧都是 100（施工偏差 18.4）
    constexpr int kAptitude = 50;
    constexpr int kFurnaceGrade = 1;    // 客栈药炉 1 品
    const int chance = fanren::rules::successChance(pill, kProficiency, kAptitude, kFurnaceGrade);
    ASSERT_GT(chance, 0);
    // 三炉独立（熟练度已到顶，涨不涨都不改成功率）：二项分布。
    const double p = chance / 100.0;
    double dist[kFurnaces + 1] = {};
    for (int k = 0; k <= kFurnaces; ++k) {
        double c = 1.0;
        for (int i = 0; i < k; ++i) c = c * (kFurnaces - i) / (i + 1);
        double v = c;
        for (int i = 0; i < k; ++i) v *= p;
        for (int i = k; i < kFurnaces; ++i) v *= (1.0 - p);
        dist[k] = v;
    }
    double expected = 0.0;
    double cumulative = 0.0;
    int quantile5 = -1;
    for (int k = 0; k <= kFurnaces; ++k) {
        expected += k * dist[k];
        cumulative += dist[k];
        if (quantile5 < 0 && cumulative >= 0.05) quantile5 = k;
    }
    // 零年药材下不了炉：药铺那一半 0 炉（判的是方子的门槛，不是商店）。
    int zeroYearUsable = 0;
    for (const auto& input : pill.inputs) zeroYearUsable += input.minAge <= 0 ? 1 : 0;
    std::cout << "[ch05 自产能力] 从 P0 = 0 出发：药铺零年药材可下养精丹炉的味数 " << zeroYearUsable
              << "（方子门槛 20 年）；Z1 三炉，成功率 " << chance << "%，期望 " << expected << " 瓶，5% 分位 "
              << quantile5 << " 瓶（施工图第 9 节：不设门槛）" << std::endl;
    EXPECT_EQ(zeroYearUsable, 0) << "养精丹方的门槛变了，第 9 节「买料」那一半要重算";
}

// 读取器自检：它认得出直接发药，不把 take 与注释当成 give；数量、年份、在不在 if 里都读得对。
TEST(Ch05LedgerReader, ItReadsGivesTakesCountsAgesAndTheIfAroundThem) {
    const std::string source =
        "-- give(\"pill_yangjing_dan\", 9) 注释不算\n"
        "give(\"pill_yangjing_dan\", 2)\n"
        "if take(\"material_lingshi\", 20) then\n"
        "elseif purse > 0 and take(\"material_lingshi\", purse) then\n"
        "    give(\"herb_huangjing_cao\", 6, 40)\n"
        "take(\"story_mo_qinbixin\", 1)\n";
    const std::vector<Flow> flows = flowsIn("probe.lua", source);
    ASSERT_EQ(flows.size(), 5u) << "注释里那一句不该被读进来";
    EXPECT_TRUE(flows[0].give);
    EXPECT_EQ(flows[0].item, "pill_yangjing_dan");
    EXPECT_EQ(flows[0].count, 2);
    EXPECT_FALSE(flows[1].give);
    EXPECT_EQ(flows[1].count, 20);
    EXPECT_TRUE(flows[1].inIf);
    EXPECT_EQ(flows[2].count, -1) << "扣的是变量，记成 -1";
    EXPECT_TRUE(flows[2].inIf) << "elseif 也算在 if 里";
    EXPECT_EQ(flows[3].age, 40);
    EXPECT_FALSE(flows[4].inIf) << "顶格的 take 不在 if 里，读取器要看得出来";
}

}  // namespace
