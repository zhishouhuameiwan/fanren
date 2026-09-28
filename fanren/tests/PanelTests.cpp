// 修炼面板与灵田面板的验收测试。
//
// 无头模式下绘制全是空操作，因此这里一律不测像素，只测两件事：
//   1. 面板对 rules 层的调用与状态写回是不是对的；
//   2. 玩家看得见的那几句话（还差多少修为、为什么催不熟）是不是真的说出来了。
//
// 最要紧的一条是「长打坐与等量多次短打坐收益一致」：它不是数值口味问题，
// 而是防刷判据——两者一旦不等，玩家迟早会发现该往哪边刷。
#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <system_error>
#include <iostream>
#include <string>
#include <vector>

#include "core/model/Types.h"
#include "core/rules/Bottle.h"
#include "core/rules/Calendar.h"
#include "core/rules/Cultivation.h"
#include "core/rules/Field.h"
#include "core/rules/Realm.h"
#include "game/Application.h"
#include "game/CultivationScene.h"
#include "game/FieldScene.h"
#include "game/WorldScene.h"
#include "game/Wording.h"
#include "io/SaveFile.h"

namespace {

using fanren::core::GameState;
using fanren::core::Point;
using fanren::game::Application;
using fanren::game::CultivationScene;
using fanren::game::FieldScene;
using fanren::game::WorldScene;
using fanren::rules::Realm;

// 第 1 章谷中那味最常见的灵草，data/items/herbs 里实有此条。
constexpr const char* kSeedId = "herb_qingfeng_cao";
constexpr const char* kFieldId = "field_shenshougu";

// 仓库根：测试可能从 build/ 或工程根启动，两处都要能找到 data/ 与 maps/。
std::string assetRoot() {
    namespace fs = std::filesystem;
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "maps" / "ch01_shenshougu.tmj")) return candidate;
    }
    return ".";
}

// 扫一颗「这段天数里一次顿悟也没有」的种子。
//
// 顿悟是逐日独立随机，长短两条路径的骰子序列不可能对齐；要断言「分毫不差」，
// 就得让比较落在确定的那一半上。种子扫出来而不写死常量：数值一改，写死的
// 那颗多半立刻失效，而失效的样子是一条偶发红灯。
std::uint32_t calmSeed(const GameState& state, int days) {
    for (std::uint32_t candidate = 1; candidate < 4096; ++candidate) {
        const auto probe = fanren::rules::meditate(
            state.realm, state.aptitude, fanren::game::meditationEffectiveness(state), days,
            candidate);
        if (!probe.insight) return candidate;
    }
    return 0;
}

GameState qiRefiningState(int aptitude) {
    GameState state;
    state.realm = Realm::QiRefining1;
    state.aptitude = aptitude;
    state.day = 1;
    return state;
}

class PanelTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto ready = app_.init(assetRoot(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
    }
    void TearDown() override { app_.shutdown(); }

    // 造一块灵田。返回 id 而不是引用：vector 再 push 一次引用就悬空了。
    void makeField(const std::string& id, int slots) {
        fanren::rules::SpiritField field;
        field.id = id;
        field.slots.resize(static_cast<std::size_t>(slots));
        app_.state().fields.push_back(std::move(field));
    }

    // 把瓶子调成「能用」的状态：有瓶、会用、还有绿液。
    void readyBottle(int drops, int capacity = 3) {
        fanren::rules::Bottle& bottle = app_.state().bottle;
        bottle.owned = true;
        bottle.matureKnown = true;
        bottle.drops = drops;
        bottle.capacity = capacity;
        bottle.lastChargeDay = app_.state().day;
    }

    Application app_;
};

// ---------------------------------------------------------------------------
// advanceDays：唯一的时间入口
// ---------------------------------------------------------------------------

TEST_F(PanelTest, AdvanceDaysMovesTheCalendarAndSettlesEverySystem) {
    GameState& state = app_.state();
    state.day = 1;
    state.realm = Realm::QiRefining1;
    readyBottle(/*drops=*/0);
    makeField(kFieldId, 2);
    ASSERT_TRUE(fanren::rules::plant(*state.findField(kFieldId), 0, kSeedId, state.day));

    app_.advanceDays(fanren::rules::kDaysPerYear);

    EXPECT_EQ(state.day, 1 + fanren::rules::kDaysPerYear) << "日历没走";

    const fanren::rules::SpiritField* field = state.findField(kFieldId);
    ASSERT_NE(field, nullptr);
    EXPECT_EQ(field->slots[0].age, 1) << "灵田没跟着长";
    EXPECT_TRUE(field->slots[0].ripe);
    EXPECT_TRUE(field->slots[1].seedId.empty()) << "空槽不该凭空长出东西";

    // 七天一滴，一年早已满瓶；容量闸在 refill 里，这里只确认它确实凝了。
    EXPECT_EQ(state.bottle.drops, state.bottle.capacity) << "瓶子没跟着凝液";
}

TEST_F(PanelTest, AdvanceDaysIsANoOpForZeroAndNegativeDays) {
    GameState& state = app_.state();
    state.day = 100;
    readyBottle(/*drops=*/1);
    makeField(kFieldId, 1);
    ASSERT_TRUE(fanren::rules::plant(*state.findField(kFieldId), 0, kSeedId, state.day));

    const GameState before = state;
    app_.advanceDays(0);
    app_.advanceDays(-30);

    EXPECT_EQ(state.day, before.day);
    EXPECT_EQ(state.bottle.drops, before.bottle.drops);
    EXPECT_EQ(state.bottle.lastChargeDay, before.bottle.lastChargeDay)
        << "倒退时间会把结算日推到未来，整套周期玩法会停摆到追平为止";
    const fanren::rules::SpiritField* field = state.findField(kFieldId);
    ASSERT_NE(field, nullptr);
    EXPECT_EQ(field->slots[0].age, before.fields[0].slots[0].age);
    EXPECT_EQ(field->slots[0].plantedDay, before.fields[0].slots[0].plantedDay);
}

TEST_F(PanelTest, AdvanceDaysNeverOverfillsTheBottle) {
    GameState& state = app_.state();
    state.day = 1;
    readyBottle(/*drops=*/0, /*capacity=*/3);

    // 挂机一百年也只有三滴：没有这道闸，配上 herbPrice 的超线性曲线就是
    // 无技巧的无限刷钱。
    app_.advanceDays(fanren::rules::kDaysPerYear * 100);
    EXPECT_EQ(state.bottle.drops, 3);
}

TEST_F(PanelTest, BottleChargeDaysGetFasterWithEachTier) {
    EXPECT_EQ(fanren::game::bottleChargeDays(Realm::Mortal), fanren::rules::kBaseChargeDays);
    EXPECT_EQ(fanren::game::bottleChargeDays(Realm::QiRefining13), 7);
    EXPECT_EQ(fanren::game::bottleChargeDays(Realm::FoundationEarly), 5);
    EXPECT_EQ(fanren::game::bottleChargeDays(Realm::CoreLate), 3);
    EXPECT_GT(fanren::game::bottleChargeDays(Realm::QiRefining1),
              fanren::game::bottleChargeDays(Realm::CoreEarly))
        << "境界越高凝液越快，这条单调性是玩家能直接感知的";
}

// ---------------------------------------------------------------------------
// 余数账：长打坐与短打坐必须等价
// ---------------------------------------------------------------------------

TEST(PanelMeditation, LongMeditationIsWorthExactlyTheSameAsManyShortOnes) {
    constexpr int kTotalDays = 360;
    constexpr int kChunkDays = 10;

    // 资质取 0：顿悟概率最低，最容易扫到整段干净的种子。
    const GameState base = qiRefiningState(/*aptitude=*/0);
    const std::uint32_t seed = calmSeed(base, kTotalDays);
    ASSERT_NE(seed, 0u) << "扫不出一颗整段无顿悟的种子";

    GameState longRun = base;
    const auto once = CultivationScene::bankMeditation(longRun, kTotalDays, seed);
    EXPECT_GT(once.cultivation, 0) << "一年闭关总该有进账，否则这条测试什么也没测";
    EXPECT_EQ(once.days, kTotalDays);

    GameState shortRuns = base;
    int shortTotal = 0;
    for (int i = 0; i < kTotalDays / kChunkDays; ++i) {
        shortTotal += CultivationScene::bankMeditation(shortRuns, kChunkDays, seed).cultivation;
    }

    EXPECT_EQ(shortTotal, once.cultivation)
        << "拆成短打坐既不该吃亏也不该占便宜，否则玩家迟早发现该往哪边刷";
    EXPECT_EQ(shortRuns.cultivation, longRun.cultivation);
}

