// 界面控件层的单测。
//
// 界面难测，所以这里刻意不测像素，只测逻辑与「不崩」：
//   - ListView 的选择、翻页、禁用项、空表边界全部是纯逻辑，不需要引擎；
//   - 需要真字体的部分（断行、右栏右对齐）借无头 engine 的 measureText 跑；
//   - 绘制函数只断言「任何退化输入都不崩、不越界、不改状态」——这正是
//     P1 对话框手写面板时最容易出事的地方。
#include <climits>
#include <string>
#include <utility>
#include <vector>

#include <SDL3/SDL.h>
#include <gtest/gtest.h>

#include "engine/Engine.h"
#include "ui/Widgets.h"

namespace {

using fanren::engine::Color;
using fanren::engine::Engine;
using fanren::engine::Rect;
using fanren::ui::drawGauge;
using fanren::ui::drawPanel;
using fanren::ui::drawTextBlock;
using fanren::ui::gaugeFillWidth;
using fanren::ui::kNoSelection;
using fanren::ui::ListItem;
using fanren::ui::ListView;
using fanren::ui::panelContentArea;
using fanren::ui::Theme;

ListItem makeItem(std::string label, bool enabled = true) {
    ListItem item;
    item.label = std::move(label);
    item.enabled = enabled;
    return item;
}

std::vector<ListItem> makeItems(int count) {
    std::vector<ListItem> items;
    items.reserve(static_cast<std::size_t>(count > 0 ? count : 0));
    for (int i = 0; i < count; ++i) {
        items.push_back(makeItem("条目" + std::to_string(i)));
    }
    return items;
}

ListView listWith(int count, int pageSize) {
    ListView view;
    view.setPageSize(pageSize);
    view.setItems(makeItems(count));
    return view;
}

// 无头引擎：不开窗口、不建渲染器，但字体真的加载，measureText 给真实尺寸。
// 每个用例自带一台，与 EngineTests 的做法一致。
class HeadlessUi : public ::testing::Test {
protected:
    void SetUp() override {
        const auto result = engine.init("fanren-ui-tests", true);
        ASSERT_TRUE(result.ok) << result.error;
        engine.pollEvents();   // 把可能残留的事件先排干，免得污染按键用例
    }
    void TearDown() override { engine.shutdown(); }

    Engine engine;
    Theme theme;
};

// 无头模式收不到真实按键，直接往 SDL 事件队列里塞（同 EngineTests）。
void pushKey(SDL_Scancode scancode, bool down) {
    SDL_Event event{};
    event.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
    event.key.scancode = scancode;
    event.key.down = down;
    event.key.repeat = false;
    SDL_PushEvent(&event);
}

}  // namespace

// ---------------------------------------------------------------------------
// 选择移动
// ---------------------------------------------------------------------------

TEST(ListViewSelection, StartsOnTheFirstEnabledItem) {
    ListView view;
    view.setItems({makeItem("不可选", false), makeItem("可选"), makeItem("也可选")});
    EXPECT_EQ(view.selection(), 1);   // 开局就停在禁用项上等于开局就是死的
    EXPECT_TRUE(view.hasSelectable());
}

TEST(ListViewSelection, MovesDownThenUp) {
    ListView view = listWith(5, 5);
    ASSERT_EQ(view.selection(), 0);
    view.moveDown();
    EXPECT_EQ(view.selection(), 1);
    view.moveDown();
    EXPECT_EQ(view.selection(), 2);
    view.moveUp();
    EXPECT_EQ(view.selection(), 1);
}

// 越界行为的裁定：上下移动**回绕**。菜单普遍很短，回绕能省掉一串按键，
// 也与 P1 DialogueScene 的选项列表（取模回绕）保持一致。
TEST(ListViewSelection, WrapsAroundAtBothEnds) {
    ListView view = listWith(3, 3);
    ASSERT_EQ(view.selection(), 0);
    view.moveUp();
    EXPECT_EQ(view.selection(), 2) << "顶端向上必须回绕到末项";
    view.moveDown();
    EXPECT_EQ(view.selection(), 0) << "末项向下必须回绕到首项";
}

