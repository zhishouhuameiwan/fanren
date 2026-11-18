// 伙伴改版·B 路「战斗」（docs/interfaces-companions.md 3.1–3.4、3.10、2.2，第 5 节「四人同场」「追击」「合招」）：
// 每一条都走真实入口——Application::startBattle 开仗、BattleScene 建场、tests/BattleHand.h 那只手按真实菜单出手。
//
// 资产根是一份临时拷贝（data/、scripts/、maps/，与 CompanionsBaseWiringTests 同一个做法），往里放几位探针伙伴、
// 几个探针角色、一门探针合招和几份探针编成，不碰仓库。期望值照契约写死：
//   * 伙伴前 3 位上场，不是伙伴的不设上限（3.1）；
//   * 追击：默契 ≥ 20（二档）、chase 为真；补一记普攻，吃破势 ×2；群攻只追第一个破势的；每回合至多一次（3.2）；
//   * 合招：默契 ≥ 70（四档）；两人各扣 2 点劲、两人本回合都算行动过、下一回合都不加劲；威力 ×1.6（3.3）；
//   * 先手：我方全员开场 2 点劲（1 + 1）、第一回合我方全员排在敌人之前（3.4）；
//   * 人数系数：1 人 ×1.0、2 人 ×1.35、3 人 ×1.7、4 人起 ×2.0，只乘气血（3.10）；
//   * 打赢一仗每位上场伙伴 +2，同一天至多 +6（2.2）。
#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "BattleHand.h"
#include "TempDir.h"
#include "core/battle/Battle.h"
#include "core/battle/Damage.h"
#include "core/model/Types.h"
#include "game/Application.h"
#include "game/BattleFx.h"
#include "game/BattleScene.h"
#include "io/BattleLoader.h"

namespace {

namespace fs = std::filesystem;
using fanren::core::battle::Action;
using fanren::core::battle::ActionKind;
using fanren::core::battle::BattleEvent;
using fanren::core::battle::BattleEventKind;
using fanren::core::battle::BattlePhase;
using fanren::core::battle::BattleState;
using fanren::core::battle::Unit;
using fanren::game::Application;
using fanren::game::BattleMenuMode;
using fanren::game::BattleScene;
using fanren::game::BattleStart;

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

// ---- 探针数据 ----

std::string role(const std::string& id, const std::string& name, int hp, int attack, int defence, int speed,
                 const std::string& extra) {
    return R"({"id": ")" + id + R"(", "name": ")" + name + R"(", "realm": "QiRefining1", "maxHp": )" +
           std::to_string(hp) + R"(, "maxMp": 0, "attack": )" + std::to_string(attack) + R"(, "defence": )" +
           std::to_string(defence) + R"(, "speed": )" + std::to_string(speed) + R"(, "element": 0, "magics": [])" +
           extra + "}";
}

std::string companion(const std::string& id, const std::string& roleId, bool chase, const std::string& duo) {
    std::string text = R"({"id": ")" + id + R"(", "met_flag": "probe.)" + id + R"(", "roles": [{"role": ")" + roleId +
                       R"("}], "chase": )" + (chase ? "true" : "false");
    if (!duo.empty()) text += R"(, "duo": ")" + duo + R"(")";
    return text + "}";
}

std::string battle(const std::string& id, const std::vector<std::string>& foes, const std::string& extra = "") {
    std::string units;
    for (const std::string& foe : foes) {
        if (!units.empty()) units += ", ";
        units += R"({"role_id": ")" + foe + R"(", "faction": "enemy"})";
    }
    return R"({"id": ")" + id + R"(", "name": "探针", "chapter": 7, "terrain": "field", "can_escape": true, )" +
           R"("defeat_is_fatal": false, "units": [)" + units + R"(], "rewards": {"cultivation": 0, )" +
           R"("spirit_stones": 0, "drops": []})" + extra + "}";
}

constexpr int kDuoPower = 20;            // 探针合招的威力；合招按 ×1.6 = 32 打
constexpr int kDuoPowerTimes16 = 32;
constexpr int kDummyHp = 9999;
constexpr int kScaledFoeHp = 100;
constexpr int kScaledFoeToughness = 3;

