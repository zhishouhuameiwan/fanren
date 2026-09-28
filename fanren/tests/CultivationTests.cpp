#include "core/rules/Bottle.h"
#include "core/rules/Calendar.h"
#include "core/rules/Cultivation.h"
#include "core/rules/Field.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace {

using fanren::rules::Bottle;
using fanren::rules::BreakthroughAttempt;
using fanren::rules::Calendar;
using fanren::rules::CultivationGain;
using fanren::rules::FieldSlot;
using fanren::rules::MatureResult;
using fanren::rules::Realm;
using fanren::rules::SpiritField;
using fanren::rules::herbMaxAgeForGrade;
using fanren::rules::herbPrice;

using fanren::rules::kBacklashLossPercent;
using fanren::rules::kBaseChargeDays;
using fanren::rules::kBottleCapacityPerTier;
using fanren::rules::kBreakthroughMaxChance;
using fanren::rules::kBreakthroughMinChance;
using fanren::rules::kDaysPerMonth;
using fanren::rules::kDaysPerYear;
using fanren::rules::kFailureLossPercent;
using fanren::rules::kMaxHerbAge;
using fanren::rules::kMaxMeditateDays;
using fanren::rules::kRipeAge;

// 一株普通灵草的基价，用作计价曲线的参照物。
constexpr int kHerbBase = 100;

// 用得到的绿液与年份上限，凑一个「已开窍」的小瓶。容量默认放到够用，
// 免得催熟相关的用例被凝液上限干扰——那是另一条规则的事。
[[nodiscard]] Bottle unlockedBottle(int drops, int lastChargeDay = 0, int capacity = 999) {
    Bottle bottle;
    bottle.owned = true;
    bottle.matureKnown = true;
    bottle.drops = drops;
    bottle.lastChargeDay = lastChargeDay;
    bottle.capacity = capacity;
    return bottle;
}

// 从 startAge 起一直往同一株上浇，直到催不动为止，返回实际用掉的滴数。
// 「浇到顶要几滴」是品阶阶梯唯一能被玩家感知的量，所以按规则层实算，不写死。
[[nodiscard]] int dropsToCeiling(int startAge, int maxAge) {
    Bottle bottle = unlockedBottle(/*drops=*/10000, /*lastChargeDay=*/0, /*capacity=*/10000);
    int age = startAge;
    for (int spent = 0; spent <= 64; ++spent) {
        const MatureResult result = matureHerb(bottle, age, maxAge);
        if (!result.ok) return spent;
        age = result.newAge;
    }
    ADD_FAILURE() << "催熟没有收敛：maxAge=" << maxAge << " 浇了 64 滴还没到顶";
    return -1;
}

[[nodiscard]] long long totalPrice(int basePrice, const std::vector<int>& ages) {
    long long total = 0;
    for (const int age : ages) total += herbPrice(basePrice, age);
    return total;
}

// 退化打法：认准第一株浇到底，浇不动了剩下的绿液就砸在手里。
[[nodiscard]] long long concentratedYield(int basePrice, int startAge, int herbCount, int drops,
                                          int ceiling) {
    std::vector<int> ages(static_cast<std::size_t>(herbCount), startAge);
    Bottle bottle = unlockedBottle(drops, /*lastChargeDay=*/0, /*capacity=*/drops);
    while (bottle.drops > 0) {
        const MatureResult result = matureHerb(bottle, ages[0], ceiling);
        if (!result.ok) break;
        ages[0] = result.newAge;
    }
    return totalPrice(basePrice, ages);
}

// 分散打法：一轮浇一株，轮着来；浇不动的跳过，全都浇不动就收手。
[[nodiscard]] long long spreadYield(int basePrice, int startAge, int herbCount, int drops,
                                    int ceiling) {
    std::vector<int> ages(static_cast<std::size_t>(herbCount), startAge);
    Bottle bottle = unlockedBottle(drops, /*lastChargeDay=*/0, /*capacity=*/drops);
    bool progressed = true;
    while (bottle.drops > 0 && progressed) {
        progressed = false;
        for (int& age : ages) {
            if (bottle.drops <= 0) break;
            const MatureResult result = matureHerb(bottle, age, ceiling);
            if (!result.ok) continue;
            age = result.newAge;
            progressed = true;
        }
    }
    return totalPrice(basePrice, ages);
}

