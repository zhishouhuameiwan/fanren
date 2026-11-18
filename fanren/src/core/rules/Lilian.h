#pragma once
// 历练的两条纯规则（伙伴改版，docs/interfaces-companions.md 3.12）：按境界取档、chance 抽签。
// 不碰 data 目录、不碰 Application，能不开窗口直接单测；Application 在进去、开战两处调它们。
#include <cstdint>
#include <string>

#include "core/model/Lilian.h"
#include "core/rules/Realm.h"

namespace fanren::rules {

// 历练里能有几档（契约：_t1 / _t2 / _t3）。
inline constexpr int kLilianMaxTiers = 3;

// 韩立此刻落在第几档（从 1 数）：第一个 maxRealm 不低于他的境界的那一档；
// 境界高过最高一档的，取最高一档。这处历练地没分档时为 0（编成 id 原样用）。
[[nodiscard]] int lilianTier(const core::LilianSite& site, Realm realm);

// 编成 id 拼上档的后缀：tier ≥ 1 → base + "_t<tier>"；tier ≤ 0 → base 原样。
[[nodiscard]] std::string tieredBattleId(const std::string& base, int tier);

// 进去那一刻的种子：只由「日历日 + 历练地」定（同一天同一处必同，换一天、换一处就变）。
[[nodiscard]] std::uint32_t lilianSeed(int day, const std::string& siteId);

// 带 chance（1–99，出现的百分比）的对象这一趟出不出现。每个对象用「种子 + 对象 id」各抽一次，
// 所以结果与图上对象的先后次序无关。chance ≥ 100 必出现，≤ 0 必不出现。
[[nodiscard]] bool lilianChanceShows(std::uint32_t seed, const std::string& objectId, int chance);

}  // namespace fanren::rules
