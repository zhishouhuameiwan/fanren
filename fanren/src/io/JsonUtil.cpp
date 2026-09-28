#include "io/JsonUtil.h"

#include <fstream>
#include <sstream>

namespace fanren::io::detail {

using nlohmann::json;

core::Result<json> readJsonFile(const std::filesystem::path& path) {
    std::error_code existsEc;
    if (!std::filesystem::exists(path, existsEc)) {
        std::ostringstream err;
        err << "文件不存在: " << path.string();
        return core::Result<json>::failure(err.str());
    }

    // 二进制模式打开：JSON 一律 UTF-8，不需要文本模式的换行转换，且能避免
    // Windows 下把 \r\n 悄悄改写成 \n 导致字节数与后面用到的偏移对不上。
    std::ifstream stream(path, std::ios::binary);
    if (!stream.is_open()) {
        std::ostringstream err;
        err << "无法打开文件: " << path.string();
        return core::Result<json>::failure(err.str());
    }

    json parsed = json::parse(stream, /*callback*/ nullptr, /*allow_exceptions*/ false);
    if (parsed.is_discarded()) {
        std::ostringstream err;
        err << "JSON 语法错误: " << path.string();
        return core::Result<json>::failure(err.str());
    }
    return core::Result<json>::success(std::move(parsed));
}

}  // namespace fanren::io::detail
