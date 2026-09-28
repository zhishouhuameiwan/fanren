#pragma once
// 主线目标的选型：在一条目标链与一份存档之间，回答「现在该做哪一步」。
//
// core 层硬约束：不含 SDL / JSON / Lua / 文件 IO，只在已解析的结构体上运算。
// 于是这条规则可以不开窗口直接单测——而它必须能被单测：一个选错步的目标行
// 会把玩家一路指到不该去的地方，那种错在画面上看起来和「没做目标系统」一样，
// 只是更糟。
#include <string>
#include <vector>

#include "core/model/Types.h"

namespace fanren::rules {

// 当前该做的那一步：**走得最远的那一条已完成的**之后那一步。
//
// 判据只有这一条，没有第二套进度。理由是目标链的 doneFlag 就是剧情脚本本来
// 就要置的完成旗标（脚本演完才置，半途 return 置不上），所以「演到哪儿」与
// 「显示到哪儿」不可能岔开——它们本来就是同一个事实。
//
// 为什么是「走得最远的」而不是「第一条没做完的」：见 .cpp 里 progressIndex 的
// 注释——检查点存档是跳着置旗标的，而那是本工程手测的正门。
//
// 全部完成时返回 nullptr（通关了，没有下一步）。返回的指针指进 chain，
// chain 一变就失效：与 ListView::selectedItem 同一个口径，即取即用。
[[nodiscard]] const core::Objective* currentObjective(const std::vector<core::Objective>& chain,
                                                      const core::GameState& state);

// 已经完成的那些，按链上的顺序。告示板用它列「做过什么」。
//
// 不返回「未来那些」：那是剧透。玩家在告示板上该看到的是走过的路与眼下这一步，
// 不是一份通关流程表。
[[nodiscard]] std::vector<const core::Objective*> completedObjectives(
    const std::vector<core::Objective>& chain, const core::GameState& state);

// ---------------------------------------------------------------------------
// 跨图指路（第 4 章复验 N-7）
// ---------------------------------------------------------------------------
//
// 目标在别的图上时，世界层从前只高亮**直接通到目标图**的那一道门。第 3 章终局站在
// 谷外，第 4 章第一步的目标图是七玄门各堂，中间隔着神手谷、炼骨崖、七玄门、彩霞山、
// 青牛镇、韩家村、山下镇——一道门都不亮，玩家只能挨张图去撞。
//
// 改成：按门的连通关系在「图的图」上找最短路，高亮这张图上通往目标的**下一道门**。
// 纯函数、不碰文件，门的清单由调用方递进来（Application::portalLinks 从 maps/ 读）。

// 一道门：从哪张图的哪个对象，通到哪张图，要什么钥匙。
struct PortalLink {
    std::string fromMap;
    std::string portalName;   // 地图对象名，如 portal_to_shenshougu
    core::Point position;     // 那道门在 fromMap 上的格子
    std::string targetMap;
    std::string requireFlag;  // 空串 = 不设闸
};

// 一张已解析地图上的全部门，按对象表的顺序。
[[nodiscard]] std::vector<PortalLink> portalLinksOf(const core::TileMap& map);

// 这道门此刻过不过得去：与 WorldScene::tryStep 放不放人是同一条判据。
[[nodiscard]] bool portalOpen(const PortalLink& link, const core::GameState& state);

// 站在 fromMap 上、要去 goalMap，该走的**下一道门**；没有时返回 nullptr。
//
// 口径：
//   · 只走**此刻开着的门**。关着的门不算路——一条要穿过锁着的门才走得通的
//     「最短路」会把人领到一堵墙前面，比不指路更糟；与「闸门关着时不高亮」同一个理由。
//   · 按门的数目求最短（广度优先）；同样短的几条取链表里靠前的那一条，
//     于是同一份地图、同一份存档永远指同一道门，测试也就钉得住。
//   · fromMap == goalMap（目标就在脚下这张图上）、或者此刻根本走不到，返回 nullptr：
//     前者由目标标记负责，后者宁可一道门都不亮，也不瞎指。
//   · 只认门，不认「走出地图边缘回上级图」（can_leave_edge）：全 18 张图眼下一张都
//     没开这一项；哪天开了，把那条边也收进 links 即可，本函数不必改。
//
// 返回的指针指进 links，links 一变就失效，即取即用。
[[nodiscard]] const PortalLink* nextPortalToward(const std::vector<PortalLink>& links,
                                                 const std::string& fromMap,
                                                 const std::string& goalMap,
                                                 const core::GameState& state);

}  // namespace fanren::rules
