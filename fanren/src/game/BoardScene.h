#pragma once
// 告示板：当前该做什么，以及走到这里都做过什么。
//
// 地图上那块 kind=board 的设施此前按下去只回一句「此处的功用尚未开启」——
// 而它正是「记事」的天然去处：世界层那一行只写得下一句话，这里写得下来龙去脉。
//
// 分层照 FieldScene 那条：**哪一步算做完了**在 core/rules/Objectives.h，
// 这里只负责把它排成几行字。面板不碰 GameState。
//
// 第 5 章起（契约 docs/interfaces-p3-ch05.md 1.7）列表里**先列支线**：进行中的写当前
// 步骤，过期的写「已过期」与原因（Q7：不许静默消失），了结的写「已了结」；**再列做过的
// 主线步骤**，与从前逐行相同。支线状态在 core/rules/Quests.h。上半截「眼下」不变，
// 仍只说主线——HUD 那一行同样只说主线（Q5）。
#include <string>
#include <vector>

#include "core/model/Types.h"
#include "game/Scene.h"
#include "ui/Widgets.h"

namespace fanren::game {

class BoardScene : public Scene {
public:
    void onEnter(Application& app) override;
    bool update(Application& app, double deltaSeconds) override;
    void render(Application& app) override;

    [[nodiscard]] bool opaque() const override { return false; }
    [[nodiscard]] std::string name() const override { return "Board"; }

    // 面板列表画得满几行。与 FieldScene::listPageRows 同一个理由：设页大小与
    // 摆列表区必须走同一份高度，否则会出现「区域画得下十几行，却只显示 8 行」，
    // 而多出来的那几行玩家得按方向键才找得到——他多半不知道下面还有。
    [[nodiscard]] static int listPageRows(const ui::Theme& theme);

    // 下半截列表的全部行：支线在前（rules::sideQuestJournal 的顺序），做过的主线步骤在后
    // （倒序：刚做完的在最上面）。静态、纯：测试直接读它，不必开窗口、不必开面板。
    //
    // 支线一行的形状（契约 1.7）：
    //   进行中 —— 「任务名：当前步骤（地名）」/「任务名：summary」，右栏「进行中」
    //   已过期 —— 「任务名：过期原因」，右栏「已过期」
    //   已了结 —— 「任务名」，右栏「已了结」
    [[nodiscard]] static std::vector<ui::ListItem> buildRows(const core::GameData& data,
                                                             const core::GameState& state);

    // 右栏那三个字。测试与面板共用，免得两边各写一份字面量。
    static constexpr const char* kActiveTag = "进行中";
    static constexpr const char* kFailedTag = "已过期";
    static constexpr const char* kCompletedTag = "已了结";

private:
    ui::ListView done_;
    // 当前这一步的两行：做什么、在哪儿。取不到目标链时留空，面板照常开得出来。
    std::string current_;
    std::string where_;
};

}  // namespace fanren::game