TEST(PanelMeditation, ASingleDayOfMeditationIsBankedNotLost) {
    GameState state = qiRefiningState(/*aptitude=*/50);
    const std::uint32_t seed = calmSeed(state, 64);
    ASSERT_NE(seed, 0u);

    // 炼气期每天不足一点修为，规则层又刻意不设每日保底，所以第一天必然是 0。
    const auto first = CultivationScene::bankMeditation(state, 1, seed);
    EXPECT_EQ(first.cultivation, 0);
    EXPECT_GT(state.cultivationRemainder, 0) << "这一天的功课必须记在账上，不能蒸发";

    int total = first.cultivation;
    for (int i = 0; i < 59; ++i) {
        total += CultivationScene::bankMeditation(state, 1, seed).cultivation;
    }
    EXPECT_GT(total, 0) << "攒够了就该兑现，否则「打坐一日」这个选项是摆设";

    GameState oneGo = qiRefiningState(/*aptitude=*/50);
    const auto lump = CultivationScene::bankMeditation(oneGo, 60, seed);
    EXPECT_EQ(total, lump.cultivation) << "六十次一日与一次六十日必须等价";
}

TEST(PanelMeditation, MeditationWithTheSameSeedIsReproducible) {
    const GameState base = qiRefiningState(/*aptitude=*/70);
    GameState first = base;
    GameState second = base;

    const auto a = CultivationScene::bankMeditation(first, 90, 20260920u);
    const auto b = CultivationScene::bankMeditation(second, 90, 20260920u);

    EXPECT_EQ(a.cultivation, b.cultivation);
    EXPECT_EQ(a.insight, b.insight);
    EXPECT_EQ(first.cultivationRemainder, second.cultivationRemainder);
}

TEST_F(PanelTest, TheRealMeditationPathIsChunkIndependentIncludingInsight) {
    // 上面那条比的是 bankMeditation，种子由测试指定，等于替被测代码把最难的
    // 一半（种子怎么取）先替它办了。这一条走玩家真正按下去的那条路
    // ——meditateFor 自己取种子、自己推日历——并且**不压制顿悟**：面板若把
    // 种子跟天数或修为绑在一起，两条路径的顿悟次数就会分家，这里必然抓到。
    const auto reset = [this]() {
        GameState& state = app_.state();
        state = GameState{};
        state.realm = Realm::QiRefining1;
        state.aptitude = 40;
        state.day = 1;
    };

    reset();
    CultivationScene chopped;
    for (int i = 0; i < 36; ++i) chopped.meditateFor(app_, 10);
    const int choppedCultivation = app_.state().cultivation;
    const int choppedDay = app_.state().day;
    const int choppedRemainder = app_.state().cultivationRemainder;

    reset();
    CultivationScene whole;
    whole.meditateFor(app_, 360);

    EXPECT_GT(choppedCultivation, 0);
    EXPECT_EQ(app_.state().cultivation, choppedCultivation)
        << "真正的入口上也必须等价，否则玩家一算就知道该怎么刷";
    EXPECT_EQ(app_.state().day, choppedDay);
    EXPECT_EQ(app_.state().cultivationRemainder, choppedRemainder) << "余数账也要落在同一处";
}

TEST_F(PanelTest, MeditatingGoesThroughTheOneTimeEntryPoint) {
    GameState& state = app_.state();
    state.day = 1;
    state.realm = Realm::QiRefining1;
    state.aptitude = 50;
    readyBottle(/*drops=*/0);
    makeField(kFieldId, 1);
    ASSERT_TRUE(fanren::rules::plant(*state.findField(kFieldId), 0, kSeedId, state.day));

    CultivationScene scene;
    const auto outcome = scene.meditateFor(app_, fanren::rules::kDaysPerYear);

    EXPECT_EQ(outcome.days, fanren::rules::kDaysPerYear);
    EXPECT_EQ(state.day, 1 + fanren::rules::kDaysPerYear);
    EXPECT_EQ(state.cultivation, outcome.cultivation);
    // 打坐一年之后灵田与瓶子都该跟着走——这正是集中在 advanceDays 的理由。
    EXPECT_EQ(state.findField(kFieldId)->slots[0].age, 1);
    EXPECT_GT(state.bottle.drops, 0);
    EXPECT_FALSE(scene.feedback().empty());
}

// ---------------------------------------------------------------------------
// 冲关
// ---------------------------------------------------------------------------

TEST(PanelCultivationMenu, BreakthroughIsDisabledAndSaysHowMuchIsMissing) {
    GameState state = qiRefiningState(/*aptitude=*/50);
    state.realmCap = Realm::QiRefining2;   // 上限放到下一层：这条测的是「尚差几点」，不是瓶颈
    state.cultivation = 5;

    const auto items = CultivationScene::buildMainItems(state);
    ASSERT_EQ(items.size(), 3u);
    EXPECT_FALSE(items[1].enabled);
    const int missing = fanren::rules::cultivationNeeded(state.realm) - state.cultivation;
    EXPECT_NE(items[1].disabledReason.find(std::to_string(missing)), std::string::npos)
        << "只把字变灰，玩家会当成 bug：还差多少必须写出来";
}

TEST(PanelCultivationMenu, BreakthroughBecomesSelectableAndShowsTheOdds) {
    GameState state = qiRefiningState(/*aptitude=*/50);
    state.realmCap = Realm::QiRefining2;   // 上限恰好放到下一层：瓶颈不挡，才看得见把握
    state.cultivation = fanren::rules::cultivationNeeded(state.realm);

    const auto items = CultivationScene::buildMainItems(state);
    ASSERT_EQ(items.size(), 3u);
    EXPECT_TRUE(items[1].enabled);
    EXPECT_TRUE(items[1].disabledReason.empty());
    const int chance = fanren::rules::breakthroughChance(state.realm, state.cultivation,
                                                         state.aptitude, 0);
    EXPECT_NE(items[1].detail.find(std::to_string(chance)), std::string::npos)
        << "冲关之前要让玩家看见把握几成";
}

TEST(PanelCultivationMenu, BreakthroughAtTheCeilingSaysSoInsteadOfShowingZeroOdds) {
    GameState state;
    state.realm = Realm::CoreLate;
    state.cultivation = 100000;

    const auto items = CultivationScene::buildMainItems(state);
    ASSERT_EQ(items.size(), 3u);
    EXPECT_FALSE(items[1].enabled);
    EXPECT_FALSE(items[1].disabledReason.empty());
}

// 下面三条靠「换一天就换一颗种子」扫出想要的判定结果。actionSeed 是纯函数，
// 因此这些循环是确定的，不会时绿时红；但它们依赖 core/rules/Cultivation.cpp
// 里的 baseChanceFor 与 kBacklashChance——那两个数若大改，循环次数要跟着调。
TEST_F(PanelTest, BreakthroughSuccessRaisesTheRealmAndSpendsCultivation) {
    bool sawSuccess = false;
    for (int day = 1; day <= 400 && !sawSuccess; ++day) {
        GameState& state = app_.state();
        state = GameState{};
        state.day = day;
        state.realm = Realm::QiRefining1;
        state.realmCap = Realm::QiRefining2;   // 上限恰好放到下一层：测的是骰子，不是瓶颈
        state.aptitude = 100;
        const int need = fanren::rules::cultivationNeeded(state.realm);
        state.cultivation = need * 3;

        CultivationScene scene;
        const auto attempt = scene.breakthrough(app_);
        if (!attempt.success) continue;

        sawSuccess = true;
        EXPECT_EQ(state.realm, Realm::QiRefining2) << "成了就该改境界";
        EXPECT_EQ(state.cultivation, need * 3 - attempt.cultivationSpent);
        EXPECT_EQ(attempt.cultivationSpent, need);
    }
    EXPECT_TRUE(sawSuccess) << "九成五的把握摇四百次一次都不成，那就是判定接错了";
}

