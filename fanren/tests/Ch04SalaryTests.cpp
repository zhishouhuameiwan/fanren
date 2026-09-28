// 第 4 章的药从哪来：本章各处发的药材，够不够炼本章要的药（复验判据 2，N-1 / 技术债 G-12）。
//
// ---------------------------------------------------------------------------
// 为什么要有这个文件
// ---------------------------------------------------------------------------
// 攻防战的平衡建在养精丹上，而养精丹只能用本章发的料在峰上那座炉子里炼。复验时
// 发现：本章实际发下来的料连养精丹那道熟练度门槛都垫不够，一瓶也炼不出；通关测试
// 靠 topUpHerbs() 在每一炉之前把料补齐，把这件事整个盖住了。测试注释里还写着
// 「这笔账另有一条用例看着」——那条用例从来不存在（复验 N-2）。
//
// 这个文件就是那条用例。形状刻意与通关测试不同（docs/README.md「判据是从被测物
// 推导出来的」那一段）：
//   · **不走位、不起脚本、不开炉**，直接读 scripts/ch04/*.lua 里的 give；
//   · 判据是 docs/ch04-design.md 3.3 那张账的**原文数字**，抄在下面，不从脚本推；
//   · 那张账自己的「炉数 → 瓶数」一列，拿 src/core/rules/Crafting.cpp 的公式重算一遍：
//     公式或方子一变，账就过期，这里红。
//
// 通关测试（tests/Ch04SliceTests.cpp）那一头只管「用这些料真炼出几瓶、够不够打」；
// 这一头只管「发的料与设计那张账对不对得上、那张账本身算得对不对」。
#include <gtest/gtest.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "core/rules/Crafting.h"
#include "game/Application.h"
#include "io/BattleLoader.h"
#include "io/DataLoader.h"
#include "io/ShopLoader.h"

namespace {

namespace fs = std::filesystem;

constexpr const char* kHuangjing = "herb_huangjing_cao";
constexpr const char* kZishen = "herb_zishen_cao";
constexpr const char* kRecipePill = "recipe_yangjing_dan";
constexpr const char* kRecipeEasy = "recipe_qingling_san";

// ---------------------------------------------------------------------------
// docs/ch04-design.md 3.3 的原文（改设计就改这里，改这里就是改设计）
// ---------------------------------------------------------------------------
// 发料表：每一批挂在哪、发多少、多少年。
struct Batch {
    const char* script;   // scripts/ch04/ 下的文件名
    const char* branch;   // 这一批在哪一条分支上（"" = 无条件）
    int huangjing;
    int zishen;
    int age;
};
constexpr Batch kDesignBatches[] = {
    {"kailu.lua", "照门中的价", 6, 3, 24},     // 节点 3，头一个月
    {"kailu.lua", "只收药钱", 10, 4, 22},      // 节点 3，头一个月（门里按用量多拨）
    {"yufeng.lua", "", 84, 42, 24},            // 节点 4，月例十二个月 ＋ 年例两个月
};

// 炉数表：本章要炼出几瓶，以及按设计口径要多少炉料才炼得出来。
constexpr int kPillsForTheSiege = 8;        // 攻防战最费的那一档（三张编成 × 门槛 40–70%）
constexpr int kPillsForTheMargin = 2;       // 「药数少两瓶也不翻」
constexpr int kPillsForNodeEleven = 3;      // xinbie.lua 那次 take(3)
constexpr int kPillsTheChapterNeeds = kPillsForTheSiege + kPillsForTheMargin + kPillsForNodeEleven;
constexpr int kFurnacesTheChapterNeeds = 40;   // 坏情况（5% 分位）也炼得出 13 瓶的最少炉数
// 设计口径：资质 50（GameState 缺省）、丹炉 1 品（ch04_luorifeng 那座）、每炉黄精 2 紫参 1、
// 都要 20 年以上；先垫清灵散到养精丹的火候门槛，再全部下炉。
constexpr int kAptitude = 50;
constexpr int kFurnaceGrade = 1;
constexpr int kHuangjingPerFurnace = 2;
constexpr int kZishenPerFurnace = 1;
constexpr int kMinHerbAge = 20;
// 照价那一条共 45 炉：期望、坏情况（5% 分位）、极坏（1% 分位）。
constexpr int kFurnacesOnTheCheaperBranch = 45;
constexpr double kExpectedPillsOnTheCheaperBranch = 23.1;
constexpr int kBadCasePillsOnTheCheaperBranch = 16;
constexpr int kWorstCasePillsOnTheCheaperBranch = 13;

std::string assetRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        const fs::path root(candidate);
        if (fs::exists(root / "scripts" / "ch04" / "kailu.lua") &&
            fs::exists(root / "data" / "recipes" / "alchemy" / "yangjing_dan.json")) {
            return candidate;
        }
    }
    return ".";
}

