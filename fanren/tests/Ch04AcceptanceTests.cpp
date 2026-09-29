// 第 4 章验收第 4 条：**那块牌子在这一章没有名字。**
//
// ---------------------------------------------------------------------------
// 为什么这一条要单独有一个文件
// ---------------------------------------------------------------------------
// 设计文档 docs/ch04-design.md 第 7 节把本章的验收列成六条。第 2、3 条
//（通关、三样引擎前置）落在 tests/Ch04SliceTests.cpp 与 Ch04{Magic,Wave,Alchemy}Tests
// 里，那些都是「玩得下去吗」。第 4 条问的是另一件事，而且没有任何一条现成的用例
// 覆盖得到它：
//
//   「升仙令的『无名』要有测试钉住：那件物品的名字与描述里不得出现『升仙』二字，
//     也不得出现任何用途暗示。**这一条比它听上去要紧**——它是四章之后才兑现的
//     伏笔，中间任何一次『顺手补个描述』都会把它毁掉。」
//
// Ch04SliceTests 里那块牌子只作为**数量**出现（itemCount(kPlaque) == 1）。
// 一条只数个数的断言对「有人给它补了一句『似有来历』」是零敏感度的：
// 补完照样是 1 件。而这正是本项目栽过的那种形状——第 3 章两种杀的触发方式装反，
// 四条测试对它满格敏感却一条没发现，因为**从来没有人写下过「应该是什么」**。
//
// 所以本文件刻意与通关测试不同形状：
//   · 不走位、不起脚本、不打仗，直接读 data/ 与 scripts/ 里的字；
//   · 判据抄自设计文档第 1.1 节与 data/items/materials/heipai.json 的 note 原文，
//     不是「仓库现在长什么样」；
//   · 禁词表自己先过一遍负向验证——把设计文档举的那三句反例喂进同一个扫描器，
//     三句都必须被抓住。禁词表被清空、正则被吃掉转义这两种空转法，本项目都栽过。
//
// ---------------------------------------------------------------------------
// 这条用例看不见什么（写下来，免得后人以为它管的比实际多）
// ---------------------------------------------------------------------------
// 判据的原话是「一个只看物品栏的玩家，应当把它当成一件没用的杂物」。
// 「像不像杂物」是人读出来的，机器只查得了禁词与结构。具体地说，本文件能证明的是：
//   能 —— 名字与描述里没有那三个字、没有列在表上的用途暗示；
//          牌子那一段的台词没有多出没审过的句子；
//          全章 291 条 ch04 文案里没有「升仙」；
//          除了战利品那一节，没有第二个脚本碰这件东西；
//          它不在任务目录里，也不会在卖出面板上被单拎出来。
//   不能 —— 有人用一句**不含任何禁词**的话暗示它不凡（「他觉得这东西不该出现在
//          一个凡人身上」）。那一句能过本文件，但过不了独立校对。
// 换言之这是一道**下限**，不是判据本身。别把它当成「过了就没问题」。
//
// ---------------------------------------------------------------------------
// 与邻居的分工（写之前逐个读过）
// ---------------------------------------------------------------------------
//   * tests/Ch04SliceTests.cpp —— 整章走通；牌子在那里只管「到没到手」。
//   * tests/ShopTests.cpp      —— tradeable=false 不上卖出列表这条**机制**已在那里
//     钉死（含配对的正向用例）。本文件不重复证机制，只证**这一件东西**确实
//     落在那条机制的保护里。
//   * tests/Ch03TriggerModeTests.cpp —— 同一种形状的前例：直接读数据、判据写死
//     成设计原文。本文件的骨架照它写。
#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "TempDir.h"
#include "core/model/Types.h"
#include "core/rules/Realm.h"
#include "game/Application.h"
#include "game/ShopScene.h"
#include "game/Wording.h"
#include "io/ShopLoader.h"

