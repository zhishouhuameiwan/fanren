#pragma once
// 配方加载：data/recipes/<类别>/<id>.json → rules::Recipe。
//
// 与 BattleLoader / ShopLoader 同一口径：一文件一张方子，重复 id 判失败而不是
// 后者覆盖前者，目录不存在按 0 张处理。
//
// **判定一个字也不在这里算。** 成功率、熟练度成长、失败损耗全在
// core/rules/Crafting.h 里；本层只把 JSON 翻成 Recipe 结构体，再把写坏的数据
// 挡在加载那一刻。规则层是纯函数，它拿到什么就算什么——一份 difficulty 写成
// 900 的方子它照样会给出一个夹在 [5, 95] 的成功率，不会有任何人喊一声。
//
// 引用校验（材料 id 与产出 id 是否真的存在于 data/items）单独走 loadRecipes 的
// GameData 那一版：loadRecipe 只拿得到一个路径，没有物品表可查。这与
// io/DataLoader.h 把跨文件核对留给 tools/validate.py 是同一条分界，区别在于
// 配方的引用**运行期就要用**（缺料提示要报出物品名），所以这一条在加载时做。
#include <map>
#include <string>

#include "core/Result.h"
#include "core/model/Types.h"
#include "core/rules/Crafting.h"

namespace fanren::io {

// 技艺名 → 枚举。认不出来时返回 false 而不是退到炼丹：一张 kind 写错的方子
// 退到炼丹，就会静默地按炼丹的失败策略（材料尽毁）去扣制符的妖丹。
[[nodiscard]] bool craftKindFromString(const std::string& text, rules::CraftKind& out);

// 枚举 → data 里写的那个词。与上面那张表是同一份，反向查。
[[nodiscard]] const char* craftKindToString(rules::CraftKind kind);

// 读取单张配方。**结构性**校验：id / name / kind / productId / productCount /
// difficulty / requiredProficiency / inputs 一项不合规就判失败，error 写明是哪个
// 文件的哪一项。不查物品表（见文件头）。
[[nodiscard]] core::Result<rules::Recipe> loadRecipe(const std::string& path);

// 扫描整个目录（递归），并**核对每一条引用**：inputs 的 itemId 与 productId 都
// 必须在 data.items 里查得到。
//
// 为什么引用校验在这里而不是留给门禁：缺料提示要报出物品名（「青风草 需 2，
// 现有 0」），拿不到条目就只能把原始 id 摆给玩家看。悬空引用在加载时喊一声，
// 比让它变成屏幕上一行 herb_qingfeng_cao 好。
[[nodiscard]] core::Result<std::map<std::string, rules::Recipe>> loadRecipes(
    const std::string& recipesRoot, const core::GameData& data);

}  // namespace fanren::io
