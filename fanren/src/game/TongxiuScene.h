#pragma once
// 蒲团上的「打坐 / 同修」（第 7 章同道录 2a·路 U，施工图 docs/ch07-tongdaolu.md 3.4；施工记录 docs/ch07-tongdaolu-u.md）。
//
// 只在百药园（kTongdaoMap）的蒲团、而且同道录里有人物时出现（Application::openFacility 判）：先两项「打坐」「同修」；
//   打坐 → 换成原来的修炼面板（CultivationScene，同一个灵脉效率）；
//   同修 → 列同道录里的人物（名字、档名；不能同修的置灰写理由），选一位 → Application::tongxiu（记账、过一天），
//          说一句「与某某同修一日。默契 +N，明天第一场战斗全队多 1 点劲」。
// 同道录里一个人物都没有时根本不压这一层，蒲团与改版前一模一样；别处的蒲团也不压。
#include <string>
#include <vector>

#include "engine/Engine.h"
#include "game/Scene.h"
#include "ui/Widgets.h"

namespace fanren::game {

class TongxiuScene : public Scene {
public:
    enum class Mode { Choose, Pick };
    // 版面：屏幕正中一块面板。
    static constexpr int kPanelW = 560;

    explicit TongxiuScene(int sitePercent, bool startPicking = false)
        : sitePercent_(sitePercent), startPicking_(startPicking) {}

    // 同道录里能同修的人物（在册、是人物），按伙伴 id 排。空 = 蒲团照旧直接打坐。
    [[nodiscard]] static std::vector<std::string> partners(Application& app);

    void onEnter(Application& app) override;
    bool update(Application& app, double deltaSeconds) override;
    void render(Application& app) override;
    [[nodiscard]] bool opaque() const override { return false; }
    [[nodiscard]] std::string name() const override { return "Tongxiu"; }

    [[nodiscard]] Mode mode() const { return mode_; }
    [[nodiscard]] const ui::ListView& list() const { return list_; }
    [[nodiscard]] engine::Rect panelRect(const ui::Theme& theme) const;

private:
    void enterChoose(Application& app);
    void enterPick(Application& app);

    int sitePercent_;
    bool startPicking_;
    bool tutorialAsked_ = false;
    Mode mode_ = Mode::Choose;
    ui::ListView list_;
    std::vector<std::string> ids_;   // Pick 时 list_ 第 i 行是谁
};

}  // namespace fanren::game
