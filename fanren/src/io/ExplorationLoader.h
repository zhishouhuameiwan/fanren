#pragma once
// 集齐奖励表 data/exploration/<章>.json 的读取器（docs/ch07-playtest-ui.md 第 4 节）。
//
// 一个文件一章，形如：
//   {"note": "...",                                   // 可选，给人看的
//    "maps": [                                        // 必填、非空
//      {"map": "ch07_jindi_waiwei",                   // 必填，图 id；整个目录里不许重复
//       "done_flag": "ch07.xunbian_waiwei",           // 必填，发过了置它；整个目录里不许重复
//       "spirit_stones": 10,                          // 可选，0–999，缺省 0
//       "items": [{"itemId": "material_hanyan_cao", "count": 1}],   // 可选，每项 count 1–99
//       "bond": 1}]}                                  // 可选，0–5，缺省 0
//
// 这里只查**形状**（必填项、类型、表外字段、取值范围、重复）。引用存不存在——图、物品、旗标——归门禁
// tools/validate.py（itemId、done_flag 这两个字段名它认得）与 tests/ExplorationTests.cpp 在真树上的那一条。
#include <map>
#include <string>

#include "core/Result.h"
#include "core/model/Exploration.h"

namespace fanren::io {

inline constexpr int kExplorationMaxStones = 999;
inline constexpr int kExplorationMaxItemCount = 99;
inline constexpr int kExplorationMaxBond = 5;

// 读整个目录（只读这一层的 .json，按文件名排序），图 id → 奖励。目录不存在 = 一张也没配，不算错；
// 任何一份写坏了、两份配了同一张图或同一面旗，整份判失败（不读进一半）。
[[nodiscard]] core::Result<std::map<std::string, core::ExplorationReward>> loadExplorationRewards(
    const std::string& dir);

}  // namespace fanren::io
