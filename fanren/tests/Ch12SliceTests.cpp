// 第 12 章验收第 5、6、8、9、10、11、12（神雷，整改第 1 轮补）、15、16 条的「切片」那一半（扫脚本 / 读数据那一半在 Ch12TriggerMode、
// Ch12BattleData、Ch12Ledger 里，测试路 A 交付）。每条写明起点存档：一律是 tests/fixtures/ch11-end-first.sav
//（T1 重生成版），由 Ch12Driver 走真实入口推到所需旗标，再在用例里写明扰动；每条都配一组对照
//（扰动前后、两版分支），证明断言不是恒真。
#include <algorithm>
#include <utility>
#include "Ch12Driver.h"
#include "core/model/AttackCategory.h"
#include "game/DialogueScene.h"
#include "script/Command.h"

namespace {
using namespace fanren;
using namespace fanren::test::ch12;

void dropAll(GameState& state, const char* id) {
    const int have = state.itemCount(id);
    if (have > 0) ASSERT_TRUE(state.removeItem(id, have)) << id;
}

void setCount(GameState& state, const char* id, int wanted) {
    const int have = state.itemCount(id);
    if (have > wanted) ASSERT_TRUE(state.removeItem(id, have - wanted)) << id;
    if (have < wanted) state.addItem(id, wanted - have);
}

bool dialogueOnTop(Driver& driver) { return dynamic_cast<game::DialogueScene*>(driver.app().topScene()) != nullptr; }

// ---- 经真菜单出一手（验收 12 神雷那半句用；行号一律取场景自己的 build*Items，与 Ch03TutorialBattle 同法）----
using core::battle::Action;
using core::battle::ActionKind;
using core::battle::BattleEvent;
using core::battle::BattleEventKind;

int unitIndexOf(const BattleScene& scene, const std::string& id) {
    const auto& units = scene.battle().units();
    for (std::size_t i = 0; i < units.size(); ++i)
        if (units[i].id == id) return static_cast<int>(i);
    return -1;
}

int targetRowOf(Driver& driver, const BattleScene& scene, const Action& shape, int target) {
    std::vector<int> indices;
    static_cast<void>(BattleScene::buildTargetItems(driver.state(), scene.battle(), shape, indices));
    for (std::size_t i = 0; i < indices.size(); ++i)
        if (indices[i] == target) return static_cast<int>(i);
    return -1;
}

// 「物品 → itemId → target」。成功返回空串，否则返回卡在哪一级。
std::string throwByMenu(Driver& driver, BattleScene& scene, int actor, const char* itemId, int target) {
    Application& app = driver.app();
    scene.openMenu(app);
    if (scene.menuMode() != game::BattleMenuMode::Root) return "the battle menu did not open";
    if (!scene.menuChoose(app, game::kBattleMenuItem)) return "the item row is greyed: " + scene.feedback();
    std::vector<std::string> ids;
    static_cast<void>(BattleScene::buildItemItems(app.data(), app.state(), scene.battle(), actor, ids));
    const auto found = std::find(ids.begin(), ids.end(), itemId);
    if (found == ids.end()) return std::string(itemId) + " is not on the item menu";
    if (!scene.menuChoose(app, static_cast<int>(found - ids.begin()))) return std::string(itemId) + " is greyed";
    Action shape;
    shape.kind = ActionKind::Item;
    shape.actorIndex = actor;
    shape.magicId = itemId;
    const int row = targetRowOf(driver, scene, shape, target);
    if (row < 0 || !scene.menuChoose(app, row)) return "the target row refused the item: " + scene.feedback();
    return {};
}

// 「攻击 →（兵刃不止一样时）weapon → target」。
std::string strikeByMenu(Driver& driver, BattleScene& scene, int actor, int weapon, int target) {
    Application& app = driver.app();
    scene.openMenu(app);
    if (!scene.menuChoose(app, game::kBattleMenuAttack)) return "the attack row is greyed: " + scene.feedback();
    if (scene.menuMode() == game::BattleMenuMode::Weapon) {
        std::vector<int> categories;
        static_cast<void>(BattleScene::buildWeaponItems(scene.battle(), actor, categories));
        const auto found = std::find(categories.begin(), categories.end(), weapon);
        if (found == categories.end()) return "the hero has no such weapon";
        if (!scene.menuChoose(app, static_cast<int>(found - categories.begin()))) return "the weapon row is greyed";
    }
    Action shape;
    shape.kind = ActionKind::Attack;
    shape.actorIndex = actor;
    shape.category = weapon;
    const int row = targetRowOf(driver, scene, shape, target);
    if (row < 0 || !scene.menuChoose(app, row)) return "the target row refused the strike: " + scene.feedback();
    return {};
}

// events()[from] 起 actor 打在 target 身上的第一击；没有就是 nullptr。
const BattleEvent* firstHit(const BattleScene& scene, std::size_t from, int actor, int target) {
    const auto& events = scene.battle().events();
    for (std::size_t i = from; i < events.size(); ++i)
        if (events[i].kind == BattleEventKind::Hit && events[i].actor == actor && events[i].target == target) return &events[i];
    return nullptr;
}

// 验收 5。起点 ch11-end-first.sav：走到 ch12.rudian（大厅打坐设施）与 ch12.butian（寒骊台台下打坐设施）各开一次面板。
// 扰动：修为硬补到 5000（结丹中期所需），证明拦人的是剧情上限、不是修为不够。
TEST(Ch12Slice, FiveThePanelAsksFiveThousandForCoreMidAndTheStoryCapRefusesThePush) {
    const Walk& walk = intentWalk(kFirst);
    ASSERT_TRUE(walk.problem.empty()) << walk.problem;
    const auto& words = game::cultivationLexicon(game::PanelStage::Immortal);
    for (const auto& [node, facility] : {std::pair{8, "facility_dazuo_dadian"}, std::pair{30, "facility_dazuo_hanlitai"}}) {
        SCOPED_TRACE(facility);
        Driver driver;
        ASSERT_TRUE(driver.resumeReached(reachedAfter(walk, node), node + 1)) << driver.problem();
        EXPECT_EQ(rules::toValue(driver.state().realm), 31);
        EXPECT_EQ(rules::toValue(driver.state().realmCap), 31);
        // 面板修为条写「x / 5000」，下一关是结丹中期。
        EXPECT_EQ(rules::cultivationNeeded(driver.state().realm), 5000);
        rules::Realm next{};
        ASSERT_TRUE(rules::tryNext(driver.state().realm, next));
        EXPECT_EQ(game::realmText(game::PanelStage::Immortal, next), "结丹中期");
        EXPECT_LT(driver.state().cultivation, 5000);
        driver.state().cultivation = 5000;
        ASSERT_TRUE(driver.open(facility)) << driver.problem();
        auto* panel = dynamic_cast<CultivationScene*>(driver.app().topScene());
        ASSERT_NE(panel, nullptr);
        const auto rows = CultivationScene::buildMainItems(driver.state());
        ASSERT_GE(rows.size(), 2u);
        EXPECT_FALSE(rows[1].enabled);
        EXPECT_EQ(rows[1].disabledReason, std::string(words.pushAtStoryCap));
        const auto before = driver.state();
        EXPECT_EQ(panel->breakthrough(driver.app()).blocked, rules::BreakthroughBlock::StoryCap);
        EXPECT_EQ(driver.state().cultivation, before.cultivation);
        EXPECT_EQ(driver.state().realm, before.realm);
        driver.close();
    }
    // 对照：上限若被抬到 32（本章 realm.cap 0 处），同样 5000 修为就不再被 StoryCap 挡——上面那条不是恒真。
    EXPECT_NE(rules::breakthroughBlock(rules::Realm::CoreEarly, 5000, rules::Realm::CoreMid),
              rules::BreakthroughBlock::StoryCap);
}

// 验收 6。起点 ch11-end-first.sav，读档后把 party 改空（扰动）：演完 n05 → 旁白换成 ch12.duoshe.not_in_party、
// ch12.duoshe == 1；空队伍下接着走到 n13 打完 ①（契约 E1 的真实入口版：我方恰韩立 ＋ 两名写死友军）。
// 对照：不扰动的那一趟，n05 由 party.remove 真把曲魂移走、不说那一句。
TEST(Ch12Slice, SixAnEmptyPartyStillPlaysTheSeizureAndFightsTheGhostKingWithTheTwoWrittenAllies) {
    Driver driver;
    ASSERT_TRUE(driver.load(kFirst)) << driver.problem();
    driver.audit = false;
    driver.state().party.clear();
    ASSERT_TRUE(driver.runTo(5)) << driver.problem();
    EXPECT_TRUE(driver.spoke("ch12.duoshe.not_in_party"));
    EXPECT_EQ(driver.state().flag("ch12.duoshe"), 1);
    EXPECT_TRUE(driver.state().party.empty());
    ASSERT_TRUE(driver.runTo(13)) << driver.problem();
    ASSERT_EQ(driver.fights().size(), 1u);
    const Fight& ghost = driver.fights()[0];
    EXPECT_EQ(ghost.id, kBattles[0]);
    EXPECT_EQ(ghost.phase, BattlePhase::Won);
    EXPECT_EQ(ghost.allies, (std::vector<std::string>{"hanli", "zi_ling_xutian", "yuan_yao_heipao"}));
    EXPECT_GT(ghost.heroHpLeft, 0);
    // 战后回写的是韩立（units[0]）：存档气血就是战场上他最后那一刻的气血。
    EXPECT_EQ(ghost.after.hp, ghost.heroHpLeft);
    EXPECT_EQ(driver.state().flag("ch12.guiwang"), 1);
    EXPECT_TRUE(driver.state().party.empty());

    Driver control;
    ASSERT_TRUE(control.load(kFirst)) << control.problem();
    ASSERT_TRUE(control.runTo(5)) << control.problem();
    EXPECT_FALSE(control.spoke("ch12.duoshe.not_in_party"));
    EXPECT_TRUE(control.spoke("ch12.duoshe.seized"));
    EXPECT_EQ(partyIds(control.ledger()[3].after), std::vector<std::string>{"qu_hun_shadan"});
    EXPECT_TRUE(control.state().party.empty());
}

// 验收 8。起点 ch11-end-first.sav，读档后删锦帕（扰动），推到 ch12.canyuan：回荒岛找文思月 → 只一句 ch12.lidao.no_map
// 就 return，mapId 仍 ch12_tushan、ch12.lidao == 0、day 不变（数据上不可达，只验分支在、不把人关死）。
// 对照：把锦帕放回，同一个人再按 → 送进大厅、拨 4 日。
TEST(Ch12Slice, EightWithoutTheBrocadeMapWenSiyueSaysOneLineAndNobodyIsShutIn) {
    Driver driver;
    ASSERT_TRUE(driver.load(kFirst)) << driver.problem();
    driver.audit = false;
    ASSERT_TRUE(driver.state().removeItem("story_jinpa_ditu", 1));
    ASSERT_TRUE(driver.runTo(6)) << driver.problem();
    ASSERT_TRUE(driver.travel("ch12_tushan")) << driver.problem();
    const GameState before = driver.state();
    driver.clearKeys();
    ASSERT_TRUE(driver.press("npc_wen_siyue")) << driver.problem();
    EXPECT_EQ(driver.keys(), std::vector<std::string>{"ch12.lidao.no_map"});
    EXPECT_EQ(driver.state().mapId, "ch12_tushan");
    EXPECT_EQ(driver.state().flag("ch12.lidao"), 0);
    EXPECT_EQ(driver.state().day, before.day);

    driver.state().addItem("story_jinpa_ditu", 1);
    driver.clearKeys();
    ASSERT_TRUE(driver.press("npc_wen_siyue")) << driver.problem();
    EXPECT_EQ(driver.state().mapId, "ch12_dadian");
    EXPECT_EQ(driver.state().flag("ch12.lidao"), 1);
    EXPECT_EQ(driver.state().day, before.day + 4);
    EXPECT_FALSE(driver.spoke("ch12.lidao.no_map"));
    EXPECT_EQ(driver.state().itemCount("story_jinpa_ditu"), 1);
}

// 验收 9。起点 ch11-end-first.sav 走到 ch12.xiagu（冰火道光壁前）。两处都选常速 → n20 走完比意图玩家多 2 日；
// n21 读出已用 3 日（days_3 那一档）、照样打过 ②。另一版常速之后在光壁旁静养 30 日 → 已用 33 日，走「时限将尽」
// 那一档（days_4），照样过关（永不超时）。对照：意图玩家两处罗烟步，已用 1 日（days_1）。
TEST(Ch12Slice, NineSlowStepsCostTwoMoreDaysAndTheFiveDayLimitNeverFails) {
    const Walk& walk = intentWalk(kFirst);
    ASSERT_TRUE(walk.problem.empty()) << walk.problem;
    const GameState reached = reachedAfter(walk, 19);
    for (const int rest : {0, 30}) {
        SCOPED_TRACE(rest);
        Driver driver;
        ASSERT_TRUE(driver.resumeReached(reached, 20)) << driver.problem();
        driver.audit = false;
        driver.picks[20] = {1, 1};
        ASSERT_TRUE(driver.runTo(20)) << driver.problem();
        EXPECT_EQ(driver.state().day, reachedAfter(walk, 20).day + 2);
        EXPECT_EQ(driver.state().day - reached.day, 3);
        EXPECT_TRUE(driver.spoke("ch12.rongyan.slow1"));
        EXPECT_TRUE(driver.spoke("ch12.rongyan.slow2"));
        EXPECT_EQ(driver.state().flag("ch12.binghuo_ri"), reached.day);
        if (rest > 0) ASSERT_TRUE(driver.sit("facility_dazuo_binghuo", rest)) << driver.problem();
        ASSERT_TRUE(driver.runTo(21)) << driver.problem();
        EXPECT_EQ(driver.state().flag("ch12.tihun"), 1);
        ASSERT_EQ(driver.fights().size(), 1u);
        EXPECT_EQ(driver.fights()[0].phase, BattlePhase::Won);
        EXPECT_TRUE(driver.spoke(rest > 0 ? "ch12.tihun.days_4" : "ch12.tihun.days_3"));
        EXPECT_FALSE(driver.spoke("ch12.tihun.days_1"));
    }
    Driver intent;
    ASSERT_TRUE(intent.resumeReached(reached, 20)) << intent.problem();
    ASSERT_TRUE(intent.runTo(21)) << intent.problem();
    EXPECT_TRUE(intent.spoke("ch12.rongyan.luo1"));
    EXPECT_TRUE(intent.spoke("ch12.rongyan.luo2"));
    EXPECT_TRUE(intent.spoke("ch12.tihun.days_1"));
    EXPECT_EQ(intent.state().day - reached.day, 2);
}

// 验收 10。起点 ch11-end-first.sav 走到 ch12.shidian（宝光阁玉桥头），两种灵石都改 0（扰动）：n24 拨 1 日、
// 两件照得；接着走到 n32 同样拨 1 日、ch12.canhun == 1。对照：低阶 0、中阶 1 → n24 扣中阶 1、不拨日。
TEST(Ch12Slice, TenWithNoStonesBothArraysCostADayAndBothTreasuresStillCome) {
    const Walk& walk = intentWalk(kFirst);
    ASSERT_TRUE(walk.problem.empty()) << walk.problem;
    const GameState reached = reachedAfter(walk, 23);
    Driver driver;
    ASSERT_TRUE(driver.resumeReached(reached, 24)) << driver.problem();
    driver.audit = false;
    dropAll(driver.state(), "material_lingshi");
    dropAll(driver.state(), "material_lingshi_zhong");
    ASSERT_TRUE(driver.runTo(24)) << driver.problem();
    EXPECT_EQ(driver.state().day, reached.day + 1);
    EXPECT_TRUE(driver.spoke("ch12.quba.paid_none"));
    EXPECT_EQ(driver.state().itemCount("story_xuese_pifeng"), 1);
    EXPECT_EQ(driver.state().itemCount("story_wuse_tonghuan"), 1);
    EXPECT_EQ(driver.state().flag("ch12.quba"), 1);
    EXPECT_EQ(driver.state().mapId, "ch12_taqian");
    ASSERT_TRUE(driver.runTo(31)) << driver.problem();
    const GameState beforeCanhun = driver.state();
    ASSERT_TRUE(driver.runTo(32)) << driver.problem();
    EXPECT_EQ(driver.state().day, beforeCanhun.day + 1);
    EXPECT_TRUE(driver.spoke("ch12.canhun.paid_none"));
    EXPECT_EQ(driver.state().flag("ch12.canhun"), 1);
    EXPECT_EQ(driver.state().mapId, "ch12_lingquan");

    Driver paid;
    ASSERT_TRUE(paid.resumeReached(reached, 24)) << paid.problem();
    paid.audit = false;
    dropAll(paid.state(), "material_lingshi");
    setCount(paid.state(), "material_lingshi_zhong", 1);
    ASSERT_TRUE(paid.runTo(24)) << paid.problem();
    EXPECT_EQ(paid.state().day, reached.day);
    EXPECT_EQ(paid.state().itemCount("material_lingshi_zhong"), 0);
    EXPECT_TRUE(paid.spoke("ch12.quba.paid_zhong"));
    EXPECT_FALSE(paid.spoke("ch12.quba.paid_none"));
    EXPECT_EQ(paid.state().itemCount("story_xuese_pifeng"), 1);
    EXPECT_EQ(paid.state().itemCount("story_wuse_tonghuan"), 1);
}

// 验收 11（幻境）。起点 ch11-end-first.sav 走到 ch12.quba（塔前石室）。两段都先选错再选对 → xinjing == 0、
// huanjing == 1；只错色段 → 2；只错惧段 → 1。不失败、日子照拨 1 日。对照：意图玩家两段一次选对 → 3（通关终点断言）。
TEST(Ch12Slice, ElevenWrongTurnsInTheIllusionNeverFailAndOnlyAFirstRightAnswerSetsItsBit) {
    const Walk& walk = intentWalk(kFirst);
    ASSERT_TRUE(walk.problem.empty()) << walk.problem;
    const GameState reached = reachedAfter(walk, 24);
    struct Version { std::deque<int> picks; int bits; };
    for (const Version& v : {Version{{0, 1, 0, 1}, 0}, Version{{0, 1, 1}, 2}, Version{{1, 0, 1}, 1}, Version{{1, 1}, 3}}) {
        SCOPED_TRACE(v.bits);
        Driver driver;
        ASSERT_TRUE(driver.resumeReached(reached, 25)) << driver.problem();
        driver.audit = false;
        driver.picks[25] = v.picks;
        ASSERT_TRUE(driver.runTo(25)) << driver.problem();
        EXPECT_EQ(driver.state().flag("ch12.xinjing"), v.bits);
        EXPECT_EQ(driver.state().flag("ch12.huanjing"), 1);
        EXPECT_EQ(driver.state().day, reached.day + 1);
        EXPECT_EQ(driver.spoke("ch12.huanjing.listen"), (v.bits & 1) == 0);
        EXPECT_EQ(driver.spoke("ch12.huanjing.go"), (v.bits & 2) == 0);
    }
}

// 验收 11（拜师）。起点 ch11-end-first.sav 走到 ch12.fanlian：先选拜青易、再选两边都不拜 → 各接一句传音、
// 回到同一抉择；最后拜极阴 → 玄阴环到手、ch12.baishi、蜘蛛不扣。对照：意图玩家一次拜极阴，两句传音都不出。
TEST(Ch12Slice, ElevenRefusingBothMastersLoopsBackAndOnlyJiyinGoesOn) {
    const Walk& walk = intentWalk(kFirst);
    ASSERT_TRUE(walk.problem.empty()) << walk.problem;
    const GameState reached = reachedAfter(walk, 17);
    Driver driver;
    ASSERT_TRUE(driver.resumeReached(reached, 18)) << driver.problem();
    driver.audit = false;
    driver.picks[18] = {1, 2, 0};
    ASSERT_TRUE(driver.runTo(18)) << driver.problem();
    const auto& keys = driver.keys();
    const auto at = [&keys](const char* key) { return std::find(keys.begin(), keys.end(), key) - keys.begin(); };
    ASSERT_TRUE(driver.spoke("ch12.baishi.qing_back") && driver.spoke("ch12.baishi.none_back") &&
                driver.spoke("ch12.baishi.bow"));
    EXPECT_LT(at("ch12.baishi.qing_back"), at("ch12.baishi.none_back"));
    EXPECT_LT(at("ch12.baishi.none_back"), at("ch12.baishi.bow"));
    EXPECT_EQ(driver.state().itemCount("story_xuanyin_huan"), 1);
    EXPECT_EQ(driver.state().flag("ch12.baishi"), 1);
    EXPECT_EQ(driver.state().itemCount("story_xueyu_zhizhu"), reached.itemCount("story_xueyu_zhizhu"));

    Driver intent;
    ASSERT_TRUE(intent.resumeReached(reached, 18)) << intent.problem();
    ASSERT_TRUE(intent.runTo(18)) << intent.problem();
    EXPECT_FALSE(intent.spoke("ch12.baishi.qing_back"));
    EXPECT_FALSE(intent.spoke("ch12.baishi.none_back"));
    EXPECT_EQ(intent.state().itemCount("story_xuanyin_huan"), 1);
}

// 验收 15。起点 ch11-end-first.sav 走到 ch12.dengdao（ch12.tuizhu 未置；此时光罩挂点还没就绪，坡道走得上去）：
// 真踩 portal_dixue → 不换图、对白是 ch12.block.dixue 那一句。对照：走到 ch12.tuizhu（n03）之后同一扇门放行，
// 落在地洞 taijie (15,21)。
TEST(Ch12Slice, FifteenTheCaveDoorRefusesUntilThePillarIsPushed) {
    Driver driver;
    ASSERT_TRUE(driver.load(kFirst)) << driver.problem();
    ASSERT_TRUE(driver.runTo(1)) << driver.problem();
    ASSERT_EQ(driver.state().flag("ch12.tuizhu"), 0);
    const auto gate = driver.object("portal_dixue");
    ASSERT_FALSE(gate.name.empty());
    EXPECT_EQ(gate.property("deny_text_key"), "ch12.block.dixue");
    static_cast<void>(driver.walk(gate.position, true));
    EXPECT_EQ(driver.state().mapId, "ch12_tushan");
    ASSERT_TRUE(dialogueOnTop(driver));
    ASSERT_FALSE(driver.app().dialogueLog().empty());
    EXPECT_EQ(driver.app().dialogueLog().back().body, driver.app().text("ch12.block.dixue"));
    driver.app().popScene();
    driver.app().tick(1.0 / 60.0);

    ASSERT_TRUE(driver.runTo(3)) << driver.problem();
    ASSERT_TRUE(driver.walk(driver.object("portal_dixue").position, true)) << driver.problem();
    EXPECT_EQ(driver.state().mapId, "ch12_dixue");
    EXPECT_EQ(driver.state().position, (Point{15, 21}));
}

// 验收 16。起点 ch11-end-first.sav 走到 ch12.tupo（鬼雾土坡）：三条错路尽头各踩一次 → 仍在 ch12_guiwu、
// 站在那一处岔口（genmaps_ch12.py 的 FORK_A/B/C），日子与旗标一样不动；走到 ch12.guiwang 之后再踩 →
// 原地不动、一句也不说（对照：同一个触发格、同一只脚，只差鬼王灭没灭）。
TEST(Ch12Slice, SixteenEachDetourPushesBackToItsOwnForkUntilTheGhostKingFalls) {
    const Walk& walk = intentWalk(kFirst);
    ASSERT_TRUE(walk.problem.empty()) << walk.problem;
    Driver driver;
    ASSERT_TRUE(driver.resumeReached(reachedAfter(walk, 12), 13)) << driver.problem();
    driver.audit = false;
    struct Detour { const char* hook; Point fork; };
    const Detour detours[] = {{"trigger_zoupian_a", {27, 33}}, {"trigger_zoupian_b", {18, 24}},
                              {"trigger_zoupian_c", {30, 16}}};
    for (const Detour& d : detours) {
        SCOPED_TRACE(d.hook);
        const GameState before = driver.state();
        driver.clearKeys();
        ASSERT_TRUE(driver.enter(d.hook)) << driver.problem();
        EXPECT_EQ(driver.state().mapId, "ch12_guiwu");
        EXPECT_EQ(driver.state().position, d.fork);
        EXPECT_EQ(driver.state().day, before.day);
        EXPECT_EQ(driver.state().flags, before.flags);
        EXPECT_TRUE(driver.spoke("ch12.zoupian.fog"));
        EXPECT_TRUE(driver.spoke("ch12.zoupian.back"));
    }
    ASSERT_TRUE(driver.runTo(13)) << driver.problem();
    ASSERT_EQ(driver.state().flag("ch12.guiwang"), 1);
    for (const Detour& d : detours) {
        SCOPED_TRACE(d.hook);
        const GameState before = driver.state();
        const Point cell = driver.object(d.hook).position;
        driver.clearKeys();
        ASSERT_TRUE(driver.enter(d.hook)) << driver.problem();
        EXPECT_EQ(driver.state().position, cell);
        EXPECT_NE(driver.state().position, d.fork);
        EXPECT_TRUE(driver.keys().empty());
        EXPECT_EQ(driver.state().flags, before.flags);
        EXPECT_EQ(driver.state().day, before.day);
    }
}

// ---- 验收 12（神雷那半句，测试审查 T-2）----
// BattleHand 不会用单体 castMagic 道具，四场胜负一律不靠神雷；施工图 8.2 ④ 却写「只有木（青元剑芒、神雷）要得了命」、
// 第 7 节「神雷两发收尾」。这里由用例自己经真菜单放神雷。
// 起点 ch11-end-first.sav 走到 ch12.butian（n30 走完，寒骊台补天丹之后）；n31 战前给神雷 2 发。④ 里韩立照那只手打，
// 只捂住木（青元剑芒、祭青蛟旗），一手一手地打到「轮到他时玄骨恰被压在 1」——木以外只压得到 1（killable_by ["木"]）；
// 那一刻起分三版：
//   对照（剑）：经菜单「攻击 → 剑 → 玄骨」，这一击类别是剑、玄骨仍 1；之后放开木让那只手收尾（战后余数 2）。
//   用掉 1 发：经菜单「物品 → 辟邪神雷 → 玄骨」，这一击类别是木、玄骨 0、胜；战后余数 1，xuangu.lua 走 take 那一支。
//   用掉 2 发：开打第一手先放一发，再照上一版收尾；战后余数 0，走「余数为 0 不调 take」那一支。
// 三版都把 n31 走完：说 .spent、不说 .keep（take(id, 0) 会被引擎当成扣 1、失败而说 .keep——2 发那一版不说 .keep，
// 就证明余数 0 那一支真没调 take；.keep 本身只在 take 余数失败时出现，余数恰是 item.count 时走不到），背包神雷 0。
enum class Finisher { Sword, Thunder };
struct ThunderPlan { int early; Finisher finisher; };
struct ThunderRun {
    std::string problem;
    std::vector<int> earlyCategories;    // 开打先放的那几发各自的类别
    bool pressed = false;                // 轮到韩立时玄骨恰 1
    int pressedRound = 0;
    int heroHpAtPress = 0;
    std::vector<std::string> prefix;     // 压到 1 那一刻的出手记录（两版比「同一个局面」）
    int finisherCategory = -1;           // 收尾那一击的类别（kCategory*）；-1 = 没找到这一击
    int boneHpAfter = -1;                // 收尾那一击之后玄骨的气血
};

void playHanlitai(Driver& driver, const Walk& walk, const ThunderPlan plan, ThunderRun& run) {
    ASSERT_TRUE(driver.resumeReached(reachedAfter(walk, 30), 31)) << driver.problem();
    test::HandPolicy noWood = intentPolicy();
    noWood.withheldMagics = {"magic_qingyuan_jianmang", "magic_ji_qingjiao"};
    driver.policy = noWood;
    driver.nextModes = {FightMode::Custom};
    driver.customFight = [&driver, &run, plan](BattleScene& scene, test::BattleHand& hand) {
        const int bone = unitIndexOf(scene, "xuan_gu");
        if (bone < 0) { run.problem = "xuan_gu is not on the field"; return false; }
        for (int step = 0; step < 4000 && scene.battle().phase() == BattlePhase::Ongoing; ++step) {
            const int actor = scene.runToAllyTurn();
            if (actor < 0) break;
            const std::size_t from = scene.battle().events().size();
            if (static_cast<int>(run.earlyCategories.size()) < plan.early) {
                if (std::string why = throwByMenu(driver, scene, actor, kThunder, bone); !why.empty()) { run.problem = why; return false; }
                const BattleEvent* hit = firstHit(scene, from, actor, bone);
                run.earlyCategories.push_back(hit ? hit->category : -1);
                continue;
            }
            if (!run.pressed && scene.battle().units()[static_cast<std::size_t>(bone)].hp == 1) {
                run.pressed = true;
                run.pressedRound = scene.battle().round();
                run.heroHpAtPress = scene.battle().units()[static_cast<std::size_t>(actor)].hp;
                run.prefix = scene.battle().log();
                std::string why = plan.finisher == Finisher::Thunder ? throwByMenu(driver, scene, actor, kThunder, bone)
                                                                     : strikeByMenu(driver, scene, actor, core::kCategorySword, bone);
                if (!why.empty()) { run.problem = why; return false; }
                const BattleEvent* hit = firstHit(scene, from, actor, bone);
                run.finisherCategory = hit ? hit->category : -1;
                run.boneHpAfter = scene.battle().units()[static_cast<std::size_t>(bone)].hp;
                if (plan.finisher == Finisher::Sword) hand.policy() = intentPolicy();   // 对照：放开木收尾
                continue;
            }
            if (!hand.act(scene, actor)) { run.problem = "the hand could not act"; return false; }
        }
        if (scene.battle().phase() == BattlePhase::Ongoing) run.problem = "battle 4 did not end";
        return run.problem.empty();
    };
    static_cast<void>(driver.runTo(31));
}

TEST(Ch12Slice, TwelveTheWoodThunderThrownFromTheMenuFinishesXuanGuWhereTheSwordOnlyHoldsHimAtOne) {
    const Walk& walk = intentWalk(kFirst);
    ASSERT_TRUE(walk.problem.empty()) << walk.problem;
    ASSERT_EQ(reachedAfter(walk, 30).flag("ch12.butian"), 1);
    Driver sword, one, two;
    ThunderRun bySword, byOne, byTwo;
    playHanlitai(sword, walk, {0, Finisher::Sword}, bySword);
    playHanlitai(one, walk, {0, Finisher::Thunder}, byOne);
    playHanlitai(two, walk, {1, Finisher::Thunder}, byTwo);
    for (const auto& pair : {std::pair{&sword, &bySword}, std::pair{&one, &byOne}, std::pair{&two, &byTwo}}) {
        Driver& d = *pair.first;
        const ThunderRun& r = *pair.second;
        SCOPED_TRACE(&d == &sword ? "sword" : &d == &one ? "one thunder" : "two thunders");
        ASSERT_TRUE(r.problem.empty()) << r.problem;
        ASSERT_TRUE(d.problem().empty()) << d.problem();
        ASSERT_TRUE(r.pressed) << "玄骨没在轮到韩立时停在 1 上";
        ASSERT_EQ(d.fights().size(), 1u);
        EXPECT_EQ(d.fights()[0].id, kBattles[3]);
        EXPECT_EQ(d.fights()[0].phase, BattlePhase::Won);
        EXPECT_EQ(d.fights()[0].before.itemCount(kThunder), 2);
        // n31 走完：余数分支只说 .spent，背包神雷收尽，ch12.xuangu 置上。
        EXPECT_EQ(d.state().flag("ch12.xuangu"), 1);
        EXPECT_TRUE(d.spoke("ch12.xuangu.spent"));
        EXPECT_FALSE(d.spoke("ch12.xuangu.keep"));
        EXPECT_EQ(d.state().itemCount(kThunder), 0);
    }
    // 对照与 1 发那一版走的是同一条路：压到 1 的回合、韩立余血、出手记录一字不差——两版只差收尾那一手。
    EXPECT_EQ(byOne.pressedRound, bySword.pressedRound);
    EXPECT_EQ(byOne.heroHpAtPress, bySword.heroHpAtPress);
    EXPECT_EQ(byOne.prefix, bySword.prefix);
    // 对照：剑只压得到 1。
    EXPECT_EQ(bySword.finisherCategory, core::kCategorySword);
    EXPECT_EQ(bySword.boneHpAfter, 1);
    EXPECT_EQ(sword.fights()[0].after.itemCount(kThunder), 2);
    // 神雷：这一击是木，把被压在 1 的玄骨打到 0、当场分出胜负。
    for (const ThunderRun* r : {&byOne, &byTwo}) {
        EXPECT_EQ(r->finisherCategory, core::kCategoryWood);
        EXPECT_EQ(r->boneHpAfter, 0);
    }
    EXPECT_EQ(byTwo.earlyCategories, std::vector<int>{core::kCategoryWood});
    EXPECT_EQ(one.fights()[0].after.itemCount(kThunder), 1);
    EXPECT_EQ(two.fights()[0].after.itemCount(kThunder), 0);
}
} // namespace
