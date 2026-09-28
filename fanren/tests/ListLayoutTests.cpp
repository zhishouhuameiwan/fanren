// 列表行数与区域高度的对账。
//
// 为什么单独一个文件：UiTests 测的是控件的交互逻辑（选择、翻页、禁用项），
// 这里测的是「摆位算式与绘制算式对不对得上」——两者算不到一块去的时候，
// 控件本身每一条单测都是绿的，玩家却看不见半数选项。P2 的对话框正是这么
// 漏的：框按 行数×行高 撑开，ListView 画之前又扣掉上下各一个 padding，
// 于是三个选项只画得出一个，游戏里 32 处 choice 全部中招。
//
// 这些断言全是纯计算，不开窗口、不要字体，因此可以按面板的真实几何去断言
// 「N 行一个都不会被藏起来」——无头 engine 的 drawText 是空操作，不这么测
// 就只能靠肉眼在游戏里逐个对话去发现。
#include <gtest/gtest.h>

#include <climits>
#include <string>
#include <vector>

#include "engine/Engine.h"
#include "game/AlchemyScene.h"
#include "game/BattleScene.h"
#include "game/CultivationScene.h"
#include "game/DialogueScene.h"
#include "game/FieldScene.h"
#include "game/ShopScene.h"
#include "ui/Widgets.h"

