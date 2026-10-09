#pragma once
// JSON 文本存档：save_version + 校验和 + GameState 全字段。
// 校验和不匹配、版本号不认识都判失败，不静默接受（防手改作弊污染 10 小时计时
// 验收，见方案 2.2）。
#include <map>
#include <string>

#include "core/Result.h"
#include "core/model/Types.h"

namespace fanren::io {

// 当前存档 schema 版本。每次 GameState 的可持久化字段发生不兼容变化，先在这里
// +1，再在 SaveFile.cpp 的迁移表里补一条 "旧版本 -> 旧版本+1" 的转换函数——
// 不要直接改旧版本的序列化代码，否则旧存档就再也读不回来了。
// 2: 加入掌天瓶、灵田、四艺熟练度、资质
// 3: 加入队伍（GameState::party）。第 3 章起同伴要持久存在。
// 4: maxHp / maxMp 改由境界给出基准（core/rules/Realm.h 的 realmMaxHp/realmMaxMp）。
//    字段本身一个也没增减，**变的是同一个 maxHp 在同一个境界下该是多少**——
//    一份炼气三层的 v3 档里它是 10，到了 v4 该是 60。这同样是不兼容变化，
//    所以照规矩升版本，并且这是本作**第一条真的做事的迁移**（1→2、2→3 都是
//    空操作）。为什么落在迁移而不是每次读档都按境界重算，见 SaveFile.cpp
//    migrations() 里那条注释。
// 5: 加入已习得法术（GameState::learnedMagics）。第 4 章韩立开始学法术，
//    「他学会了火弹术」这件事从此要能落盘（技术债 G-4 的落点）。
//    v4 档里本就没有这一项，读不到就保持空表，所以 4→5 迁移是**空操作**——
//    但仍然登记，理由与 1→2、2→3 同：第 2 章已经证明「空操作」与「忘了登记」
//    在行为上天差地别，后者会让每一份老档都被判成「无法识别的存档版本号」。
// 6: 加入剧情给的境界上限（GameState::realmCap，第 4 章二次整改·技术债 G-14）。
//    **这一条迁移真的做事**：老档里没有上限，读进来按下面 legacyRealmCap 的口径补上，
//    不是一律给凡人（那会把每一份老档锁死在当前境界以下的剧情里），也不是只取当前境界
//    （第 2 章段五之后、还没按到三层的老档会被封在二层，一直卡到第 4 章节点 1）。
// 7: 加入已揭开的破绽（GameState::knownWeaknesses，八方旅人化改造，docs/octopath-battle.md 2.5）。
//    v6 档里本就没有这一项，读不到就保持空表——**旧档 = 全都不知道**，这正是它该有的语义：
//    那会儿战斗里还没有破绽这回事。所以 6→7 迁移是空操作，但照规矩登记。
//    存档里写的是类别的中文名（"wild_wolf": ["拳", "剑"]），不写位掩码：位序是实现细节，
//    哪天调了位序，写数字的老档会悄悄把「剑」读成「刀」。
// 8: 加入野外遭遇的计数器（GameState::encounter，docs/interfaces-octo-encounters.md）。
//    v7 档里没有这一项，读不到就是全 0——**旧档 = 从没遇过**，那会儿野外还没有遭遇。
//    7→8 迁移是空操作，照规矩登记。不存它的话，读一次档就把当天的遭遇上限清零。
// 9: 加入跌落之前的最高境界（GameState::formerRealm，第 9 章 E1）。
//    v8 档缺此项即凡人（从没跌过），8→9 迁移是空操作，照规矩登记。
inline constexpr int kSaveVersion = 9;

// v5 及更早的存档读进来时，境界上限取多少（5→6 迁移的全部内容，公开出来单独测）。
//
// **上限 = max(当前境界, 由已有剧情旗标推出的上限)**（协调者 2026-09-23 裁决）：
//   · `ch01.koujue_received` 置位 → 炼气一层。这是 scripts/ch01/shenshougu_koujue.lua
//     授完口诀之后置的那个旗标，现行脚本在同一处 realm.cap(一层)；
//   · `ch02.koujue_ceng` 的值 v（1–13）→ 炼气 v 层。scripts/ch02/ceng2.lua、ceng3.lua
//     置 2、3，现行脚本在同一处 realm.cap(二层 / 三层)；
//   · 第 4 章那三次升境是脚本**直接改境界**，已经由「当前境界」覆盖，不另列旗标；
//   · 什么都没有（新开局的空档）→ 凡人。
// 这张旗标表是**老档的补救**，不是上限的来源：上限的来源是剧情脚本（realm.cap /
// realm.advance），新档一律把它明明白白地存着。以后的章节不必来这里加行。
[[nodiscard]] rules::Realm legacyRealmCap(rules::Realm realm,
                                          const std::map<std::string, int>& flags);

[[nodiscard]] core::Result<bool> saveGame(const core::GameState& state, const std::string& path);
[[nodiscard]] core::Result<core::GameState> loadGame(const std::string& path);

}  // namespace fanren::io
