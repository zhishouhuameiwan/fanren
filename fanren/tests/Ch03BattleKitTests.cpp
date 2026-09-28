// 战斗开场时到底登记了什么 —— 法术与物品。
//
// 这一整个文件是为一个缺口写的：`BattleState::addMagic` / `addItem` 在产品代码里
// **一次都没被调用过**，只有单测在调。于是 `checkCast` 的第一句 `findMagic` 必然
// 落空，每一场战斗里施法都被拒；战斗中用物品同理。后果有三层：
//
//   1. `ActionKind::Cast` 与 `ActionKind::Item` 在真实游戏里是死代码；
//   2. 刚做好的用毒系统在真实游戏里用不上 —— 契约写明毒的来源是物品与法术，
//      而这两条路在战斗里都走不通，它只在手搭的夹具里跑过；
//   3. `docs/ch03-design.md` 第 2 节要求教学战二教「法术与物品、用毒」，做不到。
//
// 顺带还盖住一件容易被忽略的事：契约 docs/interfaces-p3-ch03.md 3.5 节写着
// 「识海里没有法术、没有物品」，而 `buildFromSetup` 自陈靠「不登记」实现。
// 这句话此前是真的，但真得没有道理 —— 当时**所有**战斗都不登记，识海只是碰巧
// 和别人一样。碰巧成立的东西会在别处一变就塌，所以这里连同那条例外一起钉住。
//
// 每一条都配反向：登记过的能用 / 没登记的不能用；背包里有的登记 / 没有的不登记；
// 普通战斗登记 / 识海一件都不登记。只写一边的话，把整段登记删掉也能全绿。
#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

#include "core/battle/Battle.h"
#include "core/model/Types.h"
#include "game/Application.h"
#include "game/BattleScene.h"

