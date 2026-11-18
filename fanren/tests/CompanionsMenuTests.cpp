// 伙伴改版 D 路·主菜单（docs/interfaces-companions.md 2.2 赠礼、3.1 active、第 6 节 D 路）。
// 改 active 与赠礼都走真实菜单：世界层按 Tab 打开主菜单，方向键与确认键一路按下去。期望值照契约写死：
//   · 名册空着时左栏就是改版前那八项；名册上有人才在「状态」后面多出队伍、名册；
//   · active 只在队伍页改，伙伴能改，傀儡这类不是伙伴的队员改不了；
//   · 赠礼每人每天一次：合意 +5、寻常 +2、不喜 0（不喜也算今天送过了）；今天送过的不扣东西并给提示；
//     剧情物不在可送之列；
//   · 名册页列已结识的伙伴与默契的档名（初识 / 相熟 / 默契 / 知己 / 同心）；
//   · 小像倍数：旧表 scale 3 → ×4（与改版前一样），高清表 2 → ×2。
#include "CompanionsLaneDFixture.h"
#include "game/MenuScene.h"

namespace {

using namespace fanren::test::laned;
using fanren::game::MenuScene;
using Page = MenuScene::Page;

class MenuD : public LaneD {
protected:
    MenuScene* menu() { return dynamic_cast<MenuScene*>(app_.topScene()); }
    void openMenu() {
        press(SDL_SCANCODE_TAB);
        tick(30);
        ASSERT_EQ(top(), "Menu");
    }
    int bagCount(const std::string& item) {
        int n = 0;
        for (const auto& entry : app_.state().bag) {
            if (entry.itemId == item) n += entry.count;
        }
        return n;
    }
    int bond() { return app_.state().bondOf("probe_a"); }
    // 队伍页里，从队伍那一列（光标在第 0 行）走到赠与列表，往下 itemRow 行，确认；再按掉那句话。
    void giveFromPartyRow(int itemRow) {
        press(SDL_SCANCODE_RETURN);   // 第 0 行那位 → 两项动作
        press(SDL_SCANCODE_DOWN);     // 「赠与」
        press(SDL_SCANCODE_RETURN);   // → 背包
        for (int i = 0; i < itemRow; ++i) press(SDL_SCANCODE_DOWN);
        press(SDL_SCANCODE_RETURN);
        tick();
    }
    void dismissLine() {
        ASSERT_EQ(top(), "Dialogue");
        press(SDL_SCANCODE_RETURN);
        tick(2);
        ASSERT_EQ(top(), "Menu");
    }
};

TEST_F(MenuD, AnEmptyRosterLeavesTheEightOldPagesAndAMetCompanionAddsPartyAndRoster) {
    openMenu();
    const std::vector<Page> old{Page::Status, Page::Items, Page::Magic, Page::Journal,
                                Page::Save,   Page::Settings, Page::Home, Page::Back};
    EXPECT_EQ(menu()->pages(), old);
    press(SDL_SCANCODE_ESCAPE);
    tick(2);
    ASSERT_EQ(top(), "World");

    meet("e");   // probe_e 的 met 旗标：名册上有人了
    openMenu();
    const std::vector<Page> more{Page::Status, Page::Party, Page::Roster, Page::Items, Page::Magic,
                                 Page::Journal, Page::Save, Page::Settings, Page::Home, Page::Back};
    EXPECT_EQ(menu()->pages(), more);
}

TEST_F(MenuD, ThePartyPageTogglesActiveForCompanionsOnly) {
    meet("a");
    app_.state().party = {{"zhang_tie", -1, true}, {"qu_hun", 17, true}};
    openMenu();
    press(SDL_SCANCODE_DOWN);     // 队伍
    press(SDL_SCANCODE_RETURN);   // 进队伍那一列，光标在 zhang_tie
    press(SDL_SCANCODE_RETURN);   // 两项动作，光标在「上场 / 歇着」
    press(SDL_SCANCODE_RETURN);
    EXPECT_FALSE(app_.state().party[0].active) << "伙伴：上场 → 歇着";
    EXPECT_TRUE(app_.state().party[1].active);
    press(SDL_SCANCODE_RETURN);
    press(SDL_SCANCODE_RETURN);
    EXPECT_TRUE(app_.state().party[0].active) << "再按一次：歇着 → 上场";

    // 曲魂不是伙伴：确认只响一声错，按几下都改不了。
    press(SDL_SCANCODE_DOWN);
    for (int i = 0; i < 3; ++i) press(SDL_SCANCODE_RETURN);
    EXPECT_TRUE(app_.state().party[1].active);
    EXPECT_TRUE(app_.state().party[0].active);
    EXPECT_EQ(top(), "Menu");
}

TEST_F(MenuD, GiftsAreOncePerCompanionPerDayAndARefusedGiftCostsNothing) {
    meet("a");
    app_.state().party = {{"zhang_tie", -1, true}};
    app_.state().bag.clear();
    app_.state().addItem("material_dansha", 2);   // 合意
    app_.state().addItem("pill_huiqi_dan", 1);    // 寻常
    app_.state().addItem("material_fuzhi", 1);    // 不喜
    openMenu();
    press(SDL_SCANCODE_DOWN);     // 队伍
    press(SDL_SCANCODE_RETURN);   // 进队伍那一列

    giveFromPartyRow(0);
    EXPECT_EQ(bond(), 5) << "合意 +5";
    EXPECT_EQ(bagCount("material_dansha"), 1);
    EXPECT_EQ(lastLine(), app_.data().lookupText("ui.menu.gift.liked"));
    dismissLine();

    giveFromPartyRow(0);
    EXPECT_EQ(bond(), 5) << "今天已经送过了";
    EXPECT_EQ(bagCount("material_dansha"), 1) << "送不出去的不扣";
    EXPECT_EQ(lastLine(), app_.data().lookupText("ui.menu.gift.already"));
    dismissLine();

    app_.state().day += 1;
    giveFromPartyRow(1);
    EXPECT_EQ(bond(), 7) << "寻常 +2";
    EXPECT_EQ(bagCount("pill_huiqi_dan"), 0);
    EXPECT_EQ(lastLine(), app_.data().lookupText("ui.menu.gift.plain"));
    dismissLine();

    app_.state().day += 1;
    giveFromPartyRow(1);   // 寻常那件送完了，背包第 1 行此刻是不喜的那一件
    EXPECT_EQ(bond(), 7) << "不喜 +0";
    EXPECT_EQ(bagCount("material_fuzhi"), 0) << "不喜的也收下了";
    EXPECT_EQ(lastLine(), app_.data().lookupText("ui.menu.gift.disliked"));
    dismissLine();
    giveFromPartyRow(0);
    EXPECT_EQ(bagCount("material_dansha"), 1) << "不喜也算今天送过了";
    EXPECT_EQ(lastLine(), app_.data().lookupText("ui.menu.gift.already"));
}

TEST_F(MenuD, StoryItemsCannotBeGiven) {
    meet("a");
    app_.state().party = {{"zhang_tie", -1, true}};
    app_.state().bag.clear();
    app_.state().addItem("story_baixi_pei", 1);
    openMenu();
    press(SDL_SCANCODE_DOWN);
    press(SDL_SCANCODE_RETURN);
    giveFromPartyRow(0);
    EXPECT_EQ(bagCount("story_baixi_pei"), 1);
    EXPECT_EQ(bond(), 0);
    EXPECT_EQ(app_.state().bonds.count("probe_a"), 0u) << "账一笔没动";
    EXPECT_EQ(top(), "Menu") << "空的赠与列表上确认没有反应";
}

TEST_F(MenuD, TheRosterPageListsMetCompanionsWithTheirBondTier) {
    meet("a");
    meet("b");
    app_.state().bonds["probe_a"].value = 23;
    const auto rows = MenuScene::rosterRows(app_.data(), app_.state());
    ASSERT_EQ(rows.size(), 2u) << "没结识的 probe_c、d、e 不列";
    EXPECT_EQ(rows[0].label, app_.data().findRole("zhang_tie")->name) << "还没长成：用第一段的角色";
    EXPECT_EQ(rows[0].detail, "相熟");
    EXPECT_EQ(rows[1].label, app_.data().findRole("baiyi_shaonv")->name);
    EXPECT_EQ(rows[1].detail, "初识");
    for (const auto& [value, tier] : std::vector<std::pair<int, std::string>>{
             {44, "相熟"}, {45, "默契"}, {70, "知己"}, {95, "同心"}, {100, "同心"}}) {
        app_.state().bonds["probe_b"].value = value;
        EXPECT_EQ(MenuScene::rosterRows(app_.data(), app_.state())[1].detail, tier) << value;
    }
    app_.state().setFlag("probe.a.grown");
    EXPECT_EQ(MenuScene::rosterRows(app_.data(), app_.state())[0].label, app_.data().findRole("li_feiyu")->name);

    // 真入口：光标停到名册页、确认，菜单还开着、什么也没改。
    openMenu();
    press(SDL_SCANCODE_DOWN);
    press(SDL_SCANCODE_DOWN);
    press(SDL_SCANCODE_RETURN);
    EXPECT_EQ(top(), "Menu");
}

TEST(MenuSpriteScale, OldSheetsKeepFourAndHdSheetsDrawAtTwo) {
    EXPECT_EQ(MenuScene::menuSpriteScale(3), 4) << "旧表：与改版前写死的 ×4 一样";
    EXPECT_EQ(MenuScene::menuSpriteScale(2), 2) << "高清表";
    EXPECT_EQ(MenuScene::menuSpriteScale(1), 1);
    EXPECT_EQ(MenuScene::menuSpriteScale(0), 1) << "至少 1";
}

}  // namespace
