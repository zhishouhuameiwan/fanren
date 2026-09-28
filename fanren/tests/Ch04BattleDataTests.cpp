// 三张攻防编成与火弹术的那几个数——直接读 data，判据抄自设计文档原文。
//
// ---------------------------------------------------------------------------
// 为什么要有这一个文件（第 4 章独立校对 MEDIUM-1，判据三角的第三条边）
// ---------------------------------------------------------------------------
// 复核方的两处探针：
//   · 把 `b04_gongfang_weijian` 的三波压成**一波**——783 条测试一条也没红；
//   · 把火弹术的 `needMp` 从 8 改成 **1**——同样一条也没红。
// 两个数都被**用**着（通关测试要靠它们才走得完），却从来没有被**断言**过。
// 这正是第 3 章那一课的形状：从被测物推导出来的判据，只发现得了「它变了」，
// 发现不了「它错了」——而这两处连「它变了」都发现不了，因为没人拿它当规格读。
//
// 本文件与 `tests/Ch04SliceTests.cpp` 刻意不同形状：
//   通关测试问「这一章走不走得完」；这里问「这几个数对不对」。
//   不起脚本、不打仗、不走位，只把 data 读出来与设计文档比。
//
// ---------------------------------------------------------------------------
// 一条写断言时的规矩：钉规则，不钉普查
// ---------------------------------------------------------------------------
// `docs/README.md` 点名过一种空转法：**把内容形状当成规格**。
// 第 4 章已经栽过一次——「仓库里每一场战斗都是单波」那条断言转红了，
// 而它恰恰因为多波次功能**真的做成了**才转红：契约写的是规则
//（「没声明波次的按单波跑」），测试写成了普查（「今天有几场」）。
//
// 所以本文件**逐张按 id 钉**，不写「所有的 b04_* 都……」：
// 这三张是设计文档 3.2 节那张表点名写死的三张，不是碰巧在仓库里的三张。
// 新加一场攻防战不该让这里转红；改动这三张里的任何一张**应该**让它转红。
#include <gtest/gtest.h>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <map>
#include <utility>
#include <string>
#include <vector>

#include "core/model/Types.h"
#include "core/rules/Realm.h"
#include "game/Application.h"

namespace {

namespace fs = std::filesystem;

using fanren::core::BattleSetup;
using fanren::core::BattleUnitSpec;
using fanren::game::Application;
using fanren::rules::Realm;

// ---------------------------------------------------------------------------
// 判据。逐行抄自 docs/ch04-design.md 第 3.2 节那张表
// ---------------------------------------------------------------------------
// 每一波按 role_id 记数，而不是只记人数：只记人数的话，
// 把一个马贼换成金光上人照样绿，而那两件事差着一整场仗。
struct WaveSpec {
    int wave;
    const char* roleId;
    int count;
};

struct PlanSpec {
    int bushu;                 // 节点 6 三选一的第几项
    const char* battleId;      // gongfang.lua 首部那张映射表
    const char* what;          // 布置的名字
    int waveCount;
    int unitCount;
    int totalHp;
    // 战场宽高（16×11 / 20×14 / 16×11）随横版改造删了：没有格子，就没有「场子」。
    // 设计 3.2 那一列与「贵的不是人数，是场子」那句一起作废，见本文件下面的说明。
    std::vector<WaveSpec> waves;
};

const std::vector<PlanSpec>& designSaysSo() {
    static const std::vector<PlanSpec> kPlans = {
        {1, "b04_yelangbang_laifan", "守辕门", 4, 9, 490,
         {{0, "yelangbang_mazei", 2},
          {1, "yelangbang_mazei", 2},
          {1, "yelangbang_toumu", 1},
          {2, "yelangbang_toumu", 2},
          {3, "jia_tianlong", 1},
          {3, "jinguang_shangren", 1}}},
        {2, "b04_gongfang_weijian", "放进来围歼", 3, 10, 488,
         {{0, "yelangbang_mazei", 4},
          {1, "yelangbang_mazei", 3},
          {1, "yelangbang_toumu", 1},
          {2, "jia_tianlong", 1},
          {2, "jinguang_shangren", 1}}},
        {3, "b04_gongfang_xiadu", "先下药", 3, 7, 362,
         {{0, "yelangbang_mazei", 2},
          {1, "yelangbang_mazei", 2},
          {1, "yelangbang_toumu", 1},
          {2, "jia_tianlong", 1},
          {2, "jinguang_shangren", 1}}},
    };
    return kPlans;
}

// 本章那一门法术。判据抄自 data/magics/huodan_shu.json 的 note 与设计文档 1.2 节。
constexpr const char* kFireMagic = "magic_huodan_shu";
constexpr int kFireMagicNeedMp = 8;
constexpr int kFireMagicPower = 12;
// 「一场仗放得出十发」——本章终点是炼气八层（80 点法力），note 原话。
constexpr int kFireBoltsPerFight = 10;

std::string assetRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        const fs::path root(candidate);
        if (fs::exists(root / "data" / "battles" / "b04_gongfang_weijian.json") &&
            fs::exists(root / "scripts" / "ch04" / "gongfang.lua")) {
            return candidate;
        }
    }
    return ".";
}