TEST_F(PanelTest, FailedBreakthroughCostsCultivationButNotTheRealm) {
    bool sawFailure = false;
    for (int day = 1; day <= 400 && !sawFailure; ++day) {
        GameState& state = app_.state();
        state = GameState{};
        state.day = day;
        state.realm = Realm::FoundationLate;   // 冲结丹，基准只有一成五
        state.realmCap = Realm::CoreEarly;   // 上限恰好放到下一关：测的是失手的代价，不是瓶颈
        state.aptitude = 0;
        const int need = fanren::rules::cultivationNeeded(state.realm);
        state.cultivation = need;

        CultivationScene scene;
        const auto attempt = scene.breakthrough(app_);
        if (attempt.success) continue;

        sawFailure = true;
        EXPECT_GT(attempt.cultivationLost, 0) << "没有代价的冲关会被无脑重刷";
        EXPECT_EQ(state.realm, Realm::FoundationLate);
        EXPECT_EQ(state.cultivation, need - attempt.cultivationLost);
        EXPECT_FALSE(scene.feedback().empty());
    }
    EXPECT_TRUE(sawFailure);
}

TEST_F(PanelTest, BacklashHurtsBeyondAnOrdinaryFailure) {
    bool sawBacklash = false;
    for (int day = 1; day <= 800 && !sawBacklash; ++day) {
        GameState& state = app_.state();
        state = GameState{};
        state.day = day;
        state.realm = Realm::FoundationLate;
        state.realmCap = Realm::CoreEarly;   // 上限恰好放到下一关：测的是走火入魔，不是瓶颈
        state.aptitude = 0;
        state.cultivation = fanren::rules::cultivationNeeded(state.realm);
        state.hp = state.maxHp = 40;

        CultivationScene scene;
        const auto attempt = scene.breakthrough(app_);
        if (!attempt.backlash) continue;

        sawBacklash = true;
        EXPECT_FALSE(attempt.success);
        EXPECT_LT(state.hp, 40) << "走火入魔只弹一句话而气血不动，玩家会当成普通失败";
        EXPECT_GE(state.hp, 1) << "重伤不是猝死";
    }
    EXPECT_TRUE(sawBacklash);
}

// ---- 突破带来实打实的属性增长（缺陷 B）----
//
// 在这几条之前，突破只做一件事：把 state.realm 换成下一档。maxHp / maxMp 一个字
// 不动，而全作也没有别的地方让它们涨 —— 玩家从第 1 章练到第 3 章的炼气三层，
// 生存能力与开局一模一样，照那个走法到第 14 章还是 10 点气血。

TEST_F(PanelTest, BreakthroughRaisesHpAndMpToWhatTheNewRealmOwes) {
    bool sawSuccess = false;
    for (int day = 1; day <= 400 && !sawSuccess; ++day) {
        GameState& state = app_.state();
        state = GameState{};
        state.day = day;
        state.realm = Realm::QiRefining1;
        state.realmCap = Realm::QiRefining2;   // 上限恰好放到下一层：测的是成了之后涨多少
        state.aptitude = 100;
        state.cultivation = fanren::rules::cultivationNeeded(state.realm) * 3;
        // 进关口时的属性与当前境界自洽 —— 真机上一路都是这样（突破当场补齐，
        // 老存档由 v3→v4 迁移补齐）。
        state.hp = state.maxHp = fanren::rules::realmMaxHp(state.realm);
        state.mp = state.maxMp = fanren::rules::realmMaxMp(state.realm);
        const int hpBefore = state.maxHp;
        const int mpBefore = state.maxMp;

        CultivationScene scene;
        const auto attempt = scene.breakthrough(app_);
        if (!attempt.success) continue;

        sawSuccess = true;
        ASSERT_EQ(state.realm, Realm::QiRefining2);
        // 一 · 涨了。
        EXPECT_GT(state.maxHp, hpBefore) << "突破之后气血上限一点没动：修为还只是剧情闸门";
        EXPECT_GT(state.maxMp, mpBefore) << "法力上限也没动";
        // 二 · 涨幅与境界对得上（数值依据见 core/rules/Realm.h，曲线本身另有专测）。
        EXPECT_EQ(state.maxHp, fanren::rules::realmMaxHp(Realm::QiRefining2));
        EXPECT_EQ(state.maxMp, fanren::rules::realmMaxMp(Realm::QiRefining2));
        // 三 · 当前值跟着抬，不是停在原地。突破是脱胎换骨，不是受伤 ——
        // 只抬上限的话，玩家冲关成功之后反而看见自己一身残血。
        EXPECT_EQ(state.hp, state.maxHp) << "进关口时是满的，出来也该是满的";
        EXPECT_EQ(state.mp, state.maxMp);
        // 四 · 界面上要说出来，不能只有一根条子悄悄变长。
        EXPECT_NE(scene.feedback().find(std::to_string(state.maxHp - hpBefore)),
                  std::string::npos)
            << "反馈里没写涨了多少气血：" << scene.feedback();
    }
    EXPECT_TRUE(sawSuccess) << "九成五的把握摇四百次一次都不成，那就是判定接错了";
}

TEST_F(PanelTest, AFailedBreakthroughGrantsNothing) {
    // 负向对照：没成就一点不给。缺了这一条，「突破加属性」有可能是被写成了
    // 「点一下冲关就加」—— 那样玩家只要反复点冲关就能刷满气血。
    bool sawFailure = false;
    for (int day = 1; day <= 400 && !sawFailure; ++day) {
        GameState& state = app_.state();
        state = GameState{};
        state.day = day;
        state.realm = Realm::FoundationLate;
        state.realmCap = Realm::CoreEarly;   // 上限恰好放到下一关：测的是没成就不给
        state.aptitude = 0;
        state.cultivation = fanren::rules::cultivationNeeded(state.realm);
        state.hp = state.maxHp = fanren::rules::realmMaxHp(state.realm);
        state.mp = state.maxMp = fanren::rules::realmMaxMp(state.realm);
        const int hpBefore = state.maxHp;
        const int mpBefore = state.maxMp;

        CultivationScene scene;
        const auto attempt = scene.breakthrough(app_);
        if (attempt.success) continue;

        sawFailure = true;
        ASSERT_GT(fanren::rules::realmMaxHp(Realm::CoreEarly), hpBefore)
            << "先验：下一档确实给得更多，否则这条用例在任何实现下都绿";
        EXPECT_EQ(state.maxHp, hpBefore) << "没成却涨了气血，冲关就成了刷属性的按钮";
        EXPECT_EQ(state.maxMp, mpBefore);
    }
    EXPECT_TRUE(sawFailure);
}

TEST_F(PanelTest, BreakthroughWithoutEnoughCultivationChangesNothing) {
    GameState& state = app_.state();
    state.realm = Realm::QiRefining1;
    state.realmCap = Realm::QiRefining2;   // 不补这一行，挡住它的会是瓶颈而不是修为不够
    state.cultivation = 1;

    CultivationScene scene;
    const auto attempt = scene.breakthrough(app_);
    EXPECT_FALSE(attempt.success);
    EXPECT_EQ(attempt.cultivationLost, 0) << "修为不够连骰子都不该摇";
    EXPECT_EQ(state.cultivation, 1);
    EXPECT_EQ(state.realm, Realm::QiRefining1);
}

// ---------------------------------------------------------------------------
// 灵田：催熟的四条失败路径
// ---------------------------------------------------------------------------

TEST_F(PanelTest, MaturingWithoutTheBottleSaysSo) {
    makeField(kFieldId, 1);
    ASSERT_TRUE(fanren::rules::plant(*app_.state().findField(kFieldId), 0, kSeedId, 1));
    app_.state().bottle = fanren::rules::Bottle{};   // 还没捡到瓶子

    FieldScene scene(kFieldId);
    EXPECT_FALSE(scene.matureAt(app_, 0));
    EXPECT_FALSE(scene.feedback().empty());
}

TEST_F(PanelTest, MaturingWithoutKnowingItsUseSaysSo) {
    makeField(kFieldId, 1);
    ASSERT_TRUE(fanren::rules::plant(*app_.state().findField(kFieldId), 0, kSeedId, 1));
    readyBottle(/*drops=*/3);
    app_.state().bottle.matureKnown = false;   // 「绿瓶四年」里的那四年

    FieldScene scene(kFieldId);
    EXPECT_FALSE(scene.matureAt(app_, 0));
    EXPECT_FALSE(scene.feedback().empty());
}

TEST_F(PanelTest, MaturingWithoutDropsSaysSo) {
    makeField(kFieldId, 1);
    ASSERT_TRUE(fanren::rules::plant(*app_.state().findField(kFieldId), 0, kSeedId, 1));
    readyBottle(/*drops=*/0);

    FieldScene scene(kFieldId);
    EXPECT_FALSE(scene.matureAt(app_, 0));
    EXPECT_FALSE(scene.feedback().empty());
}

