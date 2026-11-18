#pragma once
// 历练地 data/lilian/<site>.json 的读取器（伙伴改版，docs/interfaces-companions.md 3.12）。
//
// 一个文件一处，形如：
//   {"id": "jindi",                                   // 必填，与文件名相同
//    "name_key": "lilian.jindi.name",                 // 必填，名字的文案 key
//    "unlock_flag": "ch07.chujindi",                  // 必填，置了才进得去
//    "entry": {"map": "lilian_jindi_waiwei",          // 必填，入口图
//              "spawn": "rukou"},                     // 可选，入口图上的出生点；不写 = 缺省出生点
//    "maps": ["lilian_jindi_waiwei", "..."],          // 必填、非空、不重复、都以 lilian_ 开头、含入口图
//    "tiers": [{"max_realm": "QiRefining13"},         // 必填，1–3 档，max_realm 用境界名
//              {"max_realm": "FoundationEarly"},      //   （与编成、遭遇表同一套，io::parseRealmName），
//              {"max_realm": "FoundationMid"}],       //   从低到高、严格递增
//    "note": "..."}                                   // 可选，给人看的
//
// 这里只查**形状**（必填项、类型、表外字段、上面那几条约束）。引用存不存在——图文件、旗标、
// 文案 key、每一档的编成——归门禁 tools/validate.py；文案 key 另由 Application::init 对一遍。
#include <map>
#include <string>

#include "core/Result.h"
#include "core/model/Lilian.h"

namespace fanren::io {

// 历练图的图名前缀（契约 3.12：图名以 lilian_ 开头）。
inline constexpr const char* kLilianMapPrefix = "lilian_";

// 读一个文件。失败时 error 写明文件与字段。
[[nodiscard]] core::Result<core::LilianSite> loadLilianSiteFile(const std::string& path);

// 读整个目录（只读这一层的 .json，按文件名排序）。目录不存在 = 一处历练也没有，不算错；
// 不是目录、遍历出错、任何一份写坏了，整份判失败（不读进一半）。
[[nodiscard]] core::Result<std::map<std::string, core::LilianSite>> loadLilianSites(const std::string& dir);

}  // namespace fanren::io
