#include "io/ShopLoader.h"

#include <filesystem>
#include <set>
#include <string>
#include <utility>

#include <nlohmann/json.hpp>

#include "io/JsonUtil.h"

namespace fanren::io {
namespace {

using nlohmann::json;

[[nodiscard]] int readInt(const json& node, const char* key, int fallback) {
    if (!node.contains(key) || !node[key].is_number_integer()) return fallback;
    return node[key].get<int>();
}

[[nodiscard]] double readDouble(const json& node, const char* key, double fallback) {
    if (!node.contains(key) || !node[key].is_number()) return fallback;
    return node[key].get<double>();
}

[[nodiscard]] std::string readString(const json& node, const char* key) {
    if (!node.contains(key) || !node[key].is_string()) return {};
    return node[key].get<std::string>();
}

}  // namespace

core::Result<rules::Shop> loadShop(const std::string& path) {
    using Out = core::Result<rules::Shop>;

    core::Result<json> parsed = detail::readJsonFile(path);
    if (!parsed) return Out::failure(parsed.error);
    const json& root = parsed.value;
    if (!root.is_object()) return Out::failure("商店配置必须是对象：" + path);

    rules::Shop shop;
    shop.id = readString(root, "id");
    if (shop.id.empty()) return Out::failure("商店配置缺少 id：" + path);
    shop.nameKey = readString(root, "nameKey");

    shop.buyRate = readDouble(root, "buyRate", shop.buyRate);
    shop.sellRate = readDouble(root, "sellRate", shop.sellRate);
    if (shop.buyRate <= 0.0 || shop.sellRate < 0.0) {
        return Out::failure("商店 " + shop.id + " 的买卖倍率非法：" + path);
    }
    // 买价必须高于卖价，否则「买进来再卖回去」就是一台印钞机，而这种漏洞在
    // 玩家找到之前谁也不会发现。这条不变量写在 Economy.h 的 sellRate 注释里，
    // 这里是它在数据侧的闸门。
    if (shop.sellRate >= shop.buyRate) {
        return Out::failure("商店 " + shop.id + " 的 sellRate 不低于 buyRate，买进再卖回即可刷钱：" +
                            path);
    }

    if (root.contains("entries")) {
        if (!root["entries"].is_array()) {
            return Out::failure("商店 " + shop.id + " 的 entries 必须是数组：" + path);
        }
        std::set<std::string> seen;
        for (const json& node : root["entries"]) {
            if (!node.is_object()) {
                return Out::failure("商店 " + shop.id + " 的 entries 里有非对象条目：" + path);
            }
            rules::ShopEntry entry;
            entry.itemId = readString(node, "itemId");
            if (entry.itemId.empty()) {
                return Out::failure("商店 " + shop.id + " 有条目缺少 itemId：" + path);
            }
            // 同一件货列两次会在货架上出现两行一模一样的东西，买哪一行全看
            // 运气（库存各记各的）。这在数据里基本只会是复制粘贴的手误。
            if (!seen.insert(entry.itemId).second) {
                return Out::failure("商店 " + shop.id + " 的 entries 里 " + entry.itemId +
                                    " 重复：" + path);
            }
            entry.price = readInt(node, "price", entry.price);
            entry.stock = readInt(node, "stock", entry.stock);
            entry.restockDays = readInt(node, "restockDays", entry.restockDays);
            entry.restockAmount = readInt(node, "restockAmount", entry.restockAmount);
            entry.maxStock = readInt(node, "maxStock", entry.maxStock);

            // 库存这一组只有 -1 一个哨兵（无限库存 / 不设补货上限），别的负数
            // 一律是手误——而手误在这里不会自己暴露：rules 侧把**任何**负库存都
            // 当成无限供应（Economy.cpp 的 stock < 0），于是 "restockAmount": -5
            // 这种笔误会让一件限量珍品补过几个周期之后悄悄变成随便拿。
            // 与上面那道买卖倍率的闸门同一个理由：数据错要在加载时喊出来。
            if (entry.stock < -1 || entry.maxStock < -1 || entry.restockAmount < 0 ||
                entry.restockDays < 0 || entry.price < 0) {
                return Out::failure("商店 " + shop.id + " 的条目 " + entry.itemId +
                                    " 有非法的价格或库存数值（库存与上限只许 -1 作哨兵，"
                                    "其余一律非负）：" + path);
            }
            shop.entries.push_back(std::move(entry));
        }
    }

    return Out::success(std::move(shop));
}

core::Result<std::map<std::string, rules::Shop>> loadShops(const std::string& shopsRoot) {
    using Out = core::Result<std::map<std::string, rules::Shop>>;
    namespace fs = std::filesystem;

    std::map<std::string, rules::Shop> all;
    std::error_code ec;
    if (!fs::exists(shopsRoot, ec)) {
        // 目录不存在按 0 家店处理，与 loadBattles 同口径。
        return Out::success(std::move(all));
    }

    std::map<std::string, std::string> sourceOf;
    for (const fs::directory_entry& entry : fs::recursive_directory_iterator(shopsRoot, ec)) {
        if (ec) break;
        if (!entry.is_regular_file() || entry.path().extension() != ".json") continue;

        auto one = loadShop(entry.path().string());
        if (!one) return Out::failure(one.error);

        const std::string id = one.value.id;
        const auto existing = sourceOf.find(id);
        if (existing != sourceOf.end()) {
            return Out::failure("商店 id 重复：" + id + "，见 " + existing->second + " 与 " +
                                entry.path().string());
        }
        sourceOf.emplace(id, entry.path().string());
        all.emplace(id, std::move(one.value));
    }

    return Out::success(std::move(all));
}

}  // namespace fanren::io