class Ch04BattleData : public ::testing::Test {
protected:
    void SetUp() override {
        auto ready = app_.init(assetRoot(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
    }
    void TearDown() override { app_.shutdown(); }

    const BattleSetup* setupOf(const char* id) { return app_.battleSetup(id); }

    Application app_;
};

// ---------------------------------------------------------------------------
// 先验：这三张真的在 data 里，而且真的有人
// ---------------------------------------------------------------------------
// 少了这一条，下面每一圈循环都可能在一个空 units 上跑零次然后报通过——
// 「比值分母塌成 0」那一类空转法的近亲。
TEST_F(Ch04BattleData, AllThreeCompositionsAreReallyInTheData) {
    ASSERT_EQ(designSaysSo().size(), 3u)
        << "判据表的行数变了。改它之前先改 docs/ch04-design.md 第 3.2 节";
    for (const PlanSpec& plan : designSaysSo()) {
        const BattleSetup* setup = setupOf(plan.battleId);
        ASSERT_NE(setup, nullptr) << "data/battles 里没有 " << plan.battleId;
        ASSERT_FALSE(setup->units.empty()) << plan.battleId << " 一个人也没有";
    }
}

// ---------------------------------------------------------------------------
// 正题一 · 波数与每波的人
// ---------------------------------------------------------------------------
TEST_F(Ch04BattleData, EachPlanHasExactlyTheWavesTheDesignDocLaysOut) {
    for (const PlanSpec& plan : designSaysSo()) {
        const BattleSetup* setup = setupOf(plan.battleId);
        ASSERT_NE(setup, nullptr) << plan.battleId;

        std::map<std::pair<int, std::string>, int> seen;
        int maxWave = 0;
        for (const BattleUnitSpec& unit : setup->units) {
            if (unit.ally) continue;
            seen[std::make_pair(unit.wave, unit.roleId)] += 1;
            maxWave = std::max(maxWave, unit.wave);
        }

        EXPECT_EQ(maxWave + 1, plan.waveCount)
            << plan.what << "（" << plan.battleId << "）的波数与设计文档不符";
        EXPECT_EQ(static_cast<int>(setup->units.size()), plan.unitCount)
            << plan.what << " 的人数与设计文档不符";

        for (const WaveSpec& wave : plan.waves) {
            // 键先取出来：`seen[{a, b}]` 里的逗号会被宏当成参数分隔符。
            const auto key = std::make_pair(wave.wave, std::string(wave.roleId));
            EXPECT_EQ(seen[key], wave.count)
                << plan.what << " 第 " << wave.wave << " 波的 " << wave.roleId
                << " 应有 " << wave.count << " 个";
        }
        // 判据表里没列到的组合必须一个也没有——只查「该有的有没有」，
        // 多塞一个人进去照样绿。
        EXPECT_EQ(seen.size(), plan.waves.size())
            << plan.what << " 里有判据表没列过的单位";
    }
}

// 最后一波一律是金光上人 ＋ 贾天龙（设计文档 3.2 节）。
// 这一条单独钉：它是三张之间**唯一不变的那件事**，而「不变的东西」正是
// 最容易在调编成时被顺手改掉的——本章的高潮就是那一口。
TEST_F(Ch04BattleData, TheLastWaveIsAlwaysTheTwoOfThem) {
    for (const PlanSpec& plan : designSaysSo()) {
        const BattleSetup* setup = setupOf(plan.battleId);
        ASSERT_NE(setup, nullptr) << plan.battleId;
        const int last = plan.waveCount - 1;
        std::vector<std::string> onLast;
        for (const BattleUnitSpec& unit : setup->units) {
            if (!unit.ally && unit.wave == last) onLast.push_back(unit.roleId);
        }
        std::sort(onLast.begin(), onLast.end());
        const std::vector<std::string> expected{"jia_tianlong", "jinguang_shangren"};
        EXPECT_EQ(onLast, expected)
            << plan.what << " 的最后一波不是金光上人 ＋ 贾天龙";
    }
}

// ---------------------------------------------------------------------------
// 正题二 · 三张都是死仗
// ---------------------------------------------------------------------------
// `can_escape` 假、`defeat_is_fatal` 真。写反了任何一个，攻防战就从
//「守住了门派」变成「打不过可以跑」，而硬约束 5 写的是门派存续、
// 节点 10 的战利品要从金光上人的灰里翻出来。
TEST_F(Ch04BattleData, NoneOfTheThreeLetsHimRunAndLosingEndsTheRun) {
    for (const PlanSpec& plan : designSaysSo()) {
        const BattleSetup* setup = setupOf(plan.battleId);
        ASSERT_NE(setup, nullptr) << plan.battleId;
        EXPECT_FALSE(setup->canEscape) << plan.what << " 竟然可以逃";
        EXPECT_TRUE(setup->defeatIsFatal) << plan.what << " 输了竟然不算输";
        // 从前这里还钉着战场宽高（设计 3.2「战场」一列）。横版没有格子，那一列作废。
    }

    // [配对] 同一章里另有两场**不是**死仗的（节点 5 切磋、节点 7 坊门），
    // 少了这一条，上面两句可能只是因为仓库里每一场都这样。
    for (const char* friendly : {"b04_qiecuo_feiyu", "b04_jia_tianlong"}) {
        const BattleSetup* setup = setupOf(friendly);
        ASSERT_NE(setup, nullptr) << friendly;
        EXPECT_TRUE(setup->canEscape) << friendly << " 该是走得掉的";
        EXPECT_FALSE(setup->defeatIsFatal) << friendly << " 输了不该结束这一章";
    }
}

// ---------------------------------------------------------------------------
// 正题三 · 三张的分量：不是难度滑条
// ---------------------------------------------------------------------------
// 设计文档 3.2 节原话：「第 1、2 两张的总血（490 与 488）几乎一模一样——
// 同样的分量，一张摊成四波小的，一张压成三波大的。」
// 这一条钉的是**那句话本身**，而不是某一个数：它是三张编成的全部设计意图。
TEST_F(Ch04BattleData, HoldingTheGateAndSurroundingThemWeighTheSame) {
    std::map<std::string, int> hp;
    for (const PlanSpec& plan : designSaysSo()) {
        const BattleSetup* setup = setupOf(plan.battleId);
        ASSERT_NE(setup, nullptr) << plan.battleId;
        int total = 0;
        for (const BattleUnitSpec& unit : setup->units) {
            if (unit.ally) continue;
            const fanren::core::RoleTemplate* role = app_.data().findRole(unit.roleId);
            ASSERT_NE(role, nullptr) << unit.roleId << " 不在 data/roles 里";
            total += role->maxHp;
        }
        EXPECT_EQ(total, plan.totalHp) << plan.what << " 的总血与设计文档不符";
        hp[plan.battleId] = total;
    }

    const int gate = hp["b04_yelangbang_laifan"];
    const int surround = hp["b04_gongfang_weijian"];
    ASSERT_GT(gate, 0) << "先验：分母不能是 0";
    EXPECT_LE(std::abs(gate - surround) * 100, gate * 5)
        << "守辕门 " << gate << " 与围歼 " << surround
        << " 的分量差出了百分之五。这两张的分野该是**波数与每波人数的对调**，"
           "不是难度滑条——差得多了，第 2 项就又变回一条「点了就亏」的选项。";
}

// ---------------------------------------------------------------------------
// 正题四 · 火弹术那几个数
// ---------------------------------------------------------------------------
// 复核方把 `needMp` 改成 1，全套测试一条也没红。它被用着，却没被断言过。
TEST_F(Ch04BattleData, TheFireBoltCostsWhatTheChapterWasBalancedAround) {
    const fanren::core::Magic* magic = app_.data().findMagic(kFireMagic);
    ASSERT_NE(magic, nullptr) << "data/magics 里没有 " << kFireMagic;

    EXPECT_EQ(magic->needMp, kFireMagicNeedMp);
    EXPECT_EQ(magic->power, kFireMagicPower);
    // castRange（射程 4）随横版改造删了；「隔四格就能先出手」那一半分工不复存在。
    // 取代它的是蓄劲方式：火弹是一发一发的，蓄 N 点劲连发 1+N 发、每发单独判破绽
    //（docs/octopath-battle.md 2.3）。
    EXPECT_EQ(magic->boost, fanren::core::MagicBoost::Hits) << "火弹术蓄劲该是连发";

    // note 原话那一句：「一场仗放得出十发」。钉的是**关系**不是数字——
    // 法力池或单发耗费任何一边动了，这一条都该说话。
    const int maxMp = fanren::rules::realmMaxMp(Realm::QiRefining8);
    ASSERT_GT(magic->needMp, 0) << "先验：分母不能是 0";
    EXPECT_EQ(maxMp / magic->needMp, kFireBoltsPerFight)
        << "炼气八层 " << maxMp << " 点法力、每发 " << magic->needMp
        << " 点，放得出 " << (maxMp / magic->needMp) << " 发，而本章是按十发平衡的"
           "（data/magics/huodan_shu.json 的 note、docs/ch04-design.md 1.2 节）";
}

// ---------------------------------------------------------------------------
// 正题五 · 横版里的「刀砍不动」（硬约束 7 在破势与蓄劲上的落点）
// ---------------------------------------------------------------------------
// 设计 1.2 那张表：金光上人防 30，「刀砍不动——只能靠火弹与毒」。横版里这句话落在破绽上：
// 他的破绽**只有**毒与火，兵刃五类一样也没有。写死成这两样而不是「含火」：多给他一口
// 「剑」，软剑就能削他的架势，那句话就不成立了。
// 同一波的贾天龙「刀砍得动他」（凡人）：破绽里有兵刃；他是最后一波的首领，会蓄势、
// 重招打全体——在他出手之前把他打到破势就能打断，这是这一仗新的「最硬的一口」。
TEST_F(Ch04BattleData, TheManTheBladeCannotCutIsOnlyOpenToFireAndPoison) {
    const fanren::core::RoleTemplate* jinguang = app_.data().findRole("jinguang_shangren");
    ASSERT_NE(jinguang, nullptr);
    EXPECT_EQ(jinguang->weaknesses, fanren::core::kCategoryPoison | fanren::core::kCategoryFire)
        << "设计 1.2：刀砍不动，只能靠火弹与毒";
    EXPECT_EQ(jinguang->weaknesses & fanren::core::kWeaponCategories & ~fanren::core::kCategoryPoison, 0)
        << "兵刃（剑刀拳暗器）一样也不许是他的破绽";
    EXPECT_GE(jinguang->toughness, 1);

    const fanren::core::RoleTemplate* jia = app_.data().findRole("jia_tianlong");
    ASSERT_NE(jia, nullptr);
    EXPECT_NE(jia->weaknesses & (fanren::core::kCategorySword | fanren::core::kCategoryBlade |
                                  fanren::core::kCategoryFist),
              0)
        << "设计 1.2：贾天龙是凡人，刀砍得动他——破绽里得有兵刃";
    EXPECT_GT(jia->charge.every, 0) << "最后一波的首领要会蓄势";
    EXPECT_TRUE(jia->charge.all) << "金狼那一扫打我方全体";
}

}  // namespace
