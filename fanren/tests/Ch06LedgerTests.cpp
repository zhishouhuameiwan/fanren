// 第 6 章的经济账（docs/ch06-design.md 第 9 节、验收 10、11）——直接读 scripts/ch06/*.lua、data/battles/b06_*.json、
// data/shops/、data/pathactions/，判据是第 9 节那几张表与五条的原文数字；外加切片跑 1a 与三处以物易物的脚本，
// 看补差、欠账（校对整改 16.4：药不够不再 return）在引擎里真的是那样。
//
// ---------------------------------------------------------------------------
// 为什么要有这个文件（第 4 章复验 N-1 / 二次复验 R-4）
// ---------------------------------------------------------------------------
// 第 4 章两次栽在药上：一次是测试替玩家把料补齐（药从哪来没人看着），一次是账只看药材、脚本直接发成药没人发现。
// 本章的通货是养精丹、不是灵石（第 9 节首句），账要对得上的五条（第 9 节末）：
//   1. 本章脚本 give("pill_yangjing_dan") 恰 1 处（1a 补足），且是 15 − item.count 的差额、差额 ≤ 0 不给；除此之外不发养精丹；
//   2. 三处以物易物（5b、8b×2）要的数量与本节字面量相符（5、2、7）；**药不够不再 return**（校对整改 16.4，HIGH-3）：
//      先扣手上有的药，差的每瓶按 7 块灵石补（与坊市收药同价），交易照成；灵石也补不齐时（复验整改 16.5 MEDIUM-N1）：
//      5b 草帽青年把差额免了、不记账，8b 卖符少女记账（ch06.qianyao 只记她一人，累加），出谷前找她能还（先药后灵石）；
//   3. 1a take("material_lingshi") 全额；节点 12 之前 give("material_lingshi") 0 处；
//   4. 三张必打编成 rewards.spirit_stones 都是 0；
//   5. 从两侧 fixture 出发，1a 之后养精丹都是 15、灵石都是 0（验收 11，两侧同账）。
// 形状与通关测试不同：不走位、不打仗，只读字；切片只起单个脚本。
//
// 施工偏差（第 18 节，以它为准）：
//   · 18.2 第 4 条：坊市挂牌 = 基价 × buyRate（0.5），火球符 13、护身符 15（施工图第 7 节写 8、6），定神符卖回 0 块；
//   · 18.4 第 3 条：1a 黄精、紫参不论年份全部用掉，土骨花不动。
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

#include "ChapterFixture.h"
#include "core/rules/Crafting.h"
#include "core/rules/Economy.h"
#include "game/Application.h"
#include "io/SaveFile.h"
#include "script/Command.h"

namespace {

namespace fs = std::filesystem;

using fanren::core::GameState;
using fanren::game::Application;

constexpr const char* kPill = "pill_yangjing_dan";
constexpr const char* kLingshi = "material_lingshi";

// 施工图第 9 节去向表：三处以物易物。
constexpr int kBarterFeixing = 5;   // 5b 飞行符
constexpr int kBarterBook = 2;      // 8b 长春功
constexpr int kBarterBrush = 7;     // 8b 金竺笔（含种子）
constexpr int kPillsAfterChudu = 15;   // 1a「补足到 15」
constexpr int kStonesPerPill = 7;   // 补差：药不够时一瓶折 7 块灵石（校对整改 16.4：与坊市收药同价）

std::string assetRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        const fs::path root(candidate);
        if (fs::exists(root / "scripts" / "ch06" / "chudu.lua") &&
            fs::exists(root / "data" / "shops" / "ch06_tainan_fangshi.json")) {
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

// 一次物品进出：哪个脚本、第几行、give 还是 take、物品、数量（字面量；写成变量或算式的记 -1）、
// 那一行是不是 if / if not 的条件、原样的数量表达式。
struct Flow {
    std::string script;
    int line = 0;
    bool give = false;
    std::string item;
    int count = 0;
    std::string countText;
    bool inIf = false;
    bool negated = false;   // if not take(...)：扣不下才进 then
};

std::vector<Flow> flowsIn(const std::string& script, const std::string& source) {
    std::vector<Flow> out;
    static const std::regex kCall("\\b(give|take)\\(\"([a-z0-9_]+)\"(?:,\\s*([^)]+))?\\)");
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
            f.countText = (*it)[3].matched ? std::string((*it)[3]) : std::string("1");
            while (!f.countText.empty() && f.countText.back() == ' ') f.countText.pop_back();
            f.count = std::regex_match(f.countText, std::regex("[0-9]+")) ? std::stoi(f.countText) : -1;
            const std::size_t first = code.find_first_not_of(" \t");
            const std::string head = first == std::string::npos ? std::string{} : code.substr(first);
            f.inIf = head.rfind("if ", 0) == 0 || head.rfind("elseif ", 0) == 0;
            f.negated = head.rfind("if not take(", 0) == 0;
            out.push_back(f);
        }
    }
    return out;
}

std::vector<Flow> chapterFlows(const std::string& root, int& scripts) {
    std::vector<Flow> all;
    scripts = 0;
    for (const auto& entry : fs::directory_iterator(fs::path(root) / "scripts" / "ch06")) {
        if (entry.path().extension() != ".lua") continue;
        ++scripts;
        const auto flows = flowsIn(entry.path().filename().string(), readFile(entry.path()));
        all.insert(all.end(), flows.begin(), flows.end());
    }
    return all;
}

// 去掉每一行 -- 之后的注释，剩下的代码原样连起来（补差那几段按代码认，不被注释里的字骗）。
std::string codeOf(const std::string& source) {
    std::istringstream in(source);
    std::string line;
    std::string out;
    while (std::getline(in, line)) {
        const std::size_t dash = line.find("--");
        out += (dash == std::string::npos ? line : line.substr(0, dash)) + "\n";
    }
    return out;
}

// 一个 then 块里（从 line 行往下到第一个同层的 end），return 在第一句 flag.set 之前。
bool returnsBeforeSettingAnything(const std::string& source, int line) {
    std::istringstream in(source);
    std::string text;
    int number = 0;
    while (std::getline(in, text)) {
        ++number;
        if (number <= line) continue;
        const std::size_t dash = text.find("--");
        const std::string code = dash == std::string::npos ? text : text.substr(0, dash);
        if (code.find("flag.set(") != std::string::npos) return false;
        if (code.find("return") != std::string::npos) return true;
        if (code.find("end") != std::string::npos && code.find_first_not_of(" \t") != std::string::npos &&
            code.substr(code.find_first_not_of(" \t"), 3) == "end") {
            return false;
        }
    }
    return false;
}

