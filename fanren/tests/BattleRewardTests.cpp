// 战后发放：打赢了，东西真的到手；打输了、跑了，一样也不给。
//
// ---------------------------------------------------------------------------
// 为什么要有这一个文件（第 4 章复验 N-3）
// ---------------------------------------------------------------------------
// `io/BattleLoader.cpp` 早就把 `rewards` 读进 `BattleSetup::reward`，而 src/ 里没有
// 任何一处发放它：第 3、4 章八场仗写了修为、碎银与掉落，一样也到不了玩家手里。
// 设计 3.2 把「贪的回报（修为 220、碎银 96、两块铁精）」当成三选一的另一半——
// 于是那一半不存在，而没有一条测试说得出这件事。
//
// ---------------------------------------------------------------------------
// 判据从哪来
// ---------------------------------------------------------------------------
// 发放函数照抄数据，所以「发下来的数 == 数据里的数」只证明它抄得对，证明不了
// 数据与设计对得上。这里的设计口径一律**写成字面量**、注明出处，不从 data 推：
//   · 三张攻防编成的修为 / 碎银 / 铁精：`docs/ch04-design.md` 3.2 那张表的「收获」一栏；
//   · 坊门那一战的两份金疮药：`docs/ch04-reverify.md` 第七节判据 5；
//   · 第 3 章的仗一块碎银也不发：`data/battles/b03_andao_shishou.json` 的 note
//    （节点 7 那把 30 块的短剑买不买得起，是第 2 章章末那次选择的回响）。
// 与数据逐项对账的那一条（EveryShippedRewardIsPaidInFullOnAWin）是另一件事：
// 它钉的是「发放函数没有漏发、没有多发」，不钉数值本身。
//
// ---------------------------------------------------------------------------
// 口径（契约 docs/interfaces-p2.md 第 6 节）
// ---------------------------------------------------------------------------
// 只有 Won 发。败、逃、敌逃、没打完一律不发——文档里没有一处说过另外几种算不算，
// 按「没写就只认 Won」办。只加修为、不动境界。掉落的 rate 现阶段不读、一律必掉。
#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "core/battle/Battle.h"
#include "core/model/Types.h"
#include "core/rules/Realm.h"
#include "game/Application.h"
#include "game/BattleScene.h"
#include "game/ShopScene.h"
#include "io/BattleLoader.h"

namespace {

namespace fs = std::filesystem;

using fanren::core::BagEntry;
using fanren::core::BattleReward;
using fanren::core::BattleSetup;
using fanren::core::GameState;
using fanren::core::battle::BattlePhase;
using fanren::game::Application;
using fanren::game::BattleScene;
using fanren::game::GrantedReward;
using fanren::game::battleRewardEarned;
using fanren::game::grantBattleReward;
using fanren::game::spiritStones;
using fanren::rules::Realm;

constexpr const char* kSalve = "pill_jinchuang_yao";    // 金疮药
constexpr const char* kIron = "material_jingtie";       // 铁精

std::string assetRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "data" / "battles" / "b04_jia_tianlong.json")) {
            return candidate;
        }
    }
    return ".";
}

