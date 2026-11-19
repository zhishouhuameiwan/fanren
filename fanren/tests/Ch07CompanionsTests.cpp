// 伙伴改版样板段：第 7 章血色禁地（docs/ch07-companions-design.md，下称「样板施工图」；引擎契约
// docs/interfaces-companions.md，下称「契约」）。
//
// 样板施工图第 10 节「自动的」九条，落在哪里：
//   1 两侧从 ch06-end 走完全章、最小路线与全收集路线：Ch07AcceptanceTests（两侧最小路线的两条通关、两侧全收集的两条）；
//   2 目标链逐步走门（含新三步）：ObjectiveTests 的 ObjectiveRoute 三条（沿每章目标链走门）；「机关关着、所有明雷在场时
//     主线走得通」：Ch07AcceptanceTests 两侧最小路线的通关——石窟那道残禁从头到尾没开，十二只明雷一只没打，寻路认明雷
//     面前那一格当墙（踏进去就开打）、认机关挡路，照样走到 ch07.done；静态的那一半是门禁规则 34、36；
//   3 三场仗用 BattleHand 量：本文件 No3_* 三条（⑤ 没有追击、精英有追击、破顶不用物品能赢 / 烧光她的法力就输）；
//   4 默契账：Ch07AcceptanceTests 的通关里逐步记账（最小路线 37、全收集 ≥ 45、评银剑演完已到二档、同一天并肩 ≤ 6）；
//   5 最小路线的 ch07-end 与改版前那一份逐字段比：本文件 No5_*；
//   6 历练进出一次的各项：本文件 No6_*；
//   7 她在队里时图上没有第二个她：本文件 No7_*（第 7、8 章各一条，另核第 8–13 章的图上没有人因此少了）；
//   8 v9 档读得回（CompanionsBaseDataTests 的全部交接档往返）、四道门禁、原创性机器口径（交付记录）；
//   9 随之改写的旧测试：交付记录逐条列出。
//
// 判据只从样板施工图与契约推，不从被测的脚本和数据里抄（handoff 第 6 节第 5 条）。
#include <gtest/gtest.h>

#include <SDL3/SDL.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "../vendor/json.hpp"

#include "BattleHand.h"
#include "ChapterFixture.h"
#include "TempDir.h"
#include "core/battle/Battle.h"
#include "core/model/Types.h"
#include "core/rules/Companions.h"
#include "core/rules/Realm.h"
#include "game/Application.h"
#include "game/BattleScene.h"
#include "game/LilianPartyScene.h"
#include "game/MenuScene.h"
#include "game/WorldObjects.h"
#include "game/WorldScene.h"
#include "game/WorldView.h"
#include "io/SaveFile.h"
#include "script/Command.h"

namespace {

namespace fs = std::filesystem;
using fanren::core::GameState;
using fanren::game::Application;
using fanren::rules::Realm;

std::string assetRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        const fs::path root(candidate);
        if (fs::exists(root / "scripts" / "ch07" / "wenming.lua") && fs::exists(root / "maps" / "ch07_dixia_shiku.tmj")) {
            return candidate;
        }
    }
    return ".";
}

constexpr const char* kNangong = "nangong_wan";
constexpr const char* kChen = "chen_qiaoqian";

