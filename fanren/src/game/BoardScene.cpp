#include "game/BoardScene.h"

#include <algorithm>
#include <utility>

#include "core/model/AttackCategory.h"
#include "core/rules/Lore.h"
#include "core/rules/Objectives.h"
#include "core/rules/Quests.h"
#include "core/rules/Tutorials.h"
#include "engine/TextLayout.h"
#include "game/Application.h"
#include "game/Guide.h"
#include "game/ShopScene.h"
#include "game/Wording.h"
#include "io/GuideLoader.h"
#include "ui/Widgets.h"

namespace fanren::game {
namespace {

// 第一期地基包：四页之后多了一条页签，面板比从前（200, 70, 880, 580）高一截，主线页下半截仍放得下十行以上。
constexpr int kPanelX = 180;
constexpr int kPanelY = 36;
constexpr int kPanelW = 920;
constexpr int kPanelH = 648;

constexpr int kTabBandH = 40;
constexpr int kTabFont = 20;
// 主线页上半截至少这么高（只有「眼下」与一行行动句时）；要点、详情、地点各多一行就往下长。
constexpr int kCurrentBandH = 96;
constexpr int kSmallFont = 18;
constexpr int kSmallGap = 6;
// 最底下一条：光标停着的那一行的全文 / 详情，折行显示。
constexpr int kDetailBandH = 64;
constexpr int kDetailFont = 18;
// 支线、指南、见闻三页：左边列表占这么宽，右边是选中那一条的详情。
constexpr int kSplitListW = 360;
constexpr int kPaneFont = 19;
constexpr int kHintFont = 16;

constexpr const char* kTabKeys[BoardScene::kPageCount] = {"ui.guide.tab.main", "ui.guide.tab.side",
                                                          "ui.guide.tab.guide", "ui.guide.tab.lore"};

int rowsFor(const ui::Theme& theme, int currentBandH) {
    const int contentH = kPanelH - ui::panelTitleBandHeight(/*hasTitle=*/true, theme) - kTabBandH - currentBandH -
                         kDetailBandH;
    return std::max(1, ui::listRowsThatFit(contentH, theme));
}

std::string mapName(const core::GameData& data, const std::string& mapId) {
    const std::string key = core::mapDisplayNameKey(mapId);
    return key.empty() ? mapId : data.lookupText(key);
}

std::vector<std::string> wrap(engine::Engine& eng, const std::string& text, int width, int font) {
    std::vector<std::string> out;
    for (const engine::LayoutLine& line :
         engine::layoutText(text, std::max(1, width), [&eng, font](const std::string& s) { return eng.measureText(s, font).x; })) {
        out.push_back(line.text);
    }
    if (out.empty() && !text.empty()) out.push_back(text);   // 无头量不出字：原样一行
    return out;
}

std::string categoryLabel(const core::GameData& data, const std::string& category) {
    return data.lookupText("ui.guide.lore.cat." + category);
}

}  // namespace

int BoardScene::listPageRows(const ui::Theme& theme) { return rowsFor(theme, kCurrentBandH); }

std::vector<ui::ListItem> BoardScene::sideRows(const core::GameData& data, const core::GameState& state) {
    std::vector<ui::ListItem> rows;
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
                    if (!entry.step->targetMap.empty()) row.label += "（" + mapName(data, entry.step->targetMap) + "）";
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
    return rows;
}

std::vector<ui::ListItem> BoardScene::mainRows(const core::GameData& data, const core::GameState& state) {
    std::vector<ui::ListItem> done;
    for (const core::Objective* step : rules::completedObjectives(data.objectives, state)) {
        ui::ListItem row;
        row.label = data.lookupText(step->textKey);
        done.push_back(std::move(row));
    }
    // 倒序：刚做完的那件排在最上面。翻到第三页去找「我上一步干了什么」是荒谬的。
    std::reverse(done.begin(), done.end());
    return done;
}

std::vector<ui::ListItem> BoardScene::buildRows(const core::GameData& data, const core::GameState& state) {
    std::vector<ui::ListItem> rows = sideRows(data, state);
    for (ui::ListItem& row : mainRows(data, state)) rows.push_back(std::move(row));
    return rows;
}

std::string BoardScene::rewardLine(const core::GameData& data, const core::GameState& state, const core::Quest& quest) {
    if (quest.rewards.empty()) {
        // 回报由脚本按情形发的支线：写数据里那一句（reward_key）。
        return quest.rewardKey.empty() ? std::string{} : data.lookupText("ui.guide.board.reward") + data.lookupText(quest.rewardKey);
    }
    std::string line = data.lookupText("ui.guide.board.reward");
    for (std::size_t i = 0; i < quest.rewards.size(); ++i) {
        const core::BagEntry& reward = quest.rewards[i];
        if (i > 0) line += "、";
        std::string name;
        // 钱不叫「灵石」叫什么，看剧情走到了哪儿（G-25：第 1–5 章不许上屏「灵石」）。
        if (reward.itemId == kSpiritStoneItemId) {
            name = currencyName(wordingStage(state));
        } else if (const core::Item* item = data.findItem(reward.itemId)) {
            name = item->name;
        } else {
            name = reward.itemId;   // 画面上一眼看出是哪样东西没进 data
        }
        line += name + " × " + std::to_string(std::max(1, reward.count));
        if (reward.herbAge > 0) line += "（" + std::to_string(reward.herbAge) + " 年）";
    }
    return line;
}

std::vector<std::string> BoardScene::loreBeastLines(const core::GameData& data, const core::GameState& state,
                                                    const std::map<std::string, core::BattleSetup>& battles,
                                                    const core::LoreEntry& entry) {
    std::vector<std::string> lines;
    if (entry.category != "yaoshou" || entry.roleId.empty()) return lines;
    const int known = rules::loreKnownWeaknesses(entry, state);
    lines.push_back(data.lookupText("ui.guide.lore.weak") +
                    (known != 0 ? core::categoryNames(known) : data.lookupText("ui.guide.lore.weak_none")));
    std::string drops;
    for (const std::string& id : rules::loreDrops(entry, battles)) {
        if (!drops.empty()) drops += "、";
        const core::Item* item = data.findItem(id);
        drops += item != nullptr ? item->name : id;
    }
    if (!drops.empty()) lines.push_back(data.lookupText("ui.guide.lore.drops") + drops);
    return lines;
}

void BoardScene::onEnter(Application& app) {
    list_.reset();
    rebuild(app);
}

void BoardScene::turnPage(Application& app, int step) {
    const int next = ((static_cast<int>(page_) + step) % kPageCount + kPageCount) % kPageCount;
    page_ = static_cast<Page>(next);
    list_.reset();
    rebuild(app);
}

void BoardScene::rebuild(Application& app) {
    const core::GameData& data = app.data();
    const core::GameState& state = app.state();
    const ui::Theme& theme = app.theme();
    engine::Engine& eng = app.engine();
    quests_.clear();
    cards_.clear();
    lore_.clear();
    steps_.clear();
    current_.clear();
    currentBandH_ = 0;

    std::vector<ui::ListItem> items;
    switch (page_) {
        case Page::Main: {
            const engine::Rect content =
                ui::panelContentArea(engine::Rect{kPanelX, kPanelY, kPanelW, kPanelH}, "记事", theme);
            const int width = content.w - theme.padding * 2;
            const std::vector<core::Objective>& chain = data.objectives;
            int height = 0;
            const auto add = [&](const std::string& text, int font, engine::Color color, int gap) {
                for (const std::string& line : wrap(eng, text, width, font)) {
                    current_.push_back(Line{line, font, color});
                    height += font + gap;
                }
            };
            if (const core::Objective* step = rules::currentObjective(chain, state)) {
                add(guide::objectiveActionLine(data, state, *step), theme.bodyFontSize, theme.paper, theme.lineSpacing / 2);
                if (!step->hintKey.empty()) add(app.text(step->hintKey), kSmallFont, theme.gold, kSmallGap);
                if (!step->detailKey.empty()) add(app.text(step->detailKey), kSmallFont, theme.paperDim, kSmallGap);
                add(data.lookupText("ui.guide.board.where") + mapName(data, step->targetMap), kSmallFont,
                    theme.paperDim, kSmallGap);
            } else {
                // 目标链走完（或这份数据里压根没有目标链）。说清楚是哪一种：
                // 一片空白会被当成面板坏了，而这两种情形要靠完全不同的方式去查。
                add(chain.empty() ? "此处暂无记事。" : "眼下没有非做不可的事。", theme.bodyFontSize, theme.paper,
                    theme.lineSpacing / 2);
            }
            // 「眼下」那一行标题 + 上下留白。
            currentBandH_ = std::max(kCurrentBandH, height + theme.bodyFontSize + theme.lineSpacing * 3);
            list_.setEmptyHint("（还没做成什么事）");
            list_.setPageSize(rowsFor(theme, currentBandH_));
            for (const core::Objective* step : rules::completedObjectives(chain, state)) steps_.push_back(step);
            std::reverse(steps_.begin(), steps_.end());
            items = mainRows(data, state);
            break;
        }
        case Page::Side: {
            for (const rules::QuestEntry& entry : rules::sideQuestJournal(data.quests, state)) {
                if (entry.status != rules::QuestStatus::NotAccepted) quests_.push_back(entry.quest);
            }
            for (const core::Quest* quest : quests_) {
                ui::ListItem row;
                row.label = data.lookupText(quest->titleKey);
                const rules::QuestStatus status = rules::questStatus(*quest, data.objectives, state);
                row.detail = status == rules::QuestStatus::Failed      ? kFailedTag
                             : status == rules::QuestStatus::Completed ? kCompletedTag
                                                                       : kActiveTag;
                items.push_back(std::move(row));
            }
            list_.setEmptyHint(data.lookupText("ui.guide.board.empty.side"));
            break;
        }
        case Page::Guide: {
            cards_ = rules::seenTutorials(data.tutorials, state);
            for (const core::TutorialCard* card : cards_) items.push_back(ui::ListItem{app.text(card->titleKey)});
            list_.setEmptyHint(data.lookupText("ui.guide.board.empty.guide"));
            break;
        }
        case Page::Lore: {
            // 按类别排（妖兽、灵草、人物、所闻），类内照表的次序。
            const std::vector<const core::LoreEntry*> unlocked = rules::unlockedLore(data.lore, state);
            for (const std::string& category : io::loreCategories()) {
                for (const core::LoreEntry* entry : unlocked) {
                    if (entry->category != category) continue;
                    lore_.push_back(entry);
                    ui::ListItem row;
                    row.label = data.lookupText(entry->titleKey);
                    row.detail = categoryLabel(data, entry->category);
                    items.push_back(std::move(row));
                }
            }
            list_.setEmptyHint(data.lookupText("ui.guide.board.empty.lore"));
            break;
        }
    }
    if (page_ != Page::Main) {
        const int contentH = kPanelH - ui::panelTitleBandHeight(true, theme) - kTabBandH;
        list_.setPageSize(std::max(1, ui::listRowsThatFit(contentH, theme)));
    }
    list_.setItems(std::move(items));
}

std::vector<std::string> BoardScene::paneLines(Application& app) const {
    const core::GameData& data = app.data();
    const core::GameState& state = app.state();
    std::vector<std::string> lines;
    const int at = list_.selection();
    switch (page_) {
        case Page::Main: {
            for (const Line& line : current_) lines.push_back(line.text);
            if (at >= 0 && at < static_cast<int>(steps_.size())) {
                const core::Objective* step = steps_[static_cast<std::size_t>(at)];
                lines.push_back(app.text(step->detailKey.empty() ? step->textKey : step->detailKey));
            }
            break;
        }
        case Page::Side: {
            if (at < 0 || at >= static_cast<int>(quests_.size())) break;
            const core::Quest& quest = *quests_[static_cast<std::size_t>(at)];
            lines.push_back(data.lookupText(quest.titleKey));
            lines.push_back(data.lookupText(quest.summaryKey));
            const rules::QuestStatus status = rules::questStatus(quest, data.objectives, state);
            if (status == rules::QuestStatus::Active) {
                if (const core::QuestStep* step = rules::currentQuestStep(quest, data.objectives, state)) {
                    std::string now = data.lookupText("ui.guide.board.step") + data.lookupText(step->textKey);
                    if (!step->targetMap.empty()) now += "（" + mapName(data, step->targetMap) + "）";
                    lines.push_back(now);
                }
            } else if (status == rules::QuestStatus::Failed) {
                lines.push_back(std::string(kFailedTag) + "　" + data.lookupText(quest.failTextKey));
            } else if (status == rules::QuestStatus::Completed) {
                lines.push_back(kCompletedTag);
            }
            const std::string reward = rewardLine(data, state, quest);
            if (!reward.empty()) lines.push_back(reward);
            break;
        }
        case Page::Guide: {
            if (at < 0 || at >= static_cast<int>(cards_.size())) break;
            const core::TutorialCard& card = *cards_[static_cast<std::size_t>(at)];
            lines.push_back(app.text(card.titleKey));
            for (const std::string& key : card.bodyKeys) lines.push_back(app.text(key));
            break;
        }
        case Page::Lore: {
            if (at < 0 || at >= static_cast<int>(lore_.size())) break;
            const core::LoreEntry& entry = *lore_[static_cast<std::size_t>(at)];
            lines.push_back(data.lookupText(entry.titleKey));
            lines.push_back(data.lookupText(entry.bodyKey));
            for (std::string& line : loreBeastLines(data, state, app.battles(), entry)) lines.push_back(std::move(line));
            break;
        }
    }
    return lines;
}

bool BoardScene::update(Application& app, double) {
    engine::Engine& e = app.engine();
    using Key = engine::Engine::Key;
    // 左右翻页（手柄的十字键左右同样是这两个键）。列表自己的左右是翻一屏，这里不交给它。
    if (e.keyPressed(Key::Left) || e.keyPressed(Key::Right)) {
        turnPage(app, e.keyPressed(Key::Right) ? 1 : -1);
        e.playSfx("ui_cursor");
        return true;
    }
    if (e.keyPressed(Key::Up)) list_.moveUp();
    if (e.keyPressed(Key::Down)) list_.moveDown();
    if (e.keyPressed(Key::Cancel)) return false;
    if (e.keyPressed(Key::Confirm)) {
        // 指南页：确认重看那一张。别的页这块板子只读，确认与取消都是关掉它——
        // 玩家按哪个键关面板的习惯不一样，而这里没有任何「选中了什么」的语义要保护。
        if (page_ == Page::Guide) {
            const int at = list_.selection();
            if (at >= 0 && at < static_cast<int>(cards_.size())) {
                e.playSfx("ui_confirm");
                app.replayTutorial(cards_[static_cast<std::size_t>(at)]->id);
            }
            return true;
        }
        return false;
    }
    return true;
}

void BoardScene::render(Application& app) {
    engine::Engine& eng = app.engine();
    const ui::Theme& theme = app.theme();

    const engine::Rect panel{kPanelX, kPanelY, kPanelW, kPanelH};
    const std::string title = "记事";
    ui::drawScrim(eng, theme);
    ui::drawPanel(eng, panel, title, theme);
    const engine::Rect inner = ui::panelContentArea(panel, title, theme);

    // 页签：四个词排成一行，当前页金字加一道金线。
    int tabX = inner.x + theme.padding;
    const int tabY = inner.y + 6;
    for (int i = 0; i < kPageCount; ++i) {
        const std::string label = app.data().lookupText(kTabKeys[i]);
        const bool on = i == static_cast<int>(page_);
        const engine::Point size = eng.measureText(label, kTabFont);
        eng.drawText(label, tabX, tabY, kTabFont, on ? theme.goldBright : theme.paperDim, theme.bodyStyle);
        if (on) {
            eng.fillRect(engine::RectF{static_cast<float>(tabX), static_cast<float>(tabY + kTabFont + 6),
                                       static_cast<float>(std::max(size.x, kTabFont * 2)), 2.f},
                         theme.gold, engine::BlendMode::Alpha);
        }
        tabX += std::max(size.x, kTabFont * 2) + 36;
    }
    const std::string hint = app.data().lookupText(page_ == Page::Guide ? "ui.guide.board.hint.guide"
                                                                        : "ui.guide.board.hint");
    const engine::Point hintSize = eng.measureText(hint, kHintFont);
    eng.drawText(hint, inner.x + inner.w - theme.padding - hintSize.x, tabY + 4, kHintFont, theme.paperDim,
                 theme.bodyStyle);
    ui::drawRuleH(eng, static_cast<float>(inner.x + theme.padding), static_cast<float>(inner.y + kTabBandH - 4),
                  static_cast<float>(std::max(0, inner.w - theme.padding * 2)), theme);

    const engine::Rect content{inner.x, inner.y + kTabBandH, inner.w, std::max(0, inner.h - kTabBandH)};
    if (page_ == Page::Main) {
        renderMain(app, content);
    } else {
        renderSplit(app, content);
    }
}

void BoardScene::renderMain(Application& app, const engine::Rect& content) {
    engine::Engine& eng = app.engine();
    const ui::Theme& theme = app.theme();
    int y = content.y + theme.lineSpacing;
    eng.drawText(app.data().lookupText("ui.guide.board.now"), content.x + theme.padding, y, theme.bodyFontSize,
                 theme.goldBright, theme.bodyStyle);
    y += theme.bodyFontSize + theme.lineSpacing;
    for (const Line& line : current_) {
        eng.drawText(line.text, content.x + theme.padding, y, line.font, line.color, theme.bodyStyle);
        y += line.font + (line.font == theme.bodyFontSize ? theme.lineSpacing / 2 : kSmallGap);
    }

    const int band = currentBandH_ > 0 ? currentBandH_ : kCurrentBandH;
    const engine::Rect listArea{content.x, content.y + band, content.w, std::max(0, content.h - band - kDetailBandH)};
    // 分隔线：上半截是「眼下」，下半截是「做过的」。没有它，做过的第一行看起来像是当前目标的又一行。
    ui::drawRuleH(eng, static_cast<float>(listArea.x + theme.padding), static_cast<float>(listArea.y),
                  static_cast<float>(std::max(0, listArea.w - theme.padding * 2)), theme);
    list_.render(eng, listArea, theme);

    // 光标那一步的详情（原来的长句）：列表里只写行动句，来龙去脉在这里读得全。
    const int at = list_.selection();
    if (at < 0 || at >= static_cast<int>(steps_.size())) return;
    const core::Objective* step = steps_[static_cast<std::size_t>(at)];
    const std::string full = app.text(step->detailKey.empty() ? step->textKey : step->detailKey);
    const int detailY = listArea.y + listArea.h;
    ui::drawRuleH(eng, static_cast<float>(content.x + theme.padding), static_cast<float>(detailY),
                  static_cast<float>(std::max(0, content.w - theme.padding * 2)), theme);
    // 不用 drawTextBlock：它四周各留一圈 padding，这条 64 像素的栏里一行都摆不下。
    int lineY = detailY + 8;
    const std::vector<std::string> lines = wrap(eng, full, content.w - theme.padding * 2, kDetailFont);
    for (std::size_t i = 0; i < lines.size() && i < 2; ++i) {
        eng.drawText(lines[i], content.x + theme.padding, lineY, kDetailFont, theme.paperDim, theme.bodyStyle);
        lineY += kDetailFont + 6;
    }
}

void BoardScene::renderSplit(Application& app, const engine::Rect& content) {
    engine::Engine& eng = app.engine();
    const ui::Theme& theme = app.theme();
    const engine::Rect listArea{content.x, content.y, kSplitListW, content.h};
    list_.render(eng, listArea, theme);
    // 竖分隔：左边是列表，右边是选中那一条。
    const int paneX = content.x + kSplitListW + 12;
    eng.fillRect(engine::RectF{static_cast<float>(paneX - 6), static_cast<float>(content.y + 8), 1.f,
                               static_cast<float>(std::max(0, content.h - 16))},
                 theme.goldDim, engine::BlendMode::Alpha);
    const std::vector<std::string> pane = paneLines(app);
    if (pane.empty()) return;
    const int width = content.x + content.w - theme.padding - paneX - 12;
    int y = content.y + theme.lineSpacing;
    for (std::size_t i = 0; i < pane.size(); ++i) {
        // 第一行是标题：金字；之后是正文，奖励、破绽、掉落那几行用金色压一压，一眼找得到。
        const bool head = i == 0;
        const bool tagged = !head && (pane[i].rfind(app.data().lookupText("ui.guide.board.reward"), 0) == 0 ||
                                      pane[i].rfind(app.data().lookupText("ui.guide.lore.weak"), 0) == 0 ||
                                      pane[i].rfind(app.data().lookupText("ui.guide.lore.drops"), 0) == 0);
        const int font = head ? theme.bodyFontSize : kPaneFont;
        const engine::Color color = head ? theme.goldBright : tagged ? theme.gold : theme.paper;
        for (const std::string& line : wrap(eng, pane[i], width, font)) {
            eng.drawText(line, paneX + 6, y, font, color, theme.bodyStyle);
            y += font + kSmallGap;
        }
        y += head ? theme.lineSpacing : kSmallGap;
    }
}

}  // namespace fanren::game
