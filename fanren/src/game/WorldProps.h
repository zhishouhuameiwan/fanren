#pragma once
// 世界层里伙伴改版新加的几样东西「怎么画」（docs/interfaces-companions.md 2.4–2.5、3.4–3.7、第 6 节）：
// 人物按每张表自己的倍数落在哪、四种新对象（宝箱、采集点、明雷、机关）挑哪张物件图、取不到图时的占位画法、
// 明雷本体用哪张表哪一帧。
//
// 全是纯函数，不碰引擎状态，不开窗口可测（tests/WorldViewCompanionsTests.cpp）。WorldView 照这里画；
// 「画什么」（哪个宝箱开过、哪个明雷在场）仍是 WorldScene 判好装进 WorldFrame 的，这里一条规则都不另判。
#include <string>

#include "core/model/Visual.h"
#include "engine/Engine.h"
#include "game/SpriteAtlas.h"

namespace fanren::game {

// ---- 物件 id：契约第 6 节定死，图由美术路往 index.json 的 objects 里补 ----
inline constexpr const char* kChestClosedSprite = "chest_closed";
inline constexpr const char* kChestOpenSprite = "chest_open";
inline constexpr const char* kGatherReadySprite = "gather_ready";
inline constexpr const char* kGatherSpentSprite = "gather_spent";
inline constexpr const char* kGateClosedSprite = "gate_closed";
inline constexpr const char* kGateOpenSprite = "gate_open";
inline constexpr const char* kFoeMarkSprite = "foe_mark";
inline constexpr const char* kFoeMarkEliteSprite = "foe_mark_elite";

[[nodiscard]] const char* chestSprite(bool opened);
[[nodiscard]] const char* gatherSprite(bool depleted);
[[nodiscard]] const char* gateSprite(bool open);
[[nodiscard]] const char* foeMarkSprite(bool elite);

// 机关立不立着：关着的与人物一起按脚底排序（挡在路上的样子）；开了的画进地面一层，压在柔影与人物底下，
// 不参与排序——开了的机关画成立着的，玩家就会以为路还堵着（它此刻已经不占格了）。
[[nodiscard]] bool gateStands(bool open);

// 取不到物件图时的占位画法（「缺图不崩」）：solid 为真画一块底板 + 描边 + 一个字（与设施的旧画法同一个样子）；
// 为假只描一圈很淡的框、不写字——开着的机关用它，看得出「这里原来有道关」，又不像还挡着。
struct PropPlaceholder {
    engine::Color plate{};
    const char* glyph = "";
    bool solid = true;
};
[[nodiscard]] PropPlaceholder chestPlaceholder(bool opened);
[[nodiscard]] PropPlaceholder gatherPlaceholder(bool depleted);
[[nodiscard]] PropPlaceholder gatePlaceholder(bool open);
// 明雷头顶标记取不到图时：一块小牌一个字（精英换一种颜色、换一个字）。
[[nodiscard]] PropPlaceholder foeMarkPlaceholder(bool elite);

// ---- 人物的落点（屏幕像素）----
// 帧内 (frameW/2, footY+1) 对准脚底点，按**这张表自己的** scale 放大（docs/art-sprites.md 第 4 节、5.1）。
// 旧表 scale 是 3，与改版前写死的 kArtScale 同一个数，画出来逐像素不变；高清表 ×2，一帧约 128×120，
// 会左右叠住相邻格——深度排序只看脚底，不因此乱。
[[nodiscard]] engine::RectF characterDest(const CharacterSheet& sheet, float footX, float footY);
// 最高的不透明行在屏幕上的 y：名牌、头顶标记往这条线上面放。
[[nodiscard]] float characterHeadTop(const CharacterSheet& sheet, float footY);

// ---- 明雷本体（foe.look 画哪个角色 / 敌人）----
// look 先当角色 id 查（role_variants → roles，与 NPC 同一条路），再当外观 id 查 sheets，再当敌人 id 查 enemies；
// 三处都没有两个指针都空，调用方退回旧图元并警告。人物表优先：人形明雷（劫修、护院）与 NPC 长得一样。
struct FoeBody {
    const CharacterSheet* sheet = nullptr;
    const core::EnemySheet* enemy = nullptr;
};
[[nodiscard]] FoeBody foeBodyFor(const SpriteIndex& index, const std::string& look, const ChapterDone& chapterDone);

// 敌人表只有战斗帧：明雷站着不动，取 idle 的第一帧；没有 idle 取第 0 帧（与战斗层「连 idle 都没有画第 0 帧」同一口径）。
[[nodiscard]] int enemyStandFrame(const core::EnemySheet& sheet);
[[nodiscard]] engine::RectF enemyFrameRect(const core::EnemySheet& sheet, int frame);
// 敌人表的帧有自己的朝向（非人形面朝右，人形面朝左，光球不分）。明雷面朝右（1）/ 左（3）而帧朝着另一边时翻转；
// 面朝上下、或帧不分朝向时按原样。
[[nodiscard]] bool enemyFlipped(const core::EnemySheet& sheet, int facing);
// 敌人表在世界层一律 ×kArtScale（敌人表都是 16px 美术那一套）。
[[nodiscard]] engine::RectF enemyDest(const core::EnemySheet& sheet, float footX, float footY);
[[nodiscard]] float enemyHeadTop(const core::EnemySheet& sheet, float footY);

}  // namespace fanren::game
