#include "io/PortraitLoader.h"

#include <array>
#include <cstddef>
#include <string>
#include <utility>

#include <nlohmann/json.hpp>

#include "io/StrictJson.h"

namespace fanren::io {
namespace {

using nlohmann::json;
using Out = core::Result<core::PortraitTable>;

Out fail(const std::string& where, const std::string& what) {
    return Out::failure("portraits.json 的 " + where + "：" + what);
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
std::string readSheet(const json& entry, const std::string& where, core::PortraitSheet& out) {
    const auto bad = [&where](const std::string& field, const std::string& what) {
        return "portraits.json 的 " + where + (field.empty() ? "" : "." + field) + "：" + what;
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
    return {};
}

}  // namespace

core::Result<core::PortraitTable> parsePortraits(std::string_view text) {
    auto parsed = detail::parseStrictJson(text);
    if (!parsed) return Out::failure("portraits.json " + parsed.error);
    const json& root = parsed.value;
    if (!root.is_object()) return Out::failure("portraits.json：顶层应当是对象");

    const json* portraits = nullptr;
    for (const auto& [key, value] : root.items()) {
        if (key == "_doc") {
            if (!value.is_string()) return fail(key, "应当是字符串");
        } else if (key == "portraits") {
            if (!value.is_object()) return fail(key, "应当是对象（角色 id → 头像）");
            portraits = &value;
        } else {
            return fail(key, "认不出的字段");
        }
    }
    if (portraits == nullptr) return fail("portraits", "缺少");

    core::PortraitTable table;
    for (const auto& [roleId, entry] : portraits->items()) {
        core::PortraitSheet sheet;
        if (std::string error = readSheet(entry, "portraits." + roleId, sheet); !error.empty()) {
            return Out::failure(std::move(error));
        }
        table.emplace(roleId, std::move(sheet));
    }
    return Out::success(std::move(table));
}

core::Result<core::PortraitTable> loadPortraits(const std::string& path) {
    auto text = detail::readTextFile(path);
    if (!text) return Out::failure(text.error);
    auto table = parsePortraits(text.value);
    if (!table) return Out::failure(path + "：" + table.error);
    return table;
}

}  // namespace fanren::io