class Ch07CompanionsData : public ::testing::Test {
protected:
    void SetUp() override {
        auto ready = app_.init(assetRoot(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
    }
    void TearDown() override { app_.shutdown(); }

    std::string current(const std::string& companion) {
        return fanren::rules::currentRole(app_.data().companions, companion, app_.state());
    }

    Application app_;
};

}  // namespace

// 样板施工图第 9 节「伙伴数据两份」、第 7 节「她在历练里是谁」：南宫婉的三段角色——问名之前是白衣少女，
// 问名之后到第 7 章完是新建的 nangong_wan_ch07，第 8 章起才是 nangong_wan。经 Application 读进来的真数据问。
TEST_F(Ch07CompanionsData, NangongWanIsTheGirlInWhiteThenTheChapterSevenNangongThenTheLaterOne) {
    const auto& companions = app_.data().companions;
    ASSERT_EQ(companions.count(kNangong), 1u) << "样板施工图第 9 节：data/companions/nangong_wan.json";
    ASSERT_EQ(companions.count(kChen), 1u) << "样板施工图第 9 节：data/companions/chen_qiaoqian.json";
    EXPECT_EQ(companions.size(), 2u) << "样板段只有这两位伙伴";

    GameState& s = app_.state();
    EXPECT_EQ(current(kNangong), "baiyi_shaonv") << "问名之前（ch07.nangong 没置）";
    s.setFlag("ch07.nangong");
    EXPECT_EQ(current(kNangong), "nangong_wan_ch07") << "问名之后、第 7 章结束之前";
    s.setFlag("ch07.done");
    EXPECT_EQ(current(kNangong), "nangong_wan") << "第 8 章起";

    for (const char* role : {"baiyi_shaonv", "nangong_wan_ch07", "nangong_wan"}) {
        EXPECT_EQ(fanren::rules::companionOfRole(companions, role), kNangong) << role << " 是南宫婉的一段";
    }
    EXPECT_EQ(fanren::rules::companionOfRole(companions, "chen_shimei"), kChen);
    EXPECT_EQ(fanren::rules::companionOfRole(companions, "chen_qiaoqian"), "")
        << "样板施工图第 9 节：陈师妹的 roles 现在只列 chen_shimei，名字到第 8 章才补";

    // 名册由旗标推出（契约 1、2.1）：南宫婉问名之后、陈师妹是第 7 章陆师兄那件事之后（ch07.fanhui）。
    EXPECT_EQ(companions.at(kNangong).metFlag, "ch07.nangong");
    EXPECT_EQ(companions.at(kChen).metFlag, "ch07.fanhui");
    // 样板施工图第 7 节「机关：带谁就开得了谁的」：两人的场地技不同。
    EXPECT_FALSE(companions.at(kNangong).fieldSkill.empty());
    EXPECT_FALSE(companions.at(kChen).fieldSkill.empty());
    EXPECT_NE(companions.at(kNangong).fieldSkill, companions.at(kChen).fieldSkill);
}

// 样板施工图第 7 节：「问名之后、第 7 章结束之前，用新建的角色表 nangong_wan_ch07：名字是南宫婉，境界和法术与
// baiyi_shaonv 相同（炼气十三层、朱雀环）」。
TEST_F(Ch07CompanionsData, TheChapterSevenNangongIsTheGirlInWhiteWithHerName) {
    const auto* girl = app_.data().findRole("baiyi_shaonv");
    const auto* named = app_.data().findRole("nangong_wan_ch07");
    ASSERT_NE(girl, nullptr);
    ASSERT_NE(named, nullptr);
    EXPECT_EQ(girl->name, "白衣少女") << "样板施工图第 2 节约束 1：问名之前的名签不写南宫婉";
    EXPECT_EQ(named->name, "南宫婉");
    EXPECT_EQ(named->realm, Realm::QiRefining13) << "炼气十三层（约束 2：法力被封在炼气顶峰）";
    EXPECT_EQ(named->realm, girl->realm);
    EXPECT_EQ(named->magics, (std::vector<std::string>{"magic_zhuque_huan"})) << "约束 3：朱雀环";
    EXPECT_EQ(named->magics, girl->magics);
    EXPECT_EQ(named->maxHp, girl->maxHp);
    EXPECT_EQ(named->maxMp, girl->maxMp);
    EXPECT_EQ(named->attack, girl->attack);
    EXPECT_EQ(named->defence, girl->defence);
    EXPECT_EQ(named->speed, girl->speed);
}

// ===========================================================================
// 切片：从第一侧最小路线沿途的检查点起步（tests/fixtures/ch07-*-first.sav，Ch07AcceptanceTests 的第一侧通关
// 写出、平时逐字段比），走真入口：世界层的走与按、脚本、战斗场景、组队面板、按键、主菜单。
// 资产根拷一份到临时目录（存档、设置都落在那里，不碰仓库的 saves/）。
// ===========================================================================
namespace {

using fanren::core::Point;
using fanren::core::battle::BattleEventKind;
using fanren::core::battle::BattlePhase;
using fanren::core::battle::Unit;
using fanren::game::BattleScene;
using fanren::game::LilianPartyScene;
using fanren::game::MenuScene;
using fanren::game::WorldScene;

constexpr int kUp = 0;
constexpr int kDown = 2;

void pushKey(SDL_Scancode scancode, bool down) {
    SDL_Event event{};
    event.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
    event.key.scancode = scancode;
    event.key.down = down;
    SDL_PushEvent(&event);
}

// 一场仗打下来看到的。
struct Fight {
    std::string id;
    BattlePhase phase = BattlePhase::Ongoing;
    int rounds = 0;
    int chases = 0;
    bool preemptive = false;
    int herMpLeft = -1;      // 她收场时的法力（她不在场 -1）
    int herRingCost = 0;     // 朱雀环一环要多少法力
};

class Ch07CompanionsSlice : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        root_ = std::make_unique<fanren::test::TempDir>("fanren_ch07_companions");
        for (const char* dir : {"data", "scripts", "maps"}) {
            fs::copy(fs::path(assetRoot()) / dir, root_->path() / dir, fs::copy_options::recursive);
        }
        fs::create_directories(root_->path() / "saves");
    }
    static void TearDownTestSuite() { root_.reset(); }