TEST(ListViewSelection, SkipsDisabledItems) {
    ListView view;
    view.setItems({makeItem("0"), makeItem("1", false), makeItem("2", false), makeItem("3"),
                   makeItem("4", false)});
    ASSERT_EQ(view.selection(), 0);
    view.moveDown();
    EXPECT_EQ(view.selection(), 3) << "1、2 被禁用，应当整段跳过";
    view.moveDown();
    EXPECT_EQ(view.selection(), 0) << "末尾的 4 也禁用，继续绕回首项";
    view.moveUp();
    EXPECT_EQ(view.selection(), 3);
}

TEST(ListViewSelection, StaysOnTheOnlyEnabledItem) {
    ListView view;
    view.setItems({makeItem("0", false), makeItem("1", false), makeItem("2"),
                   makeItem("3", false)});
    ASSERT_EQ(view.selection(), 2);
    view.moveDown();
    EXPECT_EQ(view.selection(), 2);
    view.moveUp();
    EXPECT_EQ(view.selection(), 2);
}

// 全禁用的裁定：selection 退到 kNoSelection，与空表同一个值——
// 调用方只要判一次 < 0，不必分别处理「没有条目」和「条目全不可选」。
TEST(ListViewSelection, ReportsNoSelectionWhenEverythingIsDisabled) {
    ListView view;
    view.setItems({makeItem("0", false), makeItem("1", false), makeItem("2", false)});
    EXPECT_EQ(view.selection(), kNoSelection);
    EXPECT_FALSE(view.hasSelectable());
    EXPECT_EQ(view.selectedItem(), nullptr);
    EXPECT_EQ(view.count(), 3) << "条目仍要显示，只是不可选";
}

// 全禁用时每个移动动作都必须在有限步内退出。内部的查找带「最多走 n 步」的
// 硬上界，这条用例就是盯着那个上界的——退化成死循环会直接卡死整个游戏。
TEST(ListViewSelection, EveryMoveTerminatesWhenEverythingIsDisabled) {
    std::vector<ListItem> items;
    for (int i = 0; i < 40; ++i) items.push_back(makeItem("锁住的" + std::to_string(i), false));
    ListView view;
    view.setPageSize(6);
    view.setItems(items);

    for (int i = 0; i < 50; ++i) {
        view.moveDown();
        view.moveUp();
        view.pageDown();
        view.pageUp();
    }
    EXPECT_EQ(view.selection(), kNoSelection);
    EXPECT_EQ(view.topRow(), 0);
    EXPECT_FALSE(view.confirm()) << "没有可选项就不该确认得出结果";
}

// ---------------------------------------------------------------------------
// 空列表
// ---------------------------------------------------------------------------

TEST(ListViewEmpty, SurvivesEveryOperationAndReportsNoSelection) {
    ListView view;
    EXPECT_EQ(view.selection(), kNoSelection);
    EXPECT_EQ(view.count(), 0);
    EXPECT_EQ(view.selectedItem(), nullptr);

    for (int i = 0; i < 10; ++i) {
        view.moveDown();
        view.moveUp();
        view.pageDown();
        view.pageUp();
    }
    EXPECT_EQ(view.selection(), kNoSelection);
    EXPECT_EQ(view.topRow(), 0);
}

TEST(ListViewEmpty, ConfirmDoesNothingButCancelStillWorks) {
    ListView view;
    view.setItems({});
    EXPECT_FALSE(view.confirm());
    EXPECT_FALSE(view.confirmed());
    // 空背包也必须退得出去，否则玩家被关在一个空面板里。
    view.cancel();
    EXPECT_TRUE(view.cancelled());
}

// ---------------------------------------------------------------------------
// 翻页
// ---------------------------------------------------------------------------

TEST(ListViewPaging, MovesByWholePagesAndClampsAtTheEnds) {
    ListView view = listWith(20, 5);
    ASSERT_EQ(view.selection(), 0);
    view.pageDown();
    EXPECT_EQ(view.selection(), 5);
    view.pageDown();
    EXPECT_EQ(view.selection(), 10);
    view.pageDown();
    EXPECT_EQ(view.selection(), 15);
    // 翻页**夹紧不回绕**：翻页是「扫长表」的动作，跳到另一头只会让人迷失位置。
    view.pageDown();
    EXPECT_EQ(view.selection(), 19);
    view.pageDown();
    EXPECT_EQ(view.selection(), 19);

    view.pageUp();
    EXPECT_EQ(view.selection(), 14);
    for (int i = 0; i < 10; ++i) view.pageUp();
    EXPECT_EQ(view.selection(), 0);
}