// 在给定条件下扫出第一个满足 predicate 的种子。突破是单次判定，硬编码某个
// 「一定成功」的种子会在成功率微调后失效；扫描则永远指向当前实现下真实存在
// 的那条路径。
template <typename Predicate>
[[nodiscard]] bool findSeed(Realm realm, int cultivation, int aptitude, int pillBonus,
                            Predicate predicate, std::uint32_t& out) {
    for (std::uint32_t seed = 1; seed < 2000u; ++seed) {
        const BreakthroughAttempt attempt =
            attemptBreakthrough(realm, cultivation, aptitude, pillBonus, seed);
        if (predicate(attempt)) {
            out = seed;
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// 日历
// ---------------------------------------------------------------------------

TEST(CalendarDisplay, FirstDayIsTheFirstOfTheFirstMonth) {
    const Calendar calendar;
    EXPECT_EQ(calendar.day, 1);
    EXPECT_EQ(calendar.year(), 1);
    EXPECT_EQ(calendar.monthOfYear(), 1);
    EXPECT_EQ(calendar.dayOfMonth(), 1);
}

TEST(CalendarDisplay, CrossesIntoTheNextMonth) {
    Calendar calendar{kDaysPerMonth};
    EXPECT_EQ(calendar.monthOfYear(), 1);
    EXPECT_EQ(calendar.dayOfMonth(), kDaysPerMonth);

    calendar.day = kDaysPerMonth + 1;
    EXPECT_EQ(calendar.year(), 1);
    EXPECT_EQ(calendar.monthOfYear(), 2);
    EXPECT_EQ(calendar.dayOfMonth(), 1);
}

TEST(CalendarDisplay, CrossesIntoTheNextYear) {
    Calendar calendar{kDaysPerYear};
    EXPECT_EQ(calendar.year(), 1);
    EXPECT_EQ(calendar.monthOfYear(), 12);
    EXPECT_EQ(calendar.dayOfMonth(), kDaysPerMonth);

    calendar.day = kDaysPerYear + 1;
    EXPECT_EQ(calendar.year(), 2);
    EXPECT_EQ(calendar.monthOfYear(), 1);
    EXPECT_EQ(calendar.dayOfMonth(), 1);

    // 第 2 章的「绿瓶四年」跨度。
    calendar.day = kDaysPerYear * 4 + 1;
    EXPECT_EQ(calendar.year(), 5);
}

TEST(CalendarDisplay, NormalisesACorruptedDay) {
    // 存档被改或字段忘了初始化时，绝不能出现「第 0 年」。
    Calendar zero{0};
    EXPECT_EQ(zero.year(), 1);
    EXPECT_EQ(zero.monthOfYear(), 1);
    EXPECT_EQ(zero.dayOfMonth(), 1);

    Calendar negative{-500};
    EXPECT_EQ(negative.year(), 1);
    EXPECT_EQ(negative.monthOfYear(), 1);
    EXPECT_EQ(negative.dayOfMonth(), 1);
}

TEST(CalendarAdvance, ReturnsDaysCrossedAndAccumulates) {
    Calendar calendar;
    EXPECT_EQ(advanceDays(calendar, 29), 29);
    EXPECT_EQ(calendar.day, 30);

    EXPECT_EQ(advanceDays(calendar, 1), 1);
    EXPECT_EQ(calendar.monthOfYear(), 2);

    EXPECT_EQ(advanceDays(calendar, kDaysPerYear), kDaysPerYear);
    EXPECT_EQ(calendar.year(), 2);
}

TEST(CalendarAdvance, ZeroDaysIsANoOp) {
    Calendar calendar{100};
    EXPECT_EQ(advanceDays(calendar, 0), 0);
    EXPECT_EQ(calendar.day, 100);
}

TEST(CalendarAdvance, NegativeDaysIsANoOpNotARewind) {
    // 时间倒退会把所有「上次结算日」推到未来，周期玩法集体停摆。
    Calendar calendar{100};
    EXPECT_EQ(advanceDays(calendar, -1), 0);
    EXPECT_EQ(calendar.day, 100);

    EXPECT_EQ(advanceDays(calendar, -100000), 0);
    EXPECT_EQ(calendar.day, 100);
}

TEST(CalendarAdvance, ClampsInsteadOfOverflowing) {
    Calendar calendar{std::numeric_limits<int>::max() - 3};
    EXPECT_EQ(advanceDays(calendar, 10), 3);
    EXPECT_EQ(calendar.day, std::numeric_limits<int>::max());
    EXPECT_GT(calendar.year(), 0);   // 溢出成负数的话年份会翻车
}

// ---------------------------------------------------------------------------
// 打坐
// ---------------------------------------------------------------------------

TEST(Meditate, ProducesPositiveGainOverAYear) {
    const CultivationGain gain = meditate(Realm::QiRefining1, 50, 100, kDaysPerYear, 1u);
    EXPECT_EQ(gain.days, kDaysPerYear);
    EXPECT_GT(gain.cultivation, 0);
}

TEST(Meditate, ZeroOrNegativeEffectivenessStillBurnsTheDays) {
    // 洞府无灵气 / 功法不契：坐了也白坐，但时间是真的过去了。
    const CultivationGain dead = meditate(Realm::QiRefining5, 60, 0, 200, 3u);
    EXPECT_EQ(dead.cultivation, 0);
    EXPECT_EQ(dead.days, 200);
    EXPECT_FALSE(dead.insight);

    const CultivationGain worse = meditate(Realm::QiRefining5, 60, -50, 200, 3u);
    EXPECT_EQ(worse.cultivation, 0);
    EXPECT_EQ(worse.days, 200);
    EXPECT_FALSE(worse.insight);
}

TEST(Meditate, NonPositiveDaysYieldNothingAtAll) {
    for (const int days : {0, -1, -10000}) {
        const CultivationGain gain = meditate(Realm::FoundationEarly, 80, 200, days, 5u);
        EXPECT_EQ(gain.cultivation, 0);
        EXPECT_EQ(gain.days, 0);
        EXPECT_FALSE(gain.insight);
    }
}

TEST(Meditate, ClampsASingleSessionToTenYears) {
    // 第 11 章的六十年闭关必须由调用方分段，好给剧情留打断点。
    const CultivationGain gain = meditate(Realm::CoreEarly, 50, 100, 100000, 7u);
    EXPECT_EQ(gain.days, kMaxMeditateDays);
    EXPECT_GT(gain.cultivation, 0);
}

TEST(Meditate, RejectsAnInvalidRealm) {
    const CultivationGain gain =
        meditate(fanren::rules::fromValue(24), 50, 100, 360, 9u);
    EXPECT_EQ(gain.cultivation, 0);
    EXPECT_EQ(gain.days, 0);
}

TEST(Meditate, IsReproducibleForTheSameSeed) {
    const CultivationGain first = meditate(Realm::QiRefining3, 45, 120, 720, 20260920u);
    const CultivationGain second = meditate(Realm::QiRefining3, 45, 120, 720, 20260920u);
    EXPECT_EQ(first.cultivation, second.cultivation);
    EXPECT_EQ(first.days, second.days);
    EXPECT_EQ(first.insight, second.insight);
}

TEST(Meditate, BetterAptitudeNeverEarnsLess) {
    int previous = -1;
    for (const int aptitude : {0, 25, 50, 75, 100}) {
        const CultivationGain gain = meditate(Realm::QiRefining9, aptitude, 100, 360, 11u);
        EXPECT_GE(gain.cultivation, previous);
        previous = gain.cultivation;
    }
    EXPECT_GT(previous, 0);
}

TEST(Meditate, InsightIsRareReproducibleAndPaysExtra) {
    // 顿悟是低概率事件，硬编码种子会在概率微调后失效，因此扫描出来再用。
    std::uint32_t lucky = 0;
    std::uint32_t plain = 0;
    for (std::uint32_t seed = 1; seed < 400u && (lucky == 0 || plain == 0); ++seed) {
        const CultivationGain gain = meditate(Realm::QiRefining1, 50, 100, 120, seed);
        if (gain.insight && lucky == 0) lucky = seed;
        if (!gain.insight && plain == 0) plain = seed;
    }
    ASSERT_NE(lucky, 0u) << "顿悟概率过低，玩家一辈子都撞不上";
    ASSERT_NE(plain, 0u) << "顿悟已成必然事件，不再是低概率额外收益";

    const CultivationGain hit = meditate(Realm::QiRefining1, 50, 100, 120, lucky);
    const CultivationGain again = meditate(Realm::QiRefining1, 50, 100, 120, lucky);
    EXPECT_TRUE(hit.insight);
    EXPECT_EQ(hit.cultivation, again.cultivation);
    EXPECT_EQ(hit.insight, again.insight);

    const CultivationGain miss = meditate(Realm::QiRefining1, 50, 100, 120, plain);
    EXPECT_FALSE(miss.insight);
    EXPECT_GT(hit.cultivation, miss.cultivation) << "顿悟必须给得出看得见的数字";
}

TEST(Meditate, HigherRealmsTakeLongerToFillDespiteFasterDays) {
    // 单日收益随境界上升，但门槛涨得更快：净效果是越往上越慢。
    const auto yearsToNext = [](Realm realm) {
        const CultivationGain gain = meditate(realm, 50, 100, kDaysPerYear, 31u);
        EXPECT_GT(gain.cultivation, 0);
        return static_cast<double>(cultivationNeeded(realm)) / gain.cultivation;
    };

    const double qi = yearsToNext(Realm::QiRefining1);
    const double foundation = yearsToNext(Realm::FoundationEarly);
    const double core = yearsToNext(Realm::CoreEarly);
    EXPECT_LT(qi, foundation);
    EXPECT_LT(foundation, core);
}

// ---------------------------------------------------------------------------
// 突破
// ---------------------------------------------------------------------------

TEST(BreakthroughChance, IsZeroBelowTheThreshold) {
    const int need = cultivationNeeded(Realm::QiRefining13);
    ASSERT_GT(need, 0);
    EXPECT_EQ(breakthroughChance(Realm::QiRefining13, need - 1, 100, 100), 0);
    EXPECT_GT(breakthroughChance(Realm::QiRefining13, need, 100, 100), 0);
}

TEST(BreakthroughChance, IsZeroAtTheSeriesCap) {
    // 结丹后期是本作上限（大纲 4.1 硬约束）。
    EXPECT_EQ(breakthroughChance(Realm::CoreLate, 999999, 100, 100), 0);
    EXPECT_EQ(breakthroughChance(fanren::rules::fromValue(24), 999999, 100, 100), 0);
}

TEST(BreakthroughChance, RisesWithPillsAndAptitudeAndSpareCultivation) {
    const int need = cultivationNeeded(Realm::QiRefining13);
    const int bare = breakthroughChance(Realm::QiRefining13, need, 40, 0);
    EXPECT_GT(breakthroughChance(Realm::QiRefining13, need, 40, 30), bare);
    EXPECT_GT(breakthroughChance(Realm::QiRefining13, need, 90, 0), bare);
    EXPECT_GT(breakthroughChance(Realm::QiRefining13, need * 2, 40, 0), bare);
}

TEST(BreakthroughChance, StaysInsideTheFivePercentWindow) {
    // 永远留 5% 失手与 5% 侥幸。
    const int floorCase =
        breakthroughChance(Realm::CoreMid, cultivationNeeded(Realm::CoreMid), 0, -100);
    EXPECT_EQ(floorCase, kBreakthroughMinChance);

    const int cap = breakthroughChance(Realm::QiRefining1, 100000, 100, 100);
    EXPECT_EQ(cap, kBreakthroughMaxChance);
}

TEST(BreakthroughChance, BigGatesAreHarderThanSmallSteps) {
    // 筑基、结丹两道关口必须明显比同档升层难，否则这两件事没有分量。
    const int step = breakthroughChance(Realm::QiRefining5, cultivationNeeded(Realm::QiRefining5), 50, 0);
    const int toFoundation =
        breakthroughChance(Realm::QiRefining13, cultivationNeeded(Realm::QiRefining13), 50, 0);
    const int toCore =
        breakthroughChance(Realm::FoundationLate, cultivationNeeded(Realm::FoundationLate), 50, 0);
    EXPECT_GT(step, toFoundation);
    EXPECT_GT(toFoundation, toCore);
}

TEST(Breakthrough, SucceedsOnAFavourableSeed) {
    const int need = cultivationNeeded(Realm::QiRefining13);
    std::uint32_t seed = 0;
    ASSERT_TRUE(findSeed(Realm::QiRefining13, need, 40, 0,
                         [](const BreakthroughAttempt& a) { return a.success; }, seed));

    const BreakthroughAttempt attempt = attemptBreakthrough(Realm::QiRefining13, need, 40, 0, seed);
    EXPECT_TRUE(attempt.success);
    EXPECT_EQ(attempt.cultivationSpent, need);
    EXPECT_EQ(attempt.cultivationLost, 0);
    EXPECT_FALSE(attempt.backlash);
}

TEST(Breakthrough, FailureAlwaysCostsCultivation) {
    const int need = cultivationNeeded(Realm::QiRefining13);
    std::uint32_t seed = 0;
    ASSERT_TRUE(findSeed(Realm::QiRefining13, need, 40, 0,
                         [](const BreakthroughAttempt& a) { return !a.success && !a.backlash; },
                         seed));

    const BreakthroughAttempt attempt = attemptBreakthrough(Realm::QiRefining13, need, 40, 0, seed);
    EXPECT_FALSE(attempt.success);
    EXPECT_EQ(attempt.cultivationSpent, 0);
    EXPECT_EQ(attempt.cultivationLost, need * kFailureLossPercent / 100);
    EXPECT_GT(attempt.cultivationLost, 0) << "没有代价的突破会被无脑重刷";
}

TEST(Breakthrough, BacklashCostsMoreThanAPlainFailure) {
    const int need = cultivationNeeded(Realm::FoundationLate);
    std::uint32_t plain = 0;
    std::uint32_t burnt = 0;
    ASSERT_TRUE(findSeed(Realm::FoundationLate, need, 40, 0,
                         [](const BreakthroughAttempt& a) { return !a.success && !a.backlash; },
                         plain));
    ASSERT_TRUE(findSeed(Realm::FoundationLate, need, 40, 0,
                         [](const BreakthroughAttempt& a) { return a.backlash; }, burnt));

    const BreakthroughAttempt mild = attemptBreakthrough(Realm::FoundationLate, need, 40, 0, plain);
    const BreakthroughAttempt harsh = attemptBreakthrough(Realm::FoundationLate, need, 40, 0, burnt);
    EXPECT_FALSE(harsh.success);
    EXPECT_EQ(harsh.cultivationLost, need * kBacklashLossPercent / 100);
    EXPECT_GT(harsh.cultivationLost, mild.cultivationLost);
}

TEST(Breakthrough, LossNeverExceedsWhatThePlayerHas) {
    // 修为刚好卡在门槛上时倒扣不能把它扣成负数。
    const int need = cultivationNeeded(Realm::QiRefining1);
    for (std::uint32_t seed = 1; seed < 60u; ++seed) {
        const BreakthroughAttempt attempt = attemptBreakthrough(Realm::QiRefining1, need, 0, -100, seed);
        EXPECT_LE(attempt.cultivationLost, need);
        EXPECT_GE(attempt.cultivationLost, 0);
    }
}

TEST(Breakthrough, InsufficientCultivationCostsNothing) {
    const int need = cultivationNeeded(Realm::FoundationEarly);
    const BreakthroughAttempt attempt =
        attemptBreakthrough(Realm::FoundationEarly, need - 1, 100, 100, 42u);
    EXPECT_FALSE(attempt.success);
    EXPECT_EQ(attempt.cultivationSpent, 0);
    EXPECT_EQ(attempt.cultivationLost, 0);
    EXPECT_FALSE(attempt.backlash);
}

TEST(Breakthrough, AtTheSeriesCapDoesNothing) {
    const BreakthroughAttempt attempt = attemptBreakthrough(Realm::CoreLate, 999999, 100, 100, 42u);
    EXPECT_FALSE(attempt.success);
    EXPECT_EQ(attempt.cultivationSpent, 0);
    EXPECT_EQ(attempt.cultivationLost, 0);
    EXPECT_FALSE(attempt.backlash);

    const BreakthroughAttempt bogus =
        attemptBreakthrough(fanren::rules::fromValue(20), 999999, 100, 100, 42u);
    EXPECT_FALSE(bogus.success);
    EXPECT_EQ(bogus.cultivationLost, 0);
}

TEST(Breakthrough, PillsNeverTurnASuccessIntoAFailure) {
    // 同一种子下嗑药只可能把失败翻成成功。「吃了丹药反而炸炉」是玩家最
    // 不能接受的随机，因此成功判定取的是第一次骰子。
    const int need = cultivationNeeded(Realm::QiRefining13);
    for (std::uint32_t seed = 1; seed < 400u; ++seed) {
        const BreakthroughAttempt bare = attemptBreakthrough(Realm::QiRefining13, need, 40, 0, seed);
        const BreakthroughAttempt dosed = attemptBreakthrough(Realm::QiRefining13, need, 40, 30, seed);
        if (bare.success) EXPECT_TRUE(dosed.success) << "seed=" << seed;
    }
}

TEST(Breakthrough, PillsRaiseTheObservedSuccessRate) {
    const int need = cultivationNeeded(Realm::QiRefining13);
    int bare = 0;
    int dosed = 0;
    for (std::uint32_t seed = 1; seed < 400u; ++seed) {
        if (attemptBreakthrough(Realm::QiRefining13, need, 40, 0, seed).success) ++bare;
        if (attemptBreakthrough(Realm::QiRefining13, need, 40, 30, seed).success) ++dosed;
    }
    EXPECT_GT(dosed, bare) << "丹药没有换来任何东西，没人会去炼它";
}

TEST(Breakthrough, IsReproducibleForTheSameSeed) {
    const int need = cultivationNeeded(Realm::FoundationLate);
    for (const std::uint32_t seed : {1u, 77u, 20260920u}) {
        const BreakthroughAttempt first = attemptBreakthrough(Realm::FoundationLate, need, 55, 20, seed);
        const BreakthroughAttempt second = attemptBreakthrough(Realm::FoundationLate, need, 55, 20, seed);
        EXPECT_EQ(first.success, second.success);
        EXPECT_EQ(first.cultivationSpent, second.cultivationSpent);
        EXPECT_EQ(first.cultivationLost, second.cultivationLost);
        EXPECT_EQ(first.backlash, second.backlash);
    }
}

// ---------------------------------------------------------------------------
// 掌天瓶：凝液
// ---------------------------------------------------------------------------

TEST(BottleRefill, GivesTheFirstDropOnTheEighthDay) {
    // 原著：砸瓶之后第八日瓶盖方开，内有一滴绿液（ch10-14）。
    Bottle bottle;
    bottle.owned = true;
    bottle.lastChargeDay = 1;

    EXPECT_EQ(refill(bottle, 7, kBaseChargeDays), 0);
    EXPECT_EQ(bottle.drops, 0);
    EXPECT_EQ(refill(bottle, 8, kBaseChargeDays), 1);
    EXPECT_EQ(bottle.drops, 1);
}

TEST(BottleRefill, CatchesUpAcrossManyCyclesAtOnce) {
    // 闭关四年出关，账要一次算清（容量放宽，本例只验补账不验上限）。
    Bottle bottle = unlockedBottle(0);
    EXPECT_EQ(refill(bottle, 30, 7), 4);
    EXPECT_EQ(bottle.drops, 4);
    EXPECT_EQ(bottle.lastChargeDay, 28);
}

TEST(BottleRefill, KeepsPartialProgressAcrossFrequentCalls) {
    // 每天点开一次瓶子界面，不能把未满周期的进度清零。
    Bottle daily = unlockedBottle(0);
    int total = 0;
    for (int day = 1; day <= 70; ++day) total += refill(daily, day, 7);
    EXPECT_EQ(total, 10);
    EXPECT_EQ(daily.drops, 10);

    Bottle lazy = unlockedBottle(0);
    EXPECT_EQ(refill(lazy, 70, 7), 10);
    EXPECT_EQ(lazy.drops, daily.drops) << "结算频率不该影响总产出";
}

TEST(BottleRefill, HigherRealmChargesFaster) {
    Bottle slow = unlockedBottle(0);
    Bottle fast = unlockedBottle(0);

    const int slowDrops = refill(slow, 90, kBaseChargeDays);
    const int fastDrops = refill(fast, 90, 3);   // 境界上去之后 chargeDays 变小
    EXPECT_GT(fastDrops, slowDrops);
    EXPECT_EQ(fast.drops, fastDrops);
}

TEST(BottleRefill, StopsAtCapacity) {
    Bottle bottle = unlockedBottle(0, 0, kBottleCapacityPerTier);

    // 挂机一百年也只有炼气期那三滴。没有这道闸，此处会是五千余滴，
    // 乘上 herbPrice 的 121 倍曲线就是无技巧的无限刷钱。
    EXPECT_EQ(refill(bottle, kDaysPerYear * 100, kBaseChargeDays), kBottleCapacityPerTier);
    EXPECT_EQ(bottle.drops, kBottleCapacityPerTier);

    // 满了之后继续推进日历，一滴不增。
    EXPECT_EQ(refill(bottle, kDaysPerYear * 101, kBaseChargeDays), 0);
    EXPECT_EQ(bottle.drops, kBottleCapacityPerTier);

    // 容量随境界成长由调用方设置，rules 层不写死。
    bottle.capacity = kBottleCapacityPerTier * 2;
    EXPECT_GT(refill(bottle, kDaysPerYear * 102, kBaseChargeDays), 0);
    EXPECT_EQ(bottle.drops, kBottleCapacityPerTier * 2);
}

TEST(BottleRefill, DoesNotBackpayTheDaysSpentFull) {
    // 这条是上限真正的判据：满瓶期间跨过的天数不能以欠账形式留着，等玩家
    // 腾出空位再一次性追认。只夹住 drops 而不拨计时，等于没堵。
    Bottle bottle = unlockedBottle(0, 0, kBottleCapacityPerTier);
    ASSERT_EQ(refill(bottle, kDaysPerYear * 100, kBaseChargeDays), kBottleCapacityPerTier);

    const int idleDay = kDaysPerYear * 101;
    ASSERT_EQ(refill(bottle, idleDay, kBaseChargeDays), 0);

    // 用掉一滴：此时账面上「欠」着一百年，绝不能立刻补回来。
    --bottle.drops;
    EXPECT_EQ(refill(bottle, idleDay, kBaseChargeDays), 0);
    EXPECT_EQ(refill(bottle, idleDay + kBaseChargeDays - 1, kBaseChargeDays), 0);
    EXPECT_EQ(bottle.drops, kBottleCapacityPerTier - 1) << "满瓶那一百年被追认成了绿液";

    // 老老实实重等满一个周期，才有第三滴。
    EXPECT_EQ(refill(bottle, idleDay + kBaseChargeDays, kBaseChargeDays), 1);
    EXPECT_EQ(bottle.drops, kBottleCapacityPerTier);
}

TEST(BottleRefill, ANonPositiveCapacityNeverFills) {
    // 存档未初始化或 game 层忘了按境界赋值时，宁可一滴不出，也不要因为
    // 负容量让上限判断反向。
    for (const int capacity : {0, -5}) {
        Bottle bottle = unlockedBottle(0, 0, capacity);
        EXPECT_EQ(refill(bottle, kDaysPerYear * 10, kBaseChargeDays), 0);
        EXPECT_EQ(bottle.drops, 0);
    }
}

TEST(BottleRefill, DoesNothingWithoutTheBottle) {
    Bottle none;
    EXPECT_EQ(refill(none, 1000, kBaseChargeDays), 0);
    EXPECT_EQ(none.drops, 0);
    EXPECT_EQ(none.lastChargeDay, 0);
}

TEST(BottleRefill, RejectsBadChargeDaysAndBackwardsTime) {
    Bottle bottle = unlockedBottle(2, 100);
    EXPECT_EQ(refill(bottle, 200, 0), 0);
    EXPECT_EQ(refill(bottle, 200, -7), 0);
    EXPECT_EQ(refill(bottle, 50, kBaseChargeDays), 0);   // 时间倒流
    EXPECT_EQ(bottle.drops, 2);
    EXPECT_EQ(bottle.lastChargeDay, 100);
}

TEST(BottleRefill, RunsThroughTheFourYearsBeforeTheSecretIsKnown) {
    // 「绿瓶四年」：拾瓶与发现催熟之能相隔四年，其间绿液照凝，玩家只是
    // 不知道它能干什么。这两个解锁节点不可合并。
    Bottle bottle;
    bottle.owned = true;
    bottle.matureKnown = false;
    bottle.capacity = kBottleCapacityPerTier;

    const int gained = refill(bottle, kDaysPerYear * 4, kBaseChargeDays);
    EXPECT_GT(gained, 0);
    EXPECT_EQ(bottle.drops, gained);

    const MatureResult blocked = matureHerb(bottle, 10, 1000);
    EXPECT_FALSE(blocked.ok);
    EXPECT_EQ(bottle.drops, gained) << "不会用的时候不该消耗绿液";
}

// ---------------------------------------------------------------------------
// 掌天瓶：催熟
// ---------------------------------------------------------------------------

TEST(MatureHerb, FailsWithoutTheBottleAndSaysWhy) {
    Bottle none;
    none.drops = 5;   // 就算凭空有绿液，没瓶子也无从谈起
    const MatureResult result = matureHerb(none, 10, 1000);
    EXPECT_FALSE(result.ok);
    EXPECT_FALSE(result.reason.empty());
    EXPECT_NE(result.reason.find("小瓶"), std::string::npos);
    EXPECT_EQ(result.newAge, 10);
    EXPECT_EQ(none.drops, 5);
}

TEST(MatureHerb, FailsWhileTheSecretIsUnknownAndSaysWhy) {
    Bottle bottle;
    bottle.owned = true;
    bottle.matureKnown = false;
    bottle.drops = 3;

    const MatureResult result = matureHerb(bottle, 10, 1000);
    EXPECT_FALSE(result.ok);
    EXPECT_NE(result.reason.find("催熟"), std::string::npos);
    EXPECT_EQ(bottle.drops, 3);
}

TEST(MatureHerb, FailsWithoutDropsAndSaysWhy) {
    Bottle bottle = unlockedBottle(0);
    const MatureResult result = matureHerb(bottle, 10, 1000);
    EXPECT_FALSE(result.ok);
    EXPECT_NE(result.reason.find("绿液"), std::string::npos);
    EXPECT_EQ(result.newAge, 10);
}

TEST(MatureHerb, JumpsTheAgeAndSpendsExactlyOneDrop) {
    Bottle bottle = unlockedBottle(3);
    const MatureResult result = matureHerb(bottle, 10, 1000);
    EXPECT_TRUE(result.ok);
    EXPECT_TRUE(result.reason.empty());
    EXPECT_EQ(result.newAge, 20);
    EXPECT_EQ(bottle.drops, 2);
}

TEST(MatureHerb, LiftsAJustRipeHerbByTheFloorAmount) {
    // 保底 10 年管的是**刚足年**那一档：1 年浇一滴得 11 年，不是 2 年。
    // 没有它，足年的苗浇一滴只多一岁，玩家会攒着绿液永远不用。
    Bottle bottle = unlockedBottle(1);
    const MatureResult result = matureHerb(bottle, fanren::rules::kRipeAge, 1000);
    EXPECT_TRUE(result.ok);
    EXPECT_EQ(result.newAge, fanren::rules::kRipeAge + fanren::rules::kMatureMinYears);
}

TEST(MatureHerb, RefusesASeedlingThatIsNotYetAYearOldAndSaysWhy) {
    // 这条是裁决，不是口味：允许对刚下种的苗浇绿液的话，
    //     种下就浇  0 → 10 → 20 → 40 → 44   4 滴 = 28 天，一天不用等
    //     等足年再浇 1 → 11 → 22 → 44       3 滴 = 21 天，但先要等 360 天
    // 多花一滴省掉整整一年，「等足年」被严格支配，灵田的槽位数与生长时间
    // 一起失去意义，第 2 章章末那场算账也就不成立了。
    Bottle bottle = unlockedBottle(3);
    const MatureResult result = matureHerb(bottle, 0, 1000);
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.failure, fanren::rules::MatureFailure::NotRipe);
    EXPECT_NE(result.reason.find("足年"), std::string::npos) << result.reason;
    EXPECT_EQ(result.newAge, 0) << "催不动就不该改年份";
    EXPECT_EQ(bottle.drops, 3) << "催不动就不该扣绿液";
}

TEST(MatureHerb, TheRipeLineIsExactlyTheOneHarvestUses) {
    // 同一株药在采收与催熟上必须给同一条线，否则玩家会撞上「能采不能催」
    // 或者反过来，而两种说法都出自同一块地。
    Bottle bottle = unlockedBottle(/*drops=*/99, /*lastChargeDay=*/0, /*capacity=*/99);
    for (int age = 0; age < fanren::rules::kRipeAge; ++age) {
        EXPECT_EQ(matureHerb(bottle, age, 1000).failure, fanren::rules::MatureFailure::NotRipe)
            << "age=" << age << " 还不足年，却催得动";
    }
    EXPECT_TRUE(matureHerb(bottle, fanren::rules::kRipeAge, 1000).ok)
        << "刚足年就该催得动——线画在别处了";
}

TEST(MatureHerb, ClampsToTheCeilingButStillSpendsTheDrop) {
    Bottle bottle = unlockedBottle(1);
    const MatureResult result = matureHerb(bottle, 600, 1000);
    EXPECT_TRUE(result.ok);
    EXPECT_EQ(result.newAge, 1000);
    EXPECT_EQ(bottle.drops, 0);
}

TEST(MatureHerb, RefusesWhenAlreadyAtTheCeiling) {
    Bottle bottle = unlockedBottle(4);
    const MatureResult result = matureHerb(bottle, 1000, 1000);
    EXPECT_FALSE(result.ok);
    EXPECT_NE(result.reason.find("极限"), std::string::npos);
    EXPECT_EQ(result.newAge, 1000);
    EXPECT_EQ(bottle.drops, 4) << "催不动就不该扣绿液";
}

TEST(MatureHerb, EveryFailureCarriesItsOwnMachineCode) {
    // 五条失败路径除了中文 reason 还各带一个机器码：调用方（脚本、面板）要按
    // 原因分支，比对中文串既脆又等于把 UI 文案钉进逻辑里。
    // 逐条比对而不是只查「非 None」：全都回填成同一个码的实现也能骗过后者。
    Bottle none;
    none.drops = 5;
    EXPECT_EQ(matureHerb(none, 10, 1000).failure, fanren::rules::MatureFailure::NoBottle);

    Bottle locked;
    locked.owned = true;
    locked.drops = 3;
    EXPECT_EQ(matureHerb(locked, 10, 1000).failure, fanren::rules::MatureFailure::MatureUnknown);

    Bottle empty = unlockedBottle(0);
    EXPECT_EQ(matureHerb(empty, 10, 1000).failure, fanren::rules::MatureFailure::NoDrops);

    Bottle green = unlockedBottle(4);
    EXPECT_EQ(matureHerb(green, 0, 1000).failure, fanren::rules::MatureFailure::NotRipe);

    Bottle topped = unlockedBottle(4);
    EXPECT_EQ(matureHerb(topped, 1000, 1000).failure, fanren::rules::MatureFailure::AtMaxAge);

    Bottle good = unlockedBottle(1);
    const MatureResult ok = matureHerb(good, 10, 1000);
    ASSERT_TRUE(ok.ok);
    EXPECT_EQ(ok.failure, fanren::rules::MatureFailure::None) << "成功时不该留着失败码";

    // 五条理由两两不同。合并成一句「不能催熟」等于把第 2 章的解锁过程抹平，
    // 而「合并」最常见的长相是有人复制粘贴时忘了改那一行中文。
    const std::vector<std::string> reasons{
        matureHerb(none, 10, 1000).reason,   matureHerb(locked, 10, 1000).reason,
        matureHerb(empty, 10, 1000).reason,  matureHerb(green, 0, 1000).reason,
        matureHerb(topped, 1000, 1000).reason,
    };
    for (std::size_t i = 0; i < reasons.size(); ++i) {
        EXPECT_FALSE(reasons[i].empty()) << "第 " << i << " 条没有理由";
        for (std::size_t j = i + 1; j < reasons.size(); ++j) {
            EXPECT_NE(reasons[i], reasons[j]) << "第 " << i << " 条与第 " << j << " 条重了";
        }
    }
}

// ---------------------------------------------------------------------------
// 掌天瓶：倒液（催熟之外的那一半消耗）
// ---------------------------------------------------------------------------

TEST(SpendDrops, TakesExactlyWhatWasAskedFor) {
    Bottle bottle = unlockedBottle(3);
    EXPECT_TRUE(fanren::rules::spendDrops(bottle, 2));
    EXPECT_EQ(bottle.drops, 1);
}

TEST(SpendDrops, TakesNothingWhenThereIsNotEnough) {
    // 扣一半再报失败的话，玩家的绿液会在一次失败的调用里凭空少掉，
    // 而这种账在存档里看不出来。
    Bottle bottle = unlockedBottle(1);
    EXPECT_FALSE(fanren::rules::spendDrops(bottle, 3));
    EXPECT_EQ(bottle.drops, 1);
}

TEST(SpendDrops, TreatsZeroAndNegativeAsNothingToDo) {
    Bottle bottle = unlockedBottle(2);
    EXPECT_TRUE(fanren::rules::spendDrops(bottle, 0));
    EXPECT_TRUE(fanren::rules::spendDrops(bottle, -5));
    EXPECT_EQ(bottle.drops, 2) << "日历那边传 0 也不算错误，这里口径一致";
}

TEST(SpendDrops, IsTheOnlyPlaceThatSubtracts) {
    // matureHerb 自己也走 spendDrops：两处各写一份减法，迟早有一处忘了先判
    // 够不够。这条从外部能观察到的部分是「催熟恰好扣一滴」，与上面那条
    // JumpsTheAgeAndSpendsExactlyOneDrop 一起把这个口径钉住。
    Bottle bottle = unlockedBottle(1);
    const MatureResult result = matureHerb(bottle, 10, 1000);
    ASSERT_TRUE(result.ok);
    EXPECT_EQ(bottle.drops, 0);
    EXPECT_FALSE(fanren::rules::spendDrops(bottle, 1)) << "空瓶再倒不出第二滴";
}

TEST(MatureHerb, IsFullyDeterministic) {
    // 催熟不掷骰子：同一状态跑两次必须一字不差，存档回放才对得上。
    Bottle first = unlockedBottle(2);
    Bottle second = unlockedBottle(2);
    const MatureResult a = matureHerb(first, 37, 900);
    const MatureResult b = matureHerb(second, 37, 900);
    EXPECT_EQ(a.ok, b.ok);
    EXPECT_EQ(a.newAge, b.newAge);
    EXPECT_EQ(a.reason, b.reason);
    EXPECT_EQ(first.drops, second.drops);
}

// ---------------------------------------------------------------------------
// 灵草年份上限：每一味药各有其顶
// ---------------------------------------------------------------------------

TEST(HerbMaxAge, RisesWithGradeAndStaysInsideTheGlobalGate) {
    // 阶梯要真的拉得开：品阶高一档，能长到的年份就该明显高一截，不然
    // 「换更好的种子」这条长线成长在数值上收不到回报。
    int previous = 0;
    for (int grade = 1; grade <= 8; ++grade) {
        const int ceiling = herbMaxAgeForGrade(grade);
        EXPECT_GT(ceiling, previous) << "grade=" << grade;
        EXPECT_LE(ceiling, kMaxHerbAge) << "grade=" << grade << " 越过了全局溢出闸";
        previous = ceiling;
    }
    // 顶档要够得着万年：本命法宝那根万年份天雷竹是八阶，它得能表达出来。
    EXPECT_EQ(herbMaxAgeForGrade(8), kMaxHerbAge);
    EXPECT_EQ(herbMaxAgeForGrade(99), kMaxHerbAge) << "表尾之外的品阶按表尾算";
}

TEST(HerbMaxAge, TreatsAMissingGradeAsTheLowestTierInsteadOfNoLimit) {
    // 老数据没写 grade 时 item.grade 是 0。这里退到最低一档，而不是「不限」——
    // 漏一个字段就换来一株能刷到万年的药，正是这次要堵的洞。
    EXPECT_EQ(herbMaxAgeForGrade(0), herbMaxAgeForGrade(1));
    EXPECT_EQ(herbMaxAgeForGrade(-3), herbMaxAgeForGrade(1)) << "负品阶也不该反向变成无上限";
    EXPECT_LT(herbMaxAgeForGrade(0), kMaxHerbAge);
}

TEST(HerbMaxAge, AFirstTierHerbTopsOutInTwoOrThreeDrops) {
    // 第 2 章的药都是一阶：攒满炼气期的一瓶（3 滴）正好把一株推到顶，
    // 再多一滴也无用，于是决策回到「浇哪一株」。
    //
    // 这条同时是防线：谁把一阶的上限调回天上，这里立刻变红。
    const int firstTier = herbMaxAgeForGrade(1);
    const int fromRipe = dropsToCeiling(kRipeAge, firstTier);
    EXPECT_GE(fromRipe, 2) << "一滴就到顶的话，绿液攒起来一起用就没有意义了";
    EXPECT_LE(fromRipe, 3) << "一阶灵草的上限被抬高了：从足年浇到顶要 " << fromRipe
                           << " 滴，超出炼气期一瓶的量";
    EXPECT_EQ(fromRipe, kBottleCapacityPerTier)
        << "一阶上限与炼气期瓶容量是配套的（见 docs/ch02-design.md 第 3 节的实算表）";

    // 底子厚的药更快到顶——绿液是翻倍，不是加固定年数，这条不能反过来。
    EXPECT_LT(dropsToCeiling(firstTier / 2, firstTier), fromRipe);
    EXPECT_EQ(dropsToCeiling(firstTier, firstTier), 0) << "已经到顶的药一滴也浇不进去";
}

TEST(MatureHerb, RefusesOnceTheSpeciesCeilingIsReachedAndSaysWhy) {
    const int ceiling = herbMaxAgeForGrade(1);
    Bottle bottle = unlockedBottle(/*drops=*/11);

    int age = kRipeAge;
    while (true) {
        const MatureResult result = matureHerb(bottle, age, ceiling);
        if (!result.ok) break;
        age = result.newAge;
        ASSERT_LE(age, ceiling) << "催熟把年份推过了这一味药自己的顶";
    }
    EXPECT_EQ(age, ceiling);

    const int dropsLeft = bottle.drops;
    ASSERT_GT(dropsLeft, 0) << "这条用例要在「还有绿液」的前提下验，不然验的是另一条失败路径";

    const MatureResult blocked = matureHerb(bottle, age, ceiling);
    EXPECT_FALSE(blocked.ok);
    EXPECT_NE(blocked.reason.find("极限"), std::string::npos)
        << "到顶要说「年份已至极限」，说成缺绿液玩家会一直攒下去：" << blocked.reason;
    EXPECT_EQ(blocked.newAge, age);
    EXPECT_EQ(bottle.drops, dropsLeft) << "催不动就不该扣绿液";
}

// ---------------------------------------------------------------------------
// 灵草计价
// ---------------------------------------------------------------------------

TEST(HerbPrice, IsSuperlinearInAge) {
    // 整个掌天瓶经济的动力就在这一条上：百年灵草远不止十年的十倍。
    EXPECT_GT(herbPrice(kHerbBase, 100), herbPrice(kHerbBase, 10) * 10);
    EXPECT_GT(herbPrice(kHerbBase, 1000), herbPrice(kHerbBase, 100) * 10);
}

TEST(HerbPrice, MaturingBeatsSellingTheYoungHerb) {
    // 循环闭合的判据：一滴绿液换来的溢价必须显著高于直接出手。
    Bottle bottle = unlockedBottle(1);
    const int asIs = herbPrice(kHerbBase, 10);
    const MatureResult grown = matureHerb(bottle, 10, kMaxHerbAge);
    ASSERT_TRUE(grown.ok);
    EXPECT_GT(herbPrice(kHerbBase, grown.newAge), asIs * 2);
}

TEST(HerbPrice, IsMonotonicAndAnchoredAtBase) {
    EXPECT_EQ(herbPrice(kHerbBase, 0), kHerbBase);
    int previous = 0;
    for (const int age : {0, 1, 5, 10, 30, 60, 100, 300, 1000, 10000}) {
        const int price = herbPrice(kHerbBase, age);
        EXPECT_GT(price, previous) << "age=" << age;
        previous = price;
    }
}

TEST(HerbPrice, HandlesDegenerateInputs) {
    EXPECT_EQ(herbPrice(0, 500), 0);
    EXPECT_EQ(herbPrice(-50, 500), 0);
    EXPECT_EQ(herbPrice(kHerbBase, -20), kHerbBase);   // 年份夹到 0
    // 万年份 * 高基价不能翻成负数。
    EXPECT_GT(herbPrice(std::numeric_limits<int>::max(), kMaxHerbAge), 0);
    EXPECT_EQ(herbPrice(kHerbBase, kMaxHerbAge * 100), herbPrice(kHerbBase, kMaxHerbAge));
}

// ---------------------------------------------------------------------------
// 回归：绿液集中浇一株 vs 分散浇
// ---------------------------------------------------------------------------

// 一株一阶灵草（黄精基价 5，见 data/items/herbs/huangjing_cao.json）。
constexpr int kFirstTierHerbBase = 5;
// 手上同时有几株可浇的药。取 11 是照着当初那个漏洞的算例：炼气期 7 天一滴，
// 11 滴（约 77 天）就够把一株推到一万年。
constexpr int kHerbsOnHand = 11;

TEST(HerbEconomyRegression, TheControlStillReproducesTheDegenerateStrategy) {
    // 先证明下面那条回归测试抓得住东西：仍按全局上限（也就是修好之前 FieldScene
    // 传给 matureHerb 的那个数）结算的话，「全浇一株」的收益是分散浇的几万倍。
    //
    // 这条控制组用例不是摆设。若哪天规则层改成「无论传什么上限都封在品阶上」，
    // 它会先变红，提醒下面那条断言已经证明不了任何事——一条永远为真的回归测试
    // 比没有测试更坏。
    const long long concentrated =
        concentratedYield(kFirstTierHerbBase, kRipeAge, kHerbsOnHand, /*drops=*/11, kMaxHerbAge);
    const long long spread =
        spreadYield(kFirstTierHerbBase, kRipeAge, kHerbsOnHand, /*drops=*/11, kMaxHerbAge);

    EXPECT_GT(concentrated, spread * 1000)
        << "按全局上限算都不再出现退化收益，下面那条回归断言已经失去意义";
}

TEST(HerbEconomyRegression, ConcentratingEveryDropOnOneHerbNoLongerRunsAway) {
    // 本次修复的核心判据。
    //
    // 修好之前：11 滴全浇一株能把它推到一万年，售价八十余万倍，总收益是分散浇
    // 的两万倍上下——最优解因此退化成「永远全浇一株」，灵田上种什么、什么时候
    // 收都不再有意义，而且第 2 章刚开局就能把后面整个游戏的钱赚够。
    //
    // 修好之后：浇到这一味药自己的顶就浇不动了，多出来的绿液砸在手里，
    // 集中浇再也跑不赢分散浇。
    const int ceiling = herbMaxAgeForGrade(1);
    const auto concentrated = [&](int drops) {
        return concentratedYield(kFirstTierHerbBase, kRipeAge, kHerbsOnHand, drops, ceiling);
    };
    const auto spread = [&](int drops) {
        return spreadYield(kFirstTierHerbBase, kRipeAge, kHerbsOnHand, drops, ceiling);
    };

    // 只够一瓶（3 滴）时，集中浇仍然更划算——这是第 2 章要教给玩家的那一课
    // 「绿液要攒着一起用」，不能连它一起改没了。但溢价必须是有限的几倍。
    EXPECT_GT(concentrated(kBottleCapacityPerTier), spread(kBottleCapacityPerTier))
        << "攒满一瓶浇一株不再划算的话，第 2 章那三次遭遇就白教了";
    EXPECT_LE(concentrated(kBottleCapacityPerTier), spread(kBottleCapacityPerTier) * 3)
        << "一瓶的溢价涨成了数量级，上限形同虚设";

    // 绿液多到一株药远吃不下时（11 滴正是当初那个算例），集中浇彻底不占便宜：
    // 剩下的绿液砸在手里，还不如挪去浇第二株。
    for (const int drops : {11, 22}) {
        EXPECT_LE(concentrated(drops), spread(drops))
            << "drops=" << drops << "：集中 " << concentrated(drops) << " vs 分散 "
            << spread(drops);
    }

    // 更要紧的是这条趋势：绿液越多，集中浇的溢价越低（一株的顶是死的，分散
    // 浇却能一直换成钱）。修好之前恰好相反——绿液越多，集中浇越是压倒性。
    // 比值用交叉相乘比，避免整数除法把差别抹平。
    const std::vector<int> budgets = {kBottleCapacityPerTier, 6, 11, 22, 44};
    for (std::size_t i = 1; i < budgets.size(); ++i) {
        const long long prevC = concentrated(budgets[i - 1]);
        const long long prevS = spread(budgets[i - 1]);
        const long long curC = concentrated(budgets[i]);
        const long long curS = spread(budgets[i]);
        EXPECT_LE(curC * prevS, prevC * curS)
            << "绿液从 " << budgets[i - 1] << " 滴涨到 " << budgets[i]
            << " 滴，集中浇的溢价反而升了——上限没拦住流量";
    }
}

TEST(HerbEconomyRegression, ExtraDropsPouredOnOneHerbBuyNothing) {
    // 上一条是相对的（集中 vs 分散），这一条是绝对的：一株药能从绿液里换来的
    // 价值有个天花板，绿液再多也顶不上去。挂机攒液刷钱的口子堵死在这里。
    const int ceiling = herbMaxAgeForGrade(1);

    const long long oneBottle = concentratedYield(kFirstTierHerbBase, kRipeAge, kHerbsOnHand,
                                                  kBottleCapacityPerTier, ceiling);
    const long long hundredBottles = concentratedYield(
        kFirstTierHerbBase, kRipeAge, kHerbsOnHand, kBottleCapacityPerTier * 100, ceiling);
    EXPECT_EQ(oneBottle, hundredBottles) << "一百瓶绿液全砸一株上，比一瓶多换来了钱";

    // 同样的算式在修好之前是天壤之别，控制组同样要留着。
    const long long brokenHundredBottles = concentratedYield(
        kFirstTierHerbBase, kRipeAge, kHerbsOnHand, kBottleCapacityPerTier * 100, kMaxHerbAge);
    EXPECT_GT(brokenHundredBottles, hundredBottles * 1000) << "控制组失效，这条断言已无意义";
}

// ---------------------------------------------------------------------------
// 灵田
// ---------------------------------------------------------------------------

TEST(SpiritFieldPlanting, OccupiesTheSlotAndResetsItsAge) {
    SpiritField field{"herb_garden", std::vector<FieldSlot>(3)};
    EXPECT_TRUE(plant(field, 1, "huang_jing", 100));

    const FieldSlot& slot = field.slots[1];
    EXPECT_EQ(slot.seedId, "huang_jing");
    EXPECT_EQ(slot.plantedDay, 100);
    EXPECT_EQ(slot.age, 0);
    EXPECT_FALSE(slot.ripe);
}

TEST(SpiritFieldPlanting, RefusesBadSlotsAndOverplanting) {
    SpiritField field{"herb_garden", std::vector<FieldSlot>(2)};
    EXPECT_FALSE(plant(field, -1, "huang_jing", 1));
    EXPECT_FALSE(plant(field, 2, "huang_jing", 1));
    EXPECT_FALSE(plant(field, 0, "", 1));

    ASSERT_TRUE(plant(field, 0, "huang_jing", 1));
    // 覆盖会静默毁掉一株可能养了几十年的灵草。
    EXPECT_FALSE(plant(field, 0, "ling_zhi", 500));
    EXPECT_EQ(field.slots[0].seedId, "huang_jing");
}

TEST(SpiritFieldGrowth, AddsOneYearPerCycleAndRipensOnce) {
    SpiritField field{"herb_garden", std::vector<FieldSlot>(1)};
    ASSERT_TRUE(plant(field, 0, "huang_jing", 0));

    EXPECT_TRUE(growField(field, kDaysPerYear - 1, kDaysPerYear).empty());
    EXPECT_EQ(field.slots[0].age, 0);
    EXPECT_FALSE(field.slots[0].ripe);

    const std::vector<int> ripened = growField(field, kDaysPerYear, kDaysPerYear);
    ASSERT_EQ(ripened.size(), 1u);
    EXPECT_EQ(ripened[0], 0);
    EXPECT_EQ(field.slots[0].age, 1);
    EXPECT_TRUE(field.slots[0].ripe);

    // 已经报过的槽位不再重复上报，否则提示与音效天天重放。
    EXPECT_TRUE(growField(field, kDaysPerYear, kDaysPerYear).empty());
    const std::vector<int> later = growField(field, kDaysPerYear * 3, kDaysPerYear);
    EXPECT_TRUE(later.empty());
    EXPECT_EQ(field.slots[0].age, 3) << "熟了也要继续长年份，何时收由玩家决定";
}

TEST(SpiritFieldGrowth, LeavesEmptySlotsAlone) {
    SpiritField field{"herb_garden", std::vector<FieldSlot>(2)};
    ASSERT_TRUE(plant(field, 0, "huang_jing", 0));

    const std::vector<int> ripened = growField(field, kDaysPerYear * 5, kDaysPerYear);
    ASSERT_EQ(ripened.size(), 1u);
    EXPECT_EQ(ripened[0], 0);
    EXPECT_TRUE(field.slots[1].seedId.empty());
    EXPECT_EQ(field.slots[1].age, 0);
    EXPECT_FALSE(field.slots[1].ripe);
}

TEST(SpiritFieldGrowth, AdvancesEveryPlantedSlotTogether) {
    SpiritField field{"bai_yao_yuan", std::vector<FieldSlot>(4)};
    ASSERT_TRUE(plant(field, 0, "huang_jing", 0));
    ASSERT_TRUE(plant(field, 2, "ling_zhi", 0));
    ASSERT_TRUE(plant(field, 3, "xue_lian", kDaysPerYear));   // 晚一年下种

    const std::vector<int> ripened = growField(field, kDaysPerYear * 2, kDaysPerYear);
    ASSERT_EQ(ripened.size(), 3u);
    EXPECT_EQ(ripened[0], 0);
    EXPECT_EQ(ripened[1], 2);
    EXPECT_EQ(ripened[2], 3);
    EXPECT_EQ(field.slots[0].age, 2);
    EXPECT_EQ(field.slots[2].age, 2);
    EXPECT_EQ(field.slots[3].age, 1);
}

TEST(SpiritFieldGrowth, RejectsNonPositiveDaysPerYear) {
    SpiritField field{"herb_garden", std::vector<FieldSlot>(1)};
    ASSERT_TRUE(plant(field, 0, "huang_jing", 0));

    EXPECT_TRUE(growField(field, 100000, 0).empty());
    EXPECT_TRUE(growField(field, 100000, -30).empty());
    EXPECT_EQ(field.slots[0].age, 0) << "年份不许倒着长";
}

TEST(SpiritFieldGrowth, KeepsTheBoostFromTheGreenLiquid) {
    // 催熟加上去的年份不能在下一次生长结算时被抹平，否则瓶子等于没用。
    SpiritField field{"herb_garden", std::vector<FieldSlot>(1)};
    ASSERT_TRUE(plant(field, 0, "huang_jing", 0));
    ASSERT_EQ(growField(field, kDaysPerYear, kDaysPerYear).size(), 1u);
    ASSERT_EQ(field.slots[0].age, 1);

    Bottle bottle = unlockedBottle(1);
    const MatureResult boosted = matureHerb(bottle, field.slots[0].age, kMaxHerbAge);
    ASSERT_TRUE(boosted.ok);
    field.slots[0].age = boosted.newAge;
    ASSERT_EQ(field.slots[0].age, 11);

    EXPECT_TRUE(growField(field, kDaysPerYear * 2, kDaysPerYear).empty());
    EXPECT_EQ(field.slots[0].age, 12);
}

TEST(SpiritFieldGrowth, StopsAtTheSpeciesCeiling) {
    // 上限若只拦绿液，「种下去搁上几百年再收」会原样刷出同一份天价，只是慢些；
    // 而跳时在剧情里是免费的（第 2 章一章就跨四年）。所以自然生长同样要按种封顶。
    const int ceiling = herbMaxAgeForGrade(1);
    SpiritField field{"herb_garden", std::vector<FieldSlot>(1)};
    ASSERT_TRUE(plant(field, 0, "huang_jing", 0, ceiling));

    ASSERT_EQ(growField(field, kDaysPerYear, kDaysPerYear).size(), 1u);
    EXPECT_EQ(field.slots[0].age, 1);

    growField(field, kDaysPerYear * 500, kDaysPerYear);
    EXPECT_EQ(field.slots[0].age, ceiling) << "一株一阶灵草在地里躺五百年也成不了万年灵药";
    EXPECT_TRUE(field.slots[0].ripe) << "封顶不该把「已熟」也一起抹掉";

    // 再跨一千年也还是这个数：封顶之后 age 不再变。
    growField(field, kDaysPerYear * 1500, kDaysPerYear);
    EXPECT_EQ(field.slots[0].age, ceiling);
}

TEST(SpiritFieldGrowth, NeverTrimsAnAgeThatIsAlreadyPastTheCeiling) {
    // 剧情直接给的高年份灵草、以及这次修复之前存下的老档，年份可能本来就在
    // 上限之上。那就让它停着，不要往回削——玩家账面上的年份变小，看起来是掉档，
    // 而这条路径每跨一年都会跑一次，掉起来还是悄悄掉的。
    const int ceiling = herbMaxAgeForGrade(1);
    SpiritField field{"herb_garden", std::vector<FieldSlot>(1)};
    ASSERT_TRUE(plant(field, 0, "huang_jing", 0, ceiling));
    field.slots[0].age = ceiling * 3;

    growField(field, kDaysPerYear * 10, kDaysPerYear);
    EXPECT_EQ(field.slots[0].age, ceiling * 3);
}

TEST(SpiritFieldGrowth, FallsBackToTheGlobalGateWhenTheSlotHasNoCap) {
    // 没标定上限的槽位（老存档、以及不关心上限的调用方）维持原样：退回全局
    // 溢出闸，不多不少。这条守的是「加了新字段但老数据不会被静默改数」。
    SpiritField field{"herb_garden", std::vector<FieldSlot>(1)};
    ASSERT_TRUE(plant(field, 0, "huang_jing", 0));
    EXPECT_EQ(field.slots[0].maxAge, 0);
    EXPECT_EQ(slotAgeCeiling(field.slots[0]), kMaxHerbAge);

    growField(field, kDaysPerYear * 20000, kDaysPerYear);
    EXPECT_EQ(field.slots[0].age, kMaxHerbAge);
}

TEST(SpiritFieldPlanting, TagsTheSlotWithTheSpeciesCeiling) {
    SpiritField field{"herb_garden", std::vector<FieldSlot>(2)};
    const int ceiling = herbMaxAgeForGrade(2);
    ASSERT_TRUE(plant(field, 0, "zi_shen", 100, ceiling));
    EXPECT_EQ(field.slots[0].maxAge, ceiling);
    EXPECT_EQ(slotAgeCeiling(field.slots[0]), ceiling);

    // 负数当「未标定」处理，不许反向变成 0 年上限——那会让这一畦一年也长不了。
    ASSERT_TRUE(plant(field, 1, "huang_jing", 100, -5));
    EXPECT_EQ(field.slots[1].maxAge, 0);
    EXPECT_EQ(slotAgeCeiling(field.slots[1]), kMaxHerbAge);
}

TEST(SpiritFieldGrowth, NeverLetsASlotCapPushPastTheGlobalGate) {
    // 槽位上限是数据来的，写过头也不能越过溢出闸：herbPrice 的平方曲线在
    // 万年以上会溢出。
    SpiritField field{"herb_garden", std::vector<FieldSlot>(1)};
    ASSERT_TRUE(plant(field, 0, "tian_lei_zhu", 0, kMaxHerbAge * 100));
    EXPECT_EQ(slotAgeCeiling(field.slots[0]), kMaxHerbAge);

    growField(field, kDaysPerYear * 50000, kDaysPerYear);
    EXPECT_EQ(field.slots[0].age, kMaxHerbAge);
}

TEST(SpiritFieldGrowth, DoesNotDriftWhenCalledEveryDay) {
    // 每日结算与一次性结算必须得到同样的年份，否则「挂机 vs 跳时」两条路
    // 会给出不同的收成。
    SpiritField daily{"a", std::vector<FieldSlot>(1)};
    SpiritField lazy{"b", std::vector<FieldSlot>(1)};
    ASSERT_TRUE(plant(daily, 0, "huang_jing", 0));
    ASSERT_TRUE(plant(lazy, 0, "huang_jing", 0));

    for (int day = 1; day <= kDaysPerYear * 5; ++day) growField(daily, day, kDaysPerYear);
    growField(lazy, kDaysPerYear * 5, kDaysPerYear);

    EXPECT_EQ(daily.slots[0].age, 5);
    EXPECT_EQ(daily.slots[0].age, lazy.slots[0].age);
    EXPECT_EQ(daily.slots[0].plantedDay, lazy.slots[0].plantedDay);
}

}  // namespace
