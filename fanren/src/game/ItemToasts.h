#pragma once
// 得物卡片（docs/ch07-playtest-ui.md 第 2 节）：拿到东西时屏幕右侧弹出的小卡片——图标、按品阶上色的名字、
// 「×数量」、说明的第一句。2026-11-20 用户试玩第 7 章：「找东西没啥回报感」，代替从前那一行要按确认才收得掉的旁白。
//
// **不挡路**：卡片不是场景，不进场景栈，不收输入。几秒后自己淡出；世界层上按确认（面前又没有别的能按的东西时）
// 提前收（WorldScene::update）。开箱、采集、脚本 give()、路径行动给物都经 Application::announceItem 往这里推。
//
// 这里分两半：ItemToasts 是纯数据（推、走时钟、收、淡入淡出的浓度），不开窗口可测；drawItemToasts 只画。
#include <cstddef>
#include <deque>
#include <string>

#include "core/model/Types.h"
#include "engine/Engine.h"
#include "ui/Widgets.h"

namespace fanren::game {

enum class BattleIcon;   // game/BattleHud.h：图标图集 art/ui/icons.png 的格子

namespace hud {
class Geometry;
}

// 品阶 → 颜色档（ui::Theme::gradeColors 的下标）：0–1 阶 0 档（凡品），2–3 阶 1 档，4–5 阶 2 档，
// 6–7 阶 3 档，8 阶以上 4 档。负数按 0 档。
[[nodiscard]] int gradeTier(int grade);

// 物品说明的第一句：到第一个句末标点（。！？）为止、不含那个标点；超过 maxGlyphs 个字（按 UTF-8 码点数）
// 就截成 maxGlyphs − 1 个字再接一个「…」。空串给空串。
[[nodiscard]] std::string firstSentence(const std::string& text, std::size_t maxGlyphs);

// 没有逐物品的图标：按种类挑图集里现成的一格（docs/ch07-playtest-ui.md 第 2 节那张表）。
// 钱（kSpiritStoneItemId）是碎银那一格；丹药里带毒的是毒那一格；兵器按它给的攻击类别。查不到物品给宝箱那一格。
[[nodiscard]] BattleIcon itemIcon(const core::Item* item, const std::string& itemId);

struct ItemToast {
    std::string itemId;
    std::string name;      // 显示名：钱按 Wording 叫「碎银 / 灵石」，查不到物品回显 id
    int count = 1;
    int tier = 0;          // gradeTier(品阶)
    BattleIcon icon{};
    std::string note;      // 说明的第一句（钱不写）
    float age = 0.f;       // 出场之后过了多少秒；负数 = 还在排队（一次给几样时错开出场）
    bool dismissed = false;   // 按确认收起：从这一刻起按快的那一档淡出
    float dismissAge = 0.f;   // 收起那一刻的 age
};

// 照数据拼一张卡片（名字、品阶档、图标、说明第一句）。count ≤ 0 按 1。
[[nodiscard]] ItemToast makeItemToast(const core::GameData& data, const core::GameState& state,
                                      const std::string& itemId, int count);

class ItemToasts {
public:
    static constexpr float kFadeIn = 0.2f;
    static constexpr float kHold = 3.2f;       // 停多久（淡入之后）
    static constexpr float kFadeOut = 0.5f;
    static constexpr float kDismissFade = 0.18f;   // 按确认收起：这么快淡完
    static constexpr float kStagger = 0.18f;   // 一次给几样：后一张比前一张晚这么久出场
    static constexpr std::size_t kMaxCards = 6;   // 同时最多几张；再多就把最早的那张挤掉

    void push(ItemToast toast);
    // 走时钟；淡完的卡片移走。
    void advance(float deltaSeconds);
    // 按确认：此刻在的卡片（含还在排队的）一起快快淡出。
    void dismiss();
    void clear() { cards_.clear(); }

    // 有卡片在（含排队的、正在淡出的）。
    [[nodiscard]] bool active() const { return !cards_.empty(); }
    // 有没被收起的卡片：此刻按确认会把它们收起。
    [[nodiscard]] bool dismissable() const;
    [[nodiscard]] const std::deque<ItemToast>& cards() const { return cards_; }

    // 一张卡片此刻的浓度 0–1：排队时 0，淡入、停、淡出（或收起后的快淡出）。
    [[nodiscard]] static float alpha(const ItemToast& card);
    // 这张卡片已经淡完（可以移走）。
    [[nodiscard]] static bool finished(const ItemToast& card);

private:
    std::deque<ItemToast> cards_;
};

// 卡片画在哪：屏幕右侧，自上往下排。index 是在 cards() 里的下标。
// top 是第一张的上沿：缺省排在右上角的小地图之下；存盘提示正亮着时排在它下面（第一期地基包集成）。
inline constexpr float kItemToastTop = 172.f;
[[nodiscard]] engine::RectF itemToastRect(std::size_t index, float top = kItemToastTop);

// 画一遍（后处理之外、世界层之上、对话框之下：Application::drawScenes 紧跟着世界层调）。
// icons 是图标图集（无效句柄时只是不画图标）；hero 是主角此刻占的屏幕矩形，卡片盖住他时淡成半透明。
void drawItemToasts(engine::Engine& engine, const ui::Theme& theme, hud::Geometry& geometry,
                    engine::TextureId icons, const ItemToasts& toasts, const engine::Rect& hero,
                    float top = kItemToastTop);

}  // namespace fanren::game
