#include "io/RecipeLoader.h"

#include <filesystem>
#include <set>
#include <string>
#include <utility>

#include <nlohmann/json.hpp>

#include "io/JsonUtil.h"

namespace fanren::io {
namespace {

using nlohmann::json;

// 难度的量程。规则层 successChance 把它夹进 [0, 100] 再算（见 Crafting.cpp），
// 所以 difficulty = 900 与 100 完全等价：两张数值截然不同的方子在游戏里一模一样，
// 而写数据的人以为自己造了一张难上九倍的方子。
//
// 熟练度门槛的量程直接取 rules::kMaxProficiency，不在这里另写一个字面量。
constexpr int kMaxDifficulty = 100;

[[nodiscard]] int readInt(const json& node, const char* key, int fallback) {
    if (!node.contains(key) || !node[key].is_number_integer()) return fallback;
    return node[key].get<int>();
}

[[nodiscard]] bool readBool(const json& node, const char* key, bool fallback) {
    if (!node.contains(key) || !node[key].is_boolean()) return fallback;
    return node[key].get<bool>();
}

[[nodiscard]] std::string readString(const json& node, const char* key) {
    if (!node.contains(key) || !node[key].is_string()) return {};
    return node[key].get<std::string>();
}

// 「这个 key 写了，但类型不对」。缺字段与写错类型是两件事：前者该退到缺省值，
// 后者该当场喊出来。不分开的话，"difficulty": "很难" 会被读成 0，变成一张
// 谁都炼得成的方子，而没有任何人会发现。
[[nodiscard]] bool presentButNotInt(const json& node, const char* key) {
    return node.contains(key) && !node[key].is_number_integer();
}

}  // namespace

bool craftKindFromString(const std::string& text, rules::CraftKind& out) {
    if (text == "alchemy") {
        out = rules::CraftKind::Alchemy;
        return true;
    }
    if (text == "talisman") {
        out = rules::CraftKind::Talisman;
        return true;
    }
    if (text == "forge") {
        out = rules::CraftKind::Forge;
        return true;
    }
    if (text == "formation") {
        out = rules::CraftKind::Formation;
        return true;
    }
    return false;
}

const char* craftKindToString(rules::CraftKind kind) {
    // 不加 default:，好让日后添第五艺时由 /W4（C4061/C4062）喊出来这里漏了一项。
    switch (kind) {
        case rules::CraftKind::Alchemy:   return "alchemy";
        case rules::CraftKind::Talisman:  return "talisman";
        case rules::CraftKind::Forge:     return "forge";
        case rules::CraftKind::Formation: return "formation";
    }
    return "";
}

core::Result<rules::Recipe> loadRecipe(const std::string& path) {
    using Out = core::Result<rules::Recipe>;

    core::Result<json> parsed = detail::readJsonFile(path);
    if (!parsed) return Out::failure(parsed.error);
    const json& root = parsed.value;
    if (!root.is_object()) return Out::failure("配方必须是对象：" + path);

    rules::Recipe recipe;
    recipe.id = readString(root, "id");
    if (recipe.id.empty()) return Out::failure("配方缺少 id：" + path);

    // name 是要上屏的（rules::makeLog 会把它拼进「《金疮药方》成」那一句），
    // 缺了就会印出一对空书名号。
    recipe.name = readString(root, "name");
    if (recipe.name.empty()) {
        return Out::failure("配方 " + recipe.id + " 缺少 name（它要直接上屏）：" + path);
    }

    const std::string kindText = readString(root, "kind");
    if (!craftKindFromString(kindText, recipe.kind)) {
        // 认不出来一律报错，**不退到炼丹**：四艺的失败损耗策略各不相同
        //（炼丹尽毁、制符保住妖丹、炼器折半、布阵原样收回），一张 kind 写错的
        // 制符方子退到炼丹，会在失败时把玩家的妖丹一并烧掉，而账面上看不出来。
        return Out::failure("配方 " + recipe.id + " 的 kind 不是 alchemy / talisman / forge / "
                            "formation 之一（现为 \"" + kindText + "\"）：" + path);
    }

    recipe.productId = readString(root, "productId");
    if (recipe.productId.empty()) {
        return Out::failure("配方 " + recipe.id + " 缺少 productId：" + path);
    }

    for (const char* key : {"productCount", "difficulty", "requiredProficiency"}) {
        if (presentButNotInt(root, key)) {
            return Out::failure("配方 " + recipe.id + " 的 " + key + " 不是整数：" + path);
        }
    }
    recipe.productCount = readInt(root, "productCount", 1);
    recipe.difficulty = readInt(root, "difficulty", 0);
    recipe.requiredProficiency = readInt(root, "requiredProficiency", 0);

    if (recipe.productCount <= 0) {
        // 产出 0 份的方子 canCraft 判「残缺不全」，但那句话说的是「方子有问题」，
        // 出现在玩家面前时已经太晚——这是数据错，该在加载时就说。
        return Out::failure("配方 " + recipe.id + " 的 productCount 必须为正（现为 " +
                            std::to_string(recipe.productCount) + "）：" + path);
    }
    // 难度越界。规则层把它夹进 [0, 100] 再算，所以 difficulty = 900 不会报错，
    // 只会与 100 完全等价——两张数值截然不同的方子在游戏里一模一样，
    // 而写数据的人以为自己造了一张难上九倍的方子。上下界都要卡。
    if (recipe.difficulty < 0 || recipe.difficulty > kMaxDifficulty) {
        return Out::failure("配方 " + recipe.id + " 的 difficulty 越界（须在 0-" +
                            std::to_string(kMaxDifficulty) + " 之间，现为 " +
                            std::to_string(recipe.difficulty) + "）：" + path);
    }
    // 熟练度门槛同理，量程取规则层的 kMaxProficiency：写成 500 的方子在规则层
    // 不会报错，只会永远开不了工（熟练度封顶 100），玩家看到的是一张
    // 「火候未到：此方需熟练度 500，现有 100」的永久禁用项。
    if (recipe.requiredProficiency < 0 ||
        recipe.requiredProficiency > rules::kMaxProficiency) {
        return Out::failure("配方 " + recipe.id + " 的 requiredProficiency 越界（须在 0-" +
                            std::to_string(rules::kMaxProficiency) + " 之间，现为 " +
                            std::to_string(recipe.requiredProficiency) + "）：" + path);
    }
    // 门闸旗标（契约 docs/interfaces-p3-ch07.md 1.4）：缺省没有门闸；写了就得是非空字符串。
    // 空串或数字悄悄收下，这张方子就成了「谁都看得见」，而写数据的人以为它锁着。
    // 旗标登没登记归门禁（字段名归一化后以 _flag 结尾，check_data_references 那一条）。
    if (root.contains("requireFlag")) {
        recipe.requireFlag = readString(root, "requireFlag");
        if (recipe.requireFlag.empty()) {
            return Out::failure("配方 " + recipe.id + " 的 requireFlag 必须是非空字符串：" + path);
        }
    }

    if (!root.contains("inputs") || !root["inputs"].is_array()) {
        return Out::failure("配方 " + recipe.id + " 缺少 inputs 数组：" + path);
    }
    for (const json& node : root["inputs"]) {
        if (!node.is_object()) {
            return Out::failure("配方 " + recipe.id + " 的 inputs 里有非对象条目：" + path);
        }
        rules::Ingredient need;
        need.itemId = readString(node, "itemId");
        if (need.itemId.empty()) {
            return Out::failure("配方 " + recipe.id + " 有材料缺少 itemId：" + path);
        }
        for (const char* key : {"count", "minAge"}) {
            if (presentButNotInt(node, key)) {
                return Out::failure("配方 " + recipe.id + " 的材料 " + need.itemId + " 的 " +
                                    key + " 不是整数：" + path);
            }
        }
        need.count = readInt(node, "count", 1);
        need.minAge = readInt(node, "minAge", 0);
        need.catalyst = readBool(node, "catalyst", false);
        if (need.count <= 0) {
            // count <= 0 在规则层被当作「不需要这一味」（isRequired），于是
            // 一张所有材料都写成 0 的方子就是凭空出产物——与「材料不够也能开工」
            // 是同一类经济漏洞，只是来路是数据而不是判定。
            return Out::failure("配方 " + recipe.id + " 的材料 " + need.itemId +
                                " 的 count 必须为正（现为 " + std::to_string(need.count) +
                                "）：" + path);
        }
        if (need.minAge < 0) {
            return Out::failure("配方 " + recipe.id + " 的材料 " + need.itemId +
                                " 的 minAge 为负：" + path);
        }
        recipe.inputs.push_back(std::move(need));
    }
    if (recipe.inputs.empty()) {
        return Out::failure("配方 " + recipe.id + " 的 inputs 是空的（不耗材料就是凭空出产物）：" +
                            path);
    }

    return Out::success(std::move(recipe));
}

core::Result<std::map<std::string, rules::Recipe>> loadRecipes(const std::string& recipesRoot,
                                                               const core::GameData& data) {
    using Out = core::Result<std::map<std::string, rules::Recipe>>;
    namespace fs = std::filesystem;

    std::map<std::string, rules::Recipe> all;
    std::error_code ec;
    if (!fs::exists(recipesRoot, ec)) {
        // 目录不存在按 0 张处理，与 loadBattles / loadShops 同口径。
        return Out::success(std::move(all));
    }

    std::map<std::string, std::string> sourceOf;
    for (const fs::directory_entry& entry : fs::recursive_directory_iterator(recipesRoot, ec)) {
        if (ec) break;
        if (!entry.is_regular_file() || entry.path().extension() != ".json") continue;

        auto one = loadRecipe(entry.path().string());
        if (!one) return Out::failure(one.error);
        const std::string file = entry.path().string();
        const rules::Recipe& recipe = one.value;

        // ---- 引用校验 ----
        // 产出与材料都要在物品册上查得到。悬空引用的症状分两种，都很难追：
        //   · 产出悬空 —— 炼成之后背包里多出一件 GameData 查不到的东西，
        //     商店与背包面板只能把原始 id 摆给玩家看；
        //   · 材料悬空 —— 这张方子永远配不出来，而禁用理由里写的是一个
        //     玩家在游戏里从没见过的 id。
        if (data.findItem(recipe.productId) == nullptr) {
            return Out::failure("配方 " + recipe.id + " 的产出 " + recipe.productId +
                                " 不在物品册上：" + file);
        }
        for (const rules::Ingredient& need : recipe.inputs) {
            if (data.findItem(need.itemId) == nullptr) {
                return Out::failure("配方 " + recipe.id + " 的材料 " + need.itemId +
                                    " 不在物品册上：" + file);
            }
        }

        const auto existing = sourceOf.find(recipe.id);
        if (existing != sourceOf.end()) {
            return Out::failure("配方 id 重复：" + recipe.id + "，见 " + existing->second +
                                " 与 " + file);
        }
        sourceOf.emplace(recipe.id, file);
        all.emplace(recipe.id, std::move(one.value));
    }

    return Out::success(std::move(all));
}

}  // namespace fanren::io