class CompanionsBattle : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        root_ = std::make_unique<fanren::test::TempDir>("fanren_companions_battle");
        for (const char* dir : {"data", "scripts", "maps"}) {
            fs::copy(repoRoot() / dir, root_->path() / dir, fs::copy_options::recursive);
        }
        const fs::path data = root_->path() / "data";
        const std::string sword = R"(, "weapons": ["剑"])";
        // 伙伴的角色：身法 4，比韩立（5）慢、比木桩（1）快；probe_ally_e 身法 30，第一个动。
        for (const char* id : {"probe_ally_a", "probe_ally_b", "probe_ally_c", "probe_ally_d", "probe_ally_n",
                               "probe_ally_v", "probe_ally_w", "probe_ally_x", "probe_ally_r"}) {
            writeFile(data / "roles" / (std::string(id) + ".json"), role(id, id, 300, 20, 5, 4, sword));
        }
        writeFile(data / "roles" / "probe_ally_e.json", role("probe_ally_e", "probe_ally_e", 300, 20, 5, 30, sword));
        const std::string weak = R"(, "toughness": 1, "weaknesses": ["拳", "剑", "水"])";
        writeFile(data / "roles" / "probe_dummy.json", role("probe_dummy", "木桩", kDummyHp, 1, 0, 1, weak));
        writeFile(data / "roles" / "probe_weak.json", role("probe_weak", "纸人", 1, 1, 0, 1, weak));
        writeFile(data / "roles" / "probe_fast_foe.json",
                  role("probe_fast_foe", "疾风", kDummyHp, 1, 50, 40, R"(, "toughness": 3, "weaknesses": ["火"])"));
        writeFile(data / "roles" / "probe_boss.json",
                  role("probe_boss", "双手", kDummyHp, 1, 0, 3, R"(, "toughness": 1, "weaknesses": ["拳"], "actions": 2)"));
        writeFile(data / "roles" / "probe_scaled_foe.json",
                  role("probe_scaled_foe", "称", kScaledFoeHp, 1, 50, 1,
                       R"(, "toughness": 3, "weaknesses": ["火"])"));
        writeFile(data / "roles" / "probe_killer.json", role("probe_killer", "凶", kDummyHp, 9999, 50, 40, ""));
        // 一击就倒、出手最快、专挑打得死的：用来让一位伙伴先倒下、仗却打赢。
        writeFile(data / "roles" / "probe_striker.json", role("probe_striker", "刺客", 1, 60, 0, 40, ""));
        // 防御高到法术只打得出保底的 1 点：连发两下，第一下破势、第二下（破势 ×2）把这 2 点气血打光。
        writeFile(data / "roles" / "probe_frail.json",
                  role("probe_frail", "朽木", 2, 1, 9999, 1, R"(, "toughness": 1, "weaknesses": ["水"])"));

        writeFile(data / "magics" / "probe_duo.json",
                  R"({"id": "magic_probe_duo", "name": "探针合击", "descKey": "magic.desc.magic_probe_duo", )"
                  R"("element": 4, "needMp": 0, "power": )" +
                      std::to_string(kDuoPower) + R"(, "needRealm": "Mortal", "boost": "power"})");

        // 连发型、敌方全体、水：蓄 1 点劲打两下。
        writeFile(data / "magics" / "probe_rain.json",
                  R"({"id": "magic_probe_rain", "name": "探针骤雨", "descKey": "magic.desc.magic_probe_rain", )"
                  R"("element": 4, "needMp": 0, "power": 1, "needRealm": "Mortal", "boost": "hits", "target": "all"})");

        const fs::path companions = data / "companions";
        writeFile(companions / "probe_a.json", companion("probe_a", "probe_ally_a", true, ""));
        writeFile(companions / "probe_b.json", companion("probe_b", "probe_ally_b", true, ""));
        writeFile(companions / "probe_c.json", companion("probe_c", "probe_ally_c", true, ""));
        writeFile(companions / "probe_d.json", companion("probe_d", "probe_ally_d", true, ""));
        writeFile(companions / "probe_n.json", companion("probe_n", "probe_ally_n", false, "magic_probe_duo"));
        writeFile(companions / "probe_e.json", companion("probe_e", "probe_ally_e", false, "magic_probe_duo"));
        writeFile(companions / "probe_x.json", companion("probe_x", "probe_ally_x", true, "magic_probe_duo"));
        writeFile(companions / "probe_r.json", companion("probe_r", "probe_ally_r", false, "magic_tianyan_shu"));
        // 同一个人两段角色：编成里写死前一段、队伍里带着后一段——场上会有两个她（契约第 4 节）。
        writeFile(companions / "probe_v.json",
                  R"({"id": "probe_v", "met_flag": "probe.v", "chase": true, )"
                  R"("roles": [{"role": "probe_ally_v", "until_flag": "probe.v_grown"}, {"role": "probe_ally_w"}]})");

        const fs::path battles = data / "battles";
        writeFile(battles / "probe_b_dummy.json", battle("probe_b_dummy", {"probe_dummy"}));
        writeFile(battles / "probe_b_two.json", battle("probe_b_two", {"probe_dummy", "probe_dummy"}));
        writeFile(battles / "probe_b_weak.json", battle("probe_b_weak", {"probe_weak"}));
        writeFile(battles / "probe_b_fast.json", battle("probe_b_fast", {"probe_fast_foe"}));
        writeFile(battles / "probe_b_boss.json", battle("probe_b_boss", {"probe_boss"}));
        writeFile(battles / "probe_b_killer.json", battle("probe_b_killer", {"probe_killer"}));
        writeFile(battles / "probe_b_striker.json", battle("probe_b_striker", {"probe_striker"}));
        writeFile(battles / "probe_b_frail.json", battle("probe_b_frail", {"probe_frail", "probe_dummy"}));
        writeFile(battles / "probe_b_scaled.json",
                  battle("probe_b_scaled", {"probe_scaled_foe"}, R"(, "scale_by_party": true)"));
        writeFile(battles / "probe_b_unscaled.json", battle("probe_b_unscaled", {"probe_scaled_foe"}));
        writeFile(battles / "probe_b_twoher.json",
                  R"({"id": "probe_b_twoher", "name": "探针", "chapter": 7, "terrain": "field", "can_escape": true, )"
                  R"("defeat_is_fatal": false, "units": [{"role_id": "probe_ally_v", "faction": "ally"}, )"
                  R"({"role_id": "probe_weak", "faction": "enemy"}]})");
    }
    static void TearDownTestSuite() { root_.reset(); }

    void SetUp() override {
        auto ready = app_.init(root_->path().string(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
        state().hp = state().maxHp = 400;
    }
    void TearDown() override { app_.shutdown(); }

    fanren::core::GameState& state() { return app_.state(); }
    void tick(int frames = 1) {
        for (int i = 0; i < frames; ++i) app_.tick(1.0 / 60.0);
    }

    void party(const std::vector<std::string>& roles) {
        state().party.clear();
        for (const std::string& id : roles) state().party.push_back(fanren::core::PartyMember{id, -1, true});
    }

    // 走真实开战入口：Application::startBattle 压栈，一帧之后场景 onEnter、建好场；此刻还没 update，
    // 局面就停在开场那一刻，可以拿那只手照真实菜单一手一手地打。
    BattleScene* open(const std::string& id, bool preemptive = false) {
        app_.startBattle(BattleStart{id, preemptive}, [](const fanren::script::CommandResult&) {});
        tick();
        return dynamic_cast<BattleScene*>(app_.topScene());
    }

    static int indexOf(const BattleState& b, const std::string& id) {
        for (std::size_t i = 0; i < b.units().size(); ++i) {
            if (b.units()[i].id == id) return static_cast<int>(i);
        }
        return -1;
    }
    static const Unit& at(const BattleState& b, int index) { return b.units()[static_cast<std::size_t>(index)]; }

    static Action make(ActionKind kind, int actor, int target = -1) {
        Action a;
        a.kind = kind;
        a.actorIndex = actor;
        a.targetIndex = target;
        return a;
    }

    static std::vector<BattleEvent> eventsSince(const BattleState& b, std::size_t from) {
        return std::vector<BattleEvent>(b.events().begin() + static_cast<std::ptrdiff_t>(from), b.events().end());
    }
    static std::vector<BattleEvent> only(const std::vector<BattleEvent>& events, BattleEventKind kind) {
        std::vector<BattleEvent> out;
        std::copy_if(events.begin(), events.end(), std::back_inserter(out),
                     [kind](const BattleEvent& e) { return e.kind == kind; });
        return out;
    }
    // 紧跟在第一条 Chase 之后的那一击。
    static const BattleEvent* chaseHit(const std::vector<BattleEvent>& events) {
        for (std::size_t i = 0; i + 1 < events.size(); ++i) {
            if (events[i].kind == BattleEventKind::Chase && events[i + 1].kind == BattleEventKind::Hit) {
                return &events[i + 1];
            }
        }
        return nullptr;
    }
    // 某位此刻做一手普攻（经菜单），返回这一手产生的事件。
    std::vector<BattleEvent> attackWith(BattleScene& scene, int actor, int target) {
        const std::size_t before = scene.battle().events().size();
        fanren::test::BattleHand hand(app_);
        EXPECT_TRUE(hand.issue(scene, make(ActionKind::Attack, actor, target))) << scene.feedback();
        return eventsSince(scene.battle(), before);
    }
    void defend(BattleScene& scene, int actor) {
        fanren::test::BattleHand hand(app_);
        EXPECT_TRUE(hand.issue(scene, make(ActionKind::Defend, actor))) << scene.feedback();
    }
    // 把仗推到 who 能行动为止（中间别的我方单位一律防御）。
    int turnOf(BattleScene& scene, int who) {
        for (int guard = 0; guard < 64; ++guard) {
            const int actor = scene.runToAllyTurn();
            if (actor < 0 || actor == who) return actor;
            defend(scene, actor);
        }
        return -1;
    }

    static std::unique_ptr<fanren::test::TempDir> root_;
    Application app_;
};

std::unique_ptr<fanren::test::TempDir> CompanionsBattle::root_;

// ---------------------------------------------------------------------------
// 3.1 谁上场
// ---------------------------------------------------------------------------

TEST_F(CompanionsBattle, OnlyTheFirstThreeActiveCompanionsTakeTheFieldButEveryOtherMemberDoes) {
    party({"probe_ally_a", "probe_ally_b", "probe_ally_c", "probe_ally_d", "kuilei_shou", "qu_hun"});
    BattleScene* scene = open("probe_b_dummy");
    ASSERT_NE(scene, nullptr);
    const BattleState& b = scene->battle();
    for (const char* id : {"hanli", "probe_ally_a", "probe_ally_b", "probe_ally_c", "kuilei_shou", "qu_hun"}) {
        const int index = indexOf(b, id);
        ASSERT_GE(index, 0) << id << " 应当上场";
        EXPECT_TRUE(at(b, index).ally) << id;
    }
    EXPECT_EQ(indexOf(b, "probe_ally_d"), -1) << "第 4 位伙伴不上场";
    EXPECT_EQ(fanren::game::allyCount(b.units()), 6);
}

TEST_F(CompanionsBattle, AnInactiveCompanionDoesNotTakeOneOfTheThreeSeats) {
    party({"probe_ally_a", "probe_ally_b", "probe_ally_c", "probe_ally_d"});
    state().party[0].active = false;
    BattleScene* scene = open("probe_b_dummy");
    ASSERT_NE(scene, nullptr);
    EXPECT_EQ(indexOf(scene->battle(), "probe_ally_a"), -1);
    EXPECT_GE(indexOf(scene->battle(), "probe_ally_d"), 0) << "a 不参战，d 补上第三位";
}

TEST_F(CompanionsBattle, NonCompanionMembersHaveNoCap) {
    party({"kuilei_shou", "qu_hun", "qu_hun_huashen", "qu_hun_shadan"});
    BattleScene* scene = open("probe_b_dummy");
    ASSERT_NE(scene, nullptr);
    EXPECT_EQ(fanren::game::allyCount(scene->battle().units()), 5) << "韩立 + 四位不是伙伴的队员";
    for (const Unit& u : scene->battle().units()) EXPECT_TRUE(u.companion.empty()) << u.id;
}

// ---------------------------------------------------------------------------
// 四人同场
// ---------------------------------------------------------------------------

TEST_F(CompanionsBattle, FourAlliesEachGetTheirOwnCommandMenu) {
    party({"probe_ally_a", "probe_ally_b", "probe_ally_c"});
    BattleScene* scene = open("probe_b_dummy");
    ASSERT_NE(scene, nullptr);
    std::set<int> commanded;
    for (int step = 0; step < 8; ++step) {
        const int actor = scene->runToAllyTurn();
        ASSERT_GE(actor, 0);
        scene->openMenu(app_);
        ASSERT_EQ(scene->menuMode(), BattleMenuMode::Root) << "轮到 " << at(scene->battle(), actor).name;
        commanded.insert(actor);
        defend(*scene, actor);
    }
    std::set<int> allies;
    for (std::size_t i = 0; i < scene->battle().units().size(); ++i) {
        if (scene->battle().units()[i].ally) allies.insert(static_cast<int>(i));
    }
    EXPECT_EQ(allies.size(), 4u);
    EXPECT_EQ(commanded, allies) << "四个人每人都轮得到下指令";
}

TEST_F(CompanionsBattle, ThePartyPanelHasARowForTheFourthAllyAndTheirFeetStayAboveIt) {
    party({"probe_ally_a", "probe_ally_b", "probe_ally_c"});
    BattleScene* scene = open("probe_b_dummy");
    ASSERT_NE(scene, nullptr);
    const std::vector<Unit>& units = scene->battle().units();
    const fanren::engine::RectF panel = fanren::game::partyPanelRect(fanren::game::allyCount(units));
    // 面板第一行从上沿下 10 开始，一行 34：四行要 10 + 4×34 = 146。
    EXPECT_GE(panel.h, 146.f);
    EXPECT_FLOAT_EQ(panel.y + panel.h, 708.f) << "底边不动";
    const std::vector<fanren::game::StagePoint> stand = fanren::game::standPoints(units);
    for (std::size_t i = 0; i < units.size(); ++i) {
        if (!units[i].ally) continue;
        EXPECT_LT(stand[i].y, panel.y - 16.f) << "我方的脚踩不进加高了的面板：" << units[i].id;
        EXPECT_GE(stand[i].y, 400.f) << "仍站在地面上：" << units[i].id;
    }
    // 三人以内面板与改版前那一块一模一样。
    const fanren::engine::RectF three = fanren::game::partyPanelRect(3);
    EXPECT_FLOAT_EQ(three.x, 848.f);
    EXPECT_FLOAT_EQ(three.y, 588.f);
    EXPECT_FLOAT_EQ(three.w, 416.f);
    EXPECT_FLOAT_EQ(three.h, 120.f);
    EXPECT_FLOAT_EQ(fanren::game::lungeFloorY(3), 600.f);
}

// ---------------------------------------------------------------------------
// 3.2 追击
// ---------------------------------------------------------------------------

TEST_F(CompanionsBattle, WhenHanLiBreaksAFoeACompanionChasesWithADoubledHit) {
    party({"probe_ally_a"});
    state().bonds["probe_a"].value = 20;
    BattleScene* scene = open("probe_b_dummy");
    ASSERT_NE(scene, nullptr);
    const BattleState& b = scene->battle();
    const int hero = indexOf(b, "hanli");
    const int ally = indexOf(b, "probe_ally_a");
    const int dummy = indexOf(b, "probe_dummy");
    ASSERT_EQ(scene->runToAllyTurn(), hero) << "韩立身法 5，先于伙伴（4）";
    const int allyBp = at(b, ally).bp;

    const std::vector<BattleEvent> events = attackWith(*scene, hero, dummy);
    ASSERT_EQ(only(events, BattleEventKind::Break).size(), 1u);
    const std::vector<BattleEvent> chases = only(events, BattleEventKind::Chase);
    ASSERT_EQ(chases.size(), 1u);
    EXPECT_EQ(chases[0].actor, ally);
    EXPECT_EQ(chases[0].target, dummy);
    EXPECT_EQ(chases[0].value, hero) << "记着是谁打出的破势";
    const BattleEvent* hit = chaseHit(events);
    ASSERT_NE(hit, nullptr);
    EXPECT_EQ(hit->actor, ally);
    EXPECT_EQ(hit->category, fanren::core::kCategorySword) << "按追击者自己的兵刃";
    const Unit& a = at(b, ally);
    const Unit& d = at(b, dummy);
    const int plain = fanren::core::battle::physicalDamage(a.attack, d.defence, a.realm, d.realm, a.element, d.element);
    EXPECT_EQ(hit->value, plain * 2) << "吃破势 ×2";
    EXPECT_EQ(a.bp, allyBp) << "不耗劲";
    EXPECT_NE(b.log().back().find("追击"), std::string::npos);
    EXPECT_EQ(scene->runToAllyTurn(), ally) << "追击不占行动：这一回合她照样轮得到";
}

TEST_F(CompanionsBattle, WhenACompanionBreaksAFoeHanLiChases) {
    party({"probe_ally_a"});
    state().bonds["probe_a"].value = 20;
    BattleScene* scene = open("probe_b_dummy");
    ASSERT_NE(scene, nullptr);
    const BattleState& b = scene->battle();
    const int hero = indexOf(b, "hanli");
    const int ally = indexOf(b, "probe_ally_a");
    const int dummy = indexOf(b, "probe_dummy");
    ASSERT_EQ(turnOf(*scene, ally), ally);
    const std::vector<BattleEvent> events = attackWith(*scene, ally, dummy);
    const std::vector<BattleEvent> chases = only(events, BattleEventKind::Chase);
    ASSERT_EQ(chases.size(), 1u);
    EXPECT_EQ(chases[0].actor, hero);
    const BattleEvent* hit = chaseHit(events);
    ASSERT_NE(hit, nullptr);
    const Unit& h = at(b, hero);
    const Unit& d = at(b, dummy);
    EXPECT_EQ(hit->value,
              2 * fanren::core::battle::physicalDamage(h.attack, d.defence, h.realm, d.realm, h.element, d.element));
    EXPECT_EQ(hit->category, fanren::core::kCategoryFist) << "韩立空手：拳";
}

TEST_F(CompanionsBattle, WithTwoCompanionsTheOneWithTheHigherBondChases) {
    party({"probe_ally_a", "probe_ally_b"});
    state().bonds["probe_a"].value = 25;
    state().bonds["probe_b"].value = 40;
    BattleScene* scene = open("probe_b_dummy");
    ASSERT_NE(scene, nullptr);
    const BattleState& b = scene->battle();
    ASSERT_EQ(scene->runToAllyTurn(), indexOf(b, "hanli"));
    const std::vector<BattleEvent> chases =
        only(attackWith(*scene, indexOf(b, "hanli"), indexOf(b, "probe_dummy")), BattleEventKind::Chase);
    ASSERT_EQ(chases.size(), 1u) << "两位伙伴也只追一次";
    EXPECT_EQ(chases[0].actor, indexOf(b, "probe_ally_b"));
}

TEST_F(CompanionsBattle, ATieInBondGoesToTheEarlierPartyMember) {
    party({"probe_ally_b", "probe_ally_a"});
    state().bonds["probe_a"].value = 30;
    state().bonds["probe_b"].value = 30;
    BattleScene* scene = open("probe_b_dummy");
    ASSERT_NE(scene, nullptr);
    const BattleState& b = scene->battle();
    ASSERT_EQ(scene->runToAllyTurn(), indexOf(b, "hanli"));
    const std::vector<BattleEvent> chases =
        only(attackWith(*scene, indexOf(b, "hanli"), indexOf(b, "probe_dummy")), BattleEventKind::Chase);
    ASSERT_EQ(chases.size(), 1u);
    EXPECT_EQ(chases[0].actor, indexOf(b, "probe_ally_b")) << "队伍里 b 排在 a 前面";
}

TEST_F(CompanionsBattle, AGroupAttackThatBreaksTwoFoesChasesOnlyTheFirst) {
    party({"probe_ally_a"});
    state().bonds["probe_a"].value = 20;
    state().realm = fanren::rules::Realm::CoreMid;
    state().mp = state().maxMp = 500;
    state().learnedMagics = {"magic_ch10_wulang"};   // 雾浪：水、敌方全体
    BattleScene* scene = open("probe_b_two");
    ASSERT_NE(scene, nullptr);
    const BattleState& b = scene->battle();
    const int hero = indexOf(b, "hanli");
    ASSERT_EQ(scene->runToAllyTurn(), hero);
    const std::size_t before = b.events().size();
    fanren::test::BattleHand hand(app_);
    Action cast = make(ActionKind::Cast, hero, indexOf(b, "probe_dummy"));
    cast.magicId = "magic_ch10_wulang";
    ASSERT_TRUE(hand.issue(*scene, cast)) << scene->feedback();
    const std::vector<BattleEvent> events = eventsSince(b, before);
    const std::vector<BattleEvent> breaks = only(events, BattleEventKind::Break);
    ASSERT_EQ(breaks.size(), 2u) << "两个木桩都被打到破势";
    const std::vector<BattleEvent> chases = only(events, BattleEventKind::Chase);
    ASSERT_EQ(chases.size(), 1u) << "群攻只追一次";
    EXPECT_EQ(chases[0].target, breaks[0].target) << "追的是第一个进入破势的";
}

TEST_F(CompanionsBattle, NoChaseBelowTheSecondTierWithoutTheChaseFlagOrFromANonCompanion) {
    struct Case {
        std::vector<std::string> party;
        const char* bondId;
        int bond;
        const char* why;
    };
    const std::vector<Case> cases{
        {{"probe_ally_a"}, "probe_a", 19, "默契 19 还在一档"},
        {{"probe_ally_n"}, "probe_n", 90, "这位伙伴的 chase 为假"},
        {{"kuilei_shou"}, "probe_a", 90, "傀儡不是伙伴"},
    };
    for (const Case& c : cases) {
        party(c.party);
        state().bonds.clear();
        state().bonds[c.bondId].value = c.bond;
        BattleScene* scene = open("probe_b_dummy");
        ASSERT_NE(scene, nullptr) << c.why;
        const BattleState& b = scene->battle();
        ASSERT_EQ(turnOf(*scene, indexOf(b, "hanli")), indexOf(b, "hanli")) << c.why;   // 傀儡兽身法 6，先于韩立
        const std::vector<BattleEvent> events = attackWith(*scene, indexOf(b, "hanli"), indexOf(b, "probe_dummy"));
        EXPECT_EQ(only(events, BattleEventKind::Break).size(), 1u) << c.why;
        EXPECT_TRUE(only(events, BattleEventKind::Chase).empty()) << c.why;
        tick(2);   // 无头：这一仗自己跑完、收场、弹栈
    }
}

TEST_F(CompanionsBattle, ABossesExtraActionsAndTheSkippedBrokenRoundsAreNotDisturbedByAChase) {
    party({"probe_ally_a"});
    state().bonds["probe_a"].value = 20;
    BattleScene* scene = open("probe_b_boss");
    ASSERT_NE(scene, nullptr);
    const BattleState& b = scene->battle();
    const int hero = indexOf(b, "hanli");
    const int ally = indexOf(b, "probe_ally_a");
    const int boss = indexOf(b, "probe_boss");
    ASSERT_EQ(scene->runToAllyTurn(), hero);
    const std::vector<int> order = b.roundOrder();
    ASSERT_EQ(std::count(order.begin(), order.end(), boss), 2) << "首领一回合两手";
    const std::size_t position = b.orderPosition();
    const std::size_t start = b.events().size();

    const std::vector<BattleEvent> events = attackWith(*scene, hero, boss);
    ASSERT_EQ(only(events, BattleEventKind::Chase).size(), 1u);
    EXPECT_EQ(b.roundOrder(), order) << "追击不改这一回合的行动序";
    EXPECT_EQ(b.orderPosition(), position + 1) << "只走了韩立这一位";
    EXPECT_EQ(scene->runToAllyTurn(), ally) << "伙伴照样轮到";
    defend(*scene, ally);

    // 破势在第 1 回合：首领失去第 1 回合余下的两手与第 2 回合，第 3 回合两手照常。
    while (b.round() < 4 && b.phase() == BattlePhase::Ongoing) {
        const int actor = scene->runToAllyTurn();
        if (actor < 0) break;
        defend(*scene, actor);
    }
    int actsBefore3 = 0;
    int actsIn3 = 0;
    int round = 0;
    for (std::size_t i = start; i < b.events().size(); ++i) {
        const BattleEvent& e = b.events()[i];
        if (e.kind == BattleEventKind::RoundStart) round = e.value;
        if (e.kind != BattleEventKind::Act || e.actor != boss) continue;
        if (round < 3) ++actsBefore3;
        if (round == 3) ++actsIn3;
    }
    EXPECT_EQ(actsBefore3, 0) << "破势的那一回合余下的两手、下一整回合都没了";
    EXPECT_EQ(actsIn3, 2) << "第 3 回合恢复，仍是两手";
}

// ---------------------------------------------------------------------------
// 3.3 合招（经真实菜单）
// ---------------------------------------------------------------------------

TEST_F(CompanionsBattle, ADuoFromTheMenuSpendsTwoBpEachUsesBothTurnsAndSkipsNextRoundsBp) {
    party({"probe_ally_n"});
    state().bonds["probe_n"].value = 70;
    BattleScene* scene = open("probe_b_dummy");
    ASSERT_NE(scene, nullptr);
    const BattleState& b = scene->battle();
    const int hero = indexOf(b, "hanli");
    const int ally = indexOf(b, "probe_ally_n");
    const int dummy = indexOf(b, "probe_dummy");

    // 第 1 回合：各 1 点劲，合招那一行是灰的、写明劲不够。
    ASSERT_EQ(scene->runToAllyTurn(), hero);
    scene->openMenu(app_);
    ASSERT_EQ(scene->menuList().count(), fanren::game::kBattleMenuCount + 1) << "有合招可出：多出第六项";
    ASSERT_EQ(scene->menuList().items()[fanren::game::kBattleMenuDuo].label, "合招");
    ASSERT_TRUE(scene->menuChoose(app_, fanren::game::kBattleMenuDuo));
    ASSERT_EQ(scene->menuMode(), BattleMenuMode::Duo);
    ASSERT_GE(scene->menuList().count(), 2);
    EXPECT_FALSE(scene->menuList().items()[0].enabled);
    EXPECT_NE(scene->menuList().items()[0].disabledReason.find("劲不够"), std::string::npos)
        << scene->menuList().items()[0].disabledReason;
    scene->closeMenu();
    defend(*scene, hero);
    ASSERT_EQ(scene->runToAllyTurn(), ally);
    defend(*scene, ally);

    // 第 2 回合：各 2 点劲。韩立经菜单发起合招。
    ASSERT_EQ(scene->runToAllyTurn(), hero);
    ASSERT_EQ(b.round(), 2);
    ASSERT_EQ(at(b, hero).bp, 2);
    ASSERT_EQ(at(b, ally).bp, 2);
    const std::size_t before = b.events().size();
    fanren::test::BattleHand hand(app_);
    ASSERT_TRUE(hand.issue(*scene, fanren::test::BattleHand::duo(b, hero, ally, dummy))) << scene->feedback();
    const std::vector<BattleEvent> events = eventsSince(b, before);
    EXPECT_EQ(at(b, hero).bp, 0) << "各扣 2 点";
    EXPECT_EQ(at(b, ally).bp, 0);
    const std::vector<BattleEvent> acts = only(events, BattleEventKind::Act);
    ASSERT_EQ(acts.size(), 1u);
    EXPECT_EQ(acts[0].actor, hero) << "发起人施放";
    EXPECT_EQ(acts[0].value, static_cast<int>(ActionKind::Duo));
    const std::vector<BattleEvent> joins = only(events, BattleEventKind::DuoJoin);
    ASSERT_EQ(joins.size(), 1u);
    EXPECT_EQ(joins[0].target, ally);
    const std::vector<BattleEvent> hits = only(events, BattleEventKind::Hit);
    ASSERT_EQ(hits.size(), 1u) << "不能叠蓄劲：一击";
    const Unit& h = at(b, hero);
    const Unit& d = at(b, dummy);
    EXPECT_EQ(hits[0].value, fanren::core::battle::magicDamage(kDuoPowerTimes16, d.defence, h.realm, d.realm, 4,
                                                               d.element))
        << "威力 ×1.6，境界按发起人";

    // 她这一回合的行动也用掉了：下一位轮到的我方已经是第 3 回合的韩立。
    ASSERT_EQ(scene->runToAllyTurn(), hero);
    EXPECT_EQ(b.round(), 3);
    for (std::size_t i = before; i < b.events().size(); ++i) {
        const BattleEvent& e = b.events()[i];
        EXPECT_FALSE(e.kind == BattleEventKind::Act && e.actor == ally) << "合招之后的第 2 回合她没有再出手";
    }
    EXPECT_EQ(at(b, hero).bp, 0) << "第 3 回合开始不加劲";
    EXPECT_EQ(at(b, ally).bp, 0);
    defend(*scene, hero);
    ASSERT_EQ(scene->runToAllyTurn(), ally);
    defend(*scene, ally);
    ASSERT_EQ(scene->runToAllyTurn(), hero);
    EXPECT_EQ(b.round(), 4);
    EXPECT_EQ(at(b, hero).bp, 1) << "第 4 回合照常加";
    EXPECT_EQ(at(b, ally).bp, 1);
}

TEST_F(CompanionsBattle, ADuoBelowTheFourthTierIsGreyedWithTheReason) {
    party({"probe_ally_n"});
    state().bonds["probe_n"].value = 69;
    BattleScene* scene = open("probe_b_dummy", /*preemptive=*/true);   // 先手：开场就有 2 点劲
    ASSERT_NE(scene, nullptr);
    ASSERT_EQ(scene->runToAllyTurn(), indexOf(scene->battle(), "hanli"));
    scene->openMenu(app_);
    ASSERT_TRUE(scene->menuChoose(app_, fanren::game::kBattleMenuDuo));
    EXPECT_FALSE(scene->menuList().items()[0].enabled);
    EXPECT_NE(scene->menuList().items()[0].disabledReason.find("知己"), std::string::npos)
        << scene->menuList().items()[0].disabledReason;
}

TEST_F(CompanionsBattle, ADuoNeedsAPartnerWhoHasNotActedThisRound) {
    party({"probe_ally_e"});   // 身法 30：每回合她先动
    state().bonds["probe_e"].value = 80;
    BattleScene* scene = open("probe_b_dummy", /*preemptive=*/true);
    ASSERT_NE(scene, nullptr);
    const BattleState& b = scene->battle();
    const int ally = indexOf(b, "probe_ally_e");
    ASSERT_EQ(scene->runToAllyTurn(), ally);
    defend(*scene, ally);
    ASSERT_EQ(scene->runToAllyTurn(), indexOf(b, "hanli"));
    scene->openMenu(app_);
    ASSERT_TRUE(scene->menuChoose(app_, fanren::game::kBattleMenuDuo));
    EXPECT_FALSE(scene->menuList().items()[0].enabled);
    EXPECT_NE(scene->menuList().items()[0].disabledReason.find("已经行动过"), std::string::npos)
        << scene->menuList().items()[0].disabledReason;
}

TEST_F(CompanionsBattle, ACompanionCanStartTheDuoAndHanLiLosesHisTurn) {
    party({"probe_ally_e"});
    state().bonds["probe_e"].value = 80;
    BattleScene* scene = open("probe_b_dummy", /*preemptive=*/true);
    ASSERT_NE(scene, nullptr);
    const BattleState& b = scene->battle();
    const int hero = indexOf(b, "hanli");
    const int ally = indexOf(b, "probe_ally_e");
    ASSERT_EQ(scene->runToAllyTurn(), ally);
    const std::size_t before = b.events().size();
    fanren::test::BattleHand hand(app_);
    ASSERT_TRUE(hand.issue(*scene, fanren::test::BattleHand::duo(b, ally, hero, indexOf(b, "probe_dummy"))))
        << scene->feedback();
    const std::vector<BattleEvent> acts = only(eventsSince(b, before), BattleEventKind::Act);
    ASSERT_EQ(acts.size(), 1u);
    EXPECT_EQ(acts[0].actor, ally);
    ASSERT_EQ(scene->runToAllyTurn(), ally) << "韩立这一回合的行动被合招用掉了，下一位就是第 2 回合的她";
    EXPECT_EQ(b.round(), 2);
}

TEST_F(CompanionsBattle, BattlesWithoutCompanionsKeepTheFiveItemMenu) {
    party({"kuilei_shou"});
    BattleScene* scene = open("probe_b_dummy");
    ASSERT_NE(scene, nullptr);
    ASSERT_GE(scene->runToAllyTurn(), 0);
    scene->openMenu(app_);
    EXPECT_EQ(scene->menuList().count(), fanren::game::kBattleMenuCount);
}

// ---------------------------------------------------------------------------
// 3.4 先手（真实开战入口）
// ---------------------------------------------------------------------------

TEST_F(CompanionsBattle, APreemptiveStartGivesEveryAllyAnExtraBpAndTheWholeFirstRound) {
    party({"probe_ally_a", "kuilei_shou"});
    BattleScene* plain = open("probe_b_fast");
    ASSERT_NE(plain, nullptr);
    {
        const BattleState& b = plain->battle();
        EXPECT_FALSE(b.units()[static_cast<std::size_t>(b.roundOrder().front())].ally) << "不偷袭：身法 40 的敌人先动";
        for (const Unit& u : b.units()) {
            if (u.ally) EXPECT_EQ(u.bp, 1) << u.id;
        }
    }
    tick(2);   // 无头：这一仗自己跑完、收场、弹栈

    BattleScene* ambush = open("probe_b_fast", /*preemptive=*/true);
    ASSERT_NE(ambush, nullptr);
    const BattleState& b = ambush->battle();
    EXPECT_TRUE(b.preemptive());
    const std::vector<int> order = b.roundOrder();
    const int foe = indexOf(b, "probe_fast_foe");
    ASSERT_EQ(order.size(), 4u);
    EXPECT_EQ(order.back(), foe) << "第一回合我方全员在所有敌人之前";
    for (const Unit& u : b.units()) {
        if (u.ally) EXPECT_EQ(u.bp, 2) << u.id << "：开场多 1 点劲";
    }
    // 只管第一回合：第二回合又是身法 40 的它先动。第一回合出手用普攻，不用防御——防御给的是
    //「下回合先手」，会把第二回合的序搅乱，那是另一个键。
    for (int step = 0; step < 3; ++step) {
        const int actor = ambush->runToAllyTurn();
        ASSERT_GE(actor, 0);
        static_cast<void>(attackWith(*ambush, actor, foe));
    }
    ASSERT_GE(ambush->runToAllyTurn(), 0);
    EXPECT_EQ(b.round(), 2);
    EXPECT_EQ(b.roundOrder().front(), foe);
}

TEST_F(CompanionsBattle, AStalePreemptiveFlagFromAnotherBattleDoesNotApply) {
    app_.startBattle(BattleStart{"probe_b_dummy", true}, [](const fanren::script::CommandResult&) {});
    tick();
    // 截图口那条路：直接压一个场景，battleStart() 还是上一仗的。
    app_.pushScene(std::make_unique<BattleScene>("probe_b_fast"));
    tick();
    auto* scene = dynamic_cast<BattleScene*>(app_.topScene());
    ASSERT_NE(scene, nullptr);
    EXPECT_FALSE(scene->battle().preemptive());
}

// ---------------------------------------------------------------------------
// 3.10 人数系数（真实开战入口）
// ---------------------------------------------------------------------------

TEST_F(CompanionsBattle, ScaleByPartyMultipliesFoeHpByAlliesOnFieldButNotToughness) {
    struct Case {
        std::vector<std::string> party;
        int allies;
        int hp;
    };
    const std::vector<Case> cases{
        {{}, 1, 100},
        {{"probe_ally_a"}, 2, 135},
        {{"probe_ally_a", "probe_ally_b"}, 3, 170},
        {{"probe_ally_a", "probe_ally_b", "probe_ally_c"}, 4, 200},
        {{"probe_ally_a", "probe_ally_b", "probe_ally_c", "kuilei_shou", "qu_hun"}, 6, 200},
    };
    for (const Case& c : cases) {
        party(c.party);
        BattleScene* scene = open("probe_b_scaled");
        ASSERT_NE(scene, nullptr);
        const BattleState& b = scene->battle();
        ASSERT_EQ(fanren::game::allyCount(b.units()), c.allies);
        const Unit& foe = at(b, indexOf(b, "probe_scaled_foe"));
        EXPECT_EQ(foe.maxHp, c.hp) << c.allies << " 人";
        EXPECT_EQ(foe.hp, c.hp) << c.allies << " 人";
        EXPECT_EQ(foe.maxToughness, kScaledFoeToughness) << "架势不乘";
        tick(2);   // 无头：这一仗自己跑完、收场、弹栈
    }
    party({"probe_ally_a", "probe_ally_b", "probe_ally_c"});
    BattleScene* plain = open("probe_b_unscaled");
    ASSERT_NE(plain, nullptr);
    EXPECT_EQ(at(plain->battle(), indexOf(plain->battle(), "probe_scaled_foe")).maxHp, kScaledFoeHp)
        << "没写 scale_by_party 的编成不乘";
}

TEST(CompanionsBattleRepo, NoExistingFormationScalesByParty) {
    Application app;
    fs::path root;
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "data" / "text" / "ch01_main.json")) root = candidate;
    }
    ASSERT_FALSE(root.empty());
    ASSERT_TRUE(app.init(root.string(), true).ok);
    // 只核第 1–13 章的旧编成（id 以 b01_ … b13_ 起头）：样板段的明雷、历练编成会开它，不在此列，也不断言总数。
    int count = 0;
    for (const fs::directory_entry& entry : fs::directory_iterator(root / "data" / "battles")) {
        if (entry.path().extension() != ".json") continue;
        const std::string id = entry.path().stem().string();
        if (id.size() < 4 || id[0] != 'b' || id[3] != '_') continue;
        const int chapter = std::atoi(id.substr(1, 2).c_str());
        if (chapter < 1 || chapter > 13) continue;
        const fanren::core::BattleSetup* setup = app.battleSetup(id);
        ASSERT_NE(setup, nullptr) << entry.path();
        EXPECT_FALSE(setup->scaleByParty) << setup->id;
        ++count;
    }
    EXPECT_GT(count, 0) << "一份旧编成也没找到：这条什么也没核";
    app.shutdown();
}

