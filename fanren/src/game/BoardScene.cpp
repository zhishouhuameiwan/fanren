#include "game/BoardScene.h"

#include <algorithm>
#include <utility>

#include "core/rules/Objectives.h"
#include "core/rules/Quests.h"
#include "engine/TextLayout.h"
#include "game/Application.h"
#include "ui/Widgets.h"

namespace fanren::game {
namespace {

constexpr int kPanelX = 200;
constexpr int kPanelY = 70;
constexpr int kPanelW = 880;
constexpr int kPanelH = 580;

// 面板上半截留给「当前」那两行，下半截是做过的事。「做什么」一行放不下时上半截往下长（currentBandH_）。
constexpr int kCurrentBandH = 96;
// 最底下一条：光标停着的那一行的全文，折行显示（2026-11-20 用户试玩：「记事里面描述没有换行，
// 导致超出界面了」——支线一行拼成「任务名：这一步（地名）」常有四十多个字，列表里只能截断）。
constexpr int kDetailBandH = 64;
constexpr int kDetailFont = 18;

int rowsFor(const ui::Theme& theme, int currentBandH) {
    const int contentH = kPanelH - ui::panelTitleBandHeight(/*hasTitle=*/true, theme) - currentBandH - kDetailBandH;
    return std::max(1, ui::listRowsThatFit(contentH, theme));
}

}  // namespace

int BoardScene::listPageRows(const ui::Theme& theme) { return rowsFor(theme, kCurrentBandH); }

std::vector<ui::ListItem> BoardScene::buildRows(const core::GameData& data,
                                               const core::GameState& state) {
    std::vector<ui::ListItem> rows;

    // ---- 支线（契约 docs/interfaces-p3-ch05.md 1.7）----
    for (const rules::QuestEntry& entry : rules::sideQuestJournal(data.quests, state)) {
        const core::Quest& quest = *entry.quest;
        const std::string title = data.lookupText(quest.titleKey);
        ui::ListItem row;
        // **不要加 default:**：新增一种状态时让编译器告警，而不是悄悄画成某一种。
        switch (entry.status) {
            case rules::QuestStatus::Active: {
                if (entry.step != nullptr) {
                    row.label = title + "：" + data.lookupText(entry.step->textKey);
                    // 这一步在哪儿（契约 Q9）。HUD 不给支线指路，告示板至少把地名说出来。
                    if (!entry.step->targetMap.empty()) {
                        const std::string key = core::mapDisplayNameKey(entry.step->targetMap);
                        row.label += "（" +
                                     (key.empty() ? entry.step->targetMap : data.lookupText(key)) +
                                     "）";
                    }
                } else {
                    // 步骤都做完了而还没了结：没有「下一步」可说，说这件事本身。
                    row.label = title + "：" + data.lookupText(quest.summaryKey);
                }
                row.detail = kActiveTag;
                break;
            }
            case rules::QuestStatus::Failed:
                // 过期要给原因（Q7）。只写「已过期」三个字，玩家不知道是错过了什么。
                row.label = title + "：" + data.lookupText(quest.failTextKey);
                row.detail = kFailedTag;
                break;
            case rules::QuestStatus::Completed:
                row.label = title;
                row.detail = kCompletedTag;
                break;
            case rules::QuestStatus::NotAccepted:
                // sideQuestJournal 不收未接的；真漏进来也不画。
                continue;
        }
        rows.push_back(std::move(row));
    }

    // ---- 做过的主线步骤：与第 5 章之前逐行相同 ----
    std::vector<ui::ListItem> done;
    for (const core::Objective* step : rules::completedObjectives(data.objectives, state)) {
        ui::ListItem row;
        row.label = data.lookupText(step->textKey);
        done.push_back(std::move(row));
    }
    // 倒序：刚做完的那件排在最上面。翻到第三页去找「我上一步干了什么」是荒谬的。
    std::reverse(done.begin(), done.end());
    for (ui::ListItem& row : done) rows.push_back(std::move(row));
    return rows;
}

void BoardScene::onEnter(Application& app) {
    const std::vector<core::Objective>& chain = app.data().objectives;
    const core::GameState& state = app.state();

    if (const core::Objective* step = rules::currentObjective(chain, state)) {
        current_ = app.text(step->textKey);
        const std::string key = core::mapDisplayNameKey(step->targetMap);
        where_ = "地点　" + (key.empty() ? step->targetMap : app.text(key));
    } else {
        // 目标链走完（或这份数据里压根没有目标链）。说清楚是哪一种：
        // 一片空白会被当成面板坏了，而这两种情形要靠完全不同的方式去查。
        current_ = chain.empty() ? "此处暂无记事。" : "眼下没有非做不可的事。";
        where_.clear();
    }

    done_.reset();
    done_.setEmptyHint("（还没做成什么事）");
    // 「做什么」按面板正文宽折行；多出来的行把上半截撑高，列表相应少几行。
    const ui::Theme& theme = app.theme();
    const engine::Rect content = ui::panelContentArea(engine::Rect{kPanelX, kPanelY, kPanelW, kPanelH}, "记事", theme);
    engine::Engine& eng = app.engine();
    currentLines_.clear();
    for (const engine::LayoutLine& line :
         engine::layoutText(current_, std::max(1, content.w - theme.padding * 2),
                            [&eng, &theme](const std::string& s) { return eng.measureText(s, theme.bodyFontSize).x; })) {
        currentLines_.push_back(line.text);
    }
    if (currentLines_.empty()) currentLines_.push_back(current_);
    currentBandH_ = kCurrentBandH +
                    static_cast<int>(currentLines_.size() - 1) * (theme.bodyFontSize + theme.lineSpacing / 2);
    done_.setPageSize(rowsFor(theme, currentBandH_));
    done_.setItems(buildRows(app.data(), state));
}

bool BoardScene::update(Application& app, double) {
    // 这块板子只读。确认与取消都是关掉它——玩家按哪个键关面板的习惯不一样，
    // 而这里没有任何「选中了什么」的语义要保护。
    if (done_.update(app.engine())) return false;
    if (done_.cancelled()) return false;
    return true;
}

void BoardScene::render(Application& app) {
    engine::Engine& eng = app.engine();
    const ui::Theme& theme = app.theme();

    const engine::Rect panel{kPanelX, kPanelY, kPanelW, kPanelH};
    const std::string title = "记事";
    ui::drawScrim(eng, theme);
    ui::drawPanel(eng, panel, title, theme);

    const engine::Rect content = ui::panelContentArea(panel, title, theme);
    int y = content.y + theme.lineSpacing;
    eng.drawText("眼下", content.x + theme.padding, y, theme.bodyFontSize, theme.goldBright,
                 theme.bodyStyle);
    y += theme.bodyFontSize + theme.lineSpacing;
    for (const std::string& line : currentLines_) {
        eng.drawText(line, content.x + theme.padding, y, theme.bodyFontSize, theme.paper, theme.bodyStyle);
        y += theme.bodyFontSize + theme.lineSpacing / 2;
    }
    if (!where_.empty()) {
        eng.drawText(where_, content.x + theme.padding, y, theme.bodyFontSize - 4, theme.paperDim,
                     theme.bodyStyle);
    }

    const int band = currentBandH_ > 0 ? currentBandH_ : kCurrentBandH;
    const engine::Rect listArea{content.x, content.y + band, content.w,
                                std::max(0, content.h - band - kDetailBandH)};
    // 分隔线：上半截是「眼下」，下半截是「做过的」。没有它，做过的第一行看起来
    // 像是当前目标的第三行。
    ui::drawRuleH(eng, static_cast<float>(listArea.x + theme.padding),
                  static_cast<float>(listArea.y),
                  static_cast<float>(std::max(0, listArea.w - theme.padding * 2)), theme);
    done_.render(eng, listArea, theme);

    // 光标那一行的全文：列表里放不下的（截成「…」的）在这里读得全。
    const int at = done_.selection();
    if (at >= 0 && at < static_cast<int>(done_.items().size())) {
        const ui::ListItem& row = done_.items()[static_cast<std::size_t>(at)];
        std::string full = row.label;
        if (!row.detail.empty()) full += "　" + row.detail;
        const int detailY = listArea.y + listArea.h;
        ui::drawRuleH(eng, static_cast<float>(content.x + theme.padding), static_cast<float>(detailY),
                      static_cast<float>(std::max(0, content.w - theme.padding * 2)), theme);
        // 不用 drawTextBlock：它四周各留一圈 padding，这条 64 像素的栏里一行都摆不下。
        const int width = std::max(1, content.w - theme.padding * 2);
        const std::vector<engine::LayoutLine> lines = engine::layoutText(
            full, width, [&eng](const std::string& s) { return eng.measureText(s, kDetailFont).x; });
        int lineY = detailY + 8;
        for (std::size_t i = 0; i < lines.size() && i < 2; ++i) {
            eng.drawText(lines[i].text, content.x + theme.padding, lineY, kDetailFont, theme.paperDim, theme.bodyStyle);
            lineY += kDetailFont + 6;
        }
    }
}

}  // namespace fanren::game
