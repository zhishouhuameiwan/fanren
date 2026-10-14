// Chapter 12 E1–E2: docs/interfaces-p3-ch12.md sections 1.3 and 2.3.
// Chapter 12 has no code item: both E entries are verification cases that pin engine behaviour
// the content track leans on and no earlier chapter exercised. Cases that read the content
// track's chapter-12 files FAIL when a file is missing or still the old placeholder: they were
// delivered with GTEST_SKIP guards, and the coordinator turned all 11 into FAIL() at integration
// once the data had landed (notes/ch12/integration.md), so a missing file is red, never skipped.
#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iterator>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "ChapterFixture.h"
#include "TempDir.h"
#include "core/battle/Battle.h"
#include "core/model/AttackCategory.h"
#include "core/model/Types.h"
#include "core/rules/Realm.h"
#include "game/Application.h"
#include "game/BattleScene.h"
#include "game/MenuScene.h"
#include "io/BattleLoader.h"
#include "io/DataLoader.h"
#include "io/SaveFile.h"
#include "script/Command.h"

// Explicit instantiation may name a private member without touching the production header
// (the Ch09EngineTests pattern). MenuScene::Member is private too, so the pointer's type stays
// a template parameter and the friend copies out only what the tests read. Contract 1.3 item 4
// only demands "no crash"; the row list is an extra read through this probe.
namespace {

struct MenuRow {
    std::string roleId;
    bool hero = false;
};
struct MenuMembersTag {};
std::vector<MenuRow> menuRows(const fanren::game::MenuScene& menu, MenuMembersTag);

template <auto Member>
struct MenuMembersProbe {
    friend std::vector<MenuRow> menuRows(const fanren::game::MenuScene& menu, MenuMembersTag) {
        std::vector<MenuRow> rows;
        for (const auto& member : menu.*Member) rows.push_back(MenuRow{member.roleId, member.hero});
        return rows;
    }
};
template struct MenuMembersProbe<&fanren::game::MenuScene::members_>;

namespace fs = std::filesystem;
namespace core = fanren::core;
namespace io = fanren::io;
namespace game = fanren::game;
namespace battle = fanren::core::battle;
using core::GameState;
using fanren::test::TempDir;
using game::Application;
using game::BattleScene;
using game::MenuScene;

constexpr const char* kQuHun = "qu_hun_shadan";
constexpr const char* kBorrowedBattle = "b11_tiandu_shi";        // E1 item 2
constexpr const char* kFriendBattle = "b08_lingkuang_shouzhen";  // E2 items 1, 3: xuan_le ally
constexpr const char* kFoeBattle = "b08_zhongrudong";            // E2 items 1, 3: xuan_le enemy
constexpr const char* kTwoSided = "xuan_le";
constexpr double kFrame = 1.0 / 60.0;

fs::path projectRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        const fs::path root(candidate);
        if (fs::exists(root / "maps/ch01_hanjiacun.tmj") && fs::exists(root / "data/battles")) return root;
    }
    return {};
}

void writeFile(const fs::path& path, const std::string& content) {
    fs::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out << content;
}

