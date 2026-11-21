#include "io/PortraitLoader.h"

#include <array>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "io/StrictJson.h"

namespace fanren::io {
namespace {

using nlohmann::json;
using Out = core::Result<core::PortraitTable>;

// 两份表同一个形状：对白头像 portraits.json（顶层键 portraits）、高清立绘 busts.json（顶层键 busts，
// 另可带 files：文件名 → sha256，由 tools/artgen/busts.py 写）。报错都以各自的文件名起头。
struct Shape {
    const char* label;    // "portraits.json" / "busts.json"
    const char* topKey;   // "portraits" / "busts"
    bool files;           // 认不认顶层的 files
};
constexpr Shape kPortraitShape{"portraits.json", "portraits", false};
constexpr Shape kBustShape{"busts.json", "busts", true};

Out fail(const Shape& shape, const std::string& where, const std::string& what) {
    return Out::failure(std::string(shape.label) + " 的 " + where + "：" + what);
}

bool isSha256(const json& v) {
    if (!v.is_string()) return false;
    const std::string& s = v.get_ref<const std::string&>();
    if (s.size() != 64) return false;
    for (const char c : s) {
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
    }
    return true;
}

// 正整数（minimum = 1）或非负整数（minimum = 0）。
bool intAtLeast(const json& v, int minimum, int& out) {
    int value = 0;
    if (!detail::toInt(v, value) || value < minimum) return false;
    out = value;
    return true;
}

int faceSlot(const std::string& name) {
    for (std::size_t i = 0; i < core::kPortraitFaces.size(); ++i) {
        if (name == core::kPortraitFaces[i]) return static_cast<int>(i);
    }
    return -1;
}

// 一条头像。错了返回报错（where 起头），对了把 out 填满、返回空串。
std::string readSheet(const Shape& shape, const json& entry, const std::string& where, core::PortraitSheet& out) {
    const auto bad = [&shape, &where](const std::string& field, const std::string& what) {
        return std::string(shape.label) + " 的 " + where + (field.empty() ? "" : "." + field) + "：" + what;
    };
    if (!entry.is_object()) return bad("", "应当是对象");
    bool hasFile = false, hasW = false, hasH = false, hasCols = false, hasFaces = false;
    for (const auto& [key, value] : entry.items()) {
        if (key == "file") {
            if (!value.is_string() || value.get_ref<const std::string&>().empty()) return bad(key, "应当是非空字符串");
            out.file = value.get<std::string>();
            hasFile = true;
        } else if (key == "frame_w") {
            if (!intAtLeast(value, 1, out.frameW)) return bad(key, "应当是正整数");
            hasW = true;
        } else if (key == "frame_h") {
            if (!intAtLeast(value, 1, out.frameH)) return bad(key, "应当是正整数");
            hasH = true;
        } else if (key == "cols") {
            if (!intAtLeast(value, 1, out.cols)) return bad(key, "应当是正整数");
            hasCols = true;
        } else if (key == "min_chapter") {
            if (!intAtLeast(value, 0, out.minChapter)) return bad(key, "应当是非负整数（0 = 不限）");
        } else if (key == "max_chapter") {
            if (!intAtLeast(value, 0, out.maxChapter)) return bad(key, "应当是非负整数（0 = 不限）");
        } else if (key == "faces") {
            if (!value.is_object()) return bad(key, "应当是对象（表情 → 帧号）");
            std::array<bool, core::kPortraitFaces.size()> seen{};
            for (const auto& [face, frame] : value.items()) {
                const int slot = faceSlot(face);
                if (slot < 0) return bad(key + "." + face, "认不出的表情（只有 normal / smile / angry / hurt）");
                if (!intAtLeast(frame, 0, out.faces[static_cast<std::size_t>(slot)])) {
                    return bad(key + "." + face, "应当是非负整数（帧号）");
                }
                seen[static_cast<std::size_t>(slot)] = true;
            }
            for (std::size_t i = 0; i < seen.size(); ++i) {
                if (!seen[i]) return bad(key, std::string("缺表情 ") + core::kPortraitFaces[i] + "（四个都要写）");
            }
            hasFaces = true;
        } else {
            return bad(key, "认不出的字段");
        }
    }
    if (!hasFile) return bad("file", "缺少");
    if (!hasW) return bad("frame_w", "缺少");
    if (!hasH) return bad("frame_h", "缺少");
    if (!hasCols) return bad("cols", "缺少");
    if (!hasFaces) return bad("faces", "缺少");
    if (out.minChapter > 0 && out.maxChapter > 0 && out.minChapter > out.maxChapter) {
        return bad("", "min_chapter 大于 max_chapter：哪一章都不生效");
    }
    return {};
}

// 一个角色的头像列表：非空数组，每一条是一张头像（带可选的 min_chapter / max_chapter）。
std::string readList(const Shape& shape, const json& list, const std::string& where,
                     std::vector<core::PortraitSheet>& out) {
    if (!list.is_array() || list.empty()) {
        return std::string(shape.label) + " 的 " + where + "：应当是非空数组（每一条一张头像，可带 min_chapter / max_chapter）";
    }
    for (std::size_t i = 0; i < list.size(); ++i) {
        core::PortraitSheet sheet;
        if (std::string error = readSheet(shape, list[i], where + "[" + std::to_string(i) + "]", sheet); !error.empty()) {
            return error;
        }
        out.push_back(std::move(sheet));
    }
    // 取的是第一条在这一章生效的：一条不限章的（没写章号，或 min ≤ 1 且不写 max）后面再跟条目，后面的永远取不到。
    // 那多半是次序写反了（该把限章的写在前面），与读取器一贯的口径一样，整份报错。
    for (std::size_t i = 0; i + 1 < out.size(); ++i) {
        if (out[i].minChapter <= 1 && out[i].maxChapter <= 0) {
            return std::string(shape.label) + " 的 " + where + "[" + std::to_string(i) + "]：这一条不限章，后面的 " +
                   std::to_string(out.size() - i - 1) + " 条永远取不到（限章的写在前面，不限章的放最后）";
        }
    }
    return {};
}

Out parseTable(const Shape& shape, std::string_view text) {
    const std::string label = shape.label;
    auto parsed = detail::parseStrictJson(text);
    if (!parsed) return Out::failure(label + " " + parsed.error);
    const json& root = parsed.value;
    if (!root.is_object()) return Out::failure(label + "：顶层应当是对象");

    const json* entries = nullptr;
    for (const auto& [key, value] : root.items()) {
        if (key == "_doc") {
            if (!value.is_string()) return fail(shape, key, "应当是字符串");
        } else if (key == shape.topKey) {
            if (!value.is_object()) return fail(shape, key, "应当是对象（角色 id → 头像）");
            entries = &value;
        } else if (shape.files && key == "files") {
            if (!value.is_object()) return fail(shape, key, "应当是对象（文件名 → sha256）");
            for (const auto& [name, digest] : value.items()) {
                if (!isSha256(digest)) return fail(shape, key + "." + name, "应当是 64 位小写十六进制的 sha256");
            }
        } else {
            return fail(shape, key, "认不出的字段");
        }
    }
    if (entries == nullptr) return fail(shape, shape.topKey, "缺少");

    core::PortraitTable table;
    for (const auto& [roleId, entry] : entries->items()) {
        std::vector<core::PortraitSheet> sheets;
        if (std::string error = readList(shape, entry, std::string(shape.topKey) + "." + roleId, sheets);
            !error.empty()) {
            return Out::failure(std::move(error));
        }
        table.emplace(roleId, std::move(sheets));
    }
    return Out::success(std::move(table));
}

Out loadTable(const Shape& shape, const std::string& path) {
    auto text = detail::readTextFile(path);
    if (!text) return Out::failure(text.error);
    auto table = parseTable(shape, text.value);
    if (!table) return Out::failure(path + "：" + table.error);
    return table;
}

}  // namespace

core::Result<core::PortraitTable> parsePortraits(std::string_view text) { return parseTable(kPortraitShape, text); }

core::Result<core::PortraitTable> loadPortraits(const std::string& path) { return loadTable(kPortraitShape, path); }

core::Result<core::PortraitTable> parseBusts(std::string_view text) { return parseTable(kBustShape, text); }

core::Result<core::PortraitTable> loadBusts(const std::string& path) { return loadTable(kBustShape, path); }

}  // namespace fanren::io
