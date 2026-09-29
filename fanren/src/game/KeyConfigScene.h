#pragma once
// 改键面板（施工契约 docs/settings.md 5.3、第 6 节）：从系统设置面板的「按键设置」进。
//
//   十行动作（第 6 节那张表的次序）+ 最后一行「恢复默认键位」；四列：固定（灰，不可改）/ 键位一 / 键位二 /
//   手柄（灰，只读：手柄键位固定，docs/gamepad.md 第 7 节）。
//   ↑↓ 选动作，←→ 选格；确认 → 说明行变成「按下新键（Esc 作罢，Backspace 清空）」，引擎进抓键；
//   抓到后按第 6 节的规则落格（Engine::assignKey / clearKey），结果（改好 / 互换 / 拒绝的原因）写在说明行。
//   立刻生效（Application::setSettings）；写盘随设置面板关闭时一起（那边比的是打开时的整份设置，含键位）。
//
// 抓键期间引擎不出任何逻辑键：Esc 不是「关面板」而是抓进来的一个键（作罢）；手柄 B 也是作罢（引擎交出 Esc 的扫描码），
// 别的手柄键不理。面板中途被关掉时 onExit 把引擎放回常态。
#include <map>
#include <string>
#include <vector>

#include "core/model/Types.h"
#include "engine/Engine.h"
#include "game/Scene.h"
#include "io/SettingsFile.h"

namespace fanren::game {

// io::Settings::keys（动作 id → 两格）↔ 引擎的整张键位表。
//   往引擎推：默认表打底，设置里列出的动作盖上去（「缺省某个动作 = 那个动作用默认」）；认不出的 id 不理。
//   往回存：只存与默认不同的动作——于是「全部默认」就是空表，与 io::Settings{} 相等，
//   开关一下改键面板、改了又改回去，都不会凭空多出一份文件。
[[nodiscard]] engine::Engine::KeyTable keyTableOf(const io::Settings& settings);
[[nodiscard]] std::map<std::string, io::KeySlots> keySettingsOf(const engine::Engine::KeyTable& table);

class KeyConfigScene : public Scene {
public:
    static constexpr int kActionRows = static_cast<int>(engine::Engine::Key::Count);
    static constexpr int kRestoreRow = kActionRows;
    static constexpr int kRowCount = kActionRows + 1;

    // 面板上可能出现的全部固有字（标题、表头、动作名、恢复默认、说明行的各种话、按键提示），
    // tests/UiStyleTests.cpp 拿禁词表扫它。
    [[nodiscard]] static std::vector<std::string> keyConfigStrings(const core::GameData& data);
    // 动作在面板上的名字（ui.keys.<id>：向上、确认、路径行动……）。
    [[nodiscard]] static std::string actionName(const core::GameData& data, engine::Engine::Key key);
    // 「手柄」那一列写的字：那个动作的手柄键显示名，按显示优先的次序、多个用「 / 」隔开（「Y / ≡」）。
    [[nodiscard]] static std::string padKeysText(engine::Engine::Key key);
    // 一次改键（或清空）的结果说成一句话：说明行写的就是它。
    [[nodiscard]] static std::string resultText(const core::GameData& data,
                                                const engine::Engine::KeyChangeResult& result);

    void onEnter(Application& app) override;
    void onExit(Application& app) override;
    bool update(Application& app, double deltaSeconds) override;
    void render(Application& app) override;

    [[nodiscard]] bool opaque() const override { return false; }
    [[nodiscard]] std::string name() const override { return "KeyConfig"; }

    // 光标（第几行、第几格）、是不是在等按键、说明行此刻写的话（空 = 写光标那一行的说明）。测试用。
    [[nodiscard]] int row() const { return row_; }
    [[nodiscard]] int slot() const { return slot_; }
    [[nodiscard]] bool capturing() const { return capturing_; }
    [[nodiscard]] const std::string& note() const { return note_; }

private:
    void startCapture(Application& app);
    void finishCapture(Application& app, engine::ScanCode code);
    void restoreDefaults(Application& app);
    void setNote(std::string text, bool error);

    void drawActionRow(Application& app, int row, int y) const;
    void drawRestoreRow(Application& app, int y) const;
    void drawNote(Application& app, int top) const;

    int row_ = 0;
    int slot_ = 0;
    bool capturing_ = false;
    std::string note_;
    bool noteIsError_ = false;
};

}  // namespace fanren::game
