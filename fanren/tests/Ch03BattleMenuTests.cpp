// 战斗动作菜单（技术债 G-5）。
//
// 这一条挡在第 3 章教学战二前面，验的是同一句话能不能成立：
// `docs/ch03-design.md` 第 2 节要教「物品与用毒」——没有入口就教不成。
//
// 从前本文件的另一半是物品使用距离（技术债 G-7：蚀心散撒得到三格）。横版没有格子，
// 那一半连同 core::Item::useRange 一起删了；「砍不动就下毒」如今落在破绽上（僵兽的破绽
// 是毒与火，撒一包算「毒」类一击），见 tests/BattleTests.cpp 的 APoisonItemIsAPoisonHit。
// 加载器对新字段的校验另见 tests/BattleDataLoadingTests.cpp。
//
// 三条纪律贯穿本文件，写断言前逐条对照 `docs/README.md` 那张「判据会说谎」表：
//
//   1. **每一条禁用理由先验它真有内容**（ASSERT_FALSE(reason.empty())），再验它
//      说的是哪一件事。只写「找不到某个词就算过」的话，理由字符串变成空的时候
//      这些断言照样全绿——那正是这张表上第二行的长相。
//   2. **每一条肯定都配一条否定**：能用的 / 不能用的，学过的 / 没学过的。
//      只写一边，把整段判定删掉也能全绿。
//   3. **菜单的判据与真发起时的判据必须是同一个**。这一条单独有一测：列表上
//      写着的那句话，要与 refusePlayerAction 给出的一字不差。
#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "core/battle/Battle.h"
#include "core/model/Types.h"
#include "game/Application.h"
#include "game/BattleScene.h"
#include "ui/Widgets.h"

namespace {

namespace fs = std::filesystem;

using fanren::core::Item;
using fanren::core::Magic;
using fanren::core::battle::Action;
using fanren::core::battle::ActionKind;
using fanren::core::battle::BattleState;
using fanren::core::battle::Unit;
using fanren::game::BattleMenuMode;
using fanren::game::BattleScene;
using fanren::game::kBattleMenuAttack;
using fanren::game::kBattleMenuCast;
using fanren::game::kBattleMenuCount;
using fanren::game::kBattleMenuDefend;
using fanren::game::kBattleMenuEscape;
using fanren::game::kBattleMenuItem;
using fanren::game::refusePlayerAction;
using fanren::rules::Realm;

// ---------------------------------------------------------------------------
// 零、这条路是封着的
//
// G-5 明写：菜单**必须只调 BattleScene::issuePlayerAction**，不能另起一条直接喂
// BattleState::apply 的路，否则扣背包会漏在新路径上。下面这条 static_assert 是
// 那句话的编译期版本：场景交出去的 BattleState 是只读的，外人拿不到能 apply 的
// 那一个，于是「另一条路」根本不存在。
// ---------------------------------------------------------------------------
static_assert(std::is_same_v<decltype(std::declval<const BattleScene&>().battle()),
                             const BattleState&>,
              "BattleScene::battle() 必须只交出只读的 BattleState —— 一旦能拿到可改的，"
              "就多了一条绕开 issuePlayerAction 的路，而扣背包只活在 issuePlayerAction 里");

Unit makeUnit(const std::string& name, bool ally, int speed) {
    Unit u;
    u.id = name;
    u.name = name;
    u.hp = u.maxHp = 100;
    u.mp = u.maxMp = 40;
    u.attack = 10;
    u.defence = 5;
    u.speed = speed;
    u.realm = Realm::QiRefining3;
    u.ally = ally;
    u.magicsExhaustive = true;
    return u;
}

Action useItem(int actor, int target, const std::string& itemId) {
    Action a;
    a.kind = ActionKind::Item;
    a.actorIndex = actor;
    a.targetIndex = target;
    a.magicId = itemId;   // 契约的 Action 没有 itemId，物品借用这个槽位
    return a;
}

// ---------------------------------------------------------------------------
// 真数据 + 真战斗：菜单
// ---------------------------------------------------------------------------

class BattleMenuTest : public ::testing::Test {
protected:
    static std::string assetRoot() {
        for (const char* candidate : {".", "..", "../..", "../../.."}) {
            if (fs::exists(fs::path(candidate) / "data" / "battles" / "b03_gu_wai_elang.json")) {
                return candidate;
            }
        }
        return ".";
    }

