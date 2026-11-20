#pragma once
// 教学卡（data/tutorials/*.json）与见闻（data/lore/*.json）的读取（第一期地基包，docs/ch07-foundation.md 2.3、2.5）。
//
// 两种文件都是「顶层一条数据条目 + 一个数组」：
//   教学卡 {"id": "tutorials_ch07", "cards": [{id, title_key, body_keys, trigger, min_chapter, max_chapter}]}
//   见闻   {"id": "lore_ch07", "chapter": 7, "entries": [{id, category, title_key, body_key, unlock_flag, role_id}]}
// 只查形状（字段在不在、类型对不对、id 不重复、category 是四种之一、body_keys 至多三行）；
// 文案 key 在不在、旗标登不登记、role_id 有没有这个角色归测试与门禁在真树上查。
// 目录不存在 = 一条也没有，不算错。表外字段报错：拼错一个 unlock_flag 的条目会一辈子锁着，而那不会自己暴露。
#include <string>
#include <vector>

#include "core/Result.h"
#include "core/model/Types.h"

namespace fanren::io {

// 一张卡至多几行正文（2.3：至多三行）。
inline constexpr std::size_t kTutorialMaxBodyLines = 3;

[[nodiscard]] core::Result<std::vector<core::TutorialCard>> loadTutorialDir(const std::string& dir);
[[nodiscard]] core::Result<std::vector<core::LoreEntry>> loadLoreDir(const std::string& dir);

// 见闻的四种类别（2.5），按记事「见闻」页的排列次序。
[[nodiscard]] const std::vector<std::string>& loreCategories();

}  // namespace fanren::io
