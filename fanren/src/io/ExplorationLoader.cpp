#include "io/ExplorationLoader.h"

#include <algorithm>
#include <filesystem>
#include <initializer_list>
#include <set>
#include <system_error>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "io/JsonUtil.h"

namespace fanren::io {
namespace {

namespace fs = std::filesystem;
using nlohmann::json;

// 表外字段一律报错：拼错一个 done_flag，那张图就每次进来都再发一遍（或一次也不发），而那不会自己暴露。
[[nodiscard]] bool firstUnknownKey(const json& object, std::initializer_list<const char*> known, std::string& unknown) {
    for (auto it = object.begin(); it != object.end(); ++it) {
        const bool listed = std::any_of(known.begin(), known.end(), [&it](const char* key) { return it.key() == key; });
        if (!listed) {
            unknown = it.key();
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool readString(const json& node, const char* key, std::string& out) {
    if (!node.contains(key) || !node[key].is_string()) return false;
    out = node[key].get<std::string>();
    return !out.empty();
}

// 可选的整数：没写 = fallback；写了就得是 [low, high] 里的整数。
[[nodiscard]] bool readInt(const json& node, const char* key, int low, int high, int fallback, int& out) {
    if (!node.contains(key)) {
        out = fallback;
        return true;
    }
    if (!node[key].is_number_integer()) return false;
    const long long value = node[key].get<long long>();
    if (value < low || value > high) return false;
    out = static_cast<int>(value);
    return true;
}

[[nodiscard]] bool readItems(const json& node, const std::string& where, core::ExplorationReward& out,
                             std::string& why) {
    if (!node.contains("items")) return true;
    if (!node["items"].is_array()) {
        why = where + " 的 items：应当是数组";
        return false;
    }
    for (const json& item : node["items"]) {
        std::string unknown;
        if (!item.is_object()) {
            why = where + " 的 items：每一项都得是对象 {\"itemId\", \"count\"}";
            return false;
        }
        if (firstUnknownKey(item, {"itemId", "count"}, unknown)) {
            why = where + " 的 items 有表外字段 " + unknown;
            return false;
        }
        core::BagEntry entry;
        if (!readString(item, "itemId", entry.itemId)) {
            why = where + " 的 items：itemId 应当是非空字符串";
            return false;
        }
        if (!item.contains("count") || !readInt(item, "count", 1, kExplorationMaxItemCount, 1, entry.count)) {
            why = where + " 的 items：" + entry.itemId + " 的 count 应当是 1–" + std::to_string(kExplorationMaxItemCount) +
                  " 的整数";
            return false;
        }
        out.items.push_back(std::move(entry));
    }
    return true;
}

[[nodiscard]] bool parseReward(const json& node, const std::string& where, core::ExplorationReward& out,
                               std::string& why) {
    if (!node.is_object()) {
        why = where + "：maps 的每一项都得是对象";
        return false;
    }
    std::string unknown;
    if (firstUnknownKey(node, {"map", "done_flag", "spirit_stones", "items", "bond"}, unknown)) {
        why = where + " 有表外字段 " + unknown;
        return false;
    }
    if (!readString(node, "map", out.mapId)) {
        why = where + " 的 map：应当是非空字符串（图 id）";
        return false;
    }
    const std::string at = where + "（" + out.mapId + "）";
    if (!readString(node, "done_flag", out.doneFlag)) {
        why = at + " 的 done_flag：应当是非空字符串";
        return false;
    }
    if (!readInt(node, "spirit_stones", 0, kExplorationMaxStones, 0, out.spiritStones)) {
        why = at + " 的 spirit_stones：应当是 0–" + std::to_string(kExplorationMaxStones) + " 的整数";
        return false;
    }
    if (!readInt(node, "bond", 0, kExplorationMaxBond, 0, out.bond)) {
        why = at + " 的 bond：应当是 0–" + std::to_string(kExplorationMaxBond) + " 的整数";
        return false;
    }
    if (!readItems(node, at, out, why)) return false;
    if (out.spiritStones == 0 && out.items.empty() && out.bond == 0) {
        why = at + "：一样奖励都没写（spirit_stones、items、bond 至少一样）";
        return false;
    }
    return true;
}

}  // namespace

core::Result<std::map<std::string, core::ExplorationReward>> loadExplorationRewards(const std::string& dir) {
    using Out = core::Result<std::map<std::string, core::ExplorationReward>>;
    std::map<std::string, core::ExplorationReward> rewards;
    const fs::path root(dir);
    std::error_code ec;
    if (!fs::exists(root, ec)) {
        if (ec) return Out::failure("读不了集齐奖励目录 " + root.string() + "：" + ec.message());
        return Out::success(std::move(rewards));
    }
    if (!fs::is_directory(root, ec)) return Out::failure("集齐奖励目录 " + root.string() + " 不是目录");

    // 手动步进、每一步带 error_code（与 loadLilianSites 同一个写法）：range-for 的 ++ 会抛异常。
    std::vector<fs::path> files;
    for (fs::directory_iterator it(root, ec); !ec && it != fs::directory_iterator(); it.increment(ec)) {
        std::error_code typeEc;
        if (it->is_regular_file(typeEc) && it->path().extension() == ".json") files.push_back(it->path());
        if (typeEc) return Out::failure("读不了 " + it->path().string() + "：" + typeEc.message());
    }
    if (ec) return Out::failure("遍历集齐奖励目录 " + root.string() + " 出错：" + ec.message());
    std::sort(files.begin(), files.end());

    std::set<std::string> flags;
    for (const fs::path& file : files) {
        const std::string where = "集齐奖励 " + file.string();
        core::Result<json> parsed = detail::readJsonFile(file);
        if (!parsed) return Out::failure(parsed.error);
        const json& doc = parsed.value;
        std::string unknown;
        if (!doc.is_object()) return Out::failure(where + "：顶层应当是对象");
        if (firstUnknownKey(doc, {"note", "maps"}, unknown)) return Out::failure(where + " 有表外字段 " + unknown);
        if (doc.contains("note") && !doc["note"].is_string()) return Out::failure(where + " 的 note：应当是字符串");
        if (!doc.contains("maps") || !doc["maps"].is_array() || doc["maps"].empty()) {
            return Out::failure(where + " 的 maps：应当是非空数组");
        }
        for (const json& node : doc["maps"]) {
            core::ExplorationReward reward;
            std::string why;
            if (!parseReward(node, where, reward, why)) return Out::failure(why);
            if (rewards.count(reward.mapId) != 0) return Out::failure(where + "：图 " + reward.mapId + " 配了两遍");
            if (!flags.insert(reward.doneFlag).second) {
                return Out::failure(where + "：旗标 " + reward.doneFlag + " 给了两张图（每张图一面，各记各的）");
            }
            rewards.emplace(reward.mapId, std::move(reward));
        }
    }
    return Out::success(std::move(rewards));
}

}  // namespace fanren::io