    void SetUp() override {
        auto ready = app_.init(root_->path().string(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
    }
    void TearDown() override { app_.shutdown(); }

    void tick(int frames = 1) {
        for (int i = 0; i < frames; ++i) app_.tick(1.0 / 60.0);
    }
    void press(SDL_Scancode scancode) {
        pushKey(scancode, true);
        tick();
        pushKey(scancode, false);
        tick();
    }
    std::string top() {
        const fanren::game::Scene* scene = app_.topScene();
        return scene == nullptr ? std::string{} : scene->name();
    }
    GameState& state() { return app_.state(); }
    WorldScene* world() { return dynamic_cast<WorldScene*>(app_.topScene()); }

    // 读一份检查点，原样当作这一局（不改字段），站回存档里那一格，压世界层。
    void startFrom(const char* fixture) {
        const auto loaded = fanren::io::loadGame(fanren::test::chapterFixturePath(assetRoot(), fixture).string());
        ASSERT_TRUE(loaded.ok) << fixture << "：" << loaded.error
                               << "（由 Ch07AcceptanceTests 第一侧通关写出：FANREN_WRITE_CH07_FIXTURES=1）";
        const GameState s = loaded.value;
        state() = s;
        ASSERT_TRUE(app_.loadMap(s.mapId, std::string{}).ok);
        state().position = s.position;
        state().facing = s.facing;
        app_.pushScene(std::make_unique<WorldScene>());
        tick();
        ASSERT_EQ(top(), "World");
    }
    // 换到同一局里的另一张图上某一格（剧情传送那样：只挪人，局面不动）。
    void standAt(const std::string& mapId, Point at, int facing) {
        ASSERT_TRUE(app_.loadMap(mapId, std::string{}).ok) << mapId;
        state().position = at;
        state().facing = facing;
        app_.replaceScene(std::make_unique<WorldScene>());
        tick();
        ASSERT_EQ(top(), "World");
    }

    // 世界层按一下确认（与玩家同一条路：WorldScene::interact），然后把压上来的东西都走完：
    // 脚本的对白一路按确认，选择取第一项；仗交给那只手。返回这一下里打过的仗。
    std::vector<Fight> interactAndPlay(const fanren::test::HandPolicy& policy) {
        std::vector<Fight> fights;
        app_.clearSpokenKeys();
        EXPECT_NE(world(), nullptr);
        if (world() == nullptr) return fights;
        EXPECT_TRUE(world()->interact(app_)) << "按了没反应";
        for (int frame = 0; frame < 12000; ++frame) {
            tick();
            if (auto* scene = dynamic_cast<BattleScene*>(app_.topScene()); scene != nullptr) {
                if (scene->battle().phase() != BattlePhase::Ongoing) continue;
                fights.push_back(playOut(*scene, policy));
                continue;
            }
            if (app_.awaitingCommand()) {
                spoken_.insert(spoken_.end(), app_.spokenKeys().begin(), app_.spokenKeys().end());
                app_.clearSpokenKeys();
                fanren::script::CommandResult result;
                result.ok = true;
                result.choiceIndex = 0;
                app_.completeCommand(result);
                app_.popScene();
                continue;
            }
            if (!app_.scripts().isRunning() && top() == "Dialogue") {
                app_.popScene();
                continue;
            }
            if (!app_.scripts().isRunning() && top() == "World") break;
        }
        EXPECT_FALSE(app_.scripts().isRunning()) << "脚本没能跑到结束";
        spoken_.insert(spoken_.end(), app_.spokenKeys().begin(), app_.spokenKeys().end());
        return fights;
    }

    Fight playOut(BattleScene& scene, const fanren::test::HandPolicy& policy) {
        Fight f;
        f.id = app_.battleStart().battleId;
        f.preemptive = scene.battle().preemptive();
        fanren::test::BattleHand hand(app_, policy);
        f.phase = hand.play(scene);
        f.rounds = scene.battle().round();
        for (const auto& event : scene.battle().events()) {
            if (event.kind == BattleEventKind::Chase) ++f.chases;
        }
        for (const Unit& u : scene.battle().units()) {
            if (u.ally && u.id != "hanli") f.herMpLeft = u.mp;
        }
        if (const auto* ring = app_.data().findMagic("magic_zhuque_huan")) f.herRingCost = ring->needMp;
        if (fanren::test::environmentFlagSet("FANREN_CH07_BATTLE_LOG")) {
            std::cout << "---- " << f.id << " ----" << std::endl;
            for (const std::string& line : scene.battle().log()) std::cout << "  " << line << std::endl;
        }
        return f;
    }

    static fanren::test::HandPolicy intentPolicy() {
        fanren::test::HandPolicy p;
        p.healAtPercent = 50;
        p.healWhenDoomed = false;
        p.fleeWhenSpent = false;
        p.pills = {"pill_yangjing_dan", "pill_jinchuang_yao"};
        return p;
    }

    bool said(const std::string& key) const { return std::find(spoken_.begin(), spoken_.end(), key) != spoken_.end(); }

    static inline std::unique_ptr<fanren::test::TempDir> root_;
    Application app_;
    std::vector<std::string> spoken_;
};

}  // namespace

// 样板施工图第 10 节「你亲手玩的」从禁地入口的检查点开始：saves/ch07-jindi.sav 读得回，是第一侧刚进外围那一刻。
TEST_F(Ch07CompanionsSlice, TheHandPlayCheckpointIsTheEntranceOfTheForbiddenGround) {
    const auto handPlay = fanren::io::loadGame((fs::path(assetRoot()) / "saves" / "ch07-jindi.sav").string());
    ASSERT_TRUE(handPlay.ok) << handPlay.error;
    const auto fixture =
        fanren::io::loadGame(fanren::test::chapterFixturePath(assetRoot(), "companions/ch07-jindi-first.sav").string());
    ASSERT_TRUE(fixture.ok) << fixture.error;
    EXPECT_EQ(handPlay.value.mapId, "ch07_jindi_waiwei") << "禁地外围的落点";
    EXPECT_EQ(handPlay.value.flag("ch07.pojin"), 1);
    EXPECT_EQ(handPlay.value.flag("ch07.wulongtan"), 0) << "外围的事一件还没演";
    EXPECT_TRUE(fanren::test::saveLineDifferences(fanren::test::comparableSaveLines(fixture.value),
                                                  fanren::test::comparableSaveLines(handPlay.value))
                    .empty())
        << "saves/ch07-jindi.sav 与通关测试比着的那一份是同一个局面";
}

// 验收 3（精英）：评银剑之后（默契 22，二档）带她去沼泽深处，从侧面摸上碧水鳄——照意图打能赢，事件流里有追击。
TEST_F(Ch07CompanionsSlice, No3_TheEliteCrocodileFallsToTheIntentPlayerAndTheFightHasAChase) {
    startFrom("companions/ch07-kundi-first.sav");
    ASSERT_FALSE(HasFatalFailure());
    ASSERT_TRUE(state().partyHas("baiyi_shaonv")) << "先验：她在队里";
    ASSERT_EQ(fanren::rules::bondTier(state().bondOf(kNangong)), 2) << "先验：评银剑演完到二档";
    // 碧水鳄在 (33,26) 面朝左；它北边 (33,25) 是侧面。
    standAt("ch07_zhaoze_shenchu", Point{33, 25}, kDown);
    ASSERT_FALSE(HasFatalFailure());
    const std::vector<Fight> fights = interactAndPlay(intentPolicy());
    ASSERT_EQ(fights.size(), 1u) << "面朝它按确认就开打";
    EXPECT_EQ(fights[0].id, "be07_bishui_e");
    EXPECT_TRUE(fights[0].preemptive) << "从侧面上去是偷袭（契约 3.4）";
    EXPECT_EQ(fights[0].phase, BattlePhase::Won) << "样板施工图第 6 节：意图玩家打得赢";
    EXPECT_GT(fights[0].chases, 0) << "样板施工图第 6 节 / 验收 3：追击的考场在这里";
    EXPECT_EQ(state().worldObjects.clearedFoes.count("ch07_zhaoze_shenchu.foe_bishui_e"), 1u) << "打赢了记档";
}

// 样板施工图 4.3 / 4.6：火灵石那一步「查背包、收走、加默契、置旗标」。火灵石也在她合意的赠礼里，玩家可能先送掉了——
// 身上没有就只说一句、不置旗标、门照样关着；再带一块来踩上去才交（默契 +5 只加这一回）。
TEST_F(Ch07CompanionsSlice, TheFireStoneStepWithoutAStoneSaysOneLineAndWaitsForTheNextTime) {
    startFrom("companions/ch07-kundi-first.sav");
    ASSERT_FALSE(HasFatalFailure());
    while (state().removeItem("material_huolingshi", 1)) {
    }
    const int bond = state().bondOf(kNangong);
    standAt("ch07_zhaoze_shenchu", Point{39, 22}, 1);
    ASSERT_FALSE(HasFatalFailure());
    const auto stepAndPlay = [&](int dx) {
        spoken_.clear();
        app_.clearSpokenKeys();
        EXPECT_TRUE(world()->tryStep(app_, dx, 0));
        for (int frame = 0; frame < 6000 && (app_.scripts().isRunning() || app_.awaitingCommand()); ++frame) {
            tick();
            if (app_.awaitingCommand()) {
                spoken_.insert(spoken_.end(), app_.spokenKeys().begin(), app_.spokenKeys().end());
                app_.clearSpokenKeys();
                fanren::script::CommandResult result;
                result.ok = true;
                app_.completeCommand(result);
                app_.popScene();
            }
        }
        tick();
    };
    stepAndPlay(1);   // (40,22)：岩缝里那一格
    EXPECT_TRUE(said("ch07.huolingshi.none")) << "身上没有火灵石：她只说一句";
    EXPECT_FALSE(said("ch07.huolingshi.give"));
    EXPECT_EQ(state().flag("ch07.huolingshi"), 0) << "不置旗标";
    EXPECT_EQ(state().bondOf(kNangong), bond);
    stepAndPlay(-1);
    state().addItem("material_huolingshi", 1, 0);
    stepAndPlay(1);
    EXPECT_TRUE(said("ch07.huolingshi.give")) << "带着火灵石再踩上去，这回交了";
    EXPECT_EQ(state().flag("ch07.huolingshi"), 1);
    EXPECT_EQ(state().itemCount("material_huolingshi"), 0) << "收走一枚";
    EXPECT_EQ(state().bondOf(kNangong), bond + 5) << "样板施工图 4.6：火灵石 +5";
}

// 验收 3（破顶）：不用物品能赢；赢了碎石塌落、退开一步，再往上一迈就是问名。
TEST_F(Ch07CompanionsSlice, No3_BreakingTheCeilingIsWonWithoutItemsAndLeavesHimOneStepBelowTheHole) {
    startFrom("companions/ch07-kundi-first.sav");
    ASSERT_FALSE(HasFatalFailure());
    state().setFlag("ch07.huolingshi");   // 火灵石那一步已交（4.3）
    standAt("ch07_dixia_shiku", Point{21, 3}, kUp);
    ASSERT_FALSE(HasFatalFailure());
    const std::map<std::string, int> bag = [&] {
        std::map<std::string, int> m;
        for (const auto& stack : state().bag) m[stack.itemId] += stack.count;
        return m;
    }();
    fanren::test::HandPolicy noItems = intentPolicy();
    noItems.pills.clear();
    noItems.poisons.clear();
    const std::vector<Fight> fights = interactAndPlay(noItems);
    ASSERT_EQ(fights.size(), 1u);
    EXPECT_EQ(fights[0].id, "b07_poding");
    EXPECT_EQ(fights[0].phase, BattlePhase::Won) << "样板施工图第 6 节：那只手不许用物品，不用物品赢不了就算设计没达标";
    // 背包里除了打穿时耗尽的那块金光砖，一样也没少、一样也没多（这一仗不掉东西）。
    std::map<std::string, int> after;
    for (const auto& stack : state().bag) after[stack.itemId] += stack.count;
    std::map<std::string, int> expected = bag;
    if (--expected["talisman_jinguangzhuan"] <= 0) expected.erase("talisman_jinguangzhuan");
    EXPECT_EQ(after, expected) << "破顶不用物品、修为 0、不掉东西：背包只少那块金光砖";
    EXPECT_EQ(state().flag("ch07.poding"), 1);
    EXPECT_EQ(state().position, (Point{21, 4})) << "碎石塌落、退开一步";
    EXPECT_EQ(state().itemCount("talisman_jinguangzhuan"), 0) << "金光砖耗尽成了废纸（约束 7）";
    // 再往上一迈：问名（4.5）。
    ASSERT_NE(world(), nullptr);
    ASSERT_TRUE(world()->tryStep(app_, 0, -1));
    for (int frame = 0; frame < 6000 && (app_.scripts().isRunning() || app_.awaitingCommand()); ++frame) {
        tick();
        if (app_.awaitingCommand()) {
            fanren::script::CommandResult result;
            result.ok = true;
            app_.completeCommand(result);
            app_.popScene();
        }
    }
    EXPECT_EQ(state().flag("ch07.nangong"), 1) << "问出了名字";
    EXPECT_FALSE(state().partyHas("baiyi_shaonv")) << "4.5：她离队";
    EXPECT_EQ(state().mapId, "ch07_huanxingshan") << "出到地面";
}

// 验收 3（破顶）：把她的法力烧光则输——韩立一门法术也不放（金光砖不砸、火弹不连发，岩层的架势就削不动），
// 只剩她的朱雀环在烧；岩层只认火（killable_by），她的法力见底之后再也烧不穿，碎石把两人砸倒。
// 输了不致命：回石窟的打坐点，挂点还备着，可以再来。
TEST_F(Ch07CompanionsSlice, No3_BurningHerManaOutLosesTheCeilingFightAndSendsThemBackToTheCushion) {
    startFrom("companions/ch07-kundi-first.sav");
    ASSERT_FALSE(HasFatalFailure());
    state().setFlag("ch07.huolingshi");
    standAt("ch07_dixia_shiku", Point{21, 3}, kUp);
    ASSERT_FALSE(HasFatalFailure());
    fanren::test::HandPolicy wasteful = intentPolicy();
    wasteful.pills.clear();
    wasteful.withheldMagics = state().learnedMagics;   // 韩立会的每一门都捂着
    const std::vector<Fight> fights = interactAndPlay(wasteful);
    ASSERT_EQ(fights.size(), 1u);
    EXPECT_EQ(fights[0].id, "b07_poding");
    EXPECT_EQ(fights[0].phase, BattlePhase::Lost) << "只靠她一个人的法力烧不穿";
    EXPECT_GE(fights[0].herMpLeft, 0) << "先验：她在场";
    EXPECT_LT(fights[0].herMpLeft, fights[0].herRingCost) << "输的时候她的法力已经烧光（连一环都放不出）";
    EXPECT_FALSE(app_.quitRequested()) << "不会死人（defeat_is_fatal false）";
    EXPECT_EQ(state().flag("ch07.poding"), 0);
    EXPECT_EQ(state().mapId, "ch07_dixia_shiku");
    EXPECT_EQ(state().position, (Point{24, 15})) << "回到石窟的打坐点旁（样板施工图 5.1）";
    EXPECT_GE(state().hp, 1);
    EXPECT_GT(state().itemCount("talisman_jinguangzhuan"), 0) << "没打穿，金光砖还在";
}

// ===========================================================================
// 验收 6：历练「禁地旧境」进出（样板施工图第 7 节；契约 3.12）。起点：第一侧出了禁地、回到百药园那一刻
// （ch07.chujindi 置了，第 7 章还没完：她此刻该用 nangong_wan_ch07）。
// ===========================================================================
namespace {

class Ch07CompanionsLilian : public Ch07CompanionsSlice {
protected:
    LilianPartyScene* panel() { return dynamic_cast<LilianPartyScene*>(app_.topScene()); }
    std::string lastLine() const {
        const auto& log = app_.dialogueLog();
        return log.empty() ? std::string{} : log.back().body;
    }
    // 走到茅屋里那处设施跟前（它在 (25,3)，人站在 (25,4) 面朝上），按确认、在面板上挑人、出发。
    void enterWith(const std::set<std::string>& roles) {
        standAt("ch06_baiyaoyuan", Point{25, 4}, kUp);
        ASSERT_FALSE(HasFatalFailure());
        press(SDL_SCANCODE_RETURN);
        tick();
        ASSERT_EQ(top(), "LilianParty") << "样板施工图 5.3：茅屋里那处设施弹组队面板";
        ASSERT_NE(panel(), nullptr);
        const std::size_t roster = app_.data().companions.size();
        for (std::size_t row = 0; row < roster; ++row) {
            const auto before = panel()->chosenRoles();
            press(SDL_SCANCODE_RETURN);
            const auto after = panel()->chosenRoles();
            if (after.size() > before.size() && roles.count(after.back()) == 0) press(SDL_SCANCODE_RETURN);   // 放下
            press(SDL_SCANCODE_DOWN);
        }
        const auto chosen = panel()->chosenRoles();
        ASSERT_EQ(std::set<std::string>(chosen.begin(), chosen.end()), roles);
        press(SDL_SCANCODE_RETURN);   // 末行：出发
        tick(2);
        ASSERT_TRUE(app_.inLilian());
        ASSERT_EQ(top(), "World");
    }
    // 面朝出口按确认出来，再按掉那句「归来」。
    void leaveByExit(const std::string& mapId, Point stand, int facing) {
        standAt(mapId, stand, facing);
        ASSERT_FALSE(HasFatalFailure());
        press(SDL_SCANCODE_RETURN);
        tick();
        ASSERT_FALSE(app_.inLilian());
        ASSERT_EQ(top(), "Dialogue");
        press(SDL_SCANCODE_RETURN);
        tick(2);
    }
    bool quickSaveOnDisk() const { return fs::exists(app_.quickSavePath()); }
    // 面朝一道机关按确认，合上冒出来的那句话。返回这道机关此刻开没开（这一趟的账）。
    bool tryGate(const std::string& mapId, Point stand, int facing, const std::string& gateId) {
        standAt(mapId, stand, facing);
        if (HasFatalFailure()) return false;
        press(SDL_SCANCODE_RETURN);
        tick(30);
        while (top() == "Dialogue") press(SDL_SCANCODE_RETURN);
        return app_.worldObjects().openedGates.count(gateId) != 0;
    }
};

}  // namespace

TEST_F(Ch07CompanionsLilian, No6_ATripThroughTheOldForbiddenGround) {
    startFrom("companions/ch07-chujindi-first.sav");
    ASSERT_FALSE(HasFatalFailure());
    ASSERT_EQ(state().flag("ch07.chujindi"), 1) << "先验：出了禁地";
    ASSERT_EQ(state().flag("ch07.done"), 0) << "先验：第 7 章还没完";
    ASSERT_TRUE(state().party.empty()) << "先验：问名之后她已离队";
    const int cultivation = state().cultivation;
    const int day = state().day;
    const auto chestsBefore = state().worldObjects.openedChests;
    const auto foesBefore = state().worldObjects.clearedFoes;
    const auto gatheredBefore = state().worldObjects.gatheredDay;

    // ---- 第一趟：只带南宫婉 ----
    enterWith({"nangong_wan_ch07"});
    ASSERT_FALSE(HasFatalFailure());
    EXPECT_EQ(state().mapId, "lilian_jindi_waiwei") << "样板施工图第 7 节：入口是外围的副本";
    ASSERT_EQ(state().party.size(), 1u);
    const auto* her = app_.data().findRole(state().party[0].roleId);
    ASSERT_NE(her, nullptr);
    EXPECT_EQ(her->name, "南宫婉") << "问名之后进历练，她的名字是南宫婉";
    EXPECT_EQ(her->magics, (std::vector<std::string>{"magic_zhuque_huan"})) << "会朱雀环";
    EXPECT_EQ(her->realm, app_.data().findRole("baiyi_shaonv")->realm) << "境界与主线里相同（炼气十三层）";

    // 里面三处存档都被拒（契约 3.12）：快捷键、主菜单、存档点设施（历练图上没摆存档点，拿一处设施对象直接走
    // Application::openFacility——存档点按确认走的就是它）。
    press(SDL_SCANCODE_F5);
    EXPECT_FALSE(quickSaveOnDisk()) << "快捷键存不了";
    app_.pushScene(std::make_unique<MenuScene>(MenuScene::Page::Save));
    tick(30);
    ASSERT_EQ(top(), "Menu");
    press(SDL_SCANCODE_RETURN);
    EXPECT_FALSE(quickSaveOnDisk()) << "主菜单存不了";
    app_.popScene();
    tick();
    fanren::core::MapObject stone;
    stone.name = "probe_save";
    stone.type = "facility";
    stone.properties["kind"] = "save";
    app_.openFacility(stone);
    tick();
    EXPECT_FALSE(quickSaveOnDisk()) << "存档点存不了";
    while (top() == "Dialogue") press(SDL_SCANCODE_RETURN);

    // 带南宫婉：开得了残禁、开不了幻阵（样板施工图第 7 节「带对的人才开得了」）。
    EXPECT_FALSE(tryGate("lilian_jindi_waiwei", Point{38, 13}, kUp, "lilian_jindi_waiwei.gate_huanzhen"))
        << "幻阵要陈师妹";
    std::string need = app_.data().lookupText("ui.world.gate.need");
    need.replace(need.find("{name}"), 6, "陈师妹");
    EXPECT_EQ(lastLine(), need) << "名册里已有会这手的人：提示「这里要某某来」（契约 3.6）";
    EXPECT_TRUE(tryGate("lilian_jindi_shiku", Point{27, 21}, 1, "lilian_jindi_shiku.gate_canjin")) << "残禁她开得了";
    // 残禁后面那个宝箱：开了（这一趟的账）。
    standAt("lilian_jindi_shiku", Point{32, 20}, 1);
    press(SDL_SCANCODE_RETURN);
    tick(30);
    while (top() == "Dialogue") press(SDL_SCANCODE_RETURN);
    EXPECT_EQ(app_.worldObjects().openedChests.count("lilian_jindi_shiku.chest_xiaoshi"), 1u);
    // 永久那份一样都没写（契约 3.12）。
    EXPECT_TRUE(state().worldObjects.openedGates.empty());
    EXPECT_EQ(state().worldObjects.openedChests, chestsBefore) << "永久的已开宝箱与进去之前一模一样";
    EXPECT_EQ(state().worldObjects.clearedFoes, foesBefore);
    EXPECT_EQ(state().worldObjects.gatheredDay, gatheredBefore);

    // 打一仗：历练的编成修为奖励为 0（样板施工图第 7 节「修为奖励为 0」）。找这一趟里在场的一只明雷，从侧后上去。
    // 石窟大厅那一只寒冰蟾每趟都在（没写 chance）：在 (24,20) 面朝上，背后是 (24,21)。
    standAt("lilian_jindi_shiku", Point{24, 21}, kUp);
    ASSERT_FALSE(HasFatalFailure());
    {
        const std::vector<Fight> fights = interactAndPlay(intentPolicy());
        ASSERT_EQ(fights.size(), 1u) << "从背后摸上去就开打";
        EXPECT_EQ(fights[0].id, "bl07_hanbing_chan_t1") << "历练里拼了档的后缀（韩立炼气，第 1 档）";
        EXPECT_TRUE(fights[0].preemptive);
        EXPECT_EQ(fights[0].phase, BattlePhase::Won) << fights[0].id;
    }
    ASSERT_TRUE(app_.inLilian()) << "打赢了还在历练里";
    EXPECT_EQ(state().cultivation, cultivation) << "历练打完修为不变";

    // 出来：队伍、图、位置恢复；日历加一天（契约 3.12）。
    leaveByExit("lilian_jindi_shiku", Point{24, 15}, 1);
    ASSERT_FALSE(HasFatalFailure());
    EXPECT_EQ(state().mapId, "ch06_baiyaoyuan");
    EXPECT_EQ(state().position, (Point{25, 4}));
    EXPECT_TRUE(state().party.empty()) << "队伍回到进去之前（空）";
    EXPECT_EQ(state().day, day + 1);
    EXPECT_EQ(state().cultivation, cultivation);
    EXPECT_TRUE(state().worldObjects.openedGates.empty()) << "机关开没开只记在那一趟里";

    // ---- 第二趟：只带陈师妹 ----反过来：开得了幻阵、开不了残禁；上一趟开过的机关关着、宝箱是满的。
    enterWith({"chen_shimei"});
    ASSERT_FALSE(HasFatalFailure());
    EXPECT_TRUE(app_.worldObjects().openedGates.empty()) << "第二趟进去机关是关着的";
    EXPECT_EQ(app_.worldObjects().openedChests.count("lilian_jindi_shiku.chest_xiaoshi"), 0u) << "第二趟宝箱是满的";
    EXPECT_TRUE(tryGate("lilian_jindi_waiwei", Point{38, 13}, kUp, "lilian_jindi_waiwei.gate_huanzhen"))
        << "幻阵陈师妹开得了";
    EXPECT_FALSE(tryGate("lilian_jindi_shiku", Point{27, 21}, 1, "lilian_jindi_shiku.gate_canjin")) << "残禁要南宫婉";
    leaveByExit("lilian_jindi_shiku", Point{24, 15}, 1);
    ASSERT_FALSE(HasFatalFailure());
    EXPECT_EQ(state().cultivation, cultivation);
}

// ===========================================================================
// 验收 7：她在队里时，图上没有第二个她（契约 3.11）。第 7、8 章各一条走真入口的；另核第 8–13 章的图上没有人因此少了。
// ===========================================================================
namespace {

int presentNpcs(Application& app, const std::string& mapId, std::set<std::string>* roles = nullptr) {
    if (!app.loadMap(mapId, std::string{}).ok) return -1;
    app.replaceScene(std::make_unique<WorldScene>());
    app.tick(1.0 / 60.0);
    auto* world = dynamic_cast<WorldScene*>(app.topScene());
    if (world == nullptr) return -1;
    const auto& frame = world->describeFrame(app);
    if (roles != nullptr) {
        for (const auto& npc : frame.npcs) roles->insert(npc.object->property("role_id"));
    }
    return static_cast<int>(frame.npcs.size());
}

}  // namespace

// 第 7 章：陈师妹在山头那一幕站在黄枫谷东口（visible ch07.chouhan、hidden ch07.murong）。她在队里时（主线里不会，
// 历练里会），那里就没有第二个她；不在队里，照旧站着。南宫婉在第 7 章任何一张图上都不摆 NPC（Ch07TriggerMode 那一条）。
TEST_F(Ch07CompanionsSlice, No7_InChapterSevenACompanionInThePartyIsNotAlsoStandingOnTheMap) {
    startFrom("companions/ch07-kundi-first.sav");
    ASSERT_FALSE(HasFatalFailure());
    // 她（白衣少女）在队里：禁地六张图上没有任何一个属于她的 NPC 在场。
    for (const char* mapId : {"ch07_jindi_waiwei", "ch07_jindi_zhongxin", "ch07_huanxingshan", "ch07_dixia_zhaoze",
                              "ch07_zhaoze_shenchu", "ch07_dixia_shiku", "ch07_jindi_wai"}) {
        std::set<std::string> roles;
        ASSERT_GE(presentNpcs(app_, mapId, &roles), 0) << mapId;
        for (const char* role : {"baiyi_shaonv", "nangong_wan_ch07", "nangong_wan"}) {
            EXPECT_EQ(roles.count(role), 0u) << mapId << "：她在队里，图上不该再站着一个 " << role;
        }
    }
    // 陈师妹：东口那一幕。
    GameState& s = state();
    s.party.clear();
    s.flags.erase("ch07.murong");
    s.setFlag("ch07.chouhan");
    std::set<std::string> without;
    ASSERT_GE(presentNpcs(app_, "ch06_huangfenggu", &without), 0);
    ASSERT_EQ(without.count("chen_shimei"), 1u) << "先验：山头那一幕陈师妹站在东口";
    s.partyAdd("chen_shimei");
    std::set<std::string> with;
    ASSERT_GE(presentNpcs(app_, "ch06_huangfenggu", &with), 0);
    EXPECT_EQ(with.count("chen_shimei"), 0u) << "契约 3.11：她在队里，东口那个她不在场";
    std::set<std::string> rest = without;
    rest.erase("chen_shimei");
    EXPECT_EQ(with, rest) << "别的人一个不少";
}

// 第 8 章起（以及第 9–13 章）：两位伙伴的任何一个角色都在队里，第 8–13 章每一张图上在场的 NPC 一个不少——
// 那些章节的戏里没有属于伙伴的 NPC，一人一身不会把谁变没。起点是第 7 章的交接档（第 8 章从它起步）。
TEST_F(Ch07CompanionsSlice, No7_FromChapterEightOnNoMapLosesAnyoneToTheOnePersonOneBodyRule) {
    // 每一章从它自己的起点看它自己的图：第 N 章的起点是第 N−1 章的交接档（第一侧）。
    // 每章看两个局面：开头（上一章的交接档）与收尾（本章的交接档）——人多半按剧情旗标来去，只看一头会漏掉另一头的人。
    const std::vector<std::pair<const char*, const char*>> chapters = {
        {"ch08_", "ch07-end-first.sav"}, {"ch08_", "ch08-end-first.sav"}, {"ch09_", "ch08-end-first.sav"},
        {"ch09_", "ch09-end-first.sav"}, {"ch10_", "ch09-end-first.sav"}, {"ch10_", "ch10-end-first.sav"},
        {"ch11_", "ch10-end-first.sav"}, {"ch11_", "ch11-end-first.sav"}, {"ch12_", "ch11-end-first.sav"},
        {"ch12_", "ch12-end-first.sav"}, {"ch13_", "ch12-end-first.sav"}, {"ch13_", "ch13-end-first.sav"}};
    int seen = 0;
    int mapsSeen = 0;
    for (const auto& [prefix, fixture] : chapters) {
        const auto loaded = fanren::io::loadGame(fanren::test::chapterFixturePath(assetRoot(), fixture).string());
        ASSERT_TRUE(loaded.ok) << fixture << "：" << loaded.error;
        std::vector<std::string> maps;
        for (const auto& entry : fs::directory_iterator(fs::path(assetRoot()) / "maps")) {
            const std::string name = entry.path().stem().string();
            if (entry.path().extension() == ".tmj" && name.rfind(prefix, 0) == 0) maps.push_back(name);
        }
        std::sort(maps.begin(), maps.end());
        for (const std::string& mapId : maps) {
            state() = loaded.value;
            state().party.clear();
            std::set<std::string> alone;
            const int before = presentNpcs(app_, mapId, &alone);
            ASSERT_GE(before, 0) << mapId;
            state() = loaded.value;
            for (const char* role : {"nangong_wan", "chen_shimei"}) state().partyAdd(role);
            std::set<std::string> together;
            EXPECT_EQ(presentNpcs(app_, mapId, &together), before) << mapId << "：带着伙伴，图上少了人";
            EXPECT_EQ(together, alone) << mapId;
            seen += before;
            ++mapsSeen;
        }
    }
    EXPECT_GE(mapsSeen, 60) << "先验：第 8–13 章的图都找到了（每张两个局面）";
    EXPECT_GT(seen, 10) << "先验：图上确实站着人（分母不塌）";
}

// ===========================================================================
// 验收 5：最小路线生成的 ch07-end 与改版前那一份逐字段比，差异只落在样板施工图第 1 节列的几类字段里。
// 改版前那一份抄在 tests/fixtures/companions/pre-v10/ch07-end-*.sav（与改版前的交接档逐字节相同）。
// ===========================================================================
namespace {

using nlohmann::json;

json payloadOf(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    std::ostringstream text;
    text << in.rdbuf();
    const json root = json::parse(text.str(), nullptr, false);
    if (root.is_discarded() || !root.contains("payload")) return json();
    return root["payload"];
}

// 第 1 节：「允许不同的只有：存档 v10 新增的字段；第 9 节列出的新旗标；已知破绽表（条目只许多不许少）；目标进度。
// 修为、灵石、物品、法术、气血上限必须一样，药品数也必须一样。」游戏时长两项（playSeconds*）不是局面。
const std::set<std::string> kV10Keys = {"bonds", "openedChests", "gatheredDay", "clearedFoes", "openedGates"};
const std::set<std::string> kNewFlags = {"ch07.xinglai",    "ch07.kundi",       "ch07.huolingshi",   "ch07.poding",
                                         "ch07.chahua_jun", "ch07.chahua_kumu", "ch07.chahua_shishi", "ch07.chahua_yanbi",
                                         "ch07.lilian_chen", "ch07.lilian_nangong"};

}  // namespace

TEST(Ch07CompanionsHandOver, No5_TheMinimalRouteEndingDiffersFromTheOldOneOnlyWhereSectionOneAllows) {
    // 第 8–13 章的交接档是从新的 ch07-end 一章一章重生成的：同一条口径一直锁到第 13 章。
    for (const std::string chapter : {"07", "08", "09", "10", "11", "12", "13"})
    for (const std::string side : {"first", "second"}) {
        const fs::path dir = fs::path(assetRoot()) / "tests" / "fixtures";
        const std::string file = "ch" + chapter + "-end-" + side + ".sav";
        const json now = payloadOf(dir / file);
        const json old = payloadOf(dir / "companions" / "pre-v10" / file);
        ASSERT_TRUE(now.is_object() && old.is_object()) << file << "：两份交接档都要读得出来";
        std::set<std::string> keys;
        for (const auto& [key, value] : now.items()) keys.insert(key);
        for (const auto& [key, value] : old.items()) keys.insert(key);
        int compared = 0;
        for (const std::string& key : keys) {
            if (kV10Keys.count(key) || key.rfind("playSeconds", 0) == 0) continue;
            if (key == "formerRealm" && !old.contains(key)) {
                // 存档 v9 的字段（第 9 章才用）：改版前那份是 v8 写的，没有它；新写出的 v10 档带着缺省值 0。
                EXPECT_EQ(now[key], 0) << file << "：formerRealm 是 v9 的字段，第 7 章终局里该是缺省";
                continue;
            }
            ASSERT_TRUE(now.contains(key) && old.contains(key)) << file << "：字段 " << key << " 只在一边有";
            ++compared;
            if (key == "bag") {
                // 物品逐件一样（堆的先后次序不算：火灵石采来又交出去、金光砖挪到破顶才扣，堆的位置会挪）。
                std::multiset<std::string> a;
                std::multiset<std::string> b;
                for (const auto& stack : now[key]) a.insert(stack.dump());
                for (const auto& stack : old[key]) b.insert(stack.dump());
                EXPECT_EQ(a, b) << file << "：物品（含药品数）必须一样";
            } else if (key == "flags") {
                json trimmed = now[key];
                for (const std::string& f : kNewFlags) trimmed.erase(f);
                EXPECT_EQ(trimmed, old[key]) << file << "：旗标只许多出第 9 节的新旗标";
            } else if (key == "knownWeaknesses") {
                for (const auto& [oldRole, cats] : old[key].items()) {
                    // 唯一的例外：⑤ 的敌人从蜕皮前的 mo_jiao 换成了蜕皮后的 mo_jiao_tuipi（协调者 2026-11-18 转来的定稿：
                    // 文案写的就是白蛟），揭开的破绽记在新 id 上；mo_jiao 此后不再出场。
                    const std::string role = oldRole == "mo_jiao" ? "mo_jiao_tuipi" : oldRole;
                    ASSERT_TRUE(now[key].contains(role)) << file << "：破绽表少了 " << role;
                    for (const auto& c : cats) {
                        EXPECT_NE(std::find(now[key][role].begin(), now[key][role].end(), c), now[key][role].end())
                            << file << "：" << role << " 的破绽少了一样";
                    }
                }
            } else {
                EXPECT_EQ(now[key], old[key]) << file << "：字段 " << key << " 必须一样（第 1 节）";
            }
        }
        EXPECT_GE(compared, 20) << "先验：真的比了一遍";
        // 新的那份里，默契账恰是最小路线的 37（4.6），世界对象一个也没动（不捡、不打可选战）。
        EXPECT_EQ(now["bonds"]["nangong_wan"]["value"], 37) << file;
        for (const char* key : {"openedChests", "clearedFoes", "openedGates"}) {
            EXPECT_TRUE(now[key].empty()) << file << "：最小路线不碰 " << key;
        }
        // 唯一动过的是火灵石矿脉：4.3 必经的那一块（采到、交给她）。
        ASSERT_EQ(now["gatheredDay"].size(), 1u) << file;
        EXPECT_TRUE(now["gatheredDay"].contains("ch07_zhaoze_shenchu.gather_huolingshi_bei")) << file;
    }
}