TEST(ListViewPaging, DoesNothingWhenEverythingFitsOnOnePage) {
    ListView view = listWith(3, 8);   // 页大小 > 项数：根本没有第二页
    ASSERT_EQ(view.selection(), 0);
    view.pageDown();
    EXPECT_EQ(view.selection(), 0);
    EXPECT_EQ(view.topRow(), 0);
    view.moveDown();
    ASSERT_EQ(view.selection(), 1);
    view.pageUp();
    EXPECT_EQ(view.selection(), 1) << "一页装得下时翻页键应当毫无动静";
    EXPECT_EQ(view.topRow(), 0);
}

TEST(ListViewPaging, ScrollWindowFollowsTheSelection) {
    ListView view = listWith(20, 5);
    for (int i = 0; i < 4; ++i) view.moveDown();
    ASSERT_EQ(view.selection(), 4);
    EXPECT_EQ(view.topRow(), 0) << "还在窗口内就不该滚";

    view.moveDown();
    EXPECT_EQ(view.selection(), 5);
    EXPECT_EQ(view.topRow(), 1) << "跑出窗口才滚，且只滚一行";

    view.moveUp();
    EXPECT_EQ(view.topRow(), 1) << "回到窗口内不该把窗口再拽回去";

    view.pageDown();
    EXPECT_EQ(view.selection(), 9);
    EXPECT_EQ(view.topRow(), 5);

    // 首项向上回绕到末项，窗口必须跟到表尾。
    ListView top = listWith(20, 5);
    top.moveUp();
    EXPECT_EQ(top.selection(), 19);
    EXPECT_EQ(top.topRow(), 15);
}

TEST(ListViewPaging, NonPositivePageSizeIsClampedToOneRow) {
    ListView view;
    view.setItems(makeItems(6));
    view.setPageSize(0);
    EXPECT_EQ(view.pageSize(), 1);
    view.setPageSize(-4);
    EXPECT_EQ(view.pageSize(), 1);

    view.pageDown();
    EXPECT_EQ(view.selection(), 1) << "一行一页时翻页等同于逐行";
    EXPECT_EQ(view.topRow(), 1);
}

TEST(ListViewPaging, HandlesTheExactOnePageBoundary) {
    ListView exact = listWith(5, 5);   // 项数正好等于页大小
    exact.pageDown();
    EXPECT_EQ(exact.selection(), 0) << "正好装满一页就没有下一页";
    for (int i = 0; i < 4; ++i) exact.moveDown();
    EXPECT_EQ(exact.selection(), 4);
    EXPECT_EQ(exact.topRow(), 0) << "正好装满一页时永远不该滚动";

    ListView overflow = listWith(6, 5);   // 只多出一项，滚动就该出现
    overflow.pageDown();
    EXPECT_EQ(overflow.selection(), 5);
    EXPECT_EQ(overflow.topRow(), 1);
}

// 页大小是调用方随手传的数，极端值不能让 base ± pageSize 的整数加法溢出。
TEST(ListViewPaging, HandlesAnAbsurdlyLargePageSize) {
    ListView view = listWith(20, 5);
    view.setPageSize(INT_MAX);
    EXPECT_EQ(view.pageSize(), INT_MAX);
    view.pageDown();
    EXPECT_EQ(view.selection(), 0);
    view.pageUp();
    EXPECT_EQ(view.selection(), 0);
    EXPECT_EQ(view.topRow(), 0);

    view.setPageSize(19);   // 页大小正好比项数少一：翻一页就到底
    view.pageDown();
    EXPECT_EQ(view.selection(), 19);
}

TEST(ListViewPaging, SkipsDisabledItemsWhenLandingAfterAPage) {
    std::vector<ListItem> items = makeItems(20);
    for (int i = 5; i <= 7; ++i) items[static_cast<std::size_t>(i)].enabled = false;
    ListView view;
    view.setPageSize(5);
    view.setItems(items);

    ASSERT_EQ(view.selection(), 0);
    view.pageDown();
    EXPECT_EQ(view.selection(), 8) << "落点 5 被禁用，应顺势往下找到 8";
}

// ---------------------------------------------------------------------------
// 确认 / 取消 / reset
// ---------------------------------------------------------------------------