namespace {

using fanren::engine::Rect;
using fanren::game::AlchemyScene;
using fanren::game::BattleScene;
using fanren::game::CultivationScene;
using fanren::game::DialogueScene;
using fanren::game::FieldScene;
using fanren::game::ShopScene;
using fanren::ui::kDefaultPageSize;
using fanren::ui::listAreaHeight;
using fanren::ui::listRowHeight;
using fanren::ui::listRowsThatFit;
using fanren::ui::ListItem;
using fanren::ui::ListView;
using fanren::ui::panelTitleBandHeight;
using fanren::ui::Theme;

std::vector<ListItem> makeItems(int count) {
    std::vector<ListItem> items;
    for (int i = 0; i < count; ++i) {
        ListItem item;
        item.label = "条目" + std::to_string(i);
        items.push_back(std::move(item));
    }
    return items;
}

// 区域宽度取一个绝对够用的值：行数只由高度决定，宽度在这里只是别退化。
constexpr int kAmpleWidth = 600;

// ---- 换算本身 ----

TEST(ListLayout, AreaHeightAndRowsAreInverses) {
    const Theme theme;
    // 往返一圈必须回到原处。这一条一旦破，摆位的人和绘制的人就各说各话了。
    for (int rows = 1; rows <= 40; ++rows) {
        const int height = listAreaHeight(rows, theme);
        EXPECT_EQ(listRowsThatFit(height, theme), rows)
            << rows << " 行要的高度 " << height << " 反过来算不出 " << rows << " 行";
    }
}

TEST(ListLayout, AreaHeightCountsTheListPadding) {
    const Theme theme;
    // 裸的 行数×行高 少掉的正是上下两个内边距——这就是对话框漏选项的那笔差额。
    EXPECT_EQ(listAreaHeight(3, theme), 3 * listRowHeight(theme) + theme.padding * 2);
    // 少一个像素就该少一行，不许四舍五入回来。
    EXPECT_LT(listRowsThatFit(listAreaHeight(3, theme) - 1, theme), 3);
}

TEST(ListLayout, DegenerateHeightsStayInRange) {
    const Theme theme;
    EXPECT_EQ(listAreaHeight(0, theme), 0) << "没有行就不该占高度";
    EXPECT_EQ(listAreaHeight(-5, theme), 0);
    EXPECT_EQ(listRowsThatFit(0, theme), 0);
    EXPECT_EQ(listRowsThatFit(-100, theme), 0);
    EXPECT_EQ(listRowsThatFit(theme.padding * 2, theme), 0) << "只放得下内边距就是一行都放不下";
    // 比一行还矮但装得下内边距：画一行被截的，总比一片空白强。
    EXPECT_EQ(listRowsThatFit(theme.padding * 2 + 1, theme), 1);

    // 行数直接来自条目数，而条目数是数据决定的：不许溢出成负高度。
    EXPECT_GT(listAreaHeight(INT_MAX, theme), 0);
    EXPECT_GT(listAreaHeight(1'000'000, theme), 0);

    Theme broken;   // 皮肤把字号行距配成 0 是配置错误，但不该表现为除零
    broken.bodyFontSize = 0;
    broken.lineSpacing = 0;
    EXPECT_GE(listRowHeight(broken), 1);
    EXPECT_GT(listRowsThatFit(1000, broken), 0);
}

TEST(ListLayout, TitleBandMatchesPanelContentArea) {
    const Theme theme;
    const Rect panel{0, 0, 400, 500};
    // 反推正文高度的人（按面板高度设页大小）与摆位的人必须拿到同一个数。
    EXPECT_EQ(fanren::ui::panelContentArea(panel, "标题", theme).h,
              panel.h - panelTitleBandHeight(true, theme));
    EXPECT_EQ(fanren::ui::panelContentArea(panel, "", theme).h,
              panel.h - panelTitleBandHeight(false, theme));
    EXPECT_EQ(panelTitleBandHeight(false, theme), 0);
}

// ---- ListView::visibleRows ----

TEST(ListLayout, VisibleRowsHonoursAreaPageSizeAndCount) {
    const Theme theme;
    ListView view;
    view.setPageSize(20);
    view.setItems(makeItems(20));

    // 区域正好给够：一行不少。
    EXPECT_EQ(view.visibleRows(Rect{0, 0, kAmpleWidth, listAreaHeight(20, theme)}, theme), 20);
    // 区域只给得起 5 行：画 5 行，剩下的靠滚动。
    EXPECT_EQ(view.visibleRows(Rect{0, 0, kAmpleWidth, listAreaHeight(5, theme)}, theme), 5);

    // 页大小是另一道闸：区域再高也不会超过它。
    view.setPageSize(6);
    EXPECT_EQ(view.visibleRows(Rect{0, 0, kAmpleWidth, listAreaHeight(20, theme)}, theme), 6);

    // 条目数是第三道闸。
    view.setPageSize(20);
    view.setItems(makeItems(3));
    EXPECT_EQ(view.visibleRows(Rect{0, 0, kAmpleWidth, listAreaHeight(20, theme)}, theme), 3);
}

TEST(ListLayout, VisibleRowsIsZeroOnUnusableAreas) {
    const Theme theme;
    ListView view;
    view.setPageSize(8);
    view.setItems(makeItems(8));

    EXPECT_EQ(view.visibleRows(Rect{0, 0, 0, 0}, theme), 0);
    EXPECT_EQ(view.visibleRows(Rect{0, 0, -50, -50}, theme), 0);
    EXPECT_EQ(view.visibleRows(Rect{0, 0, 10, 400}, theme), 0) << "宽度放不下内边距就画不了";
    EXPECT_EQ(view.visibleRows(Rect{0, 0, kAmpleWidth, 10}, theme), 0);

    ListView empty;
    EXPECT_EQ(empty.visibleRows(Rect{0, 0, kAmpleWidth, 400}, theme), 0) << "空表没有行可画";
}

// ---- 对话框选项：本次修复的正主 ----

TEST(ListLayout, DialogueShowsEveryChoiceAtOnce) {
    const Theme theme;
    // 脚本里 32 处 choice 全是 2 项或 3 项，这里一路断言到框装得下的上限为止。
    // 判据取「选项块的高度反算出来的行数 == 选项数」：少一行就是玩家要按方向键
    // 才看得到的那一行，而他多半根本不知道下面还有。
    for (int rows = 1; rows <= DialogueScene::maxOptionRows(theme); ++rows) {
        const int height = DialogueScene::optionBlockHeight(rows, theme);
        EXPECT_EQ(listRowsThatFit(height, theme), rows)
            << rows << " 个选项只画得出 " << listRowsThatFit(height, theme) << " 个";
    }
    EXPECT_EQ(DialogueScene::optionBlockHeight(0, theme), 0) << "没有选项就不该占高度";
}

TEST(ListLayout, DialogueChoiceListDrawsEveryRow) {
    const Theme theme;
    // 走真控件再验一遍：摆位算式对了，控件那边也得真的画得出来。
    for (int rows = 2; rows <= 3; ++rows) {
        ListView choices;
        choices.setPageSize(rows);
        choices.setItems(makeItems(rows));
        const Rect area{0, 0, fanren::engine::kLogicalWidth - 96,
                        DialogueScene::optionBlockHeight(rows, theme)};
        EXPECT_EQ(choices.visibleRows(area, theme), rows);
    }
}

TEST(ListLayout, DialogueBoxNeverLeavesTheScreen) {
    const Theme theme;
    // 对话框是「底边钉死、往上长」的，选项越多长得越高。没有上限的话，选项数
    // 一多框顶就会被推出屏幕上沿——框是不透明的，玩家看到的是一块从天而降、
    // 没有上边框的色板，以及被切掉的正文。
    for (const int rows : {0, 1, 3, 12, 13, 40, 1000, 100000}) {
        const Rect box = DialogueScene::boxAreaFor(rows, theme);
        EXPECT_GE(box.y, 0) << rows << " 个选项把框顶推出了屏幕";
        EXPECT_LE(box.y + box.h, fanren::engine::kLogicalHeight)
            << rows << " 个选项把框底推出了屏幕";
        EXPECT_GT(box.h, 0);
    }

    // 上限之内框还在长，上限之外就不长了——夹的是框高，不是选项数。
    const int cap = DialogueScene::maxOptionRows(theme);
    EXPECT_GT(DialogueScene::boxAreaFor(cap, theme).h, DialogueScene::boxAreaFor(1, theme).h);
    EXPECT_EQ(DialogueScene::boxAreaFor(cap + 1, theme).h,
              DialogueScene::boxAreaFor(cap, theme).h);
    EXPECT_EQ(DialogueScene::boxAreaFor(9999, theme).h, DialogueScene::boxAreaFor(cap, theme).h);
}

TEST(ListLayout, DialogueBeyondTheCapScrollsInsteadOfHiding) {
    const Theme theme;
    const int cap = DialogueScene::maxOptionRows(theme);
    ASSERT_GT(cap, 1);

    // 超出上限的选项一个都没少，只是要滚——而「要滚」的前提是翻页键真的能动。
    // 页大小若仍取选项数，ListView 会认为整表就一页，翻页键毫无动静，玩家只能
    // 一路按下键；这正是战斗菜单踩过的那个坑（见 BattleMenu 那条用例）。
    const int rows = cap + 5;
    ListView choices;
    choices.setPageSize(cap);   // DialogueScene::onEnter 设的就是这个数
    choices.setItems(makeItems(rows));

    const Rect area{0, 0, fanren::engine::kLogicalWidth - 96,
                    DialogueScene::optionBlockHeight(rows, theme)};
    EXPECT_EQ(choices.visibleRows(area, theme), cap) << "装得下多少就得画多少";

    const int before = choices.selection();
    choices.pageDown();
    EXPECT_NE(choices.selection(), before) << "装不下的选项表必须还能翻页";

    // 末项仍然够得着：从首项往上回绕一格就是它，滚动窗口跟着跳过去。
    choices.reset();
    ASSERT_EQ(choices.selection(), 0);
    choices.moveUp();
    EXPECT_EQ(choices.selection(), rows - 1) << "滚动列表的末项必须够得着";
}

// ---- 面板列表：页大小不许比面板画得下的行数还小 ----

TEST(ListLayout, PanelPageSizesFillTheirPanels) {
    const Theme theme;
    // 这几块面板都够高，画得下的行数远多于默认的 8；页大小若沿用默认值，
    // 玩家就得翻页才看得到后半张表——而表其实完整地摆得下。
    // 只断言「比默认值大」而不互相对齐：面板高度是各自的排版决定，
    // 日后谁改矮了谁自己的断言该动，不该连累另外三块。
    EXPECT_GT(AlchemyScene::listPageRows(theme), kDefaultPageSize);
    EXPECT_GT(ShopScene::listPageRows(theme), kDefaultPageSize);
    EXPECT_GT(FieldScene::listPageRows(theme), kDefaultPageSize);
    EXPECT_GT(CultivationScene::listPageRows(theme), kDefaultPageSize);

    // 炼丹面板今天要摆 8 张丹方 + 「离开」共 9 行，一行都不许藏。
    EXPECT_GE(AlchemyScene::listPageRows(theme), 9);

    ListView list;
    list.setPageSize(AlchemyScene::listPageRows(theme));
    list.setItems(makeItems(9));
    const Rect area{0, 0, kAmpleWidth, listAreaHeight(AlchemyScene::listPageRows(theme), theme)};
    EXPECT_EQ(list.visibleRows(area, theme), 9);
}

TEST(ListLayout, BattleMenuStaysPageableWhenTheListOutgrowsIt) {
    const Theme theme;
    const int rows = BattleScene::listPageRows(theme);
    EXPECT_GT(rows, 0);

    // 战斗菜单比其余面板矮（还要让出一条反馈带），data/magics 有 10 门法术，
    // 加上「返回」就装不下了——装不下不要紧，要紧的是翻页键得管用。
    // 页大小若取条目数，ListView 会认为「整张表就一页」而让翻页毫无动静，
    // 玩家只能一路按下键去撞最后两门法术。
    const int magicRows = 11;
    ASSERT_GT(magicRows, rows) << "菜单若已经装得下十一行，这条用例该换个数";

    ListView menu;
    menu.setPageSize(rows);
    menu.setItems(makeItems(magicRows));
    const int before = menu.selection();
    menu.pageDown();
    EXPECT_NE(menu.selection(), before) << "装不下的长表必须还能翻页";

    // 而画出来的行数仍然是区域能给的全部，不会因为页大小再少一行。
    EXPECT_EQ(menu.visibleRows(Rect{0, 0, kAmpleWidth, listAreaHeight(rows, theme)}, theme), rows);
}

}  // namespace
