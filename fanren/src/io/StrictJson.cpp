#include "io/StrictJson.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <limits>
#include <utility>

namespace fanren::io::detail {
namespace {

using nlohmann::json;

// 生成器写得出来的深度是四五层，上百层只可能是坏文件。nlohmann 的解析与析构都不递归，
// 深了也不会崩；可「深得离谱」本身就该是一句报错，而不是照单全收（原先两个手写读取器都这么拦）。
constexpr int kMaxDepth = 64;

// text[offset] 所在的行与列（都从 1 数）。
std::string positionOf(std::string_view text, std::size_t offset) {
    offset = std::min(offset, text.size());
    int line = 1;
    std::size_t lineStart = 0;
    for (std::size_t i = 0; i < offset; ++i) {
        if (text[i] == '\n') {
            ++line;
            lineStart = i + 1;
        }
    }
    return "第 " + std::to_string(line) + " 行第 " + std::to_string(offset - lineStart + 1) + " 列";
}

// 第一个让嵌套超过 kMaxDepth 层的括号在哪；没有返回 npos。字符串里的括号不算。
// 只数括号、不管语法：语法错交给 nlohmann 去报，它说得比这里准。
std::size_t tooDeepAt(std::string_view text) {
    int depth = 0;
    bool inString = false;
    bool escaped = false;
    for (std::size_t i = 0; i < text.size(); ++i) {
        const char c = text[i];
        if (inString) {
            if (escaped) {
                escaped = false;
            } else if (c == '\\') {
                escaped = true;
            } else if (c == '"') {
                inString = false;
            }
        } else if (c == '"') {
            inString = true;
        } else if (c == '[' || c == '{') {
            if (++depth > kMaxDepth) return i;
        } else if (c == ']' || c == '}') {
            --depth;
        }
    }
    return std::string_view::npos;
}

}  // namespace

core::Result<json> parseStrictJson(std::string_view text) {
    using R = core::Result<json>;
    if (text.substr(0, 3) == "\xEF\xBB\xBF") {
        return R::failure(positionOf(text, 0) + "：文件以 UTF-8 BOM 开头（生成器写的是不带 BOM 的 UTF-8）");
    }
    if (const std::size_t deep = tooDeepAt(text); deep != std::string_view::npos) {
        return R::failure(positionOf(text, deep) + "：嵌套超过 " + std::to_string(kMaxDepth) + " 层");
    }
    // 用抛出版本而不是 allow_exceptions=false：出错的字节位置（parse_error::byte）只在异常里，
    // 非抛出版本失败时只剩一个 discarded 值，行列号就丢了。
    try {
        return R::success(json::parse(text.begin(), text.end()));
    } catch (const json::parse_error& e) {
        // byte 是读到出错那个字符时已读的字节数（从 1 数），出错的字符在 byte - 1。
        return R::failure(positionOf(text, e.byte == 0 ? 0 : e.byte - 1) + "：" + e.what());
    } catch (const json::exception& e) {
        // 语法对、数却超出 double（1e400）：nlohmann 报 out_of_range，不带位置。
        return R::failure(std::string("数值超出范围：") + e.what());
    }
}

core::Result<std::string> readTextFile(const std::string& path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream.is_open()) return core::Result<std::string>::failure("打不开文件：" + path);
    std::string text((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    if (stream.bad()) return core::Result<std::string>::failure("读文件出错：" + path);
    return core::Result<std::string>::success(std::move(text));
}

bool toInt(const json& value, int& out) {
    constexpr int kMin = std::numeric_limits<int>::min();
    constexpr int kMax = std::numeric_limits<int>::max();
    // nlohmann 把非负整数存成 unsigned、负整数存成 signed、带小数点或指数的存成 double，三种分开量。
    if (value.is_number_unsigned()) {
        const auto n = value.get<std::uint64_t>();
        if (n > static_cast<std::uint64_t>(kMax)) return false;
        out = static_cast<int>(n);
        return true;
    }
    if (value.is_number_integer()) {
        const auto n = value.get<std::int64_t>();
        if (n < kMin || n > kMax) return false;
        out = static_cast<int>(n);
        return true;
    }
    if (value.is_number_float()) {
        const double n = value.get<double>();
        if (n != std::floor(n) || n < static_cast<double>(kMin) || n > static_cast<double>(kMax)) return false;
        out = static_cast<int>(n);
        return true;
    }
    return false;
}

}  // namespace fanren::io::detail