TEST(ListViewResult, ConfirmReportsTheSelectedIndex) {
    ListView view = listWith(4, 4);
    view.moveDown();
    view.moveDown();
    EXPECT_TRUE(view.confirm());
    EXPECT_TRUE(view.confirmed());
    EXPECT_EQ(view.selection(), 2);
    EXPECT_FALSE(view.cancelled()) << "确认不该顺手把取消也置上";
}

TEST(ListViewResult, CancelIsStickyAndNotAConfirmation) {
    ListView view = listWith(4, 4);
    view.confirm();
    view.cancel();
    EXPECT_TRUE(view.cancelled());
    EXPECT_FALSE(view.confirmed()) << "取消必须把确认标志打掉，否则上层会两头都当真";
    // 粘滞：game 层可以在本帧任意位置查，不必与控件的帧节奏对齐。
    view.moveDown();
    EXPECT_TRUE(view.cancelled());
}

// cancel() 会打掉 confirmed_，confirm() 也必须打掉粘滞的 cancelled_。
// 少了任一半，商店面板就可能同时「关窗口」和「买下选中的法器」。
TEST(ListViewResult, ConfirmAndCancelAreMutuallyExclusive) {
    ListView view = listWith(4, 4);
    view.cancel();
    ASSERT_TRUE(view.cancelled());

    EXPECT_TRUE(view.confirm());
    EXPECT_TRUE(view.confirmed());
    EXPECT_FALSE(view.cancelled()) << "确认必须把粘滞的取消打掉";

    view.cancel();
    EXPECT_TRUE(view.cancelled());
    EXPECT_FALSE(view.confirmed());
}

TEST(ListViewResult, ConfirmFailsWithoutASelectableItem) {
    ListView empty;
    EXPECT_FALSE(empty.confirm());

    ListView locked;
    locked.setItems({makeItem("a", false), makeItem("b", false)});
    EXPECT_FALSE(locked.confirm());
    EXPECT_FALSE(locked.confirmed());
}

TEST(ListViewResult, ResetClearsInteractionStateButKeepsItems) {
    ListView view = listWith(20, 4);
    view.pageDown();
    view.pageDown();
    view.confirm();
    view.cancel();
    ASSERT_NE(view.selection(), 0);
    ASSERT_TRUE(view.cancelled());

    view.reset();
    EXPECT_EQ(view.selection(), 0);
    EXPECT_EQ(view.topRow(), 0);
    EXPECT_FALSE(view.cancelled());
    EXPECT_FALSE(view.confirmed());
    // reset 只清交互状态，不动条目：换数据是 setItems 的事，两件事分开。
    EXPECT_EQ(view.count(), 20);
}

// ---------------------------------------------------------------------------
// setItems 的夹取
// ---------------------------------------------------------------------------

TEST(ListViewItems, SetItemsClampsAStaleSelectionIntoRange) {
    ListView view = listWith(20, 5);
    view.pageDown();
    view.pageDown();
    ASSERT_EQ(view.selection(), 10);

    view.setItems(makeItems(3));   // 背包里刚卖掉一大半
    EXPECT_EQ(view.selection(), 2) << "越界的旧下标要夹到末项，而不是让上层拿去索引";
    EXPECT_EQ(view.topRow(), 0);
    EXPECT_EQ(view.count(), 3);
}

TEST(ListViewItems, SetItemsSnapsOntoAnEnabledItem) {
    ListView view = listWith(20, 5);
    view.pageDown();
    view.pageDown();
    ASSERT_EQ(view.selection(), 10);

    view.setItems({makeItem("0"), makeItem("1"), makeItem("2", false)});
    EXPECT_EQ(view.selection(), 1) << "夹到的末项不可选时要吸附到最近的可选项";
}

TEST(ListViewItems, SetItemsKeepsAPendingCancel) {
    // 背包面板每帧都会 setItems 重建列表；在这里清标志会让取消键被自己吃掉。
    ListView view = listWith(5, 5);
    view.cancel();
    view.setItems(makeItems(5));
    EXPECT_TRUE(view.cancelled());
    view.reset();
    EXPECT_FALSE(view.cancelled());
}