// ---------------------------------------------------------------------------
// 2.2 打赢之后记默契
// ---------------------------------------------------------------------------

TEST_F(CompanionsBattle, WinningAddsTwoBondToEachCompanionOnFieldUpToSixADay) {
    party({"probe_ally_a", "probe_ally_b", "kuilei_shou"});
    state().day = 5;
    for (int fight = 1; fight <= 4; ++fight) {
        app_.startBattle(BattleStart{"probe_b_weak"}, [](const fanren::script::CommandResult&) {});
        tick(4);   // 无头：压栈、跑完、收场、弹栈
        ASSERT_NE(app_.topScene() == nullptr ? std::string{} : app_.topScene()->name(), "Battle");
        const int expected = std::min(6, 2 * fight);
        EXPECT_EQ(state().bondOf("probe_a"), expected) << "第 " << fight << " 仗";
        EXPECT_EQ(state().bondOf("probe_b"), expected) << "第 " << fight << " 仗";
    }
    EXPECT_EQ(state().bonds.count("kuilei_shou"), 0u) << "傀儡不是伙伴，没有默契账";
    state().day = 6;
    app_.startBattle(BattleStart{"probe_b_weak"}, [](const fanren::script::CommandResult&) {});
    tick(4);
    EXPECT_EQ(state().bondOf("probe_a"), 8) << "换了一天又能加";
}

