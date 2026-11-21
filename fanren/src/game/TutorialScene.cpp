#include "game/TutorialScene.h"

#include <algorithm>

#include "core/rules/Tutorials.h"
#include "engine/TextLayout.h"
#include "game/Application.h"
#include "ui/Widgets.h"

namespace fanren::game {
namespace {

constexpr int kPanelW = 720;
constexpr int kHintFont = 16;
constexpr int kLineGap = 6;

}  // namespace

engine::Rect TutorialScene::panelRect(int lineCount) {
    // 标题带 + 每行（字号 22 + 行距）+ 底下一行按键提示。
    const int rows = std::clamp(lineCount, 1, 6);
    const int h = 64 + rows * (22 + kLineGap) + 56;
    const int x = (engine::kLogicalWidth - kPanelW) / 2;
    // 偏上一点：底下是对白框与路径行动提示的地方，中间是主角。
    const int y = std::max(40, (engine::kLogicalHeight - h) / 2 - 90);
    return engine::Rect{x, y, kPanelW, h};
}

void TutorialScene::onEnter(Application& app) {
    age_ = 0.0;
    lines_.clear();
    const core::TutorialCard* card = rules::findTutorial(app.data().tutorials, cardId_);
    if (card == nullptr) {
        title_ = cardId_;   // 画面上一眼看出是哪张卡没进 data
        return;
    }
    title_ = app.text(card->titleKey);
    const ui::Theme& theme = app.theme();
    engine::Engine& eng = app.engine();
    const int width = kPanelW - theme.padding * 2;
    for (const std::string& key : card->bodyKeys) {
        const std::string body = app.text(key);
        const std::vector<engine::LayoutLine> wrapped = engine::layoutText(
            body, width, [&eng, &theme](const std::string& s) { return eng.measureText(s, theme.bodyFontSize).x; });
        if (wrapped.empty()) {
            lines_.push_back(body);   // 无头量不出字：原样一行
            continue;
        }
        for (const engine::LayoutLine& line : wrapped) lines_.push_back(line.text);
    }
    hint_ = app.text("ui.guide.tutorial.close");
}

bool TutorialScene::update(Application& app, double deltaSeconds) {
    age_ += deltaSeconds;
    engine::Engine& e = app.engine();
    using Key = engine::Engine::Key;
    if (!app.headless() && age_ < kArmSeconds) return true;
    if (e.keyPressed(Key::Confirm) || e.keyPressed(Key::Cancel)) {
        e.playSfx("ui_confirm");
        return false;
    }
    return true;
}

void TutorialScene::render(Application& app) {
    engine::Engine& eng = app.engine();
    const ui::Theme& theme = app.theme();
    // 底下的画面压暗一半，不全黑：玩家要看得见卡片说的那样东西（战斗里的破绽格、地图上的宝箱）。
    eng.fillRect(engine::RectF{0.f, 0.f, static_cast<float>(engine::kLogicalWidth),
                               static_cast<float>(engine::kLogicalHeight)},
                 ui::withAlpha(theme.inkDeep, 110), engine::BlendMode::Alpha);
    const engine::Rect panel = panelRect(static_cast<int>(lines_.size()));
    ui::drawPanel(eng, panel, title_, theme);
    const engine::Rect content = ui::panelContentArea(panel, title_, theme);
    int y = content.y + theme.lineSpacing;
    for (const std::string& line : lines_) {
        eng.drawText(line, content.x + theme.padding, y, theme.bodyFontSize, theme.paper, theme.bodyStyle);
        y += theme.bodyFontSize + kLineGap;
    }
    if (!hint_.empty()) {
        const engine::Point size = eng.measureText(hint_, kHintFont);
        eng.drawText(hint_, panel.x + panel.w - theme.padding - size.x, panel.y + panel.h - theme.padding - size.y + 4,
                     kHintFont, theme.goldBright, theme.bodyStyle);
    }
}

}  // namespace fanren::game
