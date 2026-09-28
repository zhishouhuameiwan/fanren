#include "core/rules/Economy.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <random>

// 灵草按年份计价的唯一出处。见下面 basePriceFor 上方的说明。
#include "core/rules/Bottle.h"

namespace fanren::rules {
namespace {

// 灵草按年份计价：一律走 rules::herbPrice，本文件不再自备第二条曲线。
//
// 这里曾经有一条独立实现（agedHerbPrice，1 + 0.02×年份 + 0.05×年份²），理由是
// 「core 层各模块并行开发期间不得互相 include」。那条理由随并行期一起过去了，
// 留下的是一株药两个价钱：一株 44 年黄精，剧情脚本按 herbPrice 结算 145 灵石，
// 商店按那条平行曲线要算出 493。商店面板一落地，玩家在同一章里就会同时看到
// 这两个数——这不是数值口味问题，是当场自相矛盾。
//
// 保留下来的是 herbPrice 而不是这条：
//   * 它是掌天瓶经济的动力曲线，理由写在 Bottle.h 上方，改它等于改整个经济；
//   * docs/ch02-design.md 第 3 节那张实算表（1 年 6 / 11 年 22 / 44 年 145）
//     是按它算的，编剧的台词与 scripts/ch02 的三次结算都照着那张表写死了。
//
// 依赖方向仍然是干净的：Economy.h 本来就 include 了 core/model/Types.h，
// 而 Types.h 里的 GameState 直接持有 rules::Bottle——两个头文件早已在同一
// 编译单元里了，这里只是把这条依赖写明。
[[nodiscard]] int basePriceFor(const core::Item& item, int herbAge) {
    if (item.kind == core::ItemKind::Herb) {
        return herbPrice(item.price, herbAge);
    }
    return item.price;
}

// 乘上倍率之后夹回 int，两头都夹。
//
// 上夹不是形式主义：herbPrice 到顶会返回 INT_MAX（万年份灵草，大纲 4.1 明写
// 会有），再乘 buyRate 1.2 就冲出 int，而 static_cast 在那之后给出的数是绕回来
// 的——绕成负数会被下面的 0 夹住，于是一件天价宝物的标价变成 0，白送。
// 同一道防线 Bottle.cpp 早就有（saturateToInt），这里补齐。
[[nodiscard]] int saturateToInt(double value) noexcept {
    constexpr int kMaxInt = std::numeric_limits<int>::max();
    // NaN 一并落到 0：写成 value <= 0 的话 NaN 两边都不成立，会一路漏到下面。
    if (!(value > 0.0)) return 0;
    if (value >= static_cast<double>(kMaxInt)) return kMaxInt;
    return static_cast<int>(std::llround(value));
}

// 对手追价的概率与幅度都随 heat（0-100）走：冷门货基础概率低、加价意思意思；
// 热门货概率高、幅度大。数值只是内部调平衡用的旋钮，不在契约里。
constexpr int kAuctionChaseBaseChance = 15;   // heat=0 时的追价概率
constexpr int kAuctionChaseHeatSpan = 70;     // heat=100 时追价概率升到约 85%
constexpr double kAuctionBumpMinRate = 0.03;  // heat=0 时的加价幅度基准（相对当前价）
constexpr double kAuctionBumpMaxRate = 0.12;  // heat=100 时的加价幅度基准

}  // namespace

void restock(Shop& shop, int currentDay) {
    for (ShopEntry& entry : shop.entries) {
        if (entry.restockDays <= 0) continue;   // 限量珍品，不补货
        if (entry.stock < 0) continue;          // 已是无限库存，补货无意义

        if (entry.lastRestockDay <= 0) {
            // 首次见到该条目：以 currentDay 为补货起点，不倒扣此前从未结算过的
            // 空窗期（否则新开的店一进游戏就被判定「已经欠了好几个周期」）。
            entry.lastRestockDay = currentDay;
            continue;
        }

        const int elapsed = currentDay - entry.lastRestockDay;
        if (elapsed < entry.restockDays) continue;

        const int cycles = elapsed / entry.restockDays;
        entry.stock += cycles * entry.restockAmount;
        if (entry.maxStock >= 0 && entry.stock > entry.maxStock) {
            entry.stock = entry.maxStock;
        }
        // 累进推进而非直接置为 currentDay：保留不足一个周期的余数，否则玩家
        // 卡着周期边界反复进出会白白多吃到补货。
        entry.lastRestockDay += cycles * entry.restockDays;
    }
}

int buyPrice(const Shop& shop, const core::Item& item, int herbAge) {
    return saturateToInt(basePriceFor(item, herbAge) * shop.buyRate);
}

int sellPrice(const Shop& shop, const core::Item& item, int herbAge) {
    return saturateToInt(basePriceFor(item, herbAge) * shop.sellRate);
}

AuctionRound bidStep(const AuctionLot& lot, AuctionRound round, int playerBid, int heat,
                     std::uint32_t seed) {
    if (round.closed) return round;   // 拍卖不会死灰复燃

    heat = std::clamp(heat, 0, 100);
    if (round.currentBid < lot.startPrice) {
        round.currentBid = lot.startPrice;   // 尚未开拍时以起拍价兜底
    }

    if (playerBid > round.currentBid) {
        round.currentBid = playerBid;
        round.playerLeading = true;
    }

    if (!round.playerLeading) {
        // 玩家本轮未曾领先：要么从未出价，要么被反超后选择不再加价——两种
        // 情况都视为玩家放弃，拍卖到此结束。
        round.closed = true;
        return round;
    }

    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> chaseRoll(0, 99);
    const int chaseChance = kAuctionChaseBaseChance + heat * kAuctionChaseHeatSpan / 100;

    if (chaseRoll(rng) < chaseChance) {
        const double rate = kAuctionBumpMinRate +
                             (kAuctionBumpMaxRate - kAuctionBumpMinRate) * heat / 100.0;
        std::uniform_int_distribution<int> variance(50, 150);   // 幅度上下浮动，避免可预测
        const int bump = std::max(1, static_cast<int>(round.currentBid * rate * variance(rng) / 100.0));
        round.rivalBid = round.currentBid + bump;
        round.currentBid = round.rivalBid;
        round.playerLeading = false;
    } else {
        // 对手放弃追价：拍卖结束，玩家是否真的拿下由调用方按
        // currentBid >= lot.reservePrice 判定。
        round.closed = true;
    }
    return round;
}

}  // namespace fanren::rules
