// P3 第 3 章：用毒（契约 docs/interfaces-p3-ch03.md 第 2 节）。
//
// 刻意做小的一套：一个回合数加一个每回合掉血，两条来源（物品与法术）、
// 一条解除（物品）。这里连同「界面上看得见」一起钉住——那一条不是锦上添花：
// 玩家掉血却不知道为什么，一律会被当成 bug。
//
// 判断「看得见」时不写成「找不到某个词就算过」：那种断言在被查的字符串变成空的
// 时候照样通过（docs/README.md 那张表的第二行）。这里先验它有内容，再验两个数
// 都在里面，另配一条没中毒必须是空串的反向用例。
#include <gtest/gtest.h>

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

#include "TempDir.h"
#include "core/battle/Battle.h"
#include "core/model/Types.h"
#include "game/BattleScene.h"
#include "io/DataLoader.h"

namespace {

using fanren::core::Item;
using fanren::core::ItemKind;
using fanren::core::Magic;
using fanren::core::battle::Action;
using fanren::core::battle::ActionKind;
using fanren::core::battle::BattlePhase;
using fanren::core::battle::BattleState;
using fanren::core::battle::Unit;
using fanren::game::poisonStatusText;
using fanren::rules::Realm;

Unit makeUnit(const std::string& name, bool ally, int speed) {
    Unit u;
    u.id = name;
    u.name = name;
    u.hp = u.maxHp = 100;
    u.mp = u.maxMp = 30;
    u.attack = 10;
    u.defence = 5;
    u.speed = speed;
    u.realm = Realm::QiRefining3;
    u.ally = ally;
    return u;
}

// 我方（快）与敌方（慢），我方先手。
BattleState makeDuel() {
    std::vector<Unit> units{makeUnit("hanli", true, 12), makeUnit("moren", false, 8)};
    BattleState state;
    state.setup(std::move(units), 3u);
    return state;
}

Item makePoisonItem() {
    Item item;
    item.id = "poison_probe";
    item.name = "probe-poison";
    item.kind = ItemKind::Pill;
    item.poison = 3;
    item.poisonPower = 6;
    return item;
}

Item makeAntidote() {
    Item item;
    item.id = "antidote_probe";
    item.name = "probe-antidote";
    item.kind = ItemKind::Pill;
    item.curesPoison = true;
    return item;
}

Action useItem(int actor, int target, const std::string& itemId) {
    Action a;
    a.kind = ActionKind::Item;
    a.actorIndex = actor;
    a.targetIndex = target;
    a.magicId = itemId;   // 契约的 Action 没有 itemId，物品借用这个槽位
    return a;
}

// 推到下一回合开始：当前行动者走完，回合结束一次，再 endTurn 才开新回合。
void advanceOneRound(BattleState& state) {
    const int from = state.round();
    for (int guard = 0; guard < 32 && state.round() == from; ++guard) {
        state.endTurn();
    }
}

// ---- 结算 ----

TEST(Ch03Poison, TicksAtRoundStartAndCountsDown) {
    // 直接把毒摆上：来源另有测试，这一条只管结算。
    std::vector<Unit> units{makeUnit("hanli", true, 12), makeUnit("moren", false, 8)};
    const int full = units[1].hp;
    units[1].poison = 3;
    units[1].poisonPower = 6;
    BattleState state;
    state.setup(units, 3u);

    // setup() 里就开了第一回合，所以带毒入场的单位开场即毒发一次。
    // 与眩晕同一个时机，也是玩家看得懂的：回合一开始，毒就发作。
    EXPECT_EQ(state.units()[1].hp, full - 6) << "每回合掉的就是 poisonPower";
    EXPECT_EQ(state.units()[1].poison, 2) << "回合数要递减";

    advanceOneRound(state);
    EXPECT_EQ(state.units()[1].hp, full - 12);
    EXPECT_EQ(state.units()[1].poison, 1);

    advanceOneRound(state);
    EXPECT_EQ(state.units()[1].hp, full - 18);
    EXPECT_EQ(state.units()[1].poison, 0);
    EXPECT_EQ(state.units()[1].poisonPower, 0) << "毒性散尽后强度要一并清掉，不留残影";

    advanceOneRound(state);
    EXPECT_EQ(state.units()[1].hp, full - 18) << "毒散了就不该再掉血";
    advanceOneRound(state);
    EXPECT_EQ(state.units()[1].hp, full - 18) << "三回合就是三回合，不多掉一点";
}

TEST(Ch03Poison, NeverKillsAndLeavesOnePointOfLife) {
    // 口径：毒只削不杀（理由见 BattleState::tickPoison 的注释）。
    // 这条断言必须先验「毒确实在掉血」，否则「打一百回合也没死」在毒根本没生效
    // 的时候照样通过——那正是判据空转最常见的一种长相。
    std::vector<Unit> units{makeUnit("hanli", true, 12), makeUnit("moren", false, 8)};
    units[0].hp = 40;
    units[0].poison = 100;
    units[0].poisonPower = 6;
    BattleState state;
    state.setup(units, 3u);

    advanceOneRound(state);
    ASSERT_LT(state.units()[0].hp, 40) << "先验：毒确实在掉血，下面的断言才有意义";

    for (int i = 0; i < 100; ++i) advanceOneRound(state);
    EXPECT_EQ(state.units()[0].hp, 1) << "毒最多把人削到 1 点气血";
    EXPECT_TRUE(state.units()[0].alive());
    EXPECT_EQ(state.phase(), BattlePhase::Ongoing) << "毒不该在无人行动的回合开始判出胜负";
}

TEST(Ch03Poison, GoesStraightThroughTheGuard) {
    // 守势挡的是外力，毒已经在体内了。反过来的话，「防御一下就不用解毒」
    // 会让解毒药与整条备毒线一起失去意义。
    //
    // ---- 这一条为什么改写（八方旅人化改造：护体罡气 → 防御减半）----
    // 从前这里量的是「毒过不过护体罡气」。罡气那一套（凝一个会被打穿的数）已经换成
    // 防御：从摆开守势起到自己下一次出手之前受伤减半（docs/octopath-battle.md 2.4）。
    // 问题的形状不变：让它**自己摆开守势**，再推到下一回合开头结算毒——那一刻 moren
    // 这一回合还没轮到出手，守势实打实地挂在身上，毒照样掉满，不减半。
    std::vector<Unit> units{makeUnit("hanli", true, 12), makeUnit("moren", false, 8)};
    units[1].poison = 2;
    units[1].poisonPower = 7;
    BattleState state;
    state.setup(units, 3u);

    state.endTurn();   // 韩立让过，轮到 moren
    ASSERT_EQ(state.currentActor(), 1);
    Action defend;
    defend.kind = ActionKind::Defend;
    defend.actorIndex = 1;
    ASSERT_TRUE(state.apply(defend).ok);
    ASSERT_TRUE(state.units()[1].guarding) << "先验：守势真的摆开了，下面那条断言才有东西可测";

    const int hp = state.units()[1].hp;
    state.endTurn();   // moren 之后没人了
    state.endTurn();   // 开新回合：毒在回合开头结算
    ASSERT_EQ(state.round(), 2);
    // 防御给了它「下回合先手」，所以新回合一开它就是行动者、守势随即散去。先验要问的是
    // **结算毒的那一刻**守势还在：事件流里这一回合的毒发排在守势散去之前。
    using fanren::core::battle::BattleEventKind;
    const auto& events = state.events();
    std::size_t tick = events.size();
    std::size_t guardEnd = events.size();
    for (std::size_t i = 0; i < events.size(); ++i) {
        if (events[i].kind == BattleEventKind::PoisonTick && events[i].target == 1) tick = i;
        if (events[i].kind == BattleEventKind::GuardEnd && events[i].actor == 1) guardEnd = i;
    }
    ASSERT_LT(tick, events.size()) << "先验：这一回合确实毒发了";
    ASSERT_LT(tick, guardEnd) << "先验：毒发时守势还挂在身上";
    EXPECT_EQ(state.units()[1].hp, hp - 7) << "守势不该替毒挡：7 点就是 7 点，不是减半的 4 点";
}

TEST(Ch03Poison, WritesALineIntoTheBattleLog) {
    std::vector<Unit> units{makeUnit("hanli", true, 12), makeUnit("moren", false, 8)};
    units[1].poison = 2;
    units[1].poisonPower = 5;
    BattleState state;
    state.setup(units, 3u);

    const std::size_t before = state.log().size();
    advanceOneRound(state);
    ASSERT_GT(state.log().size(), before) << "毒发必须在日志上留一行";
    const std::string& line = state.log().back();
    EXPECT_FALSE(line.empty());
    EXPECT_NE(line.find("moren"), std::string::npos) << "要说清是谁毒发";
    EXPECT_NE(line.find("5"), std::string::npos) << "要说清掉了多少";
}

// ---- 界面 ----

TEST(Ch03Poison, IsVisibleOnScreenWithBothNumbers) {
    Unit u = makeUnit("moren", false, 8);
    u.poison = 3;
    u.poisonPower = 6;

    const std::string text = poisonStatusText(u);
    ASSERT_FALSE(text.empty()) << "中毒必须在界面上看得出来（契约 2.2）";
    EXPECT_NE(text.find("3"), std::string::npos) << "还剩几回合要能看见";
    EXPECT_NE(text.find("6"), std::string::npos) << "每回合掉多少要能看见";
}

TEST(Ch03Poison, ShowsNothingWhenNobodyIsPoisoned) {
    // 反向对照：没中毒时必须是空串。没有这一条，上面那条只要 poisonStatusText
    // 恒返回一句固定的话就能蒙混过关。
    const Unit clean = makeUnit("moren", false, 8);
    EXPECT_TRUE(poisonStatusText(clean).empty());

    Unit expired = clean;
    expired.poison = 0;
    expired.poisonPower = 6;   // 强度没清干净时也不该显示
    EXPECT_TRUE(poisonStatusText(expired).empty());
}

// ---- 来源与解除 ----

TEST(Ch03Poison, AnItemPoisonsTheEnemyItIsThrownAt) {
    BattleState state = makeDuel();
    state.addItem(makePoisonItem());

    auto result = state.apply(useItem(0, 1, "poison_probe"));
    ASSERT_TRUE(result.ok) << result.error;
    EXPECT_EQ(state.units()[1].poison, 3);
    EXPECT_EQ(state.units()[1].poisonPower, 6);
    EXPECT_NE(result.value.find("中毒"), std::string::npos) << "日志要说明发生了什么";
}

TEST(Ch03Poison, APoisonCannotBeUsedOnYourOwnSide) {
    // 负向：毒药是往对手身上使的。判定按数据（item.poison）分流，
    // 而不是按物品 id 写死——毒药的真实条目由编剧写，引擎不该认识它的名字。
    BattleState state = makeDuel();
    state.addItem(makePoisonItem());

    const Action selfPoison = useItem(0, 0, "poison_probe");
    EXPECT_FALSE(state.isLegal(selfPoison));
    auto result = state.apply(selfPoison);
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(state.units()[0].poison, 0) << "非法动作一个字节也不该写";
    EXPECT_FALSE(state.units()[0].acted) << "非法动作不消耗回合";
}

TEST(Ch03Poison, AHealingPillStillCannotBeUsedOnTheEnemy) {
    // 反向对照：分流是按数据走的两条路，不是把闸门整个拆了。
    BattleState state = makeDuel();
    Item pill;
    pill.id = "heal_probe";
    pill.name = "probe-heal";
    pill.restoreHp = 10;
    state.addItem(pill);

    EXPECT_FALSE(state.isLegal(useItem(0, 1, "heal_probe")));
}

TEST(Ch03Poison, AnAntidoteClearsIt) {
    std::vector<Unit> units{makeUnit("hanli", true, 12), makeUnit("moren", false, 8)};
    units[0].poison = 4;
    units[0].poisonPower = 9;
    BattleState state;
    state.setup(units, 3u);
    state.addItem(makeAntidote());
    ASSERT_EQ(state.units()[0].poison, 3) << "开局第一回合就先毒发一次";

    auto result = state.apply(useItem(0, 0, "antidote_probe"));
    ASSERT_TRUE(result.ok) << result.error;
    EXPECT_EQ(state.units()[0].poison, 0);
    EXPECT_EQ(state.units()[0].poisonPower, 0);
    EXPECT_TRUE(poisonStatusText(state.units()[0]).empty()) << "解掉之后界面上也该干净";

    const int hp = state.units()[0].hp;
    advanceOneRound(state);
    EXPECT_EQ(state.units()[0].hp, hp) << "解过毒就不该再掉血";
}

// ---------------------------------------------------------------------------
// 解毒药用在没中毒的人身上：拦下来，并说清楚为什么（第 3 章独立校对 MEDIUM-1）
// ---------------------------------------------------------------------------
// 这一组补的是一处**静默失败**。从前 checkItem 对这件事一条判定都没有，
// applyItem 里解毒那一段写的是 `if (item.curesPoison && target.poison > 0)`，
// 目标没中毒时它什么也不做、末尾拼一句「并无变化」就算成功返回；
// 而 BattleScene::issuePlayerAction 只要 result.ok 就从背包里扣掉一件。
// 于是玩家点一下清毒散，代价是一件东西加一个回合，收获是一句「并无变化」。
//
// 「东西少了却不知道为什么」与本项目反复确认的「掉血却不知道为什么」是同一类
// 缺陷的两面，两条都明令禁止。
TEST(Ch03Poison, AnAntidoteIsRefusedOnSomebodyWhoIsNotPoisoned) {
    BattleState state = makeDuel();
    state.addItem(makeAntidote());
    ASSERT_EQ(state.units()[0].poison, 0) << "先验：他确实没中毒";

    const Action wasted = useItem(0, 0, "antidote_probe");
    std::string why;
    EXPECT_FALSE(state.isLegal(wasted, &why));
    // 只说「用不了」不算数：菜单会把这句话原样写在条目上（refusePlayerAction →
    // buildItemItems），玩家要能据此判断是换目标还是换东西。
    EXPECT_NE(why.find("没有中毒"), std::string::npos) << "拒绝时要说清理由，实得：" << why;
    EXPECT_NE(why.find(state.units()[0].name), std::string::npos) << "要点名是谁，实得：" << why;

    auto result = state.apply(wasted);
    EXPECT_FALSE(result.ok);
    EXPECT_FALSE(state.units()[0].acted) << "非法动作不该消耗回合";
    // 与 checkLegal 同一条口径：判定不过一个字节都不写。背包是 game 层扣的，
    // 而那一步只在 result.ok 时才走——回绝掉，东西就留住了。
    EXPECT_EQ(result.error, why) << "菜单画的禁用理由与回绝时给的那句必须字字相同";
}

TEST(Ch03Poison, TheSameAntidoteIsFineTheMomentThereIsPoisonToClear) {
    // 上一条的另一半。少了它，把 checkItem 改成「解毒药一律不许用」也能全绿，
    // 而那会把暗道那一仗里唯一的解法堵死。
    std::vector<Unit> units{makeUnit("hanli", true, 12), makeUnit("moren", false, 8)};
    units[0].poison = 4;
    units[0].poisonPower = 9;
    BattleState state;
    state.setup(units, 3u);
    state.addItem(makeAntidote());
    ASSERT_GT(state.units()[0].poison, 0) << "先验：这一回他真的中着毒";

    std::string why;
    EXPECT_TRUE(state.isLegal(useItem(0, 0, "antidote_probe"), &why)) << why;
    auto result = state.apply(useItem(0, 0, "antidote_probe"));
    ASSERT_TRUE(result.ok) << result.error;
    EXPECT_EQ(state.units()[0].poison, 0);
}

TEST(Ch03Poison, APillThatAlsoHealsIsNotCaughtByTheAntidoteGate) {
    // **边界：这一条闸只管「纯解毒药」。**
    //
    // 「回血药满血时能不能吃」是另一件事：那一类东西即便回不了血也还有别的效果，
    // 而且那条口径牵动所有回复类物品。判据因此收窄成 curesPoison 为真、
    // 而 restoreHp / restoreMp 一样不沾。这一条钉住那条边界——
    // 没有它，下次有人把判据放宽成「凡 curesPoison 都拦」，
    // 一件又回血又解毒的药会在满血且没中毒时变成不可点，而那是另一个设计决定。
    std::vector<Unit> units{makeUnit("hanli", true, 12), makeUnit("moren", false, 8)};
    units[0].hp = units[0].maxHp;   // 满血，且没中毒：两条都「没用」
    BattleState state;
    state.setup(units, 3u);

    Item both;
    both.id = "tonic_probe";
    both.name = "probe-tonic";
    both.kind = ItemKind::Pill;
    both.restoreHp = 10;
    both.curesPoison = true;
    state.addItem(both);

    ASSERT_EQ(state.units()[0].poison, 0);
    ASSERT_EQ(state.units()[0].hp, state.units()[0].maxHp);
    std::string why;
    EXPECT_TRUE(state.isLegal(useItem(0, 0, "tonic_probe"), &why))
        << "又回血又解毒的药不在这条闸的范围里，实得：" << why;
}

TEST(Ch03Poison, AMagicCanPoisonToo) {
    // 契约第 2.3 节：物品与法术各一条路径。
    BattleState state = makeDuel();
    Magic magic;
    magic.id = "poison_mist";
    magic.name = "probe-mist";
    magic.needMp = 5;
    magic.power = 4;
    magic.poison = 2;
    magic.poisonPower = 8;
    state.addMagic(magic);

    Action cast;
    cast.kind = ActionKind::Cast;
    cast.actorIndex = 0;
    cast.targetIndex = 1;
    cast.magicId = "poison_mist";
    auto result = state.apply(cast);
    ASSERT_TRUE(result.ok) << result.error;
    EXPECT_EQ(state.units()[1].poison, 2);
    EXPECT_EQ(state.units()[1].poisonPower, 8);
}

TEST(Ch03Poison, ReapplyingTakesTheLongerAndTheStrongerInsteadOfStacking) {
    // 叠加等于开了毒理系统的头，而这一章刻意不做那个（契约 2.1）。
    std::vector<Unit> units{makeUnit("hanli", true, 12), makeUnit("moren", false, 8)};
    units[1].poison = 5;
    units[1].poisonPower = 3;
    BattleState state;
    state.setup(units, 3u);
    state.addItem(makePoisonItem());   // 3 回合、每回合 6
    ASSERT_EQ(state.units()[1].poison, 4) << "开场那一回合已经毒发过一次";

    auto result = state.apply(useItem(0, 1, "poison_probe"));
    ASSERT_TRUE(result.ok) << result.error;
    EXPECT_EQ(state.units()[1].poison, 4) << "回合数取长的那个，不相加";
    EXPECT_EQ(state.units()[1].poisonPower, 6) << "强度取高的那个，不相加";
}

// ---- 数据 schema ----

namespace {

void writeFile(const std::filesystem::path& path, const std::string& text) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out << text;
}

}  // namespace

