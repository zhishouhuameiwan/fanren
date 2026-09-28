#pragma once
// 商店配置加载：data/shops/<id>.json → rules::Shop。
//
// 与 BattleLoader 同一口径：一文件一家店，重复 id 判失败而不是后者覆盖前者，
// 目录不存在按 0 家店处理（早期章节还没有店铺不该让整局加载失败）。
//
// 价格不在这里算。条目里的 price 只是数据作者写下的挂牌价，实际收付一律走
// rules::buyPrice / rules::sellPrice——加载器再算一遍就又多了一处计价实现，
// 而那正是这一轮要消掉的东西。
#include <map>
#include <string>

#include "core/Result.h"
#include "core/rules/Economy.h"

namespace fanren::io {

// 读取单家店铺。id 缺失、条目缺 itemId、倍率非法都判失败，error 写明是哪个
// 文件的哪一项。
[[nodiscard]] core::Result<rules::Shop> loadShop(const std::string& path);

// 扫描整个目录（递归）。
[[nodiscard]] core::Result<std::map<std::string, rules::Shop>> loadShops(
    const std::string& shopsRoot);

}  // namespace fanren::io
