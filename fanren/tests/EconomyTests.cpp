#include "core/rules/Economy.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

#include "core/model/Types.h"
#include "core/rules/Encounter.h"
#include "core/rules/Realm.h"

namespace {

using fanren::core::Item;
using fanren::core::ItemKind;
using fanren::rules::AuctionLot;
using fanren::rules::AuctionRound;
using fanren::rules::bidStep;
using fanren::rules::buyPrice;
using fanren::rules::EncounterEntry;
using fanren::rules::EncounterState;
using fanren::rules::EncounterTable;
using fanren::rules::Realm;
using fanren::rules::restock;
using fanren::rules::sellPrice;
using fanren::rules::Shop;
using fanren::rules::ShopEntry;
using fanren::rules::step;

Item makeHerb(int price = 100) {
    Item item;
    item.id = "herb_test";
    item.name = "测试灵草";
    item.kind = ItemKind::Herb;
    item.price = price;
    return item;
}

Item makePill(int price = 20) {
    Item item;
    item.id = "pill_test";
    item.name = "测试丹药";
    item.kind = ItemKind::Pill;
    item.price = price;
    return item;
}

Shop makeShop(double buyRate = 1.0, double sellRate = 0.5) {
    Shop shop;
    shop.id = "shop_test";
    shop.nameKey = "shop.name.test";
    shop.buyRate = buyRate;
    shop.sellRate = sellRate;
    return shop;
}

// ---------------------------------------------------------------------------
// 1. 买卖价差：货币回收口
// ---------------------------------------------------------------------------

TEST(EconomyPricing, BuyPriceExceedsSellPrice_TheSpreadIsTheMoneySink) {
    const Shop shop = makeShop();
    const Item pill = makePill(20);

    const int buy = buyPrice(shop, pill, 0);
    const int sell = sellPrice(shop, pill, 0);

    // 显式断言价差存在：没有这道口子，灵石会无限膨胀，后期所有定价失效。
    EXPECT_GT(buy, sell);
    EXPECT_EQ(buy, 20);
    EXPECT_EQ(sell, 10);
}

TEST(EconomyPricing, SpreadHoldsAcrossCustomRates) {
    // 黑市：买入价溢价，卖出价也比正经商行厚道，但买价依然必须高于卖价。
    const Shop blackMarket = makeShop(1.8, 0.65);
    const Item weapon = makePill(90);   // 非灵草即可，kind 不是 Herb 就够了

    EXPECT_GT(buyPrice(blackMarket, weapon, 0), sellPrice(blackMarket, weapon, 0));
}

TEST(EconomyPricing, NonHerbItemIgnoresHerbAgeParameter) {
    const Shop shop = makeShop();
    const Item pill = makePill(20);

    EXPECT_EQ(buyPrice(shop, pill, 0), buyPrice(shop, pill, 999));
    EXPECT_EQ(sellPrice(shop, pill, 0), sellPrice(shop, pill, 999));
}

// ---------------------------------------------------------------------------
// 2. 灵草按年份计价：超线性
// ---------------------------------------------------------------------------

TEST(EconomyHerbPricing, AgeZeroOrNegativeEqualsBasePrice) {
    const Shop shop = makeShop();
    const Item herb = makeHerb(100);

    EXPECT_EQ(buyPrice(shop, herb, 0), 100);
    EXPECT_EQ(buyPrice(shop, herb, -5), 100);
}

TEST(EconomyHerbPricing, PriceStrictlyIncreasesWithAge) {
    const Shop shop = makeShop();
    const Item herb = makeHerb(100);

    int previous = buyPrice(shop, herb, 0);
    for (int age : {1, 5, 10, 30, 60, 100}) {
        const int price = buyPrice(shop, herb, age);
        EXPECT_GT(price, previous) << "年份 " << age << " 的定价应严格高于更低年份";
        previous = price;
    }
}

TEST(EconomyHerbPricing, GrowthIsSuperlinearNotJustLinear) {
    // 超线性：同样跨 10 年的价格增量，在年份更高的区间应该更大——
    // 掌天瓶经济的兑现出口要让「催熟」显著划算，而不是线性回本。
    const Shop shop = makeShop();
    const Item herb = makeHerb(100);

    const int deltaLow = buyPrice(shop, herb, 20) - buyPrice(shop, herb, 10);
    const int deltaHigh = buyPrice(shop, herb, 100) - buyPrice(shop, herb, 90);
    EXPECT_GT(deltaHigh, deltaLow);
}

TEST(EconomyHerbPricing, CenturyOldHerbFarExceedsTenTimesADecadeOldOne) {
    // 「百年灵草远不止十年的十倍」——原著级别的设计语言，直接断言比例。
    const Shop shop = makeShop();
    const Item herb = makeHerb(100);

    const int decade = buyPrice(shop, herb, 10);
    const int century = buyPrice(shop, herb, 100);
    EXPECT_GT(century, decade * 10);
}

TEST(EconomyHerbPricing, ZeroOrNegativeBasePriceStaysNonPositive) {
    const Shop shop = makeShop();
    Item freeHerb = makeHerb(0);
    EXPECT_EQ(buyPrice(shop, freeHerb, 50), 0);
}

// ---------------------------------------------------------------------------
// 3. 补货：按日历，能跨周期一次补齐
// ---------------------------------------------------------------------------

TEST(EconomyRestock, FirstSightingOnlyRecordsDayWithoutGrantingStock) {
    Shop shop = makeShop();
    shop.entries.push_back(ShopEntry{"pill_test", 20, /*stock=*/3, /*restockDays=*/5});
    shop.entries.back().restockAmount = 5;
    shop.entries.back().maxStock = 50;

    restock(shop, /*currentDay=*/1);

    EXPECT_EQ(shop.entries[0].stock, 3) << "首次结算只记录起点，不该凭空生成库存";
    EXPECT_EQ(shop.entries[0].lastRestockDay, 1);
}

TEST(EconomyRestock, SinglePeriodElapsedAddsOneRestockAmount) {
    Shop shop = makeShop();
    shop.entries.push_back(ShopEntry{"pill_test", 20, 3, 5});
    shop.entries.back().restockAmount = 5;
    shop.entries.back().maxStock = 50;

    restock(shop, 1);    // 记录起点
    restock(shop, 6);    // 恰好过了一个周期（5 天）

    EXPECT_EQ(shop.entries[0].stock, 8);
    EXPECT_EQ(shop.entries[0].lastRestockDay, 6);
}

TEST(EconomyRestock, MultiplePeriodsElapsedCatchUpAllAtOnce) {
    // 玩家离开三个月回来，不该只补一次。
    Shop shop = makeShop();
    shop.entries.push_back(ShopEntry{"pill_test", 20, 0, 10});
    shop.entries.back().restockAmount = 2;
    shop.entries.back().maxStock = 100;

    restock(shop, 1);     // 记录起点：day 1
    restock(shop, 91);    // 90 天后回来：跨越 9 个周期

    EXPECT_EQ(shop.entries[0].stock, 18);   // 9 * 2
    EXPECT_EQ(shop.entries[0].lastRestockDay, 91);
}

TEST(EconomyRestock, RespectsMaxStockCap) {
    Shop shop = makeShop();
    shop.entries.push_back(ShopEntry{"pill_test", 20, 45, 10});
    shop.entries.back().restockAmount = 20;
    shop.entries.back().maxStock = 50;

    restock(shop, 1);
    restock(shop, 51);   // 5 个周期，理论上 45 + 100 远超上限

    EXPECT_EQ(shop.entries[0].stock, 50);
}

TEST(EconomyRestock, ZeroRestockDaysNeverRestocks) {
    // 限量珍品：restockDays == 0。
    Shop shop = makeShop();
    shop.entries.push_back(ShopEntry{"talisman_test", 54, 1, 0});

    restock(shop, 1);
    restock(shop, 10000);

    EXPECT_EQ(shop.entries[0].stock, 1);
    EXPECT_EQ(shop.entries[0].lastRestockDay, 0) << "不补货的条目不需要记录补货起点";
}

TEST(EconomyRestock, InfiniteStockEntryIsUnaffected) {
    Shop shop = makeShop();
    shop.entries.push_back(ShopEntry{"herb_test", 15, -1, 7});
    shop.entries.back().restockAmount = 10;

    restock(shop, 1);
    restock(shop, 1000);

    EXPECT_EQ(shop.entries[0].stock, -1);
}

TEST(EconomyRestock, PreservesRemainderAcrossCalls) {
    // 不足一个周期的余数应当累积，不能因为调用得勤就被吞掉。
    Shop shop = makeShop();
    shop.entries.push_back(ShopEntry{"pill_test", 20, 0, 10});
    shop.entries.back().restockAmount = 1;
    shop.entries.back().maxStock = 100;

    restock(shop, 1);     // 记录起点：day 1
    restock(shop, 8);     // 7 天，不足一个周期，不补货
    EXPECT_EQ(shop.entries[0].stock, 0);
    EXPECT_EQ(shop.entries[0].lastRestockDay, 1);

    restock(shop, 12);    // 累计 11 天：满一个周期（10 天），余 1 天保留
    EXPECT_EQ(shop.entries[0].stock, 1);
    EXPECT_EQ(shop.entries[0].lastRestockDay, 11);
}

TEST(EconomyRestock, MultipleEntriesRestockIndependently) {
    Shop shop = makeShop();
    shop.entries.push_back(ShopEntry{"pill_test", 20, 0, 5});
    shop.entries.back().restockAmount = 3;
    shop.entries.back().maxStock = 30;
    shop.entries.push_back(ShopEntry{"herb_test", 15, -1, 0});   // 无限库存，不受影响
    shop.entries.push_back(ShopEntry{"talisman_test", 54, 2, 0});   // 限量，不补货

    restock(shop, 1);
    restock(shop, 21);   // 第一条：4 个周期

    EXPECT_EQ(shop.entries[0].stock, 12);
    EXPECT_EQ(shop.entries[1].stock, -1);
    EXPECT_EQ(shop.entries[2].stock, 2);
}

// ---------------------------------------------------------------------------
// 4. 拍卖
// ---------------------------------------------------------------------------

TEST(EconomyAuction, PlayerBidTakesTheLead) {
    const AuctionLot lot{"treasure_test", 100, 200, 0};
    AuctionRound round;

    round = bidStep(lot, round, /*playerBid=*/150, /*heat=*/0, /*seed=*/1);

    EXPECT_GE(round.currentBid, 150);
    // heat=0 时基础追价概率仍可能命中，因此只断言：若玩家仍在领先，出价即为 150。
    if (round.playerLeading) {
        EXPECT_EQ(round.currentBid, 150);
    }
}

TEST(EconomyAuction, RivalCanOvertakeTheLeadingPlayer) {
    // 在大量种子里搜索至少一个会让对手追价的结果，验证「被反超」这条路径
    // 确实可达，且反超后仍未结束（玩家可以选择再加一口）。
    const AuctionLot lot{"treasure_test", 100, 5000, 0};
    bool foundOvertake = false;
    for (std::uint32_t seed = 0; seed < 500 && !foundOvertake; ++seed) {
        AuctionRound round;
        round = bidStep(lot, round, 150, /*heat=*/100, seed);
        if (!round.playerLeading && !round.closed) {
            foundOvertake = true;
            EXPECT_GT(round.rivalBid, 150);
            EXPECT_EQ(round.currentBid, round.rivalBid);
        }
    }
    EXPECT_TRUE(foundOvertake) << "heat=100 时应该能找到对手反超玩家的种子";
}

TEST(EconomyAuction, ClosedRoundNeverReopens) {
    const AuctionLot lot{"treasure_test", 100, 150, 0};
    AuctionRound round;
    round.closed = true;
    round.currentBid = 120;
    round.playerLeading = true;

    const AuctionRound result = bidStep(lot, round, 9999, 100, 42);

    EXPECT_EQ(result.currentBid, 120);
    EXPECT_EQ(result.playerLeading, true);
    EXPECT_TRUE(result.closed);
}

TEST(EconomyAuction, BelowReservePriceIsUnsold) {
    // reservePrice 远高于玩家愿意出的价，且 heat=0 时对手大概率也不会把价格
    // 顶上去——只要最终 closed 且 currentBid 仍低于 reservePrice，就是流拍。
    const AuctionLot lot{"treasure_test", 100, 100000, 0};
    AuctionRound round;
    bool sawClose = false;
    for (std::uint32_t seed = 0; seed < 50 && !sawClose; ++seed) {
        round = AuctionRound{};
        round = bidStep(lot, round, 120, 0, seed);
        if (round.closed) {
            sawClose = true;
            EXPECT_LT(round.currentBid, lot.reservePrice);
        }
    }
    EXPECT_TRUE(sawClose) << "heat=0 时应该能找到对手不追价、直接流拍的种子";
}

TEST(EconomyAuction, PlayerNeverBiddingEndsUnsoldImmediately) {
    const AuctionLot lot{"treasure_test", 100, 500, 0};
    AuctionRound round;

    round = bidStep(lot, round, /*playerBid=*/0, /*heat=*/50, /*seed=*/7);

    EXPECT_TRUE(round.closed);
    EXPECT_FALSE(round.playerLeading);
    EXPECT_LT(round.currentBid, lot.reservePrice);
}

TEST(EconomyAuction, HeatIncreasesChaseAggressiveness) {
    // 抢手程度调节追价概率：同样跑一批种子，heat=100 的追价次数应明显多于 heat=0。
    const AuctionLot lot{"treasure_test", 100, 100000, 0};
    constexpr int kTrials = 400;
    int chasesAtLowHeat = 0;
    int chasesAtHighHeat = 0;

    for (std::uint32_t seed = 0; seed < kTrials; ++seed) {
        AuctionRound lowRound;
        lowRound = bidStep(lot, lowRound, 150, /*heat=*/0, seed);
        if (!lowRound.playerLeading && !lowRound.closed) ++chasesAtLowHeat;

        AuctionRound highRound;
        highRound = bidStep(lot, highRound, 150, /*heat=*/100, seed);
        if (!highRound.playerLeading && !highRound.closed) ++chasesAtHighHeat;
    }

    EXPECT_GT(chasesAtHighHeat, chasesAtLowHeat);
}

TEST(EconomyAuction, HeatIncreasesBumpSize) {
    // 抢手程度也调节追价幅度：把每次追价的加价量平均下来比较。
    const AuctionLot lot{"treasure_test", 1000, 1000000, 0};
    constexpr int kTrials = 400;
    long long lowSum = 0, highSum = 0;
    int lowCount = 0, highCount = 0;

    for (std::uint32_t seed = 0; seed < kTrials; ++seed) {
        AuctionRound lowRound;
        lowRound = bidStep(lot, lowRound, 1500, 0, seed);
        if (!lowRound.playerLeading && !lowRound.closed) {
            lowSum += (lowRound.rivalBid - 1500);
            ++lowCount;
        }

        AuctionRound highRound;
        highRound = bidStep(lot, highRound, 1500, 100, seed);
        if (!highRound.playerLeading && !highRound.closed) {
            highSum += (highRound.rivalBid - 1500);
            ++highCount;
        }
    }

    ASSERT_GT(lowCount, 0);
    ASSERT_GT(highCount, 0);
    const double lowAverage = static_cast<double>(lowSum) / lowCount;
    const double highAverage = static_cast<double>(highSum) / highCount;
    EXPECT_GT(highAverage, lowAverage);
}

TEST(EconomyAuction, SameSeedReproducesTheSameResult) {
    const AuctionLot lot{"treasure_test", 100, 400, 0};

    AuctionRound a;
    a = bidStep(lot, a, 150, 60, 777);
    AuctionRound b;
    b = bidStep(lot, b, 150, 60, 777);

    EXPECT_EQ(a.currentBid, b.currentBid);
    EXPECT_EQ(a.rivalBid, b.rivalBid);
    EXPECT_EQ(a.playerLeading, b.playerLeading);
    EXPECT_EQ(a.closed, b.closed);
}

// ---------------------------------------------------------------------------
// 5. 遭遇表
// ---------------------------------------------------------------------------

EncounterTable makeTable(int stepsMin, int stepsMax, int dailyCap = 8) {
    EncounterTable table;
    table.id = "encounter_test";
    table.stepsMin = stepsMin;
    table.stepsMax = stepsMax;
    table.dailyCap = dailyCap;
    table.entries.push_back(EncounterEntry{"battle_low", 5, Realm::Mortal, Realm::QiRefining3});
    table.entries.push_back(EncounterEntry{"battle_high", 5, Realm::FoundationEarly, Realm::CoreLate});
    return table;
}

TEST(Encounter, TriggersWithinTheConfiguredStepsRange) {
    const EncounterTable table = makeTable(5, 12);

    for (std::uint32_t trial = 0; trial < 100; ++trial) {
        EncounterState state;
        int triggerStep = -1;
        for (int walked = 1; walked <= 200 && triggerStep < 0; ++walked) {
            const std::string battle = step(table, state, Realm::Mortal, 1, trial * 1000u + static_cast<std::uint32_t>(walked));
            if (!battle.empty()) triggerStep = walked;
        }
        ASSERT_GE(triggerStep, table.stepsMin) << "trial=" << trial;
        ASSERT_LE(triggerStep, table.stepsMax) << "trial=" << trial;
    }
}

TEST(Encounter, DoesNotTriggerBeforeStepsMin) {
    EncounterTable table = makeTable(5, 5);   // 固定阈值，去掉随机性
    EncounterState state;

    for (int walked = 1; walked <= 4; ++walked) {
        const std::string battle = step(table, state, Realm::Mortal, 1, 999);
        EXPECT_TRUE(battle.empty()) << "第 " << walked << " 步不应触发";
    }
    const std::string fifth = step(table, state, Realm::Mortal, 1, 999);
    EXPECT_FALSE(fifth.empty());
}

TEST(Encounter, DailyCapStopsFurtherTriggers) {
    EncounterTable table = makeTable(1, 1, /*dailyCap=*/2);
    EncounterState state;

    int triggered = 0;
    for (int walked = 0; walked < 10; ++walked) {
        const std::string battle = step(table, state, Realm::Mortal, /*currentDay=*/1,
                                        static_cast<std::uint32_t>(walked));
        if (!battle.empty()) ++triggered;
    }
    EXPECT_EQ(triggered, 2);
}

TEST(Encounter, DailyCapResetsOnNewDay) {
    EncounterTable table = makeTable(1, 1, /*dailyCap=*/1);
    EncounterState state;

    const std::string day1First = step(table, state, Realm::Mortal, 1, 1);
    EXPECT_FALSE(day1First.empty());
    const std::string day1Second = step(table, state, Realm::Mortal, 1, 2);
    EXPECT_TRUE(day1Second.empty()) << "同一天已达上限";

    const std::string day2First = step(table, state, Realm::Mortal, /*currentDay=*/2, 3);
    EXPECT_FALSE(day2First.empty()) << "跨天应当重置每日上限";
}

TEST(Encounter, RealmFilterExcludesOutOfRangeEntries) {
    EncounterTable table = makeTable(1, 1);   // battle_low 只到 QiRefining3
    EncounterState state;

    // 结丹初期：只有 battle_high 合法，多次触发应从不选到 battle_low。
    for (std::uint32_t seed = 0; seed < 50; ++seed) {
        EncounterState localState;
        const std::string battle = step(table, localState, Realm::CoreEarly, 1, seed);
        if (!battle.empty()) {
            EXPECT_EQ(battle, "battle_high");
        }
    }
}

TEST(Encounter, AllEntriesFilteredOutByRealmDoesNotCrashOrTrigger) {
    EncounterTable table;
    table.id = "encounter_narrow";
    table.stepsMin = 1;
    table.stepsMax = 1;
    table.dailyCap = 99;
    table.entries.push_back(EncounterEntry{"battle_low", 5, Realm::Mortal, Realm::QiRefining3});
    EncounterState state;

    // 境界远超表内所有条目的上限：应该安安静静地永远不触发。
    for (int walked = 0; walked < 20; ++walked) {
        const std::string battle = step(table, state, Realm::CoreLate, 1, static_cast<std::uint32_t>(walked));
        EXPECT_TRUE(battle.empty());
    }
}

TEST(Encounter, EmptyTableNeverTriggersAndDoesNotCrash) {
    EncounterTable table;
    table.id = "encounter_empty";
    table.stepsMin = 1;
    table.stepsMax = 1;
    table.dailyCap = 99;
    EncounterState state;

    for (int walked = 0; walked < 20; ++walked) {
        const std::string battle = step(table, state, Realm::Mortal, 1, static_cast<std::uint32_t>(walked));
        EXPECT_TRUE(battle.empty());
    }
}

TEST(Encounter, WeightedDistributionRoughlyMatchesConfiguredWeights) {
    // 权重 5:1，固定种子跑足够多次统计频率，应当明显偏向权重更高的一侧
    // （不追求精确比例，只验证方向与数量级）。
    EncounterTable table;
    table.id = "encounter_weighted";
    table.stepsMin = 1;
    table.stepsMax = 1;
    table.dailyCap = 1;   // 每次只统计一次触发，天数递增来跳过每日上限
    table.entries.push_back(EncounterEntry{"battle_common", 5, Realm::Mortal, Realm::CoreLate});
    table.entries.push_back(EncounterEntry{"battle_rare", 1, Realm::Mortal, Realm::CoreLate});

    int commonCount = 0, rareCount = 0;
    for (int day = 1; day <= 600; ++day) {
        EncounterState state;
        const std::string battle = step(table, state, Realm::Mortal, day, static_cast<std::uint32_t>(day));
        if (battle == "battle_common") ++commonCount;
        else if (battle == "battle_rare") ++rareCount;
    }

    ASSERT_GT(commonCount + rareCount, 0);
    EXPECT_GT(commonCount, rareCount) << "权重 5:1，常见遭遇理应明显更多";
}

TEST(Encounter, SameSeedReproducesTheSameResult) {
    const EncounterTable table = makeTable(3, 3);

    EncounterState stateA;
    EncounterState stateB;
    std::vector<std::string> resultsA, resultsB;
    for (int walked = 0; walked < 12; ++walked) {
        resultsA.push_back(step(table, stateA, Realm::Mortal, 1, static_cast<std::uint32_t>(walked)));
        resultsB.push_back(step(table, stateB, Realm::Mortal, 1, static_cast<std::uint32_t>(walked)));
    }

    EXPECT_EQ(resultsA, resultsB);
}

}  // namespace