namespace {

using fanren::core::Item;
using fanren::core::Magic;
using fanren::core::battle::Action;
using fanren::core::battle::ActionKind;
using fanren::core::battle::BattleState;
using fanren::core::battle::Unit;
using fanren::game::BattleScene;
using fanren::game::kMindTerrain;
using fanren::game::poisonStatusText;
using fanren::rules::Realm;

// ---------------------------------------------------------------------------
// 一、core 侧的口径：magics 留空到底是「全会」还是「一门都不会」
//
// 「留空 = 战场登记的法术都会」这条老口径在登记表恒为空的时候没有代价 ——
// 全会一场空。登记表一旦真的有东西，它就会反过来决定杂兵能放什么：同场只要有人
// 挂着一条法术，所有 magics 留空又有法力的单位都跟着会了。data 里真有这种编成
// （b11_ningcuidao_youyao：maxMp 120 却一条法术没声明的三级海兽，和会海潮诀的
// 四级海兽同场）。Unit::magicsExhaustive 就是为这件事加的，game 层一律置位。
// ---------------------------------------------------------------------------

Unit makeUnit(const std::string& name, bool ally, int speed) {
    Unit u;
    u.id = name;
    u.name = name;
    u.hp = u.maxHp = 100;
    u.mp = u.maxMp = 50;
    u.attack = 10;
    u.defence = 5;
    u.speed = speed;
    u.realm = Realm::QiRefining3;
    u.ally = ally;
    return u;
}

Magic probeMagic() {
    Magic magic;
    magic.id = "magic_probe";
    magic.name = "试探术";
    magic.needMp = 1;
    magic.power = 5;
    magic.needRealm = Realm::QiRefining1;
    return magic;
}

// 我方（快，先手）与敌方。caster 的两个字段由调用方决定。
BattleState makeCastDuel(const std::vector<std::string>& known, bool exhaustive, bool registerIt) {
    Unit caster = makeUnit("caster", true, 12);
    caster.magics = known;
    caster.magicsExhaustive = exhaustive;
    std::vector<Unit> units{std::move(caster), makeUnit("target", false, 8)};
    BattleState state;
    state.setup(std::move(units), 5u);
    if (registerIt) state.addMagic(probeMagic());
    return state;
}

Action castAt(int actor, int target, const std::string& magicId) {
    Action action;
    action.kind = ActionKind::Cast;
    action.actorIndex = actor;
    action.targetIndex = target;
    action.magicId = magicId;
    return action;
}

TEST(Ch03BattleKit, AnUnregisteredMagicIsRefusedForNotExistingAtAll) {
    // 这就是修复之前**每一场**战斗的样子：登记表空着，施法一律卡在第一句。
    // 下面那条识海的断言要靠这句话的措辞来分辨「没登记」与「没学会」。
    BattleState state = makeCastDuel({}, /*exhaustive=*/false, /*registerIt=*/false);
    ASSERT_EQ(state.currentActor(), 0);
    const auto result = state.apply(castAt(0, 1, "magic_probe"));
    EXPECT_FALSE(result.ok);
    EXPECT_NE(result.error.find("没有这门法术"), std::string::npos) << result.error;
}

TEST(Ch03BattleKit, ALegacyUnitWithAnEmptyListKnowsEverythingRegistered) {
    // 老口径原样保留：手搭的夹具与兜底遭遇照旧少写一行。
    BattleState state = makeCastDuel({}, /*exhaustive=*/false, /*registerIt=*/true);
    ASSERT_EQ(state.currentActor(), 0);
    const auto result = state.apply(castAt(0, 1, "magic_probe"));
    EXPECT_TRUE(result.ok) << result.error;
}

TEST(Ch03BattleKit, AnExhaustiveUnitWithAnEmptyListKnowsNothing) {
    // 反向：同一个局面，只多置一位，同一条法术就放不出来了。
    BattleState state = makeCastDuel({}, /*exhaustive=*/true, /*registerIt=*/true);
    ASSERT_EQ(state.currentActor(), 0);
    const auto result = state.apply(castAt(0, 1, "magic_probe"));
    EXPECT_FALSE(result.ok);
    EXPECT_NE(result.error.find("未习得"), std::string::npos) << result.error;
}

TEST(Ch03BattleKit, AnExhaustiveUnitStillCastsWhatItDeclared) {
    // 另一半：置了位不等于谁都不会 —— 声明过的照旧放得出来。
    BattleState state = makeCastDuel({"magic_probe"}, /*exhaustive=*/true, /*registerIt=*/true);
    ASSERT_EQ(state.currentActor(), 0);
    const auto result = state.apply(castAt(0, 1, "magic_probe"));
    EXPECT_TRUE(result.ok) << result.error;
}

TEST(Ch03BattleKit, ADeclaredListStillLocksOutWhatIsNotOnIt) {
    BattleState state = makeCastDuel({"magic_something_else"}, /*exhaustive=*/true,
                                     /*registerIt=*/true);
    ASSERT_EQ(state.currentActor(), 0);
    const auto result = state.apply(castAt(0, 1, "magic_probe"));
    EXPECT_FALSE(result.ok);
    EXPECT_NE(result.error.find("未习得"), std::string::npos) << result.error;
}

// ---------------------------------------------------------------------------
// 二、真数据：BattleScene 建场时登记了什么
// ---------------------------------------------------------------------------

class BattleKitTest : public ::testing::Test {
protected:
    static std::string assetRoot() {
        namespace fs = std::filesystem;
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
        // 韩立开局只有 10 点气血，两只长虫兽一个照面就能把他放倒，那样后面几条
        // 就成了「战斗早结束了」而不是被测的东西。给他一副够用的身板。
        app_.state().maxHp = 200;
        app_.state().hp = 200;
        app_.state().maxMp = 60;
        app_.state().mp = 60;
    }
    void TearDown() override { app_.shutdown(); }

    fanren::core::GameState& state() { return app_.state(); }

