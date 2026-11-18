#include "game/LilianPartyScene.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "core/rules/Companions.h"
#include "core/rules/Roster.h"
#include "game/Application.h"
#include "game/DialogueScene.h"

namespace fanren::game {
namespace {

constexpr engine::Rect kPanel{340, 130, 600, 460};
// 标题下面的两行（「外传 · 不计入正史」与说明）占的高度。
constexpr int kHeaderH = 84;

// 音效 id（docs/audio.md 第 4 节），与主菜单同一套。
constexpr const char* kSfxCursor = "ui_cursor";
constexpr const char* kSfxConfirm = "ui_confirm";
constexpr const char* kSfxCancel = "ui_cancel";
constexpr const char* kSfxError = "ui_error";

}  // namespace

LilianPartyScene::LilianPartyScene(std::string siteId) : siteId_(std::move(siteId)) {}

void LilianPartyScene::onEnter(Application& app) {
    const core::LilianSite* site = app.lilianSite(siteId_);
    title_ = site != nullptr ? app.text(site->nameKey) : siteId_;
    candidates_.clear();
    const core::GameData& data = app.data();
    for (const std::string& id : rules::roster(data.companions, app.state())) {
        const std::string role = rules::currentRole(data.companions, id, app.state());
        // 每一段都已过去的伙伴（roles 写漏了一段）没有能带的角色表，不列出来——列出来也进不了队。
        if (role.empty()) continue;
        candidates_.push_back(Candidate{id, role, app.speakerName(role), false});
    }
    hint_ = app.text(candidates_.empty() ? "ui.lilian.alone" : "ui.lilian.pick");
    list_.reset();
    list_.setPageSize(static_cast<int>(candidates_.size()) + 1);
    rebuildRows(app);
}

void LilianPartyScene::rebuildRows(Application& app) {
    std::vector<ui::ListItem> rows;
    for (const Candidate& candidate : candidates_) {
        ui::ListItem row;
        row.label = candidate.name;
        row.detail = candidate.chosen ? app.text("ui.lilian.chosen") : std::string{};
        rows.push_back(std::move(row));
    }
    rows.push_back(ui::ListItem{app.text("ui.lilian.depart"), std::string{}});
    list_.setItems(std::move(rows));
}

int LilianPartyScene::chosenCount() const {
    return static_cast<int>(std::count_if(candidates_.begin(), candidates_.end(),
                                          [](const Candidate& c) { return c.chosen; }));
}

std::vector<std::string> LilianPartyScene::chosenRoles() const {
    std::vector<std::string> out;
    for (const Candidate& candidate : candidates_) {
        if (candidate.chosen) out.push_back(candidate.roleId);
    }
    return out;
}

bool LilianPartyScene::depart(Application& app) {
    const core::LilianSite* site = app.lilianSite(siteId_);
    if (site == nullptr) return false;   // openFacility 查过了才压的面板，走不到
    const LilianEntry entry{site->id, site->entryMap, site->entrySpawn, chosenRoles()};
    auto entered = app.enterLilian(entry);
    if (!entered) {
        // 进不去（图坏了）：如实说一句，面板收起，人还在原地。
        app.engine().playSfx(kSfxError);
        const std::string said = "进不去：" + entered.error;
        app.pushDialogueLine(std::string{}, said);   // 与 showMessage 一样进回看
        app.pushScene(std::make_unique<DialogueScene>(std::string{}, said));
        return false;
    }
    app.engine().playSfx(kSfxConfirm);
    return false;
}

bool LilianPartyScene::update(Application& app, double) {
    engine::Engine& e = app.engine();
    const int before = list_.selection();
    const bool confirmed = list_.update(e);
    if (list_.selection() != before) e.playSfx(kSfxCursor);
    if (list_.cancelled()) {
        e.playSfx(kSfxCancel);
        return false;
    }
    if (!confirmed) return true;

    const int index = list_.selection();
    if (index < 0 || index >= static_cast<int>(candidates_.size())) return depart(app);

    Candidate& candidate = candidates_[static_cast<std::size_t>(index)];
    if (!candidate.chosen && chosenCount() >= kMaxCompanions) {
        hint_ = app.text("ui.lilian.full");
        e.playSfx(kSfxError);
        return true;
    }
    candidate.chosen = !candidate.chosen;
    hint_ = app.text("ui.lilian.pick");
    e.playSfx(kSfxConfirm);
    rebuildRows(app);
    return true;
}

void LilianPartyScene::render(Application& app) {
    engine::Engine& e = app.engine();
    const ui::Theme& theme = app.theme();
    ui::drawScrim(e, theme);
    ui::drawPanel(e, kPanel, title_, theme);
    const engine::Rect content = ui::panelContentArea(kPanel, title_, theme);
    const int x = content.x + theme.padding;
    e.drawText(app.text("ui.lilian.tag"), x, content.y + 6, 18, theme.gold, theme.bodyStyle);
    e.drawText(hint_, x, content.y + 40, 18, theme.paperDim, theme.bodyStyle);
    const engine::Rect listArea{content.x, content.y + kHeaderH, content.w, std::max(0, content.h - kHeaderH)};
    list_.render(e, listArea, theme);
}

}  // namespace fanren::game
