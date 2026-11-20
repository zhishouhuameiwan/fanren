#pragma once
// 对白减负的三样小东西（第 7 章地基包路 D，docs/ch07-foundation.md 4.2–4.4）：
//   · 自动播放的停顿时长（纯函数）；
//   · 整段跳过之后那一条「已跳过 N 句 · 按 Tab 回看」的提示，与世界层上按 Tab 翻开的回看面板；
//   · 伙伴插话的头顶气泡：不压场景、不收输入，人照走。
// 状态都记在 Application 里，这里只放结构、计时与画法——与 ItemToasts 同一个分法。
#include <cstddef>
#include <deque>
#include <optional>
#include <string>

#include "engine/Engine.h"
#include "game/Scene.h"
#include "game/WorldView.h"
#include "ui/Widgets.h"

namespace fanren::game {

// ---------------------------------------------------------------------------
// 自动播放（4.2）
// ---------------------------------------------------------------------------
// 一句显示完之后停多久自动翻下一句：0.8 秒加每个字 0.06 秒，至多 4 秒。graphemes 是这一句的字数（含标点）。
inline constexpr double kAutoAdvanceBase = 0.8;
inline constexpr double kAutoAdvancePerGrapheme = 0.06;
inline constexpr double kAutoAdvanceMax = 4.0;
[[nodiscard]] double autoAdvanceDelay(std::size_t graphemes);

// ---------------------------------------------------------------------------
// 整段跳过之后的提示（4.3）
// ---------------------------------------------------------------------------
// 不挡路、不用按键：停 kHold 秒、淡出 kFade 秒。停着的这段时间里，世界层上按菜单键翻开的是回看面板
// （不是主菜单）——提示上写着「按 Tab 回看」，这句话得是真的。
class SkipNotice {
public:
    static constexpr float kHold = 4.0f;
    static constexpr float kFade = 0.5f;

    void show(std::string text) {
        text_ = std::move(text);
        age_ = 0.f;
    }
    void advance(float dt) {
        if (age_ >= 0.f) age_ += dt;
        if (age_ > kHold + kFade) clear();
    }
    void clear() {
        text_.clear();
        age_ = -1.f;
    }
    [[nodiscard]] bool active() const { return age_ >= 0.f && !text_.empty(); }
    // 还停着（没开始淡出）：这段时间里菜单键归回看。
    [[nodiscard]] bool holding() const { return active() && age_ <= kHold; }
    [[nodiscard]] const std::string& text() const { return text_; }
    [[nodiscard]] float opacity() const;

private:
    std::string text_;
    float age_ = -1.f;
};

// 画在屏幕上半截正中的一条小牌子：不压对话框（框贴底）、不压左上的目标框与右上的存盘提示。
// avoid：世界层左上那一列（目标框连同收集进度，第一期地基包集成）。目标写得长、框宽到正中时，提示往右让开，
// 不压它；空矩形 = 不用让。
[[nodiscard]] engine::Rect skipNoticeArea(engine::Engine& engine, const std::string& text,
                                          const engine::RectF& avoid = {});
void drawSkipNotice(engine::Engine& engine, const ui::Theme& theme, const SkipNotice& notice,
                    const engine::RectF& avoid = {});

// ---------------------------------------------------------------------------
// 回看面板（世界层上翻开的那一份）
// ---------------------------------------------------------------------------
// 与对话框里按 Tab 翻开的是同一块面板、同一份记录（Application::dialogueLog），只是它不压在对话框上：
// 整段跳过一路跳到脚本结束时，人已经回到地图上了，提示里那句「按 Tab 回看」就靠它兑现。
// 它不是脚本命令，关掉时不回填任何东西。
class DialogueLogScene : public Scene {
public:
    void onEnter(Application& app) override;
    bool update(Application& app, double deltaSeconds) override;
    void render(Application& app) override;
    [[nodiscard]] bool opaque() const override { return false; }
    [[nodiscard]] std::string name() const override { return "DialogueLog"; }
    [[nodiscard]] const ui::ListView& list() const { return view_; }

private:
    ui::ListView view_;
};

// ---------------------------------------------------------------------------
// 插话气泡（4.4）
// ---------------------------------------------------------------------------
struct SpeechBubble {
    std::string roleId;     // 说话的角色：跟随者 / 主角按它每帧找人（人走气泡跟着走）
    std::string text;       // 正文（不带名字：气泡就浮在那个人头上）
    float seconds = 3.f;    // 停多久（不含淡入淡出）
    float age = 0.f;
    // 找不到活人时退回的固定格（NPC 站着不动，压气泡那一刻记下来）。
    std::optional<Vec2f> fixedCell;
};

class SpeechBubbles {
public:
    static constexpr float kFadeIn = 0.15f;
    static constexpr float kFadeOut = 0.35f;
    static constexpr std::size_t kMax = 3;   // 同时最多几只；再多把最早的挤掉

    void push(SpeechBubble bubble);
    void advance(float dt);
    void clear() { bubbles_.clear(); }
    [[nodiscard]] bool active() const { return !bubbles_.empty(); }
    [[nodiscard]] const std::deque<SpeechBubble>& bubbles() const { return bubbles_; }
    [[nodiscard]] static float opacity(const SpeechBubble& bubble);

private:
    std::deque<SpeechBubble> bubbles_;
};

// 气泡的框：anchor 是说话人头顶正中的屏幕点（逻辑像素）。框在它上方、尖角朝下；
// 贴着屏幕边时整个框往里挪，尖角仍指着人。纯计算，测试直接问。
struct BubbleLayout {
    engine::Rect box;
    engine::Point tip;      // 尖角的尖
};
[[nodiscard]] BubbleLayout bubbleLayout(engine::Point anchor, int textWidth, int textHeight);
void drawSpeechBubble(engine::Engine& engine, const ui::Theme& theme, const std::string& text,
                      engine::Point anchor, float opacity);

}  // namespace fanren::game
