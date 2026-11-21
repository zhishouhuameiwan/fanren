#pragma once
// 教学卡（第一期地基包，docs/ch07-foundation.md 2.3）：一块小面板——标题、至多三行、按键提示，确认键关掉。
//
// 是一层场景：压在战斗上时，战斗只更新最上面那一层，于是「战斗输入暂停」不必另写。
// 什么时候压、压哪一张、记不记看过，都是 Application 的事（tutorialEvent / requestTutorial / flushTutorials），
// 这里只负责摆字和收键。记事「指南」页重看也压它，同一个样子。
#include <string>
#include <vector>

#include "engine/Engine.h"
#include "game/Scene.h"

namespace fanren::game {

class TutorialScene : public Scene {
public:
    explicit TutorialScene(std::string cardId) : cardId_(std::move(cardId)) {}

    void onEnter(Application& app) override;
    bool update(Application& app, double deltaSeconds) override;
    void render(Application& app) override;

    [[nodiscard]] bool opaque() const override { return false; }
    [[nodiscard]] std::string name() const override { return "Tutorial"; }

    [[nodiscard]] const std::string& cardId() const { return cardId_; }
    [[nodiscard]] const std::string& title() const { return title_; }
    [[nodiscard]] const std::vector<std::string>& lines() const { return lines_; }

    // 面板摆在哪：居中，高按行数定。公开给截图与测试（不出屏、不压到对白框的位置）。
    [[nodiscard]] static engine::Rect panelRect(int lineCount);

    // 刚弹出来这么久之内不收确认：玩家正连按确认翻对白、打怪时，第一下不该把卡片顺手关掉。
    // 无头不设（测试一帧一帧地按）。
    static constexpr double kArmSeconds = 0.35;

private:
    std::string cardId_;
    std::string title_;
    std::vector<std::string> lines_;
    std::string hint_;
    double age_ = 0.0;
};

}  // namespace fanren::game
