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

// 默契加 amount（只增不减、封顶 kBondMax）。amount < 0 不动账、返回 false；其余返回 true。
// 每天的上限（并肩、赠礼）不在这里判：那两项的记账日子在 BondRecord 上，由各自的入口管。
bool addBond(core::BondRecord& record, int amount);

}  // namespace fanren::rules