TEST_F(CompanionsBattle, LosingAddsNoBond) {
    party({"probe_ally_a"});
    app_.startBattle(BattleStart{"probe_b_killer"}, [](const fanren::script::CommandResult&) {});
    tick(4);
    EXPECT_EQ(state().bondOf("probe_a"), 0);
}

TEST_F(CompanionsBattle, TheBondGoesThroughTheRealSceneAndIsReportedPerCompanion) {
    party({"probe_ally_a"});
    state().bonds["probe_a"].value = 5;
    BattleScene* scene = open("probe_b_weak");
    ASSERT_NE(scene, nullptr);
    ASSERT_TRUE(scene->runToCompletion());
    ASSERT_FALSE(scene->update(app_, 1.0 / 60.0)) << "无头：收场";
    ASSERT_EQ(scene->bondGained().count("probe_a"), 1u);
    EXPECT_EQ(scene->bondGained().at("probe_a"), 2);
    EXPECT_EQ(state().bondOf("probe_a"), 7);
}

// ---------------------------------------------------------------------------
// 编成写死的友军 + 队伍里同一个人的另一段角色：两个她，不崩
// ---------------------------------------------------------------------------

TEST_F(CompanionsBattle, TheSamePersonInTheFormationAndInThePartyDoesNotBreakTheBattle) {
    party({"probe_ally_w"});
    state().bonds["probe_v"].value = 30;
    BattleScene* scene = open("probe_b_twoher");
    ASSERT_NE(scene, nullptr);
    const BattleState& b = scene->battle();
    ASSERT_GE(indexOf(b, "probe_ally_v"), 0);
    ASSERT_GE(indexOf(b, "probe_ally_w"), 0);
    EXPECT_TRUE(at(b, indexOf(b, "probe_ally_v")).companion.empty()) << "编成里写死的友军不按伙伴算";
    EXPECT_EQ(at(b, indexOf(b, "probe_ally_w")).companion, "probe_v");
    EXPECT_TRUE(scene->runToCompletion());
    ASSERT_FALSE(scene->update(app_, 1.0 / 60.0));
    EXPECT_EQ(state().bondOf("probe_v"), 32) << "只记一次";
}

