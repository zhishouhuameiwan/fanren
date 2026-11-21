#pragma once
// 对白头像表 data/visual/portraits.json 的读取器（伙伴改版，docs/interfaces-companions.md 3.9；
// 形状见 docs/art-sprites.md 5.3）。读出来的样子在 core/model/Portraits.h。
//
// 严格：与 VisualLoader 同一个口径——缺必填项、类型不对、写了认不出的键或表情，整份报错，只报第一处
// （「portraits.json 的 portraits.nangong_wan.faces：……」），语法错带行列号。调用方（game/Portraits.cpp）
// 读坏了打一行警告、当作空表：对白照旧不带头像，不挡游戏。
//
//   {
//     "_doc": "……",                                   // 可不写：给人看的说明
//     "portraits": {                                  // 必填，可以是空表
//       "<角色 id>": [                                // 非空数组：按章生效的头像，取第一条成立的
//         {
//           "file": "nangong_wan.png",                // 相对 assets/art/portraits/
//           "frame_w": 128, "frame_h": 136, "cols": 4,  // 格子，正整数
//           "faces": {"normal": 0, "smile": 1, "angry": 2, "hurt": 3},  // 四个表情都写，帧号 ≥ 0
//           "min_chapter": 7, "max_chapter": 7        // 可不写（0 = 不限），与 role_variants 同一个意思
//         }
//       ]
//     }
//   }
#include <string>
#include <string_view>

#include "core/Result.h"
#include "core/model/Portraits.h"

namespace fanren::io {

// parse 的每一种报错都以「portraits.json」起头；load 读不了文件、内容坏了时报错带路径。
[[nodiscard]] core::Result<core::PortraitTable> parsePortraits(std::string_view json);
[[nodiscard]] core::Result<core::PortraitTable> loadPortraits(const std::string& path);

// 高清立绘表 data/visual/busts.json（女角色立绘，docs/heroine-art-build.md 第 2 节）：与头像表同一个形状、同一套严格口径，
// 只是顶层键叫 busts、图相对 assets/art/busts/，另可带一个 files（文件名 → sha256，tools/artgen/busts.py 写、--check 核）。
// 报错以「busts.json」起头。
//
//   {
//     "_doc": "……",
//     "busts": {"chen_shimei": [{"file": "chen_shimei.png", "frame_w": 840, "frame_h": 1260, "cols": 4,
//                                "faces": {"normal": 0, "smile": 1, "angry": 2, "hurt": 3}}]},
//     "files": {"chen_shimei.png": "<sha256>"}
//   }
[[nodiscard]] core::Result<core::PortraitTable> parseBusts(std::string_view json);
[[nodiscard]] core::Result<core::PortraitTable> loadBusts(const std::string& path);

}  // namespace fanren::io
