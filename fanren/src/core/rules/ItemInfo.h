#pragma once
// 物品与法术说明的四行式（第 7 章可玩性改版·第一期「地基包」路 M，docs/ch07-foundation.md 3.3）。
//
// 2026-11-20 用户试玩第 7 章：「物品说明也不够」。原来的说明只有一段风味文字（descKey），玩家拿到东西
// 不知道它回多少血、能入哪张丹方、哪儿还能弄到。这里把这几件事**全由数据生成**，不手写文案：
//
//   效果　回复气血 N、回复法力 N、解毒；符箓写施展哪门法术、类别、威力或削几点架势、不耗法力、不能蓄劲；
//         兵刃写多出哪一种普攻类别；灵草写年份。
//   用途　能入哪张配方（配方名、要几份、在哪种炉案上炼）；已结识的伙伴里谁喜欢、谁不喜。
//   来源　哪家店有售、哪张配方炼得出、哪种妖兽掉、哪张图的宝箱或采集点有、哪条支线的酬谢；都没有写「剧情所得」。
//   品阶　品阶名与阶数；基准价，或「不可买卖」。
//
// 原来的 descKey 一个字不改，界面上照旧接在这四行后面。
//
// **纯函数**：配方、商店、编成、地图上的宝箱与采集点一律由调用方递进来。配方、商店、编成在 Application 上；
// 宝箱与采集点要读 maps/，由 game 层在头一次要用时扫一遍建成 ItemSpots（game/ItemInfoIndex.h），本层不碰文件。
// 字样一律取 data/text/ui_iteminfo.json 的 ui.iteminfo.*（GameData::lookupText），随阶段换说法的几个词
//（凡人篇的「气力」「碎银」）由调用方经 InfoWords 给。
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "core/model/Types.h"
#include "core/rules/Crafting.h"
#include "core/rules/Economy.h"

namespace fanren::rules {

// 一件东西在哪张图上捡得到。
struct ItemSpot {
    enum class Kind { Chest, Gather, Reward };   // 宝箱、采集点、集齐那张图的奖励
    Kind kind = Kind::Chest;
    std::string mapId;
    std::string mapName;   // 地名（已查好文案）；空 = 回显 mapId
    int chapter = 0;       // 那张图的章号（剧透闸用）
};
// 物品 id → 它在哪些图上（同一张图同一种只记一次，次序 = 扫图的次序）。
using ItemSpots = std::map<std::string, std::vector<ItemSpot>>;

// 来源与用途要查的几张表。哪一张为 nullptr 就当它是空的。
struct ItemInfoSources {
    const std::map<std::string, Recipe>* recipes = nullptr;
    const std::map<std::string, Shop>* shops = nullptr;
    const std::map<std::string, core::BattleSetup>* battles = nullptr;
    const ItemSpots* spots = nullptr;
    // 剧透闸用的「当前章」。0 = 取 state->chapter。存档里的 GameState::chapter 并不跟着剧情走（第 7 章的存档里
    // 还是 1），game 层按章节表的 done_flag 推出真正的当前章填在这里（game::currentChapter）。
    int chapter = 0;
};

// 随阶段换说法的词（game/Wording.h）：凡人篇是「气力」「碎银」。缺省是修仙篇的说法。
struct InfoWords {
    std::string hp = "气血";
    std::string mp = "法力";
    std::string currency = "灵石";
};

struct ItemInfo {
    std::vector<std::string> effects;   // 效果（可多条；没有效果的东西是空的）
    std::vector<std::string> uses;      // 用途
    std::vector<std::string> sources;   // 来源（至少一条：都没有时是「剧情所得」）
    std::string value;                  // 品阶与价值（恒有）
};

// 一件东西的四行说明。itemId 查不到时四样全空。
//
// state 可为 nullptr（单测遍历全部物品时）：那时不设剧透闸，伙伴的喜恶不写（没有「已结识」可言），配方不论得没得到都列。
// 给了 state 时（章号取 sources.chapter，为 0 才取 state->chapter）：配方只列已经得到的（Recipe::requireFlag 已置或没有门闸）；来源里的商店、妖兽、宝箱采集、支线
// 只列章号不超过 state->chapter 的（剧透闸：第 7 章的玩家不该在说明里读到第 13 章的坊市）；全被闸掉了
// 也写「剧情所得」。herbAge 是这一堆灵草的年份（背包按年份分堆），0 = 不写年份、只写上限。
[[nodiscard]] ItemInfo describeItem(const core::GameData& data, const ItemInfoSources& sources,
                                    const core::GameState* state, const InfoWords& words,
                                    const std::string& itemId, int herbAge = 0);

// 效果行连成一行（战斗选单的说明栏、得物卡片的副行用）：「回复气血 30 · 解毒」。没有效果是空串。
[[nodiscard]] std::string effectSummary(const ItemInfo& info);

struct MagicInfo {
    std::string category;              // 类别：「火」「金、毒」；看破、削架势、辅助各有一个说法（恒有）
    std::string cost;                  // 耗法力 N（恒有）
    std::vector<std::string> lines;    // 威力与目标、蓄劲后的变化、下毒、特殊效果……
};

// 一门法术的说明。缺省 words 是修仙篇的说法。
[[nodiscard]] MagicInfo describeMagic(const core::GameData& data, const core::Magic& magic,
                                      const InfoWords& words);

// 法术的效果行连成一行（战斗选单的说明栏用）：「火 · 威力 12 · 蓄劲连发，至多 4 发」。
[[nodiscard]] std::string magicSummary(const MagicInfo& info);

// 品阶名（凡品 / 良品 / 上品 / 珍品 / 极品，与得物卡片 game::gradeTier 的五档同一个分法）。
[[nodiscard]] std::string gradeName(const core::GameData& data, int grade);

// 把模板里的 {名字} 换成值；没给的占位原样留着（画面上一眼看出缺了哪一个）。
[[nodiscard]] std::string fillTemplate(std::string text,
                                       const std::vector<std::pair<std::string, std::string>>& values);

// 商店 id 推章号：chNN_ 打头的取 NN，别的（village_general_store 之类）为 0 = 不设闸。
[[nodiscard]] int shopChapter(const std::string& shopId);

}  // namespace fanren::rules
