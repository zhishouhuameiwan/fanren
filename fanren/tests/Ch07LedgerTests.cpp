// 第 7 章的经济账（docs/ch07-design.md 第 9 节、验收 10、11）——直接读 scripts/ch07/*.lua、data/battles/b07_*.json、
// data/items/**，判据是第 9 节那几张表与六条的原文数字；外加切片跑单个脚本，看「不够就 return、不置旗标」
// 与「有则扣」在引擎里真的是那样。
//
// ---------------------------------------------------------------------------
// 死局退路：不信自报，用切片证明（测试路派工单）
// ---------------------------------------------------------------------------
// 内容路自报「每一处扣钱扣物都有兜底」。这里把玩家**合法地**能落到的最穷的局面摆出来，起那一节的脚本，看它
// 还走不走得到下一节点（完成旗标置上）：
//   · 灵石：节点 4b 之前没有商店、没有仗（第一仗在节点 13），灵石只进不出——最穷就是第 6 章一块没剩、只有两次交药的
//     24 ＋ 60 = 84；藏室 22、银丝鼎 32 在 84 上必成（第 9 节第 1 条的「最低 166」是按交接存档第二侧 82 算的，这里连
//     那 82 也拿掉）。坊市之后玩家能把钱在收药摊花光、把中阶灵石在仗里当药吃光（restoreMp）——35a 那两笔一律有则扣
//    （施工偏差 18.7）：一块灵石也没有、一块中阶也没有，也得开得了石门；
//   · 清灵散（14、26c）、定神符（13）：有则扣——身上没有，换一句照样往下（13 走真仗，陆师兄那一场）；
//   · 药篓、园角、万宝楼、地火屋盘点：条件不够就一句话 return、一件不扣、不置旗标，补够了再来就成（反面也跑一遍，
//     证明上面不是因为脚本根本不做）。
// 形状与通关测试不同：不走位，起点是手摆的最小局面、旁边写明摆了什么。
#include <gtest/gtest.h>

#include <algorithm>
#include <deque>
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

#include "BattleHand.h"
#include "ChapterFixture.h"
#include "core/battle/Battle.h"
#include "core/rules/Economy.h"
#include "core/rules/Realm.h"
#include "game/Application.h"
#include "game/BattleScene.h"
#include "game/ShopScene.h"
#include "io/SaveFile.h"
#include "script/Command.h"

namespace {

namespace fs = std::filesystem;

using fanren::core::GameState;
using fanren::game::Application;
using fanren::game::BattleScene;
using fanren::rules::Realm;

constexpr const char* kLingshi = "material_lingshi";
constexpr const char* kZhong = "material_lingshi_zhong";
constexpr const char* kZhuji = "pill_zhuji_dan";
constexpr const char* kYaofen = "material_zhuji_yaofen";
constexpr const char* kSniffed[] = {"herb_yusui_zhi", "herb_zihou_hua", "herb_tianling_guo"};

// 施工图第 9 节来源 / 去向表。
constexpr int kPayYearOne = 24;
constexpr int kPayYearTwo = 60;
constexpr int kCangshiTakes[] = {1, 1, 20};   // 4b：两个时辰 ＋ 复制两份
constexpr int kDingPrice = 32;                // 5a
constexpr int kXuLaoTotal = 54;               // 第 9 节第 1 条：许老与银丝鼎的 54
constexpr int kLowestAtCangshi = 166;         // 第 9 节第 1 条：82 + 24 + 60
constexpr int kChapterLingshiDelta = 349;     // 第 9 节第 6 条
constexpr int kChapterZhongDelta = 17;        // 第 9 节第 6 条：中阶 ≈ 17 − k
constexpr int kZhujiAtEnd = 17;               // 第 9 节第 2 条
constexpr int kYaofenGiven = 40;              // 3.2 节点 34
constexpr int kYaofenRefill = 10;             // 3.2 节点 35c：药粉用尽再给 10 份
constexpr int kMarketCap = 1300;              // 第 9 节来源表末行：收药摊那一路封顶约一千三

std::string assetRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        const fs::path root(candidate);
        if (fs::exists(root / "scripts" / "ch07" / "chukou.lua") &&
            fs::exists(root / "data" / "shops" / "ch07_fangshi_yaotan.json")) {
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

// 一次物品进出：哪个脚本、第几行、give / take / take_aged、物品、数量（字面量；写成变量或算式的记 -1）、原样的数量表达式。
struct Flow {
    std::string script;
    int line = 0;
    std::string verb;   // give / take / take_aged
    std::string item;
    int count = 0;
    std::string countText;
    std::string ageText;   // give 的第三个参数、take_aged 的年份下限（原样）
};

std::vector<Flow> flowsIn(const std::string& script, const std::string& source) {
    std::vector<Flow> out;
    static const std::regex kCall("\\b(give|take_aged|take)\\(\"([a-z0-9_]+)\"(?:,\\s*([^,)]+))?(?:,\\s*([^,)]+))?\\)");
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
            f.verb = (*it)[1];
            f.item = (*it)[2];
            f.countText = (*it)[3].matched ? std::string((*it)[3]) : std::string("1");
            while (!f.countText.empty() && f.countText.back() == ' ') f.countText.pop_back();
            f.count = std::regex_match(f.countText, std::regex("[0-9]+")) ? std::stoi(f.countText) : -1;
            f.ageText = (*it)[4].matched ? std::string((*it)[4]) : std::string();
            out.push_back(f);
        }
    }
    return out;
}

