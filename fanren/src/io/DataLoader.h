#pragma once
// 数据加载：把 data/ 下的 JSON 与 Tiled .tmj 地图解析进 core 的静态模型。
// 失败一律用 core::Result 表达，不跨模块边界抛异常（见 docs/interfaces.md 0 节）。
#include <string>

#include "core/Result.h"
#include "core/model/Types.h"

namespace fanren::io {

// 境界名 → 枚举（"QiRefining2" → Realm::QiRefining2）；认不出返回 false、out 不动。
// 数据文件里的境界一律写枚举标识符，对照表只在 DataLoader.cpp 里有一份。
[[nodiscard]] bool parseRealmName(const std::string& name, rules::Realm& out);

// 递归扫描 dataRoot（通常是 "data"）下的 items/ magics/ roles/ text/ 四个子目录，
// 填充 core::GameData 的同名字段。
//
// one-file-per-entry：items/magics/roles 下每个 JSON 文件是一条记录（可以嵌套子目
// 录，如 data/items/pills/zhuji_dan.json），这样多个关卡代理并行新增数据不会互相
// 冲突。text/ 例外：每个 JSON 文件是一个 "文案 key -> 中文文本" 的大对象。
//
// 校验：
//   - 同一分类内 id 重复 → 失败，error 写明两份文件的路径（不是后者覆盖前者）。
//   - 条目缺 id 或 name → 失败，error 写明文件路径与缺失字段。
//   - 其余字段缺失一律用 core::Item / core::Magic / core::RoleTemplate 的默认值。
//   - text/ 内出现重复 key，或某个 value 不是字符串 → 失败。
//   - 破势与蓄劲那几个字段（docs/octopath-battle.md 第 5 节）写了就得写对：类别名只许
//     十种之一、兵刃类只许兵刃五类、toughness ≥ 1、actions ≥ 1、charge 四项齐全、
//     有架势就得有破绽、magic 的 boost 只许 hits / power —— 错了报出文件与字段。
// 四个子目录若不存在则视为该分类 0 条记录，不算失败；dataRoot 本身不存在才失败。
//
// 另读 objectives/（目标链）与 quests/（支线任务，io/QuestLoader.h）：每个目标链文件
// 同一趟另产出一条主线任务，填进 GameData::quests（契约 docs/interfaces-p3-ch05.md 1.5）。
// 同章两条目标链、支线 id 重复、支线 id 与目标链撞车，都判失败。
[[nodiscard]] core::Result<core::GameData> loadGameData(const std::string& dataRoot);

// 解析 Tiled JSON 地图（.tmj）。校验规则见 docs/map_spec.md 第 3/4/5 节；
// 缺图层、图层顺序错、缺地图属性、对象矩形未对齐网格等一律判失败，error 里写明
// 是哪张图（文件名）、哪一项不合规。
//
// 需要跨文件核对的规则（portal 双向可达、npc.role_id 是否存在于 data/roles、
// script 路径是否存在、*_flag/*_key 是否已登记、BFS 连通性……）不在本函数职责内：
// 函数签名只拿到一个 tmj 路径，没有 GameData 或全地图索引可用，留给
// tools/validate.py 统一做。
[[nodiscard]] core::Result<core::TileMap> loadTileMap(const std::string& tmjPath);

}  // namespace fanren::io
