#pragma once
// 四种地图对象（宝箱、采集点、明雷、机关）与伙伴跟随的纯判定（伙伴改版，docs/interfaces-companions.md
// 2.4、3.4–3.7、3.11）。只看存档状态、对象状态与地图对象，不碰渲染、输入与 Application。
//
// WorldScene 的走位、交互与「画什么」问的都是这里：一个对象在不在场、占不占格、开没开，
// 走路的那条路与画面的那条路必须是同一个答案（与 npcVisible 同一个道理）。
//
// **对象状态一律由调用方递进来**（core::WorldObjectState）：正史里是存档那份，历练里是「这一趟」那份，
// 选哪一份是 Application::worldObjects() 的事，这里不认识历练。
#include <map>
#include <string>
#include <vector>

#include "core/model/Types.h"

namespace fanren::game::worldobj {

// 四种新对象的 type。
inline constexpr const char* kChest = "chest";
inline constexpr const char* kGather = "gather";
inline constexpr const char* kFoe = "foe";
inline constexpr const char* kGate = "gate";
[[nodiscard]] bool isWorldObjectType(const std::string& type);

// ---- 物品清单 ----
//
// 宝箱的 loot、采集点的 item 是同一种写法：`物品id` 或 `物品id:件数`，几样用逗号隔开
// （采集点通常只写一样）。件数缺省 1。写坏了（空项、件数不是正整数）整张清单作废，ok 为假：
// 门禁拦着这种写法，引擎这里只是不发半张清单。
struct ItemStack {
    std::string itemId;
    int count = 1;
};
[[nodiscard]] std::vector<ItemStack> parseItemList(const std::string& text, bool& ok);

// 背包装不装得下（契约 3.5「背包满了开不了、采不了」）。
//
// 这个游戏的背包**没有格数上限**（GameState::bag 是一张不限长的表，商店、剧情给物都不判满）。
// 所以「满」在这里只有一种意思：某一堆再加就要超过 kBagStackCap 件。取 999 是三位数的件数
// 刚好写得下；这是 C 路按「最小的做法」定的，在汇报里单列，契约改了口径改这一处。
inline constexpr int kBagStackCap = 999;
[[nodiscard]] bool bagCanTake(const core::GameState& state, const core::GameData& data,
                              const std::vector<ItemStack>& items);
// 发进背包：灵草按 data 里那一味的 herbAge 落堆（与剧情 give 同一个口径）。
void giveItems(core::GameState& state, const core::GameData& data, const std::vector<ItemStack>& items);

// ---- 在场与状态 ----

// require_flag（四种对象都可写）：没置就当这个对象不在——不画、不占格、按了不响。
[[nodiscard]] bool requirementMet(const core::GameState& state, const core::MapObject& object);

[[nodiscard]] bool chestOpened(const core::WorldObjectState& objects, const core::MapObject& chest);
// 采集点冷却中：采过、且离那一天还不到 respawn_days 天。
[[nodiscard]] bool gatherDepleted(const core::WorldObjectState& objects, const core::MapObject& gather, int today);
// 明雷此刻在不在：require_flag 满足，且没清掉、或写了 respawn_days 而日子已到。
[[nodiscard]] bool foePresent(const core::GameState& state, const core::WorldObjectState& objects,
                              const core::MapObject& foe);
[[nodiscard]] bool gateOpen(const core::WorldObjectState& objects, const core::MapObject& gate);

// 这个新对象此刻在不在场（画不画、按不按得出来）。不是四种新对象之一时返回 false。
[[nodiscard]] bool present(const core::GameState& state, const core::WorldObjectState& objects,
                           const core::MapObject& object);
// 占不占格：在场的宝箱、采集点、明雷占格；机关关着时占格、开了不占。
[[nodiscard]] bool blocks(const core::GameState& state, const core::WorldObjectState& objects,
                          const core::MapObject& object);

// ---- 明雷的运行时站位（巡逻与追击，docs/ch07-playtest-foes.md）----
//
// 写了 patrol 的明雷会离开图上那一格。它此刻站在哪、面朝哪边是**运行时**的事，不进存档：读档、换图都回原位
// （状态在 WorldScene 的 FoePatrol 里，game/FoePatrol.h）。下面凡是问「明雷在哪一格、面前是哪一格」的判定都收
// 一张站位表：明雷 id → 此刻的格与朝向。表里没有的明雷（不巡逻的、或者没传表 = nullptr）站在图上那一格、
// 面朝 facing 属性——这就是改版前「站着不动」的口径，旧的调用方不传表，答案与从前逐字相同。
struct FoePose {
    core::Point cell;
    int facing = 2;   // 同 GameState::facing
};
using FoePoses = std::map<std::string, FoePose>;
// 这只明雷此刻的站位：poses 里有它（按 id）就用表里的，否则是图上那一格、facing 属性。
[[nodiscard]] FoePose foePose(const core::MapObject& foe, const FoePoses* poses = nullptr);

// 这一格上第一个**在场**的那一类新对象（矩形命中，与 TileMap::objectAt 同一形状；明雷按它此刻那一格）；
// 没有则 nullptr。机关只认关着的（开了的机关按了不响）。
[[nodiscard]] const core::MapObject* presentAt(const core::GameState& state, const core::WorldObjectState& objects,
                                               const core::TileMap& map, core::Point cell, const std::string& type,
                                               const FoePoses* poses = nullptr);
// 这一格被哪个新对象挡着；不挡为 nullptr。明雷按它此刻那一格。
[[nodiscard]] const core::MapObject* blockerAt(const core::GameState& state, const core::WorldObjectState& objects,
                                               const core::TileMap& map, core::Point cell,
                                               const FoePoses* poses = nullptr);

// 明雷面前那一格（它此刻的格 + 此刻的朝向；facing 写错按朝下，与 NPC 同一口径）。
[[nodiscard]] core::Point foeFrontCell(const core::MapObject& foe, const FoePoses* poses = nullptr);
// 站在 cell 上会被谁看见：在场、面前那一格正是 cell 的第一个明雷；没有则 nullptr。
[[nodiscard]] const core::MapObject* foeWatching(const core::GameState& state, const core::WorldObjectState& objects,
                                                 const core::TileMap& map, core::Point cell,
                                                 const FoePoses* poses = nullptr);
// facing 属性的名字换成数字朝向（up 0 / right 1 / down 2 / left 3，写错按朝下）。与 WorldScene::facingFromName
// 同一张表；这里另写一份，是因为本文件不认识 WorldScene。
[[nodiscard]] int facingOfName(const std::string& name);

// ---- 伙伴 ----

using Companions = std::map<std::string, core::Companion>;

// 这位伙伴（不论用的是哪个角色 id）在不在队里——active 与否都算。
[[nodiscard]] bool companionInParty(const core::GameState& state, const Companions& companions,
                                    const std::string& companionId);
// 上场的伙伴：active 的伙伴按队伍次序的前 kFieldedCompanions 位，返回她们的角色 id。
// 不是伙伴的队员（傀儡、化身、曲魂）不算、也不占位（契约 3.1、3.7）。
// 与战斗那一边的 core::battle::kMaxCompanionsOnField 是同一条规矩的两处写法（地图上跟着走的就是战斗里上场的），
// WorldObjects.cpp 里有一道 static_assert 钉着两者相等；名册超过 3 人的那一期收成一个函数。
inline constexpr std::size_t kFieldedCompanions = 3;
[[nodiscard]] std::vector<std::string> fieldedCompanionRoles(const core::GameState& state,
                                                             const Companions& companions);
// 队里有没有人会这手场地技；有就返回那位队员**实际的角色 id**（GameState::party 里存的那一个，
// 脚本 party.add 定的，可能与 currentRole 推出来的不同），没有为空串。取名要用它：
// 旁白里的名字得是图上跟着的那个人。
[[nodiscard]] std::string partyFieldSkillRole(const core::GameState& state, const Companions& companions,
                                              const std::string& skill);
// 名册里（met_flag 已置）会这手的伙伴，按伙伴 id 排第一位；没有为空串。
[[nodiscard]] std::string rosterFieldSkillHolder(const core::GameState& state, const Companions& companions,
                                                 const std::string& skill);

// ---- 跟随 ----
//
// 主角走过的格子，最近的在前，至多 kFieldedCompanions 格。第 i 位跟随者站在 steps[i]；
// 足迹还不够长时，排不开的人叠在最后那一格上；一格足迹都没有时全体叠在主角格上（契约 3.7）。
struct FollowerPlace {
    core::Point cell;
    int facing = 2;
};
[[nodiscard]] std::vector<FollowerPlace> layoutFollowers(const std::vector<core::Point>& steps, core::Point hero,
                                                         int heroFacing, std::size_t count);

}  // namespace fanren::game::worldobj