TEST_F(PanelTest, MaturingAtTheAgeCeilingSaysSo) {
    makeField(kFieldId, 1);
    ASSERT_TRUE(fanren::rules::plant(*app_.state().findField(kFieldId), 0, kSeedId, 1));
    app_.state().findField(kFieldId)->slots[0].age = fanren::rules::kMaxHerbAge;
    readyBottle(/*drops=*/3);

    FieldScene scene(kFieldId);
    EXPECT_FALSE(scene.matureAt(app_, 0));
    EXPECT_FALSE(scene.feedback().empty());
    EXPECT_EQ(app_.state().bottle.drops, 3) << "催不动就不该扣绿液";
}

TEST_F(PanelTest, TheFiveMatureFailuresDoNotAllSayTheSameThing) {
    // 五条路要靠完全不同的方式去解决（找瓶子 / 长见识 / 等凝液 / 等足年 /
    // 换一株），合并成一句「不能催熟」等于把第 2 章的解锁过程整个抹平。
    makeField(kFieldId, 1);
    ASSERT_TRUE(fanren::rules::plant(*app_.state().findField(kFieldId), 0, kSeedId, 1));
    FieldScene scene(kFieldId);
    std::vector<std::string> reasons;

    app_.state().bottle = fanren::rules::Bottle{};
    ASSERT_FALSE(scene.matureAt(app_, 0));
    reasons.push_back(scene.feedback());

    readyBottle(/*drops=*/3);
    app_.state().bottle.matureKnown = false;
    ASSERT_FALSE(scene.matureAt(app_, 0));
    reasons.push_back(scene.feedback());

    readyBottle(/*drops=*/0);
    ASSERT_FALSE(scene.matureAt(app_, 0));
    reasons.push_back(scene.feedback());

    // 刚下种的一株：有瓶、会用、绿液也够，就是还不满一年。
    readyBottle(/*drops=*/3);
    app_.state().findField(kFieldId)->slots[0].age = 0;
    ASSERT_FALSE(scene.matureAt(app_, 0));
    reasons.push_back(scene.feedback());

    readyBottle(/*drops=*/3);
    app_.state().findField(kFieldId)->slots[0].age = fanren::rules::kMaxHerbAge;
    ASSERT_FALSE(scene.matureAt(app_, 0));
    reasons.push_back(scene.feedback());

    for (std::size_t i = 0; i < reasons.size(); ++i) {
        EXPECT_FALSE(reasons[i].empty());
        for (std::size_t j = i + 1; j < reasons.size(); ++j) {
            EXPECT_NE(reasons[i], reasons[j]) << "第 " << i << " 条与第 " << j << " 条重了";
        }
    }
}

TEST_F(PanelTest, MaturingAFreshSeedlingIsRefusedSoWaitingOutTheYearStaysWorthIt) {
    // 允许「种下就浇」的话，多花一滴绿液就能省掉整整一年的生长，于是等足年
    // 永远不值得做，槽位数与生长时间一起失去意义。
    makeField(kFieldId, 1);
    ASSERT_TRUE(fanren::rules::plant(*app_.state().findField(kFieldId), 0, kSeedId, 1));
    ASSERT_EQ(app_.state().findField(kFieldId)->slots[0].age, 0);
    readyBottle(/*drops=*/3);

    FieldScene scene(kFieldId);
    EXPECT_FALSE(scene.matureAt(app_, 0));
    EXPECT_NE(scene.feedback().find("足年"), std::string::npos) << scene.feedback();
    EXPECT_EQ(app_.state().findField(kFieldId)->slots[0].age, 0) << "催不动就不该改年份";
    EXPECT_EQ(app_.state().bottle.drops, 3) << "催不动就不该扣绿液";

    // 反方向：足年之后同一株就催得动了——上面那条不是把催熟整个拦死了。
    app_.state().findField(kFieldId)->slots[0].age = fanren::rules::kRipeAge;
    EXPECT_TRUE(scene.matureAt(app_, 0));
    EXPECT_EQ(app_.state().findField(kFieldId)->slots[0].age,
              fanren::rules::kRipeAge + fanren::rules::kMatureMinYears);
}

TEST_F(PanelTest, MaturingSpendsOneDropAndLiftsTheAge) {
    makeField(kFieldId, 1);
    fanren::rules::SpiritField* field = app_.state().findField(kFieldId);
    ASSERT_TRUE(fanren::rules::plant(*field, 0, kSeedId, 1));
    field->slots[0].age = 10;
    readyBottle(/*drops=*/2);

    FieldScene scene(kFieldId);
    EXPECT_TRUE(scene.matureAt(app_, 0));
    EXPECT_EQ(app_.state().findField(kFieldId)->slots[0].age, 20) << "一滴绿液把年份翻一倍";
    EXPECT_EQ(app_.state().bottle.drops, 1);
}

// ---------------------------------------------------------------------------
// 灵田：年份上限按种取，不按全局取
// ---------------------------------------------------------------------------

TEST_F(PanelTest, MaturingStopsAtTheSpeciesCeilingInsteadOfTheGlobalOne) {
    // 当初那个退化策略的现场：11 滴绿液（炼气期约 77 天）全浇在同一株上。
    // 面板从前给 matureHerb 传全局上限，于是这一株能被推到一万年、卖价八十余
    // 万倍，第 2 章刚开局就能把后面整个游戏的钱赚够。
    //
    // 现在它停在这一味药自己的顶，剩下的绿液只能挪去浇别的药——决策从
    // 「全浇一株」回到「浇哪一株」。
    makeField(kFieldId, 2);
    fanren::rules::SpiritField* field = app_.state().findField(kFieldId);
    ASSERT_TRUE(fanren::rules::plant(*field, 0, kSeedId, 1));
    field->slots[0].age = fanren::rules::kRipeAge;   // 足年，玩家真正会去浇的时机
    readyBottle(/*drops=*/11, /*capacity=*/11);

    const fanren::core::Item* herb = app_.data().findItem(kSeedId);
    ASSERT_NE(herb, nullptr);
    ASSERT_GT(herb->maxAge, 0) << "这味药在 data 里没有年份上限，下面验的就不是这件事了";
    ASSERT_LT(herb->maxAge, fanren::rules::kMaxHerbAge)
        << "一阶灵草的上限等于全局上限的话，这条用例证明不了任何事";

    FieldScene scene(kFieldId);
    int poured = 0;
    while (scene.matureAt(app_, 0)) {
        ++poured;
        ASSERT_LE(poured, 20) << "催熟停不下来";
    }

    EXPECT_GE(poured, 2);
    EXPECT_LE(poured, 3) << "一阶灵草从足年浇到顶该是两三滴的量级，实为 " << poured;
    EXPECT_EQ(app_.state().findField(kFieldId)->slots[0].age, herb->maxAge)
        << "年份没停在这一味药自己的顶上";
    EXPECT_EQ(app_.state().bottle.drops, 11 - poured) << "催不动的那几滴不该被吞掉";
    EXPECT_GT(app_.state().bottle.drops, 0) << "绿液还剩着：它们只能挪去浇别的药";
    EXPECT_NE(scene.feedback().find("极限"), std::string::npos)
        << "到顶要说「年份已至极限」，说成缺绿液玩家会一直攒下去：" << scene.feedback();
}

TEST_F(PanelTest, PlantingTagsTheSlotWithTheSpeciesCeiling) {
    // 上限要跟着种子一起进槽位，自然生长才知道该在哪停住。
    makeField(kFieldId, 1);
    app_.state().addItem(kSeedId, 1, 0);

    FieldScene scene(kFieldId);
    ASSERT_TRUE(scene.plantAt(app_, 0, kSeedId));

    const fanren::core::Item* herb = app_.data().findItem(kSeedId);
    ASSERT_NE(herb, nullptr);
    EXPECT_EQ(app_.state().findField(kFieldId)->slots[0].maxAge, herb->maxAge);
}

