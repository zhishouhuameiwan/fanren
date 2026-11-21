#include "game/TongxiuScene.h"

#include <algorithm>
#include <memory>

#include "core/rules/Companions.h"
#include "core/rules/Tongdao.h"
#include "game/Application.h"
#include "game/CultivationScene.h"
#include "game/DialogueScene.h"
#include "game/TongdaoPage.h"

namespace fanren::game {
namespace {

constexpr const char* kSfxCursor = "ui_cursor";
constexpr const char* kSfxConfirm = "ui_confirm";
constexpr const char* kSfxCancel = "ui_cancel";
constexpr const char* kSfxError = "ui_error";
constexpr int kChooseSit = 0;
constexpr int kChooseTogether = 1;
constexpr int kHintH = 40;   // 同修那一层：列表上面一句说明

}  // namespace

std::vector<std::string> TongxiuScene::partners(Application& app) {
    std::vector<std::string> out;
    for (const auto& [id, companion] : app.data().companions) {
        if (companion.kind == core::CompanionKind::Person && rules::inRoster(companion, app.state())) out.push_back(id);
    }
    return out;
}

engine::Rect TongxiuScene::panelRect(const ui::Theme& theme) const {
    const int rows = mode_ == Mode::Choose ? 2 : std::max(1, std::min(6, static_cast<int>(ids_.size())));
    const int h = ui::panelTitleBandHeight(true, theme) + (mode_ == Mode::Pick ? kHintH : 0) +
                  ui::listAreaHeight(rows, theme);
    return engine::Rect{(engine::kLogicalWidth - kPanelW) / 2, (engine::kLogicalHeight - h) / 2, kPanelW, h};
}

void TongxiuScene::onEnter(Application& app) {
    if (startPicking_) {
        enterPick(app);
    } else {
        enterChoose(app);
    }
}

void TongxiuScene::enterChoose(Application& app) {
    mode_ = Mode::Choose;
    list_.reset();
    list_.setPageSize(2);
    list_.setItems({{app.text("ui.tongdao.tongxiu.sit")}, {app.text("ui.tongdao.tongxiu.together")}});
}

void TongxiuScene::enterPick(Application& app) {
    mode_ = Mode::Pick;
    ids_ = partners(app);
    std::vector<ui::ListItem> rows;
    for (const std::string& id : ids_) {
        const core::Companion& companion = app.data().companions.at(id);
        const std::string role = rules::currentRole(app.data().companions, id, app.state());
        const core::RoleTemplate* tmpl = role.empty() ? nullptr : app.data().findRole(role);
        ui::ListItem row;
        row.label = tmpl != nullptr ? tmpl->name : id;
        row.detail = app.text(rules::bondTierNameKey(companion.line, rules::bondTier(app.state().bondOf(id))));
        if (rules::canTongxiu(app.data().companions, id, app.state()) == rules::TongdaoCheck::DoneToday) {
            row.enabled = false;
            row.disabledReason = app.text("ui.tongdao.why.today");
        }
        rows.push_back(std::move(row));
    }
    list_.reset();
    list_.setPageSize(std::max(1, std::min(6, static_cast<int>(rows.size()))));
    list_.setItems(std::move(rows));
}

bool TongxiuScene::update(Application& app, double) {
    engine::Engine& e = app.engine();
    // 教学卡「同修」：头一回看到这两项时（第一次 update 里压：onEnter 时正在遍历待压的场景表）。
    if (!tutorialAsked_) {
        tutorialAsked_ = true;
        app.tongdaoTutorial("tongdao.tongxiu");
    }
    const int before = list_.selection();
    const bool confirmed = list_.update(e);
    if (list_.selection() != before) e.playSfx(kSfxCursor);
    if (list_.cancelled()) {
        e.playSfx(kSfxCancel);
        if (mode_ == Mode::Pick && !startPicking_) {
            enterChoose(app);
            for (int i = 0; i < kChooseTogether; ++i) list_.moveDown();
            return true;
        }
        return false;
    }
    if (!confirmed) return true;
    if (mode_ == Mode::Choose) {
        if (list_.selection() == kChooseSit) {
            // 打坐：换成原来那块修炼面板（这一层帧末出栈、那一层入栈）。
            e.playSfx(kSfxConfirm);
            app.pushScene(std::make_unique<CultivationScene>(sitePercent_));
            return false;
        }
        e.playSfx(kSfxConfirm);
        enterPick(app);
        return true;
    }
    const int row = list_.selection();
    if (row < 0 || row >= static_cast<int>(ids_.size())) {
        e.playSfx(kSfxError);
        return true;
    }
    const std::string id = ids_[static_cast<std::size_t>(row)];
    const std::string name = list_.items()[static_cast<std::size_t>(row)].label;
    const rules::BondGain gain = app.tongxiu(id);
    if (gain.check != rules::TongdaoCheck::Ok) {
        e.playSfx(kSfxError);
        enterPick(app);
        return true;
    }
    e.playSfx(kSfxConfirm);
    std::string line = tongdaoFill(app.text("ui.tongdao.tongxiu.done"), "{name}", name);
    line = tongdaoFill(line, "{gain}", std::to_string(gain.gained));
    app.pushDialogueLine(std::string{}, line);
    app.pushScene(std::make_unique<DialogueScene>(std::string{}, line));
    return false;
}

void TongxiuScene::render(Application& app) {
    engine::Engine& e = app.engine();
    const ui::Theme& theme = app.theme();
    ui::drawScrim(e, theme);
    const engine::Rect panel = panelRect(theme);
    const std::string title = app.text(mode_ == Mode::Choose ? "ui.tongdao.tongxiu.title" : "ui.tongdao.tongxiu.pick");
    ui::drawPanel(e, panel, title, theme);
    engine::Rect area = ui::panelContentArea(panel, title, theme);
    if (mode_ == Mode::Pick) {
        e.drawText(app.text("ui.tongdao.tongxiu.hint"), area.x + theme.padding, area.y + 6, 17, theme.paperDim,
                   theme.bodyStyle);
        area.y += kHintH;
        area.h -= kHintH;
    }
    list_.render(e, area, theme);
    // 「外传」：同修也是外传（施工图 2.1）。
    const std::string tag = app.text("ui.tongdao.waizhuan");
    const engine::Point size = e.measureText(tag, 16);
    e.drawText(tag, panel.x + panel.w - theme.padding - size.x, panel.y + theme.lineSpacing + 4, 16, theme.cinnabar,
               theme.bodyStyle);
}

}  // namespace fanren::game