namespace {

namespace fs = std::filesystem;

using fanren::core::Item;
using fanren::game::Application;
using fanren::game::PanelStage;
using fanren::game::ShopScene;

// 本章那件东西。名字取「一块黑牌子」的理由写在 data 的 note 里：设计文档举的例子
// 是「一块古怪的牌子」，而**判据排在例子前面**——「古怪」是个招人多看一眼的词。
constexpr const char* kPlaque = "material_heipai";
constexpr const char* kPlaqueDescKey = "item.desc.material_heipai";
constexpr const char* kSpoilsScript = "scripts/ch04/zhanlipin.lua";

// 牌子那一段从这一行之后开始：剑符那一半以它自己的旗标收尾。
// 用结构锚点而不是行号，是因为行号会被任何一次改动作废，而这个锚点不会——
// 它没了，剑符那个二选一就没了。
constexpr const char* kTalismanHalfEndsAt = "flag.set(\"ch04.jianfu_dedao\"";

// ---------------------------------------------------------------------------
// 判据。三条禁令抄自 docs/ch04-design.md 第 1.1 节与 heipai.json 的 note
// ---------------------------------------------------------------------------
//   1. name 与 descKey 指的那条文案里，不得出现「升仙」二字；
//      也不得出现「令」这个字**作它的名**。
//   2. 描述里不得有任何用途暗示——不写「似有来历」「不知有何妙用」
//      「日后或有大用」这一类。
//   3. 没有任何 NPC 解释它，它也不进任务栏。
//
// 下面这张表是第 2 条的机器形式。分四族，每一族都是**替玩家把话说了**的那种词：
// 它派什么用场、它从哪来、它值多少、它日后会怎样。
//
// **刻意不收「古怪」。** 设计文档把「一块古怪的牌子」列为可接受的举例，
// 实现方选了更严的「一块黑牌子」并写明了理由。禁词表若比设计文档还严，
// 将来有人照文档改回举例的那个名字，红的会是这条断言而不是那次改动——
// 判据不该比它引的那份文档更硬。
const std::vector<std::string>& forbiddenMarks() {
    static const std::vector<std::string> kMarks = {
        // 用途
        "妙用", "大用", "有用", "用处", "派得上", "用得着",
        // 来历
        "来历", "出处", "不凡", "非凡", "不俗", "来头",
        // 价值
        "宝物", "至宝", "法宝", "灵器", "珍贵", "贵重", "值钱", "宝贝",
        // 许诺
        "日后", "将来", "他日", "早晚", "迟早",
        // 猜测的口气
        "定有", "必有", "或有", "莫非", "说不定", "兴许",
        // 名字本身（第 1 条；「令」只管名字，见下面单独那一条）
        "升仙", "令牌", "仙令",
    };
    return kMarks;
}

// 命中的第一个禁词；没有则返回空串。抽成函数是为了能把反例也喂进来。
[[nodiscard]] std::string firstForbiddenMark(const std::string& text) {
    for (const std::string& mark : forbiddenMarks()) {
        if (text.find(mark) != std::string::npos) return mark;
    }
    return std::string{};
}

std::string assetRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        const fs::path root(candidate);
        if (fs::exists(root / "data" / "items" / "materials" / "heipai.json") &&
            fs::exists(root / kSpoilsScript)) {
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

// 一行 lua 里所有被双引号括起来的串，按出现顺序。
std::vector<std::string> quotedOn(const std::string& line) {
    std::vector<std::string> out;
    size_t pos = 0;
    while (true) {
        const size_t open = line.find('"', pos);
        if (open == std::string::npos) break;
        const size_t close = line.find('"', open + 1);
        if (close == std::string::npos) break;
        out.push_back(line.substr(open + 1, close - open - 1));
        pos = close + 1;
    }
    return out;
}

// 战利品脚本里**牌子那一段**说出口的文案 key，按脚本里的顺序。
// 取「剑符那一半的收尾之后的每一句 talk」——剑符是该被解释的，牌子不是，
// 这个分界本身就是设计（zhanlipin.lua 首部原话）。
std::vector<std::string> plaqueSegmentKeys(const std::string& script) {
    std::vector<std::string> keys;
    std::istringstream lines(script);
    std::string line;
    bool afterTalismanHalf = false;
    while (std::getline(lines, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        // 注释行不算：脚本首部整段都在讲这件事，里头当然写着「升仙令」。
        const size_t firstNonSpace = line.find_first_not_of(" \t");
        if (firstNonSpace != std::string::npos && line.compare(firstNonSpace, 2, "--") == 0) {
            continue;
        }
        if (line.find(kTalismanHalfEndsAt) != std::string::npos) {
            afterTalismanHalf = true;
            continue;
        }
        if (!afterTalismanHalf) continue;
        if (line.find("talk(") == std::string::npos) continue;
        for (const std::string& quoted : quotedOn(line)) {
            if (!quoted.empty()) keys.push_back(quoted);
        }
    }
    return keys;
}

// 本文件**逐句读过**的那几条。set 相等在下面双向比对：
// 脚本里多出一句没审过的 → 红；这里留着一条脚本已经删掉的 → 也红。
const std::vector<std::string>& reviewedPlaqueKeys() {
    static const std::vector<std::string> kKeys = {
        "ch04.zhanli.book",    // 家谱：让这一堆看起来就是一堆杂物
        "ch04.zhanli.pai",     // 拨出来：三角、磨圆的边、比看着沉
        "ch04.zhanli.feel",    // 掂了掂：不是铁，也不是石头
        "ch04.zhanli.marks",   // 印子：看不出是字还是花纹
        "ch04.zhanli.pocket",  // 收起来的理由写成了没有理由
        "ch04.zhanli.end",     // 拍灰、裤子
    };
    return kKeys;
}

class Ch04PlaqueHasNoName : public ::testing::Test {
protected:
    void SetUp() override {
        root_ = assetRoot();
        auto ready = app_.init(root_, /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
        script_ = readFile(fs::path(root_) / kSpoilsScript);
    }

    void TearDown() override { app_.shutdown(); }

    // Application::data() 只有非 const 重载，所以这两个也不能是 const 成员。
    const Item* plaque() { return app_.data().findItem(kPlaque); }
    std::string textOf(const std::string& key) { return app_.data().lookupText(key); }

    Application app_;
    std::string root_;
    std::string script_;
};

// ---------------------------------------------------------------------------
// 先验一 · 被查的那几样字确实有内容
// ---------------------------------------------------------------------------
// 「找不到某个词就算过」是本项目栽过的空转法：被查的字符串一旦塌成空的，
// 下面每一条 find() 都恒真。GameData::lookupText 查不到时**返回 key 本身**，
// 而 key 里写的是 material_heipai，一个禁词也不含——少了这一条，
// 把 data/text/ch04_items.json 整个删掉，下面全部照样绿。
TEST_F(Ch04PlaqueHasNoName, TheStringsUnderTestAreReallyThereBeforeAnythingIsAssertedAboutThem) {
    const Item* item = plaque();
    ASSERT_NE(item, nullptr) << "data 里没有 " << kPlaque << "，这条用例无从谈起";
    EXPECT_EQ(item->descKey, kPlaqueDescKey);
    ASSERT_FALSE(item->name.empty()) << "名字是空的";

    const std::string desc = textOf(item->descKey);
    ASSERT_NE(desc, item->descKey) << "文案查不到，lookupText 把 key 原样还回来了";
    ASSERT_GE(desc.size(), 60u) << "描述短得不像一条描述：" << desc;

    ASSERT_FALSE(script_.empty()) << "读不到 " << kSpoilsScript;
    ASSERT_NE(script_.find("give(\"material_heipai\""), std::string::npos)
        << "战利品那一节已经不发这件东西了，本条用例守的门已经不在原处";

    for (const std::string& key : reviewedPlaqueKeys()) {
        const std::string line = textOf(key);
        ASSERT_NE(line, key) << key << " 这条文案查不到";
        ASSERT_FALSE(line.empty()) << key << " 是空的";
    }
}

// ---------------------------------------------------------------------------
// 先验二 · 禁词表自己有牙
// ---------------------------------------------------------------------------
// 一张被清空的禁词表照样能让扫描器跑完并报通过。所以先拿设计文档**自己举的
// 那三句反例**喂进同一个 firstForbiddenMark，三句都必须被抓住；
// 外加那个四章之后才该出现的名字。
// 反例取自 docs/ch04-design.md 第 1.1 节与 heipai.json note 的原文，不是我编的。
TEST_F(Ch04PlaqueHasNoName, TheScannerCatchesTheVeryPhrasesTheDesignDocForbids) {
    ASSERT_FALSE(forbiddenMarks().empty()) << "禁词表是空的，扫描器无词可扫";
    for (const std::string& mark : forbiddenMarks()) {
        ASSERT_FALSE(mark.empty()) << "禁词表里有一条空串，它会命中任何文本";
    }

    for (const char* counterexample : {"似有来历", "不知有何妙用", "日后或有大用",
                                       "此物名唤升仙令", "一块古怪的令牌"}) {
        EXPECT_FALSE(firstForbiddenMark(counterexample).empty())
            << "扫描器漏掉了设计文档点名禁掉的写法：" << counterexample;
    }

    // 配对的正向：仓库里那条真描述必须**不**被判为违规，否则上面几条只是因为
    // 扫描器见什么抓什么。
    EXPECT_TRUE(firstForbiddenMark("三角形的一块，边上磨得发圆").empty());
}

// ---------------------------------------------------------------------------
// 正题一 · 名字与描述（验收第 4 条的原话）
// ---------------------------------------------------------------------------
TEST_F(Ch04PlaqueHasNoName, NeitherTheNameNorTheDescriptionCarriesTheNameItOnlyGetsFourChaptersLater) {
    const Item* item = plaque();
    ASSERT_NE(item, nullptr);
    const std::string desc = textOf(item->descKey);

    EXPECT_EQ(item->name.find("升仙"), std::string::npos)
        << "「升仙」二字在原著里首见于 ch141，本章（ch65-99）不得出现：" << item->name;
    EXPECT_EQ(desc.find("升仙"), std::string::npos)
        << "描述里出现了「升仙」：" << desc;
    // 第 1 条后半句只管名字：不得拿「令」这个字**作它的名**。
    EXPECT_EQ(item->name.find("令"), std::string::npos)
        << "名字里带了「令」字，玩家会自己把它归成一类：" << item->name;
}

TEST_F(Ch04PlaqueHasNoName, TheDescriptionMakesNoPromiseAboutWhatItIsFor) {
    const Item* item = plaque();
    ASSERT_NE(item, nullptr);

    EXPECT_TRUE(firstForbiddenMark(item->name).empty())
        << "名字里有用途暗示「" << firstForbiddenMark(item->name) << "」：" << item->name;

    const std::string desc = textOf(item->descKey);
    EXPECT_TRUE(firstForbiddenMark(desc).empty())
        << "描述里有用途暗示「" << firstForbiddenMark(desc) << "」。"
        << "描述只写他看见了什么，判据见 docs/ch04-design.md 1.1：" << desc;
}

// ---------------------------------------------------------------------------
// 正题二 · 战利品那一节里，牌子那一段没有多出没审过的句子
// ---------------------------------------------------------------------------
// 光扫现有的几句不够：伏笔坏在「顺手补一句」上，而补出来的那一句是新的，
// 一张只列旧 key 的表根本看不见它。所以从脚本里**推**出这一段实际说了哪几句，
// 与本文件逐句读过的那张表双向比对——多一句、少一句都红。
TEST_F(Ch04PlaqueHasNoName, NobodyHasSlippedAnExtraLineIntoThePlaqueHalfOfTheScene) {
    std::vector<std::string> spoken = plaqueSegmentKeys(script_);
    ASSERT_FALSE(spoken.empty())
        << "从 " << kSpoilsScript << " 里一句也没解析出来——锚点 "
        << kTalismanHalfEndsAt << " 多半已经不在了，这条用例正在空转";

    std::vector<std::string> reviewed = reviewedPlaqueKeys();
    std::sort(spoken.begin(), spoken.end());
    std::sort(reviewed.begin(), reviewed.end());
    EXPECT_EQ(spoken, reviewed)
        << "牌子那一段的台词与本文件读过的那几句对不上。多出来的那一句没人审过，"
           "少掉的那一句说明这张表已经过期——两种都要先读 docs/ch04-design.md 1.1 "
           "再决定怎么改。";
}

TEST_F(Ch04PlaqueHasNoName, NotOneLineOfThatSceneTellsThePlayerWhatHeIsHolding) {
    // 扫脚本推出来的那一段（而不是本文件那张表），这样新加的句子也在扫描范围内：
    // 上一条负责让它红，这一条负责让它红得有话说。
    for (const std::string& key : plaqueSegmentKeys(script_)) {
        const std::string line = textOf(key);
        ASSERT_NE(line, key) << key << " 查不到文案";
        EXPECT_TRUE(firstForbiddenMark(line).empty())
            << key << " 里有「" << firstForbiddenMark(line) << "」。"
            << "zhanlipin.lua 首部原话：这一段以下不许出现任何解释性的台词。——" << line;
    }

    // 重进那一节说的那句也一并扫：它是这一景唯一一条不在上面那段里的台词。
    const std::string again = textOf("ch04.zhanli.again");
    ASSERT_NE(again, "ch04.zhanli.again");
    EXPECT_TRUE(firstForbiddenMark(again).empty()) << again;
}

// ---------------------------------------------------------------------------
// 正题三 · 全章没有第二个人提起它
// ---------------------------------------------------------------------------
// 第 3 条禁令：没有任何 NPC 解释它。机器查得到的是两件事——
// 全章文案里没有那个名字；除战利品那一节外没有第二个脚本碰这件东西。
TEST_F(Ch04PlaqueHasNoName, NoOneElseInThisChapterSaysThatNameOutLoud) {
    int scanned = 0;
    for (const auto& [key, line] : app_.data().text) {
        if (key.rfind("ch04.", 0) != 0) continue;
        ++scanned;
        EXPECT_EQ(line.find("升仙"), std::string::npos)
            << key << " 里出现了「升仙」，而那三个字在原著里首见于 ch141：" << line;
    }
    // 先验分母：扫了 0 条照样一条不报。本章文案在交接时是 291 条，
    // 这里只要一个不会被日常增删碰到的下限。
    ASSERT_GT(scanned, 100) << "只扫到 " << scanned << " 条 ch04 文案，扫描器多半没扫着东西";
}

// 扫的是**整棵 `scripts/`**，不是 `scripts/ch04/`。
//
// 这一条起初只扫本章目录，独立校对（LOW-1）指出那是错的射程：这件伏笔要活到
// 第 6 章，而第 5 章的人在自己的脚本里提一句 `material_heipai`，只看本章目录的
// 断言一个字也看不见。伏笔正是死在「别处顺手提一句」上的。
//
// 白名单里现在只有一个名字。**第 6 章揭破它的那个脚本落地时，把它加进来**——
// 到那时这条断言就从「只有战利品那一节碰它」变成「只有这两处碰它」，
// 而中间那四章仍然一个字也不许提。
//
// **白名单按相对 `scripts/` 的路径比对，不按文件名**（复验 N-10）。从前比的是
// `filename()`，于是任何一章的目录里只要有个脚本也叫 zhanlipin.lua，就会被当成
// 第 4 章那一处放行——而第 6 章揭破它的那一处正该是**另一个**被点名加进来的路径。
const std::vector<std::string>& scriptsAllowedToTouchThePlaque() {
    static const std::vector<std::string> kAllowed = {
        "ch04/zhanlipin.lua",  // 第 4 章节点 10，他从灰里翻出它的那一处
        "ch06/bilu.lua",       // 第 6 章节点 10b，《青溪笔录》识破它是黄枫谷的升仙令（收走牌子、给出令牌）
    };
    return kAllowed;
}

// scripts/ 下提到这件东西的 .lua，按相对路径（正斜杠）排好。seen 回填一共扫了几个脚本。
// 抽成函数是为了能拿一棵就地搭的假目录喂它（下面那条自检），证明它比的真是路径。
std::vector<std::string> scriptsTouchingThePlaque(const fs::path& scripts, int& seen) {
    std::vector<std::string> touching;
    seen = 0;
    for (const auto& entry : fs::recursive_directory_iterator(scripts)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ".lua") continue;
        ++seen;
        if (readFile(entry.path()).find(kPlaque) != std::string::npos) {
            touching.push_back(fs::relative(entry.path(), scripts).generic_string());
        }
    }
    std::sort(touching.begin(), touching.end());
    return touching;
}

TEST_F(Ch04PlaqueHasNoName, OnlyTheSpoilsSceneEverTouchesIt) {
    const fs::path scripts = fs::path(root_) / "scripts";
    ASSERT_TRUE(fs::is_directory(scripts)) << "找不到 " << scripts.string();

    int seen = 0;
    const std::vector<std::string> touching = scriptsTouchingThePlaque(scripts, seen);
    // 先验分母：扫了一两个文件照样一条不报。交接时全仓是 71 个脚本，
    // 这里只要一个不会被日常增删碰到的下限。
    ASSERT_GT(seen, 30) << "只扫到 " << seen << " 个脚本，这条用例没扫着东西";

    EXPECT_EQ(touching, scriptsAllowedToTouchThePlaque())
        << "碰这件东西的脚本与白名单对不上。多出来的那一个多半是在解释它，"
           "而设计文档第 1.1 节写死了：没有任何 NPC 解释它。"
           "第 6 章那一处除外——到那时请把它的相对路径加进 scriptsAllowedToTouchThePlaque()。";
}

// 自检：比的是路径，不是文件名（复验 N-10）。就地搭一棵假的 scripts/：第 4 章那一处之外，
// 第 5 章的目录里另有一个**同名**脚本也提到它。按文件名比，两个都叫 zhanlipin.lua，
// 第 5 章那一个会被白名单放行；按路径比，它必须被报出来。
TEST_F(Ch04PlaqueHasNoName, TheWhitelistComparesPathsSoASameNamedScriptElsewhereIsCaught) {
    fanren::test::TempDir tree("fanren_plaque_paths");
    const fs::path scripts = tree.path() / "scripts";
    fs::create_directories(scripts / "ch04");
    fs::create_directories(scripts / "ch05");
    const auto write = [](const fs::path& path, const std::string& body) {
        std::ofstream out(path, std::ios::binary);
        out << body;
    };
    write(scripts / "ch04" / "zhanlipin.lua", std::string("give(\"") + kPlaque + "\", 1)\n");
    write(scripts / "ch05" / "zhanlipin.lua", std::string("-- 又提了一句 ") + kPlaque + "\n");
    write(scripts / "ch05" / "other.lua", "talk(\"\", \"ch05.x\")\n");

    int seen = 0;
    const std::vector<std::string> touching = scriptsTouchingThePlaque(scripts, seen);
    ASSERT_EQ(seen, 3) << "先验：三个脚本都扫到了";
    const std::vector<std::string> expected = {"ch04/zhanlipin.lua", "ch05/zhanlipin.lua"};
    EXPECT_EQ(touching, expected) << "扫出来的不是带目录的相对路径";
    EXPECT_NE(touching, scriptsAllowedToTouchThePlaque())
        << "第 5 章那个同名脚本被白名单放行了：比的是文件名，不是路径";
}

// ---------------------------------------------------------------------------
// 判据 3 的守卫：攻防战那句台词里的数（复验 N-6）
// ---------------------------------------------------------------------------
// `ch04.gongfang.count`：「他数了数自己剩的：一炉药，八十来点力气，一双手。」
// 那时韩立已在节点 4 升到炼气八层，而设计 1.2 写死八层是「攻 15 / 防 8 / 气血 120 /
// 法力 80」。这句话与那个数的对应从前只靠人记：改成「七十来点」全套一条不红。
//
// 这一条**直接读文案**，把「来点力气」前面那个中文数读出来，与设计 1.2 的 80 比；
// 再把引擎的 realmMaxMp(炼气八层) 与同一个 80 比——两头都钉在设计原文上，
// 哪一头动了都红，而不是拿一头去验另一头。
constexpr int kDesignManaAtTheEighthLayer = 80;   // docs/ch04-design.md 1.2
constexpr const char* kSiegeCountKey = "ch04.gongfang.count";
constexpr const char* kManaPhrase = "来点力气";

// 一个 UTF-8 串里连续的中文数字读成整数（一……九、十、百），读不出返回 -1。
// 只管这句台词会用到的量级（一百以内加几十），够用就行。
int parseChineseNumber(const std::string& text) {
    static const std::vector<std::pair<std::string, int>> kDigits = {
        {"一", 1}, {"二", 2}, {"两", 2}, {"三", 3}, {"四", 4}, {"五", 5},
        {"六", 6}, {"七", 7}, {"八", 8}, {"九", 9}};
    int total = 0;
    int pending = -1;
    std::size_t pos = 0;
    bool any = false;
    while (pos < text.size()) {
        bool matched = false;
        for (const auto& [glyph, value] : kDigits) {
            if (text.compare(pos, glyph.size(), glyph) == 0) {
                pending = value;
                pos += glyph.size();
                matched = any = true;
                break;
            }
        }
        if (matched) continue;
        if (text.compare(pos, 3, "十") == 0) {
            total += (pending < 0 ? 1 : pending) * 10;
            pending = -1;
            pos += 3;
            any = true;
            continue;
        }
        if (text.compare(pos, 3, "百") == 0) {
            total += (pending < 0 ? 1 : pending) * 100;
            pending = -1;
            pos += 3;
            any = true;
            continue;
        }
        return -1;
    }
    if (!any) return -1;
    return total + (pending < 0 ? 0 : pending);
}

// 台词里紧挨着「来点力气」前面的那一串中文数字。
std::string numberBeforeManaPhrase(const std::string& line) {
    const std::size_t at = line.find(kManaPhrase);
    if (at == std::string::npos) return {};
    static const std::vector<std::string> kGlyphs = {"一", "二", "两", "三", "四", "五", "六",
                                                     "七", "八", "九", "十", "百"};
    std::size_t begin = at;
    while (begin >= 3) {
        const std::string previous = line.substr(begin - 3, 3);
        if (std::find(kGlyphs.begin(), kGlyphs.end(), previous) == kGlyphs.end()) break;
        begin -= 3;
    }
    return line.substr(begin, at - begin);
}

TEST(Ch04SiegeCount, TheNumberReaderReadsWhatTheLineCouldSay) {
    // 先验：读数器自己有牙。改成「七十」正是复验时没人发现的那一种写坏。
    EXPECT_EQ(parseChineseNumber("八十"), 80);
    EXPECT_EQ(parseChineseNumber("七十"), 70);
    EXPECT_EQ(parseChineseNumber("十五"), 15);
    EXPECT_EQ(parseChineseNumber("一百二十"), 120);
    EXPECT_EQ(parseChineseNumber("力气"), -1);
    EXPECT_EQ(parseChineseNumber(""), -1);
    EXPECT_EQ(numberBeforeManaPhrase("一炉药，七十来点力气，一双手"), "七十");
    EXPECT_EQ(numberBeforeManaPhrase("一炉药，满身力气"), "");
}

TEST_F(Ch04PlaqueHasNoName, TheSiegeLineCountsTheManaTheDesignGivesTheEighthLayer) {
    const std::string line = textOf(kSiegeCountKey);
    // 先验：文案真的在（查不到时 lookupText 返回 key 本身），而且还是那个说法。
    ASSERT_NE(line, kSiegeCountKey) << "文案里没有 " << kSiegeCountKey;
    const std::string digits = numberBeforeManaPhrase(line);
    ASSERT_FALSE(digits.empty())
        << kSiegeCountKey << " 里找不到「<中文数>" << kManaPhrase << "」这个说法了。"
        << "那句话要是改了口，请照新的说法改这条用例，数仍旧对着设计 1.2：" << line;

    EXPECT_EQ(parseChineseNumber(digits), kDesignManaAtTheEighthLayer)
        << kSiegeCountKey << " 说的是「" << digits << kManaPhrase << "」，而设计 1.2 写死"
        << "炼气八层法力 " << kDesignManaAtTheEighthLayer << "：" << line;
    EXPECT_EQ(fanren::rules::realmMaxMp(fanren::rules::Realm::QiRefining8),
              kDesignManaAtTheEighthLayer)
        << "引擎给炼气八层的法力与设计 1.2 不符，那句台词跟着就错了";
}

// ---------------------------------------------------------------------------
// 正题四 · 它不进任务栏，也不会在面板上被单拎出来
// ---------------------------------------------------------------------------
// 第 3 条禁令的后半句。data/quests/ 现在是空目录（还没造加载器），
// 所以这一条今天必然通过——它守的是**将来**：任务系统落地的那天，
// 谁把这块牌子登记成任务物品，这里红。写明这一点，免得后人把它当成已验过的事。
TEST_F(Ch04PlaqueHasNoName, ItIsNotAQuestItemAndTheQuestFolderIsStillEmptyAnyway) {
    const fs::path quests = fs::path(root_) / "data" / "quests";
    if (!fs::is_directory(quests)) {
        GTEST_SKIP() << "data/quests/ 还不存在";
    }
    for (const auto& entry : fs::recursive_directory_iterator(quests)) {
        if (!entry.is_regular_file()) continue;
        EXPECT_EQ(readFile(entry.path()).find(kPlaque), std::string::npos)
            << entry.path().filename().string() << " 把这块牌子登记成了任务物品。"
            << "设计文档 1.1：它不是任务物品，不进任务栏。";
    }
}

// 卖出面板不会把它单拎出来。tradeable=false 会被 ShopScene 静默跳过——
// **机制**已由 tests/ShopTests.cpp 连同配对的正向用例钉死，这里只证这一件东西
// 确实落在那条机制里：卖得掉，第 6 章就没了；被写成「此物不可出售」，
// 它又在一排药材里跳出来了。
TEST_F(Ch04PlaqueHasNoName, TheSellPanelNeitherPricesItNorPointsAtIt) {
    const Item* item = plaque();
    ASSERT_NE(item, nullptr);
    EXPECT_FALSE(item->tradeable) << "卖得掉的话，第 6 章那一下就没了";
    EXPECT_EQ(item->price, 0) << "标了价，玩家就知道它值多少";

    auto shop = fanren::io::loadShop(
        (fs::path(root_) / "data" / "shops" / "ch02_wairentang_yaoshang.json").string());
    ASSERT_TRUE(shop.ok) << shop.error;

    app_.state().bag.clear();
    app_.state().addItem(kPlaque, 1, 0);
    app_.state().addItem("herb_huangjing_cao", 1, 1);
    const auto stacks = ShopScene::buildSellStacks(app_.data(), app_.state(), shop.value);

    // 配对的正向先来：列表恒为空的话，下面那条循环什么也证不了。
    ASSERT_EQ(stacks.size(), 1u) << "卖出列表该只剩那株黄精";
    EXPECT_EQ(stacks.front().itemId, "herb_huangjing_cao");

    // 用词阶段取 Mortal：本章的钱还是「碎银，按块计」，「灵石」要到第 5 章才该听说。
    for (const auto& item2 :
         ShopScene::buildSellItems(PanelStage::Mortal, app_.data(), stacks)) {
        EXPECT_EQ(item2.label.find(item->name), std::string::npos)
            << "卖出面板上出现了这块牌子：" << item2.label;
        EXPECT_EQ(item2.disabledReason.find(item->name), std::string::npos)
            << "面板专门为它写了一句话，它就不是杂物了：" << item2.disabledReason;
    }
}

}  // namespace
