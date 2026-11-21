// 同道录 2a·路 K：走 Application 真入口的那一半（docs/ch07-tongdaolu-k.md 第 4、5 节）。
//   * 建场：按档解锁的法术、她的被动、在册加成、同修的劲都只在第 7 章、只在够档时进战斗；
//   * 切磋：Application::startSpar 开她那一场——只有主角一人，赢 +4 输 +2，首胜奖品，打完回满，不掉东西；
//   * 同修：Application::tongxiu 过一天、第二天第一场开场多 1 点劲（开了就用掉）；
//   * 脚本：tongdao.* 的查询与动作、show_cg / hide_cg。
#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "TempDir.h"
#include "core/battle/Battle.h"
#include "core/model/AttackCategory.h"
#include "core/model/Types.h"
#include "core/rules/Tongdao.h"
#include "game/Application.h"
#include "game/BattleScene.h"
#include "BattleHand.h"

namespace {

namespace fs = std::filesystem;
using fanren::core::battle::Unit;
using fanren::game::Application;
using fanren::game::BattleScene;
using fanren::game::BattleStart;
namespace rules = fanren::rules;

fs::path repoRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "data" / "text" / "ch01_main.json")) return candidate;
    }
    return ".";
}

void writeFile(const fs::path& path, const std::string& text) {
    fs::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out << text;
}

std::string role(const std::string& id, int hp, int attack, int speed, const std::string& extra = "") {
    return R"({"id": ")" + id + R"(", "name": ")" + id + R"(", "realm": "QiRefining10", "maxHp": )" +
           std::to_string(hp) + R"(, "maxMp": 50, "attack": )" + std::to_string(attack) +
           R"(, "defence": 1, "speed": )" + std::to_string(speed) + R"(, "element": 0, "magics": ["magic_probe_one"])" +
           extra + "}";
}

std::string sparBattle(const std::string& id, const std::string& foeRole) {
    return R"({"id": ")" + id + R"(", "name": "切磋", "chapter": 7, "terrain": "field", "can_escape": false, )"
           R"("defeat_is_fatal": false, "spar": true, "units": [{"role_id": ")" + foeRole +
           R"(", "faction": "enemy"}], "rewards": {"cultivation": 0, "spirit_stones": 0, "drops": []}})";
}

