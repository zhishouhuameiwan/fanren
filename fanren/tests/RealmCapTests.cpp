// 剧情境界上限：玩家自己在打坐面板上按的突破，不能越过剧情最近一次给的上限。
//
// ---------------------------------------------------------------------------
// 为什么要有这一个文件（技术债 G-14，第 4 章二次整改）
// ---------------------------------------------------------------------------
// 没有上限的时候，章与章之间的境界不是确定值：第 4 章的战斗奖励加日课够玩家自己按到
// 炼气九层，而设计写死「本章结束时是炼气八层」；第 2 章多打坐几十次就能把面板刷到
// 第五、第六层，而师父刚说他到第三层（第 2 章复审 N5）。平衡路要把第 4 章通关测试的
// 起点接到第 3 章通关测试的终局存档上，这件事依赖第 3 章终局的境界是确定的。
//
// ---------------------------------------------------------------------------
// 判据从哪来（一律写成字面量，不从被测常量推）
// ---------------------------------------------------------------------------
//   新开局凡人               docs/ch01-design.md 第 3 节节点 9：授口诀在章末，此前没有口诀
//   授口诀之后一层           同上 ＋ docs/ch02-design.md 第 2 节：段四才到第二层，此前至多第一层
//   第 2 章段四二层          docs/ch02-design.md 第 2 节段四
//   第 2 章段五三层          docs/ch02-design.md 第 1 节第 5 条、第 2 节段五
//   第 3 章不抬（三层）      docs/ch04-design.md 1.2：第 3 章章末炼气三层
//   第 4 章节点 1/2/4        docs/ch04-design.md 1.2：五 / 七 / 八层，由 realm.advance 顺带抬
// 契约：docs/interfaces-p2.md 第 7 节、docs/interfaces-p3-script.md 第 7 节。
#include <gtest/gtest.h>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

#include "TempDir.h"
#include "core/model/Types.h"
#include "core/rules/Cultivation.h"
#include "core/rules/Realm.h"
#include "game/Application.h"
#include "game/CultivationScene.h"
#include "game/Wording.h"
#include "io/SaveFile.h"
#include "script/Command.h"

