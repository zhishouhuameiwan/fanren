#pragma once
// 截图口（main.cpp 的 --scene）里伙伴改版加的那几个入口（docs/interfaces-companions.md 第 7 节第 5 条）。
//
// 放在游戏层而不留在 main.cpp：main 的截图口只有拍成 PNG 那一条路，用例要问「这一屏压上去了没有、
// 跟随者排开了没有」就得走同一段代码。main.cpp 与 tests/CompanionsShotTests.cpp 都调这里。
//
//   lilian:<历练地 id>        历练的组队面板（LilianPartyScene），压在当前世界层上
//   menu:party / menu:roster  主菜单的队伍页 / 名册页（名册上有人时才有这两页；没人时退回状态页）
//   walk:<方向串>             让主角照真入口（WorldScene::tryStep）走几步再拍：U 上 D 下 L 左 R 右，
//                             每一步照游戏的次序一帧帧走到主角与跟随者都停下；撞墙的那一步不算错，照样往下走。
//                             另认 Z（面朝前方按确认：开箱、采集）与 .（空转 0.25 秒，等得物卡片淡入）
//
// 带伙伴的世界层、四人战斗面板不另开入口：存档里有伙伴（--load）、编成照旧（battle:<编成 id>）就拍得到。
#include <string>

namespace fanren::game {

class Application;

// 认得 kind（与 arg）就压场景 / 走路并返回 true；认不得返回 false，什么也不做。
// 认得却拍不成也返回 false（标准错误写一行为什么），截图口照「认不出的 --scene」报错退出，不悄悄拍一张：
// 历练地不存在；名册上没人（主菜单没有队伍页、名册页）；walk 的方向串里有认不出的字、或走到半路栈顶已不是
// 世界层（撞上明雷、踩响触发器，压上了战斗或对白）。
// warmup 是截图前空转的帧数，只在需要更多时调大（现在这几个入口都不用）。
bool pushCompanionShot(Application& app, const std::string& kind, const std::string& arg, int& warmup);

// 截图模式下世界层的时钟步长（秒）。世界层被对白、菜单压着时不走自己的 update，画的时候按「这一帧过了多久」
// 补一步；截图时那就是从引擎开机到拍照的真实时长，灯的明灭随它变——同一个程序拍几次，对白类截图差出几百到
// 几千像素。截图时改用这个定数。
//
// 两个数都是 2026-11-18 在空闲的开发机上量出来的：改版前的程序拍对白，那一步实际是 0.056–0.059 秒；拍主菜单
// （菜单开场要多做一些事）约 0.07 秒。灯的明灭按步长一档一档变，对白在 0.0575–0.066 秒、菜单在 0.068–0.078 秒
// 拍出来都与底座 fef8a26 的基线截图逐像素相同（偶尔落出这一档，就是从前那几百像素的翻动）。各取那一档的中间：
// 新程序拍出来与基线相同，又离档的两边都远。截图口缺省用对白那一个，主菜单那几个入口改用菜单的。
inline constexpr double kShotWorldClockSeconds = 0.0625;
inline constexpr double kShotMenuWorldClockSeconds = 0.072;

}  // namespace fanren::game