TEST(ListViewItems, SelectedItemPointerMatchesSelection) {
    ListView view;
    view.setItems({makeItem("灵石"), makeItem("清灵散")});
    view.moveDown();
    const ListItem* selected = view.selectedItem();
    ASSERT_NE(selected, nullptr);
    EXPECT_EQ(selected->label, "清灵散");
    EXPECT_EQ(selected, &view.items()[static_cast<std::size_t>(view.selection())]);
}

// ---------------------------------------------------------------------------
// 数值条：纯计算部分不需要引擎，边界一律夹死在这里
// ---------------------------------------------------------------------------

TEST(UiGauge, FillWidthIsBoundedForEveryInput) {
    constexpr int kWidth = 200;
    const struct { int current, maximum; } kCases[] = {
        {0, 100}, {50, 100}, {100, 100},
        {150, 100},                  // 超上限：增益药、护盾溢出
        {-5, 100}, {INT_MIN, 100},   // 负血：掉到 0 以下的那一帧
        {5, 0}, {0, 0}, {5, -3},     // 上限为 0 或为负：不能除零
        {INT_MAX, INT_MAX}, {INT_MAX / 2, INT_MAX}, {INT_MAX, 1},
    };
    for (const auto& c : kCases) {
        const int w = gaugeFillWidth(kWidth, c.current, c.maximum);
        EXPECT_GE(w, 0) << c.current << "/" << c.maximum;
        EXPECT_LE(w, kWidth) << c.current << "/" << c.maximum;
    }
    EXPECT_EQ(gaugeFillWidth(kWidth, 150, 100), kWidth) << "超上限按满条";
    EXPECT_EQ(gaugeFillWidth(kWidth, -5, 100), 0) << "负值按空条";
    EXPECT_EQ(gaugeFillWidth(kWidth, 5, 0), 0) << "上限为 0 只能是空条";
    // 退化的条宽本身也要兜住。
    EXPECT_EQ(gaugeFillWidth(0, 5, 10), 0);
    EXPECT_EQ(gaugeFillWidth(-10, 5, 10), 0);
}

TEST(UiGauge, FillWidthIsMonotonicAndHitsBothEnds) {
    constexpr int kWidth = 256;
    int previous = -1;
    for (int current = 0; current <= 100; ++current) {
        const int w = gaugeFillWidth(kWidth, current, 100);
        EXPECT_GE(w, previous) << "血量增加时长度不许倒退，current=" << current;
        previous = w;
    }
    EXPECT_EQ(gaugeFillWidth(kWidth, 0, 100), 0);
    EXPECT_EQ(gaugeFillWidth(kWidth, 100, 100), kWidth);
}

// ---------------------------------------------------------------------------
// 无头引擎下的绘制与输入
// ---------------------------------------------------------------------------

TEST_F(HeadlessUi, ListViewUpdateIsANoOpWithoutInput) {
    ListView view = listWith(6, 3);
    for (int i = 0; i < 5; ++i) {
        engine.pollEvents();
        EXPECT_FALSE(view.update(engine)) << "没有输入就是没有输入";
    }
    EXPECT_EQ(view.selection(), 0);
    EXPECT_FALSE(view.cancelled());
    EXPECT_FALSE(view.confirmed());
}

TEST_F(HeadlessUi, ListViewUpdateMapsKeysToActions) {
    ListView view = listWith(6, 3);

    const auto tap = [this](SDL_Scancode code) {
        pushKey(code, true);
        engine.pollEvents();
    };
    const auto release = [this](SDL_Scancode code) {
        pushKey(code, false);
        engine.pollEvents();
    };

    tap(SDL_SCANCODE_DOWN);
    EXPECT_FALSE(view.update(engine));
    EXPECT_EQ(view.selection(), 1);
    release(SDL_SCANCODE_DOWN);

    tap(SDL_SCANCODE_UP);
    EXPECT_FALSE(view.update(engine));
    EXPECT_EQ(view.selection(), 0);
    release(SDL_SCANCODE_UP);

    // 左右键当翻页：Key 枚举里没有 PageUp/PageDown，长列表总得有办法快速挪。
    tap(SDL_SCANCODE_RIGHT);
    EXPECT_FALSE(view.update(engine));
    EXPECT_EQ(view.selection(), 3);
    release(SDL_SCANCODE_RIGHT);

    tap(SDL_SCANCODE_LEFT);
    EXPECT_FALSE(view.update(engine));
    EXPECT_EQ(view.selection(), 0);
    release(SDL_SCANCODE_LEFT);

    tap(SDL_SCANCODE_RETURN);
    EXPECT_TRUE(view.update(engine)) << "确认键必须让 update 返回真";
    EXPECT_TRUE(view.confirmed());
    EXPECT_EQ(view.selection(), 0);
    release(SDL_SCANCODE_RETURN);

    tap(SDL_SCANCODE_ESCAPE);
    EXPECT_FALSE(view.update(engine));
    EXPECT_TRUE(view.cancelled());
    EXPECT_FALSE(view.confirmed()) << "confirmed 只描述本帧，取消帧必须已经清掉";
    release(SDL_SCANCODE_ESCAPE);
}