// ---------------------------------------------------------------------------
// 验收补测（协调者 2026-11-18）
// ---------------------------------------------------------------------------

TEST_F(CompanionsBattle, HanLiChasesAtMostOncePerRound) {
    party({"probe_ally_a", "probe_ally_b"});
    state().bonds["probe_a"].value = 20;
    state().bonds["probe_b"].value = 20;
    BattleScene* scene = open("probe_b_two");
    ASSERT_NE(scene, nullptr);
    const BattleState& b = scene->battle();
    const int hero = indexOf(b, "hanli");
    const int a = indexOf(b, "probe_ally_a");
    const int bb = indexOf(b, "probe_ally_b");
    std::vector<int> dummies;
    for (std::size_t i = 0; i < b.units().size(); ++i) {
        if (b.units()[i].id == "probe_dummy") dummies.push_back(static_cast<int>(i));
    }
    ASSERT_EQ(dummies.size(), 2u);
    ASSERT_EQ(scene->runToAllyTurn(), hero);
    defend(*scene, hero);
    // 两位伙伴先后各打出一个破势：头一次韩立追，第二次不追（他这一回合追过了）。
    const int first = scene->runToAllyTurn();
    ASSERT_TRUE(first == a || first == bb);
    const std::vector<BattleEvent> once = attackWith(*scene, first, dummies[0]);
    ASSERT_EQ(only(once, BattleEventKind::Break).size(), 1u);
    ASSERT_EQ(only(once, BattleEventKind::Chase).size(), 1u);
    EXPECT_EQ(only(once, BattleEventKind::Chase)[0].actor, hero);
    const int second = scene->runToAllyTurn();
    ASSERT_EQ(second, first == a ? bb : a);
    ASSERT_EQ(b.round(), 1);
    const std::vector<BattleEvent> twice = attackWith(*scene, second, dummies[1]);
    ASSERT_EQ(only(twice, BattleEventKind::Break).size(), 1u);
    EXPECT_TRUE(only(twice, BattleEventKind::Chase).empty()) << "韩立每回合至多追一次";
}

