#pragma once
// 记事：当前该做什么、走到这里都做过什么、手上的支线、看过的教学卡、记下的见闻。
//
// 地图上那块 kind=board 的设施此前按下去只回一句「此处的功用尚未开启」——
// 而它正是「记事」的天然去处：世界层那一行只写得下一句话，这里写得下来龙去脉。
//
// 分层照 FieldScene 那条：**哪一步算做完了**在 core/rules/Objectives.h，
// 这里只负责把它排成几行字。面板不碰 GameState（指南页重看一张卡也只压场景，不改旗标）。
//
// 第一期地基包（docs/ch07-foundation.md 2.4）起分四页，左右键翻页：
//   主线 —— 上半截「眼下」：行动句（带进度）、要点、详情长句、地点；下半截做过的步骤，倒序。
//   支线 —— 进行中、已过期、已了结（rules::sideQuestJournal 的次序）；选中的那条右边写摘要、
//            当前步骤、奖励（Quest::rewards，收掉技术债 G-25）。
//   指南 —— 看过的教学卡，确认键重看。
//   见闻 —— 解锁了的见闻条目；妖兽条目补「已知破绽」与「掉落」。
// 第 5 章以来的 buildRows（支线在前、做过的主线在后的那一整张表）照旧保留：它是测试读状态的口子，
// 面板现在把它拆成支线、主线两页来画，每一行的字与从前逐字相同。
#include <map>
#include <string>
#include <vector>

#include "core/model/Types.h"
#include "game/Scene.h"
#include "ui/Widgets.h"

namespace fanren::game {

class Application;

class BoardScene : public Scene {
public:
    enum class Page { Main = 0, Side = 1, Guide = 2, Lore = 3 };
    static constexpr int kPageCount = 4;

    BoardScene() = default;
    explicit BoardScene(Page page) : page_(page) {}
    // 截图口用（guide:<页>#<n>，第一期地基包集成）：一打开光标就停在第 n 行，拍那一条的右栏正文与翻页。
    BoardScene(Page page, int startRow) : page_(page), startRow_(startRow) {}

    void onEnter(Application& app) override;
    bool update(Application& app, double deltaSeconds) override;
    void render(Application& app) override;

    [[nodiscard]] bool opaque() const override { return false; }
    [[nodiscard]] std::string name() const override { return "Board"; }

    // 面板列表画得满几行（主线页的下半截）。与 FieldScene::listPageRows 同一个理由：设页大小与
    // 摆列表区必须走同一份高度，否则会出现「区域画得下十几行，却只显示 8 行」，
    // 而多出来的那几行玩家得按方向键才找得到——他多半不知道下面还有。
    [[nodiscard]] static int listPageRows(const ui::Theme& theme);

    // 支线在前（rules::sideQuestJournal 的顺序）、做过的主线步骤在后（倒序）的整张表。静态、纯：
    // 测试直接读它，不必开窗口、不必开面板。面板的支线页与主线页各画其中一段，行字相同。
    //
    // 支线一行的形状（契约 1.7）：
    //   进行中 —— 「任务名：当前步骤（地名）」/「任务名：summary」，右栏「进行中」
    //   已过期 —— 「任务名：过期原因」，右栏「已过期」
    //   已了结 —— 「任务名」，右栏「已了结」
    [[nodiscard]] static std::vector<ui::ListItem> buildRows(const core::GameData& data,
                                                             const core::GameState& state);
    // 只要支线那一段 / 只要做过的主线那一段（buildRows 的前半、后半）。
    [[nodiscard]] static std::vector<ui::ListItem> sideRows(const core::GameData& data, const core::GameState& state);
    [[nodiscard]] static std::vector<ui::ListItem> mainRows(const core::GameData& data, const core::GameState& state);

    // 一条支线的奖励：「奖励　灵石 × 60、筑基丹 × 1」（G-25）。钱按 Wording 叫碎银 / 灵石；没写奖励为空串。
    [[nodiscard]] static std::string rewardLine(const core::GameData& data, const core::GameState& state,
                                                const core::Quest& quest);

    // 见闻页一条妖兽条目补的那两行（2.5）：「已知破绽　火、土」（一样都没记下时写「尚未看破」）、
    // 「掉落　寒蟾酥、妖兽材」（没有掉落时不写）。不是妖兽条目、或没写 role_id，返回空表。
    [[nodiscard]] static std::vector<std::string> loreBeastLines(const core::GameData& data,
                                                                 const core::GameState& state,
                                                                 const std::map<std::string, core::BattleSetup>& battles,
                                                                 const core::LoreEntry& entry);

    // 右栏那三个字。测试与面板共用，免得两边各写一份字面量。
    static constexpr const char* kActiveTag = "进行中";
    static constexpr const char* kFailedTag = "已过期";
    static constexpr const char* kCompletedTag = "已了结";

    [[nodiscard]] Page page() const { return page_; }
    // 翻页（左右键）。回绕：四页，从「见闻」往右回到「主线」。
    void turnPage(Application& app, int step);
    // 这一页列表里的行（测试与截图读它）。
    [[nodiscard]] const ui::ListView& list() const { return list_; }
    // 这一页右栏 / 上半截此刻写着的那几行字（不含列表）。测试读它断言奖励、破绽、掉落写出来了。
    [[nodiscard]] std::vector<std::string> paneLines(Application& app) const;

private:
    void rebuild(Application& app);
    void renderMain(Application& app, const engine::Rect& content);
    void renderSplit(Application& app, const engine::Rect& content);

    Page page_ = Page::Main;
    int startRow_ = 0;
    ui::ListView list_;
    // 主线页上半截：眼下这一步的几行（行动句、要点、详情、地点），按面板宽折好了。
    struct Line {
        std::string text;
        int font = 0;
        engine::Color color{};
    };
    std::vector<Line> current_;
    int currentBandH_ = 0;
    // 支线 / 指南 / 见闻页：列表每一行背后的对象（与 list_ 的条目一一对应）。
    std::vector<const core::Quest*> quests_;
    std::vector<const core::TutorialCard*> cards_;
    std::vector<const core::LoreEntry*> lore_;
    // 主线页：做过的每一步（倒序，与列表对应），下栏写它的详情。
    std::vector<const core::Objective*> steps_;
};

}  // namespace fanren::game
