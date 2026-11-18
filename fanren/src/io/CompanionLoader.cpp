#include "io/CompanionLoader.h"

#include <algorithm>
#include <filesystem>
#include <initializer_list>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "io/JsonUtil.h"

namespace fanren::io {
namespace {

namespace fs = std::filesystem;
using nlohmann::json;

// 表外字段一律报错：拼错一个 until_flag 会让她一辈子停在第一个角色上，而那不会自己暴露。
[[nodiscard]] bool firstUnknownKey(const json& object, std::initializer_list<const char*> known,
                                   std::string& unknown) {
    for (auto it = object.begin(); it != object.end(); ++it) {
        const bool listed = std::any_of(known.begin(), known.end(),
                                        [&it](const char* key) { return it.key() == key; });
        if (!listed) {
            unknown = it.key();
            return true;
        }
    }
    return false;
}

// 可选的字符串：没写保持空；写了就得是字符串。
[[nodiscard]] bool readOptionalString(const json& object, const char* key, std::string& out) {
    if (!object.contains(key)) return true;
    if (!object[key].is_string()) return false;
    out = object[key].get<std::string>();
    return true;
}

[[nodiscard]] bool readStringList(const json& node, std::vector<std::string>& out) {
    if (!node.is_array()) return false;
    for (const json& item : node) {
        if (!item.is_string() || item.get_ref<const std::string&>().empty()) return false;
        out.push_back(item.get<std::string>());
    }
    return true;
}

[[nodiscard]] bool readRoles(const json& node, const std::string& where, std::vector<core::CompanionRole>& out,
                             std::string& why) {
    if (!node.is_array() || node.empty()) {
        why = where + " 的 roles：应当是非空数组";
        return false;
    }
    for (std::size_t i = 0; i < node.size(); ++i) {
        const json& item = node[i];
        const std::string at = where + " 的 roles[" + std::to_string(i) + "]";
        std::string unknown;
        if (!item.is_object()) {
            why = at + "：应当是对象";
            return false;
        }
        if (firstUnknownKey(item, {"role", "until_flag"}, unknown)) {
            why = at + "：有不认识的字段 " + unknown + "（只认 role / until_flag）";
            return false;
        }
        core::CompanionRole role;
        if (!item.contains("role") || !item["role"].is_string() || item["role"].get<std::string>().empty()) {
            why = at + " 的 role：应当是非空字符串";
            return false;
        }
        role.roleId = item["role"].get<std::string>();
        if (!readOptionalString(item, "until_flag", role.untilFlag)) {
            why = at + " 的 until_flag：应当是字符串";
            return false;
        }
        out.push_back(std::move(role));
    }
    return true;
}

[[nodiscard]] bool readGifts(const json& object, const std::string& where, core::Companion& out, std::string& why) {
    if (!object.contains("gifts")) return true;
    const json& gifts = object["gifts"];
    std::string unknown;
    if (!gifts.is_object() || firstUnknownKey(gifts, {"like", "dislike"}, unknown)) {
        why = where + " 的 gifts：应当是只有 like / dislike 的对象";
        return false;
    }
    if ((gifts.contains("like") && !readStringList(gifts["like"], out.giftsLike)) ||
        (gifts.contains("dislike") && !readStringList(gifts["dislike"], out.giftsDislike))) {
        why = where + " 的 gifts：like / dislike 应当是物品 id 的数组";
        return false;
    }
    return true;
}

[[nodiscard]] bool parseCompanion(const json& node, const std::string& where, core::Companion& out,
                                  std::string& why) {
    if (!node.is_object()) {
        why = where + "：顶层应当是对象";
        return false;
    }
    std::string unknown;
    if (firstUnknownKey(node, {"id", "roles", "met_flag", "field_skill", "chase", "duo", "gifts", "note"},
                        unknown)) {
        why = where + "：有不认识的字段 " + unknown;
        return false;
    }
    for (const char* key : {"id", "met_flag"}) {
        if (!node.contains(key) || !node[key].is_string() || node[key].get<std::string>().empty()) {
            why = where + " 的 " + key + "：应当是非空字符串";
            return false;
        }
    }
    out.id = node["id"].get<std::string>();
    out.metFlag = node["met_flag"].get<std::string>();
    if (!node.contains("roles")) {
        why = where + "：缺少 roles";
        return false;
    }
    if (!readRoles(node["roles"], where, out.roles, why)) return false;
    std::string note;
    if (!readOptionalString(node, "field_skill", out.fieldSkill) || !readOptionalString(node, "duo", out.duo) ||
        !readOptionalString(node, "note", note)) {
        why = where + "：field_skill / duo / note 应当是字符串";
        return false;
    }
    if (node.contains("chase")) {
        if (!node["chase"].is_boolean()) {
            why = where + " 的 chase：应当是 true / false";
            return false;
        }
        out.chase = node["chase"].get<bool>();
    }
    return readGifts(node, where, out, why);
}

std::vector<fs::path> listCompanionFiles(const fs::path& dir) {
    std::vector<fs::path> files;
    std::error_code ec;
    if (!fs::exists(dir, ec) || !fs::is_directory(dir, ec)) return files;
    for (auto it = fs::recursive_directory_iterator(dir, fs::directory_options::skip_permission_denied, ec);
         it != fs::recursive_directory_iterator(); it.increment(ec)) {
        if (ec) break;
        std::error_code typeEc;
        if (it->is_regular_file(typeEc) && it->path().extension() == ".json") files.push_back(it->path());
    }
    // 排序：重复的报错要写明「哪两个文件」，不能跟着文件系统的次序在两台机器上报出两句话。
    std::sort(files.begin(), files.end());
    return files;
}

}  // namespace

core::Result<core::Companion> loadCompanionFile(const std::string& path) {
    using Out = core::Result<core::Companion>;
    core::Result<json> parsed = detail::readJsonFile(fs::path(path));
    if (!parsed) return Out::failure(parsed.error);
    core::Companion companion;
    std::string why;
    if (!parseCompanion(parsed.value, path, companion, why)) return Out::failure(why);
    return Out::success(std::move(companion));
}

core::Result<std::map<std::string, core::Companion>> loadCompanions(const std::string& companionsDir) {
    using Out = core::Result<std::map<std::string, core::Companion>>;
    std::map<std::string, core::Companion> companions;
    std::map<std::string, std::string> fileOfId;
    std::map<std::string, std::string> fileOfRole;
    for (const fs::path& file : listCompanionFiles(fs::path(companionsDir))) {
        core::Result<core::Companion> one = loadCompanionFile(file.string());
        if (!one) return Out::failure(one.error);
        if (const auto seen = fileOfId.find(one.value.id); seen != fileOfId.end()) {
            return Out::failure("伙伴 id 重复：" + one.value.id + "，见 " + seen->second + " 与 " + file.string());
        }
        fileOfId.emplace(one.value.id, file.string());
        for (const core::CompanionRole& role : one.value.roles) {
            const auto [seen, fresh] = fileOfRole.emplace(role.roleId, file.string());
            // 同一位伙伴里写两遍也算：先后两段不会是同一个角色表。
            if (!fresh) {
                return Out::failure("角色 " + role.roleId + " 挂在两处伙伴名下，见 " + seen->second + " 与 " +
                                    file.string());
            }
        }
        companions.emplace(one.value.id, std::move(one.value));
    }
    return Out::success(std::move(companions));
}

}  // namespace fanren::io