class Ch06Ledger : public ::testing::Test {
protected:
    void SetUp() override {
        root_ = assetRoot();
        auto ready = app_.init(root_, /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
        flows_ = chapterFlows(root_, scripts_);
        ASSERT_GE(scripts_, 26) << "先验：scripts/ch06/ 读得到";
        ASSERT_GE(flows_.size(), 20u) << "先验：本章的 give / take 读得到（分母不能塌）";
    }
    void TearDown() override { app_.shutdown(); }

    std::string source(const std::string& file) { return readFile(fs::path(root_) / "scripts" / "ch06" / file); }

    // 切片：起一个脚本，对白一路按确认、选择按 answer，跑到结束。返回说过的文案 key。
    std::vector<std::string> runScript(const std::string& path, int answer = 0) {
        std::vector<std::string> said;
        const auto started = app_.startEvent(path);
        EXPECT_TRUE(started.ok) << path << "：" << started.error;
        if (!started.ok) return said;
        app_.clearSpokenKeys();
        for (int frame = 0; frame < 4000 && app_.scripts().isRunning(); ++frame) {
            app_.tick(1.0 / 60.0);
            if (!app_.awaitingCommand()) continue;
            const bool talked = !app_.spokenKeys().empty();
            said.insert(said.end(), app_.spokenKeys().begin(), app_.spokenKeys().end());
            app_.clearSpokenKeys();
            fanren::script::CommandResult result;
            result.ok = true;
            result.choiceIndex = talked ? 0 : answer;
            app_.completeCommand(result);
            app_.popScene();
        }
        EXPECT_FALSE(app_.scripts().isRunning()) << path << " 没能跑到结束";
        app_.tick(1.0 / 60.0);
        said.insert(said.end(), app_.spokenKeys().begin(), app_.spokenKeys().end());
        app_.clearSpokenKeys();
        return said;
    }

    void setCount(const std::string& item, int count) {
        GameState& s = app_.state();
        while (s.removeItem(item, 1)) {
        }
        if (count > 0) s.addItem(item, count, 0);
    }

    Application app_;
    std::string root_;
    int scripts_ = 0;
    std::vector<Flow> flows_;
};

// ---------------------------------------------------------------------------
// 第 9 节第 1 条：give 养精丹恰 1 处，在 1a，是 15 − 现有 的差额；差额 ≤ 0 不给；别处（仗、路径行动、坊市）不发
// ---------------------------------------------------------------------------
TEST_F(Ch06Ledger, No1_PillsAreHandedOutOnceInOneAAsTheShortfallToFifteen) {
    std::vector<Flow> gives;
    for (const Flow& f : flows_) {
        if (f.give && f.item == kPill) gives.push_back(f);
    }
    ASSERT_EQ(gives.size(), 1u) << "施工图第 9 节第 1 条：本章脚本 give(\"pill_yangjing_dan\") 恰 1 处";
    EXPECT_EQ(gives[0].script, "chudu.lua") << "那一处在 1a（chudu.lua）";
    EXPECT_EQ(gives[0].count, -1) << "给的是差额，不是写死的数";
    EXPECT_NE(gives[0].countText.find("15"), std::string::npos) << "差额是 15 − 现有：" << gives[0].countText;
    // 编成不掉药、路径行动不给药、坊市不卖药（第 17 节第 15 条：商店只卖符纸、丹砂、现成的符）。
    for (const char* id : {"b06_yejia_xunxin", "b06_shanqiu_xisha", "b06_wufeng_qiecuo", "b06_qiecuo_wujiuzhi",
                           "b06_huangfenggu_qiecuo"}) {
        const fanren::core::BattleSetup* setup = app_.battleSetup(id);
        ASSERT_NE(setup, nullptr) << id;
        for (const auto& drop : setup->reward.drops) EXPECT_NE(drop.itemId, kPill) << id << " 掉养精丹";
    }
    int actions = 0;
    for (const fanren::core::PathAction& action : app_.data().pathActions) {
        if (action.chapter != 6) continue;
        ++actions;
        std::vector<fanren::core::BagEntry> entries = action.gives;
        entries.insert(entries.end(), action.rewardItems.begin(), action.rewardItems.end());
        if (action.kind == fanren::core::PathActionKind::Purchase) entries.push_back(action.goods);
        for (const auto& e : entries) EXPECT_NE(e.itemId, kPill) << action.id << " 发养精丹（第 9 节第 1 条：除此之外不发）";
    }
    EXPECT_GE(actions, 10) << "先验：第 6 章的路径行动读得到（data/pathactions/ch06.json）";
    const fanren::rules::Shop* shop = app_.shop("ch06_tainan_fangshi");
    ASSERT_NE(shop, nullptr);
    for (const auto& entry : shop->entries) EXPECT_NE(entry.itemId, kPill) << "坊市卖养精丹";
}

// 切片：1a 的补足，从几种起点各跑一遍（0 → 15，12 → 15，15 → 15，20 → 20 不给）。
TEST_F(Ch06Ledger, No1_OneATopsUpToFifteenAndNeverGivesWhenThereAreEnough) {
    for (const int have : {0, 12, 15, 20}) {
        app_.state() = GameState{};
        setCount(kPill, have);
        setCount(kLingshi, 40);
        runScript("ch06/chudu.lua");
        EXPECT_EQ(app_.state().itemCount(kPill), std::max(have, kPillsAfterChudu))
            << "身上 " << have << " 瓶：施工图 3.2 节点 1「补足到 15 瓶（give 差额；已有 ≥ 15 就不给）」";
        EXPECT_EQ(app_.state().itemCount(kLingshi), 0) << "第 9 节第 3 条：1a 碎银全额留给村民";
        EXPECT_EQ(app_.state().flag("ch06.chudu"), 1);
    }
}

// ---------------------------------------------------------------------------
// 第 9 节第 2 条（校对整改 16.4，HIGH-3）：三处以物易物要 5、2、7；药不够不再 return——先扣手上有的药，
// 差的每瓶按 7 块灵石补（与坊市收药同价），交易照成。灵石也补不齐时（复验整改 16.5 MEDIUM-N1）：5b 免了差额、
// 不记账；8b 记在少女名下（ch06.qianyao 累加），出谷前找她能还。
// 从前这一条判的是「每一处都 if not take(...) 然后 return、不置旗标」：那正是校对 HIGH-3 的死局——卖掉或吃掉两瓶，
// 8b 永远换不成，9a 以后整条主线锁死，本章又没有第二个养精丹的来路。
// ---------------------------------------------------------------------------
TEST_F(Ch06Ledger, No2_TheThreeBartersNeedFiveTwoSevenAndTopUpInsteadOfStopping) {
    // 三处的需要量写成字面量，交给补差：barter(5)、barter(2, …)、barter(7, …)。
    static const std::regex kBarter("\\bbarter\\(([0-9]+)[,)]");
    std::multiset<std::pair<std::string, int>> needs;
    for (const char* file : {"caomao.lua", "shaonv.lua"}) {
        const std::string code = codeOf(source(file));
        for (auto it = std::sregex_iterator(code.begin(), code.end(), kBarter); it != std::sregex_iterator(); ++it) {
            needs.insert({file, std::stoi((*it)[1])});
        }
        EXPECT_NE(code.find("local STONES_PER_PILL = 7"), std::string::npos) << file << "：补差按 7 块一瓶";
        EXPECT_NE(code.find("not take(\"pill_yangjing_dan\", pills)"), std::string::npos)
            << file << "：先扣手上有的那几瓶，扣了看返回值";
        EXPECT_NE(code.find("take(\"material_lingshi\", paid * STONES_PER_PILL)"), std::string::npos)
            << file << "：差的拿灵石补，一瓶 STONES_PER_PILL 块";
    }
    // 灵石也补不齐时（复验整改 16.5 MEDIUM-N1 ①②）：少女记账、欠的瓶数累加；草帽青年免了差额，一个字也不记。
    EXPECT_NE(codeOf(source("shaonv.lua")).find("flag.set(\"ch06.qianyao\", flag.get(\"ch06.qianyao\") + owe)"),
              std::string::npos)
        << "shaonv.lua：灵石也不够就记账，欠的瓶数累加";
    EXPECT_EQ(codeOf(source("caomao.lua")).find("ch06.qianyao"), std::string::npos)
        << "caomao.lua：草帽青年那一处补不齐是免了差额，不记账、不读欠账";
    EXPECT_NE(codeOf(source("caomao.lua")).find("\"ch06.feixingfu.waive\""), std::string::npos)
        << "caomao.lua：免差额那一句";
    const std::multiset<std::pair<std::string, int>> design = {
        {"caomao.lua", kBarterFeixing}, {"shaonv.lua", kBarterBook}, {"shaonv.lua", kBarterBrush}};
    EXPECT_EQ(needs, design) << "施工图第 9 节去向表：5b 五瓶、8b 长春功两颗、8b 金竺笔七瓶";
    // 本章扣养精丹的只有这两个脚本：两段补差，外加少女那段还账（复验整改 16.5）；扣的都是现数出来的 pills，
    // 不是字面量，也不再是「扣不下就 return」。
    std::set<std::string> pillTakers;
    for (const Flow& f : flows_) {
        if (f.give || f.item != kPill) continue;
        pillTakers.insert(f.script);
        EXPECT_EQ(f.countText, "pills") << f.script << " 第 " << f.line << " 行：扣的该是手上有的那几瓶";
        EXPECT_TRUE(f.inIf) << f.script << " 第 " << f.line << " 行的 take 没看返回值";
        EXPECT_FALSE(returnsBeforeSettingAnything(source(f.script), f.line))
            << f.script << " 第 " << f.line << " 行：扣不下还是 return 了（校对 HIGH-3 的死局）";
    }
    EXPECT_EQ(pillTakers, (std::set<std::string>{"caomao.lua", "shaonv.lua"}));
    // 补差的价与坊市收药同价（施工图 16.4「卖了再补不赚不亏」）。
    const fanren::rules::Shop* shop = app_.shop("ch06_tainan_fangshi");
    const fanren::core::Item* pill = app_.data().findItem(kPill);
    ASSERT_NE(shop, nullptr);
    ASSERT_NE(pill, nullptr);
    EXPECT_EQ(fanren::rules::sellPrice(*shop, *pill, 0), kStonesPerPill) << "补差 7 块一瓶 = 坊市收一瓶养精丹的价";
    EXPECT_EQ(kPillsAfterChudu - kBarterFeixing - kBarterBook - kBarterBrush, 1)
        << "第 9 节 8.1：以物易物之后养精丹约 1 瓶（15 − 14）";
}

// 补差三处 × 三种局面（校对整改 16.4 新增）。一个干净的局面起一个脚本：5b 要看过行情（ch06.lingshi），
// 8b 要双首鹜飞过（ch06.shuangshou）；8b 一次跑完书与笔两段。
struct BarterRun {
    int pills = 0, stones = 0, owed = 0;
    std::vector<std::string> said;
};

// 药够：扣的正是那几瓶，灵石一块不动，不欠，补差的三句一句不说。
TEST_F(Ch06Ledger, No2_WithEnoughPillsEachBarterTakesItsPillsAndNotOneStone) {
    const auto run = [&](const char* flagName, const char* script, int pills, int stones) {
        app_.state() = GameState{};
        app_.state().setFlag(flagName);
        setCount(kPill, pills);
        setCount(kLingshi, stones);
        BarterRun r;
        r.said = runScript(script);
        r.pills = app_.state().itemCount(kPill);
        r.stones = app_.state().itemCount(kLingshi);
        r.owed = app_.state().flag("ch06.qianyao");
        return r;
    };
    const BarterRun b5 = run("ch06.lingshi", "ch06/caomao.lua", kBarterFeixing, 20);
    EXPECT_EQ(b5.pills, 0);
    EXPECT_EQ(b5.stones, 20) << "药够：灵石一块不动";
    EXPECT_EQ(b5.owed, 0);
    EXPECT_EQ(app_.state().flag("ch06.feixingfu"), 1);
    EXPECT_EQ(app_.state().itemCount("talisman_feixing_fu"), 1);
    for (const char* key : {"ch06.feixingfu.short", "ch06.feixingfu.topup", "ch06.feixingfu.waive",
                            "ch06.feixingfu.waive_none", "ch06.feixingfu.back_none"}) {
        EXPECT_EQ(std::count(b5.said.begin(), b5.said.end(), key), 0) << "药够不该说 " << key;
    }
    const BarterRun b8 = run("ch06.shuangshou", "ch06/shaonv.lua", kBarterBook + kBarterBrush, 20);
    EXPECT_EQ(b8.pills, 0);
    EXPECT_EQ(b8.stones, 20) << "药够：灵石一块不动";
    EXPECT_EQ(b8.owed, 0);
    EXPECT_EQ(app_.state().flag("ch06.changchungong"), 1);
    EXPECT_EQ(app_.state().flag("ch06.jinzhubi"), 1);
    EXPECT_EQ(app_.state().itemCount("story_jinzhu_bi"), 1);
    for (const char* key : {"ch06.jinzhubi.short_book", "ch06.jinzhubi.topup_book", "ch06.jinzhubi.owe_book",
                            "ch06.jinzhubi.short_brush", "ch06.jinzhubi.topup_brush", "ch06.jinzhubi.owe_brush",
                            "ch06.jinzhubi.sniff_none"}) {
        EXPECT_EQ(std::count(b8.said.begin(), b8.said.end(), key), 0) << "药够不该说 " << key;
    }
}

// 药不够、灵石够：手上的药全扣，差的每瓶 7 块；不欠；交易照成、旗标照置。
TEST_F(Ch06Ledger, No2_ShortOfPillsTheStonesMakeUpTheDifferenceAtSevenABottle) {
    // 5b：三瓶、二十块 → 扣三瓶，补两瓶 14 块，剩 6。
    app_.state() = GameState{};
    app_.state().setFlag("ch06.lingshi");
    setCount(kPill, 3);
    setCount(kLingshi, 20);
    const std::vector<std::string> said5b = runScript("ch06/caomao.lua");
    EXPECT_EQ(app_.state().itemCount(kPill), 0);
    EXPECT_EQ(app_.state().itemCount(kLingshi), 20 - 2 * kStonesPerPill) << "差两瓶：补 14 块";
    EXPECT_EQ(app_.state().flag("ch06.qianyao"), 0) << "灵石够：不欠";
    EXPECT_EQ(app_.state().flag("ch06.feixingfu"), 1) << "交易照成";
    EXPECT_EQ(app_.state().itemCount("talisman_feixing_fu"), 1);
    EXPECT_EQ(app_.state().itemCount("material_fuzhi"), 12);
    EXPECT_EQ(std::count(said5b.begin(), said5b.end(), "ch06.feixingfu.topup"), 1);
    EXPECT_EQ(std::count(said5b.begin(), said5b.end(), "ch06.feixingfu.waive"), 0) << "补齐了：不免";

    // 8b：一瓶、一百块 → 书：扣一瓶、补一瓶 7 块；笔：一瓶也没了，补七瓶 49 块 → 剩 44。
    app_.state() = GameState{};
    app_.state().setFlag("ch06.shuangshou");
    setCount(kPill, 1);
    setCount(kLingshi, 100);
    const std::vector<std::string> said8b = runScript("ch06/shaonv.lua");
    EXPECT_EQ(app_.state().itemCount(kPill), 0);
    EXPECT_EQ(app_.state().itemCount(kLingshi), 100 - (1 + kBarterBrush) * kStonesPerPill) << "书差一颗、笔差七瓶";
    EXPECT_EQ(app_.state().flag("ch06.qianyao"), 0) << "灵石够：不欠";
    EXPECT_EQ(app_.state().flag("ch06.changchungong"), 1);
    EXPECT_EQ(app_.state().flag("ch06.jinzhubi"), 1) << "交易照成";
    EXPECT_EQ(app_.state().itemCount("story_jinzhu_bi"), 1);
    EXPECT_EQ(std::count(said8b.begin(), said8b.end(), "ch06.jinzhubi.topup_book"), 1);
    EXPECT_EQ(std::count(said8b.begin(), said8b.end(), "ch06.jinzhubi.topup_brush"), 1);
    EXPECT_EQ(std::count(said8b.begin(), said8b.end(), "ch06.jinzhubi.owe_book"), 0);
    EXPECT_EQ(std::count(said8b.begin(), said8b.end(), "ch06.jinzhubi.owe_brush"), 0);
}

// 药不够、灵石也不够：能补几瓶补几瓶，交易照成——主线从此不会卡在这里。补不齐的那一截（复验整改 16.5 MEDIUM-N1）：
// 5b 草帽青年免了、不记账（他换完就收摊，记下来没人收）；8b 记在卖符少女名下（ch06.qianyao 累加，只记她一人）。
TEST_F(Ch06Ledger, No2_ShortOfPillsAndStonesTheCapWaivesAndTheGirlKeepsTheAccount) {
    // 5b：两瓶、十块 → 扣两瓶，补一瓶 7 块（剩 3），差的两瓶免了：不记账。
    app_.state() = GameState{};
    app_.state().setFlag("ch06.lingshi");
    setCount(kPill, 2);
    setCount(kLingshi, 10);
    const std::vector<std::string> said5b = runScript("ch06/caomao.lua");
    EXPECT_EQ(app_.state().itemCount(kPill), 0);
    EXPECT_EQ(app_.state().itemCount(kLingshi), 10 - kStonesPerPill);
    EXPECT_EQ(app_.state().flag("ch06.qianyao"), 0) << "复验整改 16.5：草帽青年那一处不记账";
    EXPECT_EQ(app_.state().flag("ch06.feixingfu"), 1) << "免了差额，交易照成";
    EXPECT_EQ(app_.state().itemCount("talisman_feixing_fu"), 1);
    EXPECT_EQ(std::count(said5b.begin(), said5b.end(), "ch06.feixingfu.topup"), 1);
    EXPECT_EQ(std::count(said5b.begin(), said5b.end(), "ch06.feixingfu.waive"), 1) << "免差额那一句";

    // 8b 接着同一个局面跑：一瓶也没有、灵石只剩 3 块 → 书欠两颗、笔欠七瓶，书与笔照样到手，账记在她名下。
    app_.state().setFlag("ch06.shuangshou");
    const std::vector<std::string> said8b = runScript("ch06/shaonv.lua");
    EXPECT_EQ(app_.state().itemCount(kLingshi), 10 - kStonesPerPill) << "三块不够补一瓶：一块不扣";
    EXPECT_EQ(app_.state().flag("ch06.qianyao"), kBarterBook + kBarterBrush) << "只记她那两笔：2 ＋ 7";
    EXPECT_EQ(app_.state().flag("ch06.changchungong"), 1);
    EXPECT_EQ(app_.state().flag("ch06.jinzhubi"), 1) << "欠着，交易照成";
    EXPECT_EQ(app_.state().itemCount("story_jinzhu_bi"), 1);
    EXPECT_EQ(std::count(said8b.begin(), said8b.end(), "ch06.jinzhubi.owe_book"), 1);
    EXPECT_EQ(std::count(said8b.begin(), said8b.end(), "ch06.jinzhubi.owe_brush"), 1);
    EXPECT_EQ(std::count(said8b.begin(), said8b.end(), "ch06.jinzhubi.topup_book"), 0) << "一块灵石也补不上，不说补差";

    // 手上还有药的那一支：开口说「他有药」、免差额说「有几瓶算几瓶」（抽查 N3-L1：零瓶的另有一句）。
    EXPECT_EQ(std::count(said5b.begin(), said5b.end(), "ch06.feixingfu.back"), 1);
    EXPECT_EQ(std::count(said5b.begin(), said5b.end(), "ch06.feixingfu.back_none"), 0);
    EXPECT_EQ(std::count(said5b.begin(), said5b.end(), "ch06.feixingfu.waive_none"), 0);

    // 一瓶药、一块灵石都没有（校对给复验的判据 4 (d)）：5b 全免，8b 全记，照样走得到 9a 的门。
    // 零瓶时「他有药」「有几瓶算几瓶」「要一颗验药」都不成立，各换一句（复验整改 16.5 第四轮，抽查 N3-L1）。
    app_.state() = GameState{};
    app_.state().setFlag("ch06.lingshi");
    const std::vector<std::string> none5b = runScript("ch06/caomao.lua");
    EXPECT_EQ(app_.state().flag("ch06.feixingfu"), 1);
    EXPECT_EQ(app_.state().flag("ch06.qianyao"), 0);
    for (const char* key : {"ch06.feixingfu.back_none", "ch06.feixingfu.waive_none"}) {
        EXPECT_EQ(std::count(none5b.begin(), none5b.end(), key), 1) << "零瓶该说 " << key;
    }
    for (const char* key : {"ch06.feixingfu.back", "ch06.feixingfu.waive"}) {
        EXPECT_EQ(std::count(none5b.begin(), none5b.end(), key), 0) << "零瓶不该说 " << key;
    }
    app_.state().setFlag("ch06.shuangshou");
    const std::vector<std::string> none8b = runScript("ch06/shaonv.lua");
    EXPECT_EQ(app_.state().flag("ch06.jinzhubi"), 1) << "一瓶药都没有也换得成：9a 的门开着";
    EXPECT_EQ(app_.state().flag("ch06.qianyao"), kBarterBook + kBarterBrush);
    EXPECT_EQ(std::count(none8b.begin(), none8b.end(), "ch06.jinzhubi.sniff_none"), 1) << "零瓶：药见不着，说定拿灵石折";
    EXPECT_EQ(std::count(none8b.begin(), none8b.end(), "ch06.jinzhubi.sniff"), 0) << "零瓶：没有药可验";
}

// 还账（复验整改 16.5 MEDIUM-N1 ②）：8b 换完、出谷之前再找她——欠着就问还不还；还就先扣药、再按 7 块一瓶扣灵石，
// 还多少减多少；不还、或身上什么也凑不出，账一个不动；还清之后她才说那句闲话。
TEST_F(Ch06Ledger, No2_TheGirlsAccountIsPaidBackPillsFirstThenStonesBeforeLeaving) {
    const auto owing = [&](int owed, int pills, int stones) {
        app_.state() = GameState{};
        app_.state().setFlag("ch06.shuangshou");
        app_.state().setFlag("ch06.changchungong");
        app_.state().setFlag("ch06.jinzhubi");
        app_.state().setFlag("ch06.qianyao", owed);
        setCount(kPill, pills);
        setCount(kLingshi, stones);
    };
    const auto said = [](const std::vector<std::string>& keys, const char* key) {
        return std::count(keys.begin(), keys.end(), key);
    };

    // 不还（第二项）：一样不扣，账不动。
    owing(9, 3, 30);
    std::vector<std::string> keys = runScript("ch06/shaonv.lua", /*answer=*/1);
    EXPECT_EQ(said(keys, "ch06.npc.shaonv.after_owe"), 1) << "欠着账：她那句闲话换成欠账的一句";
    EXPECT_EQ(said(keys, "ch06.npc.shaonv.after"), 0) << "欠着账：不说不欠时那一句";
    EXPECT_EQ(said(keys, "ch06.jinzhubi.repay_later"), 1);
    EXPECT_EQ(app_.state().flag("ch06.qianyao"), 9);
    EXPECT_EQ(app_.state().itemCount(kPill), 3);
    EXPECT_EQ(app_.state().itemCount(kLingshi), 30);

    // 还（第一项）：三瓶药先抵三瓶，灵石 30 块再抵四瓶（28 块），还剩两瓶没还上。
    keys = runScript("ch06/shaonv.lua", /*answer=*/0);
    EXPECT_EQ(app_.state().itemCount(kPill), 0) << "有药先还药";
    EXPECT_EQ(app_.state().itemCount(kLingshi), 30 - 4 * kStonesPerPill) << "没药按 7 块一瓶还";
    EXPECT_EQ(app_.state().flag("ch06.qianyao"), 2) << "还多少减多少：9 − 3 − 4";
    EXPECT_EQ(said(keys, "ch06.jinzhubi.repay_pill"), 1);
    EXPECT_EQ(said(keys, "ch06.jinzhubi.repay_stone"), 1);
    EXPECT_EQ(said(keys, "ch06.jinzhubi.repay_part"), 1);
    EXPECT_EQ(said(keys, "ch06.jinzhubi.repay_clear"), 0);

    // 身上凑不出一瓶的钱（2 块）：说一句，账不动。
    keys = runScript("ch06/shaonv.lua", /*answer=*/0);
    EXPECT_EQ(said(keys, "ch06.jinzhubi.repay_none"), 1);
    EXPECT_EQ(app_.state().flag("ch06.qianyao"), 2);
    EXPECT_EQ(app_.state().itemCount(kLingshi), 2);

    // 再凑 12 块（共 14）：还清两瓶，账清了；再找她，只说不欠时那一句，一样不扣。
    setCount(kLingshi, 14);
    keys = runScript("ch06/shaonv.lua", /*answer=*/0);
    EXPECT_EQ(app_.state().flag("ch06.qianyao"), 0);
    EXPECT_EQ(app_.state().itemCount(kLingshi), 0);
    EXPECT_EQ(said(keys, "ch06.jinzhubi.repay_clear"), 1);
    setCount(kPill, 5);
    keys = runScript("ch06/shaonv.lua", /*answer=*/0);
    EXPECT_EQ(said(keys, "ch06.npc.shaonv.after"), 1) << "还清了：那句闲话照常";
    EXPECT_EQ(said(keys, "ch06.npc.shaonv.after_owe"), 0);
    EXPECT_EQ(app_.state().itemCount(kPill), 5) << "不欠了：一瓶也不扣";
    EXPECT_EQ(app_.state().flag("ch06.jinzhubi"), 1) << "还账不重演 8b 的交易";
    EXPECT_EQ(app_.state().itemCount("story_jinzhu_bi"), 0) << "还账不重演 8b 的交易（笔不会再给一支）";
}

// 欠着她账的一支（复验整改 16.5 MEDIUM-N1 ③）：打探 shaonv_dating（「上回的丹药换得太便宜」、白送一瓶金疮药）
// 不挂出来；还清之后照常。走的是规则层同一个入口（Application::pathActionsFor，世界层 E 键问的就是它）。
TEST_F(Ch06Ledger, No2_WhileHeOwesHerTheInquiryIsNotOffered) {
    const auto offered = [&]() {
        for (const fanren::core::PathAction* action : app_.pathActionsFor("npc_maifu_shaonv")) {
            if (action->id == "shaonv_dating") return true;
        }
        return false;
    };
    app_.state() = GameState{};
    app_.state().mapId = "ch06_tainan_gu";
    app_.state().setFlag("ch06.jinzhubi");
    app_.state().setFlag("ch06.qianyao", 2);
    EXPECT_FALSE(offered()) << "欠着她两瓶：不该挂出那句「换得太便宜」和白送的药";
    app_.state().setFlag("ch06.qianyao", 0);
    EXPECT_TRUE(offered()) << "还清了：打探照常挂出";
    app_.state().setFlag("ch06.chugu");
    EXPECT_FALSE(offered()) << "先验：出了谷她就不在这张图上了，打探跟着撤";
}

// 校对给复验的判据 3：从 1a 之后的 15 瓶出发，先卖两瓶（坊市 7 块一瓶）、再在 ① 里吃掉一瓶，照样走得到 9a
//（9a 的 guard 是 ch06.jinzhubi）。
TEST_F(Ch06Ledger, No2_SellingTwoAndEatingOneStillReachesNineA) {
    app_.state() = GameState{};
    setCount(kPill, kPillsAfterChudu - 2 - 1);
    setCount(kLingshi, 2 * kStonesPerPill);
    app_.state().setFlag("ch06.lingshi");
    runScript("ch06/caomao.lua");
    EXPECT_EQ(app_.state().flag("ch06.feixingfu"), 1);
    app_.state().setFlag("ch06.shuangshou");
    runScript("ch06/shaonv.lua");
    EXPECT_EQ(app_.state().flag("ch06.jinzhubi"), 1) << "卖两瓶、吃一瓶之后 8b 照样换得成：9a 的门开着";
    EXPECT_EQ(app_.state().itemCount(kPill), 0);
    EXPECT_EQ(app_.state().itemCount(kLingshi), 0) << "差的两瓶正好拿卖药的 14 块补上";
    EXPECT_EQ(app_.state().flag("ch06.qianyao"), 0);
}

// ---------------------------------------------------------------------------
// 第 9 节第 3 条：1a 碎银全额；节点 12 之前 give 灵石 0 处（路径行动也不发）
// ---------------------------------------------------------------------------
TEST_F(Ch06Ledger, No3_SilverIsLeftWhollyInOneAAndNoStoneIsGivenBeforeTwelve) {
    std::vector<Flow> silverTakes;
    std::set<std::string> givers;
    std::set<std::string> topUps;
    for (const Flow& f : flows_) {
        if (f.item != kLingshi) continue;
        if (f.give) {
            givers.insert(f.script);
        } else if (f.script == "chudu.lua") {
            silverTakes.push_back(f);
        } else {
            // 以物易物药不够时拿灵石补差（校对整改 16.4，HIGH-3）：扣的是差几瓶的钱，不是全额。
            topUps.insert(f.script);
            EXPECT_EQ(f.countText, "paid * STONES_PER_PILL") << f.script << " 第 " << f.line << " 行扣灵石，却不是补差";
            EXPECT_TRUE(f.inIf) << f.script << " 第 " << f.line << " 行扣灵石没看返回值";
        }
    }
    EXPECT_EQ(topUps, (std::set<std::string>{"caomao.lua", "shaonv.lua"})) << "补差只在 5b、8b 两段以物易物里";
    ASSERT_EQ(silverTakes.size(), 1u) << "本章脚本扣灵石（碎银）全额的只有 1a 那一处";
    EXPECT_EQ(silverTakes[0].script, "chudu.lua");
    EXPECT_EQ(silverTakes[0].count, -1) << "全额：扣的是 item.count 数出来的那个数，不是字面量";
    EXPECT_TRUE(silverTakes[0].inIf) << "1a 的 take 看返回值";
    // 全额：扣的那个变量，就是同一个脚本里 item.count("material_lingshi") 数出来的那一个，原样、不打折。
    const std::string variable = silverTakes[0].countText;
    EXPECT_TRUE(std::regex_match(variable, std::regex("[A-Za-z_][A-Za-z0-9_]*")))
        << "1a 扣的不是原样那个数：" << variable;
    EXPECT_NE(source("chudu.lua").find("local " + variable + " = item.count(\"material_lingshi\")"), std::string::npos)
        << "1a 扣的 " << variable << " 不是 item.count(\"material_lingshi\") 数出来的全额";
    // 节点 12 之前：3.1 表里 12 之前那些挂点的脚本一处也不给灵石。给灵石的只有 12（搜身）与 18b（送物）。
    EXPECT_EQ(givers, (std::set<std::string>{"xisha.lua", "maiping.lua"}))
        << "施工图第 9 节第 3 条：节点 12 之前 give(\"material_lingshi\") 0 处；之后只有搜身与送物";
    for (const fanren::core::PathAction& action : app_.data().pathActions) {
        if (action.chapter != 6) continue;
        for (const auto& e : action.gives) EXPECT_NE(e.itemId, kLingshi) << action.id << " 发灵石";
        for (const auto& e : action.rewardItems) EXPECT_NE(e.itemId, kLingshi) << action.id << " 发灵石";
    }
}

// ---------------------------------------------------------------------------
// 第 9 节第 4 条：三张必打编成 rewards.spirit_stones 都是 0
// ---------------------------------------------------------------------------
TEST_F(Ch06Ledger, No4_TheThreeMustFightsPayNoStone) {
    int cultivation = 0;
    for (const char* id : {"b06_yejia_xunxin", "b06_shanqiu_xisha", "b06_wufeng_qiecuo"}) {
        const fanren::core::BattleSetup* setup = app_.battleSetup(id);
        ASSERT_NE(setup, nullptr) << id;
        EXPECT_EQ(setup->reward.spiritStones, 0) << id << "：第 9 节第 4 条（② 的五十块是脚本搜身给的）";
        cultivation += setup->reward.cultivation;
    }
    EXPECT_EQ(cultivation, 60 + 150 + 60) << "第 9 节来源表：三场必打修为 60 / 150 / 60";
    // 可选切磋 ×2 修为 30 / 30：由路径行动条目发（编成全 0，规则 26）。
    std::map<std::string, int> spar;
    for (const fanren::core::PathAction& action : app_.data().pathActions) {
        if (action.chapter == 6 && action.kind == fanren::core::PathActionKind::Challenge) spar[action.battleId] = action.rewardCultivation;
    }
    EXPECT_EQ(spar, (std::map<std::string, int>{{"b06_huangfenggu_qiecuo", 30}, {"b06_qiecuo_wujiuzhi", 30}}))
        << "第 9 节来源表：可选切磋 ×2 修为 30 / 30";
}

// ---------------------------------------------------------------------------
// 第 9 节第 5 条 / 验收 11：从两侧 fixture 出发，1a 之后养精丹都是 15、灵石都是 0
// ---------------------------------------------------------------------------
// 起点原样读第 5 章两份交接存档（不改一个字段），只起 1a 那一个脚本。走位走到 1a 的那一遍在
// Ch06AcceptanceTests 两条通关里（1a 之后当场判）。
TEST_F(Ch06Ledger, No5_BothHandOversLeaveOneAWithFifteenPillsAndNoStone) {
    std::vector<std::tuple<int, int, int>> after;
    for (const char* file : {fanren::test::kChapterFiveEndingFirst, fanren::test::kChapterFiveEndingSecond}) {
        auto loaded = fanren::io::loadGame(fanren::test::chapterFixturePath(root_, file).string());
        ASSERT_TRUE(loaded.ok) << file << "：" << loaded.error;
        app_.state() = loaded.value;
        const int before = app_.state().itemCount(kPill);
        runScript("ch06/chudu.lua");
        after.emplace_back(before, app_.state().itemCount(kPill), app_.state().itemCount(kLingshi));
        EXPECT_EQ(app_.state().itemCount(kPill), kPillsAfterChudu) << file << "：1a 之后养精丹 15";
        EXPECT_EQ(app_.state().itemCount(kLingshi), 0) << file << "：1a 之后灵石 0";
        EXPECT_EQ(app_.state().itemCount("herb_huangjing_cao"), 0) << file << "：施工偏差 18.4 第 3 条";
        EXPECT_EQ(app_.state().itemCount("herb_zishen_cao"), 0) << file << "：施工偏差 18.4 第 3 条";
        EXPECT_EQ(app_.state().bottle.drops, 0) << file << "：bottle.spend(3)";
    }
    ASSERT_EQ(after.size(), 2u);
    EXPECT_NE(std::get<0>(after[0]), std::get<0>(after[1])) << "先验：两侧交来的养精丹不同（12 / 0），同账才有意义";
}

// ---------------------------------------------------------------------------
// 坊市（施工图第 7 节、16.1 第 5 条拍板、施工偏差 18.2 第 4 条）
// ---------------------------------------------------------------------------
TEST_F(Ch06Ledger, TheMarketSellsPaperCinnabarAndTalismansAndBuysPillsAtOneTwelfth) {
    const fanren::rules::Shop* shop = app_.shop("ch06_tainan_fangshi");
    ASSERT_NE(shop, nullptr) << "施工图第 7 节：新商店 ch06_tainan_fangshi";
    EXPECT_DOUBLE_EQ(shop->sellRate, 0.08) << "施工图 16.1 第 5 条：sellRate 0.08";
    EXPECT_DOUBLE_EQ(shop->buyRate, 0.5) << "施工偏差 18.2 第 4 条：buyRate 0.5（符纸挂 1 块一张）";
    std::map<std::string, int> listed;
    for (const auto& entry : shop->entries) {
        const fanren::core::Item* item = app_.data().findItem(entry.itemId);
        ASSERT_NE(item, nullptr) << entry.itemId;
        listed[entry.itemId] = fanren::rules::buyPrice(*shop, *item, 0);
    }
    const std::map<std::string, int> design = {{"material_fuzhi", 1},        {"material_dansha", 2},
                                               {"talisman_dingshen_fu", 2},  {"talisman_huoqiu_fu", 13},
                                               {"talisman_hushen_fu", 15}};
    EXPECT_EQ(listed, design) << "施工图第 7 节 / 施工偏差 18.2 第 4 条：符纸 1、丹砂 2、定神符 2、火球符 13、护身符 15";
    // 收：养精丹基价 90 卖出得 7 块（施工图第 7 节原文「正好是原著『五瓶换一张三十块的符』的比价」）；定神符卖回 0。
    const fanren::core::Item* pill = app_.data().findItem(kPill);
    ASSERT_NE(pill, nullptr);
    EXPECT_EQ(fanren::rules::sellPrice(*shop, *pill, 0), 7) << "施工图第 7 节：养精丹卖 7 块灵石";
    const fanren::core::Item* dingshen = app_.data().findItem("talisman_dingshen_fu");
    ASSERT_NE(dingshen, nullptr);
    EXPECT_EQ(fanren::rules::sellPrice(*shop, *dingshen, 0), 0) << "施工偏差 18.2 第 4 条：定神符卖回 0 块";
    // 飞行符 tradeable false（E9）：卖不掉，也就不会被人换成钱再去坊市。
    const fanren::core::Item* feixing = app_.data().findItem("talisman_feixing_fu");
    ASSERT_NE(feixing, nullptr);
    EXPECT_FALSE(feixing->tradeable) << "施工图 E9：飞行符 tradeable false";
}

// 制符（施工图第 7 节）：定神符方 difficulty 40、requiredProficiency 0、符纸 1 ＋ 丹砂 1、没有妖丹引；
// 制符桌 grade 2、熟练度 0、资质 50 时成功率「两成上下」。
TEST_F(Ch06Ledger, TheCalmingTalismanRecipeIsPaperAndCinnabarAtAboutOneInFive) {
    const auto found = app_.recipes().find("recipe_dingshen_fu");
    ASSERT_NE(found, app_.recipes().end()) << "施工图第 7 节：data/recipes/talisman/dingshen_fu.json";
    const fanren::rules::Recipe& recipe = found->second;
    EXPECT_EQ(recipe.kind, fanren::rules::CraftKind::Talisman);
    EXPECT_EQ(recipe.productId, "talisman_dingshen_fu");
    EXPECT_EQ(recipe.difficulty, 40);
    EXPECT_EQ(recipe.requiredProficiency, 0);
    std::map<std::string, int> inputs;
    for (const auto& in : recipe.inputs) inputs[in.itemId] += in.count;
    EXPECT_EQ(inputs, (std::map<std::string, int>{{"material_dansha", 1}, {"material_fuzhi", 1}}))
        << "施工图第 7 节：输入符纸 1 ＋ 丹砂 1，没有妖丹引（「妖丹」首见 ch344）";
    const int chance = fanren::rules::successChance(recipe, /*proficiency=*/0, /*aptitude=*/50, /*toolGrade=*/2);
    std::cout << "[ch06 制符] 定神符 熟练度 0、资质 50、grade 2 成功率 " << chance << "%（施工图第 7 节：两成上下）" << std::endl;
    EXPECT_GE(chance, 10) << "施工图第 7 节：目标两成上下";
    EXPECT_LE(chance, 30) << "施工图第 7 节：目标两成上下";
}

// ---------------------------------------------------------------------------
// 18b 叶师叔送物（施工图 3.2 节点 18b 原文，第 9 节来源表「18b 送物 ＋30～60」）：
//   huinuo == 1 → 灵石 60、火球符 2、护身符 1、金刺符 1；huinuo == 2 → 灵石 30、火球符 1；
//   rangdan == 2 → 在前两者基础上各减灵石 10。四种组合各切片跑一遍 maiping.lua。
// ---------------------------------------------------------------------------
TEST_F(Ch06Ledger, TheDeliveryAtEighteenBIsTheFormulaOfNodeEighteen) {
    struct Want {
        int huinuo, rangdan;
        int lingshi, huoqiu, hushen, jinci;
    };
    const std::vector<Want> kDesign = {
        {1, 1, 60, 2, 1, 1}, {1, 2, 50, 2, 1, 1}, {2, 1, 30, 1, 0, 0}, {2, 2, 20, 1, 0, 0}};
    int least = 1 << 30;
    for (const Want& w : kDesign) {
        app_.state() = GameState{};
        app_.state().setFlag("ch06.renyao");
        app_.state().setFlag("ch06.huinuo", w.huinuo);
        app_.state().setFlag("ch06.rangdan", w.rangdan);
        runScript("ch06/maiping.lua");
        const GameState& s = app_.state();
        EXPECT_EQ(s.itemCount(kLingshi), w.lingshi) << "huinuo " << w.huinuo << " / rangdan " << w.rangdan;
        EXPECT_EQ(s.itemCount("talisman_huoqiu_fu"), w.huoqiu) << "huinuo " << w.huinuo << " / rangdan " << w.rangdan;
        EXPECT_EQ(s.itemCount("talisman_hushen_fu"), w.hushen) << "huinuo " << w.huinuo << " / rangdan " << w.rangdan;
        EXPECT_EQ(s.itemCount("talisman_jinci_fu"), w.jinci) << "huinuo " << w.huinuo << " / rangdan " << w.rangdan;
        least = std::min(least, w.lingshi);
    }
    // 搜身 50 ＋ 送物最少 20 = 70：两处都选第二项、出谷前一块没攒的人，章末只有 70 块。原验收第 2 条写「灵石 ≥ 80」，
    // 与这张表对不上；校对整改 16.4（LOW-11 (b)）把验收第 2 条改成了公式「50 ＋ 送物 ＋ 卖出 − 买入 − 补差」，
    // 通关测试按公式判（Ch06AcceptanceTests expectTheChapterEndState）。
    EXPECT_EQ(50 + least, 70) << "送物最少 20（huinuo 2、rangdan 2）";
    std::cout << "[ch06 章末灵石下限] 搜身 50 ＋ 送物最少 " << least << " = " << (50 + least)
              << "（出谷前没攒钱、14 与 17b 都选第二项的那一种）" << std::endl;
}

// 读取器自检：它认得出直接发药、if not take、变量数量与注释。
TEST(Ch06LedgerReader, ItReadsGivesTakesCountsAndTheNotAroundThem) {
    const std::string source =
        "-- give(\"pill_yangjing_dan\", 9) 注释不算\n"
        "give(\"pill_yangjing_dan\", 15 - have)\n"
        "if not take(\"pill_yangjing_dan\", 5) then\n"
        "    talk(\"\", \"x\")\n"
        "    return\n"
        "end\n"
        "if silver > 0 and take(\"material_lingshi\", silver) then\n"
        "take(\"material_fuzhi\", 12)\n";
    const std::vector<Flow> flows = flowsIn("probe.lua", source);
    ASSERT_EQ(flows.size(), 4u) << "注释里那一句不该被读进来";
    EXPECT_TRUE(flows[0].give);
    EXPECT_EQ(flows[0].count, -1) << "算式记成 -1";
    EXPECT_NE(flows[0].countText.find("15"), std::string::npos);
    EXPECT_TRUE(flows[1].negated);
    EXPECT_EQ(flows[1].count, 5);
    EXPECT_TRUE(returnsBeforeSettingAnything(source, flows[1].line));
    EXPECT_TRUE(flows[2].inIf);
    EXPECT_FALSE(flows[2].negated);
    EXPECT_EQ(flows[2].count, -1);
    EXPECT_FALSE(flows[3].inIf) << "顶格的 take 不在 if 里，读取器要看得出来";
    // 反面：then 块里先置旗标再 return，要读得出来。
    const std::string bad = "if not take(\"pill_yangjing_dan\", 5) then\n    flag.set(\"x\")\n    return\nend\n";
    EXPECT_FALSE(returnsBeforeSettingAnything(bad, 1));
}

}  // namespace
