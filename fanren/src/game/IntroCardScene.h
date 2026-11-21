#pragma once
// 登场卡（女角色立绘，docs/heroine-art-build.md 3.5.3）：脚本 intro_card(role) 叫的那一张。
//
// 画面：整屏压暗（六成黑，地图隐约可见）；她的立绘（normal 那一格，整个人）0.25 秒从右侧滑入并淡入，停在中间偏右；
// 左侧是名字（大字，金色）、一行身份、一行短评。出现 kArmSeconds 之后才收确认 / 取消键，按下 kFadeOutSeconds 淡出；
// 自动播放开着时 kAutoSeconds 后自己收。收起时回填脚本命令（Application::completeCommand）。
//
// 压不压、有没有立绘、跳过时略过、无头立即完成，都是 Application 的事（Application::pushIntroCard）；
// 这里只摆画面、收键、走时间线。不记存档、不进回看。
#include <cstdint>
#include <string>

#include "engine/Engine.h"
#include "game/Scene.h"

namespace fanren::game {

class IntroCardScene : public Scene {
public:
    // bust：立绘那一格（整格，normal）；name / title / line：已取好的文字。
    IntroCardScene(std::string role, engine::TextureId texture, engine::RectF cell, std::string name,
                   std::string title, std::string line);

    void onEnter(Application& app) override;
    bool update(Application& app, double deltaSeconds) override;
    void render(Application& app) override;

    [[nodiscard]] bool opaque() const override { return false; }
    [[nodiscard]] std::string name() const override { return "IntroCard"; }

    [[nodiscard]] const std::string& role() const { return role_; }
    [[nodiscard]] double age() const { return age_; }
    [[nodiscard]] bool closing() const { return closing_; }

    static constexpr double kSlideSeconds = 0.25;
    static constexpr double kArmSeconds = 0.6;
    static constexpr double kFadeOutSeconds = 0.2;
    static constexpr double kAutoSeconds = 2.5;
    static constexpr int kBustH = 660;          // 立绘画出来的高
    static constexpr int kBustCenterX = 860;    // 停下时立绘中线的 x（中间偏右）
    static constexpr int kSlideFrom = 140;      // 从右边多远滑进来
    static constexpr int kTextX = 150;          // 左侧文字的左边
    static constexpr std::uint8_t kDim = 153;   // 压暗：六成黑

    // 时间线（纯函数，测试用）：此刻整张卡的透明度 0..1、立绘离停点还差几像素。
    [[nodiscard]] static float opacityAt(double age, bool closing, double closingAge);
    [[nodiscard]] static float slideOffsetAt(double age);
    // 立绘停下时画在哪（整格按 bustRosterSource 裁出「整个人」那一块，放到高 kBustH）。
    [[nodiscard]] static engine::RectF bustRect(const engine::RectF& src);

private:
    void close(Application& app);

    std::string role_;
    engine::TextureId texture_;
    engine::RectF cell_;
    std::string name_;
    std::string title_;
    std::string line_;
    double age_ = 0.0;
    bool closing_ = false;
    double closingAge_ = 0.0;
    bool done_ = false;
};

}  // namespace fanren::game
