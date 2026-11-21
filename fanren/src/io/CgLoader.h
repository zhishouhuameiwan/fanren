#pragma once
// 事件图表 data/visual/cg.json 的读取器（第 7 章同道录 2a·路 U，施工图 docs/ch07-tongdaolu.md 3.5；
// 施工记录 docs/ch07-tongdaolu-u.md）。表与图归路 N 建，这里只读。
//
// 形状照 busts.json 的风格定死，严格：缺必填项、类型不对、写了认不出的键，整份报错（只报第一处，以「cg.json」起头）。
//
//   {
//     "_doc": "……",                                   // 可不写
//     "cg": {"<事件图 id>": {"file": "<文件名>.png"}},  // 必填，可以是空表；图相对 assets/art/cg/
//     "files": {"<文件名>.png": "<sha256>"}             // 可不写：文件名 → 64 位小写十六进制
//   }
//
// 调用方（game/EventCg.cpp）读坏了打一行警告、当作空表：脚本 show_cg 照走，只是不画。
#include <map>
#include <string>
#include <string_view>

#include "core/Result.h"

namespace fanren::io {

using CgTable = std::map<std::string, std::string>;   // 事件图 id → 文件名

[[nodiscard]] core::Result<CgTable> parseCgTable(std::string_view json);
[[nodiscard]] core::Result<CgTable> loadCgTable(const std::string& path);

}  // namespace fanren::io
