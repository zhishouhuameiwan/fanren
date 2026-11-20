#pragma once
// 指引的几句共用措辞（第一期地基包，docs/ch07-foundation.md 第 2 节）。
//
// 目标框（WorldScene / WorldView）与记事（BoardScene）说的是同一件事，所以「行动句怎么接进度」
// 「经哪几张图怎么写」只在这里定义一次——两处各拼一遍，迟早一处写「1/3」一处写「(1/3)」。
// 只拼字，不碰存档；数数的判据在 core/rules/Objectives.h。
#include <string>
#include <vector>

#include "core/model/Types.h"

namespace fanren::game::guide {

// 照第一期新版式写过的一步（有要点、详情或进度的那些，第 7 章起）。目标框的「前往」行据此决定
// 写整条路还是照旧只报下一站：别的章没改数据，画面与从前逐字相同（判据 5）。
[[nodiscard]] bool upgradedObjective(const core::Objective& step);

// 行动句，有进度就接「　1/3」。
[[nodiscard]] std::string objectiveActionLine(const core::GameData& data, const core::GameState& state,
                                              const core::Objective& step);

// 「（经黄枫谷、岳麓殿）」。空表为空串；多于 kMaxVia 个只写前几个，后面接「…」。
inline constexpr std::size_t kMaxVia = 3;
[[nodiscard]] std::string viaClause(const std::vector<std::string>& mapNames);

}  // namespace fanren::game::guide
