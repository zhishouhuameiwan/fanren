#include "game/DialogueExtras.h"

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

#include "engine/TextLayout.h"
#include "game/Application.h"
#include "game/DialogueScene.h"

namespace fanren::game {
namespace {

// 跳过提示那块小牌子。
constexpr int kNoticeFontSize = 18;
constexpr int kNoticePadX = 18;
constexpr int kNoticePadY = 8;
constexpr int kNoticeTop = 120;   // 地名横幅之下、目标框右侧那一片空处

// 气泡。字比对话框小一号：它是旁听，不是正戏。
constexpr int kBubbleFontSize = 18;
constexpr int kBubbleMaxTextWidth = 300;
constexpr int kBubblePadX = 12;
constexpr int kBubblePadY = 8;
constexpr int kBubbleTipH = 10;
constexpr int kBubbleTipHalfW = 8;
constexpr int kBubbleScreenMargin = 8;
constexpr int kBubbleLineGap = 4;

std::uint8_t scaled(std::uint8_t alpha, float opacity) {
    return static_cast<std::uint8_t>(std::lround(static_cast<float>(alpha) * std::clamp(opacity, 0.f, 1.f)));
}

engine::Color faded(const engine::Color& c, float opacity) { return ui::withAlpha(c, scaled(c.a, opacity)); }

}  // namespace

double autoAdvanceDelay(std::size_t graphemes) {
    return std::min(kAutoAdvanceMax, kAutoAdvanceBase + kAutoAdvancePerGrapheme * static_cast<double>(graphemes));
}

// ---------------------------------------------------------------------------
// 跳过提示
// ---------------------------------------------------------------------------

float SkipNotice::opacity() const {
    if (!active()) return 0.f;
    if (age_ <= kHold) return 1.f;
    return std::clamp(1.f - (age_ - kHold) / kFade, 0.f, 1.f);
}

engine::Rect skipNoticeArea(engine::Engine& engine, const std::string& text) {
    const engine::Point size = engine.measureText(text, kNoticeFontSize);
    const int w = size.x + kNoticePadX * 2;
    const int h = kNoticeFontSize + kNoticePadY * 2;
    return engine::Rect{(engine::kLogicalWidth - w) / 2, kNoticeTop, w, h};
}

void drawSkipNotice(engine::Engine& engine, const ui::Theme& theme, const SkipNotice& notice) {
    if (!notice.active()) return;
    const float opacity = notice.opacity();
    const engine::Rect area = skipNoticeArea(engine, notice.text());
    const engine::RectF areaF{static_cast<float>(area.x), static_cast<float>(area.y), static_cast<float>(area.w),
                              static_cast<float>(area.h)};
    engine.fillRect(areaF, faded(ui::withAlpha(theme.inkDeep, 220), opacity), engine::BlendMode::Alpha);
    ui::drawFrame(engine, areaF, faded(theme.goldDim, opacity));
    engine.drawText(notice.text(), area.x + kNoticePadX, area.y + kNoticePadY - 1, kNoticeFontSize,
                    faded(theme.paper, opacity));
}

// ---------------------------------------------------------------------------
// 回看面板（世界层）
// ---------------------------------------------------------------------------

void DialogueLogScene::onEnter(Application& app) {
    std::vector<ui::ListItem> items;
    const std::vector<Application::SpokenLine>& log = app.dialogueLog();
    // 与对话框里那一份同一个次序：刚说过的在最上面。
    for (auto it = log.rbegin(); it != log.rend(); ++it) {
        ui::ListItem row;
        row.label = it->speaker.empty() ? it->body : (it->speaker + "：" + it->body);
        items.push_back(std::move(row));
    }
    view_.reset();
    view_.setEmptyHint("（还没有说过话）");
    view_.setPageSize(DialogueScene::logPageRows(app.theme()));
    view_.setItems(std::move(items));
}

bool DialogueLogScene::update(Application& app, double) {
    engine::Engine& eng = app.engine();
    if (eng.keyPressed(engine::Engine::Key::Menu)) return false;
    if (view_.update(eng) || view_.cancelled()) return false;
    return true;
}

void DialogueLogScene::render(Application& app) {
    engine::Engine& eng = app.engine();
    const ui::Theme& theme = app.theme();
    const engine::Rect area = DialogueScene::logPanelArea();
    const std::string title = "回看";
    ui::drawPanel(eng, area, title, theme);
    view_.render(eng, ui::panelContentArea(area, title, theme), theme);
}

// ---------------------------------------------------------------------------
// 插话气泡
// ---------------------------------------------------------------------------

void SpeechBubbles::push(SpeechBubble bubble) {
    // 同一个人又开口：旧的那只先收掉，头顶不叠两只。
    bubbles_.erase(std::remove_if(bubbles_.begin(), bubbles_.end(),
                                  [&](const SpeechBubble& b) { return b.roleId == bubble.roleId; }),
                   bubbles_.end());
    bubble.age = 0.f;
    bubbles_.push_back(std::move(bubble));
    while (bubbles_.size() > kMax) bubbles_.pop_front();
}

void SpeechBubbles::advance(float dt) {
    for (SpeechBubble& b : bubbles_) b.age += dt;
    bubbles_.erase(std::remove_if(bubbles_.begin(), bubbles_.end(),
                                  [](const SpeechBubble& b) { return b.age > kFadeIn + b.seconds + kFadeOut; }),
                   bubbles_.end());
}

float SpeechBubbles::opacity(const SpeechBubble& bubble) {
    if (bubble.age < kFadeIn) return std::clamp(bubble.age / kFadeIn, 0.f, 1.f);
    const float outStart = kFadeIn + bubble.seconds;
    if (bubble.age <= outStart) return 1.f;
    return std::clamp(1.f - (bubble.age - outStart) / kFadeOut, 0.f, 1.f);
}

BubbleLayout bubbleLayout(engine::Point anchor, int textWidth, int textHeight) {
    const int w = std::max(0, textWidth) + kBubblePadX * 2;
    const int h = std::max(0, textHeight) + kBubblePadY * 2;
    int x = anchor.x - w / 2;
    x = std::clamp(x, kBubbleScreenMargin, std::max(kBubbleScreenMargin, engine::kLogicalWidth - kBubbleScreenMargin - w));
    int y = anchor.y - kBubbleTipH - h;
    y = std::max(kBubbleScreenMargin, y);
    const int tipX = std::clamp(anchor.x, x + kBubbleTipHalfW + 2, x + w - kBubbleTipHalfW - 2);
    return BubbleLayout{engine::Rect{x, y, w, h}, engine::Point{tipX, y + h + kBubbleTipH}};
}

void drawSpeechBubble(engine::Engine& engine, const ui::Theme& theme, const std::string& text, engine::Point anchor,
                      float opacity) {
    if (opacity <= 0.f || text.empty()) return;
    const auto measure = [&engine](const std::string& s) { return engine.measureText(s, kBubbleFontSize).x; };
    const std::vector<engine::LayoutLine> lines = engine::layoutText(text, kBubbleMaxTextWidth, measure);
    int textW = 0;
    for (const engine::LayoutLine& line : lines) textW = std::max(textW, line.width);
    const int lineH = kBubbleFontSize + kBubbleLineGap;
    const int textH = static_cast<int>(lines.size()) * lineH - kBubbleLineGap;
    const BubbleLayout layout = bubbleLayout(anchor, textW, textH);
    const engine::RectF box{static_cast<float>(layout.box.x), static_cast<float>(layout.box.y),
                            static_cast<float>(layout.box.w), static_cast<float>(layout.box.h)};
    const engine::Color fill = faded(ui::withAlpha(theme.paper, 235), opacity);
    const engine::Color edge = faded(theme.goldDim, opacity);
    engine.fillRect(box, fill, engine::BlendMode::Alpha);
    ui::drawFrame(engine, box, edge);
    // 尖角：从框底边往下指着说话的人。
    const float baseY = box.y + box.h - 1.f;
    const float tx = static_cast<float>(layout.tip.x);
    const std::vector<engine::Vertex> tip{
        {tx - kBubbleTipHalfW, baseY, fill, 0.f, 0.f},
        {tx + kBubbleTipHalfW, baseY, fill, 0.f, 0.f},
        {tx, static_cast<float>(layout.tip.y), fill, 0.f, 0.f},
    };
    engine.drawGeometry(engine::kInvalidTexture, tip, {}, engine::BlendMode::Alpha);
    int y = layout.box.y + kBubblePadY - 1;
    for (const engine::LayoutLine& line : lines) {
        engine.drawText(line.text, layout.box.x + kBubblePadX, y, kBubbleFontSize, faded(theme.inkDeep, opacity));
        y += lineH;
    }
}

}  // namespace fanren::game