TEST_F(HeadlessUi, ListViewRenderSurvivesEveryShape) {
    const Rect normal{100, 100, 400, 300};

    ListView emptyView;   // 空表要画出「空空如也」，不是一片空白
    emptyView.render(engine, normal, theme);

    ListView locked;
    locked.setItems({makeItem("辟谷丹", false), makeItem("黄枫丹", false)});
    locked.render(engine, normal, theme);

    ListView view = listWith(40, 8);
    view.pageDown();
    view.pageDown();
    const int selectionBefore = view.selection();
    const int topBefore = view.topRow();

    view.render(engine, normal, theme);
    view.render(engine, Rect{0, 0, 0, 0}, theme);         // 零面积
    view.render(engine, Rect{0, 0, -50, -50}, theme);     // 负面积
    view.render(engine, Rect{0, 0, 10, 10}, theme);       // 比内边距还小
    view.render(engine, Rect{0, 0, 200, 60}, theme);      // 比 pageSize 矮得多
    view.render(engine, Rect{1200, 700, 400, 300}, theme);  // 整块在屏幕外

    // render 是 const，绝不许把选择或滚动位置改掉。
    EXPECT_EQ(view.selection(), selectionBefore);
    EXPECT_EQ(view.topRow(), topBefore);

    // 超长标签 + 禁用理由：右栏的起点要被挤回左边界而不是跑到面板左边去。
    // 无头 engine 的 drawText 是真空操作，拿不到实际落笔位置，所以这里
    // 只能断言「不崩」；精确的落墨范围要等到有渲染录制器才测得了。
    ListItem wide = makeItem(std::string(500, 'X'), false);
    wide.detail = "9999";
    wide.disabledReason = "材料不足：三叶紫芝 ×2";
    ListView wideView;
    wideView.setItems({wide});
    wideView.render(engine, Rect{0, 0, 120, 200}, theme);
    SUCCEED();
}

TEST_F(HeadlessUi, ListViewEmptyHintIsConfigurable) {
    ListView view;
    view.setEmptyHint("囊中空空");
    view.render(engine, Rect{0, 0, 300, 200}, theme);
    EXPECT_EQ(view.selection(), kNoSelection);
    SUCCEED();
}

TEST_F(HeadlessUi, DrawTextBlockSurvivesDegenerateText) {
    const Rect area{40, 40, 400, 200};

    drawTextBlock(engine, "", area, 22, theme);                      // 空串
    drawTextBlock(engine, "。", area, 22, theme);                    // 单个禁则标点
    drawTextBlock(engine, "。，、；：？！）》」』】％", area, 22, theme);  // 纯标点：整行都是行首禁则
    drawTextBlock(engine, "（《「『【", area, 22, theme);             // 纯行尾禁则

    // 超长无空格串：贪心断行最容易在这里退化成死循环或一字一行。
    drawTextBlock(engine, std::string(4000, 'W'), area, 22, theme);
    std::string longChinese;
    for (int i = 0; i < 600; ++i) longChinese += "韩立";
    drawTextBlock(engine, longChinese, area, 22, theme);

    // 非法 UTF-8（半个汉字）也不能让面板崩掉：将来 mod 文案是玩家可控的。
    std::string truncated;
    truncated.push_back(static_cast<char>(0xE9));
    truncated.push_back(static_cast<char>(0x9F));
    drawTextBlock(engine, truncated, area, 22, theme);

    // 字号与区域的退化取值。
    drawTextBlock(engine, "韩立", area, 0, theme);
    drawTextBlock(engine, "韩立", area, -8, theme);
    drawTextBlock(engine, "韩立", Rect{0, 0, 0, 0}, 22, theme);
    drawTextBlock(engine, "韩立", Rect{0, 0, theme.padding * 2, 200}, 22, theme);  // 内宽为 0
    drawTextBlock(engine, "韩立", Rect{0, 0, 400, theme.padding * 2}, 22, theme);  // 内高为 0
    SUCCEED();
}