TEST_F(CompanionsBattle, AGroupAttackWhoseFirstBrokenFoeFallsInTheSameActionChasesNobody) {
    party({"probe_ally_a"});
    state().bonds["probe_a"].value = 20;
    state().realm = fanren::rules::Realm::QiRefining1;   // 与探针敌人同境界：没有境界压制
    state().learnedMagics = {"magic_probe_rain"};
    BattleScene* scene = open("probe_b_frail");
    ASSERT_NE(scene, nullptr);
    const BattleState& b = scene->battle();
    const int hero = indexOf(b, "hanli");
    const int frail = indexOf(b, "probe_frail");
    ASSERT_EQ(scene->runToAllyTurn(), hero);
    const std::size_t before = b.events().size();
    fanren::test::BattleHand hand(app_);
    Action rain = make(ActionKind::Cast, hero, frail);
    rain.magicId = "magic_probe_rain";
    rain.boost = 1;   // 连发两下
    ASSERT_TRUE(hand.issue(*scene, rain)) << scene->feedback();
    const std::vector<BattleEvent> events = eventsSince(b, before);
    const std::vector<BattleEvent> breaks = only(events, BattleEventKind::Break);
    ASSERT_EQ(breaks.size(), 2u) << "两个都破势";
    ASSERT_EQ(breaks[0].target, frail) << "朽木先破势";
    EXPECT_FALSE(at(b, frail).alive()) << "朽木在同一手里被打倒";
    EXPECT_TRUE(only(events, BattleEventKind::Chase).empty()) << "第一个破势的倒了：不追，也不改去追第二个";
}

