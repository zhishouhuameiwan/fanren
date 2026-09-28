#pragma once
// io 模块内部：严格读 JSON 文本，语法错报到「第几行第几列」。不对外公开（与 JsonUtil.h 同一类）。
//
// 为什么不直接用 JsonUtil.h 的 readJsonFile：那一份用 nlohmann 的非抛出重载，坏文件只换来
// 「JSON 语法错误: <路径>」——够数据表用（tools/validate.py 先一步查过语法），可美术产物的描述文件
// （index.json、meta.json）与章节表没有那道前置检查，人手改坏一个逗号，行列号是唯一的线索。
// 这几份文件原先各由上层一个手写读取器读（ui::Json、game::VisualJson），都带行列号、都拒 BOM、
// 都限嵌套深度；统一到 io 之后这几道闸一道不少。
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

#include "core/Result.h"

namespace fanren::io::detail {

// 解析一整段文本。失败时 error 形如「第 3 行第 14 列：……」。
// 比 nlohmann 的缺省多拦两样：UTF-8 BOM（nlohmann 会悄悄跳过，生成器从不写它）与过深的嵌套。
[[nodiscard]] core::Result<nlohmann::json> parseStrictJson(std::string_view text);

// 读整个文件（二进制方式：JSON 一律 UTF-8，不要文本模式那一层换行转换）。
// 打不开、读出错都算失败，error 里带路径。
[[nodiscard]] core::Result<std::string> readTextFile(const std::string& path);

// 整数：JSON 的数不分整数与小数（1.5e3 也是整数），帧号、坐标、颜色分量要的是整数。
// value 是数、没有小数部分、落在 int 范围里才算；out 只在返回 true 时写入。
[[nodiscard]] bool toInt(const nlohmann::json& value, int& out);

}  // namespace fanren::io::detail
