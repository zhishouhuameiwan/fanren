#include "game/IntroCardScene.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include "game/Application.h"
#include "game/Busts.h"
#include "ui/Widgets.h"

namespace fanren::game {
namespace {

constexpr int kNameFont = 52;
constexpr int kTitleFont = 24;
constexpr int kLineFont = 22;
constexpr const char* kCardSfx = "chapter_card";   // 现成的一声（章节卡同一声），不新增音频

std::uint8_t scaled(std::uint8_t a, float k) {
    return static_cast<std::uint8_t>(std::lround(static_cast<float>(a) * std::clamp(k, 0.f, 1.f)));
}

}  // namespace

IntroCardScene::IntroCardScene(std::string role, engine::TextureId texture, engine::RectF cell, std::string name,
                               std::string title, std::string line)
    : role_(std::move(role)), texture_(texture), cell_(cell), name_(std::move(name)), title_(std::move(title)),
      line_(std::move(line)) {}

float IntroCardScene::opacityAt(double age, bool closing, double closingAge) {
    const float in = static_cast<float>(std::clamp(age / kSlideSeconds, 0.0, 1.0));
    const float out = closing ? static_cast<float>(std::clamp(1.0 - closingAge / kFadeOutSeconds, 0.0, 1.0)) : 1.f;
    return in * out;
}

float IntroCardScene::slideOffsetAt(double age) {
    const double t = std::clamp(age / kSlideSeconds, 0.0, 1.0);
    const double eased = 1.0 - (1.0 - t) * (1.0 - t);   // 先快后慢
    return static_cast<float>((1.0 - eased) * kSlideFrom);
}

engine::RectF IntroCardScene::bustRect(const engine::RectF& src) {
    if (src.w <= 0.f || src.h <= 0.f) return engine::RectF{};
    const float h = static_cast<float>(kBustH);
    const float w = h * src.w / src.h;
    return engine::RectF{static_cast<float>(kBustCenterX) - w / 2.f, static_cast<float>(engine::kLogicalHeight) - h, w,
                         h};
}

void IntroCardScene::onEnter(Application& app) {
    age_ = 0.0;
    closing_ = false;
    closingAge_ = 0.0;
    done_ = false;
    app.engine().playSfx(kCardSfx);
}

void IntroCardScene::close(Application& app) {
    if (done_) return;
    done_ = true;
    script::CommandResult result;
    result.ok = true;
    app.completeCommand(result);
}

bool IntroCardScene::update(Application& app, double deltaSeconds) {
    if (done_) return false;
    age_ += deltaSeconds;
    if (closing_) {
        closingAge_ += deltaSeconds;
        if (closingAge_ >= kFadeOutSeconds) {
            close(app);
            return false;
        }
        return true;
    }
    engine::Engine& e = app.engine();
    using Key = engine::Engine::Key;
    if (age_ >= kArmSeconds && (e.keyPressed(Key::Confirm) || e.keyPressed(Key::Cancel))) {
        closing_ = true;
        return true;
    }
    if (app.settings().autoAdvance && age_ >= kAutoSeconds) closing_ = true;
    return true;
}

void IntroCardScene::render(Application& app) {
    engine::Engine& eng = app.engine();
    const ui::Theme& theme = app.theme();
    const float k = opacityAt(age_, closing_, closingAge_);
    eng.fillRect(engine::RectF{0.f, 0.f, static_cast<float>(engine::kLogicalWidth),
                               static_cast<float>(engine::kLogicalHeight)},
                 engine::Color{0, 0, 0, scaled(kDim, k)}, engine::BlendMode::Alpha);

    const engine::RectF src = bustRosterSource(cell_);
    engine::RectF dst = bustRect(src);
    dst.x += slideOffsetAt(age_);
    engine::DrawOptions options;
    options.tint.a = scaled(255, k);
    if (dst.w > 0.f) eng.drawTexture(texture_, src, dst, options);

    // 左侧三行：名字（金）、身份、短评；中间一道两头渐隐的暗金线。
    const int nameY = 280;
    eng.drawText(name_, kTextX, nameY, kNameFont, ui::withAlpha(theme.goldBright, scaled(255, k)), theme.titleStyle);
    const int ruleY = nameY + kNameFont + 18;
    if (k > 0.f) ui::drawRuleH(eng, static_cast<float>(kTextX), static_cast<float>(ruleY), 360.f, theme);
    eng.drawText(title_, kTextX, ruleY + 16, kTitleFont, ui::withAlpha(theme.paper, scaled(255, k)), theme.bodyStyle);
    eng.drawText(line_, kTextX, ruleY + 16 + kTitleFont + 16, kLineFont, ui::withAlpha(theme.paperDim, scaled(255, k)),
                 theme.bodyStyle);
}

}  // namespace fanren::game