TEST_F(CompanionsBattle, ADuoWithBoostIsRefused) {
    party({"probe_ally_n"});
    state().bonds["probe_n"].value = 70;
    BattleScene* scene = open("probe_b_dummy", /*preemptive=*/true);
    ASSERT_NE(scene, nullptr);
    const BattleState& b = scene->battle();
    const int hero = indexOf(b, "hanli");
    const int ally = indexOf(b, "probe_ally_n");
    ASSERT_EQ(scene->runToAllyTurn(), hero);
    Action duo = fanren::test::BattleHand::duo(b, hero, ally, indexOf(b, "probe_dummy"));
    duo.boost = 1;
    const std::size_t before = b.events().size();
    const auto refused = scene->issuePlayerAction(app_, duo);
    EXPECT_FALSE(refused.ok) << "合招不能叠蓄劲";
    EXPECT_NE(refused.error.find("蓄"), std::string::npos) << refused.error;
    EXPECT_EQ(b.events().size(), before) << "回绝不动局面";
    EXPECT_EQ(at(b, hero).bp, 2);
    EXPECT_EQ(at(b, ally).bp, 2);
    duo.boost = 0;
    EXPECT_TRUE(scene->issuePlayerAction(app_, duo).ok) << "不蓄劲就出得来";
}

