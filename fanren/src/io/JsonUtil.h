#pragma once
// io 模块内部共享的 JSON 读取小工具。不对外公开（不在 docs/interfaces.md 里），
// 只是把 DataLoader 和 TileMapLoader 都要用到的“打开文件 + 解析 JSON + 报错”
// 抽成一处，避免两处各写一份、日后错误信息格式不一致。
#include <filesystem>
#include <string>

#include <nlohmann/json.hpp>

#include "core/Result.h"

namespace fanren::io::detail {

// 读取并解析一个 JSON 文件。文件不存在、打不开、JSON 语法错误都归为失败，
// error 里带上文件路径，方便定位是哪一份数据坏了。
//
// 用 json::parse 的非抛出重载（allow_exceptions=false）而不是 try/catch 包裹
// 抛出版本：本函数只负责“解析”这一步，用返回值表达失败比在这里引入异常处理
// 更直接，也让调用方不必为这一步专门写 catch。
[[nodiscard]] core::Result<nlohmann::json> readJsonFile(const std::filesystem::path& path);

}  // namespace fanren::io::detail
