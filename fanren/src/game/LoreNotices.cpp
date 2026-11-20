#include "game/LoreNotices.h"

#include <algorithm>
#include <utility>

#include "game/WorldHud.h"

namespace fanren::game {
namespace {

constexpr int kFont = 18;
constexpr float kTop = 18.f;

const std::string& emptyString() {
    static const std::string empty;
    return empty;
}

}  // namespace

void LoreNotices::push(std::string text) {
    if (text.empty()) return;
    if (queue_.size() >= kMaxQueued) {
        queue_.pop_front();
        age_ = 0.f;
    }
    queue_.push_back(std::move(text));
}

void LoreNotices::advance(float deltaSeconds) {
    if (queue_.empty()) return;
    age_ += std::max(0.f, deltaSeconds);
    if (age_ >= kFadeIn + kHold + kFadeOut) {
        queue_.pop_front();
        age_ = 0.f;
    }
}

const std::string& LoreNotices::current() const {
    return queue_.empty() ? emptyString() : queue_.front();
}

float LoreNotices::alpha() const {
    if (queue_.empty()) return 0.f;
    if (age_ < kFadeIn) return age_ / kFadeIn;
    if (age_ < kFadeIn + kHold) return 1.f;
    return std::clamp(1.f - (age_ - kFadeIn - kHold) / kFadeOut, 0.f, 1.f);
}

void drawLoreNotice(engine::Engine& engine, const ui::Theme& theme, hud::Geometry& geometry,
                    const LoreNotices& notices) {
    const std::string& text = notices.current();
    const float alpha = notices.alpha();
    if (text.empty() || alpha <= 0.f) return;
    const engine::Point size = engine.measureText(text, kFont);
    if (size.x <= 0) return;
    // 一条墨底、两端渐隐，中间一枚金菱形领头：比目标框轻，一看就是「顺带告诉你一声」。
    const float w = static_cast<float>(size.x) + 72.f;
    const float h = static_cast<float>(size.y) + 14.f;
    const float x = (static_cast<float>(engine::kLogicalWidth) - w) / 2.f;
    engine::Color band = theme.inkDeep;
    band.a = 210;
    engine::Color clear = band;
    clear.a = 0;
    engine.drawGradientH({x - 40.f, kTop, 40.f, h}, hud::faded(clear, alpha), hud::faded(band, alpha),
                         engine::BlendMode::Alpha);
    engine.fillRect({x, kTop, w, h}, hud::faded(band, alpha), engine::BlendMode::Alpha);
    engine.drawGradientH({x + w, kTop, 40.f, h}, hud::faded(band, alpha), hud::faded(clear, alpha),
                         engine::BlendMode::Alpha);
    engine.fillRect({x, kTop + h - 1.f, w, 1.f}, hud::faded(theme.goldDim, alpha), engine::BlendMode::Alpha);
    geometry.diamond(x + 22.f, kTop + h / 2.f, 4.f, 5.f, hud::faded(theme.goldBright, alpha));
    geometry.draw(engine, engine::BlendMode::Alpha);
    engine.drawText(text, static_cast<int>(x + 40.f), static_cast<int>(kTop + 7.f), kFont,
                    hud::faded(theme.paper, alpha), theme.bodyStyle);
}

}  // namespace fanren::game