namespace {

namespace fs = std::filesystem;

using fanren::core::GameState;
using fanren::game::Application;
using fanren::game::CultivationScene;
using fanren::game::PanelStage;
using fanren::rules::BreakthroughAttempt;
using fanren::rules::BreakthroughBlock;
using fanren::rules::Realm;
using fanren::rules::breakthroughBlock;
using fanren::rules::tryBreakthrough;

fs::path findProjectRoot() {
    fs::path dir = fs::current_path();
    for (int depth = 0; depth < 8; ++depth) {
        if (fs::exists(dir / "scripts" / "common" / "api.lua") && fs::is_directory(dir / "data") &&
            fs::is_directory(dir / "src")) {
            return dir;
        }
        const fs::path parent = dir.parent_path();
        if (parent.empty() || parent == dir) break;
        dir = parent;
    }
    return {};
}

// 按到第一次成为止：突破是骰子，换一天换一颗种子（breakthroughSeed 是纯函数，
// 所以这个循环是确定的）。修为给足，失手的倒扣不会把它扣到门槛以下。
BreakthroughAttempt pressUntilItWorks(Application& app, int maxDays = 400) {
    BreakthroughAttempt last;
    for (int day = 1; day <= maxDays; ++day) {
        app.state().day = day;
        CultivationScene panel;
        last = panel.breakthrough(app);
        if (last.success || last.blocked != BreakthroughBlock::None) return last;
    }
    return last;
}

// ===========================================================================
// 一、规则层的唯一入口
// ===========================================================================

TEST(RealmCapRule, AtTheCapNoAmountOfCultivationGetsThrough) {
    EXPECT_EQ(breakthroughBlock(Realm::QiRefining3, 1000000, Realm::QiRefining3),
              BreakthroughBlock::StoryCap);
    const BreakthroughAttempt refused =
        tryBreakthrough(Realm::QiRefining3, 1000000, Realm::QiRefining3, 100, 100, 7u);
    EXPECT_EQ(refused.blocked, BreakthroughBlock::StoryCap);
    EXPECT_FALSE(refused.success);
    EXPECT_EQ(refused.cultivationSpent, 0);
    EXPECT_EQ(refused.cultivationLost, 0) << "按不下去不是一次尝试，一分不扣";
}

TEST(RealmCapRule, ACapThatAllowsExactlyTheNextLayerLetsTheDiceDecide) {
    // 上限恰好是下一层：放行。与上一条合起来夹住「差一层」的两个方向。
    EXPECT_EQ(breakthroughBlock(Realm::QiRefining2, 1000, Realm::QiRefining3),
              BreakthroughBlock::None);
    // 放行之后就是骰子本身，一个比特都不另改：同一颗种子，结果逐项相同。
    for (std::uint32_t seed = 1; seed <= 64; ++seed) {
        const BreakthroughAttempt viaEntry =
            tryBreakthrough(Realm::QiRefining2, 40, Realm::QiRefining3, 30, 0, seed);
        const BreakthroughAttempt dice =
            fanren::rules::attemptBreakthrough(Realm::QiRefining2, 40, 30, 0, seed);
        EXPECT_EQ(viaEntry.blocked, BreakthroughBlock::None);
        EXPECT_EQ(viaEntry.success, dice.success) << "seed " << seed;
        EXPECT_EQ(viaEntry.cultivationSpent, dice.cultivationSpent) << "seed " << seed;
        EXPECT_EQ(viaEntry.cultivationLost, dice.cultivationLost) << "seed " << seed;
        EXPECT_EQ(viaEntry.backlash, dice.backlash) << "seed " << seed;
    }
}

TEST(RealmCapRule, TheCapIsReportedBeforeTheShortfall) {
    // 到了瓶颈、修为也不够：报瓶颈。那时说「尚差几点」是在骗人——差的不是点数。
    EXPECT_EQ(breakthroughBlock(Realm::QiRefining3, 0, Realm::QiRefining3),
              BreakthroughBlock::StoryCap);
    // 对照组：上限没到时，修为不够就是修为不够。
    EXPECT_EQ(breakthroughBlock(Realm::QiRefining2, 0, Realm::QiRefining3),
              BreakthroughBlock::NotEnough);
    // 本作上限另是一回事，与剧情无关。
    EXPECT_EQ(breakthroughBlock(Realm::CoreLate, 1000000, Realm::CoreLate),
              BreakthroughBlock::SeriesMax);
}

// ---- 扫源码找「绕过唯一入口直接摇骰子」的那几行 ----
//
// 去掉 // 行注释与 /* */ 块注释，换行留着。**不认字符串字面量**：字符串里出现 "//" 时，
// 那一行后半截会被当成注释丢掉（可能漏报）。src/ 眼下没有这种写法，申报在下面那条用例的注释里。
std::string withoutComments(const std::string& source) {
    std::string out;
    out.reserve(source.size());
    for (std::size_t i = 0; i < source.size();) {
        if (source.compare(i, 2, "//") == 0) {
            const std::size_t eol = source.find('\n', i);
            if (eol == std::string::npos) break;
            i = eol;   // 换行本身留着
            continue;
        }
        if (source.compare(i, 2, "/*") == 0) {
            const std::size_t close = source.find("*/", i + 2);
            if (close == std::string::npos) break;
            out.push_back(' ');
            i = close + 2;
            continue;
        }
        out.push_back(source[i]);
        ++i;
    }
    return out;
}

bool isIdentifierChar(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_';
}

// 注释之外，attemptBreakthrough 以**整词**出现过没有——不论后面跟的是 `(`、`;` 还是别的。
// 按整词而不是按 `attemptBreakthrough(` 这个子串：取函数指针再调用
//（`auto dice = rules::attemptBreakthrough; dice(...)`）与 `using` 引进来，都要算。
bool mentionsTheDiceOutsideComments(const std::string& source) {
    const std::string code = withoutComments(source);
    const std::string word = "attemptBreakthrough";
    for (std::size_t at = code.find(word); at != std::string::npos; at = code.find(word, at + 1)) {
        const bool leftClear = at == 0 || !isIdentifierChar(code[at - 1]);
        const std::size_t end = at + word.size();
        const bool rightClear = end >= code.size() || !isIdentifierChar(code[end]);
        if (leftClear && rightClear) return true;
    }
    return false;
}

// 先验扫描器本身：该抓的抓得住，注释与更长的名字不误报。
TEST(RealmCapRule, TheDiceScannerSeesPastCommentsAndCatchesFunctionPointers) {
    EXPECT_TRUE(mentionsTheDiceOutsideComments("a = rules::attemptBreakthrough(r, c, 0, 0, s);"));
    EXPECT_TRUE(mentionsTheDiceOutsideComments(
        "auto dice = rules::attemptBreakthrough;\na = dice(r, c, 0, 0, s);"))
        << "取函数指针再调用没被抓住";
    EXPECT_TRUE(mentionsTheDiceOutsideComments("using fanren::rules::attemptBreakthrough;"));
    EXPECT_FALSE(mentionsTheDiceOutsideComments("// 骰子是 attemptBreakthrough(...)，别直接调\nint x = 0;"))
        << "行注释里提一句被当成了调用";
    EXPECT_FALSE(mentionsTheDiceOutsideComments("/* attemptBreakthrough( */ int x = 0;"))
        << "块注释里提一句被当成了调用";
    EXPECT_FALSE(mentionsTheDiceOutsideComments("int attemptBreakthroughCount = 0;"))
        << "更长的名字被当成了这个函数";
}

// 「以后任何突破入口都走这一处」：游戏层不许绕过 tryBreakthrough 直接摇骰子。
// 扫的是 src/ 全树：注释之外，attemptBreakthrough 这个词只许出现在它自己的定义处
//（src/core/rules/Cultivation.h / .cpp，**按相对路径**豁免，别处的同名文件不豁免）。
//
// **它防的是常见误用，不是恶意绕过。** 看得见：直接调用、取函数指针再调用、`using` 引进来。
// 看不见（盲区，照 docs/README.md「判据自己会说谎」申报）：
//   · 宏拼接出来的名字（`a##Breakthrough` 一类）——源码里根本没有这个整词；
//   · 在 Cultivation.h / .cpp 里**另开**一个不查上限的公开函数再从别处调它——那两个文件是豁免的；
//   · 字符串字面量：注释剥离不认字符串，字符串里有 "//" 时那一行后半截会被丢掉（漏报），
//     字符串里写了这个函数名会被当成代码（误报）。src/ 眼下两种都没有。
TEST(RealmCapRule, NothingOutsideTheRulesLayerRollsTheBreakthroughDiceDirectly) {
    const fs::path root = findProjectRoot();
    ASSERT_FALSE(root.empty()) << "找不到工程根目录";
    std::vector<std::string> offenders;
    int scanned = 0;
    bool panelUsesTheEntry = false;
    for (const auto& entry : fs::recursive_directory_iterator(root / "src")) {
        if (!entry.is_regular_file()) continue;
        const std::string ext = entry.path().extension().string();
        if (ext != ".cpp" && ext != ".h") continue;
        ++scanned;
        std::ifstream in(entry.path(), std::ios::binary);
        std::ostringstream text;
        text << in.rdbuf();
        const std::string body = text.str();
        const std::string relative = fs::relative(entry.path(), root).generic_string();
        if (relative == "src/game/CultivationScene.cpp" &&
            withoutComments(body).find("rules::tryBreakthrough(") != std::string::npos) {
            panelUsesTheEntry = true;
        }
        if (relative == "src/core/rules/Cultivation.cpp" || relative == "src/core/rules/Cultivation.h") {
            continue;
        }
        if (mentionsTheDiceOutsideComments(body)) offenders.push_back(relative);
    }
    ASSERT_GT(scanned, 20) << "先验：真的扫到了源码";
    EXPECT_TRUE(panelUsesTheEntry) << "先验：打坐面板确实走的是 rules::tryBreakthrough";
    for (const std::string& file : offenders) {
        ADD_FAILURE() << file << " 绕过 rules::tryBreakthrough 直接摇了突破的骰子：剧情上限在那里不起作用";
    }
}

// ===========================================================================
// 二、面板：瓶颈与运气差分开说
// ===========================================================================

TEST(RealmCapWording, TheBottleneckIsNeverSaidTheSameWayAsBadLuck) {
    for (PanelStage stage : {PanelStage::Mortal, PanelStage::Immortal}) {
        const auto& words = fanren::game::cultivationLexicon(stage);
        const std::string reason = words.pushAtStoryCap;
        const std::string feedback = words.atStoryCap;
        ASSERT_FALSE(reason.empty());
        ASSERT_FALSE(feedback.empty());
        EXPECT_NE(feedback, words.notReady) << "瓶颈说成了火候未到";
        EXPECT_EQ(feedback.find(words.breakFailPrefix), std::string::npos) << "瓶颈说成了没冲过去";
        EXPECT_NE(reason, words.pushCapped) << "剧情上限说成了本作上限";
        EXPECT_EQ(reason.find(words.pushShortPrefix), std::string::npos)
            << "瓶颈说成了「尚差几点」";
    }
}

// 这两句是 C++ 字面量，不在 data/text/chNN*.json 里，LexiconTests 扫不到它们。
// 等效检查：两个阶段的这两句里，LexiconTests 那张首见章号表上的词一个都不许有
// ——比按章判更严（凡人用词第 1 章就上屏，第 1 章覆盖到原著 ch9，表上每个词都晚于它）。
// 凡人阶段另有 tests/PanelTests.cpp 的禁词扫描（cultivationPanelStrings 已把这两句列进去）。
TEST(RealmCapWording, NeitherLineSaysAWordTheLexiconTableHasNotReleased) {
    // 与 tests/LexiconTests.cpp 的 terms() 同一张表。那边加词时这里跟着加。
    const std::vector<std::string> kTerms{"长春功", "筑基", "惊蛟会", "灵石",
                                          "元婴",   "结丹", "升仙令", "银月"};
    const auto firstHit = [&kTerms](const std::string& text) {
        for (const std::string& word : kTerms) {
            if (text.find(word) != std::string::npos) return word;
        }
        return std::string{};
    };
    // 先验扫描器有牙。
    ASSERT_EQ(firstHit("卡在筑基的门槛上"), "筑基");
    ASSERT_TRUE(firstHit("卡在这一层了").empty());

    for (PanelStage stage : {PanelStage::Mortal, PanelStage::Immortal}) {
        const auto& words = fanren::game::cultivationLexicon(stage);
        for (const std::string text : {std::string(words.pushAtStoryCap), std::string(words.atStoryCap)}) {
            EXPECT_TRUE(firstHit(text).empty()) << "上限提示里出现了「" << firstHit(text) << "」：" << text;
        }
    }
}

// ===========================================================================
// 三、第 3 章：上限内按得动，到上限按不动
// ===========================================================================

class RealmCapPanel : public ::testing::Test {
protected:
    void SetUp() override {
        const fs::path root = findProjectRoot();
        ASSERT_FALSE(root.empty());
        auto ready = app_.init(root.string(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
    }
    void TearDown() override { app_.shutdown(); }
    Application app_;
};

TEST_F(RealmCapPanel, ChapterThreeCanPressUpToTheThirdLayerAndNoFurther) {
    GameState& s = app_.state();
    s = GameState{};
    // 第 3 章的上限：第 2 章 ceng3.lua 抬到三层，第 3 章一处不抬（ch04-design 1.2）。
    s.realmCap = Realm::QiRefining3;
    s.realm = Realm::QiRefining2;   // 第 2 章修为不够、还停在第二层的那个玩家
    s.aptitude = 100;
    s.cultivation = 1000;

    const BreakthroughAttempt up = pressUntilItWorks(app_);
    ASSERT_TRUE(up.success) << "上限之内却按不上去（blocked=" << static_cast<int>(up.blocked) << "）";
    ASSERT_EQ(s.realm, Realm::QiRefining3);

    // 到了三层：修为再多也按不动，一分不扣，说的是瓶颈。
    s.cultivation = 100000;
    const auto& words = fanren::game::cultivationLexicon(fanren::game::wordingStage(s));
    const auto items = CultivationScene::buildMainItems(s);
    ASSERT_EQ(items.size(), 3u);
    EXPECT_FALSE(items[1].enabled);
    EXPECT_EQ(items[1].disabledReason, words.pushAtStoryCap);

    CultivationScene panel;
    const BreakthroughAttempt stuck = panel.breakthrough(app_);
    EXPECT_EQ(stuck.blocked, BreakthroughBlock::StoryCap);
    EXPECT_EQ(s.realm, Realm::QiRefining3) << "第 3 章按出了第四层：章末境界又不确定了";
    EXPECT_GE(s.cultivation, 100000) << "按不下去不该扣修为";
    EXPECT_EQ(panel.feedback(), words.atStoryCap);
}

// ===========================================================================
// 四、剧情抬上限：真脚本、真的 api.lua
// ===========================================================================
//
// 夹具资源根：data/ 与整棵 scripts/ 原样复制一份（夹具脚本要与 api.lua 同根才加载得到，
// 而正式的 scripts/ 不该为了测试塞假脚本）。跑的是**真的**第 1、2 章脚本与真的 api.lua。

fs::path& scriptRoot() {
    static fs::path path;
    return path;
}

class RealmCapScript : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        const fs::path source = findProjectRoot();
        ASSERT_FALSE(source.empty());
        fs::path& root = scriptRoot();
        root = fanren::test::uniqueTempPath("fanren_realm_cap_root");
        std::error_code ec;
        fs::create_directories(root, ec);
        fs::copy(source / "data", root / "data", fs::copy_options::recursive, ec);
        ASSERT_FALSE(ec) << "复制 data/ 失败: " << ec.message();
        fs::copy(source / "scripts", root / "scripts", fs::copy_options::recursive, ec);
        ASSERT_FALSE(ec) << "复制 scripts/ 失败: " << ec.message();
        fs::create_directories(root / "scripts" / "t", ec);
    }
    static void TearDownTestSuite() {
        std::error_code ec;
        fs::remove_all(scriptRoot(), ec);
    }