    void SetUp() override {
        auto ready = app_.init(assetRoot(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
        // 韩立开局只有 10 点气血，一个照面就会被放倒，那样后面几条测的就成了
        // 「战斗早结束了」。给他一副够用的身板（与 Ch03BattleKitTests 同一个办法）。
        app_.state().maxHp = 240;
        app_.state().hp = 240;
        app_.state().maxMp = 60;
        app_.state().mp = 60;
    }
    void TearDown() override { app_.shutdown(); }

    fanren::core::GameState& state() { return app_.state(); }

    static int indexOfLabel(const fanren::ui::ListView& list, const std::string& label) {
        for (int i = 0; i < list.count(); ++i) {
            if (list.items()[static_cast<std::size_t>(i)].label == label) return i;
        }
        return -1;
    }

    static int firstEnabled(const fanren::ui::ListView& list) {
        for (int i = 0; i < list.count(); ++i) {
            const fanren::ui::ListItem& row = list.items()[static_cast<std::size_t>(i)];
            if (row.enabled && row.label != "返回") return i;
        }
        return -1;
    }

    static bool logHas(const BattleState& battle, const std::string& needle) {
        return std::any_of(
            battle.log().begin(), battle.log().end(),
            [&needle](const std::string& line) { return line.find(needle) != std::string::npos; });
    }

    // 过一个回合：开菜单选「防御」。这是玩家真会做的那一步，也顺手钉住
    // 「菜单里的动作确实结束本单位的回合」。
    void passTurn(BattleScene& scene) {
        scene.openMenu(app_);
        ASSERT_EQ(scene.menuMode(), BattleMenuMode::Root);
        ASSERT_TRUE(scene.menuChoose(app_, kBattleMenuDefend))
            << scene.menuList().items()[static_cast<std::size_t>(kBattleMenuDefend)].disabledReason;
    }

    fanren::game::Application app_;
};

TEST_F(BattleMenuTest, TheRootMenuOffersTheFiveActionsInAFixedOrder) {
    BattleScene scene("b03_gu_wai_elang");
    scene.onEnter(app_);
    ASSERT_EQ(scene.menuMode(), BattleMenuMode::Closed) << "开场菜单是收着的";

    ASSERT_GE(scene.runToAllyTurn(), 0);
    scene.openMenu(app_);
    ASSERT_EQ(scene.menuMode(), BattleMenuMode::Root);

    const fanren::ui::ListView& list = scene.menuList();
    ASSERT_EQ(list.count(), kBattleMenuCount);
    // 施工图 docs/octopath-overhaul.md 第 2 节：攻击 / 法术 / 物品 / 防御 / 逃跑。
    //（从前第二项叫「施法」，横版菜单按施工图改叫「法术」。）第二项的名字按阶段取（Wording.h）：
    // 第 1–5 章是凡人阶段，与主菜单同叫「法门」（终审 LOW-9）；修仙阶段那一半见 tests/GameWiringTests.cpp。
    EXPECT_EQ(list.items()[static_cast<std::size_t>(kBattleMenuAttack)].label, "攻击");
    EXPECT_EQ(list.items()[static_cast<std::size_t>(kBattleMenuCast)].label, "法门");
    EXPECT_EQ(list.items()[static_cast<std::size_t>(kBattleMenuItem)].label, "物品");
    EXPECT_EQ(list.items()[static_cast<std::size_t>(kBattleMenuDefend)].label, "防御");
    EXPECT_EQ(list.items()[static_cast<std::size_t>(kBattleMenuEscape)].label, "逃跑");
}

TEST_F(BattleMenuTest, TheMenuIsNotOfferedOnTheEnemyTurn) {
    // 反向：菜单不是随时能开的。开场第一个动的是饿狼（身法 9 对韩立的 5）。
    BattleScene scene("b03_gu_wai_elang");
    scene.onEnter(app_);
    const int first = scene.battle().currentActor();
    ASSERT_GE(first, 0);
    ASSERT_FALSE(scene.battle().units()[static_cast<std::size_t>(first)].ally)
        << "这一场的编成变了：开场就轮到我方的话，这一条测的不是它想测的东西";

    scene.openMenu(app_);
    EXPECT_EQ(scene.menuMode(), BattleMenuMode::Closed);
}

TEST_F(BattleMenuTest, ChoosingAnItemFromTheMenuSpendsItFromTheBag) {
    // 这是 G-5 的正面：玩家**选得出** Item，而且那一步确实走了 issuePlayerAction
    // —— 背包少了一颗就是证据（core 不持有玩家的物品栏，BattleState::apply 不会
    // 也不可能扣它）。
    state().hp = 40;
    state().addItem("pill_jinchuang_yao", 2);

    BattleScene scene("b03_gu_wai_elang");
    scene.onEnter(app_);
    ASSERT_TRUE(scene.battle().hasItem("pill_jinchuang_yao"));

    const int actor = scene.runToAllyTurn();
    ASSERT_GE(actor, 0);
    const int hpBefore = scene.battle().units()[static_cast<std::size_t>(actor)].hp;

    scene.openMenu(app_);
    ASSERT_TRUE(scene.menuList().items()[static_cast<std::size_t>(kBattleMenuItem)].enabled)
        << scene.menuList().items()[static_cast<std::size_t>(kBattleMenuItem)].disabledReason;
    ASSERT_TRUE(scene.menuChoose(app_, kBattleMenuItem));
    ASSERT_EQ(scene.menuMode(), BattleMenuMode::Item);

    const int pill = indexOfLabel(scene.menuList(), "金疮药");
    ASSERT_GE(pill, 0) << "背包里有、战斗里有用的东西必须列出来";
    ASSERT_TRUE(scene.menuList().items()[static_cast<std::size_t>(pill)].enabled)
        << scene.menuList().items()[static_cast<std::size_t>(pill)].disabledReason;
    ASSERT_TRUE(scene.menuChoose(app_, pill));
    ASSERT_EQ(scene.menuMode(), BattleMenuMode::Target);

    const int self = indexOfLabel(scene.menuList(), app_.speakerName("hanli"));
    ASSERT_GE(self, 0) << "丹药该能给自己用，自己必须在目标里";
    ASSERT_TRUE(scene.menuChoose(app_, self));

    EXPECT_EQ(scene.menuMode(), BattleMenuMode::Closed) << "做完一步菜单就该收起来";
    EXPECT_EQ(state().itemCount("pill_jinchuang_yao"), 1)
        << "用掉一颗背包就该少一颗 —— 少了才说明这一步走的是 issuePlayerAction";
    EXPECT_GT(scene.battle().units()[static_cast<std::size_t>(actor)].hp, hpBefore);
}

TEST_F(BattleMenuTest, ABattleStateOnItsOwnNeverTouchesTheBag) {
    // 上一条的反向，也是 G-5 那句话的实证：同样一件丹药，绕开 issuePlayerAction
    // 直接喂给 BattleState::apply，药照样生效、背包却一颗不少。
    state().addItem("pill_jinchuang_yao", 2);
    const Item* pill = app_.data().findItem("pill_jinchuang_yao");
    ASSERT_NE(pill, nullptr);
    ASSERT_GT(pill->restoreHp, 0) << "先验：这颗药真能回血，否则下面看不出它生效过";

    std::vector<Unit> units{makeUnit("韩立", true, 12), makeUnit("野狗", false, 8)};
    units[0].hp = 50;
    BattleState bare;
    bare.setup(std::move(units), 3u);
    bare.addItem(*pill);
    ASSERT_EQ(bare.currentActor(), 0);

    const auto result = bare.apply(useItem(0, 0, "pill_jinchuang_yao"));
    ASSERT_TRUE(result.ok) << result.error;
    EXPECT_GT(bare.units()[0].hp, 50) << "药在 core 里照样生效";
    EXPECT_EQ(state().itemCount("pill_jinchuang_yao"), 2)
        << "而背包一颗没少 —— 这就是「另起一条直接喂 apply 的路」的代价";
}

TEST_F(BattleMenuTest, ChoosingASpellFromTheMenuReallyCastsIt) {
    // G-5 的另一半：玩家选得出 Cast。让一个会火球术的同伴上场——这也正是队伍系统落地后
    // 该有的样子。
    const fanren::core::RoleTemplate* mate = app_.data().findRole("jiexiu");
    ASSERT_NE(mate, nullptr);
    ASSERT_FALSE(mate->magics.empty()) << "先验：这位同伴本来就该会法术";
    const Magic* fireball = app_.data().findMagic("magic_huoqiu_shu");
    ASSERT_NE(fireball, nullptr);
    ASSERT_GE(mate->maxMp, fireball->needMp) << "先验：法力本来就够，下面的禁用不会是法力问题";

    state().party.push_back(fanren::core::PartyMember{"jiexiu", -1, true});

    BattleScene scene("b03_gu_wai_elang");
    scene.onEnter(app_);
    ASSERT_TRUE(scene.battle().hasMagic("magic_huoqiu_shu")) << "同伴声明了它，本场就该登记上";

    bool cast = false;
    int mpBefore = 0;
    int mpAfter = 0;
    for (int guard = 0; guard < 40 && !cast; ++guard) {
        const int actor = scene.runToAllyTurn();
        ASSERT_GE(actor, 0) << "战斗先结束了，没等到同伴出手";
        if (scene.battle().units()[static_cast<std::size_t>(actor)].id != "jiexiu") {
            passTurn(scene);
            continue;
        }

        scene.openMenu(app_);
        ASSERT_TRUE(scene.menuList().items()[static_cast<std::size_t>(kBattleMenuCast)].enabled)
            << scene.menuList().items()[static_cast<std::size_t>(kBattleMenuCast)].disabledReason;
        ASSERT_TRUE(scene.menuChoose(app_, kBattleMenuCast));
        ASSERT_EQ(scene.menuMode(), BattleMenuMode::Magic);

        const int spell = indexOfLabel(scene.menuList(), fireball->name);
        ASSERT_GE(spell, 0) << "他会的法术必须列出来";
        // 横版没有射程：轮到他就放得出来（从前这里要等狼扑进火球术的四格射程）。
        ASSERT_TRUE(scene.menuList().items()[static_cast<std::size_t>(spell)].enabled)
            << scene.menuList().items()[static_cast<std::size_t>(spell)].disabledReason;
        ASSERT_TRUE(scene.menuChoose(app_, spell));
        ASSERT_EQ(scene.menuMode(), BattleMenuMode::Target);

        const int target = firstEnabled(scene.menuList());
        ASSERT_GE(target, 0) << "场上有敌人，目标那一级却一条也点不动";
        mpBefore = scene.battle().units()[static_cast<std::size_t>(actor)].mp;
        ASSERT_TRUE(scene.menuChoose(app_, target));
        mpAfter = scene.battle().units()[static_cast<std::size_t>(actor)].mp;
        cast = true;
    }

    ASSERT_TRUE(cast) << "四十个回合都没能从菜单里放出一条法术";
    EXPECT_EQ(mpAfter, mpBefore - fireball->needMp) << "法力得真的扣";
    EXPECT_TRUE(logHas(scene.battle(), "施展")) << "战斗日志里得真有这一发";
}

TEST_F(BattleMenuTest, HanLiHasLearnedNoSpellAndTheMenuSaysSoInWords) {
    // 空列表不能给一个空框，要写明是「他还没学过」——这与「这一场施展不出来」
    // 要靠完全不同的方式解决。
    BattleScene scene("b03_gu_wai_elang");
    scene.onEnter(app_);
    const int actor = scene.runToAllyTurn();
    ASSERT_GE(actor, 0);
    const Unit& hero = scene.battle().units()[static_cast<std::size_t>(actor)];
    ASSERT_TRUE(hero.magicsExhaustive) << "先验：他那份清单是穷尽的，空就是真的一条不会";
    ASSERT_TRUE(hero.magics.empty()) << "先验：他现在确实一条法术都没有";

    scene.openMenu(app_);
    const fanren::ui::ListItem& castRow = scene.menuList().items()[static_cast<std::size_t>(kBattleMenuCast)];
    EXPECT_FALSE(castRow.enabled);
    ASSERT_FALSE(castRow.disabledReason.empty()) << "空的就该说为什么空，不能只是变灰";
    EXPECT_NE(castRow.disabledReason.find("还没学过"), std::string::npos)
        << castRow.disabledReason;
    EXPECT_NE(castRow.disabledReason.find(hero.name), std::string::npos)
        << "说的是谁也要写出来: " << castRow.disabledReason;

    // 点不动：禁用项按下去确实什么也不发生。
    EXPECT_FALSE(scene.menuChoose(app_, kBattleMenuCast));
    EXPECT_EQ(scene.menuMode(), BattleMenuMode::Root) << "被拒的选择不该把菜单带到别处";

    std::vector<std::string> ids;
    const std::vector<fanren::ui::ListItem> rows = BattleScene::buildMagicItems(
        app_.data(), state(), scene.battle(), actor, ids);
    EXPECT_TRUE(ids.empty());
    ASSERT_FALSE(rows.empty());
    EXPECT_NE(rows.front().disabledReason.find("还没学过"), std::string::npos)
        << rows.front().disabledReason;
}

TEST_F(BattleMenuTest, ACompanionWhoKnowsASpellMakesTheSameEntryLightUp) {
    // 上一条的反向对照：「法术」不是永远灰着的。
    state().party.push_back(fanren::core::PartyMember{"jiexiu", -1, true});
    BattleScene scene("b03_gu_wai_elang");
    scene.onEnter(app_);

    bool sawLit = false;
    for (int guard = 0; guard < 12 && !sawLit; ++guard) {
        const int actor = scene.runToAllyTurn();
        ASSERT_GE(actor, 0);
        if (scene.battle().units()[static_cast<std::size_t>(actor)].id == "jiexiu") {
            std::vector<std::string> ids;
            static_cast<void>(BattleScene::buildMagicItems(app_.data(), state(), scene.battle(),
                                                           actor, ids));
            EXPECT_FALSE(ids.empty()) << "他会火球术，列表里就该有";
            scene.openMenu(app_);
            EXPECT_TRUE(scene.menuList().items()[static_cast<std::size_t>(kBattleMenuCast)].enabled)
                << scene.menuList().items()[static_cast<std::size_t>(kBattleMenuCast)].disabledReason;
            sawLit = true;
            break;
        }
        passTurn(scene);
    }
    EXPECT_TRUE(sawLit) << "十二个回合都没轮到那位同伴";
}

TEST_F(BattleMenuTest, AnEmptyBagMakesTheItemEntrySayWhyItIsEmpty) {
    BattleScene scene("b03_gu_wai_elang");
    scene.onEnter(app_);
    ASSERT_EQ(scene.battle().registeredItemCount(), 0u) << "先验：背包空着，一件也没登记";
    ASSERT_GE(scene.runToAllyTurn(), 0);

    scene.openMenu(app_);
    const fanren::ui::ListItem& itemRow = scene.menuList().items()[static_cast<std::size_t>(kBattleMenuItem)];
    EXPECT_FALSE(itemRow.enabled);
    ASSERT_FALSE(itemRow.disabledReason.empty());
    EXPECT_NE(itemRow.disabledReason.find("身上没有"), std::string::npos)
        << itemRow.disabledReason;
}

TEST_F(BattleMenuTest, TheMindBattleSaysTheseThingsDoNotWorkHere) {
    // 反向对照：同样是「一件都没有」，识海那一场的原因完全不同 —— 背包里有，
    // 只是这一场用不上（契约 docs/interfaces-p3-ch03.md 3.5 节的明写例外）。
    state().addItem("pill_jinchuang_yao", 3);
    BattleScene mind("b03_shihai_duoshe");
    mind.onEnter(app_);
    ASSERT_EQ(mind.battle().registeredItemCount(), 0u);

    const int actor = mind.runToAllyTurn();
    ASSERT_GE(actor, 0);
    mind.openMenu(app_);
    const fanren::ui::ListItem& itemRow = mind.menuList().items()[static_cast<std::size_t>(kBattleMenuItem)];
    EXPECT_FALSE(itemRow.enabled);
    ASSERT_FALSE(itemRow.disabledReason.empty());
    EXPECT_NE(itemRow.disabledReason.find("这一场用不上"), std::string::npos)
        << itemRow.disabledReason;
}

TEST_F(BattleMenuTest, TheLastPillStaysOnTheListAndSaysTheBagIsEmpty) {
    // 背包没有了：登记表是开战那一刻的快照，用光之后它还在表里 —— 直接从列表里抹掉，
    // 玩家会以为界面丢了一行。
    state().hp = 40;
    state().addItem("pill_jinchuang_yao", 1);

    BattleScene scene("b03_gu_wai_elang");
    scene.onEnter(app_);
    const int actor = scene.runToAllyTurn();
    ASSERT_GE(actor, 0);

    scene.openMenu(app_);
    ASSERT_TRUE(scene.menuChoose(app_, kBattleMenuItem));
    const int pill = indexOfLabel(scene.menuList(), "金疮药");
    ASSERT_GE(pill, 0);
    ASSERT_TRUE(scene.menuList().items()[static_cast<std::size_t>(pill)].enabled);
    ASSERT_TRUE(scene.menuChoose(app_, pill));
    const int self = indexOfLabel(scene.menuList(), app_.speakerName("hanli"));
    ASSERT_GE(self, 0);
    ASSERT_TRUE(scene.menuChoose(app_, self));
    ASSERT_EQ(state().itemCount("pill_jinchuang_yao"), 0);

    // 下一个我方回合再看那一行。
    const int again = scene.runToAllyTurn();
    ASSERT_GE(again, 0);
    scene.openMenu(app_);
    ASSERT_TRUE(scene.menuChoose(app_, kBattleMenuItem));
    const int gone = indexOfLabel(scene.menuList(), "金疮药");
    ASSERT_GE(gone, 0) << "用光了也要照样列出来";
    const fanren::ui::ListItem& row = scene.menuList().items()[static_cast<std::size_t>(gone)];
    EXPECT_FALSE(row.enabled);
    ASSERT_FALSE(row.disabledReason.empty());
    EXPECT_NE(row.disabledReason.find("背包"), std::string::npos) << row.disabledReason;
    EXPECT_FALSE(scene.menuChoose(app_, gone)) << "点不动";
}

TEST_F(BattleMenuTest, AMagicThatHurtsNobodySaysSoInsteadOfBlamingTheRange) {
    // 御风决（power 0）从前在菜单里是一条写着「超出施法距离」的灰行——对一门辅助法术
    // 那是误导（data/magics/yufeng_jue.json 的 note 请主控裁决过）。横版没有距离，
    // 挡它的理由就得是实话：它不是伤人的法术。
    const Magic* wind = app_.data().findMagic("magic_yufeng_jue");
    ASSERT_NE(wind, nullptr);
    ASSERT_EQ(wind->power, 0) << "先验：它本来就不伤人";
    state().realm = Realm::QiRefining8;
    state().maxMp = state().mp = 80;
    ASSERT_TRUE(state().learnMagic("magic_yufeng_jue"));
    ASSERT_TRUE(state().learnMagic("magic_huodan_shu"));

    BattleScene scene("b03_gu_wai_elang");
    scene.onEnter(app_);
    const int actor = scene.runToAllyTurn();
    ASSERT_GE(actor, 0);
    std::vector<std::string> ids;
    const std::vector<fanren::ui::ListItem> rows =
        BattleScene::buildMagicItems(app_.data(), state(), scene.battle(), actor, ids);
    ASSERT_EQ(ids.size(), 2u);
    for (std::size_t i = 0; i < ids.size(); ++i) {
        if (ids[i] == "magic_yufeng_jue") {
            EXPECT_FALSE(rows[i].enabled);
            ASSERT_FALSE(rows[i].disabledReason.empty());
            // 规则层说「法术」，凡人阶段的界面转述成「法门」（Wording.h 的 stageWords）。
            EXPECT_NE(rows[i].disabledReason.find("不是伤人的法门"), std::string::npos)
                << rows[i].disabledReason;
            EXPECT_EQ(rows[i].disabledReason.find("距离"), std::string::npos)
                << "横版没有距离，理由里不该再出现它：" << rows[i].disabledReason;
        } else {
            // 反向：同一张表里伤人的那一门是亮的。
            EXPECT_TRUE(rows[i].enabled) << ids[i] << "：" << rows[i].disabledReason;
        }
    }
}

TEST_F(BattleMenuTest, AMagicWithoutEnoughManaSaysSo) {
    // 法力不足。就地搭一个局面，不必先凑一整场真战斗。
    Magic spell;
    spell.id = "magic_probe";
    spell.name = "试探术";
    spell.needMp = 20;
    spell.power = 5;
    spell.needRealm = Realm::QiRefining1;

    std::vector<Unit> units{makeUnit("术者", true, 12), makeUnit("靶子", false, 8)};
    units[0].magics = {spell.id};
    units[0].mp = spell.needMp - 1;   // 差一点
    BattleState battle;
    battle.setup(std::move(units), 9u);
    battle.addMagic(spell);
    ASSERT_EQ(battle.currentActor(), 0);

    fanren::core::GameData data;
    data.magics.emplace(spell.id, spell);
    fanren::core::GameState empty;

    std::vector<std::string> ids;
    std::vector<fanren::ui::ListItem> rows =
        BattleScene::buildMagicItems(data, empty, battle, 0, ids);
    ASSERT_EQ(ids.size(), 1u) << "他声明了这一条，就该列出来 —— 列不出来等于把问题藏了";
    ASSERT_FALSE(rows.empty());
    EXPECT_FALSE(rows.front().enabled);
    ASSERT_FALSE(rows.front().disabledReason.empty());
    // 规则层说「法力不足」，凡人阶段的界面转述成「气力不足」——与主菜单的「气力」同词。
    EXPECT_NE(rows.front().disabledReason.find("气力不足"), std::string::npos)
        << rows.front().disabledReason;

    // 反向：只把法力补满，同一条就亮了。没有这一句，把该项写死成禁用也能全绿。
    std::vector<Unit> rich = battle.units();
    rich[0].mp = spell.needMp;
    BattleState ready;
    ready.setup(std::move(rich), 9u);
    ready.addMagic(spell);
    ids.clear();
    rows = BattleScene::buildMagicItems(data, empty, ready, 0, ids);
    ASSERT_EQ(ids.size(), 1u);
    EXPECT_TRUE(rows.front().enabled) << rows.front().disabledReason;
}

TEST_F(BattleMenuTest, EveryReasonOnScreenIsTheOneTheActionWouldActuallyGive) {
    // 纪律三：菜单画的禁用理由与真发起时被回绝的那句话，必须一字不差。
    //
    // 局面：暗道那一仗我方第一次出手时还没中毒（僵兽身法 3，排在韩立后面），
    // 于是一包蚀心散点得动（往敌人身上撒）、一包清毒散点不动（「没有中毒，用不上」）。
    // 从前这里的「说不通」是毒够不着（使用距离）；横版没有距离，换成解毒药这一条。
    state().addItem("pill_shixin_san", 1);
    state().addItem("pill_qingdu_san", 1);

    BattleScene scene("b03_andao_shishou");
    scene.onEnter(app_);
    const int actor = scene.runToAllyTurn();
    ASSERT_GE(actor, 0);
    ASSERT_EQ(scene.battle().units()[static_cast<std::size_t>(actor)].poison, 0)
        << "先验：韩立此刻没中毒，清毒散这一行才说得通「用不上」";

    std::vector<std::string> ids;
    const std::vector<fanren::ui::ListItem> rows =
        BattleScene::buildItemItems(app_.data(), state(), scene.battle(), actor, ids);
    ASSERT_EQ(ids.size(), 2u) << "两件都该在表里";

    int disabled = 0;
    const bool actorSide = scene.battle().units()[static_cast<std::size_t>(actor)].ally;
    for (std::size_t i = 0; i < ids.size(); ++i) {
        const Item* item = app_.data().findItem(ids[i]);
        ASSERT_NE(item, nullptr);
        // 往哪一边使由数据决定：毒往对面，药给自己人。那一边**每一个人**都真的试一遍。
        const bool wantAlly = item->poison <= 0;
        std::vector<std::string> answers;
        for (std::size_t u = 0; u < scene.battle().units().size(); ++u) {
            const Unit& unit = scene.battle().units()[u];
            if (!unit.alive() || (unit.ally == actorSide) != wantAlly) continue;
            answers.push_back(refusePlayerAction(state(), scene.battle(),
                                                 useItem(actor, static_cast<int>(u), ids[i])));
        }
        ASSERT_FALSE(answers.empty()) << ids[i] << "：那一边一个人也没有，比不出东西";
        const bool anyoneOk = std::any_of(answers.begin(), answers.end(),
                                          [](const std::string& why) { return why.empty(); });
        const fanren::ui::ListItem& row = rows[i];
        EXPECT_EQ(row.enabled, anyoneOk) << ids[i];
        if (!row.enabled) {
            ++disabled;
            // 灰着的那一行写的话，正是对着那一边的人真发起时被回绝的那一句。
            EXPECT_NE(std::find(answers.begin(), answers.end(), row.disabledReason), answers.end())
                << ids[i] << " 上写的话与真发起时说的不一样：" << row.disabledReason;
        }
    }
    // 先验：这一局面里确实**有**说不通的项。一条禁用都没有的话，上面那圈比较等于没比。
    EXPECT_GE(disabled, 1) << "这一局面本来就该有点不动的东西（清毒散）";
}

TEST_F(BattleMenuTest, TheEscapeEntryRepeatsWhyThisFightCannotBeLeft) {
    // 识海之战是剧情硬仗（can_escape=false）。逃跑那一项要原样转述 core 的理由。
    BattleScene mind("b03_shihai_duoshe");
    mind.onEnter(app_);
    ASSERT_GE(mind.runToAllyTurn(), 0);
    mind.openMenu(app_);
    const fanren::ui::ListItem& escape = mind.menuList().items()[static_cast<std::size_t>(kBattleMenuEscape)];
    EXPECT_FALSE(escape.enabled);
    ASSERT_FALSE(escape.disabledReason.empty());
    EXPECT_NE(escape.disabledReason.find("无法回避"), std::string::npos)
        << escape.disabledReason;

    // 反向对照：普通遭遇战逃得掉，这一项就是亮的。
    BattleScene wild("b03_gu_wai_elang");
    wild.onEnter(app_);
    ASSERT_GE(wild.runToAllyTurn(), 0);
    wild.openMenu(app_);
    EXPECT_TRUE(wild.menuList().items()[static_cast<std::size_t>(kBattleMenuEscape)].enabled)
        << wild.menuList().items()[static_cast<std::size_t>(kBattleMenuEscape)].disabledReason;
}

TEST_F(BattleMenuTest, CancellingWalksBackOneLevelAtATime) {
    state().addItem("pill_jinchuang_yao", 1);
    BattleScene scene("b03_gu_wai_elang");
    scene.onEnter(app_);
    ASSERT_GE(scene.runToAllyTurn(), 0);

    scene.openMenu(app_);
    ASSERT_TRUE(scene.menuChoose(app_, kBattleMenuItem));
    ASSERT_EQ(scene.menuMode(), BattleMenuMode::Item);
    const int pill = indexOfLabel(scene.menuList(), "金疮药");
    ASSERT_GE(pill, 0);
    ASSERT_TRUE(scene.menuChoose(app_, pill));
    ASSERT_EQ(scene.menuMode(), BattleMenuMode::Target);

    scene.menuBack(app_);
    EXPECT_EQ(scene.menuMode(), BattleMenuMode::Item) << "从择敌退回物品，不是一路退到底";
    scene.menuBack(app_);
    EXPECT_EQ(scene.menuMode(), BattleMenuMode::Root);
    scene.menuBack(app_);
    EXPECT_EQ(scene.menuMode(), BattleMenuMode::Closed);
    EXPECT_EQ(state().itemCount("pill_jinchuang_yao"), 1) << "一路退出来不该用掉任何东西";
}

// ---------------------------------------------------------------------------
// 横版新添：兵刃那一级、蓄劲、目标行上的破绽
// ---------------------------------------------------------------------------

TEST_F(BattleMenuTest, OneWeaponGoesStraightToTheTargetsTwoWeaponsAskFirst) {
    // 空手的韩立只有拳：攻击直接进择敌。揣上软剑就多一级「兵刃」，拳与剑两行。
    BattleScene bare("b03_gu_wai_elang");
    bare.onEnter(app_);
    ASSERT_GE(bare.runToAllyTurn(), 0);
    bare.openMenu(app_);
    ASSERT_TRUE(bare.menuChoose(app_, kBattleMenuAttack));
    EXPECT_EQ(bare.menuMode(), BattleMenuMode::Target) << "只有一样兵刃，不该多问一级";

    state().addItem("weapon_yudai_duanjian", 1);
    BattleScene armed("b03_gu_wai_elang");
    armed.onEnter(app_);
    ASSERT_GE(armed.runToAllyTurn(), 0);
    armed.openMenu(app_);
    ASSERT_TRUE(armed.menuChoose(app_, kBattleMenuAttack));
    ASSERT_EQ(armed.menuMode(), BattleMenuMode::Weapon);
    EXPECT_GE(indexOfLabel(armed.menuList(), "拳"), 0) << "空手永远算一样";
    const int sword = indexOfLabel(armed.menuList(), "剑");
    ASSERT_GE(sword, 0) << "揣着玉带短剑就有「剑」";
    ASSERT_TRUE(armed.menuChoose(app_, sword));
    EXPECT_EQ(armed.menuMode(), BattleMenuMode::Target);
    armed.menuBack(app_);
    EXPECT_EQ(armed.menuMode(), BattleMenuMode::Weapon) << "从择敌退回兵刃那一级";
}

TEST_F(BattleMenuTest, TheTargetRowShowsToughnessAndTheWeaknessSlots) {
    // 设计原文：目标选择时列出目标并显示其已知破绽。没揭开的写「？」。
    state().learnWeaknesses("wild_wolf", fanren::core::kCategorySword);
    BattleScene scene("b03_gu_wai_elang");
    scene.onEnter(app_);
    ASSERT_GE(scene.runToAllyTurn(), 0);
    scene.openMenu(app_);
    ASSERT_TRUE(scene.menuChoose(app_, kBattleMenuAttack));
    ASSERT_EQ(scene.menuMode(), BattleMenuMode::Target);
    const int wolf = firstEnabled(scene.menuList());
    ASSERT_GE(wolf, 0);
    const std::string detail = scene.menuList().items()[static_cast<std::size_t>(wolf)].detail;
    ASSERT_FALSE(detail.empty());
    EXPECT_NE(detail.find("破绽"), std::string::npos) << detail;
    EXPECT_NE(detail.find("剑"), std::string::npos) << "揭开过的写出类别：" << detail;
    EXPECT_NE(detail.find("？"), std::string::npos) << "没揭开的写问号：" << detail;
    // 那一行只放破绽（终审 LOW-1：三样拼一行，名字一长，被截掉的总是破绽）；架势与气血整句写在
    // 菜单下方的说明区，说的是同一只狼。
    for (const Unit& unit : scene.battle().units()) {
        if (unit.ally || unit.id != "wild_wolf") continue;
        const std::string note = fanren::game::foeStatusText(unit);
        EXPECT_NE(note.find("气血"), std::string::npos) << note;
        EXPECT_NE(note.find("架势"), std::string::npos) << note;
        EXPECT_NE(note.find("破绽 " + std::string(detail.substr(detail.find("破绽") + std::string("破绽 ").size()))),
                  std::string::npos)
            << "说明区的破绽与那一行是同一排：" << note << " / " << detail;
        break;
    }
}

TEST_F(BattleMenuTest, TheBoostIsClampedToWhatTheActorHasAndRidesOnTheAttack) {
    BattleScene scene("b03_gu_wai_elang");
    scene.onEnter(app_);
    const int actor = scene.runToAllyTurn();
    ASSERT_GE(actor, 0);
    scene.openMenu(app_);
    const int bp = scene.battle().units()[static_cast<std::size_t>(actor)].bp;
    ASSERT_GE(bp, 1) << "先验：开场 1 点劲";
    scene.setBoost(9);
    EXPECT_EQ(scene.boost(), std::min(3, bp)) << "左右键加不过现有的劲，也加不过 3";
    scene.boostDown();
    scene.boostDown();
    scene.boostDown();
    scene.boostDown();
    EXPECT_EQ(scene.boost(), 0) << "减不到负数";

    scene.setBoost(1);
    ASSERT_EQ(scene.boost(), 1);
    ASSERT_TRUE(scene.menuChoose(app_, kBattleMenuAttack));
    const int target = firstEnabled(scene.menuList());
    ASSERT_GE(target, 0);
    const std::size_t hitsBefore = scene.battle().events().size();
    ASSERT_TRUE(scene.menuChoose(app_, target));
    int hits = 0;
    for (std::size_t i = hitsBefore; i < scene.battle().events().size(); ++i) {
        if (scene.battle().events()[i].kind == fanren::core::battle::BattleEventKind::Hit &&
            scene.battle().events()[i].actor == actor) {
            ++hits;
        }
    }
    EXPECT_EQ(hits, 2) << "蓄 1 点劲 = 连击 2 下";
    EXPECT_EQ(scene.boost(), 0) << "出过手蓄劲归零，免得下一位接着用";
}

}  // namespace