TEST_F(PanelTest, FieldGrowthStopsAtTheSpeciesCeilingAcrossTimeSkips) {
    // 上限若只拦绿液，「种下去搁上几百年再收」会原样刷出同一份天价，只是慢些，
    // 而跳时在剧情里是免费的。这条走的是真正的时间入口 advanceDays，
    // 从播种到封顶整条链路都在里面。
    makeField(kFieldId, 1);
    app_.state().addItem(kSeedId, 1, 0);
    FieldScene scene(kFieldId);
    ASSERT_TRUE(scene.plantAt(app_, 0, kSeedId));

    const fanren::core::Item* herb = app_.data().findItem(kSeedId);
    ASSERT_NE(herb, nullptr);

    app_.advanceDays(fanren::rules::kDaysPerYear * 500);
    EXPECT_EQ(app_.state().findField(kFieldId)->slots[0].age, herb->maxAge)
        << "一株一阶灵草在地里躺五百年也成不了万年灵药";
}

TEST_F(PanelTest, OpeningTheFieldBackfillsTheCeilingOnOlderSaves) {
    // 这次修复之前存下的档，槽位里根本没有上限（读回来是 0），自然生长会一路
    // 长到全局上限。打开灵田面板时按 data 补齐，老档也就跟着堵上了。
    makeField(kFieldId, 1);
    fanren::rules::SpiritField* field = app_.state().findField(kFieldId);
    ASSERT_TRUE(fanren::rules::plant(*field, 0, kSeedId, 1));
    ASSERT_EQ(field->slots[0].maxAge, 0) << "这条用例要从「没有上限」的老档状态起步";
    field->slots[0].age = 300;   // 老档里已经长过头的一株

    FieldScene scene(kFieldId);
    scene.onEnter(app_);

    const fanren::core::Item* herb = app_.data().findItem(kSeedId);
    ASSERT_NE(herb, nullptr);
    EXPECT_EQ(app_.state().findField(kFieldId)->slots[0].maxAge, herb->maxAge)
        << "老档的槽位没被补上上限";
    EXPECT_EQ(app_.state().findField(kFieldId)->slots[0].age, 300)
        << "补上限不该顺手把玩家已经拿到的年份削掉——那看起来就是掉档";

    // 补过之后就不再往上长了。
    app_.advanceDays(fanren::rules::kDaysPerYear * 50);
    EXPECT_EQ(app_.state().findField(kFieldId)->slots[0].age, 300);
}

// ---------------------------------------------------------------------------
// 灵田：播种与收割
// ---------------------------------------------------------------------------

TEST_F(PanelTest, PlantingFillsAnEmptySlotAndSpendsTheSeed) {
    makeField(kFieldId, 3);
    app_.state().day = 42;
    app_.state().addItem(kSeedId, 2, 0);

    FieldScene scene(kFieldId);
    EXPECT_TRUE(scene.plantAt(app_, 1, kSeedId));

    const fanren::rules::SpiritField* field = app_.state().findField(kFieldId);
    ASSERT_NE(field, nullptr);
    EXPECT_EQ(field->slots[1].seedId, kSeedId);
    EXPECT_EQ(field->slots[1].plantedDay, 42);
    EXPECT_EQ(field->slots[1].age, 0);
    EXPECT_FALSE(field->slots[1].ripe);
    EXPECT_TRUE(field->slots[0].seedId.empty()) << "只该占用选中的那一畦";
    EXPECT_EQ(app_.state().itemCount(kSeedId), 1) << "种下去的那株要从背包里扣掉";
}

TEST_F(PanelTest, PlantingCannotOverwriteAnOccupiedSlot) {
    makeField(kFieldId, 1);
    app_.state().addItem(kSeedId, 2, 0);
    FieldScene scene(kFieldId);
    ASSERT_TRUE(scene.plantAt(app_, 0, kSeedId));
    app_.state().findField(kFieldId)->slots[0].age = 88;   // 已经养了很久的一株

    EXPECT_FALSE(scene.plantAt(app_, 0, kSeedId)) << "覆种会静默销毁一株养了几十年的灵草";
    EXPECT_FALSE(scene.feedback().empty());
    EXPECT_EQ(app_.state().findField(kFieldId)->slots[0].age, 88);
    EXPECT_EQ(app_.state().itemCount(kSeedId), 1) << "没种成就不该扣种子";
}

TEST_F(PanelTest, HarvestCarriesTheAgeIntoTheBag) {
    makeField(kFieldId, 1);
    fanren::rules::SpiritField* field = app_.state().findField(kFieldId);
    ASSERT_TRUE(fanren::rules::plant(*field, 0, kSeedId, 1));
    field->slots[0].age = 137;
    field->slots[0].ripe = true;

    FieldScene scene(kFieldId);
    EXPECT_TRUE(scene.harvestAt(app_, 0));

    const std::vector<fanren::core::BagEntry>& bag = app_.state().bag;
    ASSERT_EQ(bag.size(), 1u);
    EXPECT_EQ(bag[0].itemId, kSeedId);
    EXPECT_EQ(bag[0].count, 1);
    // 年份不带进背包，催熟攒下的价值就在收割这一步丢光，整条经济循环断在这里。
    EXPECT_EQ(bag[0].herbAge, 137);
    EXPECT_TRUE(app_.state().findField(kFieldId)->slots[0].seedId.empty()) << "收完该腾空";
}

TEST_F(PanelTest, HarvestingAnUnripeSlotIsRefusedWithAReason) {
    makeField(kFieldId, 1);
    ASSERT_TRUE(fanren::rules::plant(*app_.state().findField(kFieldId), 0, kSeedId, 1));

    FieldScene scene(kFieldId);
    EXPECT_FALSE(scene.harvestAt(app_, 0));
    EXPECT_FALSE(scene.feedback().empty());
    EXPECT_TRUE(app_.state().bag.empty());
    EXPECT_EQ(app_.state().findField(kFieldId)->slots[0].seedId, kSeedId);
}

TEST_F(PanelTest, SlotListShowsEverySlotPlusTheWayOut) {
    makeField(kFieldId, 3);
    fanren::rules::SpiritField* field = app_.state().findField(kFieldId);
    ASSERT_TRUE(fanren::rules::plant(*field, 0, kSeedId, 1));
    field->slots[0].age = 5;

    const auto items = FieldScene::buildSlotItems(app_.data(), *field);
    ASSERT_EQ(items.size(), 4u) << "三畦加一条「离开」";
    EXPECT_NE(items[0].detail.find("5"), std::string::npos) << "年份要看得见";
    EXPECT_FALSE(items[1].detail.empty()) << "空畦也要有字，留白只会被当成没画出来";
    EXPECT_TRUE(items[3].enabled);
}

TEST_F(PanelTest, SeedListOnlyOffersHerbsFromTheBag) {
    makeField(kFieldId, 1);
    app_.state().addItem(kSeedId, 3, 0);
    app_.state().addItem("pill_huiqi_dan", 5, 0);   // 丹药不是种子

    std::vector<std::string> seedIds;
    const auto items = FieldScene::buildSeedItems(app_.data(), app_.state(), seedIds);
    ASSERT_EQ(seedIds.size(), 1u);
    EXPECT_EQ(seedIds[0], kSeedId);
    ASSERT_EQ(items.size(), 2u) << "一味灵草加一条「返回」";
    EXPECT_NE(items[0].detail.find("3"), std::string::npos);
}

// ---------------------------------------------------------------------------
// 接进游戏：面朝设施按确认键
// ---------------------------------------------------------------------------

TEST_F(PanelTest, FacingTheMeditationSpotOpensTheCultivationPanel) {
    auto loaded = app_.loadMap("ch01_shenshougu", std::string{});
    ASSERT_TRUE(loaded.ok) << loaded.error;

    const fanren::core::TileMap* map = app_.currentMap();
    ASSERT_NE(map, nullptr);
    const fanren::core::MapObject* spot = nullptr;
    for (const auto& object : map->objects) {
        if (object.type == "facility" && object.property("kind") == "meditate") spot = &object;
    }
    ASSERT_NE(spot, nullptr) << "第 1 章谷中地图应当有一个打坐点";

    app_.state().position = Point{spot->position.x, spot->position.y + 1};
    app_.state().facing = 0;   // 朝上，正对蒲团

    WorldScene world;
    EXPECT_TRUE(world.interact(app_)) << "facility 此前完全没接，按确认键毫无反应";
    app_.tick(1.0 / 60.0);   // 让延迟入栈的面板真正进场景栈
    ASSERT_NE(app_.topScene(), nullptr);
    EXPECT_EQ(app_.topScene()->name(), "Cultivation");
}