TEST_F(HeadlessUi, DrawPanelAndContentAreaSurviveDegenerateRects) {
    drawPanel(engine, Rect{60, 60, 500, 300}, "储物袋", theme);
    drawPanel(engine, Rect{60, 60, 500, 300}, "", theme);      // 无标题
    drawPanel(engine, Rect{0, 0, 0, 0}, "储物袋", theme);
    drawPanel(engine, Rect{0, 0, -10, -10}, "储物袋", theme);
    drawPanel(engine, Rect{0, 0, 10, 10}, "储物袋", theme);     // 标题比面板还高

    const Rect full{60, 60, 500, 300};
    const Rect titled = panelContentArea(full, "储物袋", theme);
    EXPECT_GT(titled.y, full.y) << "有标题时正文区必须往下让";
    EXPECT_LT(titled.h, full.h);

    const Rect untitled = panelContentArea(full, "", theme);
    EXPECT_EQ(untitled.y, full.y) << "没有标题就没有让位的理由";
    EXPECT_EQ(untitled.h, full.h);

    // 面板比标题还矮时正文区会算成负高度，必须压到 0——
    // 调用方拿到的矩形要能安全地丢给任何绘制函数。
    const Rect tiny = panelContentArea(Rect{0, 0, 10, 4}, "储物袋", theme);
    EXPECT_GE(tiny.w, 0);
    EXPECT_GE(tiny.h, 0);
    drawTextBlock(engine, "韩立", tiny, 22, theme);
    SUCCEED();
}

// Theme 的存在意义就是「将来换皮只改一处」，所以必须有一条用例证明控件
// 在非默认配色/间距下也不会把东西画到区域外。全部用例都只用默认 Theme，
// 等于这条承诺从没被验证过。
TEST_F(HeadlessUi, WidgetsSurviveANonDefaultTheme) {
    Theme tight;
    tight.padding = 0;
    tight.lineSpacing = 40;     // 行距远大于内边距：光标条会往区域上沿顶
    tight.bodyFontSize = 6;
    tight.titleFontSize = 90;   // 标题比面板还高
    tight.cursorAlpha = 255;

    ListView view = listWith(30, 6);
    view.pageDown();
    view.render(engine, Rect{0, 0, 300, 120}, tight);
    view.render(engine, Rect{0, 0, 300, 20}, tight);

    drawPanel(engine, Rect{0, 0, 300, 40}, "储物袋", tight);
    drawTextBlock(engine, "韩立提着药锄，沿着山路慢慢往上走。", Rect{0, 0, 300, 120}, 6, tight);
    drawGauge(engine, Rect{0, 0, 300, 10}, 30, 100, Color{80, 160, 200, 255}, tight);

    const Rect content = panelContentArea(Rect{0, 0, 300, 40}, "储物袋", tight);
    EXPECT_GE(content.w, 0);
    EXPECT_GE(content.h, 0) << "标题比面板高时正文区高度必须压到 0，不能是负数";
    SUCCEED();
}

TEST_F(HeadlessUi, DrawGaugeSurvivesExtremeValues) {
    const Color blood{198, 64, 64, 255};
    const Rect bar{40, 400, 260, 18};

    drawGauge(engine, bar, 70, 100, blood, theme);
    drawGauge(engine, bar, 999, 100, blood, theme);     // 超上限
    drawGauge(engine, bar, -50, 100, blood, theme);     // 负血
    drawGauge(engine, bar, 5, 0, blood, theme);         // 上限为 0
    drawGauge(engine, bar, 0, 0, blood, theme);
    drawGauge(engine, bar, INT_MAX, 1, blood, theme);
    drawGauge(engine, bar, INT_MIN, INT_MAX, blood, theme);
    drawGauge(engine, Rect{0, 0, 0, 0}, 50, 100, blood, theme);
    drawGauge(engine, Rect{0, 0, -20, -5}, 50, 100, blood, theme);
    SUCCEED();
}
