#pragma once
// 脚本的 fade.out(ms) / fade.in(ms) / wait(ms) 三条命令（此前一直是空操作）。
//
// 黑幕本身**不归这个场景画**：fade.out 之后画面要一直黑到 fade.in，而这中间脚本照样会
// 说话、会换图——场景一退场黑幕就没了。所以浓度记在 Application（screenFade），由
// Application::drawScenes 画在世界层之上、对话框之下（黑场里的旁白读得见）。
// 本场景只管「这几百毫秒里把浓度从 a 推到 b，推完回填命令」。
//
// 无头模式下根本不压它：Application::dispatch 当场把浓度设到终值并回填，与改造前
// 「直接当作已完成」一样快，测试与无头 bot 一帧也不多等。
#include <string>

#include "game/Scene.h"

namespace fanren::game {

class FadeScene : public Scene {
public:
    enum class Kind { Out, In, Wait };

    FadeScene(Kind kind, int milliseconds);

    // t 秒时黑幕的浓度（0 透明、1 全黑）。from 是开始时的浓度：fade.out 从当前浓度
    // 推到 1，fade.in 推到 0，wait 原样不动。时长 ≤ 0 直接取终值。纯函数，测试钉着。
    [[nodiscard]] static float levelAt(Kind kind, float from, double t, double duration);

    void onEnter(Application& app) override;
    bool update(Application& app, double deltaSeconds) override;
    void render(Application&) override {}

    // 黑幕压在世界层上，底下的世界照画。
    [[nodiscard]] bool opaque() const override { return false; }
    [[nodiscard]] std::string name() const override { return "Fade"; }

private:
    Kind kind_;
    double duration_;
    double elapsed_ = 0.0;
    float from_ = 0.f;
};

}  // namespace fanren::game
