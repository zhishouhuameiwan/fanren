#include "game/SettingsScene.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <memory>
#include <utility>

#include "game/Application.h"
#include "game/KeyConfigScene.h"

namespace fanren::game {
namespace {

// 摆位。面板居中；一行 = 列表行高（字号 + 行距，与各面板同一个数）。
// 「手柄震动」一行加进来时（docs/gamepad.md 第 8 节）高 592 → 622、仍上下居中，说明行与按键提示的间距不变。
constexpr engine::Rect kPanel{330, 49, 620, 622};
constexpr int kTopGap = 6;          // 标题带之下、第一个分组之上
constexpr int kIndent = 20;         // 行名比分组名缩进这么多
constexpr int kValueLeft = 300;     // 值那一栏从面板左边起多远
constexpr int kGroupFontSize = 18;
constexpr int kNoteFontSize = 18;
constexpr int kHintFontSize = 16;
constexpr int kRuleGap = 8;         // 「恢复默认」上面那道分隔线与上下两行的间距

// 音量那一栏：十格方块 + 右边的数字。
constexpr int kCellSize = 14;
constexpr int kCellStep = 20;

// 两档 / 多档的值：〈 值 〉。值写在一格固定宽的槽正中，两个括号钉在槽的两端，左右键按下去不跳位。
constexpr int kChoiceHalfWidth = 84;

// 音效 id（docs/audio.md 第 4 节）。调音效音量时在新增益下响一声 ui_confirm 试听（契约 5.2）。
constexpr const char* kSfxCursor = "ui_cursor";
constexpr const char* kSfxConfirm = "ui_confirm";
constexpr const char* kSfxClose = "ui_close";
constexpr const char* kSfxError = "ui_error";

struct RowText {
    const char* label;
    const char* desc;
};

// 行的名字与说明（下标即 SettingsScene::kBgmVolume … kRestore）。
constexpr std::array<RowText, SettingsScene::kRowCount> kRowText{{
    {"ui.settings.bgm_volume", "ui.settings.desc.bgm_volume"},
    {"ui.settings.sfx_volume", "ui.settings.desc.sfx_volume"},
    {"ui.settings.fullscreen", "ui.settings.desc.fullscreen"},
    {"ui.settings.scale", "ui.settings.desc.scale"},
    {"ui.settings.vsync", "ui.settings.desc.vsync"},
    {"ui.settings.effects", "ui.settings.desc.effects"},
    {"ui.settings.screen_shake", "ui.settings.desc.screen_shake"},
    {"ui.settings.text_speed", "ui.settings.desc.text_speed"},
    {"ui.settings.pad_rumble", "ui.settings.desc.pad_rumble"},
    {"ui.settings.keys", "ui.settings.desc.keys"},
    {"ui.settings.restore", "ui.settings.desc.restore"},
}};

// 三个分组：名字、从第几行起、几行。「恢复默认」不进分组，单独排在分隔线下面。
struct Group {
    const char* label;
    int first;
    int count;
};
constexpr std::array<Group, 3> kGroups{{
    {"ui.settings.group.sound", SettingsScene::kBgmVolume, 2},
    {"ui.settings.group.display", SettingsScene::kFullscreen, 5},
    {"ui.settings.group.play", SettingsScene::kTextSpeed, 3},
}};

// 各档的叫法。
constexpr const char* kValueKeys[] = {
    "ui.settings.value.window", "ui.settings.value.fullscreen", "ui.settings.value.integer",
    "ui.settings.value.fit",    "ui.settings.value.on",         "ui.settings.value.off",
    "ui.settings.value.full",   "ui.settings.value.lite",       "ui.settings.value.slow",
    "ui.settings.value.normal", "ui.settings.value.fast",       "ui.settings.value.instant",
};

// 固有字里其余几条：标题、说明行的几种话、按键提示（键盘版与手柄版，docs/gamepad.md 第 6 节）。
constexpr const char* kOtherKeys[] = {
    "ui.settings.title",           "ui.settings.restored", "ui.settings.save_failed",
    "ui.settings.save_failed.again", "ui.settings.hint",   "ui.settings.hint.pad",
};

const char* textSpeedKey(io::TextSpeed speed) {
    switch (speed) {
        case io::TextSpeed::Slow: return "ui.settings.value.slow";
        case io::TextSpeed::Normal: return "ui.settings.value.normal";
        case io::TextSpeed::Fast: return "ui.settings.value.fast";
        case io::TextSpeed::Instant: return "ui.settings.value.instant";
    }
    return "ui.settings.value.normal";
}

const char* onOffKey(bool on) {
    return on ? "ui.settings.value.on" : "ui.settings.value.off";
}

constexpr int kTextSpeedCount = 4;   // io::TextSpeed 的档数；枚举的次序即由慢到快

}  // namespace

std::string SettingsScene::rowLabel(const core::GameData& data, int row) {
    if (row < 0 || row >= kRowCount) return {};
    return data.lookupText(kRowText[static_cast<std::size_t>(row)].label);
}

std::string SettingsScene::valueText(const core::GameData& data, const io::Settings& settings, int row) {
    switch (row) {
        case kBgmVolume: return std::to_string(settings.bgmVolume);
        case kSfxVolume: return std::to_string(settings.sfxVolume);
        case kFullscreen:
            return data.lookupText(settings.fullscreen ? "ui.settings.value.fullscreen" : "ui.settings.value.window");
        case kScale:
            return data.lookupText(settings.scale == io::DisplayScale::Integer ? "ui.settings.value.integer"
                                                                               : "ui.settings.value.fit");
        case kVSync: return data.lookupText(onOffKey(settings.vsync));
        case kEffects:
            return data.lookupText(settings.effects == io::EffectsLevel::Full ? "ui.settings.value.full"
                                                                              : "ui.settings.value.lite");
        case kScreenShake: return data.lookupText(onOffKey(settings.screenShake));
        case kTextSpeed: return data.lookupText(textSpeedKey(settings.textSpeed));
        case kPadRumble: return data.lookupText(onOffKey(settings.padRumble));
        default: return {};
    }
}

std::vector<std::string> SettingsScene::settingsStrings(const core::GameData& data) {
    std::vector<std::string> out;
    for (const RowText& row : kRowText) {
        out.push_back(data.lookupText(row.label));
        out.push_back(data.lookupText(row.desc));
    }
    for (const Group& group : kGroups) out.push_back(data.lookupText(group.label));
    for (const char* key : kValueKeys) out.push_back(data.lookupText(key));
    for (const char* key : kOtherKeys) out.push_back(data.lookupText(key));
    return out;
}

io::Settings SettingsScene::adjusted(const io::Settings& settings, int row, int dir) {
    io::Settings next = settings;
    if (dir == 0) return next;
    const int step = dir > 0 ? 1 : -1;
    switch (row) {
        case kBgmVolume:
            next.bgmVolume = std::clamp(next.bgmVolume + step, 0, io::kVolumeLevels);
            break;
        case kSfxVolume:
            next.sfxVolume = std::clamp(next.sfxVolume + step, 0, io::kVolumeLevels);
            break;
        // 两档的：← 取契约第 1 节表里写在前面的那一档，→ 取后面那一档，到头就停。
        // 不做「按一下翻一下」：左右键带自动重复，按住会让全屏与窗口每秒来回切十几次（有光敏风险）。
        case kFullscreen:
            next.fullscreen = step > 0;   // 窗口 / 全屏
            break;
        case kScale:
            next.scale = step > 0 ? io::DisplayScale::Fit : io::DisplayScale::Integer;   // 整数倍 / 铺满
            break;
        case kVSync:
            next.vsync = step < 0;   // 开 / 关
            break;
        case kEffects:
            next.effects = step > 0 ? io::EffectsLevel::Lite : io::EffectsLevel::Full;   // 完整 / 精简
            break;
        case kScreenShake:
            next.screenShake = step < 0;   // 开 / 关
            break;
        case kTextSpeed: {
            const int index = std::clamp(static_cast<int>(next.textSpeed) + step, 0, kTextSpeedCount - 1);
            next.textSpeed = static_cast<io::TextSpeed>(index);
            break;
        }
        case kPadRumble:
            next.padRumble = step < 0;   // 开 / 关
            break;
        default:
            break;   // 按键设置、恢复默认：不吃左右键
    }
    return next;
}

void SettingsScene::onEnter(Application& app) {
    opened_ = app.settings();
    note_.clear();
    noteIsError_ = false;
    failedAt_.reset();

    std::vector<ui::ListItem> items(static_cast<std::size_t>(kRowCount));
    for (int i = 0; i < kRowCount; ++i) items[static_cast<std::size_t>(i)].label = rowLabel(app.data(), i);
    rows_.reset();
    rows_.setPageSize(kRowCount);
    rows_.setItems(std::move(items));
    keysOpen_ = false;
}

bool SettingsScene::update(Application& app, double) {
    engine::Engine& e = app.engine();
    using Key = engine::Engine::Key;
    // 能走到这里，说明面板又是最上面那一层了：改键面板若开过，此刻已经收了。
    keysOpen_ = false;
    // 截图口 settings:keys 要「直接翻开改键面板」：放在第一次 update 里压，不在 onEnter 里压——
    // onEnter 是 Application 正在遍历待压场景表的时候调的（与主菜单「直接翻开记事」同一个理由）。
    if (openKeyConfig_) {
        openKeyConfig_ = false;
        for (int guard = 0; guard < kRowCount && rows_.selection() != kKeys; ++guard) rows_.moveDown();
        activate(app);
        return true;
    }

    // 取消优先：同一帧 Esc 与别的键一起下来，先关（与 ListView 同一个口径）。
    if (e.keyPressed(Key::Cancel) || e.keyPressed(Key::Menu)) return !close(app);

    const int before = rows_.selection();
    if (e.keyPressed(Key::Up)) rows_.moveUp();
    if (e.keyPressed(Key::Down)) rows_.moveDown();
    if (rows_.selection() != before) {
        e.playSfx(kSfxCursor);
        // 「已恢复默认」这种一次性的话，光标一动就撤；写盘失败的原因留着，直到又改了什么或关掉面板。
        if (!noteIsError_) note_.clear();
    }

    const int dir = (e.keyPressed(Key::Right) ? 1 : 0) - (e.keyPressed(Key::Left) ? 1 : 0);
    if (dir != 0) adjust(app, dir);
    if (e.keyPressed(Key::Confirm)) activate(app);
    return true;
}

void SettingsScene::adjust(Application& app, int dir) {
    const int row = rows_.selection();
    const io::Settings next = adjusted(app.settings(), row, dir);
    if (next == app.settings()) return;   // 到头了，或这一行不吃左右键
    app.setSettings(next);
    note_.clear();
    noteIsError_ = false;
    // 调过值：写盘失败的说明已经撤了，下次关面板一律重新试着写——哪怕调走又调回、恰好回到失败那一刻的值，
    // 也要再报一次，不能屏上什么都没有就关了。
    failedAt_.reset();
    // 调音效音量时响的这一声就是在新增益下试听；音乐音量直接听正在放的曲子。
    app.engine().playSfx(row == kSfxVolume ? kSfxConfirm : kSfxCursor);
}

void SettingsScene::activate(Application& app) {
    switch (rows_.selection()) {
        case kRestore:
            // 全部（含键位）回到默认：默认值就是改造前的行为。
            app.setSettings(io::Settings{});
            note_ = app.text("ui.settings.restored");
            noteIsError_ = false;
            failedAt_.reset();   // 同上：说明已换成「已恢复默认」，下次关面板重新试着写
            app.engine().playSfx(kSfxConfirm);
            break;
        case kKeys:
            // 压改键面板（docs/settings.md 5.3）。这边照「记事」的做法让开：改键面板开着时本面板一笔不画。
            // 写盘失败的原因留着（回来还看得见），一次性的话（「已恢复默认」）撤掉。
            app.engine().playSfx(kSfxConfirm);
            app.pushScene(std::make_unique<KeyConfigScene>());
            keysOpen_ = true;
            if (!noteIsError_) note_.clear();
            break;
        default:
            break;   // 值那几行用左右键调
    }
}

bool SettingsScene::close(Application& app) {
    engine::Engine& e = app.engine();
    // 有改动才写；上一次关面板时写失败的那一份原样没动，这一次就直接关（「再按一次才关」）。
    // 失败之后又改过什么——值那几行、恢复默认、改键面板里的键位都算——就重新试着写。
    const io::Settings& now = app.settings();
    if (now != opened_ && (!failedAt_ || now != *failedAt_)) {
        auto saved = app.saveSettings();
        if (!saved) {
            // 不静默：留在面板上，说明行先写「再按一次…」，再写短原因（说明行只有两行，放不下的是原因的尾巴，
            // 不是该怎么办）。带整条路径的完整原因 Application::saveSettings 已经打到标准错误。
            failedAt_ = now;
            note_ = app.text("ui.settings.save_failed.again") + app.text("ui.settings.save_failed") + saved.error;
            noteIsError_ = true;
            e.playSfx(kSfxError);
            return false;
        }
    }
    e.playSfx(kSfxClose);
    return true;
}

void SettingsScene::showSaveFailure(const Application& app, const std::string& reason) {
    note_ = app.text("ui.settings.save_failed") + reason;
    noteIsError_ = true;
}

// ---------------------------------------------------------------------------
// 画
// ---------------------------------------------------------------------------

void SettingsScene::render(Application& app) {
    // 改键面板压在上面：它自己压纱、画面板，两层面板的字叠在一起谁也读不清。
    // 问「顶上是不是我」而不只看 keysOpen_：改键面板收起的那一帧，本面板的 update 还没跑、keysOpen_ 还没清，
    // 只看旗子的话那一帧两块面板都不画，底下的画面会亮一下。
    if (keysOpen_ && app.topScene() != this) return;
    engine::Engine& e = app.engine();
    const ui::Theme& theme = app.theme();
    const int rowH = ui::listRowHeight(theme);

    // 先压一层纱：底下的标题画面、主菜单的模糊底图就退成背景（契约 5.2）。
    ui::drawScrim(e, theme);
    const std::string title = app.text("ui.settings.title");
    ui::drawPanel(e, kPanel, title, theme);
    const engine::Rect content = ui::panelContentArea(kPanel, title, theme);

    int y = content.y + kTopGap;
    for (const Group& group : kGroups) {
        e.drawText(app.text(group.label), content.x + theme.padding, y + (theme.bodyFontSize - kGroupFontSize),
                   kGroupFontSize, theme.gold, theme.bodyStyle);
        y += rowH;
        for (int row = group.first; row < group.first + group.count; ++row) {
            drawRow(app, row, y);
            y += rowH;
        }
    }
    // 「恢复默认」不是一个设置项：一道分隔线把它与上面隔开。
    ui::drawRuleH(e, static_cast<float>(content.x + theme.padding), static_cast<float>(y + kRuleGap / 2),
                  static_cast<float>(content.w - theme.padding * 2), theme);
    y += kRuleGap;
    drawRow(app, kRestore, y);
    y += rowH + kRuleGap;

    drawNote(app, y);

    const std::string hint = app.text("ui.settings.hint");
    const engine::Point size = e.measureText(hint, kHintFontSize);
    e.drawText(hint, kPanel.x + kPanel.w - theme.padding - size.x, kPanel.y + kPanel.h - theme.padding - size.y + 4,
               kHintFontSize, theme.paperDim, theme.bodyStyle);
}

void SettingsScene::drawRow(Application& app, int row, int y) const {
    engine::Engine& e = app.engine();
    const ui::Theme& theme = app.theme();
    const ui::ListItem& item = rows_.items()[static_cast<std::size_t>(row)];
    const bool selected = row == rows_.selection();
    if (selected) {
        ui::drawSelection(e,
                          engine::Rect{kPanel.x + theme.padding / 2, y - theme.lineSpacing / 2,
                                       kPanel.w - theme.padding, ui::listRowHeight(theme)},
                          theme);
    }
    const engine::Color color = !item.enabled ? theme.paperDim : selected ? theme.goldBright : theme.paper;
    e.drawText(item.label, kPanel.x + theme.padding + kIndent, y, theme.bodyFontSize, color, theme.bodyStyle);

    const io::Settings& settings = app.settings();
    switch (row) {
        case kBgmVolume:
            drawVolume(app, settings.bgmVolume, y, selected);
            break;
        case kSfxVolume:
            drawVolume(app, settings.sfxVolume, y, selected);
            break;
        case kKeys: {
            // 这一行不调值，确认后进改键面板：右栏一个「›」。
            const std::string right = "›";
            const int w = e.measureText(right, theme.bodyFontSize).x;
            e.drawText(right, kPanel.x + kPanel.w - theme.padding - w, y, theme.bodyFontSize, theme.paperDim,
                       theme.bodyStyle);
            break;
        }
        case kRestore:
            break;
        default:
            // 到头的那一边括号变暗：问的就是左右键真正会做的那件事（adjusted），不另写一份判断。
            drawChoice(app, valueText(app.data(), settings, row), adjusted(settings, row, -1) != settings,
                       adjusted(settings, row, +1) != settings, y, selected);
            break;
    }
}

void SettingsScene::drawVolume(Application& app, int level, int y, bool selected) const {
    engine::Engine& e = app.engine();
    const ui::Theme& theme = app.theme();
    const int left = kPanel.x + kValueLeft;
    const int top = y + (theme.bodyFontSize - kCellSize) / 2 + 1;
    for (int i = 0; i < io::kVolumeLevels; ++i) {
        const engine::RectF cell{static_cast<float>(left + i * kCellStep), static_cast<float>(top),
                                 static_cast<float>(kCellSize), static_cast<float>(kCellSize)};
        if (i < level) {
            e.fillRect(cell, selected ? theme.goldBright : theme.gold, engine::BlendMode::Alpha);
        } else {
            e.fillRect(cell, theme.inkDeep, engine::BlendMode::Alpha);
            ui::drawFrame(e, cell, theme.goldDim);
        }
    }
    const std::string number = std::to_string(level);
    const int w = e.measureText(number, theme.bodyFontSize).x;
    e.drawText(number, kPanel.x + kPanel.w - theme.padding - w, y, theme.bodyFontSize,
               selected ? theme.goldBright : theme.paper, theme.bodyStyle);
}

void SettingsScene::drawChoice(Application& app, const std::string& value, bool canLeft, bool canRight, int y,
                               bool selected) const {
    engine::Engine& e = app.engine();
    const ui::Theme& theme = app.theme();
    const int center = kPanel.x + kValueLeft + (kPanel.w - theme.padding - kValueLeft) / 2;
    const int w = e.measureText(value, theme.bodyFontSize).x;
    e.drawText(value, center - w / 2, y, theme.bodyFontSize, selected ? theme.goldBright : theme.paper,
               theme.bodyStyle);
    // 到头的那一边括号淡下去：按过去也不会再动。
    const std::string leftMark = "〈";
    const std::string rightMark = "〉";
    const int rightW = e.measureText(rightMark, theme.bodyFontSize).x;
    e.drawText(leftMark, center - kChoiceHalfWidth, y, theme.bodyFontSize, canLeft ? theme.gold : theme.goldDim,
               theme.bodyStyle);
    e.drawText(rightMark, center + kChoiceHalfWidth - rightW, y, theme.bodyFontSize,
               canRight ? theme.gold : theme.goldDim, theme.bodyStyle);
}

void SettingsScene::drawNote(Application& app, int top) const {
    engine::Engine& e = app.engine();
    const ui::Theme& theme = app.theme();
    ui::drawRuleH(e, static_cast<float>(kPanel.x + theme.padding), static_cast<float>(top),
                  static_cast<float>(kPanel.w - theme.padding * 2), theme);
    // 说明行：临时的话（恢复默认、写盘失败）优先，否则写光标那一行的说明。
    const int row = rows_.selection();
    const std::string text = !note_.empty() ? note_
                             : row >= 0     ? app.text(kRowText[static_cast<std::size_t>(row)].desc)
                                            : std::string{};
    // drawTextBlock 四周各让一个 padding，所以区域往上提一截，字落在分隔线下 10 像素处。
    const int textTop = top + 10;
    const engine::Rect area{kPanel.x, textTop - theme.padding, kPanel.w,
                            2 * (kNoteFontSize + theme.lineSpacing) + theme.padding * 2};
    ui::Theme tone = theme;
    if (!note_.empty()) tone.paper = noteIsError_ ? theme.cinnabar : theme.jade;
    ui::drawTextBlock(e, text, area, kNoteFontSize, tone);
}

}  // namespace fanren::game