    void SetUp() override {
        auto ready = app_.init(scriptRoot().string(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
    }
    void TearDown() override { app_.shutdown(); }

    GameState& state() { return app_.state(); }

    // 像主循环那样把脚本推到结束，替玩家按确认 / 选第 choiceIndex 项。
    void runScript(const std::string& path, int choiceIndex = 0) {
        const auto started = app_.startEvent(path);
        ASSERT_TRUE(started.ok) << path << ": " << started.error;
        for (int frame = 0; frame < 4000 && app_.scripts().isRunning(); ++frame) {
            app_.tick(1.0 / 60.0);
            if (!app_.awaitingCommand()) continue;
            fanren::script::CommandResult result;
            result.ok = true;
            result.choiceIndex = choiceIndex;
            app_.completeCommand(result);
            app_.popScene();
        }
        ASSERT_FALSE(app_.scripts().isRunning()) << path << " 没能跑到结束";
        app_.tick(1.0 / 60.0);
    }

    void runInline(const std::string& name, const std::string& lua) {
        const fs::path path = scriptRoot() / "scripts" / "t" / name;
        {
            std::ofstream out(path, std::ios::binary | std::ios::trunc);
            ASSERT_TRUE(out.good());
            out << lua;
        }
        runScript("t/" + name);
    }

    Application app_;
};

TEST_F(RealmCapScript, ANewGameStartsAsAMortal) {
    // 缺省值就是新开局：main 不读档时用的正是 init 之后这份状态。
    EXPECT_EQ(state().realmCap, Realm::Mortal) << "新开局还没拿到口诀，一层也不该按得上去";
    EXPECT_EQ(GameState{}.realmCap, Realm::Mortal);
}

TEST_F(RealmCapScript, ReceivingTheMantraOpensTheFirstLayerOnBothBranches) {
    for (int branch : {0, 1}) {
        GameState& s = state();
        s = GameState{};
        s.setFlag("ch01.dazuo_done");   // 授口诀那一节的前置：第一次打坐已经坐过
        ASSERT_EQ(s.realmCap, Realm::Mortal) << "先验";
        runScript("ch01/shenshougu_koujue.lua", branch);
        ASSERT_EQ(s.flag("ch01.koujue_received"), 1) << "先验：这一节真的演完了（分支 " << branch << "）";
        EXPECT_EQ(s.realmCap, Realm::QiRefining1) << "授完口诀，上限该到第一层（分支 " << branch << "）";
    }
}

TEST_F(RealmCapScript, TheSecondAndThirdLayerScenesRaiseTheCapAndThePanelCanThenPress) {
    GameState& s = state();
    s = GameState{};
    s.realmCap = Realm::QiRefining1;     // 第 1 章授口诀之后
    s.realm = Realm::QiRefining1;
    s.aptitude = 100;
    s.cultivation = 5000;

    // 段四之前：第一层就是顶。
    ASSERT_EQ(breakthroughBlock(s.realm, s.cultivation, s.realmCap), BreakthroughBlock::StoryCap);

    s.setFlag("ch02.duan4_start");
    runScript("ch02/ceng2.lua");
    ASSERT_EQ(s.flag("ch02.koujue_ceng"), 2) << "先验：段四那一节真的演完了";
    EXPECT_EQ(s.realmCap, Realm::QiRefining2) << "剧情说到第二层，上限该到第二层";
    EXPECT_TRUE(pressUntilItWorks(app_).success) << "上限抬起来之后该按得动";
    EXPECT_EQ(s.realm, Realm::QiRefining2);
    EXPECT_EQ(breakthroughBlock(s.realm, s.cultivation, s.realmCap), BreakthroughBlock::StoryCap)
        << "段五之前第二层就是顶";

    s.setFlag("ch02.duan5_start");
    runScript("ch02/ceng3.lua");
    ASSERT_EQ(s.flag("ch02.koujue_ceng"), 3) << "先验：段五那一节真的演完了";
    EXPECT_EQ(s.realmCap, Realm::QiRefining3) << "四年苦修的终点：上限该到第三层";
    EXPECT_TRUE(pressUntilItWorks(app_).success);
    EXPECT_EQ(s.realm, Realm::QiRefining3);
    EXPECT_EQ(breakthroughBlock(s.realm, s.cultivation, s.realmCap), BreakthroughBlock::StoryCap)
        << "第 2 章章末第三层就是顶，第 3 章一处不抬";
}

// realm.cap 的语义：至少到这一层、只升不降、非法编号回绝。走真的 api.lua 糖衣。
constexpr const char* kCapScript = R"lua(
local ok, code = realm.cap(flag.get("test_target"))
flag.set("cap_ok", ok and 1 or 0)
flag.set("cap_code_no_realm", code == "no_realm" and 1 or 0)
)lua";

TEST_F(RealmCapScript, RealmCapRaisesButNeverLowers) {
    GameState& s = state();
    s.realmCap = Realm::QiRefining5;

    s.setFlag("test_target", 3);
    runInline("realm_cap.lua", kCapScript);
    EXPECT_EQ(s.flag("cap_ok"), 1) << "已经不低于：结果成立，照样 ok（读档重跑同一段不算错）";
    EXPECT_EQ(s.realmCap, Realm::QiRefining5) << "上限被往下压了";

    s.setFlag("test_target", 7);
    runInline("realm_cap.lua", kCapScript);
    EXPECT_EQ(s.flag("cap_ok"), 1);
    EXPECT_EQ(s.realmCap, Realm::QiRefining7);
    EXPECT_EQ(s.realm, Realm::Mortal) << "realm.cap 只动上限，不替他把境界推上去";

    s.setFlag("test_target", 15);   // 炼气与筑基之间的空段，不是一个境界
    runInline("realm_cap.lua", kCapScript);
    EXPECT_EQ(s.flag("cap_ok"), 0);
    EXPECT_EQ(s.flag("cap_code_no_realm"), 1);
    EXPECT_EQ(s.realmCap, Realm::QiRefining7);
}

// realm.advance 顺带把上限抬到目标层：第 4 章那三次升境不必另写一句 realm.cap。
constexpr const char* kAdvanceScript = R"lua(
local ok = realm.advance(flag.get("test_target"))
flag.set("adv_ok", ok and 1 or 0)
)lua";

TEST_F(RealmCapScript, AStoryAdvanceCarriesTheCapUpWithIt) {
    GameState& s = state();
    s.realm = Realm::QiRefining3;
    s.realmCap = Realm::QiRefining3;   // 第 3 章交过来的那一份
    s.setFlag("test_target", 5);       // 第 4 章节点 1
    runInline("realm_advance_cap.lua", kAdvanceScript);
    ASSERT_EQ(s.flag("adv_ok"), 1);
    ASSERT_EQ(s.realm, Realm::QiRefining5);
    EXPECT_EQ(s.realmCap, Realm::QiRefining5) << "剧情升了境，上限却还停在三层";

    // 升境本身不受上限约束：上限在后面，剧情在前面。
    s.realmCap = Realm::QiRefining3;
    s.setFlag("test_target", 8);
    runInline("realm_advance_cap.lua", kAdvanceScript);
    EXPECT_EQ(s.flag("adv_ok"), 1);
    EXPECT_EQ(s.realm, Realm::QiRefining8);
    EXPECT_EQ(s.realmCap, Realm::QiRefining8);
}

// ===========================================================================
// 五、存档：往返与老档迁移
// ===========================================================================

TEST(RealmCapSave, TheCapSurvivesASaveRoundTrip) {
    GameState s;
    s.mapId = "ch04_getang";
    s.realm = Realm::QiRefining5;
    s.realmCap = Realm::QiRefining7;   // 与境界不同，才看得出存的是上限不是境界
    const fs::path path = fanren::test::uniqueTempPath("fanren_realm_cap_save", ".json");
    ASSERT_TRUE(fanren::io::saveGame(s, path.string()).ok);
    const auto back = fanren::io::loadGame(path.string());
    ASSERT_TRUE(back.ok) << back.error;
    EXPECT_EQ(back.value.realmCap, Realm::QiRefining7);
    std::error_code ec;
    fs::remove(path, ec);
}

// 迁移口径（协调者 2026-09-23 裁决）：上限 = max(当前境界, 由已有剧情旗标推出的上限)。
TEST(RealmCapSave, LegacyCapTakesTheStoryFlagsWhenTheyAreAhead) {
    // 第 2 章段五之后、玩家还没按到三层的老档：剧情早已说过他到了第三层。
    const std::map<std::string, int> flags{{"ch01.koujue_received", 1}, {"ch02.koujue_ceng", 3}};
    EXPECT_EQ(fanren::io::legacyRealmCap(Realm::QiRefining2, flags), Realm::QiRefining3)
        << "只取当前境界的话，这份档会被封在二层，一直卡到第 4 章节点 1";
    // 只授了口诀、还没入门的档：上限到第一层。
    EXPECT_EQ(fanren::io::legacyRealmCap(Realm::Mortal, {{"ch01.koujue_received", 1}}),
              Realm::QiRefining1);
}

TEST(RealmCapSave, LegacyCapTakesTheRealmWhenItIsAhead) {
    // 第 4 章节点 1 之后的老档：脚本直接把境界推到五层，旗标只说到三层。
    const std::map<std::string, int> flags{{"ch01.koujue_received", 1}, {"ch02.koujue_ceng", 3}};
    EXPECT_EQ(fanren::io::legacyRealmCap(Realm::QiRefining5, flags), Realm::QiRefining5);
}

TEST(RealmCapSave, LegacyCapOfAFreshSaveIsMortal) {
    EXPECT_EQ(fanren::io::legacyRealmCap(Realm::Mortal, {}), Realm::Mortal);
    // 无关的旗标不抬上限。
    EXPECT_EQ(fanren::io::legacyRealmCap(Realm::Mortal, {{"ch01.sanshu_met", 1}}), Realm::Mortal);
}

// 端到端：一份手拼的 v5 档经真的 loadGame 读进来，走 5→6 迁移。
// payload 必须写成 nlohmann::json::dump() 的紧凑形、键按字母序（校验和算的就是它），
// 做法与 tests/Ch03PartyTests.cpp 的 canonicalPayload 同一个。
std::uint64_t fnv1a64(const std::string& data) {
    std::uint64_t hash = 0xcbf29ce484222325ULL;
    for (const char c : data) {
        hash ^= static_cast<unsigned char>(c);
        hash *= 0x100000001b3ULL;
    }
    return hash;
}

std::string saveText(int version, const std::string& payload) {
    std::ostringstream hex;
    hex << std::hex << std::setw(16) << std::setfill('0')
        << fnv1a64(std::to_string(version) + "|" + payload);
    return R"({"save_version":)" + std::to_string(version) + R"(,"checksum":")" + hex.str() +
           R"(","payload":)" + payload + "}";
}