TEST_F(PanelTest, FacingTheHerbPlotOpensTheFieldPanelAndBuildsTheField) {
    auto loaded = app_.loadMap("ch01_shenshougu", std::string{});
    ASSERT_TRUE(loaded.ok) << loaded.error;

    const fanren::core::TileMap* map = app_.currentMap();
    ASSERT_NE(map, nullptr);
    const fanren::core::MapObject* plot = nullptr;
    for (const auto& object : map->objects) {
        if (object.type == "facility" && object.property("kind") == "field") plot = &object;
    }
    ASSERT_NE(plot, nullptr);
    const std::string refId = plot->property("ref_id");
    ASSERT_FALSE(refId.empty());
    ASSERT_EQ(app_.state().findField(refId), nullptr) << "开局存档里本来没有这块田";

    app_.state().position = Point{plot->position.x, plot->position.y + 1};
    app_.state().facing = 0;

    WorldScene world;
    EXPECT_TRUE(world.interact(app_));
    app_.tick(1.0 / 60.0);
    ASSERT_NE(app_.topScene(), nullptr);
    EXPECT_EQ(app_.topScene()->name(), "Field");

    // 灵田的定义在地图上，第一次走到田边就该按 slots 建出来。
    const fanren::rules::SpiritField* field = app_.state().findField(refId);
    ASSERT_NE(field, nullptr);
    EXPECT_EQ(field->slots.size(), 6u) << "槽位数要照地图上的 slots 属性来";
}

TEST_F(PanelTest, FacilitiesOccupyTheirCellSoTheyCanBeFaced) {
    auto loaded = app_.loadMap("ch01_shenshougu", std::string{});
    ASSERT_TRUE(loaded.ok) << loaded.error;

    const fanren::core::TileMap* map = app_.currentMap();
    ASSERT_NE(map, nullptr);
    const fanren::core::MapObject* spot = nullptr;
    for (const auto& object : map->objects) {
        if (object.type == "facility" && object.property("kind") == "meditate") spot = &object;
    }
    ASSERT_NE(spot, nullptr);

    const Point beside{spot->position.x, spot->position.y + 1};
    app_.state().position = beside;
    app_.state().facing = 2;

    // 能站上去的话，玩家会正踩在蒲团上却怎么按都没反应。
    WorldScene world;
    EXPECT_FALSE(world.tryStep(app_, 0, -1));
    EXPECT_EQ(app_.state().position, beside);
    EXPECT_EQ(app_.state().facing, 0) << "撞不过去也要把脸转过去";
}

// ---------------------------------------------------------------------------
// 灵田：解锁之前，「催熟」不许摆在那里等人点
//
// 设计文档第 3 节的明文：「解锁前，灵田面板的催熟项应当直接不显示『可催熟』
// 的引导，不要让玩家在解锁前反复去点。」硬约束 #1 又要求拾瓶与发现催熟相隔
// 四年——那四年里玩家**必然**处在「有瓶有液但不会用」的状态，所以这不是一个
// 边角情形，是一段必经的三个游戏年。
// ---------------------------------------------------------------------------

namespace {

fanren::rules::FieldSlot ripeSlot(int age = fanren::rules::kRipeAge) {
    fanren::rules::FieldSlot slot;
    slot.seedId = kSeedId;
    slot.age = age;
    slot.ripe = age >= fanren::rules::kRipeAge;
    slot.maxAge = 44;
    return slot;
}

fanren::rules::Bottle bottleWith(bool owned, bool matureKnown, int drops) {
    fanren::rules::Bottle bottle;
    bottle.owned = owned;
    bottle.matureKnown = matureKnown;
    bottle.drops = drops;
    bottle.capacity = 3;
    return bottle;
}

}  // namespace

TEST(FieldSlotActions, MatureIsGreyedOutWhileHeStillDoesNotKnowWhatTheDropIsFor) {
    const auto items =
        FieldScene::buildSlotActionItems(bottleWith(true, /*matureKnown=*/false, 3), ripeSlot(), 44);

    ASSERT_EQ(items.size(), 3u) << "三个动作的下标写死在 update 里，条目数不许变";
    EXPECT_FALSE(items[0].enabled) << "不会用的时候还能点，玩家点一次被回绝一次，整整三年";
    EXPECT_FALSE(items[0].disabledReason.empty()) << "只把字变灰，玩家只会当成 bug";
    EXPECT_NE(items[0].disabledReason.find("尚不知"), std::string::npos)
        << "理由要站在少年这一侧说（他不知道这滴绿的有什么用），实为："
        << items[0].disabledReason;
    EXPECT_TRUE(items[0].detail.empty())
        << "「耗绿液一滴」是可催熟的引导，解锁前不该出现，实为：" << items[0].detail;
}

TEST(FieldSlotActions, MatureBecomesSelectableTheMomentHeKnows) {
    // 反方向。只差 matureKnown 一个布尔，其余一模一样——两条合起来才证明
    // 「置灰」是这个开关管的，而不是被别的什么顺手关掉了。
    const auto locked =
        FieldScene::buildSlotActionItems(bottleWith(true, /*matureKnown=*/false, 3), ripeSlot(), 44);
    const auto known =
        FieldScene::buildSlotActionItems(bottleWith(true, /*matureKnown=*/true, 3), ripeSlot(), 44);

    ASSERT_EQ(known.size(), 3u);
    EXPECT_FALSE(locked[0].enabled);
    EXPECT_TRUE(known[0].enabled) << "会用了还点不动，那是把整条经济循环锁死了";
    EXPECT_TRUE(known[0].disabledReason.empty());
    EXPECT_EQ(known[0].detail, "耗绿液一滴") << "能催熟了就要把代价写出来";
}

TEST_F(PanelTest, TheMatureRowNeverPromisesSomethingMatureAtWouldRefuse) {
    // 负向的那一半：把「这一行能不能点」与「点下去会不会成」逐一对齐。
    // 谁把置灰去掉（或者反过来，把它拦得太宽），这条矩阵立刻转红——
    // 这正是本项目最惨那次事故缺的东西：没人验过检查本身有没有牙。
    makeField(kFieldId, 1);
    int enabledSeen = 0;
    int disabledSeen = 0;

    for (const bool owned : {false, true}) {
        for (const bool known : {false, true}) {
            for (const int drops : {0, 2}) {
                for (const int age : {0, fanren::rules::kRipeAge, 44}) {
                    fanren::rules::SpiritField* field = app_.state().findField(kFieldId);
                    field->slots[0] = ripeSlot(age);
                    app_.state().bottle = bottleWith(owned, known, drops);

                    const auto items =
                        FieldScene::buildSlotActionItems(app_.state().bottle, field->slots[0], 44);
                    ASSERT_EQ(items.size(), 3u);

                    FieldScene scene(kFieldId);
                    const bool done = scene.matureAt(app_, 0);
                    EXPECT_EQ(items[0].enabled, done)
                        << "列表说 " << (items[0].enabled ? "能点" : "点不动") << "，实际却 "
                        << (done ? "成了" : "被回绝了") << "（owned=" << owned
                        << " known=" << known << " drops=" << drops << " age=" << age << "）";
                    if (!done) {
                        EXPECT_EQ(items[0].disabledReason, scene.feedback())
                            << "列表上的理由与点下去得到的理由不是同一句";
                    }
                    (items[0].enabled ? enabledSeen : disabledSeen) += 1;
                }
            }
        }
    }
    // 两边都要真的出现过，否则上面那圈循环可能全落在同一侧，什么也没验。
    EXPECT_GT(enabledSeen, 0) << "一整轮下来没有一格是能点的，这条用例是空转的";
    EXPECT_GT(disabledSeen, 0) << "一整轮下来没有一格是点不动的，这条用例是空转的";
}

// ---------------------------------------------------------------------------
// 分阶段用词：凡人篇不许漏出修仙界的字眼
// ---------------------------------------------------------------------------

namespace {

using fanren::game::PanelStage;

// 禁词表。前一半来自 docs/ch02-design.md 第 1 节硬约束 #8 的原文，后一半是
// docs/ch02-review.md HIGH-2 点名的那几个（境界 / 法力 / 灵石）。
const std::vector<std::string>& forbiddenWords() {
    static const std::vector<std::string> kWords{
        "修仙", "灵根", "法术", "灵气", "气感", "炼气", "筑基", "真元", "走火入魔",
        "法力", "境界", "灵石", "元神", "金丹", "神通", "法宝", "丹田", "真气",
    };
    return kWords;
}

std::string firstForbiddenWord(const std::string& text) {
    for (const std::string& word : forbiddenWords()) {
        if (text.find(word) != std::string::npos) return word;
    }
    return {};
}

}  // namespace

