#include "io/CgLoader.h"

#include <string>
#include <utility>

#include <nlohmann/json.hpp>

#include "io/StrictJson.h"

namespace fanren::io {
namespace {

using nlohmann::json;
using Out = core::Result<CgTable>;

Out fail(const std::string& where, const std::string& what) { return Out::failure("cg.json 的 " + where + "：" + what); }

bool isSha256(const json& v) {
    if (!v.is_string()) return false;
    const std::string& s = v.get_ref<const std::string&>();
    if (s.size() != 64) return false;
    for (const char c : s) {
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
    }
    return true;
}

bool endsWithPng(const std::string& name) {
    return name.size() > 4 && name.compare(name.size() - 4, 4, ".png") == 0;
}

}  // namespace

core::Result<CgTable> parseCgTable(std::string_view text) {
    auto parsed = detail::parseStrictJson(text);
    if (!parsed) return Out::failure("cg.json " + parsed.error);
    const json& root = parsed.value;
    if (!root.is_object()) return Out::failure("cg.json：顶层应当是对象");
    const json* entries = nullptr;
    for (const auto& [key, value] : root.items()) {
        if (key == "_doc") {
            if (!value.is_string()) return fail(key, "应当是字符串");
        } else if (key == "cg") {
            if (!value.is_object()) return fail(key, "应当是对象（事件图 id → {\"file\": …}）");
            entries = &value;
        } else if (key == "files") {
            if (!value.is_object()) return fail(key, "应当是对象（文件名 → sha256）");
            for (const auto& [name, digest] : value.items()) {
                if (!isSha256(digest)) return fail(key + "." + name, "应当是 64 位小写十六进制的 sha256");
            }
        } else {
            return fail(key, "认不出的字段");
        }
    }
    if (entries == nullptr) return fail("cg", "缺少");
    CgTable table;
    for (const auto& [id, entry] : entries->items()) {
        const std::string where = "cg." + id;
        if (!entry.is_object()) return fail(where, "应当是对象（只有 file 一项）");
        std::string file;
        for (const auto& [field, value] : entry.items()) {
            if (field != "file") return fail(where + "." + field, "认不出的字段");
            if (!value.is_string() || value.get_ref<const std::string&>().empty()) {
                return fail(where + ".file", "应当是非空字符串");
            }
            file = value.get<std::string>();
        }
        if (file.empty()) return fail(where + ".file", "缺少");
        if (!endsWithPng(file) || file.find('/') != std::string::npos || file.find('\\') != std::string::npos) {
            return fail(where + ".file", "应当是 assets/art/cg/ 下的一个 .png 文件名（不带目录）");
        }
        table.emplace(id, std::move(file));
    }
    return Out::success(std::move(table));
}

core::Result<CgTable> loadCgTable(const std::string& path) {
    auto text = detail::readTextFile(path);
    if (!text) return Out::failure(text.error);
    auto table = parseCgTable(text.value);
    if (!table) return Out::failure(path + "：" + table.error);
    return table;
}

}  // namespace fanren::io
