#pragma once
// 物品与法术说明的 game 层（第 7 章可玩性改版·第一期，路 M，docs/ch07-foundation.md 3.3）。
//
// 规则在 core/rules/ItemInfo.h（纯函数，全由数据生成）；这里做三件事：
//   1. 扫 maps/ 建「哪张图的宝箱、采集点里有它」的索引（buildItemSpots），连同集齐奖励一起；
//   2. 从 Application 上拿配方、商店、编成、用词，转调规则层（itemInfoOf / magicInfoOf）；
//   3. 画那一整块说明：主菜单物品页、法术页与商店共用一个画法（drawItemDetail / drawMagicDetail），
//      各面板不各写一份——四行的次序、标签色、分隔线一处定。
#include <map>
#include <string>
#include <vector>

#include "core/model/Exploration.h"
#include "core/model/Types.h"
#include "core/rules/ItemInfo.h"
#include "engine/Engine.h"

namespace fanren::game {

class Application;

// 扫 assetRoot/maps/*.tmj：宝箱的 loot、采集点的 item 里每一样，记下那张图（同图同种只记一次）；
// 再把每张图的集齐奖励（data/exploration）记成 Reward。载不进来的图跳过（门禁管它）。
[[nodiscard]] rules::ItemSpots buildItemSpots(const std::string& assetRoot, const core::GameData& data,
                                              const std::map<std::string, core::ExplorationReward>& rewards);

// 剧透闸用的当前章：章节表里第一章没演完的那一章（全演完了就是最后一章）；与此刻所在地图的章号取大的。
// 存档里的 GameState::chapter 不跟着剧情走，不能用它。
[[nodiscard]] int currentChapter(Application& app);

// 这一阶段的用词（凡人篇的「气力」「碎银」，game/Wording.h）。
[[nodiscard]] rules::InfoWords infoWords(const core::GameState& state);

// 带剧透闸（只列已到章节的来源、已得到的配方）与已结识伙伴喜恶的那一份。
[[nodiscard]] rules::ItemInfo itemInfoOf(Application& app, const std::string& itemId, int herbAge = 0);
[[nodiscard]] rules::MagicInfo magicInfoOf(Application& app, const core::Magic& magic);

// 得物卡片、战斗选单那一行：有效果就是效果行的第一条（itemInfoOf），没有为空串。
[[nodiscard]] std::string itemEffectLine(Application& app, const std::string& itemId, int herbAge = 0);
// 战斗选单的说明栏：物品的全部效果连成一行（rules::effectSummary）、法术的类别与各条效用（rules::magicSummary）。
[[nodiscard]] std::string itemEffectSummary(Application& app, const std::string& itemId);
[[nodiscard]] std::string magicEffectSummary(Application& app, const core::Magic& magic);

// 说明块里的一行：label 为空 = 不带标签的正文（风味文字）；rule = 一条分隔线（label、text 不读）。
struct DetailRow {
    std::string label;
    std::string text;
    bool rule = false;
};
// 物品说明块的行（纯，可测）：效果、用途、来源、品阶与价值各一行（同类几条用「；」连），空的那类不写；
// 然后一条分隔线、原来的风味文字（flavor 为空就不画线）。
[[nodiscard]] std::vector<DetailRow> itemDetailRows(const core::GameData& data, const rules::ItemInfo& info,
                                                    const std::string& flavor);
// 法术说明块：类别、消耗、效用（几条用「；」连），分隔线，风味文字。
[[nodiscard]] std::vector<DetailRow> magicDetailRows(const core::GameData& data, const rules::MagicInfo& info,
                                                     const std::string& flavor);

// 一整块说明：名字（按品阶上色）与各行。
struct DetailBlock {
    std::string title;
    engine::Color color{};
    std::vector<DetailRow> rows;
};
// 物品：名字（灵草带年份）、四行、分隔线、原来的风味文字；extra 非空时接在风味文字后面另起一段
//（物品页的「按确认使用」那一句）。查不到物品时整块是空的。
[[nodiscard]] DetailBlock itemDetailBlock(Application& app, const std::string& itemId, int herbAge,
                                          const std::string& extra = {});
[[nodiscard]] DetailBlock magicDetailBlock(Application& app, const core::Magic& magic);

// 选中的那一样不变就不重算（一样东西的来源要翻全部商店、编成、配方，几毫秒，不该每帧算一遍）。
// 面板进栈时 clear 一次：背包、章节变了说明跟着变。
class DetailCache {
public:
    [[nodiscard]] const DetailBlock& item(Application& app, const std::string& itemId, int herbAge,
                                          const std::string& extra = {});
    [[nodiscard]] const DetailBlock& magic(Application& app, const core::Magic& magic);
    void clear() { key_.clear(); }

private:
    std::string key_;
    DetailBlock block_;
};

// 画说明块：顶上一行名字，下面逐行（标签金色、正文纸色、风味文字暗一档）；画不下的截断。
void drawDetailBlock(Application& app, const engine::Rect& area, const DetailBlock& block, int fontSize = 18);

}  // namespace fanren::game