    // 场上第一个还活着的敌人。
    //
    // 战棋那几年这里是「离得最近的那一个」：物品有使用距离（技术债 G-7），第一个
    // 多半正好站得最远。横版没有距离，又回到最朴素的那一个。
    static int firstFoe(const BattleState& battle, int actorIndex) {
        const auto& units = battle.units();
        const bool side = units[static_cast<std::size_t>(actorIndex)].ally;
        for (std::size_t i = 0; i < units.size(); ++i) {
            if (units[i].ally != side && units[i].alive()) return static_cast<int>(i);
        }
        return -1;
    }

    static bool logHas(const BattleState& battle, const std::string& needle) {
        return std::any_of(battle.log().begin(), battle.log().end(),
                           [&needle](const std::string& line) {
                               return line.find(needle) != std::string::npos;
                           });
    }

    fanren::game::Application app_;
};

// ---- 法术的登记范围 ----

TEST_F(BattleKitTest, ABattleRegistersExactlyWhatItsUnitsDeclare) {
    // 先验判据：被查的那两件事在 data 里确实成立。少了这一段，下面两条断言
    // 在「角色没了法术」或「data/magics 空了」的时候会一起变绿。
    const fanren::core::RoleTemplate* jiang = app_.data().findRole("jiang_shou");
    ASSERT_NE(jiang, nullptr);
    ASSERT_FALSE(jiang->magics.empty()) << "僵兽本来就该挂着尸毒爪，不然这条在测空气";
    ASSERT_NE(app_.data().findMagic("magic_huoqiu_shu"), nullptr)
        << "火球术在 data/magics 里确实存在 —— 下面那条「没登记」才有意义";

    BattleScene scene("b03_andao_shishou");
    scene.onEnter(app_);

    EXPECT_TRUE(scene.battle().hasMagic("magic_shidu_zhua")) << "场上有人声明了它，就该能用";
    EXPECT_FALSE(scene.battle().hasMagic("magic_huoqiu_shu"))
        << "这一场没人声明火球术，登记的不是整张 data/magics 表";
    EXPECT_EQ(scene.battle().registeredMagicCount(), 1u) << "登记范围恰好是场上声明的并集";
}

TEST_F(BattleKitTest, ABattleWhereNobodyDeclaresAMagicRegistersNone) {
    // 反向对照：登记范围由编成决定，不是「凡开战就把法术表铺开」。
    const fanren::core::RoleTemplate* wolf = app_.data().findRole("wild_wolf");
    ASSERT_NE(wolf, nullptr);
    ASSERT_TRUE(wolf->magics.empty()) << "饿狼本来就不会法术，不然这条在测别的东西";

    BattleScene scene("b03_gu_wai_elang");
    scene.onEnter(app_);
    EXPECT_EQ(scene.battle().registeredMagicCount(), 0u);
}

TEST_F(BattleKitTest, EveryUnitOnTheFieldIsPinnedToItsOwnSpellList) {
    // 只收窄登记范围救不了杂兵：并集里仍然有别人的法术。真正把每个单位钉在
    // 自己那份清单上的是这一位，game 层对每个单位都置了它。
    BattleScene scene("b03_andao_shishou");
    scene.onEnter(app_);

    int emptyListed = 0;
    for (const Unit& unit : scene.battle().units()) {
        EXPECT_TRUE(unit.magicsExhaustive) << unit.id << " 的法术清单必须是穷尽的";
        if (unit.magics.empty()) ++emptyListed;
    }
    // 先验：这一场确实有「一条法术都没声明」的单位（韩立与两只长虫兽），
    // 否则上面那圈断言等于没说 —— 穷尽与否只在清单为空时才有分别。
    EXPECT_GE(emptyListed, 2) << "这一场本来就该有几个一条法术都不会的";
}

// ---- 物品的登记范围：数据驱动，game 层不认识任何物品 id ----

TEST_F(BattleKitTest, TheBagAndTheDataTogetherDecideWhatCanBeUsed) {
    const Item* pill = app_.data().findItem("pill_jinchuang_yao");
    const Item* stone = app_.data().findItem("material_lingshi");
    const Item* poison = app_.data().findItem("pill_shixin_san");
    ASSERT_NE(pill, nullptr);
    ASSERT_NE(stone, nullptr);
    ASSERT_NE(poison, nullptr);
    // 先验：判据本身在这三件东西上分得开。三个都是 true 或都是 false 的话，
    // 底下那三条断言里必有一条永远成立。
    ASSERT_TRUE(fanren::core::battleUsable(*pill)) << "金疮药回气血，战斗里当然有用";
    ASSERT_TRUE(fanren::core::battleUsable(*poison)) << "蚀心散下毒";
    ASSERT_FALSE(fanren::core::battleUsable(*stone)) << "灵石在战斗里什么也做不了";

    state().addItem("pill_jinchuang_yao", 1);
    state().addItem("material_lingshi", 5);
    // 蚀心散**不放进背包**：它战斗里有用，但玩家手上没有。

    BattleScene scene("b03_gu_wai_elang");
    scene.onEnter(app_);

    EXPECT_TRUE(scene.battle().hasItem("pill_jinchuang_yao")) << "背包里有、战斗里有用";
    EXPECT_FALSE(scene.battle().hasItem("material_lingshi")) << "背包里有，但战斗里没用";
    EXPECT_FALSE(scene.battle().hasItem("pill_shixin_san")) << "战斗里有用，但背包里没有";
    EXPECT_EQ(scene.battle().registeredItemCount(), 1u);
}

TEST_F(BattleKitTest, UsingAPillInBattleReallyTakesItOutOfTheBag) {
    state().maxHp = 200;
    state().hp = 40;                       // 留出回血的余地
    state().addItem("pill_jinchuang_yao", 2);

    BattleScene scene("b03_gu_wai_elang");
    scene.onEnter(app_);
    ASSERT_TRUE(scene.battle().hasItem("pill_jinchuang_yao"));

    // 落在真实行动序上：韩立身法写死是 5，两只饿狼是 9，他本来就不是先手。
    const int actor = scene.runToAllyTurn();
    ASSERT_GE(actor, 0) << "轮不到我方行动，这一场的编成变了";
    const int hpBefore = scene.battle().units()[static_cast<std::size_t>(actor)].hp;

    Action use;
    use.kind = ActionKind::Item;
    use.actorIndex = actor;
    use.targetIndex = actor;
    use.magicId = "pill_jinchuang_yao";
    const auto result = scene.issuePlayerAction(app_, use);
    ASSERT_TRUE(result.ok) << result.error;

    EXPECT_GT(scene.battle().units()[static_cast<std::size_t>(actor)].hp, hpBefore)
        << "药得真的回血";
    EXPECT_EQ(state().itemCount("pill_jinchuang_yao"), 1) << "用掉一颗，背包就该少一颗";
}

TEST_F(BattleKitTest, TheLastPillCannotBeUsedTwice) {
    // 反向：登记表是开战那一刻的快照，最后一颗用完之后它还留在表里。
    // 不在 game 层拦一道，同一颗药能被用到天荒地老。
    state().maxHp = 200;
    state().hp = 40;
    state().addItem("pill_jinchuang_yao", 1);

    BattleScene scene("b03_gu_wai_elang");
    scene.onEnter(app_);

    const int actor = scene.runToAllyTurn();
    ASSERT_GE(actor, 0);
    Action use;
    use.kind = ActionKind::Item;
    use.actorIndex = actor;
    use.targetIndex = actor;
    use.magicId = "pill_jinchuang_yao";
    ASSERT_TRUE(scene.issuePlayerAction(app_, use).ok);
    ASSERT_EQ(state().itemCount("pill_jinchuang_yao"), 0);

    // 再用一次。这道拦截在 game 层，排在 core 的一切判定之前，所以被回绝的
    // 理由**必须是背包**，而不是「本回合已经行动过」—— 两者都会让动作失败，
    // 但只有前者说明这道拦截真的在。拦截若被删掉，这一句立刻转红。
    const int hpBefore = scene.battle().units()[static_cast<std::size_t>(actor)].hp;
    const auto again = scene.issuePlayerAction(app_, use);
    EXPECT_FALSE(again.ok) << "背包空了还能再用一次";
    EXPECT_NE(again.error.find("背包"), std::string::npos) << again.error;
    EXPECT_EQ(scene.battle().units()[static_cast<std::size_t>(actor)].hp, hpBefore)
        << "被拒的动作一个字节都不该改";
}

// ---- 用毒：走完整条真实战斗流程，不是手搭的夹具 ----

TEST_F(BattleKitTest, PoisonFromTheBagWorksInARealBattle) {
    const Item* poison = app_.data().findItem("pill_shixin_san");
    ASSERT_NE(poison, nullptr);
    ASSERT_GT(poison->poison, 0);
    ASSERT_GT(poison->poisonPower, 0);
    state().addItem("pill_shixin_san", 1);

    BattleScene scene("b03_andao_shishou");
    scene.onEnter(app_);
    ASSERT_TRUE(scene.battle().hasItem("pill_shixin_san"));

    // 从前（战棋）这里要先靠「防御」过回合、等笼中兽扑到三格之内（蚀心散的 useRange 3）。
    // 横版没有距离：轮到我方就撒。本条测的仍是「一包真背包里的毒，在真实战斗流程里能把人毒倒」。
    const int actor = scene.runToAllyTurn();
    ASSERT_GE(actor, 0) << "轮不到我方行动，这一场的编成变了";
    const int foe = firstFoe(scene.battle(), actor);
    ASSERT_GE(foe, 0);
    ASSERT_EQ(scene.battle().units()[static_cast<std::size_t>(foe)].poison, 0)
        << "下毒之前它不该已经中毒";

    Action throwIt;
    throwIt.kind = ActionKind::Item;
    throwIt.actorIndex = actor;
    throwIt.targetIndex = foe;
    throwIt.magicId = "pill_shixin_san";
    const auto result = scene.issuePlayerAction(app_, throwIt);
    ASSERT_TRUE(result.ok) << result.error;
    EXPECT_EQ(state().itemCount("pill_shixin_san"), 0);

    const Unit& poisoned = scene.battle().units()[static_cast<std::size_t>(foe)];
    EXPECT_EQ(poisoned.poison, poison->poison);
    EXPECT_EQ(poisoned.poisonPower, poison->poisonPower);
    // 界面上看得见：先验它真有内容，再验两个数都在里面。
    const std::string shown = poisonStatusText(poisoned);
    ASSERT_FALSE(shown.empty());
    EXPECT_NE(shown.find(std::to_string(poison->poison)), std::string::npos) << shown;
    EXPECT_NE(shown.find(std::to_string(poison->poisonPower)), std::string::npos) << shown;

    // 接着把这一场打完：毒必须在真实的回合推进里结算，而不是只挂个状态。
    scene.runToCompletion(200);
    EXPECT_TRUE(logHas(scene.battle(), "毒发")) << "整场打下来没有一次毒发";
}

TEST_F(BattleKitTest, WithoutThePowderNobodyPoisonsTheBeast) {
    // 反向对照：上一条那句「僵兽 毒发」是玩家撒的粉带来的，
    // 不是这一场自带的。
    //
    // 原来这条问的是「全场一次毒发也没有」，前提写着「尸毒爪射程 1、
    // AI 永远先挑近战，所以它自己不会放」。那个前提早就不成立了：先是 C3-1 把爪子
    // 改成射程 2，如今横版的敌方 AI 是「有伤人的法术且法力够就先放法术」，
    // 爪子开场第一手就放——玩家一定中毒，清毒散在同一场里就用得上。
    //
    // 所以收窄到它本来就想问的那一件：没人向**僵兽**撒粉，僵兽就不该毒发。
    BattleScene scene("b03_andao_shishou");
    scene.onEnter(app_);
    ASSERT_EQ(scene.battle().registeredItemCount(), 0u) << "背包空着，一件也不该登记";

    std::string beastName;
    for (const Unit& u : scene.battle().units()) {
        if (!u.ally) { beastName = u.name; break; }
    }
    ASSERT_FALSE(beastName.empty()) << "场上找不到敌人，下面那句 EXPECT_FALSE 会成为空转";

    scene.runToCompletion(200);

    EXPECT_FALSE(logHas(scene.battle(), beastName + " 毒发"))
        << "没人向它撒粉，僵兽却毒发了";
    // 先验：这把尺子咬得动。没有它，上一句在「毒发」这三个字压根儿
    // 沿不出来的情形下照样通过。现在爪子放得出来，韩立必然中毒。
    EXPECT_TRUE(logHas(scene.battle(), "毒发"))
        << "全场一次毒发也没有，那上一句的 EXPECT_FALSE 证明不了任何事";
}

// ---- 识海：一件都登记不上 ----

TEST_F(BattleKitTest, TheMindBattleRegistersNothingAtAll) {
    // 先验判据，三条缺一不可，否则「一件都没有」可能只是因为本来就没东西可登记：
    //   · 两团光球的角色确实挂着法术；
    //   · 那些法术在 data/magics 里确实存在；
    //   · 背包里确实有战斗中用得上的药。
    const fanren::core::RoleTemplate* soul = app_.data().findRole("mo_juren_yuanshen");
    ASSERT_NE(soul, nullptr);
    ASSERT_FALSE(soul->magics.empty()) << "墨居仁元神本来就挂着两条法术";
    for (const std::string& id : soul->magics) {
        ASSERT_NE(app_.data().findMagic(id), nullptr) << id << " 在 data/magics 里查不到";
    }
    state().addItem("pill_jinchuang_yao", 3);
    state().addItem("pill_qingdu_san", 2);
    ASSERT_TRUE(fanren::core::battleUsable(*app_.data().findItem("pill_jinchuang_yao")));

    const fanren::core::BattleSetup* setup = app_.battleSetup("b03_shihai_duoshe");
    ASSERT_NE(setup, nullptr);
    ASSERT_EQ(setup->terrain, kMindTerrain);

    BattleScene mind("b03_shihai_duoshe");
    mind.onEnter(app_);
    EXPECT_EQ(mind.battle().registeredMagicCount(), 0u) << "识海里没有法术（契约 3.5）";
    EXPECT_EQ(mind.battle().registeredItemCount(), 0u) << "识海里没有物品（契约 3.5）";

    // 对照：同一份背包、同一段登记代码，换一场普通战斗就登记得上。
    // 没有这一条，上面两句在「登记整个坏掉了」的时候照样全绿。
    BattleScene ordinary("b03_gu_wai_elang");
    ordinary.onEnter(app_);
    EXPECT_GT(ordinary.battle().registeredItemCount(), 0u);
}

TEST_F(BattleKitTest, TheSoulCannotThrowAFireballInsideTheMind) {
    BattleScene mind("b03_shihai_duoshe");
    mind.onEnter(app_);
    const BattleState& battle = mind.battle();
    ASSERT_TRUE(battle.devourMode());

    int soul = -1;
    for (std::size_t i = 0; i < battle.units().size(); ++i) {
        if (battle.units()[i].id == "mo_juren_yuanshen") soul = static_cast<int>(i);
    }
    ASSERT_GE(soul, 0);
    const Unit& caster = battle.units()[static_cast<std::size_t>(soul)];
    const Unit& hero = battle.units()[0];
    ASSERT_TRUE(hero.ally);

    const Magic* fireball = app_.data().findMagic("magic_huoqiu_shu");
    ASSERT_NE(fireball, nullptr);

    // 先验：除了「没登记」，这一发火球该满足的条件一个不缺 —— 轮到它、
    // 法力够、境界够、清单里确实有这一条。少了这一段，下面那句 EXPECT_FALSE
    // 可能只是因为别的哪一条不对，而那种绿灯在把例外删掉之后照样是绿的。
    ASSERT_EQ(battle.currentActor(), soul) << "元神身法最高，开场就该轮到它";
    ASSERT_GE(caster.mp, fireball->needMp) << "法力本身够用";
    ASSERT_NE(std::find(caster.magics.begin(), caster.magics.end(), "magic_huoqiu_shu"),
              caster.magics.end())
        << "它自己那份清单里确实写着火球术";

    EXPECT_FALSE(battle.isLegal(castAt(soul, 0, "magic_huoqiu_shu")))
        << "识海里放得出火球，这一章的失重感就没了";

    // 把「识海的例外」拿掉是什么样：同样两个单位、同样的局面，只多登记一条
    // 火球术，这一发立刻就合法。这条是上面那句的分母 —— 它证明「非法」的原因
    // 只可能是没登记，而不是别的什么。
    std::vector<Unit> copy = battle.units();
    BattleState asIfRegistered;
    asIfRegistered.setup(std::move(copy), 1u);
    asIfRegistered.setDevourMode(true);
    asIfRegistered.addMagic(*fireball);
    ASSERT_EQ(asIfRegistered.currentActor(), soul);
    EXPECT_TRUE(asIfRegistered.isLegal(castAt(soul, 0, "magic_huoqiu_shu")))
        << "只多登记一条就该合法 —— 否则上面那条 EXPECT_FALSE 什么也没证明";
}

TEST_F(BattleKitTest, NobodyCanUseAnItemInsideTheMindEither) {
    state().addItem("pill_jinchuang_yao", 3);
    BattleScene mind("b03_shihai_duoshe");
    mind.onEnter(app_);

    const int actor = mind.runToAllyTurn();
    ASSERT_GE(actor, 0);
    Action use;
    use.kind = ActionKind::Item;
    use.actorIndex = actor;
    use.targetIndex = actor;
    use.magicId = "pill_jinchuang_yao";
    const auto result = mind.issuePlayerAction(app_, use);
    EXPECT_FALSE(result.ok) << "识海里丹药不该管用";
    EXPECT_NE(result.error.find("没有这件物品"), std::string::npos) << result.error;
    EXPECT_EQ(state().itemCount("pill_jinchuang_yao"), 3) << "被拒的动作不该动背包";
}

// ---- 施法在真实战斗流程里确实发生 ----

TEST_F(BattleKitTest, AnEnemyReallyCastsInARealBattle) {
    // 登记只是第一步，「在真实的一局里真的放出来过」才是这个缺口的全貌。
    // 陆师兄会火球术，敌方 AI 法力够就先放法术。
    BattleScene scene("b07_lu_shixiong");
    scene.onEnter(app_);
    ASSERT_GT(scene.battle().registeredMagicCount(), 0u);
    scene.runToCompletion(200);
    EXPECT_TRUE(logHas(scene.battle(), "施展")) << "整场打下来一次法术都没放出来";
}

TEST_F(BattleKitTest, ABattleWithNoSpellcasterNeverCasts) {
    // 反向对照：上面那条不是「日志里总会有『施展』两个字」。
    BattleScene scene("b03_gu_wai_elang");
    scene.onEnter(app_);
    ASSERT_EQ(scene.battle().registeredMagicCount(), 0u);
    scene.runToCompletion(200);
    EXPECT_FALSE(logHas(scene.battle(), "施展"));
}

}  // namespace
