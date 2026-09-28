#pragma once
// 支线任务加载：data/quests/<id>.json → core::Quest（契约 docs/interfaces-p3-ch05.md 1.2、1.6）。
//
// 主线任务不在这里读：它由目标链原地生成（io/DataLoader.cpp 读 data/objectives/ 的那一趟），
// 所以 data/quests/ 里只许 kind = "side"。
//
// 这里只查**形状**：必填字段、类型、表外字段、谓词三种写法、非空与成对。引用是否存在
// （文案 key、旗标登记、物品 id、地图对象）归 tools/validate.py 规则 23——函数签名只拿到
// 一个文件，没有全仓的索引可查，与 loadTileMap 同一个分工。
#include <string>
#include <vector>

#include "core/Result.h"
#include "core/model/Types.h"

namespace fanren::io {

// 读一个支线文件。失败时 error 写明文件与字段。
[[nodiscard]] core::Result<core::Quest> loadQuestFile(const std::string& path);

// 读整个目录（递归，按路径排序）。目录不存在 = 0 条支线，不算错；
// 两个文件同一个 id 判失败，写明两份文件（不是后者覆盖前者）。
[[nodiscard]] core::Result<std::vector<core::Quest>> loadQuests(const std::string& questsDir);

}  // namespace fanren::io