class BattleRewardGrant : public ::testing::Test {
protected:
    void SetUp() override {
        auto ready = app_.init(assetRoot(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
    }
    void TearDown() override { app_.shutdown(); }

    // 真编成。拿不到就是 data 被改名或删了——那种情况下面每一条都该红在这里，
    // 而不是红在一个莫名其妙的空指针上。
    const BattleSetup& setup(const std::string& id) {
        const BattleSetup* found = app_.battleSetup(id);
        EXPECT_NE(found, nullptr) << "data/battles 里找不到 " << id;
        static const BattleSetup kEmpty;
        return found != nullptr ? *found : kEmpty;
    }

    Application app_;
};

// ---------------------------------------------------------------------------
// 一、设计口径：打赢之后多出来的，正是设计写的那几样
// ---------------------------------------------------------------------------

struct DesignRow {
    const char* battleId;
    const char* plan;
    int cultivation;
    int money;
    int iron;
    int salve;   // 金疮药（二次复验 R-8 补进设计 3.2 的那一项）
    const char* source;
};

// docs/ch04-design.md 3.2「攻防战三张编成」那张表的「收获」一栏，原样照抄。
constexpr DesignRow kSiegeDesign[] = {
    {"b04_yelangbang_laifan", "守辕门", 180, 60, 0, 3,
     "3.2 第 1 行：修为 180 / 碎银 60 ＋ 三份金疮药"},
    {"b04_gongfang_weijian", "放进来围歼", 220, 96, 2, 4,
     "3.2 第 2 行：修为 220 / 碎银 96 ＋ 四份金疮药 ＋ 两块铁精"},
    {"b04_gongfang_xiadu", "先下药", 150, 40, 0, 1, "3.2 第 3 行：修为 150 / 碎银 40 ＋ 一份金疮药"},
};

TEST_F(BattleRewardGrant, EachSiegePlanPaysWhatDesignThreeTwoLists) {
    for (const DesignRow& row : kSiegeDesign) {
        GameState s;   // 空背包起步：增量就是余额，不会被别处的东西搅浑
        ASSERT_EQ(spiritStones(s), 0) << "先验：起步一块钱也没有";

        const GrantedReward granted =
            grantBattleReward(s, setup(row.battleId).reward, BattlePhase::Won);

        EXPECT_EQ(s.cultivation, row.cultivation)
            << row.plan << "（" << row.battleId << "）的修为与设计不符。docs/ch04-design.md "
            << row.source;
        EXPECT_EQ(spiritStones(s), row.money)
            << row.plan << " 的碎银与设计不符（商店认的那一个钱袋，ShopScene::spiritStones）。"
            << row.source;
        EXPECT_EQ(s.itemCount(kIron), row.iron) << row.plan << " 的铁精与设计不符。" << row.source;
        EXPECT_EQ(s.itemCount(kSalve), row.salve) << row.plan << " 的金疮药与设计不符。" << row.source;

        // 返回值与存档增量是同一件事：日后界面说「收获」读的是返回值。
        EXPECT_EQ(granted.cultivation, row.cultivation) << row.plan;
        EXPECT_EQ(granted.money, row.money) << row.plan;
    }
}

// 复验判据 5 原话：「打完坊门那一战背包里多两份金疮药」。
TEST_F(BattleRewardGrant, TheFightUnderTheWardGateLeavesTwoWoundSalves) {
    GameState s;
    s.addItem(kSalve, 1);   // 身上本来就有一份：验的是「多两份」，不是「有两份」
    ASSERT_EQ(s.itemCount(kSalve), 1);

    static_cast<void>(grantBattleReward(s, setup("b04_jia_tianlong").reward, BattlePhase::Won));

    EXPECT_EQ(s.itemCount(kSalve), 3)
        << "坊门下那一场（b04_jia_tianlong）打赢该多两份金疮药（docs/ch04-reverify.md 第七节判据 5）";
}

// 第 3 章的仗一块碎银也不发。
//
// 出处是 b03_andao_shishou 的 note：「spirit_stones 刻意给 0：节点 7 那把 30 块的玉带
// 短剑买不买得起，是第 2 章章末那次选择的回响，这一仗掉钱会把那条分支冲掉」。
// 谷外那一仗（节点 1）排在节点 7 之前，同一个理由只会更硬。
// 奖励接上以前这一条天然成立（什么都不发）；接上以后它才第一次有了意义。
TEST_F(BattleRewardGrant, ChapterThreeFightsPayNoMoneySoTheSwordStaysAnEcho) {
    for (const char* id : {"b03_gu_wai_elang", "b03_andao_shishou"}) {
        GameState s;
        const GrantedReward granted = grantBattleReward(s, setup(id).reward, BattlePhase::Won);
        // 先验：这一场确实发了点什么（修为）。否则「一块钱也没多」可能只是因为
        // 发放整个没跑——那正是「找不到就算过」的空转法。
        ASSERT_FALSE(granted.empty()) << id << " 打赢了却什么也没发，下面那条就什么也没证明";
        EXPECT_EQ(spiritStones(s), 0)
            << id << " 掉了钱：第 3 章节点 7 那把短剑买不买得起会因此被冲掉（见 b03_andao_shishou 的 note）";
    }
}

// ---------------------------------------------------------------------------
// 二、口径：只有 Won 发
// ---------------------------------------------------------------------------

TEST_F(BattleRewardGrant, OnlyAWinPays) {
    EXPECT_TRUE(battleRewardEarned(BattlePhase::Won));
    EXPECT_FALSE(battleRewardEarned(BattlePhase::Lost));
    EXPECT_FALSE(battleRewardEarned(BattlePhase::Escaped));
    EXPECT_FALSE(battleRewardEarned(BattlePhase::EnemyFled));
    EXPECT_FALSE(battleRewardEarned(BattlePhase::Ongoing));
}

TEST_F(BattleRewardGrant, LosingRunningOrAnUnfinishedFightPaysNothing) {
    // 拿三张里最肥的那一份来试：它若是空的，「什么也没多」就证明不了任何事。
    const BattleReward& richest = setup("b04_gongfang_weijian").reward;
    {
        GameState probe;
        ASSERT_FALSE(grantBattleReward(probe, richest, BattlePhase::Won).empty())
            << "先验：同一份奖励打赢是发得下来的";
    }

    for (BattlePhase phase : {BattlePhase::Lost, BattlePhase::Escaped, BattlePhase::EnemyFled,
                              BattlePhase::Ongoing}) {
        GameState s;
        s.cultivation = 7;
        s.addItem(kSalve, 1);
        const GrantedReward granted = grantBattleReward(s, richest, phase);
        EXPECT_TRUE(granted.empty()) << "战果 " << static_cast<int>(phase) << " 不该发任何东西";
        EXPECT_EQ(s.cultivation, 7) << "战果 " << static_cast<int>(phase) << " 却涨了修为";
        EXPECT_EQ(spiritStones(s), 0) << "战果 " << static_cast<int>(phase) << " 却给了钱";
        EXPECT_EQ(s.itemCount(kSalve), 1) << "战果 " << static_cast<int>(phase) << " 却给了药";
        EXPECT_EQ(s.itemCount(kIron), 0) << "战果 " << static_cast<int>(phase) << " 却给了铁精";
        EXPECT_EQ(s.bag.size(), 1u) << "背包里多出了东西";
    }
}

// 识海那一场设计上必然以 EnemyFled 收场（契约 docs/interfaces-p3-ch03.md 第 5.3 节），
// 所以按「只有 Won 发」的口径，它一样东西也发不下来（docs/interfaces-p2.md 6.1）。
//
// 这条用例钉两件事，意图与数据改之前一样：
//   1. **敌逃不发**，哪怕那一份奖励不是空的。从前拿识海数据里的 150 当那份「不空的
//      奖励」；2026-09-23 那个数按协调者的默认口径改成了 0（技术债 G-16），
//      于是这里另拿一份确实写了东西的奖励来试，否则「什么也没发」在空奖励上恒真。
//   2. **数据不许说谎**：识海那一场的奖励必须是空的。它永远拿不到，写着 150
//      就是一个玩家永远到不了手的数。谁把它改回去，这里当场红——要让识海也发，
//      得先由协调者改口径、再动 battleRewardEarned，这一条会一起红，提醒改的人那是有意的。
TEST_F(BattleRewardGrant, TheDreamFightEndsInFlightSoItsCultivationIsNeverPaid) {
    const BattleReward& notEmpty = setup("b04_gongfang_weijian").reward;
    ASSERT_GT(notEmpty.cultivation, 0) << "先验：拿来试的这一份奖励确实写了修为";
    {
        GameState s;
        const GrantedReward granted = grantBattleReward(s, notEmpty, BattlePhase::EnemyFled);
        EXPECT_TRUE(granted.empty()) << "敌逃也发了东西";
        EXPECT_EQ(s.cultivation, 0);
    }

    const BattleReward& dream = setup("b03_shihai_duoshe").reward;
    EXPECT_EQ(dream.cultivation, 0)
        << "识海那一场必然敌逃、永远发不下来，数据却写着修为 " << dream.cultivation
        << "：一个到不了手的数（docs/interfaces-p2.md 6.1、docs/tech-debt.md G-16）";
    EXPECT_EQ(dream.spiritStones, 0);
    EXPECT_TRUE(dream.drops.empty());
}

// ---------------------------------------------------------------------------
// 三、只加修为，不动境界
// ---------------------------------------------------------------------------
// 第 4 章那三次升境是脚本做的（realm.advance），突破是打坐面板里玩家自己按的那一下。
// 奖励若顺手把境界推上去，就绕过了两者、也抢在了剧情前头。这里把修为喂过门槛，
// 再验境界与两条上限一个字节没动。
TEST_F(BattleRewardGrant, CultivationNeverMovesTheRealm) {
    GameState s;
    s.realm = Realm::QiRefining8;
    s.hp = s.maxHp = fanren::rules::realmMaxHp(Realm::QiRefining8);
    s.mp = s.maxMp = fanren::rules::realmMaxMp(Realm::QiRefining8);
    const int need = fanren::rules::cultivationNeeded(Realm::QiRefining8);
    ASSERT_GT(need, 0);
    s.cultivation = need - 1;

    static_cast<void>(
        grantBattleReward(s, setup("b04_gongfang_weijian").reward, BattlePhase::Won));

    // 先验：修为确实越过了门槛。没越过的话，「境界没变」什么也证明不了。
    ASSERT_GE(s.cultivation, need) << "先验：这份奖励要足以把修为推过八层的门槛";
    EXPECT_EQ(s.realm, Realm::QiRefining8) << "奖励把境界推上去了：那是突破面板与剧情脚本的事";
    EXPECT_EQ(s.maxHp, fanren::rules::realmMaxHp(Realm::QiRefining8));
    EXPECT_EQ(s.maxMp, fanren::rules::realmMaxMp(Realm::QiRefining8));
}

// ---------------------------------------------------------------------------
// 四、发放函数不漏发、不多发（与数据逐项对账，不钉数值）
// ---------------------------------------------------------------------------
TEST_F(BattleRewardGrant, EveryShippedRewardIsPaidInFullOnAWin) {
    const auto battles = fanren::io::loadBattles(assetRoot() + "/data/battles");
    ASSERT_TRUE(battles.ok) << battles.error;

    int withDrops = 0;
    for (const auto& [id, battle] : battles.value) {
        const BattleReward& reward = battle.reward;
        GameState s;
        const GrantedReward granted = grantBattleReward(s, reward, BattlePhase::Won);

        EXPECT_EQ(s.cultivation, std::max(0, reward.cultivation)) << id;
        EXPECT_EQ(spiritStones(s), std::max(0, reward.spiritStones)) << id;
        std::map<std::string, int> expected;
        for (const BagEntry& drop : reward.drops) {
            if (drop.count > 0) expected[drop.itemId] += drop.count;
        }
        for (const auto& [item, count] : expected) {
            EXPECT_EQ(s.itemCount(item), count) << id << " 的掉落 " << item << " 没有如数发下";
        }
        EXPECT_EQ(granted.drops.size(), reward.drops.size()) << id;
        if (!reward.drops.empty()) ++withDrops;
    }
    // 先验：真有掉落可对。一场掉落都没有的话，上面那个循环一条掉落也没验过。
    EXPECT_GT(withDrops, 0) << "data/battles 里一场有掉落的战斗也没有，这条用例在空转";
}

// 负数一律不收：加载器按 readInt 收数，一份写成负数的奖励不该变成倒扣。
TEST_F(BattleRewardGrant, ANegativeAmountIsNeverADeduction) {
    BattleReward typo;
    typo.cultivation = -30;
    typo.spiritStones = -5;
    typo.drops.push_back(BagEntry{kSalve, -2, 0});

    GameState s;
    s.cultivation = 40;
    s.addItem(fanren::game::kSpiritStoneItemId, 10);
    s.addItem(kSalve, 3);

    const GrantedReward granted = grantBattleReward(s, typo, BattlePhase::Won);
    EXPECT_TRUE(granted.empty());
    EXPECT_EQ(s.cultivation, 40);
    EXPECT_EQ(spiritStones(s), 10);
    EXPECT_EQ(s.itemCount(kSalve), 3);
}

// ---------------------------------------------------------------------------
// 五、掉落的 rate：引擎现阶段不读，一律必掉
// ---------------------------------------------------------------------------
// 加载器不读 rate（io/BattleLoader.cpp），发放也就无从按概率发。数据里眼下全是 100，
// 于是「一律必掉」与数据说的是同一件事。**谁写了一个不是 100 的 rate，这里当场红**：
// 那一刻引擎会把它当成必掉，而写数据的人以为是按概率——要么先把随机源接上
//（战斗已有一颗可注入的种子，core::battle::BattleState::setup 的 seed），
// 要么别写。这是契约写死的规则（docs/interfaces-p2.md 6.3），不是今天的普查。

// 一份战斗 JSON 原文里引擎兑现不了的 rate，一条一句。
//
// 扫的是**文件原文**：测试目标拿不到 nlohmann（fanren_io 只 PRIVATE 链接它，
// tests/IoTests.cpp 首部写明了），与 tests/LexiconTests.cpp 同一个办法。
// 战斗数据里只有掉落带 "rate" 这个键，所以按键名扫就是按掉落扫；缺省 = 必掉，放行。
std::vector<std::string> unsupportedRates(const std::string& text, const std::string& where) {
    const auto blank = [](char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; };
    std::vector<std::string> problems;
    const std::string key = "\"rate\"";
    for (std::size_t at = text.find(key); at != std::string::npos; at = text.find(key, at + 1)) {
        std::size_t i = at + key.size();
        while (i < text.size() && blank(text[i])) ++i;
        if (i >= text.size() || text[i] != ':') continue;   // 不是键（比如出现在字符串里）
        ++i;
        while (i < text.size() && blank(text[i])) ++i;
        std::size_t endOfValue = i;
        while (endOfValue < text.size() && !blank(text[endOfValue]) && text[endOfValue] != ',' &&
               text[endOfValue] != '}' && text[endOfValue] != ']') {
            ++endOfValue;
        }
        const std::string value = text.substr(i, endOfValue - i);
        if (value != "100") {
            problems.push_back(where + " 里有一条掉落写了 rate=" + value +
                               "，而引擎现阶段一律必掉（不读 rate）");
        }
    }
    return problems;
}

TEST(BattleRewardData, TheRateScannerHasTeeth) {
    // 先验扫描器本身：写了 50 的必须被抓住，写了 100 与没写的必须放行。
    const std::string bad = R"({"rewards":{"drops":[{"item_id":"x","count":1,"rate": 50}]}})";
    const std::string ok =
        R"({"rewards":{"drops":[{"item_id":"x","count":1,"rate":100},{"item_id":"y","count":1}]}})";
    EXPECT_EQ(unsupportedRates(bad, "probe").size(), 1u) << "rate=50 没被抓住，扫描器没有牙";
    EXPECT_TRUE(unsupportedRates(ok, "probe").empty()) << "100 与缺省都该放行";
}

TEST(BattleRewardData, EveryDropRateIsOneTheEngineCanHonour) {
    const fs::path root = fs::path(assetRoot()) / "data" / "battles";
    int files = 0;
    std::vector<std::string> problems;
    for (const fs::directory_entry& entry : fs::recursive_directory_iterator(root)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".json") continue;
        std::ifstream in(entry.path(), std::ios::binary);
        ASSERT_TRUE(in.is_open()) << entry.path().string();
        std::ostringstream text;
        text << in.rdbuf();
        ++files;
        const auto found = unsupportedRates(text.str(), entry.path().filename().string());
        problems.insert(problems.end(), found.begin(), found.end());
    }
    // 先验：真的扫到了东西。目录找错了的话，下面那条会在零个文件上恒真；
    // 一条掉落都没有的话，它扫的是空气。掉落数走真加载器数，不按 "rate" 键数——
    // 按键数的话，有人把 rate 全删掉（缺省即必掉，合法）这条先验就会冤红。
    ASSERT_GT(files, 0) << "一份战斗数据也没扫到：" << root.string();
    const auto battles = fanren::io::loadBattles(root.string());
    ASSERT_TRUE(battles.ok) << battles.error;
    int drops = 0;
    for (const auto& entry : battles.value) {
        drops += static_cast<int>(entry.second.reward.drops.size());
    }
    ASSERT_GT(drops, 0) << "战斗数据里一条掉落也没有，这条用例在空转";
    for (const std::string& problem : problems) ADD_FAILURE() << problem;
}

// ---------------------------------------------------------------------------
// 六、接线：真的经 BattleScene::finish 发下去
// ---------------------------------------------------------------------------
// 上面几条只测发放函数本身。这几条走玩家会走的那一条路：真编成开打、打到分出胜负、
// 由 update 自己收场（finish）。把 finish 里那一句发放摘掉，第一条必须红。

class BattleRewardScene : public ::testing::Test {
protected:
    void SetUp() override {
        auto ready = app_.init(assetRoot(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
    }
    void TearDown() override { app_.shutdown(); }

    GameState& state() { return app_.state(); }

    // 一个打谁都一下就倒的韩立：本组测的是「收场时发不发」，不是平衡。
    void makeOverwhelming() {
        GameState& s = state();
        s.realm = Realm::CoreLate;
        s.hp = s.maxHp = 5000;
        s.mp = s.maxMp = 0;
    }

    Application app_;
};

TEST_F(BattleRewardScene, WinningARealFightPaysThroughFinish) {
    makeOverwhelming();
    state().addItem(kSalve, 1);

    BattleScene scene("b04_jia_tianlong");
    scene.onEnter(app_);
    ASSERT_TRUE(scene.runToCompletion()) << "先验：这一场要真的打赢";
    ASSERT_TRUE(scene.grantedReward().empty()) << "先验：还没收场，什么也不该发";
    const int cultivationBefore = state().cultivation;
    const int moneyBefore = spiritStones(state());
    ASSERT_EQ(state().itemCount(kSalve), 1) << "先验：收场之前背包没被动过";

    static_cast<void>(scene.update(app_, 0.0));   // 见到胜负已分，自己 finish

    EXPECT_EQ(state().itemCount(kSalve), 3)
        << "坊门那一战打赢，经 finish 收场后该多两份金疮药（复验判据 5）";
    EXPECT_GT(state().cultivation, cultivationBefore) << "打赢了修为一点没涨";
    EXPECT_GT(spiritStones(state()), moneyBefore) << "打赢了一块碎银也没到手";
    // 场景报的与存档里多出来的是同一件事。
    EXPECT_EQ(scene.grantedReward().cultivation, state().cultivation - cultivationBefore);
    EXPECT_EQ(scene.grantedReward().money, spiritStones(state()) - moneyBefore);

    // 同一场不会发两遍：finish 有 finished_ 闸，再推一帧也不会再发。
    static_cast<void>(scene.update(app_, 0.0));
    EXPECT_EQ(state().itemCount(kSalve), 3) << "同一场仗发了两遍";
}

TEST_F(BattleRewardScene, LosingARealFightPaysNothingThroughFinish) {
    // 守辕门那一张不许逃（can_escape=false），于是这里只可能是输。
    GameState& s = state();
    s.realm = Realm::Mortal;
    s.hp = s.maxHp = 1;
    s.mp = s.maxMp = 0;
    s.cultivation = 5;

    BattleScene scene("b04_yelangbang_laifan");
    scene.onEnter(app_);
    static_cast<void>(scene.runToCompletion());
    ASSERT_EQ(scene.battle().phase(), BattlePhase::Lost) << "先验：这一场要真的输掉";

    static_cast<void>(scene.update(app_, 0.0));

    EXPECT_TRUE(scene.grantedReward().empty()) << "输了却发了东西";
    EXPECT_EQ(s.cultivation, 5) << "输了却涨了修为";
    EXPECT_EQ(spiritStones(s), 0) << "输了却给了钱";
    EXPECT_EQ(s.itemCount(kSalve), 0) << "输了却给了金疮药";
}

TEST_F(BattleRewardScene, RunningAwayPaysNothingThroughFinish) {
    // 坊门那一场许逃。韩立血厚（撑得住掷几回骰子）、刀钝（不会先把人打光）。
    GameState& s = state();
    s.realm = Realm::Mortal;
    s.hp = s.maxHp = 5000;
    s.mp = s.maxMp = 0;

    BattleScene scene("b04_jia_tianlong");
    scene.onEnter(app_);
    for (int attempt = 0; attempt < 200; ++attempt) {
        if (scene.battle().phase() != BattlePhase::Ongoing) break;
        if (scene.runToAllyTurn() < 0) break;
        scene.openMenu(app_);
        if (!scene.menuChoose(app_, fanren::game::kBattleMenuEscape)) break;
    }
    ASSERT_EQ(scene.battle().phase(), BattlePhase::Escaped) << "先验：这一场要真的逃掉";

    static_cast<void>(scene.update(app_, 0.0));

    EXPECT_TRUE(scene.grantedReward().empty()) << "逃了却发了东西";
    EXPECT_EQ(s.cultivation, 0) << "逃了却涨了修为";
    EXPECT_EQ(spiritStones(s), 0) << "逃了却给了钱";
    EXPECT_EQ(s.itemCount(kSalve), 0) << "逃了却给了金疮药";
}

}  // namespace