TEST_F(CompanionsBattle, ADuoDoesNotTriggerAChaseEvenWithAThirdCompanionOnField) {
    party({"probe_ally_x", "probe_ally_a"});
    state().bonds["probe_x"].value = 80;   // chase 为真、有合招
    state().bonds["probe_a"].value = 60;   // 第三位：chase 为真、默契够追
    BattleScene* scene = open("probe_b_dummy", /*preemptive=*/true);
    ASSERT_NE(scene, nullptr);
    const BattleState& b = scene->battle();
    const int hero = indexOf(b, "hanli");
    ASSERT_EQ(scene->runToAllyTurn(), hero);
    const std::size_t before = b.events().size();
    fanren::test::BattleHand hand(app_);
    ASSERT_TRUE(hand.issue(*scene, fanren::test::BattleHand::duo(b, hero, indexOf(b, "probe_ally_x"),
                                                                 indexOf(b, "probe_dummy"))))
        << scene->feedback();
    const std::vector<BattleEvent> events = eventsSince(b, before);
    ASSERT_EQ(only(events, BattleEventKind::Break).size(), 1u) << "合招打出了破势";
    EXPECT_TRUE(only(events, BattleEventKind::Chase).empty()) << "合招这一手不触发追击";
}

TEST_F(CompanionsBattle, ADuoPartnersGuardEndsTheMomentTheDuoIsCalled) {
    party({"probe_ally_n"});
    state().bonds["probe_n"].value = 70;
    BattleScene* scene = open("probe_b_dummy");
    ASSERT_NE(scene, nullptr);
    const BattleState& b = scene->battle();
    const int hero = indexOf(b, "hanli");
    const int ally = indexOf(b, "probe_ally_n");
    // 第 1 回合两人都防御：第 2 回合两人都有先手，韩立身法快，排在她前面；她仍在守势里。
    ASSERT_EQ(scene->runToAllyTurn(), hero);
    defend(*scene, hero);
    ASSERT_EQ(scene->runToAllyTurn(), ally);
    defend(*scene, ally);
    ASSERT_EQ(scene->runToAllyTurn(), hero);
    ASSERT_EQ(b.round(), 2);
    ASSERT_TRUE(at(b, ally).guarding);
    const std::size_t logBefore = b.log().size();
    const std::size_t before = b.events().size();
    fanren::test::BattleHand hand(app_);
    ASSERT_TRUE(hand.issue(*scene, fanren::test::BattleHand::duo(b, hero, ally, indexOf(b, "probe_dummy"))))
        << scene->feedback();
    EXPECT_FALSE(at(b, ally).guarding) << "陪着出了合招，这一回合的行动就用掉了：守势当场散去";
    const std::vector<BattleEvent> ends = only(eventsSince(b, before), BattleEventKind::GuardEnd);
    ASSERT_EQ(ends.size(), 1u);
    EXPECT_EQ(ends[0].actor, ally);
    bool said = false;
    for (std::size_t i = logBefore; i < b.log().size(); ++i) {
        said = said || b.log()[i] == at(b, ally).name + " 收起守势";
    }
    EXPECT_TRUE(said) << "日志上与轮到本人时同一说法";
}

TEST_F(CompanionsBattle, ARevealDuoIsCalledStraightFromThePartnerRow) {
    party({"probe_ally_r"});
    state().bonds["probe_r"].value = 70;
    BattleScene* scene = open("probe_b_dummy", /*preemptive=*/true);
    ASSERT_NE(scene, nullptr);
    const BattleState& b = scene->battle();
    const int hero = indexOf(b, "hanli");
    const int ally = indexOf(b, "probe_ally_r");
    ASSERT_EQ(scene->runToAllyTurn(), hero);
    const std::size_t before = b.events().size();
    fanren::test::BattleHand hand(app_);
    ASSERT_TRUE(hand.issue(*scene, fanren::test::BattleHand::duo(b, hero, ally, -1))) << scene->feedback();
    EXPECT_EQ(scene->menuMode(), BattleMenuMode::Closed) << "看破不挑目标：选中就出";
    const std::vector<BattleEvent> events = eventsSince(b, before);
    const std::vector<BattleEvent> acts = only(events, BattleEventKind::Act);
    ASSERT_EQ(acts.size(), 1u);
    EXPECT_EQ(acts[0].value, static_cast<int>(ActionKind::Duo));
    EXPECT_EQ(only(events, BattleEventKind::Reveal).size(), 1u);
    EXPECT_TRUE(only(events, BattleEventKind::Hit).empty());
    EXPECT_EQ(at(b, hero).bp, 0);
    EXPECT_EQ(at(b, ally).bp, 0);
    const Unit& dummy = at(b, indexOf(b, "probe_dummy"));
    EXPECT_EQ(dummy.revealed, dummy.weaknesses) << "破绽全部揭开";
}

TEST_F(CompanionsBattle, ACompanionWhoFellStillGetsTheBondForTheWin) {
    state().party = {fanren::core::PartyMember{"probe_ally_a", 1, true}};
    BattleScene* scene = open("probe_b_striker");
    ASSERT_NE(scene, nullptr);
    ASSERT_TRUE(scene->runToCompletion());
    EXPECT_FALSE(at(scene->battle(), indexOf(scene->battle(), "probe_ally_a")).alive()) << "她先倒下了";
    ASSERT_FALSE(scene->update(app_, 1.0 / 60.0));
    EXPECT_EQ(state().bondOf("probe_a"), 2) << "上过场、并肩打赢，倒下了也记";
}

TEST(CompanionsBattleLoader, AScaleByPartyThatIsNotABoolIsAnError) {
    const fanren::test::TempDir dir("fanren_scale_by_party");
    const fs::path file = dir.path() / "b_probe.json";
    writeFile(file, battle("b_probe", {"wild_wolf"}, R"(, "scale_by_party": "true")"));
    const auto loaded = fanren::io::loadBattle(file.string());
    ASSERT_FALSE(loaded.ok);
    EXPECT_NE(loaded.error.find("scale_by_party"), std::string::npos) << loaded.error;
    writeFile(file, battle("b_probe", {"wild_wolf"}, R"(, "scale_by_party": true)"));
    const auto good = fanren::io::loadBattle(file.string());
    ASSERT_TRUE(good.ok) << good.error;
    EXPECT_TRUE(good.value.scaleByParty);
}

TEST(CompanionsBattlePanel, ThePanelStopsGrowingAtSixRowsAndASeventhAllyHasNoRow) {
    const fanren::engine::RectF six = fanren::game::partyPanelRect(6);
    const fanren::engine::RectF seven = fanren::game::partyPanelRect(7);
    EXPECT_FLOAT_EQ(seven.y, six.y) << "6 行封顶";
    EXPECT_FLOAT_EQ(seven.h, six.h);
    EXPECT_FLOAT_EQ(six.h, 120.f + 3 * 34.f);
    // 面板第一行从上沿下 10 起、一行 34：6 行放得下，第 7 行出了底边（drawPartyPanel 就停在那里）。
    EXPECT_LE(10.f + 6 * 34.f, six.h);
    EXPECT_GT(10.f + 7 * 34.f, seven.h);
}

}  // namespace