std::string readBytes(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

int categoryOf(const char* name) {
    const int bit = core::categoryFromName(name);
    EXPECT_TRUE(core::isSingleCategory(bit)) << name;
    return bit;
}

int allyCount(const battle::BattleState& state) {
    return static_cast<int>(std::count_if(state.units().begin(), state.units().end(),
                                          [](const battle::Unit& u) { return u.ally; }));
}

int countId(const battle::BattleState& state, const std::string& id) {
    return static_cast<int>(std::count_if(state.units().begin(), state.units().end(),
                                          [&id](const battle::Unit& u) { return u.id == id; }));
}

// The only unit with this id; fails the test (and returns null) when there is none or several.
const battle::Unit* uniqueUnit(const battle::BattleState& state, const std::string& id) {
    const battle::Unit* found = nullptr;
    for (const battle::Unit& unit : state.units()) {
        if (unit.id != id) continue;
        EXPECT_EQ(found, nullptr) << id << " 在场上不止一个";
        found = &unit;
    }
    EXPECT_NE(found, nullptr) << id << " 不在场上";
    return found;
}

std::vector<std::string> sortedAllyRoles(const core::BattleSetup& setup) {
    std::vector<std::string> ids;
    for (const core::BattleUnitSpec& unit : setup.units) {
        if (unit.ally) ids.push_back(unit.roleId);
    }
    std::sort(ids.begin(), ids.end());
    return ids;
}

// Contract 2.3 item 5: every metal-element spell that an ally of this setup can cast.
std::vector<std::string> alliesCastMetal(const core::BattleSetup& setup, const core::GameData& data) {
    const int metal = core::categoryFromName("金");
    std::vector<std::string> found;
    for (const core::BattleUnitSpec& unit : setup.units) {
        if (!unit.ally) continue;
        const core::RoleTemplate* role = data.findRole(unit.roleId);
        if (role == nullptr) continue;
        for (const std::string& id : role->magics) {
            const core::Magic* magic = data.findMagic(id);
            if (magic != nullptr && (core::categoriesOfElement(magic->element) & metal) != 0) found.push_back(id);
        }
    }
    return found;
}

// Contract 2.3 item 4: enemy roles whose killable_by reaches outside their weaknesses. Rule 24
// and the loader only refuse an empty array, so this is the only place the subset is checked.
std::set<std::string> killableOutsideWeaknesses(const std::vector<core::BattleSetup>& setups,
                                                const core::GameData& data) {
    std::set<std::string> bad;
    for (const core::BattleSetup& setup : setups) {
        for (const core::BattleUnitSpec& unit : setup.units) {
            if (unit.ally) continue;
            const core::RoleTemplate* role = data.findRole(unit.roleId);
            if (role != nullptr && (role->killableBy & ~role->weaknesses) != 0) bad.insert(role->id);
        }
    }
    return bad;
}

std::vector<MenuRow> menuRows(const MenuScene& menu) { return menuRows(menu, MenuMembersTag{}); }

// ===========================================================================
// Shared fixture (contract 1.3 head): headless Application on tests/fixtures/ch10-end-first.sav,
// whose party is exactly qu_hun_shadan. Probes run through startEvent from a temporary
// scriptRoot holding the real common/api.lua (the Ch11Fixture pattern).
// ===========================================================================
class Ch12Fixture : public ::testing::Test {
protected:
    void SetUp() override {
        root_ = projectRoot();
        ASSERT_FALSE(root_.empty());
        const auto ready = app_.init(root_.string(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
        const auto loaded = io::loadGame(
            fanren::test::chapterFixturePath(root_.string(), fanren::test::kChapterTenEndingFirst).string());
        ASSERT_TRUE(loaded.ok) << loaded.error;
        app_.state() = loaded.value;
        ASSERT_EQ(state().party.size(), 1u) << "先验：第 10 章终局夹具恰有一名同伴";
        ASSERT_EQ(state().party.front().roleId, kQuHun);
        ASSERT_TRUE(state().party.front().active);

        const fs::path scripts = tmp_.path() / "scripts";
        fs::create_directories(scripts / "common");
        fs::copy_file(root_ / "scripts/common/api.lua", scripts / "common/api.lua",
                      fs::copy_options::overwrite_existing);
        const auto rehosted = app_.scripts().init(scripts.string(), &app_.state());
        ASSERT_TRUE(rehosted.ok) << rehosted.error;
    }
    void TearDown() override { app_.shutdown(); }

    GameState& state() { return app_.state(); }

    // The probes issue no command that waits on the player; one that does is a test bug.
    void runLua(const std::string& body) {
        const std::string name = "probe_" + std::to_string(++scriptCount_) + ".lua";
        writeFile(tmp_.path() / "scripts" / name, body + "\n");
        const auto started = app_.startEvent(name);
        ASSERT_TRUE(started.ok) << started.error;
        for (int frame = 0; frame < 4000 && app_.scripts().isRunning(); ++frame) {
            app_.tick(kFrame);
            if (app_.awaitingCommand()) {
                ADD_FAILURE() << "探针不该停下来等玩家：" << body;
                fanren::script::CommandResult result;
                result.ok = false;
                app_.completeCommand(result);
            }
        }
        ASSERT_FALSE(app_.scripts().isRunning()) << body << " 没能跑到结束";
    }

    // Builds the real scene headlessly; the caller reads scene.battle().units().
    void enter(BattleScene& scene) { scene.onEnter(app_); }

    fs::path root_;
    TempDir tmp_{"fanren_ch12_engine"};
    int scriptCount_ = 0;
    Application app_;
};

// ===========================================================================
// E1 1.3 item 1: party.remove reports true once, the party is then empty, and a second remove
// reports false without crashing.
// ===========================================================================
TEST_F(Ch12Fixture, RemovingQuHunReportsTrueAndEmptiesThePartyAndASecondRemoveReportsFalse) {
    const std::string probe = "local ok = party.remove(\"qu_hun_shadan\")\n"
                              "flag.set(\"probe.ch12.remove_ok\", ok and 1 or 0)\n"
                              "flag.set(\"probe.ch12.party_size\", party.size())";
    // Sentinels: a 0 read back must have come from the script, not from an unset flag.
    state().flags["probe.ch12.remove_ok"] = 99;
    state().flags["probe.ch12.party_size"] = 99;
    runLua(probe);
    EXPECT_EQ(state().flag("probe.ch12.remove_ok"), 1);
    EXPECT_EQ(state().flag("probe.ch12.party_size"), 0);
    EXPECT_TRUE(state().party.empty());

    // Negative: nobody left to remove — reported as false, the party stays empty.
    state().flags["probe.ch12.remove_ok"] = 99;
    state().flags["probe.ch12.party_size"] = 99;
    runLua(probe);
    EXPECT_EQ(state().flag("probe.ch12.remove_ok"), 0) << "不在队里却报 true：n05 的旁白分支永远走不到";
    EXPECT_EQ(state().flag("probe.ch12.party_size"), 0);
    EXPECT_TRUE(state().party.empty());
}

// ===========================================================================
// E1 1.3 item 2: an empty party in a battle with written allies — the hero plus the setup's
// allies, nobody grown out of thin air. The negatives put qu_hun back (active, then inactive).
// ===========================================================================
TEST_F(Ch12Fixture, AnEmptyPartyEntersTheBorrowedBattleWithTheHeroAndItsFourWrittenAllies) {
    if (app_.battleSetup(kBorrowedBattle) == nullptr) FAIL() << "借用的编成 " << kBorrowedBattle << " 不在了";
    state().party.clear();
    BattleScene scene(kBorrowedBattle);
    enter(scene);
    const battle::BattleState& field = scene.battle();
    ASSERT_FALSE(field.units().empty());
    EXPECT_EQ(field.units()[0].id, "hanli");
    EXPECT_TRUE(field.units()[0].ally);
    EXPECT_EQ(allyCount(field), 5);
    EXPECT_EQ(countId(field, "xueyu_zhizhu_ally"), 2);
    EXPECT_EQ(countId(field, "kuilei_yuan"), 2);
    EXPECT_EQ(countId(field, kQuHun), 0);
}

TEST_F(Ch12Fixture, PuttingQuHunBackAddsExactlyHimAndAnInactiveQuHunAddsNobody) {
    if (app_.battleSetup(kBorrowedBattle) == nullptr) FAIL() << "借用的编成 " << kBorrowedBattle << " 不在了";
    {
        BattleScene scene(kBorrowedBattle);
        enter(scene);
        EXPECT_EQ(allyCount(scene.battle()), 6) << "空队伍那一条的「没有」得来自队伍，不是来自编成";
        EXPECT_EQ(countId(scene.battle(), kQuHun), 1);
    }
    state().party.front().active = false;
    {
        BattleScene scene(kBorrowedBattle);
        enter(scene);
        EXPECT_EQ(allyCount(scene.battle()), 5);
        EXPECT_EQ(countId(scene.battle(), kQuHun), 0);
    }
}

// ===========================================================================
// E1 1.3 item 3: empty party save → load → save is byte-identical; the one-member control
// proves the comparison reads a real field.
// ===========================================================================
TEST_F(Ch12Fixture, AnEmptyPartySavesLoadsAndSavesAgainByteForByte) {
    state().party.clear();
    const fs::path a = tmp_.path() / "empty_a.sav";
    const fs::path b = tmp_.path() / "empty_b.sav";
    ASSERT_TRUE(io::saveGame(state(), a.string()).ok);
    const auto loaded = io::loadGame(a.string());
    ASSERT_TRUE(loaded.ok) << loaded.error;
    EXPECT_TRUE(loaded.value.party.empty());
    ASSERT_TRUE(io::saveGame(loaded.value, b.string()).ok);
    EXPECT_EQ(readBytes(a), readBytes(b));
    EXPECT_FALSE(readBytes(a).empty());
}

TEST_F(Ch12Fixture, AOneMemberPartyRoundTripsItsIdAndHitPointsAndDiffersFromTheEmptySave) {
    state().party.front().hp = 345;
    const fs::path one = tmp_.path() / "one_a.sav";
    const fs::path oneAgain = tmp_.path() / "one_b.sav";
    ASSERT_TRUE(io::saveGame(state(), one.string()).ok);
    const auto loaded = io::loadGame(one.string());
    ASSERT_TRUE(loaded.ok) << loaded.error;
    ASSERT_EQ(loaded.value.party.size(), 1u);
    EXPECT_EQ(loaded.value.party.front().roleId, kQuHun);
    EXPECT_EQ(loaded.value.party.front().hp, 345);
    EXPECT_TRUE(loaded.value.party.front().active);
    ASSERT_TRUE(io::saveGame(loaded.value, oneAgain.string()).ok);
    EXPECT_EQ(readBytes(one), readBytes(oneAgain));

    state().party.clear();
    const fs::path empty = tmp_.path() / "empty.sav";
    ASSERT_TRUE(io::saveGame(state(), empty.string()).ok);
    EXPECT_NE(readBytes(one), readBytes(empty)) << "有人与没人存出来一样：比的不是真字段";
}

// ===========================================================================
// E1 1.3 item 4: the menu's party page with an empty party. The contract pins "no crash"; the
// private row list is read through MenuMembersProbe as well. The control (qu_hun active) proves
// the probe reads live rows; the negative is an id findRole cannot resolve.
// ===========================================================================
void openMenuOneFrame(Application& app, MenuScene& menu) {
    menu.onEnter(app);
    static_cast<void>(menu.update(app, kFrame));
    menu.render(app);
}

TEST_F(Ch12Fixture, TheMenuWithAnEmptyPartyShowsOnlyTheHeroAndSurvivesAFrame) {
    state().party.clear();
    MenuScene menu;
    openMenuOneFrame(app_, menu);
    ASSERT_EQ(menuRows(menu).size(), 1u);
    EXPECT_EQ(menuRows(menu).front().roleId, "hanli");
    EXPECT_TRUE(menuRows(menu).front().hero);
}

TEST_F(Ch12Fixture, TheMenuListsAnActiveCompanionAndSkipsAnUnknownRoleWithoutCrashing) {
    {
        MenuScene menu;
        openMenuOneFrame(app_, menu);
        ASSERT_EQ(menuRows(menu).size(), 2u) << "对照：曲魂在队时该多一行";
        EXPECT_EQ(menuRows(menu).back().roleId, kQuHun);
    }
    state().party.clear();
    core::PartyMember ghost;
    ghost.roleId = "no_such_role";
    state().party.push_back(ghost);
    {
        MenuScene menu;
        openMenuOneFrame(app_, menu);
        ASSERT_EQ(menuRows(menu).size(), 1u) << "findRole 查不到的同伴该被跳过";
        EXPECT_EQ(menuRows(menu).front().roleId, "hanli");
    }
}

// ===========================================================================
// E1 1.3 item 5: the four real chapter-12 setups (content track B). Each FAILs (since integration;
// it skipped before) when its file is missing or b12_neidian_duoding is still the old placeholder
// (duoding_xiushi), which counts as "not landed".
// ===========================================================================
struct RealSetup {
    const char* id;
    std::vector<std::string> allies;   // sorted
};

const std::vector<RealSetup>& realSetups() {
    static const std::vector<RealSetup> table{
        {"b12_guiwu_guiwang", {"yuan_yao_heipao", "zi_ling_xutian"}},
        {"b12_heishamo_tiehuoyi", {"yuan_yao_jiedan"}},
        {"b12_neidian_duoding", {"wu_chou", "xuan_gu"}},
        {"b12_hanlitai_xuangu", {}},
    };
    return table;
}

fs::path battlePath(const fs::path& root, const std::string& id) { return root / "data/battles" / (id + ".json"); }

// Empty when landed; otherwise the reason the case FAILs.
std::string notLandedReason(const fs::path& root, const std::string& id) {
    const fs::path path = battlePath(root, id);
    if (!fs::exists(path)) return "等内容路 data/battles/" + id + ".json 落地";
    const auto setup = io::loadBattle(path.string());
    if (!setup.ok) return {};   // landed but broken: let the case run and report the load error
    const bool placeholder = std::any_of(setup.value.units.begin(), setup.value.units.end(),
                                         [](const core::BattleUnitSpec& u) { return u.roleId == "duoding_xiushi"; });
    if (placeholder) return "等内容路 B 就地改写 data/battles/" + id + ".json（现在还是旧占位）";
    return {};
}

void expectRealSetupAllies(Application& app, const fs::path& root, const RealSetup& real) {
    const auto setup = io::loadBattle(battlePath(root, real.id).string());
    ASSERT_TRUE(setup.ok) << setup.error;
    EXPECT_FALSE(setup.value.heroAbsent) << real.id;
    EXPECT_EQ(sortedAllyRoles(setup.value), real.allies) << real.id;

    ASSERT_NE(app.battleSetup(real.id), nullptr) << real.id << " 没进 Application 的编成表";
    app.state().party.clear();
    BattleScene scene(real.id);
    scene.onEnter(app);
    const battle::BattleState& field = scene.battle();
    ASSERT_FALSE(field.units().empty());
    EXPECT_EQ(field.units()[0].id, "hanli") << real.id;
    EXPECT_EQ(allyCount(field), 1 + static_cast<int>(real.allies.size())) << real.id;
    EXPECT_EQ(countId(field, kQuHun), 0) << real.id;
    for (const std::string& ally : real.allies) {
        const battle::Unit* unit = uniqueUnit(field, ally);
        ASSERT_NE(unit, nullptr) << real.id << " / " << ally;
        EXPECT_TRUE(unit->ally) << real.id << " / " << ally;
    }
}

TEST_F(Ch12Fixture, TheGuiwuSetupSeatsTheHeroAndItsTwoWrittenAlliesWithAnEmptyParty) {
    const RealSetup& real = realSetups()[0];
    if (const std::string why = notLandedReason(root_, real.id); !why.empty()) FAIL() << why;
    expectRealSetupAllies(app_, root_, real);
}

TEST_F(Ch12Fixture, TheHeishamoSetupSeatsTheHeroAndYuanYaoWithAnEmptyParty) {
    const RealSetup& real = realSetups()[1];
    if (const std::string why = notLandedReason(root_, real.id); !why.empty()) FAIL() << why;
    expectRealSetupAllies(app_, root_, real);
}

TEST_F(Ch12Fixture, TheNeidianSetupSeatsTheHeroWuChouAndXuanGuWithAnEmptyParty) {
    const RealSetup& real = realSetups()[2];
    if (const std::string why = notLandedReason(root_, real.id); !why.empty()) FAIL() << why;
    expectRealSetupAllies(app_, root_, real);
}

TEST_F(Ch12Fixture, TheHanlitaiSetupSeatsTheHeroAloneWithAnEmptyParty) {
    const RealSetup& real = realSetups()[3];
    if (const std::string why = notLandedReason(root_, real.id); !why.empty()) FAIL() << why;
    expectRealSetupAllies(app_, root_, real);
}

// ===========================================================================
// E2 2.3 item 1: the precedent on both sides. xuan_le is an ally in b08_lingkuang_shouzhen and an
// enemy in b08_zhongrudong; both scenes read one in-memory role that also carries killableBy 木,
// so every difference comes from the faction alone.
// ===========================================================================
class Ch12TwoSided : public Ch12Fixture {
protected:
    void SetUp() override {
        Ch12Fixture::SetUp();
        if (HasFatalFailure()) return;
        ASSERT_NE(app_.battleSetup(kFriendBattle), nullptr) << "E2 借用的编成没了：先改契约 5.3 的借用行";
        ASSERT_NE(app_.battleSetup(kFoeBattle), nullptr) << "E2 借用的编成没了：先改契约 5.3 的借用行";
        role_ = &app_.data().roles.at(kTwoSided);
        ASSERT_EQ(role_->toughness, 6) << "先验：宣乐的架势";
        ASSERT_EQ(role_->charge.every, 3) << "先验：宣乐的蓄势";
        ASSERT_NE(role_->weaknesses, 0);
        ASSERT_FALSE(role_->charge.textKey.empty());
        wood_ = categoryOf("木");
        role_->killableBy = wood_;   // memory only; no file is written
    }

    // Copies, so the scene can go out of scope before the caller reads the result.
    battle::Unit xuanLeIn(const char* battleId) {
        BattleScene scene(battleId);
        enter(scene);
        const battle::Unit* unit = uniqueUnit(scene.battle(), kTwoSided);
        return unit == nullptr ? battle::Unit{} : *unit;
    }

    core::RoleTemplate* role_ = nullptr;
    int wood_ = 0;
};

TEST_F(Ch12TwoSided, AsAnAllyXuanLeHasNoStanceWeaknessChargeOrKillableByButKeepsMagicsAndActions) {
    const battle::Unit unit = xuanLeIn(kFriendBattle);
    ASSERT_EQ(unit.id, kTwoSided);
    EXPECT_TRUE(unit.ally);
    EXPECT_EQ(unit.maxToughness, 0);
    EXPECT_EQ(unit.weaknesses, 0);
    EXPECT_EQ(unit.revealed, 0);
    EXPECT_EQ(unit.chargeEvery, 0);
    EXPECT_TRUE(unit.chargeTextKey.empty()) << unit.chargeTextKey;
    EXPECT_EQ(unit.killableBy, 0) << core::categoryNames(unit.killableBy);
    EXPECT_EQ(unit.magics, role_->magics);
    // xuan_le's actions is 1, the Unit default, so this line idles (gate review G-1); the real ③
    // case below pins the copy on xuan_gu's 2.
    EXPECT_EQ(unit.actions, role_->actions) << "友军照抄 actions（契约 2.1）";
}

TEST_F(Ch12TwoSided, AsAnEnemyTheSameRoleCarriesStanceWeaknessChargeAndKillableBy) {
    const battle::Unit unit = xuanLeIn(kFoeBattle);
    ASSERT_EQ(unit.id, kTwoSided);
    EXPECT_FALSE(unit.ally);
    EXPECT_EQ(unit.maxToughness, role_->toughness);
    EXPECT_EQ(unit.weaknesses, role_->weaknesses);
    EXPECT_EQ(unit.chargeEvery, 3);
    EXPECT_EQ(unit.chargeTextKey, role_->charge.textKey);
    EXPECT_EQ(unit.killableBy, wood_) << core::categoryNames(unit.killableBy);
    EXPECT_EQ(unit.magics, role_->magics);
    EXPECT_EQ(unit.actions, role_->actions);
}

// ===========================================================================
// E2 2.3 item 3: weakness bookkeeping lights only enemies. With xuan_le's record cleared both
// sides start dark; after learning every weakness the enemy starts lit and the ally stays dark.
// ===========================================================================
TEST_F(Ch12TwoSided, KnownWeaknessesLightTheEnemyAndNeverTheAlly) {
    state().knownWeaknesses.erase(kTwoSided);
    ASSERT_EQ(state().knownWeaknessesOf(kTwoSided), 0);
    EXPECT_EQ(xuanLeIn(kFriendBattle).revealed, 0);
    EXPECT_EQ(xuanLeIn(kFoeBattle).revealed, state().knownWeaknessesOf(kTwoSided) & role_->weaknesses);

    state().learnWeaknesses(kTwoSided, role_->weaknesses);
    ASSERT_EQ(state().knownWeaknessesOf(kTwoSided), role_->weaknesses);
    EXPECT_EQ(xuanLeIn(kFriendBattle).revealed, 0) << "记过的破绽亮在了友军头上";
    EXPECT_EQ(xuanLeIn(kFoeBattle).revealed, role_->weaknesses);
}

// ===========================================================================
// E2 2.3 item 2: an ally can be killed. Built straight on BattleState: hero, a 10-hp ally and a
// fast enemy whose sword strike on the ally goes first. Negative: the same ally carrying
// killableBy 木 is held at 1 — what an ally would look like if the !ally guard moved.
// ===========================================================================
battle::Unit probeUnit(const char* id, bool ally, int hp, int attack, int speed) {
    battle::Unit u;
    u.id = u.name = id;
    u.hp = u.maxHp = hp;
    u.mp = u.maxMp = 100;
    u.attack = attack;
    u.defence = 0;
    u.speed = speed;
    u.realm = fanren::rules::Realm::CoreEarly;
    u.ally = ally;
    u.weapons = categoryOf("剑") | categoryOf("拳");
    return u;
}

battle::BattleState allyUnderTheSword(int allyKillableBy) {
    battle::Unit friendly = probeUnit("ally_probe", true, 10, 1, 5);
    friendly.killableBy = allyKillableBy;
    battle::BattleState state;
    state.setup({probeUnit("hanli", true, 500, 10, 10), friendly, probeUnit("foe_probe", false, 1000, 80, 100)}, 7u);
    EXPECT_EQ(state.currentActor(), 2) << "先验：敌方最快、先出手";
    battle::Action strike;
    strike.kind = battle::ActionKind::Attack;
    strike.actorIndex = 2;
    strike.targetIndex = 1;
    strike.category = categoryOf("剑");
    std::string why;
    EXPECT_TRUE(state.isLegal(strike, &why)) << why;
    EXPECT_GE(state.estimateHitDamage(strike, 1), 10) << "先验：这一击本该打穿 10 点气血";
    const auto applied = state.apply(strike);
    EXPECT_TRUE(applied.ok) << applied.error;
    return state;
}

TEST(Ch12AllyKillable, AnAllyWithoutKillableByDiesToTheSwordAndTheBattleGoesOn) {
    const battle::BattleState state = allyUnderTheSword(0);
    const battle::Unit& ally = state.units()[1];
    EXPECT_EQ(ally.hp, 0);
    EXPECT_FALSE(ally.alive());
    EXPECT_EQ(state.phase(), battle::BattlePhase::Ongoing) << "友军倒下只是少一个人";
}

TEST(Ch12AllyKillable, AnAllyCarryingKillableByWoodIsHeldAtOneBySword) {
    const battle::BattleState state = allyUnderTheSword(categoryOf("木"));
    const battle::Unit& ally = state.units()[1];
    EXPECT_EQ(ally.hp, 1);
    EXPECT_TRUE(ally.alive());
    EXPECT_EQ(state.phase(), battle::BattlePhase::Ongoing);
}

// ===========================================================================
// E2 2.3 items 4, 5 — the two helpers on hand-built data, so their negatives run before the
// content track lands anything.
// ===========================================================================
core::BattleSetup setupOf(std::initializer_list<std::pair<const char*, bool>> units) {
    core::BattleSetup setup;
    setup.id = "b12_probe";
    for (const auto& [roleId, ally] : units) {
        core::BattleUnitSpec spec;
        spec.roleId = roleId;
        spec.ally = ally;
        setup.units.push_back(spec);
    }
    return setup;
}

TEST(Ch12KillableSubset, KillableByOutsideWeaknessesIsReportedForEnemiesOnly) {
    core::GameData data;
    core::RoleTemplate boss;
    boss.id = "probe_boss";
    boss.weaknesses = categoryOf("木") | categoryOf("火");
    boss.killableBy = categoryOf("木");
    data.roles[boss.id] = boss;
    const std::vector<core::BattleSetup> asEnemy{setupOf({{"probe_boss", false}})};
    EXPECT_TRUE(killableOutsideWeaknesses(asEnemy, data).empty());

    data.roles[boss.id].killableBy |= categoryOf("金");   // one bit outside the weaknesses
    EXPECT_EQ(killableOutsideWeaknesses(asEnemy, data), std::set<std::string>{"probe_boss"});
    const std::vector<core::BattleSetup> asAlly{setupOf({{"probe_boss", true}})};
    EXPECT_TRUE(killableOutsideWeaknesses(asAlly, data).empty()) << "友军不吃 killable_by，不该报";
}

TEST(Ch12AlliesCastMetal, AMetalSpellOnAnAllyIsReportedAndOnAnEnemyIsNot) {
    const fs::path root = projectRoot();
    ASSERT_FALSE(root.empty());
    const auto loaded = io::loadGameData((root / "data").string());
    ASSERT_TRUE(loaded.ok) << loaded.error;
    core::GameData data = loaded.value;
    ASSERT_NE(data.findRole("wu_chou"), nullptr);
    const core::Magic* jinfu = data.findMagic("magic_ji_jinfu");
    ASSERT_NE(jinfu, nullptr);
    ASSERT_NE(core::categoriesOfElement(jinfu->element) & categoryOf("金"), 0) << "先验：祭金蚨是金";

    const core::BattleSetup withWuChou = setupOf({{"wu_chou", true}});
    EXPECT_TRUE(alliesCastMetal(withWuChou, data).empty()) << "先验：现有乌丑不配金";
    data.roles.at("wu_chou").magics.push_back("magic_ji_jinfu");
    EXPECT_EQ(alliesCastMetal(withWuChou, data), std::vector<std::string>{"magic_ji_jinfu"});
    EXPECT_TRUE(alliesCastMetal(setupOf({{"wu_chou", false}}), data).empty()) << "敌方配金不归这条管";
}

// ===========================================================================
// E2 2.3 items 4, 5 on the real chapter-12 files (content track B). FAILs (since integration; it
// skipped before) unless xuan_gu.json and all four setups have landed (b12_neidian_duoding
// rewritten in place, see notLandedReason).
// ===========================================================================
std::string realDataNotLanded(const fs::path& root) {
    if (!fs::exists(root / "data/roles/xuan_gu.json")) return "等内容路 data/roles/xuan_gu.json 落地";
    for (const RealSetup& real : realSetups()) {
        if (std::string why = notLandedReason(root, real.id); !why.empty()) return why;
    }
    return {};
}

std::vector<core::BattleSetup> loadRealSetups(const fs::path& root) {
    std::vector<core::BattleSetup> setups;
    for (const RealSetup& real : realSetups()) {
        auto setup = io::loadBattle(battlePath(root, real.id).string());
        EXPECT_TRUE(setup.ok) << setup.error;
        if (setup.ok) setups.push_back(std::move(setup.value));
    }
    return setups;
}

TEST(Ch12RealFriendFoe, XuanGuIsKillableByWoodOnlyActsTwiceChargesEveryThreeAndCastsOnlyMoyanHuo) {
    const fs::path root = projectRoot();
    ASSERT_FALSE(root.empty());
    if (const std::string why = realDataNotLanded(root); !why.empty()) FAIL() << why;
    const auto loaded = io::loadGameData((root / "data").string());
    ASSERT_TRUE(loaded.ok) << loaded.error;
    const core::RoleTemplate* xuanGu = loaded.value.findRole("xuan_gu");
    ASSERT_NE(xuanGu, nullptr);
    EXPECT_EQ(xuanGu->killableBy, categoryOf("木")) << core::categoryNames(xuanGu->killableBy);
    EXPECT_EQ(xuanGu->killableBy & ~xuanGu->weaknesses, 0) << "killable_by 必须 ⊆ weaknesses";
    EXPECT_EQ(xuanGu->actions, 2);
    EXPECT_EQ(xuanGu->charge.every, 3);
    EXPECT_EQ(xuanGu->magics, std::vector<std::string>{"magic_moyan_huo"});
}

TEST(Ch12RealFriendFoe, EveryKillableEnemyOfTheFourSetupsStaysInsideItsWeaknesses) {
    const fs::path root = projectRoot();
    ASSERT_FALSE(root.empty());
    if (const std::string why = realDataNotLanded(root); !why.empty()) FAIL() << why;
    const auto loaded = io::loadGameData((root / "data").string());
    ASSERT_TRUE(loaded.ok) << loaded.error;
    core::GameData data = loaded.value;
    const std::vector<core::BattleSetup> setups = loadRealSetups(root);
    ASSERT_EQ(setups.size(), realSetups().size());

    std::set<std::string> killable;
    for (const core::BattleSetup& setup : setups) {
        for (const core::BattleUnitSpec& unit : setup.units) {
            const core::RoleTemplate* role = data.findRole(unit.roleId);
            ASSERT_NE(role, nullptr) << setup.id << " / " << unit.roleId;
            if (!unit.ally && role->killableBy != 0) killable.insert(role->id);
        }
    }
    const std::set<std::string> expected{"langxing_qiling_yin", "sanshou_guiwu", "tiehuo_yiqun",
                                         "tiehuo_yiqun_da", "xuan_gu"};
    EXPECT_EQ(killable, expected);
    EXPECT_TRUE(killableOutsideWeaknesses(setups, data).empty());

    // Negative: one extra bit outside xuan_gu's weaknesses must be caught.
    core::RoleTemplate& xuanGu = data.roles.at("xuan_gu");
    const int outside = core::kAllCategories & ~xuanGu.weaknesses;
    ASSERT_NE(outside, 0);
    xuanGu.killableBy |= outside & -outside;
    EXPECT_EQ(killableOutsideWeaknesses(setups, data), std::set<std::string>{"xuan_gu"});
}

TEST(Ch12RealFriendFoe, TheNeidianAlliesCastNoMetalUntilWuChouIsHandedJinfu) {
    const fs::path root = projectRoot();
    ASSERT_FALSE(root.empty());
    if (const std::string why = realDataNotLanded(root); !why.empty()) FAIL() << why;
    const auto loaded = io::loadGameData((root / "data").string());
    ASSERT_TRUE(loaded.ok) << loaded.error;
    core::GameData data = loaded.value;
    const auto setup = io::loadBattle(battlePath(root, "b12_neidian_duoding").string());
    ASSERT_TRUE(setup.ok) << setup.error;
    EXPECT_TRUE(alliesCastMetal(setup.value, data).empty()) << "③ 的金得是玩家的（I 表）";
    data.roles.at("wu_chou").magics.push_back("magic_ji_jinfu");
    EXPECT_EQ(alliesCastMetal(setup.value, data), std::vector<std::string>{"magic_ji_jinfu"});
}

// E2 2.3 item 4, the real scenes: in ③ xuan_gu and wu_chou fight as allies with none of the four
// enemy-only fields but with their role's actions (contract 2.1: actions sit outside the
// if (!ally) guard; xuan_gu acts twice, which the balance track measured); the silver wolf is
// killable by metal only; in ④ xuan_gu is the sole enemy, killable by wood only and charging
// every three of its rounds. A missing ally is a failure, not a skipped check (test review T-7).
TEST_F(Ch12Fixture, InTheNeidianXuanGuAndWuChouAreAlliesWithoutEnemyOnlyFieldsAndTheSilverWolfNeedsMetal) {
    if (const std::string why = realDataNotLanded(root_); !why.empty()) FAIL() << why;
    BattleScene scene("b12_neidian_duoding");
    enter(scene);
    for (const char* id : {"xuan_gu", "wu_chou"}) {
        const battle::Unit* unit = uniqueUnit(scene.battle(), id);
        ASSERT_NE(unit, nullptr) << id << " 不在 ③ 的真场景里";
        EXPECT_TRUE(unit->ally) << id;
        EXPECT_EQ(unit->killableBy, 0) << id;
        EXPECT_EQ(unit->maxToughness, 0) << id;
        EXPECT_EQ(unit->weaknesses, 0) << id;
        EXPECT_EQ(unit->chargeEvery, 0) << id;
        EXPECT_EQ(unit->actions, app_.data().roles.at(id).actions) << id << "：友军照抄 actions（契约 2.1）";
    }
    // Gate review G-1: xuan_le's actions is 1, the Unit default, so the precedent case cannot tell a
    // copied value from an uncopied one. xuan_gu's 2 can.
    const battle::Unit* xuanGu = uniqueUnit(scene.battle(), "xuan_gu");
    ASSERT_NE(xuanGu, nullptr);
    EXPECT_EQ(xuanGu->actions, 2) << "③ 里玩家一回合能让玄骨动两手";
    const battle::Unit* wolf = uniqueUnit(scene.battle(), "langxing_qiling_yin");
    ASSERT_NE(wolf, nullptr);
    EXPECT_FALSE(wolf->ally);
    EXPECT_EQ(wolf->killableBy, categoryOf("金")) << core::categoryNames(wolf->killableBy);
}

TEST_F(Ch12Fixture, OnTheHanlitaiXuanGuIsTheOnlyEnemyKillableByWoodAndChargingEveryThree) {
    if (const std::string why = realDataNotLanded(root_); !why.empty()) FAIL() << why;
    BattleScene scene("b12_hanlitai_xuangu");
    enter(scene);
    std::vector<const battle::Unit*> enemies;
    for (const battle::Unit& unit : scene.battle().units()) {
        if (!unit.ally) enemies.push_back(&unit);
    }
    ASSERT_EQ(enemies.size(), 1u);
    EXPECT_EQ(enemies.front()->id, "xuan_gu");
    EXPECT_EQ(enemies.front()->killableBy, categoryOf("木")) << core::categoryNames(enemies.front()->killableBy);
    EXPECT_EQ(enemies.front()->chargeEvery, 3);
    EXPECT_GT(enemies.front()->maxToughness, 0);
}

}  // namespace