TEST(Ch03PoisonData, LoadsPoisonAndCureFieldsFromItems) {
    fanren::test::TempDir dir("fanren_ch03_poison_data");
    writeFile(dir.path() / "items" / "p.json",
              R"({"id":"probe_poison","name":"probe-poison","kind":"pill",)"
              R"("poison":4,"poisonPower":7})");
    writeFile(dir.path() / "items" / "c.json",
              R"({"id":"probe_cure","name":"probe-cure","kind":"pill","curesPoison":true})");

    auto data = fanren::io::loadGameData(dir.path().string());
    ASSERT_TRUE(data.ok) << data.error;
    const Item* poison = data.value.findItem("probe_poison");
    ASSERT_NE(poison, nullptr);
    EXPECT_EQ(poison->poison, 4);
    EXPECT_EQ(poison->poisonPower, 7);
    EXPECT_FALSE(poison->curesPoison);
    const Item* cure = data.value.findItem("probe_cure");
    ASSERT_NE(cure, nullptr);
    EXPECT_TRUE(cure->curesPoison);
    EXPECT_EQ(cure->poison, 0);
}

TEST(Ch03PoisonData, HalfWrittenPoisonIsRefused) {
    // 负向：只写了回合数没写强度的毒药会被正常用掉、正常扣掉一件，唯独不中毒，
    // 而症状是「这药好像没用」。这一条要求它在加载时就炸出来。
    fanren::test::TempDir dir("fanren_ch03_poison_half");
    writeFile(dir.path() / "items" / "p.json",
              R"({"id":"probe_half","name":"probe-half","kind":"pill","poison":4})");

    auto data = fanren::io::loadGameData(dir.path().string());
    EXPECT_FALSE(data.ok) << "写了一半的毒必须被挡下";
}

