#pragma once
// 伙伴数据 data/companions/<id>.json 的读取器（伙伴改版，docs/interfaces-companions.md 2.1）。
//
// 一个文件一位伙伴，形如：
//   {"id": "nangong_wan",
//    "roles": [{"role": "baiyi_shaonv", "until_flag": "ch07.nangong"}, {"role": "nangong_wan"}],
//    "met_flag": "ch07.nangong", "field_skill": "break_seal", "chase": true,
//    "duo": "duo_nangong_zhuque", "gifts": {"like": [...], "dislike": [...]}, "note": "..."}
//
// 这里只查**形状**：必填项（id、roles、met_flag）、类型、表外字段、roles 每条有 role；
// 再加一条跨文件的：同一个角色 id 不许属于两位伙伴（companionOfRole 靠它答得唯一）。
// 引用存不存在（角色、法术、物品、旗标）归门禁 tools/validate.py——函数只拿得到这一个目录，
// 与 QuestLoader、PathActionLoader 同一个分工。
#include <map>
#include <string>

#include "core/Result.h"
#include "core/model/Types.h"

namespace fanren::io {

// 读一个伙伴文件。失败时 error 写明文件与字段。
[[nodiscard]] core::Result<core::Companion> loadCompanionFile(const std::string& path);

// 读整个目录（递归，按路径排序）。目录不存在 = 一位伙伴也没有，不算错；
// 两个文件同一个 id、同一个角色 id 挂在两位伙伴名下，都判失败并写明两份文件。
[[nodiscard]] core::Result<std::map<std::string, core::Companion>> loadCompanions(
    const std::string& companionsDir);

}  // namespace fanren::io
