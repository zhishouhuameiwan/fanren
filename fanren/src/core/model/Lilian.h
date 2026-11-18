#pragma once
// 历练地的定义（伙伴改版，docs/interfaces-companions.md 3.12）：data/lilian/<site>.json 一份一处。
//
// 读它的是 io/LilianLoader.h，存它的是 Application（不进 GameData：契约第 6 节「四路之间的几条线」）。
// 形状写在 LilianLoader.h 的开头；按境界取档、chance 抽签在 core/rules/Lilian.h。
#include <string>
#include <vector>

#include "core/rules/Realm.h"

namespace fanren::core {

// 一档：韩立的境界不高过 maxRealm 就落在这一档（各档按 maxRealm 从低到高排）。
// 第 i 档（从 1 数）的编成 id 是「foe.battle + _t<i>」。
struct LilianTier {
    rules::Realm maxRealm = rules::Realm::Mortal;
};

struct LilianSite {
    std::string id;                  // 与文件名相同
    std::string nameKey;             // 名字（文案 key），组队面板的标题
    std::string unlockFlag;          // 置了才进得去
    std::string entryMap;            // 入口图
    std::string entrySpawn;          // 入口图上的出生点；空 = 那张图的缺省出生点
    std::vector<std::string> maps;   // 用到的图（都以 lilian_ 开头，含入口图）
    std::vector<LilianTier> tiers;   // 按境界分的档，1–3 档
};

}  // namespace fanren::core
