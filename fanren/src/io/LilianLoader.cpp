#include "io/LilianLoader.h"

#include <algorithm>
#include <filesystem>
#include <initializer_list>
#include <set>
#include <system_error>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "core/rules/Lilian.h"
#include "io/DataLoader.h"
#include "io/JsonUtil.h"

namespace fanren::io {
namespace {

namespace fs = std::filesystem;
using nlohmann::json;

// 表外字段一律报错：拼错一个 unlock_flag 会让入口永远开着（或永远关着），而那不会自己暴露。
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

[[nodiscard]] bool readRequiredString(const json& node, const char* key, std::string& out) {
    if (!node.contains(key) || !node[key].is_string()) return false;
    out = node[key].get<std::string>();
    return !out.empty();
}

[[nodiscard]] bool startsWith(const std::string& text, const char* prefix) {
    return text.rfind(prefix, 0) == 0;
}

[[nodiscard]] bool readEntry(const json& node, const std::string& where, core::LilianSite& out,
                             std::string& why) {
    if (!node.contains("entry") || !node["entry"].is_object()) {
        why = where + " 的 entry：应当是对象 {\"map\", \"spawn\"}";
        return false;
    }
    const json& entry = node["entry"];
    std::string unknown;
    if (firstUnknownKey(entry, {"map", "spawn"}, unknown)) {
        why = where + " 的 entry 有表外字段 " + unknown;
        return false;
    }
    if (!readRequiredString(entry, "map", out.entryMap)) {
        why = where + " 的 entry.map：应当是非空字符串";
        return false;
    }
    if (entry.contains("spawn") && !readRequiredString(entry, "spawn", out.entrySpawn)) {
        why = where + " 的 entry.spawn：写了就得是非空字符串";
        return false;
    }
    return true;
}

[[nodiscard]] bool readMaps(const json& node, const std::string& where, core::LilianSite& out, std::string& why) {
    if (!node.contains("maps") || !node["maps"].is_array() || node["maps"].empty()) {
        why = where + " 的 maps：应当是非空数组";
        return false;
    }
    std::set<std::string> seen;
    for (const json& item : node["maps"]) {
        if (!item.is_string() || item.get_ref<const std::string&>().empty()) {
            why = where + " 的 maps：每一项都得是非空字符串";
            return false;
        }
        const std::string map = item.get<std::string>();
        // 契约 3.12：历练图是普通地图文件，图名以 lilian_ 开头。别的图（正史的图）不许列进来——
        // 那张图上对象的状态会记进「这一趟」，正史里开过的宝箱进来就成了关着的。
        if (!startsWith(map, kLilianMapPrefix)) {
            why = where + " 的 maps：" + map + " 不以 " + kLilianMapPrefix + " 开头";
            return false;
        }
        if (!seen.insert(map).second) {
            why = where + " 的 maps：" + map + " 写了两遍";
            return false;
        }
        out.maps.push_back(map);
    }
    if (seen.count(out.entryMap) == 0) {
        why = where + " 的 entry.map（" + out.entryMap + "）不在 maps 里";
        return false;
    }
    return true;
}

[[nodiscard]] bool readTiers(const json& node, const std::string& where, core::LilianSite& out,
                             std::string& why) {
    if (!node.contains("tiers") || !node["tiers"].is_array() || node["tiers"].empty() ||
        node["tiers"].size() > static_cast<std::size_t>(rules::kLilianMaxTiers)) {
        why = where + " 的 tiers：应当是 1–" + std::to_string(rules::kLilianMaxTiers) + " 项的数组";
        return false;
    }
    const json& tiers = node["tiers"];
    for (std::size_t i = 0; i < tiers.size(); ++i) {
        const std::string at = where + " 的 tiers[" + std::to_string(i) + "]";
        const json& item = tiers[i];
        std::string unknown;
        if (!item.is_object()) {
            why = at + "：应当是对象 {\"max_realm\"}";
            return false;
        }
        if (firstUnknownKey(item, {"max_realm"}, unknown)) {
            why = at + " 有表外字段 " + unknown;
            return false;
        }
        core::LilianTier tier;
        if (!item.contains("max_realm") || !item["max_realm"].is_string() ||
            !parseRealmName(item["max_realm"].get<std::string>(), tier.maxRealm)) {
            why = at + " 的 max_realm：应当是境界名（如 QiRefining13、FoundationEarly）";
            return false;
        }
        // 严格递增：两档同一个上限，后一档永远取不到；倒过来写，低境界会落进高档。
        if (!out.tiers.empty() && rules::toValue(tier.maxRealm) <= rules::toValue(out.tiers.back().maxRealm)) {
            why = at + " 的 max_realm 不高于上一档：各档应从低到高、严格递增";
            return false;
        }
        out.tiers.push_back(tier);
    }
    return true;
}

[[nodiscard]] bool parseSite(const json& node, const fs::path& path, core::LilianSite& out, std::string& why) {
    const std::string where = "历练地 " + path.string();
    if (!node.is_object()) {
        why = where + "：顶层应当是对象";
        return false;
    }
    std::string unknown;
    if (firstUnknownKey(node, {"id", "name_key", "unlock_flag", "entry", "maps", "tiers", "note"}, unknown)) {
        why = where + " 有表外字段 " + unknown;
        return false;
    }
    if (!readRequiredString(node, "id", out.id)) {
        why = where + " 的 id：应当是非空字符串";
        return false;
    }
    if (out.id != path.stem().string()) {
        why = where + " 的 id（" + out.id + "）与文件名不一致";
        return false;
    }
    if (!readRequiredString(node, "name_key", out.nameKey)) {
        why = where + " 的 name_key：应当是非空字符串（文案 key）";
        return false;
    }
    if (!readRequiredString(node, "unlock_flag", out.unlockFlag)) {
        why = where + " 的 unlock_flag：应当是非空字符串";
        return false;
    }
    if (node.contains("note") && !node["note"].is_string()) {
        why = where + " 的 note：应当是字符串";
        return false;
    }
    return readEntry(node, where, out, why) && readMaps(node, where, out, why) && readTiers(node, where, out, why);
}

}  // namespace

core::Result<core::LilianSite> loadLilianSiteFile(const std::string& path) {
    using Out = core::Result<core::LilianSite>;
    core::Result<json> parsed = detail::readJsonFile(fs::path(path));
    if (!parsed) return Out::failure(parsed.error);
    core::LilianSite site;
    std::string why;
    if (!parseSite(parsed.value, fs::path(path), site, why)) return Out::failure(why);
    return Out::success(std::move(site));
}

core::Result<std::map<std::string, core::LilianSite>> loadLilianSites(const std::string& dir) {
    using Out = core::Result<std::map<std::string, core::LilianSite>>;
    std::map<std::string, core::LilianSite> sites;
    const fs::path root(dir);
    std::error_code ec;
    if (!fs::exists(root, ec)) {
        if (ec) return Out::failure("读不了历练目录 " + root.string() + "：" + ec.message());
        return Out::success(std::move(sites));
    }
    if (!fs::is_directory(root, ec)) return Out::failure("历练目录 " + root.string() + " 不是目录");

    // 手动步进、每一步带 error_code（与 loadPortalLinks 同一个写法）：range-for 的 ++ 会抛异常。
    std::vector<fs::path> files;
    for (fs::directory_iterator it(root, ec); !ec && it != fs::directory_iterator(); it.increment(ec)) {
        std::error_code typeEc;
        if (it->is_regular_file(typeEc) && it->path().extension() == ".json") files.push_back(it->path());
        if (typeEc) return Out::failure("读不了 " + it->path().string() + "：" + typeEc.message());
    }
    if (ec) return Out::failure("遍历历练目录 " + root.string() + " 出错：" + ec.message());
    std::sort(files.begin(), files.end());

    for (const fs::path& file : files) {
        core::Result<core::LilianSite> one = loadLilianSiteFile(file.string());
        if (!one) return Out::failure(one.error);
        // id 与文件名相同，同一层目录里不会有两份同名的文件，所以不必再查 id 重复。
        sites.emplace(one.value.id, std::move(one.value));
    }
    return Out::success(std::move(sites));
}

}  // namespace fanren::io
