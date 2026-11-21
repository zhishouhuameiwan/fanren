#pragma once
// 伙伴的两个查询与默契的档（伙伴改版，docs/interfaces-companions.md 2.1、2.2）。纯函数，不碰 data 目录。
//
// **队伍里存的是角色 id**（GameState::party），不是伙伴 id。凡是新规则要问「这是哪位伙伴」
// （默契、追击、合招、场地技、插话的 require_party、一人一身），一律经 companionOfRole；
// 只有历练组队与主菜单名册页要问「她此刻该用哪个角色」，经 currentRole。别处不许自己去翻 roles。
#include <map>
#include <string>

#include "core/model/Types.h"

namespace fanren::rules {

// 这个角色属于哪位伙伴；不属于任何伙伴（傀儡、化身、路人）时为空串。
[[nodiscard]] std::string companionOfRole(const std::map<std::string, core::Companion>& companions,
                                          const std::string& roleId);

// 这位伙伴此刻该用的角色 id：roles 里第一条 until_flag 还没置的（没写 until_flag 的一直有效）。
// 伙伴不存在、或每一段都已过去时为空串。
[[nodiscard]] std::string currentRole(const std::map<std::string, core::Companion>& companions,
                                      const std::string& companionId, const core::GameState& state);

// 默契的上限与五档的门槛（契约 2.2 的初值）：初识 0、相熟 20、默契 45、知己 70、同心 95。
inline constexpr int kBondMax = 100;
inline constexpr int kBondTierThresholds[] = {0, 20, 45, 70, 95};

// 数值落在第几档（1–5）。
[[nodiscard]] int bondTier(int value);

// 这位伙伴此刻的默契上限（同道录 2a，docs/ch07-tongdaolu.md 3.2）：伙伴表写了封顶（capFlag / capTier）且那面旗
// 还没置，就是第 capTier 档的最高值（下一档门槛减一：四档 = 94）；否则 kBondMax。
// 南宫婉第 7 章封在四档，就是这一条。下面每个记账入口都收一个 cap，调用方取这里的数传进去。
[[nodiscard]] int bondCap(const core::Companion& companion, const core::GameState& state);
// 按伙伴 id 查；查不到的伙伴答 kBondMax。
[[nodiscard]] int bondCapOf(const std::map<std::string, core::Companion>& companions, const std::string& companionId,
                            const core::GameState& state);

// 默契加 amount（只增不减、封顶 cap，cap 缺省 kBondMax）。amount < 0 不动账、返回 false；其余返回 true。
// 已经高过 cap 的账（旧档）不往下压，只是不再涨。
// 不判每天的上限：剧情脚本的 bond.add 走这一条。并肩、赠礼有每天的账，走下面两个入口。
bool addBond(core::BondRecord& record, int amount, int cap = kBondMax);

// ---- 按天记账的两个来源（契约 2.2）：战斗结算（B 路）与赠礼（D 路）只调这里，不各写一套 ----
//
// today 是 GameState::day。日历从第 1 天起（rules::Calendar::day、GameState::day 的缺省都是 1）
// 且只往前走，所以 BondRecord 里拿 0 当「从来没有过」不会与真的某一天撞上。

inline constexpr int kBondPerBattleWin = 2;        // 并肩打赢一仗
inline constexpr int kBondBattleDailyCap = 6;      // 同一位伙伴、同一天，并肩那一项至多加这么多
inline constexpr int kBondGiftLiked = 5;
inline constexpr int kBondGiftPlain = 2;
inline constexpr int kBondGiftDisliked = 0;

// 并肩打赢一仗：+kBondPerBattleWin，同一天累计到 kBondBattleDailyCap 为止，仍封顶 kBondMax。
// 返回这一仗实际加了多少（到了当天上限或已满 100 就是 0）。
int addBattleBond(core::BondRecord& record, int today, int cap = kBondMax);

enum class GiftTaste { Liked, Plain, Disliked };

// 一件物品对这位伙伴算什么：在 like 里是合意，在 dislike 里是不喜，都不在是寻常（两张表都列了按合意算）。
[[nodiscard]] GiftTaste giftTaste(const core::Companion& companion, const std::string& itemId);

struct GiftOutcome {
    bool given = false;    // false = 今天已经收过了，账一点没动（东西也不该扣，由调用方照此办）
    GiftTaste taste = GiftTaste::Plain;
    int gained = 0;        // 实际加了多少（封顶之后）
};

// 赠礼：每人每天一次。今天收过了 → given = false、不动账；否则记下今天，合意 +5、寻常 +2、不喜 0。
// 不喜的也算今天送过了。
GiftOutcome giveGift(core::BondRecord& record, const core::Companion& companion, const std::string& itemId,
                     int today, int cap = kBondMax);

}  // namespace fanren::rules
