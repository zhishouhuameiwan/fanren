#include "game/KeyConfigScene.h"

#include <array>
#include <cstddef>
#include <optional>
#include <utility>

#include "game/Application.h"
#include "ui/Widgets.h"

namespace fanren::game {
namespace {

using Key = engine::Engine::Key;
using KeyChange = engine::Engine::KeyChange;

// 摆位。面板居中；一行 = 列表行高（字号 + 行距，与各面板同一个数）。
// 最右加了一列只读的「手柄」（docs/gamepad.md 第 7 节）：面板从 840 宽加到 980、仍居中，原有各列离面板左边的距离不变。
constexpr engine::Rect kPanel{150, 70, 980, 580};
constexpr int kTopGap = 6;
constexpr int kIndent = 20;          // 动作名比面板内边距再缩进这么多（与设置面板的行名同一处起笔）
constexpr int kFixedX = 190;         // 「固定」那一栏从面板左边起多远
constexpr std::array<int, 2> kSlotCenter{510, 690};   // 两格自定义键位的中线（从面板左边起）
constexpr int kSlotHalfWidth = 75;
constexpr int kPadX = 800;           // 「手柄」那一栏从面板左边起多远（最宽的「十字↑ / 摇杆↑」18 号字 126 像素）
constexpr int kSmallFontSize = 18;
constexpr int kNoteFontSize = 18;
constexpr int kHintFontSize = 16;
constexpr int kRuleGap = 8;

constexpr const char* kSfxCursor = "ui_cursor";
constexpr const char* kSfxConfirm = "ui_confirm";
constexpr const char* kSfxCancel = "ui_cancel";
constexpr const char* kSfxError = "ui_error";
constexpr const char* kSfxClose = "ui_close";

// 文案里点名某个动作的占位（「这是「{action}」的固定键」）。与 Application::text 展开的 {key.<id>} 不是一回事。
constexpr std::string_view kActionSlot = "{action}";

// 说明行的各种话（resultText 用）与其余固有字，keyConfigStrings 一并列出。
constexpr const char* kResultKeys[] = {
    "ui.keys.done.assigned",  "ui.keys.done.unchanged",     "ui.keys.done.swapped",   "ui.keys.done.moved",
    "ui.keys.done.traded",    "ui.keys.done.took",          "ui.keys.done.cleared",   "ui.keys.done.already_empty",
    "ui.keys.done.cancelled", "ui.keys.done.restored",      "ui.keys.reject.fixed",   "ui.keys.reject.reserved",
    "ui.keys.reject.last",    "ui.keys.reject.invalid",
};
constexpr const char* kOtherKeys[] = {
    "ui.keys.title",   "ui.keys.col.action", "ui.keys.col.fixed", "ui.keys.col.slot1", "ui.keys.col.slot2",
    "ui.keys.col.gamepad", "ui.keys.restore",    "ui.keys.empty",     "ui.keys.capture",   "ui.keys.desc",
    "ui.keys.desc.restore", "ui.keys.hint",
    // 最后按的是手柄时换上的那几条（docs/gamepad.md 第 6 节）。
    "ui.keys.capture.pad", "ui.keys.desc.pad", "ui.keys.desc.restore.pad", "ui.keys.hint.pad",
};

// 手柄那一列的分隔（第 7 节：多个手柄键用「 / 」隔开）。
constexpr std::string_view kPadSeparator = " / ";

[[nodiscard]] bool rejected(KeyChange change) {
    return change == KeyChange::RejectedFixed || change == KeyChange::RejectedReserved ||
           change == KeyChange::RejectedLastKey || change == KeyChange::RejectedInvalid;
}

// 把文案里每一处 {action} 换成动作名（一句话里可能点名两次）。
[[nodiscard]] std::string withAction(std::string text, const std::string& name) {
    for (std::size_t at = text.find(kActionSlot); at != std::string::npos;
         at = text.find(kActionSlot, at + name.size())) {
        text.replace(at, kActionSlot.size(), name);
    }
    return text;
}

[[nodiscard]] engine::RectF toRectF(const engine::Rect& r) {
    return engine::RectF{static_cast<float>(r.x), static_cast<float>(r.y), static_cast<float>(r.w),
                         static_cast<float>(r.h)};
}

}  // namespace

engine::Engine::KeyTable keyTableOf(const io::Settings& settings) {
    engine::Engine::KeyTable table = engine::Engine::defaultKeyTable();
    for (const auto& [id, slots] : settings.keys) {
        if (const std::optional<Key> key = engine::Engine::keyFromId(id)) {
            table[static_cast<std::size_t>(*key)] = slots;
        }
    }
    return table;
}

std::map<std::string, io::KeySlots> keySettingsOf(const engine::Engine::KeyTable& table) {
    std::map<std::string, io::KeySlots> keys;
    for (std::size_t k = 0; k < engine::Engine::kKeyCount; ++k) {
        const auto key = static_cast<Key>(k);
        if (table[k] != engine::Engine::defaultCustomKeys(key)) keys[engine::Engine::keyId(key)] = table[k];
    }
    return keys;
}

std::string KeyConfigScene::actionName(const core::GameData& data, Key key) {
    return data.lookupText(std::string("ui.keys.") + engine::Engine::keyId(key));
}

std::string KeyConfigScene::padKeysText(Key key) {
    std::string text;
    for (const engine::Engine::PadButton button : engine::Engine::padButtons(key)) {
        if (!text.empty()) text += kPadSeparator;
        text += engine::Engine::padLabel(button);
    }
    return text;
}

std::vector<std::string> KeyConfigScene::keyConfigStrings(const core::GameData& data) {
    std::vector<std::string> out;
    for (int k = 0; k < kActionRows; ++k) out.push_back(actionName(data, static_cast<Key>(k)));
    for (const char* key : kResultKeys) out.push_back(data.lookupText(key));
    for (const char* key : kOtherKeys) out.push_back(data.lookupText(key));
    return out;
}

std::string KeyConfigScene::resultText(const core::GameData& data, const engine::Engine::KeyChangeResult& result) {
    const std::string other = result.other == Key::Count ? std::string{} : actionName(data, result.other);
    switch (result.change) {
        case KeyChange::Assigned: return data.lookupText("ui.keys.done.assigned");
        case KeyChange::Unchanged: return data.lookupText("ui.keys.done.unchanged");
        case KeyChange::SwappedSlots: return data.lookupText("ui.keys.done.swapped");
        case KeyChange::MovedSlot: return data.lookupText("ui.keys.done.moved");
        case KeyChange::TradedWith: return withAction(data.lookupText("ui.keys.done.traded"), other);
        case KeyChange::TookFrom: return withAction(data.lookupText("ui.keys.done.took"), other);
        case KeyChange::Cleared: return data.lookupText("ui.keys.done.cleared");
        case KeyChange::AlreadyEmpty: return data.lookupText("ui.keys.done.already_empty");
        case KeyChange::RejectedFixed: return withAction(data.lookupText("ui.keys.reject.fixed"), other);
        case KeyChange::RejectedReserved: return data.lookupText("ui.keys.reject.reserved");
        case KeyChange::RejectedLastKey: return withAction(data.lookupText("ui.keys.reject.last"), other);
        case KeyChange::RejectedInvalid: return data.lookupText("ui.keys.reject.invalid");
    }
    return {};
}

void KeyConfigScene::onEnter(Application&) {
    row_ = 0;
    slot_ = 0;
    capturing_ = false;
    note_.clear();
    noteIsError_ = false;
}

void KeyConfigScene::onExit(Application& app) {
    // 在等按键时被关掉（不该发生，但「不该发生」不是「不会发生」）：不收回的话，之后一个逻辑键都不出。
    if (capturing_) app.engine().cancelKeyCapture();
    capturing_ = false;
}

bool KeyConfigScene::update(Application& app, double) {
    engine::Engine& e = app.engine();
    if (capturing_) {
        // 抓键期间引擎不出任何逻辑键：Esc 在这里是抓进来的一个键（作罢），不是「关面板」。
        if (const std::optional<engine::ScanCode> code = e.takeCapturedKey()) {
            finishCapture(app, *code);
        } else if (!e.capturingKey()) {
            // 引擎那边已经不在抓了、也没抓到东西（被别处收回）：别一直卡在「等按键」上。
            capturing_ = false;
            setNote(std::string{}, false);
        }
        return true;
    }
    if (e.keyPressed(Key::Cancel) || e.keyPressed(Key::Menu)) {
        e.playSfx(kSfxClose);
        return false;
    }
    const int rowBefore = row_;
    const int slotBefore = slot_;
    if (e.keyPressed(Key::Up)) row_ = (row_ + kRowCount - 1) % kRowCount;
    if (e.keyPressed(Key::Down)) row_ = (row_ + 1) % kRowCount;
    if (e.keyPressed(Key::Left)) slot_ = 0;
    if (e.keyPressed(Key::Right)) slot_ = 1;
    if (row_ != rowBefore || slot_ != slotBefore) {
        e.playSfx(kSfxCursor);
        note_.clear();
        noteIsError_ = false;
    }
    if (e.keyPressed(Key::Confirm)) {
        if (row_ == kRestoreRow) {
            restoreDefaults(app);
        } else {
            startCapture(app);
        }
    }
    return true;
}

void KeyConfigScene::setNote(std::string text, bool error) {
    note_ = std::move(text);
    noteIsError_ = error;
}

void KeyConfigScene::startCapture(Application& app) {
    app.engine().playSfx(kSfxConfirm);
    app.engine().beginKeyCapture();
    capturing_ = true;
    setNote(app.text("ui.keys.capture"), false);
}

void KeyConfigScene::finishCapture(Application& app, engine::ScanCode code) {
    capturing_ = false;
    engine::Engine& e = app.engine();
    if (code == engine::Engine::kEscapeCode) {
        setNote(app.text("ui.keys.done.cancelled"), false);
        e.playSfx(kSfxCancel);
        return;
    }
    const auto action = static_cast<Key>(row_);
    const engine::Engine::KeyTable table = keyTableOf(app.settings());
    const bool clear = code == engine::Engine::kBackspaceCode || code == engine::Engine::kDeleteCode;
    const engine::Engine::KeyChangeResult result =
        clear ? engine::Engine::clearKey(table, action, slot_) : engine::Engine::assignKey(table, action, slot_, code);
    if (result.table != table) {
        // 立刻生效：settings → 引擎的键位表，提示文案里的 {key.<id>} 下一帧就跟着变。
        io::Settings next = app.settings();
        next.keys = keySettingsOf(result.table);
        app.setSettings(next);
    }
    const bool refused = rejected(result.change);
    setNote(resultText(app.data(), result), refused);
    e.playSfx(refused ? kSfxError : kSfxConfirm);
}

void KeyConfigScene::restoreDefaults(Application& app) {
    io::Settings next = app.settings();
    next.keys.clear();
    app.setSettings(next);
    setNote(app.text("ui.keys.done.restored"), false);
    app.engine().playSfx(kSfxConfirm);
}

// ---------------------------------------------------------------------------
// 画
// ---------------------------------------------------------------------------

void KeyConfigScene::render(Application& app) {
    engine::Engine& e = app.engine();
    const ui::Theme& theme = app.theme();
    const int rowH = ui::listRowHeight(theme);

    // 设置面板在底下让开（只剩它压的那层纱也不画），这里自己压一层：底下的标题画面 / 主菜单底图退成背景。
    ui::drawScrim(e, theme);
    const std::string title = app.text("ui.keys.title");
    ui::drawPanel(e, kPanel, title, theme);
    const engine::Rect content = ui::panelContentArea(kPanel, title, theme);

    int y = content.y + kTopGap;
    const int headerY = y + (theme.bodyFontSize - kSmallFontSize);
    e.drawText(app.text("ui.keys.col.action"), kPanel.x + theme.padding + kIndent, headerY, kSmallFontSize,
               theme.gold, theme.bodyStyle);
    e.drawText(app.text("ui.keys.col.fixed"), kPanel.x + kFixedX, headerY, kSmallFontSize, theme.gold,
               theme.bodyStyle);
    for (std::size_t s = 0; s < kSlotCenter.size(); ++s) {
        const std::string head = app.text(s == 0 ? "ui.keys.col.slot1" : "ui.keys.col.slot2");
        const int w = e.measureText(head, kSmallFontSize).x;
        e.drawText(head, kPanel.x + kSlotCenter[s] - w / 2, headerY, kSmallFontSize, theme.gold, theme.bodyStyle);
    }
    e.drawText(app.text("ui.keys.col.gamepad"), kPanel.x + kPadX, headerY, kSmallFontSize, theme.gold, theme.bodyStyle);
    y += rowH;
    for (int r = 0; r < kActionRows; ++r) {
        drawActionRow(app, r, y);
        y += rowH;
    }
    // 「恢复默认键位」不是一个动作：一道分隔线把它与上面隔开。
    ui::drawRuleH(e, static_cast<float>(content.x + theme.padding), static_cast<float>(y + kRuleGap / 2),
                  static_cast<float>(content.w - theme.padding * 2), theme);
    y += kRuleGap;
    drawRestoreRow(app, y);
    y += rowH + kRuleGap;
    drawNote(app, y);

    const std::string hint = app.text("ui.keys.hint");
    const engine::Point size = e.measureText(hint, kHintFontSize);
    e.drawText(hint, kPanel.x + kPanel.w - theme.padding - size.x, kPanel.y + kPanel.h - theme.padding - size.y + 4,
               kHintFontSize, theme.paperDim, theme.bodyStyle);
}

void KeyConfigScene::drawActionRow(Application& app, int row, int y) const {
    engine::Engine& e = app.engine();
    const ui::Theme& theme = app.theme();
    const auto key = static_cast<Key>(row);
    const bool selected = row == row_;
    if (selected) {
        ui::drawSelection(e,
                          engine::Rect{kPanel.x + theme.padding / 2, y - theme.lineSpacing / 2,
                                       kPanel.w - theme.padding, ui::listRowHeight(theme)},
                          theme);
    }
    e.drawText(actionName(app.data(), key), kPanel.x + theme.padding + kIndent, y, theme.bodyFontSize,
               selected ? theme.goldBright : theme.paper, theme.bodyStyle);

    // 固定键：灰，不可改。没有固定键的动作写「—」。
    std::string fixed;
    for (const engine::ScanCode code : engine::Engine::fixedKeys(key)) {
        if (!fixed.empty()) fixed += "　";
        fixed += engine::Engine::keyLabel(code);
    }
    if (fixed.empty()) fixed = app.text("ui.keys.empty");
    e.drawText(fixed, kPanel.x + kFixedX, y + (theme.bodyFontSize - kSmallFontSize) / 2 + 1, kSmallFontSize,
               theme.paperDim, theme.bodyStyle);
    // 手柄：只读，画法同「固定」那一列（灰，光标选不到；docs/gamepad.md 第 7 节）。手柄键位固定，这里改的只是键盘。
    e.drawText(padKeysText(key), kPanel.x + kPadX, y + (theme.bodyFontSize - kSmallFontSize) / 2 + 1, kSmallFontSize,
               theme.paperDim, theme.bodyStyle);

    const engine::Engine::KeySlots slots = keyTableOf(app.settings())[static_cast<std::size_t>(row)];
    for (std::size_t s = 0; s < slots.size(); ++s) {
        const bool cursor = selected && static_cast<int>(s) == slot_;
        const int center = kPanel.x + kSlotCenter[s];
        if (cursor) {
            // 光标所在的那一格框起来；等按键时框亮一档。
            const engine::Rect cell{center - kSlotHalfWidth, y - 3, kSlotHalfWidth * 2, theme.bodyFontSize + 6};
            ui::drawFrame(e, toRectF(cell), capturing_ ? theme.goldBright : theme.gold);
        }
        const std::string label = slots[s] != 0 ? engine::Engine::keyLabel(slots[s]) : app.text("ui.keys.empty");
        const int w = e.measureText(label, theme.bodyFontSize).x;
        const engine::Color color = slots[s] == 0 ? theme.paperDim : cursor ? theme.goldBright : theme.paper;
        e.drawText(label, center - w / 2, y, theme.bodyFontSize, color, theme.bodyStyle);
    }
}

void KeyConfigScene::drawRestoreRow(Application& app, int y) const {
    engine::Engine& e = app.engine();
    const ui::Theme& theme = app.theme();
    const bool selected = row_ == kRestoreRow;
    if (selected) {
        ui::drawSelection(e,
                          engine::Rect{kPanel.x + theme.padding / 2, y - theme.lineSpacing / 2,
                                       kPanel.w - theme.padding, ui::listRowHeight(theme)},
                          theme);
    }
    e.drawText(app.text("ui.keys.restore"), kPanel.x + theme.padding + kIndent, y, theme.bodyFontSize,
               selected ? theme.goldBright : theme.paper, theme.bodyStyle);
}

void KeyConfigScene::drawNote(Application& app, int top) const {
    engine::Engine& e = app.engine();
    const ui::Theme& theme = app.theme();
    ui::drawRuleH(e, static_cast<float>(kPanel.x + theme.padding), static_cast<float>(top),
                  static_cast<float>(kPanel.w - theme.padding * 2), theme);
    const std::string text = !note_.empty()          ? note_
                             : row_ == kRestoreRow ? app.text("ui.keys.desc.restore")
                                                   : app.text("ui.keys.desc");
    // 与设置面板的说明行同一个画法：drawTextBlock 四周各让一个 padding，区域往上提一截。
    const int textTop = top + 10;
    const engine::Rect area{kPanel.x, textTop - theme.padding, kPanel.w,
                            2 * (kNoteFontSize + theme.lineSpacing) + theme.padding * 2};
    ui::Theme tone = theme;
    if (!note_.empty() && !capturing_) tone.paper = noteIsError_ ? theme.cinnabar : theme.jade;
    if (capturing_) tone.paper = theme.goldBright;
    ui::drawTextBlock(e, text, area, kNoteFontSize, tone);
}

}  // namespace fanren::game
