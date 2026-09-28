#pragma once
// 路径行动加载：data/pathactions/chNN.json → core::PathAction（契约 docs/interfaces-octo-pathactions.md 第 2 节）。
//
// 分三层，与 QuestLoader 的「形状归加载器、引用归门禁」相比**多给了一层引用**：
//   · loadPathActionFile —— 只看一个文件：字段表、类型、表外字段、三种行动各自的必填、
//     谓词三种写法、done_flag 的名字、until 必须写本章收尾、when 必须锚在本章。
//   · loadPathActionDir  —— 整个目录，仍然只查形状，外加「一章一个文件」。loadGameData 走这一层
//     （补丁见契约第 4.3 节）：它只拿得到 data/，而好几组测试只把 data/ 抄进临时根目录、
//     不抄 maps/——在那里查地图，等于让它们全体红。
//   · loadPathActions    —— 目录的形状，再加 io 层查得到的引用：物品、文案、角色（GameData）、
//     旗标已登记（data/flags.json）、挂的地图与 NPC 对象存在（maps/*.tmj）、
//     切磋的编成存在且输了不死、编成自己不发奖励（data/battles/）；pending 的切磋反过来
//     要求编成**还不存在**（建好了就该去掉 pending）。tests/PathActionTests.cpp 在真树上跑它。
// 引用多查一层的理由：路径行动挂在 NPC 身上，拼错一个对象名的后果是「那个人身上什么也没有」，
// 与「这一章本来就没给他写」在画面上一模一样，没人会来报。
//
// 仍归门禁（tools/validate.py 的 check_path_actions）的：同一 NPC 同一时段两条同类条目
// 不得重叠、谓词里的旗标有人会置、条目时段落在 NPC 在场的时段里——这几条要么要全仓脚本，
// 要么是内容层面的推理，不是「读得进来」的前提。
#include <string>
#include <vector>

#include "core/Result.h"
#include "core/model/Types.h"

namespace fanren::io {

// 读一章。失败时 error 写明文件、条目与字段。
[[nodiscard]] core::Result<std::vector<core::PathAction>> loadPathActionFile(const std::string& path);

// 读一个目录（通常是 data/pathactions，递归、按路径排序）下的全部章节文件，只查形状。
// 目录不存在 = 0 条，不算错；两个文件同属一章判失败，写明两份文件。
// 返回的条目按（章号，文件内次序）排好。
[[nodiscard]] core::Result<std::vector<core::PathAction>> loadPathActionDir(
    const std::string& pathActionsDir);

// 读 dataRoot/pathactions/ 下的全部章节文件并查引用。
//
// data 是已经加载好的 GameData（物品、文案、角色从它查）；旗标登记表读 dataRoot/flags.json，
// 编成读 dataRoot/battles/，地图读 dataRoot/../maps/（资产根的约定：maps/ 与 data/ 同级，
// Application 与测试都这样摆）。目录不存在 = 0 条，不算错；地图只在有条目挂在上面时才读。
// 返回的条目按（章号，文件内次序）排好。
[[nodiscard]] core::Result<std::vector<core::PathAction>> loadPathActions(
    const std::string& dataRoot, const core::GameData& data);

}  // namespace fanren::io