class Ch07Ledger : public ::testing::Test {
protected:
    void SetUp() override {
        root_ = assetRoot();
        auto ready = app_.init(root_, /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
        for (const auto& entry : fs::directory_iterator(fs::path(root_) / "scripts" / "ch07")) {
            if (entry.path().extension() != ".lua") continue;
            ++scripts_;
            const auto flows = flowsIn(entry.path().filename().string(), readFile(entry.path()));
            flows_.insert(flows_.end(), flows.begin(), flows.end());
        }
        ASSERT_GE(scripts_, 46) << "先验：scripts/ch07/ 读得到";
        ASSERT_GE(flows_.size(), 60u) << "先验：本章的 give / take 读得到（分母不能塌）";
    }
    void TearDown() override { app_.shutdown(); }

    std::string source(const std::string& file) { return readFile(fs::path(root_) / "scripts" / "ch07" / file); }
    GameState& state() { return app_.state(); }

    // 切片：起一个脚本，对白一路按确认、选择按 answer，跑到结束。仗交给意图玩家那只手打。返回说过的文案 key。
    std::vector<std::string> runScript(const std::string& path, int answer = 0) {
        std::vector<std::string> said;
        const auto started = app_.startEvent(path);
        EXPECT_TRUE(started.ok) << path << "：" << started.error;
        if (!started.ok) return said;
        app_.clearSpokenKeys();
        for (int frame = 0; frame < 8000 && app_.scripts().isRunning(); ++frame) {
            app_.tick(1.0 / 60.0);
            if (auto* fight = dynamic_cast<BattleScene*>(app_.topScene()); fight != nullptr) {
                if (fight->battle().phase() == fanren::core::battle::BattlePhase::Ongoing) {
                    fanren::test::HandPolicy p;
                    p.healAtPercent = 50;
                    p.healWhenDoomed = false;
                    p.pills = {"pill_yangjing_dan", "pill_jinchuang_yao"};
                    fanren::test::BattleHand hand(app_, p);
                    hand.play(*fight);
                }
                continue;
            }
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

    static bool has(const std::vector<std::string>& said, const std::string& key) {
        return std::find(said.begin(), said.end(), key) != said.end();
    }

    void setCount(const std::string& item, int count, int age = 0) {
        while (state().removeItem(item, 1)) {
        }
        if (count > 0) state().addItem(item, count, age);
    }

    // 一个修仙阶段、十一层、满血满法力的空局面（节点 3 之后的他）。
    void freshEleventh() {
        state() = GameState{};
        state().setFlag("story.xiuxian_known");
        state().realm = state().realmCap = Realm::QiRefining11;
        state().hp = state().maxHp = fanren::rules::realmMaxHp(Realm::QiRefining11);
        state().mp = state().maxMp = fanren::rules::realmMaxMp(Realm::QiRefining11);
    }

    Application app_;
    std::string root_;
    int scripts_ = 0;
    std::vector<Flow> flows_;
};

// ---------------------------------------------------------------------------
// 第 9 节第 1 条：节点 4b 之前灵石只进不出（两次交药 24、60）；藏室 1 + 1 + 20、银丝鼎 32，合计 54；
// 交接存档两侧 + 84 都不少于 166
// ---------------------------------------------------------------------------
TEST_F(Ch07Ledger, No1_StonesOnlyComeInBeforeTheLibraryAndBothSidesAffordXuLao) {
    std::multiset<std::pair<std::string, int>> lingshi;
    for (const Flow& f : flows_) {
        if (f.item == kLingshi) lingshi.insert({f.verb + " " + f.script, f.count});
    }
    // 4b 之前的挂点（3.1 表：1、2、3、Z1、4a）一处也不扣灵石；两次交药给 24、60。
    for (const char* early : {"chaibao.lua", "yaolou.lua", "yuelu_ru.lua"}) {
        EXPECT_EQ(lingshi.count({std::string("take ") + early, 1}) + lingshi.count({std::string("take ") + early, -1}), 0u)
            << early << " 在节点 4b 之前扣了灵石";
    }
    EXPECT_EQ(lingshi.count({"give yaolou.lua", kPayYearOne}), 1u) << "3.2 节点 2：give(\"material_lingshi\", 24)";
    EXPECT_EQ(lingshi.count({"give yaolou.lua", kPayYearTwo}), 1u) << "3.2 节点 3：give(\"material_lingshi\", 60)";
    std::multiset<int> cangshi;
    for (const Flow& f : flows_) {
        if (f.item == kLingshi && f.verb == "take" && f.script == "cangshi.lua") cangshi.insert(f.count);
    }
    EXPECT_EQ(cangshi, std::multiset<int>(std::begin(kCangshiTakes), std::end(kCangshiTakes))) << "3.2 节点 4b：1、1、20";
    EXPECT_EQ(lingshi.count({"take dihuo_wen.lua", kDingPrice}), 1u) << "3.2 节点 5a：银丝鼎 32";
    EXPECT_EQ(kCangshiTakes[0] + kCangshiTakes[1] + kCangshiTakes[2] + kDingPrice, kXuLaoTotal) << "第 9 节第 1 条：54";
    // 两侧交接存档 + 84 ≥ 166（第 9 节第 1 条的「最低也有」；交接存档第二侧 82 是它的来处）。
    for (const char* file : {fanren::test::kChapterSixEndingFirst, fanren::test::kChapterSixEndingSecond}) {
        auto loaded = fanren::io::loadGame(fanren::test::chapterFixturePath(root_, file).string());
        ASSERT_TRUE(loaded.ok) << file << "：" << loaded.error;
        EXPECT_GE(loaded.value.itemCount(kLingshi) + kPayYearOne + kPayYearTwo, kLowestAtCangshi)
            << file << "：第 9 节第 1 条「两次交药之后最低 82 + 24 + 60 = 166」";
    }
}

// 切片（死局退路）：第 6 章一块灵石也没剩的那一种（比交接存档还穷），两次交药之后只有 84——藏室、银丝鼎都在它上面成；
// 21 块：藏室先数够再动手，一块也不扣、不置旗标（「看返回值；不够就 return」）；补到 22 再来就成。
TEST_F(Ch07Ledger, DeadEnds_TheLibraryAndTheCauldronGoThroughOnTheLeanestPurse) {
    freshEleventh();
    state().setFlag("ch07.yuelu");
    setCount(kLingshi, kPayYearOne + kPayYearTwo);
    runScript("ch07/cangshi.lua");
    EXPECT_EQ(state().flag("ch07.cangshi"), 1) << "84 块：藏室该成";
    runScript("ch07/dihuo_wen.lua");
    EXPECT_EQ(state().flag("ch07.yinsi"), 1) << "84 − 22 = 62 块：银丝鼎该买得起";
    EXPECT_EQ(state().itemCount(kLingshi), kPayYearOne + kPayYearTwo - kXuLaoTotal);
    EXPECT_EQ(state().itemCount("story_yinsi_ding"), 1);
    // 反面：21 块（不可能的局面，只为证明脚本真的看返回值）。
    freshEleventh();
    state().setFlag("ch07.yuelu");
    setCount(kLingshi, 21);
    const auto poor = runScript("ch07/cangshi.lua");
    EXPECT_EQ(state().flag("ch07.cangshi"), 0) << "不够就 return，不置旗标";
    EXPECT_EQ(state().itemCount(kLingshi), 21) << "先数够再动手：一块也不扣";
    EXPECT_TRUE(has(poor, "ch07.cangshi.poor"));
    setCount(kLingshi, 22);
    runScript("ch07/cangshi.lua");
    EXPECT_EQ(state().flag("ch07.cangshi"), 1) << "补够了再来就成";
    EXPECT_EQ(state().itemCount(kLingshi), 0);
    // 银丝鼎：31 块买不起，一块不扣、不给鼎、不置旗标。
    setCount(kLingshi, kDingPrice - 1);
    runScript("ch07/dihuo_wen.lua");
    EXPECT_EQ(state().flag("ch07.yinsi"), 0);
    EXPECT_EQ(state().itemCount(kLingshi), kDingPrice - 1);
    EXPECT_EQ(state().itemCount("story_yinsi_ding"), 0);
}

// 切片（死局退路，施工偏差 18.7）：35a 传送阵一块、定金一块中阶都有则扣——坊市里把灵石花光、把中阶灵石在仗里当药吃光，
// 都是合法的。三种局面：两样都有 / 只剩低阶（一百块抵一块中阶）/ 两样都没有（丑汉许他先欠着）。每一种都开得了石门。
TEST_F(Ch07Ledger, DeadEnds_TheFireRoomOpensWhateverIsLeftInThePurse) {
    struct Purse {
        int lingshi, zhong;
        int lingshiAfter, zhongAfter;
        const char* depositKey;
    };
    const std::vector<Purse> kPurses = {
        {300, 15, 299, 14, "ch07.chouhan.deposit"},
        {300, 0, 199, 0, "ch07.chouhan.deposit_low"},
        {0, 0, 0, 0, "ch07.chouhan.deposit_owe"},
        {50, 0, 49, 0, "ch07.chouhan.deposit_owe"},
    };
    for (const Purse& p : kPurses) {
        freshEleventh();
        state().setFlag("ch07.sannian");
        setCount(kLingshi, p.lingshi);
        setCount(kZhong, p.zhong);
        const auto said = runScript("ch07/chouhan.lua");
        EXPECT_EQ(state().flag("ch07.dihuo"), 1) << "灵石 " << p.lingshi << "、中阶 " << p.zhong << "：石门也得开";
        EXPECT_EQ(state().itemCount(kLingshi), p.lingshiAfter) << "灵石 " << p.lingshi << "、中阶 " << p.zhong;
        EXPECT_EQ(state().itemCount(kZhong), p.zhongAfter) << "灵石 " << p.lingshi << "、中阶 " << p.zhong;
        EXPECT_TRUE(has(said, p.depositKey)) << "灵石 " << p.lingshi << "、中阶 " << p.zhong << "：定金那一句该是 " << p.depositKey;
        EXPECT_EQ(has(said, "ch07.chouhan.fee"), p.lingshi > 0) << "传送阵那一块有则扣";
        EXPECT_EQ(has(said, "ch07.chouhan.nofee"), p.lingshi == 0);
    }
}

// 切片（死局退路）：清灵散（14、26c）有则扣；13 的定神符有则扣——陆师兄那一场走真仗。
TEST_F(Ch07Ledger, DeadEnds_TheOptionalTakesStepAsideWhenTheBagIsEmpty) {
    for (const int qingling : {0, 1}) {
        freshEleventh();
        state().setFlag("ch07.yeyu");
        setCount("pill_qingling_san", qingling);
        const auto said = runScript("ch07/fanhui.lua");
        EXPECT_EQ(state().flag("ch07.fanhui"), 1) << "清灵散 " << qingling << "：节点 14 照样走完";
        EXPECT_EQ(state().itemCount("pill_qingling_san"), 0);
        EXPECT_EQ(has(said, "ch07.fanhui.qingling"), qingling > 0);
        EXPECT_EQ(has(said, "ch07.fanhui.noqingling"), qingling == 0);
        freshEleventh();
        state().setFlag("ch07.mairen");
        setCount("pill_qingling_san", qingling);
        const auto said2 = runScript("ch07/youdi.lua");
        EXPECT_EQ(state().flag("ch07.zihouhua"), 1) << "清灵散 " << qingling << "：节点 26c 照样走完";
        EXPECT_EQ(has(said2, "ch07.youdi.qingling"), qingling > 0);
        EXPECT_EQ(has(said2, "ch07.youdi.noqingling"), qingling == 0);
    }
    for (const int dingshen : {0, 1}) {
        freshEleventh();
        state().setFlag("ch07.likai");
        for (const char* id : {"magic_huodan_shu", "magic_liusha_shu", "magic_bingdong_shu", "magic_ji_jianfu", "magic_ji_jinfu",
                               "magic_ji_jinguangzhuan"}) {
            state().learnMagic(id);
        }
        state().addItem("talisman_jianfu", 1, 0);
        state().addItem("story_jinggang_huan", 1, 0);
        state().addItem("pill_yangjing_dan", 1, 0);
        setCount("talisman_dingshen_fu", dingshen);
        const auto said = runScript("ch07/yeyu.lua");
        ASSERT_EQ(state().flag("ch07.yeyu"), 1) << "定神符 " << dingshen << "：① 打赢了、节点 13 走完";
        EXPECT_EQ(state().itemCount("talisman_dingshen_fu"), 0);
        EXPECT_EQ(has(said, "ch07.yeyu.freeze_fu"), dingshen > 0) << "有定神符就用符";
        EXPECT_EQ(has(said, "ch07.yeyu.freeze_shu"), dingshen == 0) << "没有就改成定神术——不设死局";
    }
}

// 切片（死局退路）：药篓、园角、万宝楼条件不够就一句话 return——一件不扣、不置旗标；够了再来就成。
// 药篓按年份下限算（第 17 节第 23 条）：四十三年的黄精交不上、四十四年的交得上（44 年这一株不因为「刚种下的苗
// 年份最低」被跳过——E4 的 take_aged 从够格里挑）。
TEST_F(Ch07Ledger, DeadEnds_TheBasketTheCornerAndTheTreasureHouseWaitUntilThereIsEnough) {
    freshEleventh();
    state().setFlag("ch07.chaibao");
    state().realm = state().realmCap = Realm::QiRefining9;
    setCount(kLingshi, 0);
    state().addItem("herb_huangjing_cao", 2, 44);
    state().addItem("herb_huangjing_cao", 3, 43);
    state().addItem("herb_huangjing_cao", 1, 0);
    runScript("ch07/yaolou.lua");
    EXPECT_EQ(state().flag("ch07.jiaoyao1"), 0) << "两株够年份：还差一株，不置旗标";
    EXPECT_EQ(state().itemCount("herb_huangjing_cao"), 6) << "一株也不扣";
    EXPECT_EQ(state().itemCount(kLingshi), 0);
    state().addItem("herb_huangjing_cao", 1, 44);
    runScript("ch07/yaolou.lua");
    EXPECT_EQ(state().flag("ch07.jiaoyao1"), 1);
    EXPECT_EQ(state().itemCountAtLeastAge("herb_huangjing_cao", 44), 0) << "交的是三株四十四年的";
    EXPECT_EQ(state().itemCount("herb_huangjing_cao"), 4) << "四十三年的、零年的苗原样留着";
    EXPECT_EQ(state().itemCount(kLingshi), kPayYearOne);
    // 第二年：一株百年紫参不够，交两株；月例 60、十一层。
    state().addItem("herb_zishen_cao", 1, 100);
    state().addItem("herb_zishen_cao", 1, 99);
    runScript("ch07/yaolou.lua");
    EXPECT_EQ(state().flag("ch07.danbao"), 0) << "一株百年：还差一株";
    EXPECT_EQ(state().itemCount("herb_zishen_cao"), 2);
    state().addItem("herb_zishen_cao", 1, 100);
    runScript("ch07/yaolou.lua");
    EXPECT_EQ(state().flag("ch07.danbao"), 1);
    EXPECT_EQ(state().itemCount(kLingshi), kPayYearOne + kPayYearTwo);
    EXPECT_EQ(state().realm, Realm::QiRefining11);
    // 园角：千年黄精芝一株 → 还差着；两株 → 置旗标、**药不扣**（节点 11 才交出去）。
    freshEleventh();
    state().setFlag("ch07.lianqi");
    state().addItem("herb_huangjing_zhi", 1, 1536);
    state().addItem("herb_huangjing_zhi", 1, 999);
    runScript("ch07/yuanjiao.lua");
    EXPECT_EQ(state().flag("ch07.qiannian"), 0) << "一株过千年：还差着";
    state().addItem("herb_huangjing_zhi", 1, 1000);
    runScript("ch07/yuanjiao.lua");
    EXPECT_EQ(state().flag("ch07.qiannian"), 1);
    EXPECT_EQ(state().itemCount("herb_huangjing_zhi"), 3) << "3.2 节点 9：药不扣";
    // 万宝楼：交的是两株 ≥ 1000 年的，九百九十九年那株留下；不够两株就一句话、一件不给。
    state().setFlag("ch07.fangshi");
    runScript("ch07/wanbaolou.lua");
    EXPECT_EQ(state().flag("ch07.wanbaolou"), 1);
    EXPECT_EQ(state().itemCountAtLeastAge("herb_huangjing_zhi", 1000), 0);
    EXPECT_EQ(state().itemCount("herb_huangjing_zhi"), 1) << "九百九十九年那株留着";
    freshEleventh();
    state().setFlag("ch07.fangshi");
    state().addItem("herb_huangjing_zhi", 1, 1000);
    runScript("ch07/wanbaolou.lua");
    EXPECT_EQ(state().flag("ch07.wanbaolou"), 0) << "一株千年换不来四件";
    EXPECT_EQ(state().itemCount("herb_huangjing_zhi"), 1);
    EXPECT_EQ(state().itemCount("talisman_tianleizi"), 0);
    EXPECT_FALSE(state().knowsMagic("magic_ji_jinfu"));
}

// ---------------------------------------------------------------------------
// 第 9 节第 2 条：give 筑基丹只有三处（13 给 2、34 给 1、35c 差额补足到 25）；take 只有一处（36 扣 8）；章末恰 17
// ---------------------------------------------------------------------------
TEST_F(Ch07Ledger, No2_FoundationPillsComeFromThreePlacesAndLeaveFromOne) {
    std::multiset<std::pair<std::string, std::string>> gives;
    std::multiset<std::pair<std::string, std::string>> takes;
    for (const Flow& f : flows_) {
        if (f.item != kZhuji) continue;
        (f.verb == "give" ? gives : takes).insert({f.script, f.countText});
    }
    ASSERT_EQ(gives.size(), 3u) << "第 9 节第 2 条：give(\"pill_zhuji_dan\") 只有三处";
    EXPECT_EQ(gives.count({"yeyu.lua", "2"}), 1u) << "节点 13：2";
    EXPECT_EQ(gives.count({"sannian.lua", "1"}), 1u) << "节点 34：1";
    std::string topUp;
    for (const auto& [script, text] : gives) {
        if (script == "banian.lua") topUp = text;
    }
    EXPECT_FALSE(topUp.empty()) << "节点 35c：差额补足";
    EXPECT_FALSE(std::regex_match(topUp, std::regex("[0-9]+"))) << "35c 给的是差额，不是写死的数：" << topUp;
    EXPECT_NE(source("banian.lua").find("25 - item.count(\"pill_zhuji_dan\")"), std::string::npos)
        << "35c：差额是 25 − 现有（第 9 节第 2 条「补足到 25」）";
    EXPECT_EQ(takes, (std::multiset<std::pair<std::string, std::string>>{{"zhuji.lua", "8"}})) << "第 9 节第 2 条：只有节点 36 扣 8";
    // 编成不掉筑基丹、路径行动不给（第 9 节第 3 条的另一半）。
    for (const char* id : {"b07_lu_shixiong", "b07_yixiantian", "b07_fengyue", "b07_zhongxinqu_duoyao", "b07_zhaoze_shouyao",
                           "be07_tuishan_shou", "be07_tiebi_yuan", "be07_huoyan_shu"}) {
        const fanren::core::BattleSetup* setup = app_.battleSetup(id);
        ASSERT_NE(setup, nullptr) << id;
        EXPECT_TRUE(setup->reward.drops.empty()) << id;
    }
    for (const fanren::core::PathAction& action : app_.data().pathActions) {
        if (action.chapter != 7) continue;
        for (const auto& e : action.gives) EXPECT_NE(e.itemId, kZhuji) << action.id;
        for (const auto& e : action.rewardItems) EXPECT_NE(e.itemId, kZhuji) << action.id;
    }
}

// 切片：35c 从几种起点各跑一遍——4 颗（原有 3 ＋ 亲手 1）→ 25；10 → 25；25 → 25 不给；3 颗且药粉用尽 → 补药粉 10、
// 不置旗标；3 颗还有药粉 → 一句「一颗也没成」、不置旗标。36：25 → 17。
TEST_F(Ch07Ledger, No2_HalfAYearTopsUpToTwentyFiveAndTheSeatLeavesSeventeen) {
    for (const int have : {4, 10, 25}) {
        freshEleventh();
        state().setFlag("ch07.feidan");
        setCount(kZhuji, have);
        setCount(kYaofen, 7);
        const int day = state().day;
        runScript("ch07/banian.lua");
        EXPECT_EQ(state().flag("ch07.chengdan"), 1) << "身上 " << have << " 颗";
        EXPECT_EQ(state().itemCount(kZhuji), std::max(have, 25)) << "身上 " << have << " 颗：补足到 25（差额 ≤ 0 不给）";
        EXPECT_EQ(state().itemCount(kYaofen), 0) << "药粉余量 take 掉";
        EXPECT_EQ(state().day - day, 180) << "3.3：半年";
    }
    freshEleventh();
    state().setFlag("ch07.feidan");
    setCount(kZhuji, 3);
    setCount(kYaofen, 0);
    const auto refill = runScript("ch07/banian.lua");
    EXPECT_EQ(state().flag("ch07.chengdan"), 0) << "一颗也没成：不置旗标";
    EXPECT_EQ(state().itemCount(kYaofen), kYaofenRefill) << "3.2 节点 35c：药粉用尽就再给 10 份——不设死局";
    EXPECT_TRUE(has(refill, "ch07.banian.refill"));
    const auto notyet = runScript("ch07/banian.lua");
    EXPECT_EQ(state().flag("ch07.chengdan"), 0) << "还有药粉、一颗也没成：不置旗标，去炉边接着炼";
    EXPECT_EQ(state().itemCount(kYaofen), kYaofenRefill) << "药粉不重复给";
    EXPECT_TRUE(has(notyet, "ch07.banian.notyet"));
    // 36：25 → 17（两个选项都就地，章末恰 17）。
    for (const int answer : {0, 1}) {
        freshEleventh();
        state().realm = state().realmCap = Realm::QiRefining11;
        state().setFlag("ch07.chengdan");
        setCount(kZhuji, 25);
        runScript("ch07/zhuji.lua", answer);
        EXPECT_EQ(state().itemCount(kZhuji), kZhujiAtEnd) << "第 9 节第 2 条：章末恰 17（选项 " << answer << "）";
        EXPECT_EQ(state().flag("ch07.done"), 1);
    }
}

// ---------------------------------------------------------------------------
// 第 9 节第 3 条：五张必打编成 rewards.spirit_stones 都是 0、drops 都空；灵石的 give 只在节点 2（24）、3（60）、13（20）、
// 32（Z2 的 50）、34（300）；第 6 条：本章脚本灵石净进 349、中阶净进 17（不含 Z2）
// ---------------------------------------------------------------------------
TEST_F(Ch07Ledger, No3_And6_StonesComeOnlyFromTheScriptsTheDesignNames) {
    for (const char* id : {"b07_lu_shixiong", "b07_yixiantian", "b07_fengyue", "b07_zhongxinqu_duoyao", "b07_zhaoze_shouyao"}) {
        const fanren::core::BattleSetup* setup = app_.battleSetup(id);
        ASSERT_NE(setup, nullptr) << id;
        EXPECT_EQ(setup->reward.spiritStones, 0) << id << "：第 9 节第 3 条";
        EXPECT_TRUE(setup->reward.drops.empty()) << id << "：第 9 节第 3 条";
    }
    std::multiset<std::pair<std::string, int>> gives;
    int net = 0;
    int zhongNet = 0;
    for (const Flow& f : flows_) {
        if (f.item == kLingshi) {
            if (f.verb == "give") gives.insert({f.script, f.count});
            if (f.script == "chukou.lua") continue;   // Z2 的加赏另算（第 9 节第 6 条「不含 Z2」）
            if (f.script == "chouhan.lua" && f.count == 100) continue;   // 没有中阶时拿一百块抵的那一支（施工偏差 18.7）
            net += (f.verb == "give" ? 1 : -1) * f.count;
        }
        if (f.item == kZhong) {
            if (f.script == "chukou.lua") continue;
            zhongNet += (f.verb == "give" ? 1 : -1) * f.count;
        }
    }
    const std::multiset<std::pair<std::string, int>> design = {
        {"yaolou.lua", kPayYearOne}, {"yaolou.lua", kPayYearTwo}, {"yeyu.lua", 20}, {"chukou.lua", 50}, {"sannian.lua", 300}};
    EXPECT_EQ(gives, design) << "第 9 节第 3 条：灵石的 give 只在节点 2、3、13、32（Z2）、34";
    EXPECT_EQ(net, kChapterLingshiDelta) << "第 9 节第 6 条：章末灵石 ≈ 起点 + 349";
    EXPECT_EQ(zhongNet, kChapterZhongDelta) << "第 9 节第 6 条：中阶 ≈ 17 − k";
    // 路径行动不给灵石、不给中阶（本章那一条求购是花钱的）。
    for (const fanren::core::PathAction& action : app_.data().pathActions) {
        if (action.chapter != 7) continue;
        for (const auto& e : action.gives) {
            EXPECT_NE(e.itemId, kLingshi) << action.id;
            EXPECT_NE(e.itemId, kZhong) << action.id;
        }
    }
    // 切片：Z2 两处都去过才加赏（第 6 节：灵石 50 ＋ 中阶 1）。
    for (const int both : {0, 1}) {
        freshEleventh();
        state().setFlag("ch07.xiashan");
        state().setFlag("ch07.yujian_a");
        if (both) state().setFlag("ch07.yujian_b");
        runScript("ch07/chukou.lua");
        EXPECT_EQ(state().itemCount(kLingshi), both ? 50 : 0) << "Z2 " << (both ? "两处都去过" : "只去了一处");
        EXPECT_EQ(state().itemCount(kZhong), both ? 1 : 0);
    }
}

// ---------------------------------------------------------------------------
// 第 9 节第 4 条：千年药、禁地药、筑基丹、药粉全部 tradeable=false；收药摊收不了它们
// ---------------------------------------------------------------------------
TEST_F(Ch07Ledger, No4_TheThousandYearHerbTheForbiddenHerbsThePillAndThePowderCannotBeSold) {
    const fanren::rules::Shop* shop = app_.shop("ch07_fangshi_yaotan");
    ASSERT_NE(shop, nullptr) << "第 7 节：坊市收药摊 ch07_fangshi_yaotan";
    EXPECT_DOUBLE_EQ(shop->sellRate, 0.3) << "第 9 节来源表末行：sellRate 0.3";
    for (const char* id : {"herb_huangjing_zhi", "herb_yusui_zhi", "herb_zihou_hua", "herb_tianling_guo", "pill_zhuji_dan",
                           "material_zhuji_yaofen"}) {
        const fanren::core::Item* item = app_.data().findItem(id);
        ASSERT_NE(item, nullptr) << id;
        EXPECT_FALSE(item->tradeable) << id << "：第 9 节第 4 条 tradeable=false";
    }
    const fanren::core::Item* zhi = app_.data().findItem("herb_huangjing_zhi");
    ASSERT_NE(zhi, nullptr);
    EXPECT_EQ(zhi->maxAge, 2000) << "第 7 节：黄精芝五阶，上限 2000 年";
    EXPECT_EQ(zhi->grade, 5);
    // 走真面板：背包里放一株 1536 年的黄精芝、禁地三药、筑基丹、药粉，收药摊的卖出列表里一样也没有。
    state() = GameState{};
    state().addItem("herb_huangjing_zhi", 1, 1536);
    for (const char* herb : kSniffed) state().addItem(herb, 1, 400);
    state().addItem(kZhuji, 1, 0);
    state().addItem(kYaofen, 1, 0);
    state().addItem("herb_zishen_cao", 1, 100);   // 反面：紫参收得了
    const auto stacks = fanren::game::ShopScene::buildSellStacks(app_.data(), state(), *shop);
    std::set<std::string> sellable;
    for (const auto& stack : stacks) sellable.insert(stack.itemId);
    EXPECT_EQ(sellable, (std::set<std::string>{"herb_zishen_cao"})) << "收药摊只收得了紫参那一株";
}

// 第 9 节来源表末行：「坊市收药摊那一路……封顶约一千三（校对时若远超这个数，说明哪味药的 tradeable 或上限漏了）」。
// 这一章交到他手里、店里收得了的药（节点 1 的苗：黄精、紫参）每一味推到顶卖给收药摊，一株也不该超过全章的封顶。
// 判据随校对整改 16.5（M4）改过：血红芝推到 300 年一株值 3,460，裁决取最窄的一刀——它改 tradeable=false
// （本作里它只剩 Z1 这一个用处），收药摊与年份上限不动。所以血红芝从「收得了」的名单挪到下面的反面断言里。
TEST_F(Ch07Ledger, TheMarketRoadNeverPaysMoreForOneHerbThanTheWholeChapterCap) {
    const fanren::rules::Shop* shop = app_.shop("ch07_fangshi_yaotan");
    ASSERT_NE(shop, nullptr);
    const fanren::core::Item* xuehong = app_.data().findItem("herb_xuehong_zhi");
    ASSERT_NE(xuehong, nullptr);
    EXPECT_FALSE(xuehong->tradeable) << "校对整改 16.5 M4：血红芝收药摊不收（推到 300 年一株就是 3,460 块）";
    std::ostringstream table;
    int best = 0;
    for (const char* id : {"herb_huangjing_cao", "herb_zishen_cao"}) {
        const fanren::core::Item* item = app_.data().findItem(id);
        ASSERT_NE(item, nullptr) << id;
        ASSERT_TRUE(item->tradeable) << "先验：" << id << " 收药摊收得了";
        const int price = fanren::rules::sellPrice(*shop, *item, item->maxAge);
        table << " " << id << "@" << item->maxAge << "=" << price;
        best = std::max(best, price);
        EXPECT_LE(price, kMarketCap) << id << " 推到顶（" << item->maxAge << " 年）卖 " << price
                                     << " 块：一株就超过施工图第 9 节「收药摊那一路封顶约一千三」——tradeable 或上限漏了";
    }
    std::cout << "[ch07 收药摊一株到顶的价]" << table.str() << "（施工图封顶约 " << kMarketCap << "）" << std::endl;
}

// ---------------------------------------------------------------------------
// 第 9 节第 5 条 / 验收 11、13：嗅灵兽之后百年以上都是 0、幼苗一株不少——两侧同账；n 为 0 时一株不扣
// ---------------------------------------------------------------------------
TEST_F(Ch07Ledger, No5_TheSnifferTakesEveryHundredYearHerbAndNoSeedlingFromBothSides) {
    for (const char* file : {fanren::test::kChapterSixEndingFirst, fanren::test::kChapterSixEndingSecond}) {
        auto loaded = fanren::io::loadGame(fanren::test::chapterFixturePath(root_, file).string());
        ASSERT_TRUE(loaded.ok) << file << "：" << loaded.error;
        state() = loaded.value;
        state().setFlag("ch07.xiashan");
        // 禁地所得（3.2 节点 26、27、28、31 的 give 原数）：成熟 22 株、幼苗 18 株。
        state().addItem("herb_zihou_hua", 4, 1);
        state().addItem("herb_yusui_zhi", 4, 1);
        state().addItem("herb_tianling_guo", 2, 1);
        state().addItem("herb_zihou_hua", 2, 1);
        state().addItem("herb_tianling_guo", 4, 1);
        state().addItem("herb_yusui_zhi", 8, 400);
        state().addItem("herb_zihou_hua", 7, 400);
        state().addItem("herb_tianling_guo", 7, 300);
        state().addItem("herb_yusui_zhi", 2, 1);
        int seedlings = 0;
        int mature = 0;
        for (const char* herb : kSniffed) {
            seedlings += state().itemCount(herb) - state().itemCountAtLeastAge(herb, 100);
            mature += state().itemCountAtLeastAge(herb, 100);
        }
        ASSERT_EQ(mature, 22) << "先验：3.2 节点 31「成熟的，二十二株」";
        ASSERT_EQ(seedlings, 18) << "先验：第 9 节来源表「幼苗 18 株」";
        runScript("ch07/chukou.lua");
        EXPECT_EQ(state().flag("ch07.chujindi"), 1) << file;
        int left = 0;
        for (const char* herb : kSniffed) {
            EXPECT_EQ(state().itemCountAtLeastAge(herb, 100), 0) << file << "：" << herb << " 百年以上一株不留";
            left += state().itemCount(herb);
        }
        EXPECT_EQ(left, seedlings) << file << "：幼苗（< 100 年）一株不少";
    }
    // n 为 0：身上只有幼苗——take_aged 的 count ≤ 0 会按 1 算（契约 4.2），脚本得先判 n > 0，一株也不扣。
    state() = GameState{};
    state().setFlag("ch07.xiashan");
    for (const char* herb : kSniffed) state().addItem(herb, 3, 1);
    runScript("ch07/chukou.lua");
    for (const char* herb : kSniffed) EXPECT_EQ(state().itemCount(herb), 3) << herb << "：没有百年的，就一株也不交";
}

// 读取器自检：它认得出 give / take / take_aged、字面量与表达式、年份参数与注释。
TEST(Ch07LedgerReader, ItReadsGivesTakesAgedTakesAndTheirArguments) {
    const std::string source =
        "-- give(\"pill_zhuji_dan\", 9) 注释不算\n"
        "give(\"herb_yusui_zhi\", 8, 400)\n"
        "if n > 0 and take_aged(\"herb_zihou_hua\", n, 100) then\n"
        "take(\"material_lingshi\", 32)\n"
        "give(\"pill_zhuji_dan\", need)\n";
    const std::vector<Flow> flows = flowsIn("probe.lua", source);
    ASSERT_EQ(flows.size(), 4u) << "注释里那一句不该被读进来";
    EXPECT_EQ(flows[0].verb, "give");
    EXPECT_EQ(flows[0].count, 8);
    EXPECT_EQ(flows[0].ageText, "400");
    EXPECT_EQ(flows[1].verb, "take_aged");
    EXPECT_EQ(flows[1].count, -1);
    EXPECT_EQ(flows[1].ageText, "100");
    EXPECT_EQ(flows[2].verb, "take");
    EXPECT_EQ(flows[2].count, 32);
    EXPECT_EQ(flows[3].count, -1) << "算式记成 -1";
}

}  // namespace
