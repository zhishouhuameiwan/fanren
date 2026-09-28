#pragma once
// 战斗的 HUD（墨金主题，docs/octopath-battle.md 第 6 节）：顶端行动序条、右上回合与波次、
// 敌人脚下的架势盾 / 破绽格 / 细血条、目标指针与名牌、首领蓄势的预告横幅、右下我方状态、
// 左下日志、战果卡。画在后处理之外：字与图标不糊、不压暗、不被调色。
//
// 只画不算：显示用的局面（ShownUnit）与台上的位置（BattleView）都由调用方给，这里一个判据都不自己写。
#include <string>
#include <vector>

#include "core/battle/Battle.h"
#include "core/model/Types.h"
#include "engine/Engine.h"
#include "game/BattleFx.h"
#include "game/BattleScene.h"
#include "ui/Widgets.h"

namespace fanren::game {

class BattleView;

// UI 图标（assets/art/ui/icons.png，16×16，运行时 ×2）。矩形抄自 icons.json（tools/artgen/fx.py 生成）：
// game 层不解析 JSON，A2 改了图集的排法要跟着改 iconRect。
enum class BattleIcon {
    Sword,
    Blade,
    Fist,
    Hidden,
    Poison,
    Metal,
    Wood,
    Water,
    Fire,
    Earth,
    Unknown,
    Stance,
    BeadFull,
    BeadEmpty,
    Pointer,
    Cultivation,
    Silver,
    Chest,
    Medicine,
};
[[nodiscard]] engine::RectF iconRect(BattleIcon icon);
// 单个类别位 → 图标；认不出的（0、多位）给「？」。
[[nodiscard]] BattleIcon categoryIcon(int category);

// 战果卡上的一行：图标 + 一句。修为、钱（名字走 Wording：第 1–5 章是「碎银」）、每一样掉落。
struct RewardLine {
    BattleIcon icon = BattleIcon::Chest;
    std::string text;
};
[[nodiscard]] std::vector<RewardLine> rewardLines(const GrantedReward& granted, const core::GameData& data,
                                                  const core::GameState& state,
                                                  const std::string& cultivationWord);

// 一帧 HUD 要的全部东西。
struct HudFrame {
    const core::battle::BattleState& battle;
    const std::vector<ShownUnit>& shown;
    const BattleView& view;
    const ui::Theme& theme;
    engine::TextureId icons = engine::kInvalidTexture;
    int actor = -1;
    int boost = 0;
    int target = -1;            // 选目标时指着谁
    bool choosing = false;      // 我方正在选命令
};

void drawOrderStrip(engine::Engine& e, const HudFrame& f, const std::string& nowWord, const std::string& nextWord);
void drawRoundInfo(engine::Engine& e, const HudFrame& f, const std::string& round, const std::string& wave);
void drawEnemyPlates(engine::Engine& e, const HudFrame& f, const std::string& chargeWord);
void drawTargetPointer(engine::Engine& e, const HudFrame& f);
void drawPartyPanel(engine::Engine& e, const HudFrame& f);
// 首领蓄势的预告句：age 秒前亮出来，life 秒后收起。
void drawChargeBanner(engine::Engine& e, const ui::Theme& theme, const std::string& text, float age, float life);
void drawBattleLog(engine::Engine& e, const ui::Theme& theme, const std::string& line, const std::string& hint);

// 战果卡：胜利是「胜」字大卡 + 收获逐行飞入；败北、逃走、对手遁走各一句简短收场。
struct ResultCard {
    core::battle::BattlePhase phase = core::battle::BattlePhase::Won;
    std::string title;
    std::string subtitle;
    std::vector<RewardLine> lines;
    std::string hint;
    float age = 0.f;
};
// 最后一行飞到位的时刻：这之前按确认键不收场（免得一下按穿，玩家根本没看见得了什么）。
[[nodiscard]] float resultCardSettleTime(const ResultCard& card);
void drawResultCard(engine::Engine& e, const ui::Theme& theme, engine::TextureId icons, const ResultCard& card);

}  // namespace fanren::game