// ---------------------------------------------------------------------------
// 读脚本里的 give
// ---------------------------------------------------------------------------
// 一处药材发放：哪个脚本、第几行、缩进了没有（缩进 = 在某个 if 里，是分支发的）。
struct Give {
    std::string script;
    int line = 0;
    std::string item;
    int count = 0;
    int age = 0;
    bool indented = false;
};

// 一段 lua 源码里所有发黄精 / 紫参的 give(...)。注释行不算。
// 只认 `give("<id>", <整数>, <整数>)` 这一种写法——数目写成变量的话这里读不出来，
// 会原样报出来（count = -1），而不是悄悄当成 0。
std::vector<Give> herbGivesIn(const std::string& script, const std::string& source) {
    std::vector<Give> out;
    std::istringstream lines(source);
    std::string line;
    int number = 0;
    while (std::getline(lines, line)) {
        ++number;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const std::size_t first = line.find_first_not_of(" \t");
        if (first == std::string::npos || line.compare(first, 2, "--") == 0) continue;
        const std::size_t call = line.find("give(\"", first);
        if (call == std::string::npos) continue;
        const std::size_t idBegin = call + 6;
        const std::size_t idEnd = line.find('"', idBegin);
        if (idEnd == std::string::npos) continue;
        const std::string item = line.substr(idBegin, idEnd - idBegin);
        if (item != kHuangjing && item != kZishen) continue;
        Give give;
        give.script = script;
        give.line = number;
        give.item = item;
        give.indented = first > 0;
        int values[2] = {-1, -1};
        std::size_t pos = idEnd + 1;
        for (int& value : values) {
            pos = line.find(',', pos);
            if (pos == std::string::npos) break;
            ++pos;
            while (pos < line.size() && line[pos] == ' ') ++pos;
            std::size_t end = pos;
            while (end < line.size() && line[end] >= '0' && line[end] <= '9') ++end;
            if (end > pos) value = std::stoi(line.substr(pos, end - pos));
            pos = end;
        }
        give.count = values[0];
        give.age = values[1];
        out.push_back(give);
    }
    return out;
}

std::string readFile(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

// scripts/ch04/ 下每一个脚本里的药材发放。seen 回填扫了几个脚本。
std::vector<Give> chapterFourHerbGives(int& seen) {
    std::vector<Give> all;
    seen = 0;
    const fs::path dir = fs::path(assetRoot()) / "scripts" / "ch04";
    std::vector<fs::path> files;
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".lua") files.push_back(entry.path());
    }
    std::sort(files.begin(), files.end());
    for (const fs::path& file : files) {
        ++seen;
        for (const Give& give : herbGivesIn(file.filename().string(), readFile(file))) {
            all.push_back(give);
        }
    }
    return all;
}

std::string describe(const Give& give) {
    return give.script + ":" + std::to_string(give.line) + " give(" + give.item + ", " +
           std::to_string(give.count) + ", " + std::to_string(give.age) + ")" +
           (give.indented ? "（缩进，分支里）" : "（顶层，无条件）");
}

