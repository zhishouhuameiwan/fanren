// 第 13 章验收第 5、6、7、8、9、10、11、12、13、27 条的「切片」那一半（扫脚本 / 读数据那一半在 Ch13TriggerMode、
// Ch13BattleData、Ch13Ledger、Ch13EngineTests 里，测试路 A 交付）。每条写明起点存档：一律是
// tests/fixtures/ch12-end-first.sav（T1 重生成版），由 Ch13Driver 走真实入口推到所需旗标（intentWalk 的
// 「第 n 节点走完那一刻」），再在用例里写明扰动；每条都配一组对照（扰动前后、两版分支），证明断言不是恒真。
#include <algorithm>
#include <utility>
#include "Ch13Driver.h"
#include "core/model/AttackCategory.h"
#include "game/DialogueScene.h"
#include "game/FieldScene.h"

namespace {
using namespace fanren;
using namespace fanren::test::ch13;

void setCount(GameState& state, const char* id, int wanted) {
    const int have = state.itemCount(id);
    if (have > wanted) ASSERT_TRUE(state.removeItem(id, have - wanted)) << id;
    if (have < wanted) state.addItem(id, wanted - have);
}

const Walk& firstWalk() {
    const Walk& walk = intentWalk(kFirst);
    EXPECT_TRUE(walk.problem.empty()) << walk.problem;
    return walk;
}

std::string topName(Driver& driver) { return driver.app().topScene() ? driver.app().topScene()->name() : std::string{}; }

// 一句旁白 / 对白压在栈顶时收掉它（设施拦人那一句不是脚本，不进 pump）。
void dismiss(Driver& driver) {
    driver.app().popScene();
    driver.app().tick(1.0 / 60.0);
}

// ---- 验收 5：境界一处上限、玩家自冲、出关门闸 ----
// 起点 ch12-end-first.sav，推到 ch13.biguan（n27 走完：上限已抬到 32、人在雾中岛凹谷院里）。扰动：修为写成
// 4000 / 5000。4000：踩出关带 → 一句「瓶颈未破」、不置旗、不拨日，挂点还响。5000：真实入口 openFacility
//（facility_jingshi_wz）→ 先在静室坐 1 日（meditateFor）、再把修为写成 5000 → CultivationScene 冲关。冲关种子是面板的
// breakthroughSeed（day、修为、境界的哈希）：day 53942、修为 5000、结丹初期这一颗冲得成（85%）→ realm 32、面板下一关
// 报 StoryCap；再踩 → ch13.chuguan == 1（契约 E1 的真实入口版）。不坐那 1 日（day 53941）的那一颗掷不中：倒扣两成、
// 境界不动、出关带照旧一句——成与不成都是那颗种子定的，不是恒真。
// 对照：推到 ch13.liandan（n26，上限仍 31），同样 5000 → StoryCap、一分不扣。
TEST(Ch13Slice, FiveTheExitWaitsForCoreMidAndThePanelPushesThereThenReportsTheCap) {
    const Walk& walk = firstWalk();
    const GameState reached = reachedAfter(walk, 27);
    ASSERT_EQ(rules::toValue(reached.realmCap), 32);
    ASSERT_EQ(rules::toValue(reached.realm), 31);
    Driver driver;
    ASSERT_TRUE(driver.resumeReached(reached, 28)) << driver.problem();
    driver.state().cultivation = 4000;
    for (int again = 0; again < 2; ++again) {
        SCOPED_TRACE(again);
        driver.clearKeys();
        const int day = driver.state().day;
        ASSERT_TRUE(driver.enter("trigger_chuguan")) << driver.problem();
        EXPECT_EQ(driver.keys(), std::vector<std::string>{"ch13.chuguan.not_yet"});
        EXPECT_EQ(driver.state().flag("ch13.chuguan"), 0);
        EXPECT_EQ(driver.state().day, day);
        EXPECT_EQ(rules::toValue(driver.state().realm), 31);
    }

    Driver unlucky;
    ASSERT_TRUE(unlucky.resumeReached(reached, 28)) << unlucky.problem();
    unlucky.state().cultivation = 5000;
    ASSERT_TRUE(unlucky.open("facility_jingshi_wz")) << unlucky.problem();
    auto* unluckyPanel = dynamic_cast<CultivationScene*>(unlucky.app().topScene());
    ASSERT_NE(unluckyPanel, nullptr);
    EXPECT_EQ(unlucky.state().day, 53941);
    const auto missed = unluckyPanel->breakthrough(unlucky.app());
    EXPECT_EQ(missed.blocked, rules::BreakthroughBlock::None);
    EXPECT_FALSE(missed.success);
    EXPECT_EQ(unlucky.state().cultivation, 5000 - missed.cultivationLost);
    EXPECT_GE(missed.cultivationLost, 1000);
    EXPECT_EQ(rules::toValue(unlucky.state().realm), 31);
    unlucky.close();
    unlucky.clearKeys();
    ASSERT_TRUE(unlucky.enter("trigger_chuguan")) << unlucky.problem();
    EXPECT_EQ(unlucky.keys(), std::vector<std::string>{"ch13.chuguan.not_yet"});

    Driver push;
    ASSERT_TRUE(push.resumeReached(reached, 28)) << push.problem();
    ASSERT_TRUE(push.sit("facility_jingshi_wz", 1)) << push.problem();
    push.state().cultivation = 5000;
    EXPECT_EQ(push.state().day, 53942);
    ASSERT_TRUE(push.open("facility_jingshi_wz")) << push.problem();
    auto* panel = dynamic_cast<CultivationScene*>(push.app().topScene());
    ASSERT_NE(panel, nullptr);
    EXPECT_EQ(rules::cultivationNeeded(push.state().realm), 5000);
    const auto attempt = panel->breakthrough(push.app());
    EXPECT_EQ(attempt.blocked, rules::BreakthroughBlock::None);
    ASSERT_TRUE(attempt.success) << "固定种子冲得成";
    EXPECT_EQ(rules::toValue(push.state().realm), 32);
    EXPECT_EQ(push.state().maxHp, rules::realmMaxHp(rules::Realm::CoreMid));
    EXPECT_EQ(push.state().maxMp, rules::realmMaxMp(rules::Realm::CoreMid));
    const auto rows = CultivationScene::buildMainItems(push.state());
    ASSERT_GE(rows.size(), 2u);
    EXPECT_FALSE(rows[1].enabled);
    EXPECT_EQ(rows[1].disabledReason,
              std::string(game::cultivationLexicon(game::PanelStage::Immortal).pushAtStoryCap));
    const int cultivation = push.state().cultivation;
    EXPECT_EQ(panel->breakthrough(push.app()).blocked, rules::BreakthroughBlock::StoryCap);
    EXPECT_EQ(push.state().cultivation, cultivation);
    push.close();
    const int day = push.state().day;
    push.clearKeys();
    ASSERT_TRUE(push.enter("trigger_chuguan")) << push.problem();
    EXPECT_EQ(push.state().flag("ch13.chuguan"), 1);
    EXPECT_EQ(push.state().day, day + 3600 + 3600);
    EXPECT_FALSE(push.spoke("ch13.chuguan.not_yet"));

    Driver capped;
    ASSERT_TRUE(capped.resumeReached(reachedAfter(walk, 26), 27)) << capped.problem();
    capped.state().cultivation = 5000;
    ASSERT_TRUE(capped.open("facility_jingshi_wz")) << capped.problem();
    auto* cappedPanel = dynamic_cast<CultivationScene*>(capped.app().topScene());
    ASSERT_NE(cappedPanel, nullptr);
    EXPECT_EQ(cappedPanel->breakthrough(capped.app()).blocked, rules::BreakthroughBlock::StoryCap);
    EXPECT_EQ(capped.state().cultivation, 5000);
    EXPECT_EQ(rules::toValue(capped.state().realm), 31);
    capped.close();
}

// ---- 验收 6：跨图旧田 ----
// 起点 ch12-end-first.sav，推到 ch13.rucheng（n07 走完，城门之内）。扰动：给 field_tianxing_39 第 0 格种一株
// 熟了的六十年黄精（第 12 章终档这块田六槽全空，没有可采的东西）。从 facility_yaoyuan_39 进去是同一块田、
// 不新建；采收进包、年份随包。负向：ch13.rucheng 抹回 0（扰动——n07 的带横贯城门，不踩它进不了城，「未置时
// 站在设施前」只能这样造）→ 药园、静室都拦：栈顶不是 Field / Cultivation，播的是 ui.facility.locked。
TEST(Ch13Slice, SixTheOldFieldOpensAsItIsAcrossTheMapAndBothRoomsWaitForTheGate) {
    const Walk& walk = firstWalk();
    const GameState reached = reachedAfter(walk, 7);
    ASSERT_EQ(reached.flag("ch13.rucheng"), 1);
    constexpr const char* kField = "field_tianxing_39";
    constexpr const char* kStalk = "herb_huangjing_cao";
    Driver driver;
    ASSERT_TRUE(driver.resumeReached(reached, 8)) << driver.problem();
    auto* field = driver.state().findField(kField);
    ASSERT_NE(field, nullptr);
    ASSERT_EQ(field->slots.size(), 6u);
    for (const auto& slot : field->slots) ASSERT_TRUE(slot.seedId.empty()) << "先验：真档 6 槽全空";
    field->slots[0].seedId = kStalk;
    field->slots[0].plantedDay = driver.state().day;
    field->slots[0].age = 60;
    field->slots[0].ripe = true;
    const std::size_t fields = driver.state().fields.size();
    ASSERT_TRUE(driver.open("facility_yaoyuan_39")) << driver.problem();
    auto* scene = dynamic_cast<game::FieldScene*>(driver.app().topScene());
    ASSERT_NE(scene, nullptr) << topName(driver);
    EXPECT_EQ(scene->fieldId(), kField);
    EXPECT_EQ(driver.state().fields.size(), fields) << "旧田原样、不新建";
    const int total = driver.state().itemCount(kStalk);
    const int aged = driver.state().itemCountOfAge(kStalk, 60);
    ASSERT_TRUE(scene->harvestAt(driver.app(), 0)) << scene->feedback();
    EXPECT_EQ(driver.state().itemCount(kStalk), total + 1);
    EXPECT_EQ(driver.state().itemCountOfAge(kStalk, 60), aged + 1) << "年份随包，不丢";
    EXPECT_TRUE(driver.state().findField(kField)->slots[0].seedId.empty());
    driver.close();
    ASSERT_TRUE(driver.open("facility_jingshi_39")) << driver.problem();
    EXPECT_EQ(topName(driver), "Cultivation");
    driver.close();

    Driver locked;
    ASSERT_TRUE(locked.resumeReached(reached, 8)) << locked.problem();
    locked.state().setFlag("ch13.rucheng", 0);
    for (const char* name : {"facility_yaoyuan_39", "facility_jingshi_39"}) {
        SCOPED_TRACE(name);
        const std::size_t lines = locked.app().dialogueLog().size();
        ASSERT_TRUE(locked.open(name)) << locked.problem();
        EXPECT_NE(topName(locked), "Field");
        EXPECT_NE(topName(locked), "Cultivation");
        ASSERT_EQ(locked.app().dialogueLog().size(), lines + 1);
        EXPECT_EQ(locked.app().dialogueLog().back().body, locked.app().text("ui.facility.locked"));
        dismiss(locked);
        EXPECT_EQ(locked.state().fields.size(), fields);
    }
    // 读地图与文案（16.1 第 45 条）：两处设施 0 个 deny_text_key；data/text/ch13*.json 0 个 ch13.block.* key。
    const auto tianxing = objects(json(root() / "maps" / "ch13_tianxing.tmj"));
    for (const char* name : {"facility_yaoyuan_39", "facility_jingshi_39"}) {
        ASSERT_TRUE(tianxing.count(name)) << name;
        EXPECT_FALSE(properties(tianxing.at(name)).contains("deny_text_key")) << name;
        EXPECT_EQ(properties(tianxing.at(name)).value("require_flag", ""), "ch13.rucheng") << name;
    }
    int texts = 0;
    for (const auto& file : fs::directory_iterator(root() / "data" / "text")) {
        const auto stem = file.path().stem().string();
        if (stem.rfind("ch13", 0) != 0 || file.path().extension() != ".json") continue;
        ++texts;
        const Json table = json(file.path());
        for (const auto& entry : table.items())
            EXPECT_NE(entry.key().rfind("ch13.block.", 0), 0u) << stem << ' ' << entry.key();
    }
    EXPECT_GE(texts, 6) << "先验：第 13 章的文案文件读到了";
}

// 账上每一样都没动（除了 except 里列的）。
void expectAccountsKept(const GameState& before, const GameState& after, std::initializer_list<const char*> except = {}) {
    for (const char* id : kAccounts) {
        if (std::find_if(except.begin(), except.end(), [id](const char* e) { return std::string(e) == id; }) != except.end())
            continue;
        EXPECT_EQ(after.itemCount(id), before.itemCount(id)) << id;
    }
}

// ---- 验收 7：偷渡三支 ----
// 起点 ch12-end-first.sav，推到 ch13.yirong（n11 走完，四十九层石阶）。扰动：两种付法的本钱写成下表各行。
// 排的是第几项由 choose 定；排了列表里没有的那一项（越界）由 ScriptHost 按取消处理——拿它证明「列了几项」。
struct Toll {
    int low;
    int wuji;
    int pick;
    int toudu;      // 0 ＝ 取消：不扣、不置旗、不传送
    int lowAfter;
    int wujiAfter;
    const char* said;
};

TEST(Ch13Slice, SevenTheTollListsOnlyWhatHeCanPayAndTheBladeIsAlwaysThere) {
    const GameState reached = reachedAfter(firstWalk(), 11);
    ASSERT_EQ(reached.flag("ch13.yirong"), 1);
    const Toll tolls[] = {
        {0, 0, 0, 3, 0, 0, "ch13.chuansong.paid_blade"},           // 只列第三支，背包不变
        {0, 0, 1, 0, 0, 0, nullptr},                               // 对照：只有一项，第二项越界 → 取消
        {10000, 4, 0, 1, 0, 4, "ch13.chuansong.paid_stone"},        // 三支都列：第一支恰扣 10000
        {10000, 4, 1, 2, 10000, 0, "ch13.chuansong.paid_dan"},      // 第二支恰扣五级 4
        {10000, 4, 2, 3, 10000, 4, "ch13.chuansong.paid_blade"},    // 第三项在：真列了三项
        {10000, 0, 2, 0, 10000, 0, nullptr},                       // 灵石够、五级不够：两项，第三项越界 → 取消
        {0, 4, 0, 2, 0, 0, "ch13.chuansong.paid_dan"},             // 五级够、灵石不够：第一项是五级
    };
    for (const Toll& t : tolls) {
        SCOPED_TRACE(std::to_string(t.low) + " stones, " + std::to_string(t.wuji) + " cores, pick " + std::to_string(t.pick));
        Driver driver;
        ASSERT_TRUE(driver.resumeReached(reached, 12)) << driver.problem();
        setCount(driver.state(), kLow, t.low);
        setCount(driver.state(), kWuji, t.wuji);
        const GameState before = driver.state();
        driver.clearKeys();
        driver.choose({t.pick});
        ASSERT_TRUE(driver.press("npc_gu_laozhe")) << driver.problem();
        EXPECT_TRUE(driver.choicesDrained());
        EXPECT_EQ(driver.state().flag("ch13.toudu"), t.toudu);
        EXPECT_EQ(driver.state().flag("ch13.chuansong"), t.toudu == 0 ? 0 : 1);
        EXPECT_EQ(driver.state().mapId, t.toudu == 0 ? "ch13_tianxing" : "ch13_waihai");
        EXPECT_EQ(driver.state().itemCount(kLow), t.lowAfter);
        EXPECT_EQ(driver.state().itemCount(kWuji), t.wujiAfter);
        expectAccountsKept(before, driver.state(), {kLow, kWuji});
        EXPECT_EQ(driver.state().day, before.day);
        if (t.said) EXPECT_TRUE(driver.spoke(t.said)) << t.said;
        for (const char* key : {"ch13.chuansong.paid_stone", "ch13.chuansong.paid_dan", "ch13.chuansong.paid_blade"})
            if (!t.said || std::string(key) != t.said) EXPECT_FALSE(driver.spoke(key)) << key;
    }
}

// ---- 验收 8：诱妖循环与收手门槛 ----
// 起点 ch12-end-first.sav，推到 ch13.buzhen（n23 走完：登礁、阵已布，草 4、绿液 9、低阶 2931）。逐支扰动：
// biguan 已置；草 0 而灵石够；草 0、灵石 0（兜底）；绿液 0；照打一胜；开打即逃。另推到 ch13.lieyao == 1 / 2
// 去收阵石。
TEST(Ch13Slice, EightTheLureLoopSpendsWhatItShouldOnEachBranchAndTheReefWaitsForTwoWins) {
    const Walk& walk = firstWalk();
    const GameState reached = reachedAfter(walk, 23);
    ASSERT_EQ(reached.flag("ch13.buzhen"), 1);
    ASSERT_GE(reached.itemCount(kHerb), 1);
    ASSERT_GE(reached.bottle.drops, 1);
    struct Branch {
        const char* what;
        void (*perturb)(GameState&);
        std::vector<std::string> keys;
        int days;
        int herbDelta;
    };
    const Branch quiet[] = {
        {"after the retreat", [](GameState& s) { s.setFlag("ch13.biguan"); }, {"ch13.youyao.closed"}, 0, 0},
        {"no herb but stones", [](GameState& s) { setCount(s, kHerb, 0); }, {"ch13.youyao.no_herb"}, 0, 0},
        {"no herb and no stones", [](GameState& s) { setCount(s, kHerb, 0); setCount(s, kLow, 0); },
         {"ch13.youyao.grow"}, 30, 1},
        {"no green drop", [](GameState& s) { s.bottle.drops = 0; }, {"ch13.youyao.no_drop"}, 5, 0},
    };
    for (const Branch& b : quiet) {
        SCOPED_TRACE(b.what);
        Driver driver;
        ASSERT_TRUE(driver.resumeReached(reached, 24)) << driver.problem();
        b.perturb(driver.state());
        const GameState before = driver.state();
        driver.clearKeys();
        ASSERT_TRUE(driver.press("trigger_youyao")) << driver.problem();
        EXPECT_EQ(driver.keys(), b.keys);
        EXPECT_TRUE(driver.fights().empty());
        EXPECT_EQ(driver.state().day, before.day + b.days);
        EXPECT_EQ(driver.state().itemCount(kHerb), before.itemCount(kHerb) + b.herbDelta);
        EXPECT_EQ(driver.state().flag("ch13.lieyao"), 0);
        expectAccountsKept(before, driver.state(), {kHerb});
    }
    // 照打一胜：绿液战前扣 1、拨 3 日；胜 → 草 −1、lieyao ＋1、drops 进包（六级 1、五级 2）。
    // 开打即逃：绿液照扣（3.2），草与 lieyao 都不变、一颗妖丹也没有。
    for (const FightMode mode : {FightMode::Intent, FightMode::Flee}) {
        SCOPED_TRACE(static_cast<int>(mode));
        Driver driver;
        ASSERT_TRUE(driver.resumeReached(reached, 24)) << driver.problem();
        const GameState before = driver.state();
        driver.nextModes = {mode};
        driver.clearKeys();
        ASSERT_TRUE(driver.press("trigger_youyao")) << driver.problem();
        ASSERT_EQ(driver.fights().size(), 1u);
        const bool won = mode == FightMode::Intent;
        EXPECT_EQ(driver.fights()[0].phase, won ? BattlePhase::Won : BattlePhase::Escaped);
        EXPECT_TRUE(driver.spoke("ch13.youyao.drip"));
        EXPECT_EQ(driver.state().day, before.day + 3);
        EXPECT_EQ(driver.state().itemCount(kHerb), before.itemCount(kHerb) - (won ? 1 : 0));
        EXPECT_EQ(driver.state().flag("ch13.lieyao"), won ? 1 : 0);
        EXPECT_EQ(driver.state().itemCount(kLiuji), before.itemCount(kLiuji) + (won ? 1 : 0));
        EXPECT_EQ(driver.state().itemCount(kWuji), before.itemCount(kWuji) + (won ? 2 : 0));
        EXPECT_EQ(driver.spoke("ch13.youyao.escaped"), !won);
        EXPECT_EQ(driver.spoke("ch13.youyao.win"), won);
        expectAccountsKept(before, driver.state(), {kHerb, kLiuji, kWuji});
    }
    // 收阵石：lieyao == 1 → 一句、不置旗、不拨日；== 2（n24 走完那一刻）→ 六级 6、五级 10、六七级材料 1、2415 日。
    Driver once;
    ASSERT_TRUE(once.resumeReached(reached, 24)) << once.problem();
    ASSERT_TRUE(once.press("trigger_youyao")) << once.problem();
    ASSERT_EQ(once.state().flag("ch13.lieyao"), 1);
    const GameState oneWin = once.state();
    once.clearKeys();
    ASSERT_TRUE(once.press("trigger_shoushou")) << once.problem();
    EXPECT_EQ(once.keys(), std::vector<std::string>{"ch13.chongmo.early"});
    EXPECT_EQ(once.state().flag("ch13.chongmo"), 0);
    EXPECT_EQ(once.state().day, oneWin.day);
    expectAccountsKept(oneWin, once.state());
    Driver twice;
    const GameState twoWins = reachedAfter(walk, 24);
    ASSERT_EQ(twoWins.flag("ch13.lieyao"), 2);
    ASSERT_TRUE(twice.resumeReached(twoWins, 25)) << twice.problem();
    ASSERT_TRUE(twice.press("trigger_shoushou")) << twice.problem();
    EXPECT_EQ(twice.state().flag("ch13.chongmo"), 1);
    EXPECT_EQ(twice.state().day, twoWins.day + 2415);
    EXPECT_EQ(twice.state().itemCount(kLiuji), twoWins.itemCount(kLiuji) + 6);
    EXPECT_EQ(twice.state().itemCount(kWuji), twoWins.itemCount(kWuji) + 10);
    EXPECT_EQ(twice.state().itemCount(kCargo), twoWins.itemCount(kCargo) + 1);
    expectAccountsKept(twoWins, twice.state(), {kLiuji, kWuji, kCargo});
}

// ---- 验收 9：重炼、玄阴经、五色珠、金丝蚕 ----
// 起点 ch12-end-first.sav。一版读档后删炼晶（扰动）、推到 ch13.xuanyin（n19 走完）：n20 只换一句（no_crystal，
// 不说 seal）、照样学会蜂云合剑、ch13.chonglian == 1、拨 360 日。另一版读档后把金丝蚕扣成 0（扰动）推过 n22：
// n18 无幻焰蛾那一段（moth / feed）、n22 一次 take 也没有（silk / silk_gone / silk_fail 都不说）。
// 对照：不扰动的意图玩家（炼晶 1、金丝蚕 3）说 seal、moth、silk_gone，n22 之后金丝蚕 0。
TEST(Ch13Slice, NineTheOldThingsAreTakenWhenPresentAndTheirAbsenceOnlyChangesALine) {
    Driver crystal;
    ASSERT_TRUE(crystal.load(kFirst)) << crystal.problem();
    crystal.audit = false;
    ASSERT_TRUE(crystal.state().removeItem("material_lianjing", 1));
    ASSERT_TRUE(crystal.runTo(19)) << crystal.problem();
    const GameState before = crystal.state();
    crystal.clearKeys();
    ASSERT_TRUE(crystal.runTo(20)) << crystal.problem();
    EXPECT_TRUE(crystal.spoke("ch13.chonglian.no_crystal"));
    EXPECT_FALSE(crystal.spoke("ch13.chonglian.seal"));
    EXPECT_TRUE(crystal.spoke("ch13.chonglian.join"));
    const auto& learned = crystal.state().learnedMagics;
    EXPECT_NE(std::find(learned.begin(), learned.end(), "magic_fengyun_heji"), learned.end());
    EXPECT_EQ(crystal.state().flag("ch13.chonglian"), 1);
    EXPECT_EQ(crystal.state().day, before.day + 360);
    EXPECT_EQ(crystal.state().itemCount("material_lianjing"), 0);

    Driver silkless;
    ASSERT_TRUE(silkless.load(kFirst)) << silkless.problem();
    silkless.audit = false;
    setCount(silkless.state(), kSilk, 0);
    ASSERT_TRUE(silkless.runTo(22)) << silkless.problem();
    for (const char* key : {"ch13.chongshi.moth", "ch13.chongshi.feed", "ch13.wusezhu.silk", "ch13.wusezhu.silk_gone",
                            "ch13.wusezhu.silk_fail"})
        EXPECT_FALSE(silkless.spoke(key)) << key;
    EXPECT_EQ(silkless.state().itemCount(kSilk), 0);
    EXPECT_EQ(silkless.state().itemCount("story_wuse_yuanzhu"), 0);
    EXPECT_EQ(silkless.state().flag("ch13.wusezhu"), 1);

    Driver intent;
    ASSERT_TRUE(intent.load(kFirst)) << intent.problem();
    ASSERT_TRUE(intent.runTo(22)) << intent.problem();
    for (const char* key : {"ch13.chonglian.seal", "ch13.chongshi.moth", "ch13.chongshi.feed", "ch13.wusezhu.silk",
                            "ch13.wusezhu.silk_gone"})
        EXPECT_TRUE(intent.spoke(key)) << key;
    EXPECT_FALSE(intent.spoke("ch13.chonglian.no_crystal"));
    EXPECT_EQ(intent.state().itemCount(kSilk), 0);
    EXPECT_EQ(intent.ledger()[17].after.itemCount(kSilk), kStartSilk) << "n18 只读不扣";
}

// ---- 验收 10：至元丹兜底 ----
// 起点 ch12-end-first.sav，推到 ch13.chuguan（n28 走完：结丹中期，公孙杏、孙师叔在甬道口）。三份扰动：
// 至元丹 3（六级照旧）；至元丹 0、六级 1；两样都 0。回话排第一项，至元丹那一问排列出来的第一项。
// 对照：丹 3、六级 8 时排第三项 → 开炉一月，丹与六级都不动（三支都列）。
struct Pill {
    int pills;
    int liuji;
    int pick;
    int way;
    int days;
    int pillsAfter;
    int liujiAfter;
};

TEST(Ch13Slice, TenThePillQuestionAlwaysHasTheFurnaceAndEachAnswerIsRecorded) {
    const GameState reached = reachedAfter(firstWalk(), 28);
    ASSERT_EQ(reached.flag("ch13.chuguan"), 1);
    ASSERT_EQ(reached.itemCount(kLiuji), 8);
    constexpr const char* kPill = "pill_zhiyuan_dan";
    const Pill cases[] = {
        {3, 8, 0, 1, 3, 0, 8},
        {0, 1, 0, 2, 3, 0, 0},
        {0, 0, 0, 3, 33, 0, 0},
        {3, 8, 2, 3, 33, 3, 8},
    };
    for (const Pill& c : cases) {
        SCOPED_TRACE(std::to_string(c.pills) + " pills, " + std::to_string(c.liuji) + " cores, pick " + std::to_string(c.pick));
        Driver driver;
        ASSERT_TRUE(driver.resumeReached(reached, 29)) << driver.problem();
        setCount(driver.state(), kPill, c.pills);
        setCount(driver.state(), kLiuji, c.liuji);
        const GameState before = driver.state();
        driver.choose({0, c.pick});
        ASSERT_TRUE(driver.press("npc_gongsun_xing")) << driver.problem();
        EXPECT_TRUE(driver.choicesDrained());
        EXPECT_EQ(driver.state().flag("ch13.zhiyuan"), c.way);
        EXPECT_EQ(driver.state().flag("ch13.gongsun"), 1);
        EXPECT_EQ(driver.state().flag("ch13.shouchao"), 1);
        EXPECT_EQ(driver.state().day, before.day + c.days);
        EXPECT_EQ(driver.state().itemCount(kPill), c.pillsAfter);
        EXPECT_EQ(driver.state().itemCount(kLiuji), c.liujiAfter);
        expectAccountsKept(before, driver.state(), {kPill, kLiuji});
    }
}

// ---- 验收 11：铜片必得 ----
// 起点 ch12-end-first.sav，推到 ch13.lidao（n30 走完，秘市岸 (14,46)）。扰动：五级 0、灵石 0 → 只列「以材料抵」：
// 排第一项 → story_tongpian 1、ch13.tongpian == 3、灵石五级都不动；排第二项（越界）→ 按取消：不给铜片、不置旗。
// 对照：不扰动（五级 14、灵石 2931 不到 5000）排第一项是五级那一支（ch13.tongpian == 1、五级 −1）。
TEST(Ch13Slice, ElevenTheCopperShardComesEvenWithEmptyPockets) {
    const GameState reached = reachedAfter(firstWalk(), 30);
    ASSERT_EQ(reached.mapId, "ch13_waihai");
    struct Shard { bool broke; int pick; int tongpian; int wujiDelta; };
    for (const Shard s : {Shard{true, 0, 3, 0}, Shard{true, 1, 0, 0}, Shard{false, 0, 1, -1}}) {
        SCOPED_TRACE(std::to_string(s.broke) + " pick " + std::to_string(s.pick));
        Driver driver;
        ASSERT_TRUE(driver.resumeReached(reached, 31)) << driver.problem();
        if (s.broke) {
            setCount(driver.state(), kWuji, 0);
            setCount(driver.state(), kLow, 0);
        }
        const GameState before = driver.state();
        driver.choose(s.pick == 0 ? std::deque<int>{0, 0, 0} : std::deque<int>{s.pick});
        ASSERT_TRUE(driver.press("trigger_mishi")) << driver.problem();
        EXPECT_TRUE(driver.choicesDrained());
        EXPECT_EQ(driver.shops(), std::vector<std::string>{"ch13_mishi_tan"});
        EXPECT_EQ(driver.state().flag("ch13.tongpian"), s.tongpian);
        EXPECT_EQ(driver.state().flag("ch13.jiaohuan"), s.tongpian == 0 ? 0 : 1);
        EXPECT_EQ(driver.state().itemCount("story_tongpian"), s.tongpian == 0 ? 0 : 1);
        EXPECT_EQ(driver.state().itemCount(kWuji), before.itemCount(kWuji) + s.wujiDelta);
        EXPECT_EQ(driver.state().itemCount(kLow), before.itemCount(kLow));
        EXPECT_EQ(driver.state().itemCount(kCargo), before.itemCount(kCargo) - (s.tongpian == 0 ? 0 : 1));
        EXPECT_EQ(driver.state().day, before.day + (s.tongpian == 0 ? 0 : 5));
    }
}

// 黑石城的六个 NPC（5.1：hidden_flag 都是 ch13.shouchao）。
constexpr std::array<const char*, 6> kBlackstone{"npc_yi_jing", "npc_xu_yun", "npc_ming_furen", "npc_paimaishi",
                                                 "npc_li_xun", "npc_gu_yu"};

int visibleOf(Driver& driver, const std::array<const char*, 6>& names) {
    int count = 0;
    const auto* map = driver.app().currentMap();
    for (const char* name : names) {
        const auto o = driver.object(name);
        if (!o.name.empty() && game::WorldScene::visibleNpcAt(driver.state(), *map, o.position)) ++count;
    }
    return count;
}

// ---- 验收 12：兽潮只经人口、店随人撤 ----
// 起点 ch12-end-first.sav，推到 ch13.lidao（n30 走完，秘市岸）——ch13.gongsun（n29，雾中岛）之后到外星海的唯一一条路。
// 黑石城六个 NPC 都不在；从秘市岸往北走，踩中部礁路口 trigger_feixu → 一句废墟旁白、不拦路（接着往北走得进北区）；
// 告示墙 trigger_xuanshang 只一句（ruin）。对照：推到 ch13.mingzhu（n14 走完，兽潮之前）——六人都在、
// 废墟那一格一句不响、告示墙说的不是 ruin。
TEST(Ch13Slice, TwelveAfterTheBeastTideTheCityIsEmptyAndTheRuinLineNeverBlocks) {
    const Walk& walk = firstWalk();
    const GameState reached = reachedAfter(walk, 30);
    ASSERT_EQ(reached.flag("ch13.shouchao"), 1);
    Driver driver;
    ASSERT_TRUE(driver.resumeReached(reached, 31)) << driver.problem();
    EXPECT_EQ(visibleOf(driver, kBlackstone), 0);
    const auto band = driver.object("trigger_feixu");
    ASSERT_FALSE(band.name.empty());
    driver.clearKeys();
    ASSERT_TRUE(driver.enter("trigger_feixu")) << driver.problem();
    EXPECT_EQ(driver.keys(), std::vector<std::string>{"ch13.feixu.ruin"});
    EXPECT_EQ(driver.state().mapId, "ch13_waihai");
    EXPECT_EQ(driver.state().position.y, band.position.y) << "停在废墟那一行，没被推回";
    const Point north{driver.state().position.x, band.position.y - 1};
    ASSERT_TRUE(driver.walk(north)) << "不拦路：从带上接着往北走";
    EXPECT_EQ(driver.state().position, north);
    driver.clearKeys();
    ASSERT_TRUE(driver.press("trigger_xuanshang")) << driver.problem();
    EXPECT_EQ(driver.keys(), std::vector<std::string>{"ch13.xuanshang.ruin"});

    Driver before;
    ASSERT_TRUE(before.resumeReached(reachedAfter(walk, 14), 15)) << before.problem();
    EXPECT_EQ(visibleOf(before, kBlackstone), 6);
    before.clearKeys();
    ASSERT_TRUE(before.walk(before.object("trigger_feixu").position, true)) << before.problem();
    EXPECT_TRUE(before.keys().empty()) << "兽潮之前废墟那一格不响";
    ASSERT_TRUE(before.press("trigger_xuanshang")) << before.problem();
    EXPECT_FALSE(before.spoke("ch13.xuanshang.ruin"));
    EXPECT_TRUE(before.spoke("ch13.xuanshang.short")) << "五级不足 3：告示墙说的是「不够」";
}

// ---- 验收 13：往返黑石城 ----
// 起点 ch12-end-first.sav。推到 ch13.lifu（n17 走完，雾中岛登岸）：码头尽头 trigger_qiyuan_hang 选「去」→ day ＋12、
// 落黑石城西门外礁 (7,12)；接着西门外礁头 trigger_hui_wuzhong 选「去」→ day ＋12、落 andeng (59,37)。
// 选「不去」→ 原地、日子不动。推到 ch13.chuguan（n28，biguan 已置）再踩码头 → 一句、mapId 不变；推到 ch13.lidao（n30）
// 从秘市岸走到西门外礁头再踩 → 一句、mapId 不变。
TEST(Ch13Slice, ThirteenTheRoundTripCostsTwelveDaysEachWayUntilTheRetreatAndTheDeparture) {
    const Walk& walk = firstWalk();
    Driver driver;
    ASSERT_TRUE(driver.resumeReached(reachedAfter(walk, 17), 18)) << driver.problem();
    int day = driver.state().day;
    driver.choose({1});
    ASSERT_TRUE(driver.enter("trigger_qiyuan_hang")) << driver.problem();
    EXPECT_EQ(driver.state().mapId, "ch13_wuzhong");
    EXPECT_EQ(driver.state().day, day);
    // 选「不去」之后人站在带上：先走开一格再踩回去。
    driver.choose({0});
    ASSERT_TRUE(driver.enter("trigger_qiyuan_hang")) << driver.problem();
    EXPECT_EQ(driver.state().mapId, "ch13_waihai");
    EXPECT_EQ(driver.state().position, (Point{7, 12}));
    EXPECT_EQ(driver.state().day, day + 12);
    day = driver.state().day;
    driver.choose({0});
    ASSERT_TRUE(driver.enter("trigger_hui_wuzhong")) << driver.problem();
    EXPECT_EQ(driver.state().mapId, "ch13_wuzhong");
    EXPECT_EQ(driver.state().position, (Point{59, 37}));
    EXPECT_EQ(driver.state().day, day + 12);

    // ch13.biguan 置后：推到 ch13.chuguan（n28 走完——闭关之后人在凹谷院里，出院必踩出关带，境界未到时那一格只说
    //「瓶颈未破」；取出关之后那一刻，biguan 仍置着）。
    Driver closed;
    ASSERT_TRUE(closed.resumeReached(reachedAfter(walk, 28), 29)) << closed.problem();
    ASSERT_EQ(closed.state().flag("ch13.biguan"), 1);
    day = closed.state().day;
    closed.clearKeys();
    ASSERT_TRUE(closed.enter("trigger_qiyuan_hang")) << closed.problem();
    EXPECT_EQ(closed.keys(), std::vector<std::string>{"ch13.qiyuan_hang.closed"});
    EXPECT_EQ(closed.state().mapId, "ch13_wuzhong");
    EXPECT_EQ(closed.state().day, day);

    Driver left;
    ASSERT_TRUE(left.resumeReached(reachedAfter(walk, 30), 31)) << left.problem();
    ASSERT_TRUE(left.enter("trigger_feixu")) << left.problem();
    day = left.state().day;
    left.clearKeys();
    ASSERT_TRUE(left.enter("trigger_hui_wuzhong")) << left.problem();
    EXPECT_EQ(left.keys(), std::vector<std::string>{"ch13.hui_wuzhong.closed"});
    EXPECT_EQ(left.state().mapId, "ch13_waihai");
    EXPECT_EQ(left.state().day, day);
}

// ---- 验收 27：NPC 节点前置与两处确认 ----
// 起点 ch12-end-first.sav。
// ① 推到 ch13.daoqiyuan（n13 走完，石屋门外）再把 ch13.daoqiyuan 抹回 0（扰动——n13 的带横贯石屋门外，
//    不踩它到不了黑石城，「chuansong 已置、daoqiyuan 未置还站在城里」只能这样造）：找明夫人 → 一句、mingzhu 0；
//    找离寻 → 一句、cunjin 0、背包无上古典籍玉简。
// ② 推到 ch13.daoqiyuan、不扰动、仍先找离寻 → 同上（n14 跳不过去）；对照：先找明夫人再找离寻 → 玉简到手。
// ③ n27、n30 的确认选「再等等 / 再留几日」或取消 → 旗标不置、日子不动（realmCap 仍 31、mapId 仍雾中岛）。
TEST(Ch13Slice, TwentySevenTheShopkeepersWaitForTheirTurnAndBothPointsOfNoReturnAskFirst) {
    const Walk& walk = firstWalk();
    const GameState landed = reachedAfter(walk, 13);
    ASSERT_EQ(landed.flag("ch13.daoqiyuan"), 1);
    for (const bool erased : {true, false}) {
        SCOPED_TRACE(erased ? "daoqiyuan erased" : "daoqiyuan set");
        Driver driver;
        ASSERT_TRUE(driver.resumeReached(landed, 14)) << driver.problem();
        if (erased) {
            driver.state().setFlag("ch13.daoqiyuan", 0);
            driver.clearKeys();
            ASSERT_TRUE(driver.press("npc_ming_furen")) << driver.problem();
            EXPECT_EQ(driver.keys(), std::vector<std::string>{"ch13.mingzhu.early"});
            EXPECT_EQ(driver.state().flag("ch13.mingzhu"), 0);
            EXPECT_TRUE(driver.shops().empty());
        }
        const int day = driver.state().day;
        driver.clearKeys();
        ASSERT_TRUE(driver.press("npc_li_xun")) << driver.problem();
        EXPECT_EQ(driver.keys(), std::vector<std::string>{"ch13.cunjin.early"});
        EXPECT_EQ(driver.state().flag("ch13.cunjin"), 0);
        EXPECT_EQ(driver.state().itemCount("story_guyu_yujian"), 0);
        EXPECT_EQ(driver.state().day, day);
    }
    Driver inOrder;
    ASSERT_TRUE(inOrder.resumeReached(landed, 14)) << inOrder.problem();
    inOrder.choose({0});
    ASSERT_TRUE(inOrder.press("npc_ming_furen")) << inOrder.problem();
    ASSERT_TRUE(inOrder.press("npc_li_xun")) << inOrder.problem();
    EXPECT_EQ(inOrder.state().flag("ch13.cunjin"), 1);
    EXPECT_EQ(inOrder.state().itemCount("story_guyu_yujian"), 1);

    // n27 闭关的确认：再等等（第二项）/ 取消 → 一样不动。
    for (const int pick : {1, -1}) {
        SCOPED_TRACE(pick);
        Driver driver;
        ASSERT_TRUE(driver.resumeReached(reachedAfter(walk, 26), 27)) << driver.problem();
        const GameState before = driver.state();
        driver.choose({pick});
        ASSERT_TRUE(driver.press("trigger_jingshimen")) << driver.problem();
        EXPECT_EQ(driver.state().flag("ch13.biguan"), 0);
        EXPECT_EQ(rules::toValue(driver.state().realmCap), 31);
        EXPECT_EQ(driver.state().day, before.day);
        EXPECT_EQ(driver.spoke("ch13.biguan.wait"), pick == 1);
        // 挂点没兑现（once 不算数）：再按、选就此闭关 → 置旗、上限 32。
        driver.choose({0});
        ASSERT_TRUE(driver.press("trigger_jingshimen")) << driver.problem();
        EXPECT_EQ(driver.state().flag("ch13.biguan"), 1);
        EXPECT_EQ(rules::toValue(driver.state().realmCap), 32);
    }
    // n30 离岛的确认：再留几日 / 取消 → 不置旗、不拨日、仍在雾中岛；走下带再踩、选这就走 → 秘市岸。
    for (const int pick : {1, -1}) {
        SCOPED_TRACE(pick);
        Driver driver;
        ASSERT_TRUE(driver.resumeReached(reachedAfter(walk, 29), 30)) << driver.problem();
        const GameState before = driver.state();
        driver.choose({pick});
        ASSERT_TRUE(driver.enter("trigger_lidao")) << driver.problem();
        EXPECT_EQ(driver.state().flag("ch13.lidao"), 0);
        EXPECT_EQ(driver.state().mapId, "ch13_wuzhong");
        EXPECT_EQ(driver.state().day, before.day);
        driver.choose({0});
        ASSERT_TRUE(driver.enter("trigger_lidao")) << driver.problem();
        EXPECT_EQ(driver.state().flag("ch13.lidao"), 1);
        EXPECT_EQ(driver.state().mapId, "ch13_waihai");
        EXPECT_EQ(driver.state().position, (Point{14, 46}));
    }
}

// ---- 8.2 ①「五行环一下打穿儒生的架势」（测试审查 T-3）----
// BattleHand 只列伤人的法术（power > 0 或带毒），五行环 power 0、effect stagger，通关与平衡里一发也没放过（技术债
// G-37）。这里由用例自己经真菜单放。起点 ch12-end-first.sav 走到 ch13.panzha（n04 走完：码头盘查之后、出海口带之前，
// ① 之前最近的可达点）；n05 踩出海口带进 ①，韩立第一手经真菜单「法术 → 祭五行环 → 白面儒生」：
//   五行环：这一手的类别是金（Act 事件）；儒生架势 3 → 0、当场破势（Break 事件）；不是一击——没有 Hit、气血不动；
//           韩立法力 −30（needMp）。
//   对照（祭金蚨）：同一局面、同一条菜单路径改放祭金蚨（金、伤人）：一击打中破绽「金」，架势只减 1、不破势、有伤害。
// 两版放之前的局面一字不差（出手记录、双方气血、法力、架势），只差这一手；之后都交还那只手打完，n05 走完（逐节点账照开）。
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

// 「法术 → magicId → target」，行号一律取场景自己的 build*Items（与 BattleHand::issue 同源）。成功返回空串，否则返回卡在哪一级。
std::string castByMenu(Driver& driver, BattleScene& scene, int actor, const char* magicId, int target) {
    Application& app = driver.app();
    scene.setBoost(0);
    scene.openMenu(app);
    if (scene.menuMode() != game::BattleMenuMode::Root) return "the battle menu did not open";
    if (!scene.menuChoose(app, game::kBattleMenuCast)) return "the cast row is greyed: " + scene.feedback();
    std::vector<std::string> ids;
    static_cast<void>(BattleScene::buildMagicItems(app.data(), app.state(), scene.battle(), actor, ids));
    const auto found = std::find(ids.begin(), ids.end(), magicId);
    if (found == ids.end()) return std::string(magicId) + " is not on the spell menu";
    if (!scene.menuChoose(app, static_cast<int>(found - ids.begin()))) return std::string(magicId) + " is greyed: " + scene.feedback();
    Action shape;
    shape.kind = ActionKind::Cast;
    shape.actorIndex = actor;
    shape.magicId = magicId;
    std::vector<int> indices;
    static_cast<void>(BattleScene::buildTargetItems(app.state(), scene.battle(), shape, indices));
    const auto row = std::find(indices.begin(), indices.end(), target);
    if (row == indices.end() || !scene.menuChoose(app, static_cast<int>(row - indices.begin())))
        return "the target row refused the spell: " + scene.feedback();
    return {};
}

struct OpeningCast {
    std::string problem;
    std::vector<std::string> prefix;   // 放这一手之前的出手记录
    int heroHp = -1, mpBefore = -1, mpAfter = -1;
    int scholarHpBefore = -1, scholarHpAfter = -1;
    int toughnessBefore = -1, maxToughness = -1, toughnessAfter = -1;
    bool brokenBefore = true, brokenAfter = false;
    int actCategory = -1;              // 这一手 Act 事件的类别（kCategory*）
    int staggerCut = -1;               // Stagger 事件削掉的点数；-1 ＝ 没有这条事件
    int hits = 0;                      // 打在儒生身上的 Hit 事件条数
    int hitCategory = -1;
    bool breakEvent = false;
};

void openWithSpell(Driver& driver, const Walk& walk, const char* spell, OpeningCast& run) {
    ASSERT_TRUE(driver.resumeReached(reachedAfter(walk, 4), 5, &kFirst)) << driver.problem();
    driver.nextModes = {FightMode::Custom};
    driver.customFight = [&driver, &run, spell](BattleScene& scene, test::BattleHand& hand) {
        const int scholar = unitIndexOf(scene, "nixing_rusheng");
        const int actor = scene.runToAllyTurn();
        if (scholar < 0 || actor < 0) { run.problem = "no scholar on the field or no turn for Han Li"; return false; }
        const auto unit = [&scene](int i) { return scene.battle().units()[static_cast<std::size_t>(i)]; };
        run.prefix = scene.battle().log();
        run.heroHp = unit(actor).hp;
        run.mpBefore = unit(actor).mp;
        run.scholarHpBefore = unit(scholar).hp;
        run.toughnessBefore = unit(scholar).toughness;
        run.maxToughness = unit(scholar).maxToughness;
        run.brokenBefore = unit(scholar).broken();
        const std::size_t from = scene.battle().events().size();
        if (std::string why = castByMenu(driver, scene, actor, spell, scholar); !why.empty()) { run.problem = why; return false; }
        run.mpAfter = unit(actor).mp;
        run.scholarHpAfter = unit(scholar).hp;
        run.toughnessAfter = unit(scholar).toughness;
        run.brokenAfter = unit(scholar).broken();
        const auto& events = scene.battle().events();
        for (std::size_t i = from; i < events.size(); ++i) {
            const BattleEvent& e = events[i];
            if (e.kind == BattleEventKind::Act && e.actor == actor && run.actCategory < 0) run.actCategory = e.category;
            if (e.target != scholar) continue;
            if (e.kind == BattleEventKind::Stagger && e.actor == actor) run.staggerCut = e.value;
            if (e.kind == BattleEventKind::Break) run.breakEvent = true;
            if (e.kind == BattleEventKind::Hit && e.actor == actor) { ++run.hits; run.hitCategory = e.category; }
        }
        hand.play(scene);
        return scene.battle().phase() != BattlePhase::Ongoing;
    };
    ASSERT_TRUE(driver.runTo(5)) << driver.problem() << ' ' << run.problem;
}

TEST(Ch13Slice, FourTheRingFromTheMenuBreaksTheScholarAtOnceWhereTheGoldBladeOnlyChipsOne) {
    const Walk& walk = firstWalk();
    ASSERT_EQ(reachedAfter(walk, 4).flag("ch13.panzha"), 1);
    Driver ringDriver, goldDriver;
    OpeningCast ring, gold;
    openWithSpell(ringDriver, walk, "magic_ji_wuxinghuan", ring);
    openWithSpell(goldDriver, walk, "magic_ji_jinfu", gold);
    for (const auto& pair : {std::pair{&ringDriver, &ring}, std::pair{&goldDriver, &gold}}) {
        SCOPED_TRACE(pair.second == &ring ? "ring" : "gold blade");
        ASSERT_TRUE(pair.second->problem.empty()) << pair.second->problem;
        ASSERT_EQ(pair.first->fights().size(), 1u);
        EXPECT_EQ(pair.first->fights()[0].id, kBattles[0]);
        EXPECT_EQ(pair.first->fights()[0].phase, BattlePhase::Won);
        EXPECT_EQ(pair.first->state().flag("ch13.zhuibing"), 1);
        EXPECT_EQ(pair.second->toughnessBefore, 3);
        EXPECT_EQ(pair.second->maxToughness, 3);
        EXPECT_FALSE(pair.second->brokenBefore);
        EXPECT_EQ(pair.second->actCategory, core::kCategoryMetal) << "这一手的类别是金";
    }
    // 对照与被测是同一个局面：放之前的出手记录、韩立气血与法力、儒生气血一字不差。
    EXPECT_EQ(ring.prefix, gold.prefix);
    EXPECT_EQ(ring.heroHp, gold.heroHp);
    EXPECT_EQ(ring.mpBefore, gold.mpBefore);
    EXPECT_EQ(ring.scholarHpBefore, gold.scholarHpBefore);
    // 五行环：一下削 3、架势到 0、当场破势；不伤人、法力 −30。
    EXPECT_EQ(ring.staggerCut, 3);
    EXPECT_EQ(ring.toughnessAfter, 0);
    EXPECT_TRUE(ring.brokenAfter);
    EXPECT_TRUE(ring.breakEvent);
    EXPECT_EQ(ring.hits, 0);
    EXPECT_EQ(ring.scholarHpAfter, ring.scholarHpBefore);
    EXPECT_EQ(ring.mpBefore - ring.mpAfter, 30);
    // 祭金蚨：一击打中破绽「金」、架势只减 1、不破势、有伤害；法力 −12。
    EXPECT_EQ(gold.staggerCut, -1);
    EXPECT_EQ(gold.hits, 1);
    EXPECT_EQ(gold.hitCategory, core::kCategoryMetal);
    EXPECT_EQ(gold.toughnessAfter, 2);
    EXPECT_FALSE(gold.brokenAfter);
    EXPECT_FALSE(gold.breakEvent);
    EXPECT_LT(gold.scholarHpAfter, gold.scholarHpBefore);
    EXPECT_EQ(gold.mpBefore - gold.mpAfter, 12);
    std::cout << "[ch13 ring] ring toughness " << ring.toughnessBefore << "->" << ring.toughnessAfter << " mp "
              << ring.mpBefore << "->" << ring.mpAfter << " | gold toughness " << gold.toughnessBefore << "->"
              << gold.toughnessAfter << " hp " << gold.scholarHpBefore << "->" << gold.scholarHpAfter << " rounds "
              << ringDriver.fights()[0].rounds << " / " << goldDriver.fights()[0].rounds << '\n';
}
} // namespace