TEST(Ch03PoisonData, NegativePoisonIsRefused) {
    fanren::test::TempDir dir("fanren_ch03_poison_neg");
    writeFile(dir.path() / "items" / "p.json",
              R"({"id":"probe_neg","name":"probe-neg","kind":"pill",)"
              R"("poison":-4,"poisonPower":3})");

    auto data = fanren::io::loadGameData(dir.path().string());
    EXPECT_FALSE(data.ok) << "负数不该被夹成 0 悄悄收下";
}

TEST(Ch03PoisonData, AnItemThatBothPoisonsAndCuresIsRefused) {
    fanren::test::TempDir dir("fanren_ch03_poison_both");
    writeFile(dir.path() / "items" / "p.json",
              R"({"id":"probe_both","name":"probe-both","kind":"pill",)"
              R"("poison":2,"poisonPower":2,"curesPoison":true})");

    auto data = fanren::io::loadGameData(dir.path().string());
    EXPECT_FALSE(data.ok) << "既下毒又解毒多半是把两份数据抄混了";
}

TEST(Ch03PoisonData, HalfWrittenPoisonOnAMagicIsRefused) {
    fanren::test::TempDir dir("fanren_ch03_poison_magic");
    writeFile(dir.path() / "magics" / "m.json",
              R"({"id":"probe_mist","name":"probe-mist","poisonPower":5})");

    auto data = fanren::io::loadGameData(dir.path().string());
    EXPECT_FALSE(data.ok) << "法术那边是同一套口径";
}

TEST(Ch03PoisonData, ShippedPoisonAndAntidoteAreLoadable) {
    // 引擎方加的那两条真数据（data/items/pills/）也要能被加载器认下来：
    // schema 与真实文件对不上的话，上面那些临时夹具全绿也说明不了什么。
    namespace fs = std::filesystem;
    std::string root = ".";
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "data" / "items" / "pills")) {
            root = candidate;
            break;
        }
    }
    auto data = fanren::io::loadGameData(std::string(root) + "/data");
    ASSERT_TRUE(data.ok) << data.error;

    const Item* poison = data.value.findItem("pill_shixin_san");
    ASSERT_NE(poison, nullptr) << "data/items/pills/shixin_san.json 没被加载";
    EXPECT_GT(poison->poison, 0);
    EXPECT_GT(poison->poisonPower, 0);

    const Item* cure = data.value.findItem("pill_qingdu_san");
    ASSERT_NE(cure, nullptr) << "data/items/pills/qingdu_san.json 没被加载";
    EXPECT_TRUE(cure->curesPoison);
}

}  // namespace