// ---------------------------------------------------------------------------
// 炉数 → 瓶数：照 Crafting.cpp 的公式把分布算出来
// ---------------------------------------------------------------------------
// 打法与通关测试的 brewEverything 一样：先垫清灵散到养精丹的熟练度门槛，再全部下炉
// 炼养精丹，炼到料尽。成功率与熟练度涨幅都直接调 rules::successChance /
// rules::proficiencyGain，方子取 data/recipes 里真的那两张——所以公式或方子一变，
// 这里算出来的数就变，设计那张账过期了会当场红。
struct PillOdds {
    double expected = 0.0;
    int quantile5 = 0;
    int quantile1 = 0;
};

PillOdds pillOdds(const fanren::rules::Recipe& easy, const fanren::rules::Recipe& pill,
                  int furnaces) {
    using State = std::pair<int, int>;   // (剩几炉料, 熟练度)
    std::map<State, double> warm{{{furnaces, 0}, 1.0}};
    std::map<std::tuple<int, int, int>, double> brew;   // (剩几炉, 熟练度, 已成几瓶)
    while (!warm.empty()) {
        std::map<State, double> next;
        for (const auto& [state, p] : warm) {
            const auto [left, prof] = state;
            if (prof >= pill.requiredProficiency || left == 0) {
                brew[{left, prof, 0}] += p;
                continue;
            }
            const double chance =
                fanren::rules::successChance(easy, prof, kAptitude, kFurnaceGrade) / 100.0;
            next[{left - 1, prof + fanren::rules::proficiencyGain(easy, true)}] += p * chance;
            next[{left - 1, prof + fanren::rules::proficiencyGain(easy, false)}] += p * (1 - chance);
        }
        warm = std::move(next);
    }
    std::map<int, double> pills;
    while (!brew.empty()) {
        std::map<std::tuple<int, int, int>, double> next;
        for (const auto& [state, p] : brew) {
            const auto [left, prof, made] = state;
            if (left == 0 || prof < pill.requiredProficiency) {
                pills[made] += p;
                continue;
            }
            const double chance =
                fanren::rules::successChance(pill, prof, kAptitude, kFurnaceGrade) / 100.0;
            // 熟练度过了上限只是不再涨成功率，封在 1000 以免状态无限多。
            const int onSuccess = std::min(prof + fanren::rules::proficiencyGain(pill, true), 1000);
            const int onFailure = std::min(prof + fanren::rules::proficiencyGain(pill, false), 1000);
            next[{left - 1, onSuccess, made + 1}] += p * chance;
            next[{left - 1, onFailure, made}] += p * (1 - chance);
        }
        brew = std::move(next);
    }
    PillOdds odds;
    double cumulative = 0.0;
    bool got5 = false;
    bool got1 = false;
    for (const auto& [made, p] : pills) {
        odds.expected += made * p;
        cumulative += p;
        if (!got1 && cumulative >= 0.01) {
            odds.quantile1 = made;
            got1 = true;
        }
        if (!got5 && cumulative >= 0.05) {
            odds.quantile5 = made;
            got5 = true;
        }
    }
    return odds;
}

