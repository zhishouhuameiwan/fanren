#pragma once
// 系统设置面板（施工契约 docs/settings.md 5.2）。标题画面的「设置」与主菜单的「设置」共用这一个。
//
//   声音　音乐音量 / 音效音量（0–10，一格一档）
//   画面　显示模式 / 画面缩放 / 垂直同步 / 画面特效 / 战斗震屏
//   游玩　文字速度 / 手柄震动 / 按键设置（确认 → 压改键面板 KeyConfigScene，这边让开、一笔不画）
//   ────　恢复默认
//   下面一行写光标那一行的说明，最下面一行写按键。
//
// 行的下标即契约第 1 节的 # 次序（kBgmVolume … kRestore），测试按下标驱动。「手柄震动」是 docs/gamepad.md 第 8 节
// 插在「文字速度」之后的一行，它后面的两行各往后挪一个下标。
// ←→ 调值**立刻生效**（Application::setSettings）。Esc / X / 菜单键关面板：有改动（含改键面板里改的键位）才写盘；
// 写失败时留在面板上，把原因写在说明行，再按一次才关（这一局照样按改过的跑）。
//
// 已知取舍（契约 5.2）：只有栈顶场景会 update，从标题画面进来时标题背景的漂移会停住；
// 压着的那层墨色让它读起来像「退成背景」，不另做。
#include <optional>
#include <string>
#include <vector>

#include "core/model/Types.h"
#include "game/Scene.h"
#include "io/SettingsFile.h"
#include "ui/Widgets.h"

namespace fanren::game {

class SettingsScene : public Scene {
public:
    static constexpr int kBgmVolume = 0;
    static constexpr int kSfxVolume = 1;
    static constexpr int kFullscreen = 2;
    static constexpr int kScale = 3;
    static constexpr int kVSync = 4;
    static constexpr int kEffects = 5;
    static constexpr int kScreenShake = 6;
    static constexpr int kTextSpeed = 7;
    static constexpr int kPadRumble = 8;
    static constexpr int kKeys = 9;
    static constexpr int kRestore = 10;
    static constexpr int kRowCount = 11;

    // 面板上可能出现的全部固有字：标题、分组、行名、各档的叫法、说明、提示、成败的话。
    // 与 render 取的是同一批 key，tests/UiStyleTests.cpp 拿禁词表扫它。
    [[nodiscard]] static std::vector<std::string> settingsStrings(const core::GameData& data);

    // 第 row 行的名字（「音乐音量」…）与右边写的值（音量是数字，其余是「窗口」「整数倍」「开」这类叫法；
    // 按键设置、恢复默认两行没有值，返回空串）。主菜单「设置」那一页的摘要也用这两个。
    [[nodiscard]] static std::string rowLabel(const core::GameData& data, int row);
    [[nodiscard]] static std::string valueText(const core::GameData& data, const io::Settings& settings,
                                               int row);

    // 第 row 行往 dir（-1 = ←，+1 = →）调一格之后的设置。纯函数，测试直接喂。一律有方向、到头就停：
    //   音量、文字速度一格一档，不回绕（从「瞬显」再按 → 不该跳回「慢」）；
    //   两档的（显示模式、缩放、垂直同步、特效、震屏、手柄震动）← 取契约第 1 节表里写在前面的那一档，→ 取后面那一档——
    //   不做「按一下翻一下」：左右键带自动重复，按住会让全屏与窗口每秒来回切十几次（有光敏风险）；
    //   按键设置、恢复默认不吃左右键，原样返回。
    [[nodiscard]] static io::Settings adjusted(const io::Settings& settings, int row, int dir);

    // openKeyConfig：一打开就直接翻开改键面板（截图口 settings:keys 用；游戏里从「按键设置」那一行进）。
    explicit SettingsScene(bool openKeyConfig = false) : openKeyConfig_(openKeyConfig) {}

    void onEnter(Application& app) override;
    bool update(Application& app, double deltaSeconds) override;
    void render(Application& app) override;

    [[nodiscard]] bool opaque() const override { return false; }
    [[nodiscard]] std::string name() const override { return "Settings"; }

    // 光标在第几行；说明行此刻写的话（空 = 写光标那一行的说明）。测试断言「恢复默认之后说了什么」
    // 「写盘失败之后说了什么」用。
    [[nodiscard]] int selection() const { return rows_.selection(); }
    [[nodiscard]] const std::string& note() const { return note_; }

    // 面板开着时别处写盘失败了（Alt+Enter 切全屏后那一次写盘）：把短原因写在说明行上，不静默。
    // 关面板时有改动照样再试着写。
    void showSaveFailure(const Application& app, const std::string& reason);

private:
    void adjust(Application& app, int dir);
    void activate(Application& app);
    // 关面板。返回 false = 写盘失败，留在面板上（原因已写进说明行）。
    bool close(Application& app);

    void drawRow(Application& app, int row, int y) const;
    void drawVolume(Application& app, int level, int y, bool selected) const;
    void drawChoice(Application& app, const std::string& value, bool canLeft, bool canRight, int y,
                    bool selected) const;
    void drawNote(Application& app, int top) const;

    ui::ListView rows_;       // 只借它的光标（上下回绕、跳过不可选的那一行），画法自己来
    io::Settings opened_;     // 打开时的设置：关面板时与它比，有改动才写盘
    std::string note_;        // 说明行的临时话（恢复默认 / 写盘失败）；空 = 写光标那一行的说明
    bool noteIsError_ = false;
    // 关面板时写盘失败的那一刻的设置。此后原样不动再按关就直接关；改过什么就重新试着写：
    // 在本面板调值、恢复默认时直接清掉它（说明也撤了，调走再调回也要再报一次）；改键面板那条路靠比值。
    std::optional<io::Settings> failedAt_;
    bool keysOpen_ = false;   // 改键面板正压在上面（它一收、本面板又是顶层时 update 里清掉）
    bool openKeyConfig_ = false;
};

}  // namespace fanren::game
