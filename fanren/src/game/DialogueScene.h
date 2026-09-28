#pragma once
// 对话与选择。脚本的 Talk / Choice 命令都落到这里。
//
// 面板、选项列表、断行都交给 ui 层：P1 时这里手写了一份，结果是同一套东西
// 在对话框和将来的商店、背包里各走一样。控件只回报选择，状态变更在 Application。
#include <optional>
#include <string>
#include <vector>

#include "engine/Engine.h"
#include "engine/TextLayout.h"
#include "game/Scene.h"
#include "io/SettingsFile.h"
#include "ui/Widgets.h"

namespace fanren::game {

class DialogueScene : public Scene {
public:
    // speaker 为空表示旁白（不画名签）。
    DialogueScene(std::string speaker, std::string body);

    // 追加选项后本场景变为选择模式：正文显示完毕才出现选项。
    void setChoices(std::vector<std::string> options);

    // 正文不逐字滚，一上来就全显。选项框里留着的「刚说完的那一句」用它：那句刚才已经
    // 一个字一个字读过了（Application::dispatch 的 Choice）。
    void revealAtOnce() { revealAtOnce_ = true; }

    // 名签的矩形：压在对话框上沿、靠左，一半在框外。宽度随名字长短（textWidth 是名字的
    // 像素宽）。公开是为了让测试断言「选项再多、框长到最高，名签也还在屏幕里」。
    [[nodiscard]] static engine::Rect nameTagArea(const engine::Rect& box, int textWidth);

    // 选项块要占的高度。boxArea 与 render 都走它——对话框有多高、选项区摆在
    // 哪里，这两处必须同一个数，否则对话框会按一个高度撑开、选项却按另一个
    // 高度去画，差出来的那点高度直接表现为「有选项没画出来」。
    // 公开是为了让测试不开窗口就能断言「n 个选项在这块高度里一个都不少」。
    [[nodiscard]] static int optionBlockHeight(int rows, const ui::Theme& theme);

    // 选项区最多给到几行。对话框是「底边钉死、往上长」的，选项再多也不许把
    // 框顶推出屏幕；超出这个数的选项照样选得到，改为在列表里滚动，右下角那个
    // 「第几/共几」的刻度会告诉玩家下面还有。
    [[nodiscard]] static int maxOptionRows(const ui::Theme& theme);

    // 对话框的位置与大小，只由选项行数与皮肤决定的纯函数。
    // 公开是为了让测试能直接断言「选项数取任何值，框都还在屏幕里」。
    [[nodiscard]] static engine::Rect boxAreaFor(int rows, const ui::Theme& theme);

    // 回看面板的矩形与它画得满几行。两者必须同源：设页大小与摆列表区各算一份
    // 高度，就会出现「区域画得下十几行、却只显示 8 行」——而回看面板里被藏起来
    // 的那几行，正是玩家按 Tab 时要找的那几行。
    [[nodiscard]] static engine::Rect logPanelArea();
    [[nodiscard]] static int logPageRows(const ui::Theme& theme);

    // 文字速度 → 每秒几个字（docs/settings.md 第 1 节：慢 30 / 标准 60 / 快 120）。
    // 瞬显不是一个速率，单独判：返回 nullopt，意思是「一上来就全显」。
    [[nodiscard]] static std::optional<double> graphemesPerSecond(io::TextSpeed speed);
    // 逐字显示此刻用的速率：update 与 onEnter 调的就是它（读 app.settings().textSpeed）。
    // 无头下对话框直接全显，看不出速率，所以测试问的是这一个函数，不另写一份。
    [[nodiscard]] static std::optional<double> revealRate(const Application& app);

    void onEnter(Application& app) override;
    bool update(Application& app, double deltaSeconds) override;
    void render(Application& app) override;

    [[nodiscard]] bool opaque() const override { return false; }
    [[nodiscard]] std::string name() const override { return "Dialogue"; }

private:
    void revealAll();
    [[nodiscard]] bool fullyRevealed() const;
    [[nodiscard]] std::string revealedText() const;
    void finish(Application& app, int choiceIndex);
    [[nodiscard]] engine::Rect boxArea(const ui::Theme& theme) const;
    // 回看面板：把 Application 的对话记录装进列表，最近的一行在最上面。
    void openLog(Application& app);
    void renderLog(Application& app) const;
    // 框上沿的两块小牌：左边说话人的名签，右边那行按键提示。
    void renderNameTag(Application& app, const engine::Rect& box) const;
    void renderKeyHint(Application& app, const engine::Rect& box) const;
    // 正文读完、等着按确认时右下角那个缓慢起伏的「▼」。
    void renderContinueMark(Application& app, const engine::Rect& box) const;

    std::string speaker_;
    std::string body_;
    bool hasChoices_ = false;
    bool revealAtOnce_ = false;
    // 场景自己累加的秒数，给「▼」的起伏当相位。不用 Engine::elapsedSeconds：
    // 截图口只 tick 不 beginFrame，拿引擎时钟拍出来的画面每次不一样。
    double time_ = 0.0;

    std::vector<std::string> graphemes_;   // 正文按字符切好，供逐字显示
    std::size_t revealed_ = 0;
    double revealAccumulator_ = 0.0;
    ui::ListView choices_;
    bool done_ = false;

    // 长按快进已经按住了多久。每个对话框自己从零数起，于是快进的节奏是
    // 「每框停一下」而不是「一口气冲到底」——玩家仍然看得见每一句闪过。
    double holdSeconds_ = 0.0;
    // 回看面板开着的时候，对话本身不推进：否则玩家一边翻记录一边把没读的
    // 那几句按掉了。
    bool log_ = false;
    ui::ListView logView_;
};

}  // namespace fanren::game