// realmCap < 0 表示根本不写这个字段（v5 及更早的档就是这样）。
std::string handPayload(int realm, const std::string& flagsJson, int realmCap = -1) {
    std::string p = "{";
    p += R"("bag":[],)";
    p += R"("chapter":2,)";
    p += R"("cultivation":12,)";
    p += R"("day":900,)";
    p += R"("facing":2,)";
    p += R"("flags":)" + flagsJson + ",";
    p += R"("hp":48,)";
    p += R"("mapId":"ch02_jusuo",)";
    p += R"("maxHp":48,)";
    p += R"("maxMp":20,)";
    p += R"("mp":20,)";
    p += R"("playSecondsGameplay":0.0,)";
    p += R"("playSecondsSystem":0.0,)";
    p += R"("position":{"x":3,"y":4},)";
    p += R"("realm":)" + std::to_string(realm);
    // 键按字母序："realm" 是 "realmCap" 的前缀，排在它前面。
    if (realmCap >= 0) p += R"(,"realmCap":)" + std::to_string(realmCap);
    p += "}";
    return p;
}

TEST(RealmCapSave, AnOldChapterTwoSaveMigratesToTheLayerTheStoryAlreadyReached) {
    const fs::path path = fanren::test::uniqueTempPath("fanren_realm_cap_v5", ".json");
    {
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        out << saveText(5, handPayload(2, R"({"ch01.koujue_received":1,"ch02.koujue_ceng":3})"));
    }
    const auto loaded = fanren::io::loadGame(path.string());
    ASSERT_TRUE(loaded.ok) << loaded.error;
    EXPECT_EQ(loaded.value.realm, Realm::QiRefining2) << "先验：迁移不动境界";
    EXPECT_EQ(loaded.value.realmCap, Realm::QiRefining3)
        << "段五之后的 v5 老档：上限该取剧情已经说到的第三层";
    std::error_code ec;
    fs::remove(path, ec);
}