TEST(PanelWording, TheScannerItselfHasTeeth) {
    // **先验扫描器，再用扫描器验别人。** 本项目栽得最惨的一次就是校验器的正则
    // 被吃掉转义、从此永远报通过而无人发现——一条只见过合法输入的检查不是检查。
    EXPECT_EQ(firstForbiddenWord("真元逆冲，走火入魔！气血翻涌"), "真元");
    EXPECT_EQ(firstForbiddenWord("忽而心有所悟，周身灵气自行汇聚"), "灵气");
    EXPECT_EQ(firstForbiddenWord("炼气三层"), "炼气");
    EXPECT_EQ(firstForbiddenWord("囊中灵石不足"), "灵石");
    EXPECT_TRUE(firstForbiddenWord("一直拗口的那一句忽然顺了下来").empty());
    EXPECT_TRUE(firstForbiddenWord("碎银 12 块").empty());
    EXPECT_FALSE(forbiddenWords().empty());
}

TEST(PanelWording, TheMortalPanelSaysNothingAnElevenYearOldCouldNotSay) {
    const auto strings = fanren::game::cultivationPanelStrings(PanelStage::Mortal);
    // 条目数兜一下底：谁把用词表清空或者漏列一半，下面那圈循环会静默通过。
    EXPECT_GE(strings.size(), 50u) << "用词表缩水了，这条扫描就扫了个寂寞";

    for (const std::string& text : strings) {
        EXPECT_TRUE(firstForbiddenWord(text).empty())
            << "凡人阶段的面板上出现了「" << firstForbiddenWord(text) << "」：" << text;
    }
    EXPECT_TRUE(firstForbiddenWord(fanren::game::currencyName(PanelStage::Mortal)).empty());
    EXPECT_TRUE(firstForbiddenWord(fanren::game::currencyAmount(PanelStage::Mortal, 145)).empty());
}

TEST(PanelWording, TheImmortalPanelIsExactlyWhatWouldHaveFailedTheScan) {
    // 上一条的负向对照，也是「换词开关切错表会不会被抓住」的证明：
    // 修仙那一套本身**必然**踩禁词。所以只要 wordingStage 在凡人状态下错选了
    // 修仙表，上一条就一定转红——它不可能靠运气蒙混过去。
    const auto strings = fanren::game::cultivationPanelStrings(PanelStage::Immortal);
    int hits = 0;
    for (const std::string& text : strings) {
        if (!firstForbiddenWord(text).empty()) ++hits;
    }
    EXPECT_GE(hits, 4) << "修仙那一套里居然扫不出几个修仙词，两张表多半是同一张";

    // 修仙阶段该说的话照旧说，不是把词删光了事。
    const auto& words = fanren::game::cultivationLexicon(PanelStage::Immortal);
    EXPECT_STREQ(words.mpLabel, "法力　");
    EXPECT_STREQ(words.realmLabel, "境界　");
    EXPECT_NE(std::string(words.backlashPrefix).find("走火入魔"), std::string::npos);
    EXPECT_EQ(fanren::game::realmText(PanelStage::Immortal, Realm::QiRefining3), "炼气三层");
    EXPECT_STREQ(fanren::game::currencyName(PanelStage::Immortal), "灵石");
}

TEST(PanelWording, TheSwitchIsTheStoryFlagAndNotTheRealm) {
    // 这条钉住的是**触发条件的选择**。第 2 章章末口诀到第三层，引擎里就是
    // QiRefining3——按境界切的话，正是在这一章最不该出事的地方漏出「炼气三层」。
    GameState state;
    state.realm = Realm::QiRefining3;
    state.cultivation = 5;

    ASSERT_EQ(fanren::game::wordingStage(state), PanelStage::Mortal)
        << "境界升到炼气三层就换词的话，第 2 章章末必然违约";
    EXPECT_EQ(fanren::game::realmText(fanren::game::wordingStage(state), state.realm), "第三层");
    EXPECT_STREQ(fanren::game::currencyName(state), "碎银");

    // 连拼出来的那几行一并扫：状态栏与列表项都是现拼的，用词表干净不等于
    // 拼出来的句子干净。
    const auto items = CultivationScene::buildMainItems(state);
    for (const fanren::ui::ListItem& item : items) {
        EXPECT_TRUE(firstForbiddenWord(item.label).empty()) << item.label;
        EXPECT_TRUE(firstForbiddenWord(item.detail).empty()) << item.detail;
        EXPECT_TRUE(firstForbiddenWord(item.disabledReason).empty()) << item.disabledReason;
    }

    // 剧情旗标一置，整套词立刻回到修仙那一边。
    state.setFlag(fanren::game::kXiuxianKnownFlag);
    EXPECT_EQ(fanren::game::wordingStage(state), PanelStage::Immortal);
    EXPECT_EQ(fanren::game::realmText(fanren::game::wordingStage(state), state.realm), "炼气三层");
    EXPECT_STREQ(fanren::game::currencyName(state), "灵石");
}

TEST(PanelWording, TheMortalPanelDoesNotOfferAYearLongSeclusion) {
    const auto& mortal = CultivationScene::meditateOptions(PanelStage::Mortal);
    ASSERT_FALSE(mortal.empty());
    int longest = 0;
    for (const auto& option : mortal) longest = std::max(longest, option.days);
    EXPECT_EQ(longest, fanren::rules::kDaysPerMonth)
        << "一个十来岁的武馆杂役闭关一年，四年苦修就成了按一次按钮";

    const auto& immortal = CultivationScene::meditateOptions(PanelStage::Immortal);
    int immortalLongest = 0;
    for (const auto& option : immortal) immortalLongest = std::max(immortalLongest, option.days);
    EXPECT_EQ(immortalLongest, fanren::rules::kDaysPerYear) << "修仙之后闭关一年要还回去";
}

// ---------------------------------------------------------------------------
// 修炼节奏：四年苦修要真的是四年
// ---------------------------------------------------------------------------

namespace {

// 第 2 章主线的日历。天数逐条取自 scripts/ch02/*.lua 里的 advance_days，
// 段落划分对应 docs/ch02-design.md 第 2 节那五段。
//
// 只算**主线必然发生**的那些：段一那 30 日是 caiyao.lua 的闸门
// （today() - diyike_day >= 30），玩家非坐不可；其余全是脚本推的。
// cuishu.lua 第二场那 21 天要等绿液消耗接上之后才会真的走，故意不算进来
// ——少算只会让下面的结论更保守。
struct ChapterBeat {
    const char* label;
    int meditateDays;
    int scriptDays;
    bool segmentEnd;
};

const std::vector<ChapterBeat>& chapterTwoBeats() {
    static const std::vector<ChapterBeat> kBeats{
        {"段一·节点 2 第一次打坐（教修炼面板）", 0, 0, false},
        {"段一·凑满采药的三十日闸门", 30, 0, false},
        {"段一末·第一次卖药（maiyao +180）", 0, 180, true},
        {"段二·第八日开盖（kaigai +7）", 0, 7, false},
        {"段二末·藏起瓶子（cangping +180）", 0, 180, true},
        {"段三·次日交药（zhitong +1）", 0, 1, false},
        {"段三末·一年过去（zhitong +365）", 0, 365, true},
        {"段四末·同伴还人情（renqing +365）", 0, 365, true},
        {"段五·试药兔死、发现催熟（shiyao +1）", 0, 1, false},
        {"章末·算账（suanzhang）", 0, 0, true},
    };
    return kBeats;
}

struct BeatState {
    std::string label;
    bool segmentEnd = false;
    int day = 0;
    int cultivation = 0;
    Realm realm = Realm::Mortal;
};

// 「按主线正常推进的玩家」：主线要他坐的那三十日坐满，其余一天也不多坐；
// 每到一处就打开面板（结日课），能冲关就冲，冲不动就走。这是**下界**——
// 多打坐的玩家只会更快，所以拿它当判据最稳。
std::vector<BeatState> playChapterTwo(Application& app) {
    GameState& state = app.state();
    state = GameState{};
    state.realm = Realm::Mortal;
    state.realmCap = Realm::CoreLate;   // 这条量的是日课曲线本身，不让剧情上限替它把「不多不少」
    state.aptitude = 50;
    state.day = 1;

    std::vector<BeatState> trace;
    for (const ChapterBeat& beat : chapterTwoBeats()) {
        CultivationScene scene;
        scene.onEnter(app);   // 打开面板：先把欠的日课结清
        if (beat.meditateDays > 0) scene.meditateFor(app, beat.meditateDays);
        if (beat.scriptDays > 0) app.advanceDays(beat.scriptDays);

        CultivationScene after;
        after.onEnter(app);
        for (int guard = 0; guard < 16; ++guard) {
            const auto items = CultivationScene::buildMainItems(state);
            if (!items[1].enabled) break;
            after.breakthrough(app);
        }
        trace.push_back(BeatState{beat.label, beat.segmentEnd, state.day, state.cultivation,
                                  state.realm});
    }
    return trace;
}

}  // namespace

