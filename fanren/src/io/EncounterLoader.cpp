#include "io/EncounterLoader.h"

#include <filesystem>
#include <utility>

#include <nlohmann/json.hpp>

#include "io/DataLoader.h"
#include "io/JsonUtil.h"

namespace fanren::io {
namespace {

using nlohmann::json;
using Table = rules::EncounterTable;

// 可选整数字段：不在就保持 out 不变；在了就必须是不小于 floor 的整数。
[[nodiscard]] bool readOptionalInt(const json& node, const char* key, int floor, int& out) {
    if (!node.contains(key)) return true;
    if (!node[key].is_number_integer() || node[key].get<int>() < floor) return false;
    out = node[key].get<int>();
    return true;
}

// 可选境界字段：不在就保持 out 不变；在了就必须是认得出的境界名。
[[nodiscard]] bool readOptionalRealm(const json& node, const char* key, rules::Realm& out) {
    if (!node.contains(key)) return true;
    return node[key].is_string() && parseRealmName(node[key].get<std::string>(), out);
}

[[nodiscard]] core::Result<rules::EncounterEntry> readEntry(
    const json& node, const std::string& where, const std::map<std::string, core::BattleSetup>& battles) {
    using Out = core::Result<rules::EncounterEntry>;
    if (!node.is_object()) return Out::failure(where + " 不是对象");
    rules::EncounterEntry entry;
    if (!node.contains("battleId") || !node["battleId"].is_string()) {
        return Out::failure(where + " 缺少 battleId");
    }
    entry.battleId = node["battleId"].get<std::string>();
    if (battles.find(entry.battleId) == battles.end()) {
        return Out::failure(where + " 的 battleId 在 data/battles 里不存在：" + entry.battleId);
    }
    // 权重 0 的一条永远抽不中：写它的人要的多半是「先关掉」，那就删掉这一条。
    if (!readOptionalInt(node, "weight", 1, entry.weight)) {
        return Out::failure(where + " 的 weight 必须是不小于 1 的整数");
    }
    if (!readOptionalRealm(node, "minRealm", entry.minRealm) ||
        !readOptionalRealm(node, "maxRealm", entry.maxRealm)) {
        return Out::failure(where + " 的 minRealm / maxRealm 不是认得出的境界名（写枚举标识符，如 QiRefining3）");
    }
    if (rules::toValue(entry.minRealm) > rules::toValue(entry.maxRealm)) {
        return Out::failure(where + " 的 minRealm 高于 maxRealm：这一条哪个境界都遇不上");
    }
    return Out::success(std::move(entry));
}

}  // namespace

core::Result<Table> loadEncounterTable(const std::string& path,
                                       const std::map<std::string, core::BattleSetup>& battles) {
    using Out = core::Result<Table>;

    core::Result<json> parsed = detail::readJsonFile(path);
    if (!parsed) return Out::failure(parsed.error);
    const json& root = parsed.value;
    if (!root.is_object()) return Out::failure("遭遇表必须是对象：" + path);

    Table table;
    if (!root.contains("id") || !root["id"].is_string() || root["id"].get<std::string>().empty()) {
        return Out::failure("遭遇表缺少 id：" + path);
    }
    table.id = root["id"].get<std::string>();

    // 步数区间与每日上限在表上是缺省值；地图上的遭遇区（map_spec 4.5）写了就以区为准。
    if (!readOptionalInt(root, "stepsMin", 1, table.stepsMin) ||
        !readOptionalInt(root, "stepsMax", 1, table.stepsMax) ||
        !readOptionalInt(root, "dailyCap", 1, table.dailyCap)) {
        return Out::failure("遭遇表 " + table.id + " 的 stepsMin / stepsMax / dailyCap 必须是正整数：" + path);
    }
    if (table.stepsMin > table.stepsMax) {
        return Out::failure("遭遇表 " + table.id + " 的 stepsMin 大于 stepsMax：" + path);
    }

    if (!root.contains("entries") || !root["entries"].is_array() || root["entries"].empty()) {
        return Out::failure("遭遇表 " + table.id + " 的 entries 必须是非空数组：" + path);
    }
    for (std::size_t i = 0; i < root["entries"].size(); ++i) {
        const std::string where = "遭遇表 " + table.id + " 的 entries[" + std::to_string(i) + "]";
        auto entry = readEntry(root["entries"][i], where, battles);
        if (!entry) return Out::failure(entry.error + "：" + path);
        table.entries.push_back(std::move(entry.value));
    }
    return Out::success(std::move(table));
}

core::Result<std::map<std::string, Table>> loadEncounterTables(
    const std::string& encountersRoot, const std::map<std::string, core::BattleSetup>& battles) {
    using Out = core::Result<std::map<std::string, Table>>;
    namespace fs = std::filesystem;

    std::map<std::string, Table> all;
    std::error_code ec;
    if (!fs::exists(encountersRoot, ec)) return Out::success(std::move(all));

    std::map<std::string, std::string> sourceOf;
    for (const fs::directory_entry& entry : fs::recursive_directory_iterator(encountersRoot, ec)) {
        if (ec) break;
        if (!entry.is_regular_file() || entry.path().extension() != ".json") continue;

        auto one = loadEncounterTable(entry.path().string(), battles);
        if (!one) return Out::failure(one.error);

        const std::string id = one.value.id;
        const auto existing = sourceOf.find(id);
        if (existing != sourceOf.end()) {
            return Out::failure("遭遇表 id 重复：" + id + "，见 " + existing->second + " 与 " +
                                entry.path().string());
        }
        sourceOf.emplace(id, entry.path().string());
        all.emplace(id, std::move(one.value));
    }
    return Out::success(std::move(all));
}

}  // namespace fanren::io