// 当前版本的档却缺了 realmCap（手拼的档、夹具）：fromJson 照老档的口径推一遍
//（io/SaveFile.cpp 里 `if (p.contains("realmCap"))` 的 else 那一支）。数字写死：
// 一层的境界 ＋ 剧情已经说到第三层 → 三层。只用境界推的话是一层，给凡人的话更低。
TEST(RealmCapSave, ACurrentVersionSaveWithoutTheFieldIsDerivedFromItsStoryFlags) {
    const fs::path path = fanren::test::uniqueTempPath("fanren_realm_cap_nofield", ".json");
    {
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        out << saveText(fanren::io::kSaveVersion, handPayload(1, R"({"ch02.koujue_ceng":3})"));
    }
    const auto loaded = fanren::io::loadGame(path.string());
    ASSERT_TRUE(loaded.ok) << loaded.error;
    EXPECT_EQ(loaded.value.realm, Realm::QiRefining1) << "先验：境界照原样读进来";
    EXPECT_EQ(loaded.value.realmCap, Realm::QiRefining3)
        << "缺字段的当前版本档，上限该按剧情旗标推出第三层";
    std::error_code ec;
    fs::remove(path, ec);
}

TEST(RealmCapSave, ACorruptCapIsRefusedNotSilentlyAccepted) {
    // 当前版本的档里写了一个不存在的编号：与 realm 同一条口径，判失败。
    const fs::path path = fanren::test::uniqueTempPath("fanren_realm_cap_bad", ".json");
    {
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        out << saveText(fanren::io::kSaveVersion, handPayload(3, "{}", /*realmCap=*/15));
    }
    const auto loaded = fanren::io::loadGame(path.string());
    EXPECT_FALSE(loaded.ok) << "上限编号 15 不是任何境界，却被读进来了";
    EXPECT_NE(loaded.error.find("realmCap"), std::string::npos) << loaded.error;
    std::error_code ec;
    fs::remove(path, ec);
}

}  // namespace