class TongdaoWiring : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        root_ = std::make_unique<fanren::test::TempDir>("fanren_tongdao_wiring");
        for (const char* dir : {"data", "scripts", "maps"}) {
            fs::copy(repoRoot() / dir, root_->path() / dir, fs::copy_options::recursive);
        }
        const fs::path data = root_->path() / "data";
        for (const char* m : {"one", "two", "three"}) {
            writeFile(data / "magics" / (std::string("probe_") + m + ".json"),
                      std::string(R"({"id": "magic_probe_)") + m + R"(", "name": "探针)" + m +
                          R"(", "element": 1, "needMp": 0, "power": 5, "needRealm": "Mortal"})");
        }
        writeFile(data / "roles" / "probe_qing_role.json", role("probe_qing_role", 300, 10, 4));
        writeFile(data / "magics" / "probe_heal.json",
                  R"({"id": "magic_probe_heal", "name": "探针回春", "needMp": 0, "power": 0, "effect": "heal", "amount": 50})");
        writeFile(data / "magics" / "probe_wall.json",
                  R"({"id": "magic_probe_wall", "name": "探针剑幕", "needMp": 0, "power": 0, "effect": "bulwark", "amount": 30})");
        writeFile(data / "roles" / "probe_mend_role.json",
                  R"({"id": "probe_mend_role", "name": "探针医", "realm": "QiRefining10", "maxHp": 300, "maxMp": 50, )"
                  R"("attack": 10, "defence": 1, "speed": 60, "element": 0, )"
                  R"("magics": ["magic_probe_heal", "magic_probe_wall"]})");
        writeFile(data / "roles" / "probe_win_role.json", role("probe_win_role", 1, 1, 1, R"(, "toughness": 1, "weaknesses": ["拳"])"));
        writeFile(data / "roles" / "probe_lose_role.json", role("probe_lose_role", 99999, 99999, 99, R"(, "toughness": 9, "weaknesses": ["毒"])"));
        writeFile(data / "roles" / "probe_foe.json", role("probe_foe", 60, 1, 1, R"(, "toughness": 5, "weaknesses": ["火"])"));

        const fs::path companions = data / "companions";
        writeFile(companions / "probe_qing.json", R"({
          "id": "probe_qing", "met_flag": "probe.qing", "chase": true,
          "roles": [{"role": "probe_qing_role",
                     "magics": [{"tier": 3, "magic": "magic_probe_two"}, {"tier": 4, "magic": "magic_probe_three"}],
                     "passive": {"kind": "opening_bp", "values": [1, 2]}}],
          "roster_bonus": {"kind": "element_stagger", "value": 1, "element": "fire"}
        })");
        writeFile(companions / "probe_mend.json",
                  R"({"id": "probe_mend", "met_flag": "probe.mend", "roles": [{"role": "probe_mend_role"}]})");
        writeFile(companions / "probe_win.json", R"({
          "id": "probe_win", "met_flag": "probe.win", "roles": [{"role": "probe_win_role"}],
          "spar": {"battle": "probe_spar_win",
                   "prizes": [{"tier": 2, "items": [{"item": "pill_huiqi_dan", "count": 2}]}]}
        })");
        writeFile(companions / "probe_lose.json", R"({
          "id": "probe_lose", "met_flag": "probe.lose", "roles": [{"role": "probe_lose_role"}],
          "spar": {"battle": "probe_spar_lose"}
        })");
        writeFile(data / "battles" / "probe_spar_win.json", sparBattle("probe_spar_win", "probe_win_role"));
        writeFile(data / "battles" / "probe_spar_lose.json", sparBattle("probe_spar_lose", "probe_lose_role"));
        writeFile(data / "battles" / "probe_plain.json",
                  R"({"id": "probe_plain", "name": "探针", "chapter": 7, "terrain": "field", "can_escape": true, )"
                  R"("defeat_is_fatal": false, "units": [{"role_id": "probe_foe", "faction": "enemy"}], )"
                  R"("rewards": {"cultivation": 0, "spirit_stones": 0, "drops": []}})");
    }
    static void TearDownTestSuite() { root_.reset(); }

    void SetUp() override {
        auto ready = app_.init(root_->path().string(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
        state().hp = state().maxHp = 400;
        state().mp = state().maxMp = 60;
        state().day = 30;
        state().setFlag("ch06.done");   // 第 7 章里
        for (const char* flag : {"probe.qing", "probe.win", "probe.lose", "probe.mend"}) state().setFlag(flag);
    }
    void TearDown() override { app_.shutdown(); }

    fanren::core::GameState& state() { return app_.state(); }
    void tick(int frames = 1) {
        for (int i = 0; i < frames; ++i) app_.tick(1.0 / 60.0);
    }
    BattleScene* open(const std::string& id) {
        app_.startBattle(BattleStart{id}, [](const fanren::script::CommandResult&) {});
        tick();
        return dynamic_cast<BattleScene*>(app_.topScene());
    }
    static const Unit* unitOf(const BattleScene& scene, const std::string& id) {
        for (const Unit& u : scene.battle().units()) {
            if (u.id == id) return &u;
        }
        return nullptr;
    }
    static bool knows(const Unit& u, const std::string& magic) {
        return std::find(u.magics.begin(), u.magics.end(), magic) != u.magics.end();
    }
    void finishBattles() {
        tick();
        for (int i = 0; i < 16 && dynamic_cast<BattleScene*>(app_.topScene()) != nullptr; ++i) tick();
    }
    void run(const std::string& source) {
        writeFile(root_->path() / "scripts" / "t" / "probe.lua", source);
        const auto started = app_.startEvent("t/probe.lua");
        ASSERT_TRUE(started.ok) << started.error;
        for (int frame = 0; frame < 128 && app_.scripts().isRunning(); ++frame) tick();
        ASSERT_FALSE(app_.scripts().isRunning());
        ASSERT_TRUE(app_.scripts().lastError().empty()) << app_.scripts().lastError();
    }

    static std::unique_ptr<fanren::test::TempDir> root_;
    Application app_;
};

std::unique_ptr<fanren::test::TempDir> TongdaoWiring::root_;

// ---------------------------------------------------------------------------
// 建场
// ---------------------------------------------------------------------------

TEST_F(TongdaoWiring, TierSpellsAndThePassiveEnterTheBattleOnlyAtTheirTierAndOnlyInChapterSeven) {
    state().partyAdd("probe_qing_role");
    struct Case {
        int bond;
        bool chapter7Done;
        bool two, three;
        int bp;
    };
    for (const Case& c : {Case{20, false, false, false, 1}, Case{45, false, true, false, 2},
                          Case{70, false, true, true, 2}, Case{95, false, true, true, 3},
                          Case{95, true, false, false, 1}}) {
        state().bonds["probe_qing"].value = c.bond;
        state().setFlag("ch07.done", c.chapter7Done ? 1 : 0);
        BattleScene* scene = open("probe_plain");
        ASSERT_NE(scene, nullptr);
        const Unit* qing = unitOf(*scene, "probe_qing_role");
        ASSERT_NE(qing, nullptr);
        EXPECT_TRUE(knows(*qing, "magic_probe_one")) << "角色表里那一门一直在";
        EXPECT_EQ(knows(*qing, "magic_probe_two"), c.two) << "默契 " << c.bond;
        EXPECT_EQ(knows(*qing, "magic_probe_three"), c.three) << "默契 " << c.bond;
        EXPECT_EQ(scene->battle().hasMagic("magic_probe_two"), c.two) << "登记表与菜单同一份";
        EXPECT_EQ(scene->battle().units()[0].bp, c.bp) << "被动 opening_bp：三档 +1、五档 +2；默契 " << c.bond;
        finishBattles();
    }
}

TEST_F(TongdaoWiring, TheRosterBonusReachesTheHeroAfterQingjiaoEvenIfSheStaysHome) {
    ASSERT_TRUE(state().party.empty());
    BattleScene* scene = open("probe_plain");
    ASSERT_NE(scene, nullptr);
    EXPECT_TRUE(scene->battle().units()[0].extraStagger.empty()) << "还没请教";
    finishBattles();

    state().bonds["probe_qing"].value = 45;
    ASSERT_EQ(rules::recordQingjiao(app_.data().companions, "probe_qing", state()), rules::TongdaoCheck::Ok);
    scene = open("probe_plain");
    ASSERT_NE(scene, nullptr);
    ASSERT_EQ(scene->battle().units()[0].extraStagger.size(), 1u) << "她不在队里，在册加成照样生效";
    EXPECT_EQ(scene->battle().units()[0].extraStagger[0].categories, fanren::core::kCategoryFire);
    finishBattles();

    state().setFlag("ch07.done");
    scene = open("probe_plain");
    ASSERT_NE(scene, nullptr);
    EXPECT_TRUE(scene->battle().units()[0].extraStagger.empty()) << "第 8 章起不套";
    finishBattles();
}

TEST_F(TongdaoWiring, TheMenuOffersAlliesForHealAndNoTargetForBulwarkAndTheHandPlaysThem) {
    state().partyAdd("probe_mend_role");
    state().hp = 100;   // 四成之下
    BattleScene* scene = open("probe_plain");
    ASSERT_NE(scene, nullptr);
    const auto& units = scene->battle().units();
    int mend = -1;
    for (std::size_t i = 0; i < units.size(); ++i) {
        if (units[i].id == "probe_mend_role") mend = static_cast<int>(i);
    }
    ASSERT_GE(mend, 0);
    fanren::core::battle::Action heal;
    heal.kind = fanren::core::battle::ActionKind::Cast;
    heal.actorIndex = mend;
    heal.magicId = "magic_probe_heal";
    std::vector<int> targets;
    static_cast<void>(BattleScene::buildTargetItems(state(), scene->battle(), heal, targets));
    ASSERT_FALSE(targets.empty());
    for (const int t : targets) EXPECT_TRUE(units[static_cast<std::size_t>(t)].ally) << "回血只列我方";

    // 她身法 60，第一个动；那只手（真实菜单）该先给韩立回血。
    ASSERT_EQ(scene->runToAllyTurn(), mend);
    fanren::test::BattleHand hand(app_);
    const fanren::core::battle::Action chosen = hand.decide(scene->battle(), mend);
    EXPECT_EQ(chosen.magicId, "magic_probe_heal");
    EXPECT_EQ(chosen.targetIndex, 0);
    ASSERT_TRUE(hand.issue(*scene, chosen)) << scene->feedback();
    EXPECT_EQ(scene->battle().units()[0].hp, 150);

    // 下一回合轮到她：剑幕从菜单里选中就出，不进择敌。
    for (int guard = 0; guard < 8 && scene->runToAllyTurn() != mend; ++guard) {
        fanren::core::battle::Action defend;
        defend.kind = fanren::core::battle::ActionKind::Defend;
        defend.actorIndex = scene->runToAllyTurn();
        ASSERT_TRUE(hand.issue(*scene, defend)) << scene->feedback();
    }
    ASSERT_EQ(scene->runToAllyTurn(), mend);
    fanren::core::battle::Action wall;
    wall.kind = fanren::core::battle::ActionKind::Cast;
    wall.actorIndex = mend;
    wall.magicId = "magic_probe_wall";
    ASSERT_TRUE(hand.issue(*scene, wall)) << scene->feedback();
    EXPECT_EQ(scene->battle().units()[0].status.bulwarkPercent, 30);
    finishBattles();
}

// ---------------------------------------------------------------------------
// 同修
// ---------------------------------------------------------------------------

TEST_F(TongdaoWiring, TongxiuPassesADayAndTheNextFirstBattleOpensWithOneMoreBp) {
    const rules::BondGain gain = app_.tongxiu("probe_qing");
    EXPECT_EQ(gain.check, rules::TongdaoCheck::Ok);
    EXPECT_EQ(gain.gained, 4);
    EXPECT_EQ(state().day, 31) << "过一天走的是平常那条路";
    EXPECT_EQ(state().flag(rules::kTongxiuBonusDayFlag), 31);

    BattleScene* scene = open("probe_plain");
    ASSERT_NE(scene, nullptr);
    EXPECT_EQ(scene->battle().units()[0].bp, 2);
    EXPECT_EQ(state().flag(rules::kTongxiuBonusDayFlag), 0) << "开了这一场就用掉";
    finishBattles();
    scene = open("probe_plain");
    ASSERT_NE(scene, nullptr);
    EXPECT_EQ(scene->battle().units()[0].bp, 1) << "第二场没有了";
    finishBattles();

    EXPECT_EQ(app_.tongxiu("probe_win").check, rules::TongdaoCheck::Ok) << "第二天又能同修";
    EXPECT_EQ(app_.tongxiu("nobody").check, rules::TongdaoCheck::NoCompanion);
    EXPECT_EQ(state().day, 32) << "不成的那一次不过日子";
}

// ---------------------------------------------------------------------------
// 切磋
// ---------------------------------------------------------------------------

TEST_F(TongdaoWiring, ASparIsHeroAloneAgainstHerAndAWinPaysTheFirstWinPrize) {
    state().partyAdd("probe_qing_role");
    state().bonds["probe_win"].value = 20;
    state().hp = 200;
    state().mp = 10;
    bool done = false;
    rules::SparOutcome got;
    const auto check = app_.startSpar("probe_win", [&](const rules::SparOutcome& outcome,
                                                       const fanren::script::CommandResult& result) {
        done = true;
        got = outcome;
        EXPECT_TRUE(result.battleWon);
    });
    ASSERT_EQ(check, rules::TongdaoCheck::Ok);
    tick();
    auto* scene = dynamic_cast<BattleScene*>(app_.topScene());
    ASSERT_NE(scene, nullptr);
    ASSERT_EQ(scene->battle().units().size(), 2u) << "只有他和她：队伍不带";
    EXPECT_TRUE(scene->battle().spar());
    EXPECT_FALSE(scene->battle().units()[1].ally) << "她在敌方那一边";
    finishBattles();
    ASSERT_TRUE(done);
    EXPECT_EQ(got.gained, 4);
    EXPECT_EQ(got.prizeTier, 2);
    EXPECT_EQ(state().itemCount("pill_huiqi_dan"), 2);
    EXPECT_EQ(state().bondOf("probe_win"), 24);
    EXPECT_EQ(state().flag(rules::sparDayFlag("probe_win")), 30);
    EXPECT_EQ(state().hp, 400) << "打完回满";
    EXPECT_EQ(state().mp, 60);
    EXPECT_EQ(state().bondOf("probe_qing"), 0) << "队里的人没上场，不记并肩";
    EXPECT_EQ(app_.startSpar("probe_win", nullptr), rules::TongdaoCheck::DoneToday);
}

TEST_F(TongdaoWiring, LosingASparIsNotADefeatTheHeroIsRestoredAndGetsTwo) {
    state().bonds["probe_lose"].value = 20;
    const auto bag = state().bag;
    bool done = false;
    ASSERT_EQ(app_.startSpar("probe_lose",
                             [&](const rules::SparOutcome& outcome, const fanren::script::CommandResult& result) {
                                 done = true;
                                 EXPECT_FALSE(result.battleWon);
                                 EXPECT_EQ(result.code, "lost");
                                 EXPECT_EQ(outcome.gained, 2);
                                 EXPECT_EQ(outcome.prizeTier, 0);
                             }),
              rules::TongdaoCheck::Ok);
    tick();   // 压栈要等一帧
    finishBattles();
    ASSERT_TRUE(done);
    EXPECT_EQ(state().hp, 400) << "气血见底就停，打完回满";
    EXPECT_EQ(state().bondOf("probe_lose"), 22);
    EXPECT_EQ(state().bag.size(), bag.size()) << "不掉东西";
    EXPECT_EQ(dynamic_cast<BattleScene*>(app_.topScene()), nullptr);
}

TEST_F(TongdaoWiring, ASparBelowTierTwoDoesNotStart) {
    EXPECT_EQ(app_.startSpar("probe_win", nullptr), rules::TongdaoCheck::TierLow);
    EXPECT_EQ(dynamic_cast<BattleScene*>(app_.topScene()), nullptr);
    EXPECT_EQ(app_.startSpar("probe_qing", nullptr), rules::TongdaoCheck::NoSpar);
}

// ---------------------------------------------------------------------------
// 脚本
// ---------------------------------------------------------------------------

TEST_F(TongdaoWiring, TheScriptApiQueriesAndActsThroughTheRules) {
    state().bonds["probe_qing"].value = 45;
    run(R"(
        assert(tongdao.effects_active())
        assert(tongdao.in_roster("probe_qing") and not tongdao.in_roster("nobody"))
        assert(tongdao.kind("probe_qing") == "person" and tongdao.kind("nobody") == "")
        assert(tongdao.line("probe_qing") == "general")
        assert(tongdao.line("chen_qiaoqian") == "hongyan")
        assert(tongdao.tier("probe_qing") == 3)
        assert(tongdao.tier_name("chen_qiaoqian", 5) == "ui.bond.hongyan.tier5")
        assert(tongdao.tier_name("probe_qing") == "ui.bond.tier3")
        assert(tongdao.cap_tier("nangong_wan") == 4 and tongdao.cap_tier("probe_qing") == 5)
        local roster = tongdao.roster()
        local seen = false
        for _, id in ipairs(roster) do if id == "probe_qing" then seen = true end end
        assert(seen)

        local ok, code = tongdao.can("spar", "probe_qing")
        assert(not ok and code == "no_spar", code)
        ok, code = tongdao.can("qingjiao", "probe_qing")
        assert(ok and code == "", code)
        assert(not tongdao.bonus_unlocked("probe_qing"))
        ok, code = tongdao.qingjiao("probe_qing")
        assert(ok, code)
        assert(tongdao.qingjiao_done("probe_qing"))
        assert(tongdao.bonus_unlocked("probe_qing") and tongdao.bonus_active("probe_qing"))
        ok, code = tongdao.qingjiao("probe_qing")
        assert(not ok and code == "already", code)
        assert(tongdao.bonus_slots() == 4)
        ok = tongdao.bonus_select("probe_qing", true)
        assert(ok and tongdao.bonus_selected("probe_qing"))
        tongdao.bonus_select("probe_qing", false)
        assert(not tongdao.bonus_selected("probe_qing"))

        assert(tongdao.xuhua_pending("probe_qing") == 2)
        ok, code = tongdao.xuhua_mark("probe_qing", 2)
        assert(ok, code)
        assert(tongdao.xuhua_pending("probe_qing") == 3 and tongdao.xuhua_done("probe_qing") == 2)
        ok, code = tongdao.xuhua_mark("probe_qing", 5)
        assert(not ok and code == "tier_low", code)

        local gained
        ok, code, gained = tongdao.tongxiu("probe_qing")
        assert(ok and gained == 4, code)
        ok, code = tongdao.tongxiu("nobody")
        assert(not ok and code == "no_companion", code)

        show_cg("probe_cg")
        flag.set("t.cg_shown", 1)
    )");
    EXPECT_EQ(state().flag("t.cg_shown"), 1);
    EXPECT_EQ(app_.currentCg(), "probe_cg") << "命令走通：记下 id，画面归路 U";
    EXPECT_EQ(state().day, 31);
    EXPECT_EQ(state().bondOf("probe_qing"), 49);
    run("hide_cg()");
    EXPECT_TRUE(app_.currentCg().empty());
}

TEST_F(TongdaoWiring, ASparFromAScriptWaitsForTheBattleAndReturnsTheResult) {
    state().bonds["probe_win"].value = 20;
    run(R"(
        local won, code, gained = tongdao.spar("probe_win")
        assert(won and code == "" and gained == 4, tostring(code))
        won, code = tongdao.spar("probe_win")
        assert(not won and code == "done_today", code)
        flag.set("t.spar_done", 1)
    )");
    EXPECT_EQ(state().flag("t.spar_done"), 1);
    EXPECT_EQ(state().bondOf("probe_win"), 24);
}

}  // namespace
