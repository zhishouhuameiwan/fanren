#pragma once
// 一张图「找齐」的奖励（docs/ch07-playtest-ui.md 第 4 节）：这张图上此刻在场的宝箱全开了、采集点都采过了，
// 就说一句、给一份。data/exploration/<章>.json 一份一章，读它的是 io/ExplorationLoader.h，存它的是 Application
// （与历练地同一个做法：不进 GameData）。怎么数、什么时候发在 game/Exploration.h。
#include <string>
#include <vector>

#include "core/model/Types.h"

namespace fanren::core {

struct ExplorationReward {
    std::string mapId;                // 哪张图
    std::string doneFlag;             // 发过了就置它（登记在 data/flags.json）：每张图只给一次，读档后不重复
    int spiritStones = 0;             // 钱（kSpiritStoneItemId 那件物品）
    std::vector<BagEntry> items;      // 现成的材料，各几件（herbAge 一律 0：灵草的年份按物品自己的口径落堆）
    int bond = 0;                     // 此刻上场的每一位伙伴各加多少默契（rules::addBond，不受每天的上限管）
};

}  // namespace fanren::core
