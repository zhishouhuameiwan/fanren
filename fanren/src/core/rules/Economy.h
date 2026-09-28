#pragma once
// 商店与拍卖规则。签名以 docs/interfaces-p2.md 第 3 节为准。
//
// core 层硬约束：不含 SDL / JSON / Lua / 文件 IO，只在已解析的结构体上运算；
// 随机数种子由调用方传入，保证「同种子 + 同输入」必得同结果。
#include <cstdint>
#include <string>
#include <vector>

#include "core/model/Types.h"

namespace fanren::rules {

struct ShopEntry {
    std::string itemId;
    int price = 0;        // 当前挂牌价，供 UI 直接显示；不参与 buyPrice/sellPrice 的计算
    int stock = -1;        // -1 表示无限（rules 侧对任何负数一视同仁，但数据里
                           // 只许写 -1：io/ShopLoader.cpp 会拒收别的负数，
                           // 否则一个写错号的补货量能把限量珍品变成无限供应）
    int restockDays = 0;   // 0 表示不补货（限量珍品）

    // ---- 契约（docs/interfaces-p2.md 第 3 节）之外的扩展字段 ----
    // 一律追加在结构体末尾，不破坏聚合初始化——先例见 core/battle/Battle.h 里
    // Unit 的扩展字段。补货要「一次补上跨越多个周期的量」，仅凭 restockDays
    // 这一个周期长度不够：还需要知道每周期补多少（restockAmount）、补到多少
    // 封顶（maxStock）、以及上次结算到哪一天（lastRestockDay，用来推算经过了
    // 几个周期）。这三项契约原文未给，已在模块交付报告中列为契约歧义，此处
    // 先按最小可用集实现，供主控复核。
    int restockAmount = 1;   // 每个补货周期补充的数量
    int maxStock = -1;       // 补货结果上限；-1 表示不设上限
    int lastRestockDay = 0;  // 上次补货结算到的日期；0 表示尚未初始化
};

struct Shop {
    std::string id;
    std::string nameKey;
    double buyRate = 1.0;    // 买入价倍率
    double sellRate = 0.5;   // 卖出价倍率：玩家卖东西天然吃亏，这是货币回收口
    std::vector<ShopEntry> entries;
};

// 按日历补货：对每个 restockDays > 0 的条目，按经过的天数结算跨越了几个周期，
// 一次性补齐——玩家离开三个月回来不该只补一次；lastRestockDay 按周期数累进
// 推进而不是直接置为 currentDay，避免吞掉不足一个周期的余数。
void restock(Shop& shop, int currentDay);

// 买入/卖出价。灵草（core::ItemKind::Herb）按年份计价，曲线一律取
// rules::herbPrice（见 core/rules/Bottle.h）——本模块**不得**再自备一条同类
// 曲线：同一株药在剧情脚本与商店里算出两个价钱，玩家当场就看得见。
// 其余物品直接按 item.price 乘对应倍率。只要 shop.buyRate > shop.sellRate，
// 同一 item/herbAge 下 buyPrice 恒大于 sellPrice——这是货币回收口的落地处，
// 没有这道价差灵石会无限膨胀。
[[nodiscard]] int buyPrice(const Shop& shop, const core::Item& item, int herbAge);
[[nodiscard]] int sellPrice(const Shop& shop, const core::Item& item, int herbAge);

// 拍卖会。原著的标志性场景，也是稀有配方与法器的主要出口。
struct AuctionLot {
    std::string itemId;
    int startPrice = 0;
    int reservePrice = 0;   // 低于此价流拍
    int herbAge = 0;
};

struct AuctionRound {
    int currentBid = 0;
    int rivalBid = 0;
    bool playerLeading = false;
    bool closed = false;
};

// 对手出价一步。heat 表示这件拍品的抢手程度（0-100）：越高，对手继续追价的
// 概率越大、追价幅度也越大——冷门货几轮就没人跟，热门货会被抬到离谱，让玩家
// 在「该不该再加一口」上真的要判断，而不是看纯随机的脸色。
//
// playerBid 高于 round.currentBid 才算一次真实加价；否则视为本轮玩家不再
// 加价——若此时玩家并未处于领先（要么从未出价，要么已被反超），拍卖立即
// 结束。结束后 closed == true，是否流拍由调用方按
// closed && currentBid < lot.reservePrice 判断；已经 closed 的 round 原样
// 返回，拍卖不会死灰复燃。
AuctionRound bidStep(const AuctionLot& lot, AuctionRound round, int playerBid,
                     int heat, std::uint32_t seed);

}  // namespace fanren::rules