class Ch04Salary : public ::testing::Test {
protected:
    void SetUp() override {
        auto ready = app_.init(assetRoot(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
    }
    void TearDown() override { app_.shutdown(); }

    const fanren::rules::Recipe* recipe(const char* id) {
        const auto found = app_.recipes().find(id);
        return found == app_.recipes().end() ? nullptr : &found->second;
    }

    fanren::game::Application app_;
};

// ---------------------------------------------------------------------------
// 一 · 脚本发的，就是设计那张发料表上的每一批，一批不多一批不少
// ---------------------------------------------------------------------------
TEST_F(Ch04Salary, TheScriptsHandOutExactlyTheBatchesTheDesignTableLists) {
    int seen = 0;
    const std::vector<Give> gives = chapterFourHerbGives(seen);
    // 先验分母：本章 16 个脚本；只扫到一两个说明路径找错了。
    ASSERT_GT(seen, 10) << "只扫到 " << seen << " 个第 4 章脚本";
    ASSERT_FALSE(gives.empty()) << "第 4 章一处药材也没发：读取器没读着东西，或者发料整个被删了";

    for (const Give& give : gives) {
        EXPECT_GT(give.count, 0) << "数目读不出来（写成变量了？）：" << describe(give);
        EXPECT_GT(give.age, 0) << "年份读不出来：" << describe(give);
        // 缩进的 give 只许是 kailu.lua 的两条报价——别处一缩进，这一批就成了有条件的，
        // 下面那条「最便宜的那一条分支也够」就算不清了。
        if (give.indented) {
            EXPECT_EQ(give.script, "kailu.lua")
                << "只有节点 3 那两条报价是分支发的；这一批挪进了 if，账算不清：" << describe(give);
        }
    }

    // 逐批对表。kailu.lua 两条分支各一批，按年份认（设计表里两批年份不同）。
    for (const Batch& batch : kDesignBatches) {
        int huangjing = 0;
        int zishen = 0;
        int found = 0;
        for (const Give& give : gives) {
            if (give.script != batch.script || give.age != batch.age) continue;
            if (give.indented != (batch.branch[0] != '\0')) continue;
            ++found;
            (give.item == kHuangjing ? huangjing : zishen) += give.count;
        }
        EXPECT_EQ(found, 2) << batch.script << "「" << batch.branch << "」那一批该是黄精、紫参各一条 give";
        EXPECT_EQ(huangjing, batch.huangjing)
            << batch.script << "「" << batch.branch << "」那一批黄精与设计 3.3 不符";
        EXPECT_EQ(zishen, batch.zishen)
            << batch.script << "「" << batch.branch << "」那一批紫参与设计 3.3 不符";
    }
    // 反过来：脚本里没有设计表以外的药材发放。
    EXPECT_EQ(gives.size(), std::size(kDesignBatches) * 2u)
        << "第 4 章的药材发放与设计 3.3 的发料表条数不符（多了或少了一批）";
}

// ---------------------------------------------------------------------------
// 二 · 最便宜的那一条分支，发的料也够本章要炼的炉数
// ---------------------------------------------------------------------------
TEST_F(Ch04Salary, EvenTheCheaperBranchCoversTheFurnacesTheChapterNeeds) {
    int seen = 0;
    const std::vector<Give> gives = chapterFourHerbGives(seen);
    ASSERT_FALSE(gives.empty());

    // 无条件的每一批都算；kailu.lua 两条报价取**少的那一条**（玩家可能挑任何一条）。
    int huangjingAlways = 0;
    int zishenAlways = 0;
    std::map<int, std::pair<int, int>> byAge;   // kailu.lua 两条报价各一批 (黄精, 紫参)，按年份分
    for (const Give& give : gives) {
        EXPECT_GE(give.age, kMinHerbAge)
            << "这一批过不了养精丹那道 " << kMinHerbAge << " 年门槛，发了也炼不成：" << describe(give);
        if (!give.indented) {
            (give.item == kHuangjing ? huangjingAlways : zishenAlways) += give.count;
        } else {
            auto& pair = byAge[give.age];
            (give.item == kHuangjing ? pair.first : pair.second) += give.count;
        }
    }
    ASSERT_EQ(byAge.size(), 2u) << "先验：节点 3 那两条报价各发一批";
    int furnacesOnTheCheaperBranch = 1 << 30;
    for (const auto& [age, pair] : byAge) {
        const int huangjing = huangjingAlways + pair.first;
        const int zishen = zishenAlways + pair.second;
        const int furnaces = std::min(huangjing / kHuangjingPerFurnace, zishen / kZishenPerFurnace);
        furnacesOnTheCheaperBranch = std::min(furnacesOnTheCheaperBranch, furnaces);
        EXPECT_GE(furnaces, kFurnacesTheChapterNeeds)
            << "年份 " << age << " 那一条报价全章只发了黄精 " << huangjing << "、紫参 " << zishen
            << "，够 " << furnaces << " 炉；设计 3.3 要 " << kFurnacesTheChapterNeeds << " 炉";
    }
    EXPECT_EQ(furnacesOnTheCheaperBranch, kFurnacesOnTheCheaperBranch)
        << "照价那一条的炉数与设计 3.3 写的不符";
}

// ---------------------------------------------------------------------------
// 三 · 那张账自己算得对：炉数 → 瓶数，照 Crafting.cpp 的公式
// ---------------------------------------------------------------------------
TEST_F(Ch04Salary, TheFurnaceTableStillMatchesTheCraftingFormula) {
    const fanren::rules::Recipe* pill = recipe(kRecipePill);
    const fanren::rules::Recipe* easy = recipe(kRecipeEasy);
    ASSERT_NE(pill, nullptr) << "data/recipes 里没有养精丹方";
    ASSERT_NE(easy, nullptr) << "data/recipes 里没有清灵散方";

    // 设计口径里的方子：每炉黄精 2、紫参 1、都要 20 年以上；清灵散门槛为 0（垫火候用）。
    int huangjing = 0;
    int zishen = 0;
    for (const auto& input : pill->inputs) {
        EXPECT_GE(input.minAge, kMinHerbAge) << input.itemId;
        if (input.itemId == kHuangjing) huangjing += input.count;
        if (input.itemId == kZishen) zishen += input.count;
    }
    EXPECT_EQ(huangjing, kHuangjingPerFurnace) << "养精丹方每炉用的黄精与设计 3.3 不符";
    EXPECT_EQ(zishen, kZishenPerFurnace) << "养精丹方每炉用的紫参与设计 3.3 不符";
    EXPECT_EQ(easy->requiredProficiency, 0) << "清灵散不再是零门槛，垫火候那一步算不成了";
    ASSERT_GT(pill->requiredProficiency, 0) << "先验：养精丹确实有火候门槛（垫火候那一步才有意义）";

    // 设计写的需求：13 瓶；40 炉是坏情况也炼得出 13 瓶的最少炉数。
    ASSERT_EQ(kPillsTheChapterNeeds, 13);
    const PillOdds atNeed = pillOdds(*easy, *pill, kFurnacesTheChapterNeeds);
    const PillOdds belowNeed = pillOdds(*easy, *pill, kFurnacesTheChapterNeeds - 1);
    EXPECT_GE(atNeed.quantile5, kPillsTheChapterNeeds)
        << kFurnacesTheChapterNeeds << " 炉的坏情况只有 " << atNeed.quantile5 << " 瓶";
    EXPECT_LT(belowNeed.quantile5, kPillsTheChapterNeeds)
        << "少一炉坏情况也够——设计 3.3 写的「最少 " << kFurnacesTheChapterNeeds << " 炉」不是最少";

    // 本章实发（照价那一条）：期望、坏情况、极坏，三个数都写在设计 3.3 里。
    const PillOdds supply = pillOdds(*easy, *pill, kFurnacesOnTheCheaperBranch);
    EXPECT_NEAR(supply.expected, kExpectedPillsOnTheCheaperBranch, 0.05);
    EXPECT_EQ(supply.quantile5, kBadCasePillsOnTheCheaperBranch);
    EXPECT_EQ(supply.quantile1, kWorstCasePillsOnTheCheaperBranch);
    // 「不能刚好压线」：坏情况比要的多出一截，极坏也不低于要的。
    EXPECT_GT(supply.quantile5, kPillsTheChapterNeeds);
    EXPECT_GE(supply.quantile1, kPillsTheChapterNeeds);
    std::cout << "[ch04 炉数表] 要 " << kPillsTheChapterNeeds << " 瓶；" << kFurnacesTheChapterNeeds
              << " 炉：期望 " << atNeed.expected << "、5% 分位 " << atNeed.quantile5 << "；实发 "
              << kFurnacesOnTheCheaperBranch << " 炉：期望 " << supply.expected << "、5% 分位 "
              << supply.quantile5 << "、1% 分位 " << supply.quantile1 << std::endl;
}

// ---------------------------------------------------------------------------
// 四 · 养精丹只出自峰上那座炉子（二次复验 R-4）
// ---------------------------------------------------------------------------
// 上面三条只管**药材**。复验时有人在 jinguang.lua 里加一行 give("pill_yangjing_dan", 5)，
// 攻防战前身上凭空多了五瓶，全套一条不红——设计 3.3 那张账从「炼得出几瓶」起算，
// 任何一处直接发成药、一场掉养精丹的仗、一家卖养精丹的铺子，都会让它悄悄失真。
//
// 设计 3.3 写死的来源（本条的判据）：
//   · 养精丹只出自峰上那座丹炉，用本章发的药材炼；
//   · 第 4 章的脚本**一样成药也不直接发**（give("pill_…")）——本章的成药只有炉子炼的，
//     与设计 3.2 那张表里各场仗掉的金疮药；
//   · 第 4 章的仗不掉养精丹；第 4 章玩家够得着的铺子（第 1–4 章地图上的店、第 1–4 章脚本开的店）
//     不卖养精丹。
constexpr const char* kYangjing = "pill_yangjing_dan";

// 一段 lua 里所有 give("pill_…") 的行号（注释行不算）。
std::vector<int> pillGiveLinesIn(const std::string& source) {
    std::vector<int> out;
    std::istringstream lines(source);
    std::string line;
    int number = 0;
    while (std::getline(lines, line)) {
        ++number;
        const std::size_t first = line.find_first_not_of(" \t");
        if (first == std::string::npos || line.compare(first, 2, "--") == 0) continue;
        const std::size_t call = line.find("give(");
        if (call == std::string::npos) continue;
        const std::size_t quote = line.find('"', call);
        if (quote != std::string::npos && line.compare(quote + 1, 5, "pill_") == 0) {
            out.push_back(number);
        }
    }
    return out;
}

// 一段 lua 里 shop("<id>") 打开的店。
std::vector<std::string> shopsOpenedIn(const std::string& source) {
    std::vector<std::string> out;
    std::size_t pos = 0;
    while ((pos = source.find("shop(\"", pos)) != std::string::npos) {
        // 排除 function shop(shop_id) 这种定义：前一个字符若是标识符字符就不算调用。
        const bool call = pos == 0 || !(std::isalnum(static_cast<unsigned char>(source[pos - 1])) ||
                                        source[pos - 1] == '_');
        const std::size_t begin = pos + 6;
        const std::size_t end = source.find('"', begin);
        if (end == std::string::npos) break;
        if (call) out.push_back(source.substr(begin, end - begin));
        pos = end;
    }
    return out;
}

TEST_F(Ch04Salary, TheFurnaceIsTheOnlySourceOfYangjingPills) {
    const fs::path root(assetRoot());

    // ---- 第 4 章的脚本：一样成药也不直接发 ----
    int scripts = 0;
    for (const auto& entry : fs::directory_iterator(root / "scripts" / "ch04")) {
        if (!entry.is_regular_file() || entry.path().extension() != ".lua") continue;
        ++scripts;
        for (const int line : pillGiveLinesIn(readFile(entry.path()))) {
            ADD_FAILURE() << "scripts/ch04/" << entry.path().filename().string() << ":" << line
                          << " 直接发了成药。设计 3.3：本章的成药只有峰上炉子炼的与各场仗掉的金疮药；"
                          << "要加来源，先改设计 3.3 那张账，再改这一条";
        }
    }
    ASSERT_GT(scripts, 10) << "先验：本章的脚本确实扫到了";

    // ---- 第 4 章的仗：不掉养精丹 ----
    auto battles = fanren::io::loadBattles((root / "data" / "battles").string());
    ASSERT_TRUE(battles.ok) << battles.error;
    int chapterFourBattles = 0;
    for (const auto& [id, setup] : battles.value) {
        if (setup.chapter != 4) continue;
        ++chapterFourBattles;
        for (const fanren::core::BagEntry& drop : setup.reward.drops) {
            EXPECT_NE(drop.itemId, kYangjing) << id << " 掉养精丹：设计 3.3 那张账没算这一笔";
        }
    }
    ASSERT_GE(chapterFourBattles, 5) << "先验：第 4 章那五场仗（切磋、坊门、三张攻防）都读到了";

    // ---- 第 4 章够得着的铺子：不卖养精丹 ----
    std::vector<std::string> reachable;
    for (const auto& entry : fs::directory_iterator(root / "maps")) {
        if (entry.path().extension() != ".tmj") continue;
        auto map = fanren::io::loadTileMap(entry.path().string());
        ASSERT_TRUE(map.ok) << map.error;
        if (map.value.chapter > 4) continue;
        for (const fanren::core::MapObject& object : map.value.objects) {
            if (object.type == "facility" && object.property("kind") == "shop" &&
                !object.property("ref_id").empty()) {
                reachable.push_back(object.property("ref_id"));
            }
        }
    }
    for (const char* chapter : {"ch01", "ch02", "ch03", "ch04"}) {
        const fs::path dir = root / "scripts" / chapter;
        if (!fs::is_directory(dir)) continue;
        for (const auto& entry : fs::directory_iterator(dir)) {
            if (entry.path().extension() != ".lua") continue;
            for (const std::string& id : shopsOpenedIn(readFile(entry.path()))) reachable.push_back(id);
        }
    }
    ASSERT_FALSE(reachable.empty()) << "先验：第 1–4 章至少有一家铺子（山下镇药市、外刃堂药商）";
    auto shops = fanren::io::loadShops((root / "data" / "shops").string());
    ASSERT_TRUE(shops.ok) << shops.error;
    int entries = 0;
    for (const std::string& id : reachable) {
        const auto found = shops.value.find(id);
        ASSERT_NE(found, shops.value.end()) << "铺子 " << id << " 在 data/shops 里找不到";
        for (const auto& shopEntry : found->second.entries) {
            ++entries;
            EXPECT_NE(shopEntry.itemId, kYangjing)
                << id << " 卖养精丹：第 4 章的玩家买得到，设计 3.3 那张账就不再是全部来源";
        }
    }
    ASSERT_GT(entries, 0) << "先验：那几家铺子的货确实扫到了";
}

TEST(Ch04SalaryReader, ItCatchesAPillHandedOutDirectlyButNotATakeOrAComment) {
    const std::string source =
        "-- give(\"pill_yangjing_dan\", 5) 注释里的不算\n"
        "local has = take(\"pill_yangjing_dan\", 3)\n"
        "give(\"herb_huangjing_cao\", 84, 24)\n"
        "    give(\"pill_yangjing_dan\", 5)\n"
        "give(\"pill_jinchuang_yao\", 1)\n";
    const std::vector<int> expected = {4, 5};
    EXPECT_EQ(pillGiveLinesIn(source), expected) << "该抓到第 4、5 行（take、药材、注释都不算）";
    const std::vector<std::string> opened =
        shopsOpenedIn("function shop(shop_id)\nshop(\"a_shop\")\n  shop(\"b_shop\")\n");
    const std::vector<std::string> expectedShops = {"a_shop", "b_shop"};
    EXPECT_EQ(opened, expectedShops);
}

// ---------------------------------------------------------------------------
// 判据自检：读取器有牙
// ---------------------------------------------------------------------------
// 「读不出来就算没发」「缩进了也当无条件」这两种空转，都拿一段就地写的脚本喂进去验。
TEST(Ch04SalaryReader, ItReadsTheCountAgeAndWhetherTheGiveSitsInABranch) {
    const std::string source =
        "-- give(\"herb_huangjing_cao\", 999, 99) 注释里的不算\n"
        "give(\"herb_huangjing_cao\", 84, 24)\n"
        "if pick == 2 then\n"
        "    give(\"herb_zishen_cao\", 4, 22)\n"
        "end\n"
        "give(\"material_heipai\", 1)\n"
        "give(\"herb_zishen_cao\", amount, 24)\n";
    const std::vector<Give> gives = herbGivesIn("x.lua", source);
    ASSERT_EQ(gives.size(), 3u) << "该读出三条药材发放（注释与别的物品不算）";
    EXPECT_EQ(gives[0].count, 84);
    EXPECT_EQ(gives[0].age, 24);
    EXPECT_FALSE(gives[0].indented);
    EXPECT_EQ(gives[1].item, kZishen);
    EXPECT_TRUE(gives[1].indented) << "if 里的 give 没被认成分支发的";
    EXPECT_EQ(gives[2].count, -1) << "数目写成变量时该如实读不出，不能当成 0 或别的数";
}

}  // namespace
