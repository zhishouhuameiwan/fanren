#include "core/rules/Objectives.h"

#include <cstddef>
#include <deque>
#include <map>

namespace fanren::rules {
namespace {

// 这一步算不算做完了。
//
// doneFlag 为空的条目一律**当作没做完**，而不是当作做完了：空 doneFlag 是数据
// 写漏了，而漏写的那一步如果被当成已完成，目标行会径直跳过它——玩家于是在某
// 一步上永远得不到提示，且没有任何迹象表明发生过这件事。停在这一步上至少是
// 看得见的（而门禁那边 tools/validate.py 不许 doneFlag 为空，这里只是兜住
// 「数据绕过了门禁」这一种情形，不替它修）。
[[nodiscard]] bool finished(const core::Objective& step, const core::GameState& state) {
    if (step.doneFlag.empty()) return false;
    return state.flag(step.doneFlag) != 0;
}

// 进度 = **走得最远的那一步**，返回它之后那一格的下标。
//
// 不是「第一条没做完的」。两者在正常推进下逐格相同（旗标就是一条条顺着置的），
// 分歧只在「跳着置位」的存档上，而那种存档在本工程里是**一等公民**：
// saves/ 下那两份检查点是手测的正门（saves/README.md），它们由 tools/mksave
// 生成，只带章末旗标（ch01.done / ch02.done / ch03.done）与本章前几个节点的，
// 中间那些一个都没有。
//
// 按「第一条没做完的」算，读第 4 章检查点站在演武场上，目标行会写着
// 「在家中听三叔把话说完」、并指着韩家村——那正是这一批要修的毛病本身，
// 只是换了个形状。
[[nodiscard]] std::size_t progressIndex(const std::vector<core::Objective>& chain,
                                        const core::GameState& state) {
    std::size_t next = 0;
    for (std::size_t i = 0; i < chain.size(); ++i) {
        if (finished(chain[i], state)) next = i + 1;
    }
    return next;
}

}  // namespace

const core::Objective* currentObjective(const std::vector<core::Objective>& chain,
                                        const core::GameState& state) {
    const std::size_t next = progressIndex(chain, state);
    return next < chain.size() ? &chain[next] : nullptr;
}

std::vector<const core::Objective*> completedObjectives(const std::vector<core::Objective>& chain,
                                                        const core::GameState& state) {
    std::vector<const core::Objective*> done;
    const std::size_t next = progressIndex(chain, state);
    for (std::size_t i = 0; i < next; ++i) {
        // 只列真的做过的那些。跳章的存档会在这里留出空档（中间几步的旗标
        // 压根没置过），那是实情——告示板宁可显示一段有缺口的来路，
        // 也不要替玩家编出他没走过的那几步。
        if (finished(chain[i], state)) done.push_back(&chain[i]);
    }
    return done;
}

std::vector<PortalLink> portalLinksOf(const core::TileMap& map) {
    std::vector<PortalLink> links;
    for (const core::MapObject& object : map.objects) {
        if (object.type != "portal") continue;
        const std::string target = object.property("target_map");
        // 没写目标图的门是地图写坏了（门禁规则会拦），这里只是不让它进图里当一条
        // 通往「空串」的边——那会让 BFS 把空串当成一张图。
        if (target.empty()) continue;
        links.push_back(PortalLink{map.id, object.name, object.position, target,
                                   object.property("require_flag")});
    }
    return links;
}

bool portalOpen(const PortalLink& link, const core::GameState& state) {
    return link.requireFlag.empty() || state.flag(link.requireFlag) != 0;
}

const PortalLink* nextPortalToward(const std::vector<PortalLink>& links,
                                   const std::string& fromMap, const std::string& goalMap,
                                   const core::GameState& state) {
    if (fromMap.empty() || goalMap.empty() || fromMap == goalMap) return nullptr;

    // 每张图记下「从起点出发、第一步走的是哪道门」。起点自己不记（它不需要第一步）。
    // 广度优先保证第一次到达即最短；links 按固定顺序遍历，平手时取靠前的那一条。
    std::map<std::string, const PortalLink*> firstHop;
    firstHop.emplace(fromMap, nullptr);
    std::deque<std::string> frontier{fromMap};

    while (!frontier.empty()) {
        const std::string here = frontier.front();
        frontier.pop_front();
        const PortalLink* inherited = firstHop.at(here);

        for (const PortalLink& link : links) {
            if (link.fromMap != here || !portalOpen(link, state)) continue;
            if (firstHop.count(link.targetMap) != 0) continue;
            const PortalLink* hop = inherited != nullptr ? inherited : &link;
            if (link.targetMap == goalMap) return hop;
            firstHop.emplace(link.targetMap, hop);
            frontier.push_back(link.targetMap);
        }
    }
    return nullptr;
}

}  // namespace fanren::rules
