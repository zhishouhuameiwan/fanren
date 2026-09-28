#pragma once
// 人物精灵与地图物件的图集：从精灵索引（assets/art/sprites/index.json，A2 路 tools/artgen/sprites.py 生成，
// schema 见 docs/art-sprites.md 第 5 节）里挑外观、算帧。
//
// 读在 io 层（io/VisualLoader.h 的 loadSpriteIndex，严格：缺必填项整份报错，调用方退回旧画法并打一行警告），
// 表的样子在 core/model/Visual.h；这里只回答「这个角色此刻用哪张表、这一刻画哪一帧」。
// 世界画面（WorldView）与主菜单的小像（MenuScene）共用这一份——原先界面层另有一份 ui::SpriteIndex，
// 两份各读一遍 index.json、各写一遍变体规则，统一到这里之后同一个角色不会在两处长得不一样。
// 战斗帧、非人形敌人、按战斗覆写的外观也在同一张表里（core::SpriteIndex 的 battle / enemies / battleOverrides），
// 战斗画面取用时照同一套口径往这里补函数。
#include <functional>
#include <string>

#include "core/model/Visual.h"
#include "engine/Engine.h"

namespace fanren::game {

// 表的类型就是 io 层读出来的那几样；世界画面沿用这几个名字。
using core::CharacterSheet;
using core::LookVariant;
using core::ObjectSprite;
using core::SpriteIndex;

// 第 n 章演完了没有。判据由调用方给：世界画面拿章节表（data/chapters.json）里那一章的 done_flag
// 去问存档（Application::chapterTable().chapterDone），主菜单的小像也走这一条。
// **不用 GameState::chapter**：那个字段从来没有任何脚本推进过，第 4 章的存档检查点里它仍是 1。
// 韩立、张铁「第 1–2 章是少年」于是与第 2 章的章末旗标 ch02.done 一一对应。
using ChapterDone = std::function<bool(int)>;

// 这个角色此刻该用哪套外观：先看 role_variants（按表里的次序，第一条成立的算数），再看 roles；
// 都没有返回空串——调用方退回旧画法并打一行警告（施工图 1.2「缺图不崩」）。
// 只回答「外观 id」，不管那张表在不在 sheets 里（sheetForRole 管）。
[[nodiscard]] std::string lookForRole(const SpriteIndex& index, const std::string& roleId,
                                      const ChapterDone& chapterDone);

// 上一个再往前走一步：外观 id 对应的人物表；没有外观、或外观不在 sheets 里时为 nullptr。
[[nodiscard]] const CharacterSheet* sheetForRole(const SpriteIndex& index, const std::string& roleId,
                                                 const ChapterDone& chapterDone);

// 这一刻该画哪一帧。moving 为假时是那一向的站立帧；为真时按 walk_cycle 以 walk_fps 轮播，
// walkClock 是这一段连续走路累计的秒数。facing 同 GameState::facing。
[[nodiscard]] int walkFrame(const SpriteIndex& index, const CharacterSheet& sheet, int facing,
                            bool moving, float walkClock);

// 帧号 → 人物表里的像素矩形：x = (i % cols) * frameW，y = (i / cols) * frameH。
[[nodiscard]] engine::RectF sheetFrameRect(const CharacterSheet& sheet, int frame);

// 物件在第 seconds 秒显示第几帧（fps 为 0 或只有一帧时恒为 0）。
[[nodiscard]] int objectFrame(const ObjectSprite& sprite, float seconds);

// 物件第 frame 帧在图集里的像素矩形（frame 由 objectFrame 给，不越界）。
[[nodiscard]] engine::RectF objectFrameRect(const ObjectSprite& sprite, int frame);

}  // namespace fanren::game
