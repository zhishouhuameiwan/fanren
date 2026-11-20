#pragma once
// 每张图的收集进度与集齐奖励（docs/ch07-playtest-ui.md 第 3、4 节）。2026-11-20 用户试玩第 7 章：
// 「找东西没啥回报感」——地名下（目标框底下）显示「宝箱 2/3 · 采集 1/2」，一张图找齐给一份奖励。
//
// 数的口径：
//   · 只数此刻**在场**的宝箱、采集点（worldobj::present：require_flag 没置的不算，不画、不占格，也不进分母）；
//   · 宝箱「开了」看 openedChests；采集点「采过」看 gatheredDay 里有没有它——采过一次就一直算，
//     冷却好了又能采不把它退回去。gatheredDay 本来就进存档，所以「采过」读档后照样在，不另加字段；
//   · **历练里不显示、不发奖**：带 chance 的宝箱没抽中时直接记成「已开」（看上去是被人先开过的），数出来的
//     「2/3」是假的；那一趟的账出来就扔，每趟都能再集齐一次，奖励就成了刷的东西。历练的宝箱自带灵石，回报在那里。
#include <string>

#include "core/model/Types.h"

namespace fanren::game {

class Application;

namespace explore {

struct Tally {
    int chests = 0;      // 在场的宝箱
    int opened = 0;      // 其中开了的
    int gathers = 0;     // 在场的采集点
    int gathered = 0;    // 其中采过的

    // 这张图上有没有可数的东西（没有就不显示那一行）。
    [[nodiscard]] bool counted() const { return chests + gathers > 0; }
    [[nodiscard]] bool complete() const { return counted() && opened == chests && gathered == gathers; }
};

// 这张图此刻的进度（纯函数：只看存档状态、对象状态与地图对象）。
[[nodiscard]] Tally tally(const core::GameState& state, const core::WorldObjectState& objects, const core::TileMap& map);

// 世界层此刻要显示的进度：历练里、没有图时、图上没有可数的东西时都是空的（counted() 为假）。
[[nodiscard]] Tally currentTally(Application& app);

// 「宝箱 2/3 · 采集 1/2」；找齐了末尾再接「 · 已寻遍」。图上没有采集点就不写那一截，没有宝箱同理。
// 没有可数的东西时是空串。字样取 ui.world.explore.*（data/text/ui_world_objects.json）。
[[nodiscard]] std::string progressLine(Application& app, const Tally& tally);

// 开箱、采集之后调一次：这张图配了集齐奖励（data/exploration）、还没发过、此刻正好找齐了——置那面旗、
// 发灵石与材料（各弹一张得物卡片）、给上场的伙伴加默契，再旁白一句（进对话记录，压一个对话框）。
// 返回 true = 这一下发了奖。历练里、没配、发过了、没找齐都返回 false，什么也不动。
bool settle(Application& app);

}  // namespace explore
}  // namespace fanren::game