TEST_F(PanelTest, TheFourYearsOfChapterTwoAddUpToTheThirdLayer) {
    const std::vector<BeatState> trace = playChapterTwo(app_);
    ASSERT_FALSE(trace.empty());

    // 这张表直接进交付报告，跑一次就是一次实算，不靠手推。
    std::cout << "\n[ch02 修炼节奏实算]\n";
    for (const BeatState& beat : trace) {
        std::cout << (beat.segmentEnd ? "  * " : "    ") << "第 " << beat.day << " 日　"
                  << fanren::game::realmText(PanelStage::Mortal, beat.realm) << "　火候 "
                  << beat.cultivation << "　" << beat.label << "\n";
    }
    std::cout << std::endl;

    const BeatState& last = trace.back();
    EXPECT_EQ(last.realm, Realm::QiRefining3)
        << "章末应当正好在口诀第三层，实为 "
        << fanren::game::realmText(PanelStage::Mortal, last.realm);
    // 前四年不该早早就冲过头：第三层是大纲给第 2 章的终点，不是中途站。
    EXPECT_LT(trace.front().realm, Realm::QiRefining1) << "开章就已经入门了，那四年白写";
}

TEST_F(PanelTest, OneYearOfSittingIsNowhereNearTheThirdLayer) {
    // 校对方算过的那条路：按一次「闭关一年」就够到门槛、85% 冲进第一层。
    // 改完之后这一按仍然能入门（不该把玩家的主动性也一并拿掉），但离第三层
    // 差得远——四年苦修不是一次按钮能顶掉的。
    GameState& state = app_.state();
    state = GameState{};
    state.realm = Realm::Mortal;
    state.realmCap = Realm::CoreLate;   // 量的是一年打坐本身够不够，不让瓶颈替它挡住
    state.aptitude = 50;
    state.day = 1;

    CultivationScene scene;
    scene.onEnter(app_);
    scene.meditateFor(app_, fanren::rules::kDaysPerYear);
    for (int guard = 0; guard < 16; ++guard) {
        const auto items = CultivationScene::buildMainItems(state);
        if (!items[1].enabled) break;
        scene.breakthrough(app_);
    }

    EXPECT_LT(state.realm, Realm::QiRefining3)
        << "一次闭关一年就到了第三层，整章的前提被抽空了；实为 "
        << fanren::game::realmText(PanelStage::Mortal, state.realm);
    EXPECT_EQ(state.day, 1 + fanren::rules::kDaysPerYear);
}

// ---------------------------------------------------------------------------
// 日课本身
// ---------------------------------------------------------------------------

TEST(DailyPractice, SettlingOftenIsWorthExactlyAsMuchAsSettlingOnce) {
    // 防刷判据，与「长打坐 == 多次短打坐」同一条纪律：日课若能靠反复开关面板
    // 多结几次，玩家迟早发现。
    constexpr int kSpan = 1000;

    GameState once;
    once.day = 1;
    once.lastPracticeDay = 1;
    once.day = 1 + kSpan;
    const int lump = fanren::game::settleDailyPractice(once);

    GameState often;
    often.day = 1;
    often.lastPracticeDay = 1;
    int drip = 0;
    for (int i = 0; i < kSpan; ++i) {
        often.day += 1;
        drip += fanren::game::settleDailyPractice(often);
    }

    EXPECT_GT(lump, 0) << "一千天日课一点也没有，那这条测试什么也没测";
    EXPECT_EQ(drip, lump);
    EXPECT_EQ(often.cultivation, once.cultivation);
    EXPECT_EQ(often.lastPracticeDay, once.lastPracticeDay) << "水位也要落在同一处";
}

TEST(DailyPractice, LeavesTheOddDaysOnTheWatermarkInsteadOfEatingThem) {
    GameState state;
    state.lastPracticeDay = 1;
    state.day = 1 + fanren::game::kDailyPracticeBlockDays - 1;   // 差一天不足一块

    EXPECT_EQ(fanren::game::settleDailyPractice(state), 0);
    EXPECT_EQ(state.lastPracticeDay, 1) << "不足一块就把水位推走，那几天的功夫就蒸发了";
}

TEST(DailyPractice, DoesNotRunOnceHeKnowsAboutImmortals) {
    // 日课只补凡人那一段：那一段的日历是剧情整年整年替玩家推的。修仙之后
    // 闭关多久是玩家自己的决定，再叠一层被动收益等于改掉后面每一章的节奏。
    GameState state;
    state.realm = Realm::QiRefining3;
    state.lastPracticeDay = 1;
    state.day = 2000;
    state.setFlag(fanren::game::kXiuxianKnownFlag);

    EXPECT_EQ(fanren::game::settleDailyPractice(state), 0);
    EXPECT_EQ(state.cultivation, 0);
    EXPECT_EQ(state.lastPracticeDay, state.day) << "水位要拨到当天，免得日后忽然补一大笔";

    // 反方向：同样的天数，凡人阶段是有进账的——上面那条不是因为算法坏了才为零。
    GameState mortal;
    mortal.realm = Realm::QiRefining3;
    mortal.lastPracticeDay = 1;
    mortal.day = 2000;
    EXPECT_GT(fanren::game::settleDailyPractice(mortal), 0);
}

TEST(DailyPractice, AnOldSaveDoesNotGetAWindfall) {
    // 水位为 0 是「还没起课」。倒补的话，这次改动之前存下的档一打开面板就凭空
    // 多出几百天修为。
    GameState state;
    state.day = 5000;
    state.lastPracticeDay = 0;

    EXPECT_EQ(fanren::game::settleDailyPractice(state), 0);
    EXPECT_EQ(state.cultivation, 0);
    EXPECT_EQ(state.lastPracticeDay, 5000) << "从今天起算";
}

TEST_F(PanelTest, MeditatingDoesNotGetPaidTwiceForTheSameDays) {
    // 打坐的那几天按静坐全额结过了，日课不许再按同样的日子结一遍。
    GameState& state = app_.state();
    state = GameState{};
    state.realm = Realm::Mortal;
    state.aptitude = 50;
    state.day = 1;

    CultivationScene scene;
    scene.onEnter(app_);                    // 起课
    ASSERT_EQ(state.lastPracticeDay, 1);
    scene.meditateFor(app_, 100);
    EXPECT_EQ(state.lastPracticeDay, 101) << "水位没跟着打坐走，这一百天会被日课再算一遍";

    const int afterSitting = state.cultivation;
    CultivationScene again;
    again.onEnter(app_);
    EXPECT_EQ(state.cultivation, afterSitting) << "同样的日子被结了两遍";
}

TEST(DailyPractice, SurvivesASaveRoundTrip) {
    // 水位不落盘的话，读档之后面板把它拨到当天，存档与上次开面板之间那段
    // 日子的功课就白过了——而那正是剧情推时间最多的一段。
    GameState before;
    before.day = 777;
    before.lastPracticeDay = 321;

    namespace fs = std::filesystem;
    // 目录名带上单调时钟与计数器：临时目录是全机共用的，固定名字会在两个测试
    // 进程同时跑的时候互相踩（并行构建下已经踩过一次）。同 IoTests 的 TempDir。
    static std::atomic<std::uint64_t> counter{0};
    const auto stamp =
        static_cast<std::uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count());
    const fs::path dir = fs::temp_directory_path() /
                         ("fanren_panel_test_" + std::to_string(stamp) + "_" +
                          std::to_string(counter.fetch_add(1)));
    fs::create_directories(dir);
    const fs::path path = dir / "practice.json";
    ASSERT_TRUE(fanren::io::saveGame(before, path.string()));
    const auto loaded = fanren::io::loadGame(path.string());
    ASSERT_TRUE(loaded) << loaded.error;
    EXPECT_EQ(loaded.value.lastPracticeDay, 321);

    std::error_code ec;
    fs::remove_all(dir, ec);
}

}  // namespace
