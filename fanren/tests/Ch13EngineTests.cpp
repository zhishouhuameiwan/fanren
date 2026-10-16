// Chapter 13 E1–E2: docs/interfaces-p3-ch13.md sections 1.3 and 2.3.
// Chapter 13 has no code item: both E entries are verification cases that pin engine behaviour
// the content track leans on and no earlier chapter exercised — a Core-early panel breakthrough
// (E1) and one spirit field opened from a second map (E2). src/ is untouched. Cases that read the
// content track's chapter-13 files (1.3 item 8, 2.3 items 5–6) read the real maps and scripts; the
// content track has landed (chapter-13 integration), so a missing file is a FAIL(), not a skip.
#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "ChapterFixture.h"
#include "TempDir.h"
#include "core/model/Types.h"
#include "core/rules/Calendar.h"
#include "core/rules/Cultivation.h"
#include "core/rules/Field.h"
#include "core/rules/Realm.h"
#include "game/Application.h"
#include "game/CultivationScene.h"
#include "game/FieldScene.h"
#include "game/Scene.h"
#include "game/Wording.h"
#include "io/DataLoader.h"
#include "io/SaveFile.h"
#include "script/Command.h"

namespace {

namespace fs = std::filesystem;
namespace rules = fanren::rules;
namespace core = fanren::core;
namespace io = fanren::io;
namespace game = fanren::game;
using core::GameState;
using core::MapObject;
using fanren::test::TempDir;
using game::Application;
using rules::BreakthroughBlock;
using rules::Realm;

constexpr const char* kFieldId = "field_tianxing_39";
constexpr const char* kHerbId = "herb_huangjing_cao";
constexpr const char* kRuchengFlag = "ch13.rucheng";
constexpr const char* kLifuFlag = "ch13.lifu";
constexpr const char* kYaoyuanName = "facility_yaoyuan_39";
constexpr int kAptitude = 50;
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

MapObject facilityObject(const std::string& name,
                         std::initializer_list<std::pair<const char*, const char*>> props) {
    MapObject object;
    object.name = name;
    object.type = "facility";
    for (const auto& [key, value] : props) object.properties.emplace(key, value);
    return object;
}

// ---------------------------------------------------------------------------
// Lua scanning (1.3 item 8). Live code only: `--[[ … ]]` / `--[==[ … ]==]` block comments and
// `--` line comments are blanked (newlines kept). Strings are not parsed — the repository's
// scripts carry no `--` inside string literals (the NpcPresenceTests::luaCodeOf convention).
// ---------------------------------------------------------------------------
std::string luaLiveCode(const std::string& source) {
    std::string code;
    std::size_t at = 0;
    while (at < source.size()) {
        const std::size_t dash = source.find("--", at);
        if (dash == std::string::npos) {
            code.append(source, at, std::string::npos);
            break;
        }
        code.append(source, at, dash - at);
        const std::size_t open = dash + 2;
        if (open < source.size() && source[open] == '[') {
            std::size_t probe = open + 1;
            while (probe < source.size() && source[probe] == '=') ++probe;
            if (probe < source.size() && source[probe] == '[') {
                const std::string closer = "]" + std::string(probe - open - 1, '=') + "]";
                const std::size_t end = source.find(closer, probe + 1);
                const std::size_t stop = end == std::string::npos ? source.size() : end + closer.size();
                code.append(static_cast<std::size_t>(std::count(source.begin() + static_cast<std::ptrdiff_t>(dash),
                                                                source.begin() + static_cast<std::ptrdiff_t>(stop), '\n')),
                            '\n');
                at = stop;
                continue;
            }
        }
        const std::size_t eol = source.find('\n', dash);
        at = eol == std::string::npos ? source.size() : eol;
    }
    return code;
}

// How many times `call` (e.g. "realm.advance(") appears in the live code of `source`.
int countCalls(const std::string& source, const std::string& call) {
    const std::string code = luaLiveCode(source);
    int found = 0;
    for (std::size_t at = code.find(call); at != std::string::npos; at = code.find(call, at + call.size())) ++found;
    return found;
}

// The first non-blank line of live code: chuguan.lua's gate must be the script's first statement.
std::string firstStatementLine(const std::string& source) {
    const std::string code = luaLiveCode(source);
    std::size_t at = 0;
    while (at < code.size()) {
        const std::size_t eol = code.find('\n', at);
        std::string line = code.substr(at, eol == std::string::npos ? std::string::npos : eol - at);
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.find_first_not_of(" \t") != std::string::npos) return line;
        if (eol == std::string::npos) break;
        at = eol + 1;
    }
    return {};
}

// ---------------------------------------------------------------------------
// Map helpers (2.3 item 5).
// ---------------------------------------------------------------------------
struct LoadedMap {
    std::string id;   // file stem, e.g. ch11_dongfu
    core::TileMap map;
};

const MapObject* findObject(const core::TileMap& map, const std::string& name) {
    const auto it = std::find_if(map.objects.begin(), map.objects.end(),
                                 [&](const MapObject& o) { return o.name == name; });
    return it == map.objects.end() ? nullptr : &*it;
}

MapObject* findObject(core::TileMap& map, const std::string& name) {
    const auto it = std::find_if(map.objects.begin(), map.objects.end(),
                                 [&](const MapObject& o) { return o.name == name; });
    return it == map.objects.end() ? nullptr : &*it;
}

// Every ref_id of a field facility → the set of slot counts it carries across the given maps
// (a missing slots property is what openFacility would build: kDefaultFieldSlots).
std::map<std::string, std::set<int>> fieldSlotsByRefId(const std::vector<LoadedMap>& maps) {
    std::map<std::string, std::set<int>> out;
    for (const LoadedMap& loaded : maps) {
        for (const MapObject& object : loaded.map.objects) {
            if (object.type != "facility" || object.property("kind") != "field") continue;
            const std::string refId = object.property("ref_id");
            if (refId.empty()) continue;
            const std::string slots = object.property("slots");
            out[refId].insert(slots.empty() ? game::kDefaultFieldSlots : std::atoi(slots.c_str()));
        }
    }
    return out;
}

// Loads the named maps (stems) from maps/; fails the test on any map the engine loader refuses.
std::vector<LoadedMap> loadMaps(const fs::path& root, std::initializer_list<const char*> stems) {
    std::vector<LoadedMap> maps;
    for (const char* stem : stems) {
        const fs::path tmj = root / "maps" / (std::string(stem) + ".tmj");
        const auto loaded = io::loadTileMap(tmj.string());
        EXPECT_TRUE(loaded.ok) << tmj << ": " << loaded.error;
        if (loaded.ok) maps.push_back(LoadedMap{stem, loaded.value});
    }
    return maps;
}

// Every maps/*.tmj — 2.3 item 5 asks for the whole repository, not a hand-picked pair.
std::vector<LoadedMap> loadAllMaps(const fs::path& root) {
    std::vector<LoadedMap> maps;
    for (const auto& entry : fs::directory_iterator(root / "maps")) {
        if (!entry.is_regular_file() || entry.path().extension() != ".tmj") continue;
        const auto loaded = io::loadTileMap(entry.path().string());
        EXPECT_TRUE(loaded.ok) << entry.path() << ": " << loaded.error;
        if (loaded.ok) maps.push_back(LoadedMap{entry.path().stem().string(), loaded.value});
    }
    return maps;
}

LoadedMap* findMap(std::vector<LoadedMap>& maps, const std::string& id) {
    const auto it = std::find_if(maps.begin(), maps.end(), [&](const LoadedMap& m) { return m.id == id; });
    return it == maps.end() ? nullptr : &*it;
}

// ===========================================================================
// Shared fixture (contract 1.3 head): headless Application on tests/fixtures/ch11-end-first.sav
// (realm 31 / cap 31 / former 22 / aptitude 50 / 700 / 470, read 2026-10-13). Probes run through
// startEvent from a temporary scriptRoot holding the real common/api.lua (Ch11Fixture pattern).
// ===========================================================================
class Ch13Fixture : public ::testing::Test {
protected:
    void SetUp() override {
        root_ = projectRoot();
        ASSERT_FALSE(root_.empty());
        const auto ready = app_.init(root_.string(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
        const auto loaded = io::loadGame(
            fanren::test::chapterFixturePath(root_.string(), fanren::test::kChapterElevenEndingFirst).string());
        ASSERT_TRUE(loaded.ok) << loaded.error;
        app_.state() = loaded.value;
        ASSERT_EQ(rules::toValue(state().realm), 31) << "先验：第 11 章终局夹具是结丹初期";
        ASSERT_EQ(rules::toValue(state().realmCap), 31);
        ASSERT_EQ(rules::toValue(state().formerRealm), 22);
        ASSERT_EQ(state().aptitude, kAptitude);
        ASSERT_EQ(state().maxHp, 700);
        ASSERT_EQ(state().maxMp, 470);

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

    // n27's one line, run for real through the script host.
    void liftCapToCoreMid() {
        state().flags["probe.ch13.cap_ok"] = 99;   // sentinel
        runLua("local ok = realm.cap(realm.CORE_MID)\nflag.set(\"probe.ch13.cap_ok\", ok and 1 or 0)");
        ASSERT_EQ(state().flag("probe.ch13.cap_ok"), 1);
        ASSERT_EQ(rules::toValue(state().realmCap), 32);
    }

    // openFacility, one frame, and the name of whatever is on top ("" when nothing is).
    std::string openAndName(const MapObject& facility) {
        app_.openFacility(facility);
        app_.tick(kFrame);
        game::Scene* top = app_.topScene();
        return top == nullptr ? std::string{} : top->name();
    }

    void closeTop() {
        app_.popScene();
        app_.tick(kFrame);
    }

    // A meditation panel from a real facility; null (and a failure) when something else opened.
    game::CultivationScene* openPanel(const MapObject& room) {
        const std::string name = openAndName(room);
        EXPECT_EQ(name, "Cultivation");
        return dynamic_cast<game::CultivationScene*>(app_.topScene());
    }

    fs::path root_;
    TempDir tmp_{"fanren_ch13_engine"};
    int scriptCount_ = 0;
    Application app_;
};

// ===========================================================================
// E1 1.3 item 1: at 31/31 five thousand points are blocked by the cap, the dice are not rolled
// and nothing is charged; lifting the cap alone (no other change) unblocks the same state.
// ===========================================================================
TEST_F(Ch13Fixture, FiveThousandAtThirtyOneIsBlockedByTheStoryCapAndNothingIsCharged) {
    state().cultivation = 5000;
    EXPECT_EQ(rules::breakthroughBlock(state().realm, state().cultivation, state().realmCap),
              BreakthroughBlock::StoryCap);
    for (const std::uint32_t seed : {0u, 1u, 7u, 12345u, 0xFFFFFFFFu}) {
        const rules::BreakthroughAttempt attempt = rules::tryBreakthrough(
            state().realm, state().cultivation, state().realmCap, state().aptitude, 0, seed);
        EXPECT_EQ(attempt.blocked, BreakthroughBlock::StoryCap) << seed;
        EXPECT_FALSE(attempt.success) << seed;
        EXPECT_EQ(attempt.cultivationLost, 0) << seed;
        EXPECT_EQ(attempt.cultivationSpent, 0) << seed;
        EXPECT_FALSE(attempt.backlash) << seed;
    }
}

TEST_F(Ch13Fixture, NegativeTheSameStateWithTheCapAtThirtyTwoIsNotBlocked) {
    state().cultivation = 5000;
    state().realmCap = Realm::CoreMid;
    EXPECT_EQ(rules::breakthroughBlock(state().realm, state().cultivation, state().realmCap),
              BreakthroughBlock::None)
        << "挡住 5000 的是上限，不是修为";
}

// ===========================================================================
// E1 1.3 item 2: realm.cap(CORE_MID) only lifts the cap — realm, vitals and cultivation stay.
// ===========================================================================
TEST_F(Ch13Fixture, StoryCapToCoreMidLiftsOnlyTheCap) {
    state().hp = 555;   // not full: a cap that healed or lifted vitals would show here
    const int mp = state().mp;
    const int cultivation = state().cultivation;
    state().flags["probe.ch13.cap_ok"] = 99;   // sentinel: a 1 below was written by the probe
    runLua("local ok = realm.cap(realm.CORE_MID)\nflag.set(\"probe.ch13.cap_ok\", ok and 1 or 0)");
    EXPECT_EQ(state().flag("probe.ch13.cap_ok"), 1);
    EXPECT_EQ(rules::toValue(state().realmCap), 32);
    EXPECT_EQ(rules::toValue(state().realm), 31) << "realm.cap 不替他冲关";
    EXPECT_EQ(state().maxHp, 700);
    EXPECT_EQ(state().hp, 555);
    EXPECT_EQ(state().maxMp, 470);
    EXPECT_EQ(state().mp, mp);
    EXPECT_EQ(state().cultivation, cultivation);
    EXPECT_EQ(rules::toValue(state().formerRealm), 22);
}

// Negative: an invalid number is refused with "no_realm" and moves nothing; a lower target is a
// no-op that still reports true (only-up). raiseRealmCap made assignable fails the last block.
TEST_F(Ch13Fixture, NegativeAnInvalidCapIsRefusedAndALowerCapNeverLowersIt) {
    const std::string probe99 =
        "local ok, code = realm.cap(99)\n"
        "flag.set(\"probe.ch13.cap_ok\", ok and 1 or 0)\n"
        "flag.set(\"probe.ch13.cap_norealm\", code == \"no_realm\" and 1 or 0)";
    state().flags["probe.ch13.cap_ok"] = 99;
    state().flags["probe.ch13.cap_norealm"] = 99;
    runLua(probe99);
    EXPECT_EQ(state().flag("probe.ch13.cap_ok"), 0) << "编号写错必须报 false，不许静默收下";
    EXPECT_EQ(state().flag("probe.ch13.cap_norealm"), 1);
    EXPECT_EQ(rules::toValue(state().realmCap), 31);

    ASSERT_NO_FATAL_FAILURE(liftCapToCoreMid());
    state().flags["probe.ch13.cap_ok"] = 99;
    runLua(probe99);
    EXPECT_EQ(state().flag("probe.ch13.cap_ok"), 0);
    EXPECT_EQ(rules::toValue(state().realmCap), 32);

    state().flags["probe.ch13.cap_ok"] = 99;
    runLua("local ok = realm.cap(realm.CORE_EARLY)\nflag.set(\"probe.ch13.cap_ok\", ok and 1 or 0)");
    EXPECT_EQ(state().flag("probe.ch13.cap_ok"), 1) << "已经不低于：照样返回 true";
    EXPECT_EQ(rules::toValue(state().realmCap), 32) << "只升不降";
    EXPECT_EQ(rules::toValue(state().realm), 31);
}

// ===========================================================================
// E1 1.3 item 3: thresholds and chances at Core early, literals written out (not derived).
// ===========================================================================
TEST(Ch13CoreMidRule, TheThresholdIsFiveThousandAndTheChanceIsEightyFiveEightyNineAndNinetyFive) {
    constexpr Realm kHere = Realm::CoreEarly;
    constexpr Realm kCap = Realm::CoreMid;
    ASSERT_EQ(rules::cultivationNeeded(kHere), 5000);

    EXPECT_EQ(rules::breakthroughBlock(kHere, 4999, kCap), BreakthroughBlock::NotEnough);
    EXPECT_EQ(rules::breakthroughChance(kHere, 4999, kAptitude, 0), 0);

    EXPECT_EQ(rules::breakthroughBlock(kHere, 5000, kCap), BreakthroughBlock::None);
    EXPECT_EQ(rules::breakthroughChance(kHere, 5000, kAptitude, 0), 85) << "60 ＋ 25 ＋ 0 ＋ 0";
    EXPECT_EQ(rules::breakthroughChance(kHere, 6000, kAptitude, 0), 89) << "超额 1000 × 20 / 5000 ＝ 4";
    EXPECT_EQ(rules::breakthroughChance(kHere, 10000, kAptitude, 0), 95) << "60 ＋ 25 ＋ 20 ＝ 105，夹到 95";
}

// Negative: one point short is not an attempt — nothing is lost on any seed.
TEST(Ch13CoreMidRule, NegativeOnePointShortIsNotAnAttemptAndLosesNothing) {
    for (std::uint32_t seed = 0; seed < 200; ++seed) {
        const rules::BreakthroughAttempt attempt =
            rules::tryBreakthrough(Realm::CoreEarly, 4999, Realm::CoreMid, kAptitude, 0, seed);
        ASSERT_EQ(attempt.blocked, BreakthroughBlock::NotEnough) << seed;
        ASSERT_FALSE(attempt.success) << seed;
        ASSERT_EQ(attempt.cultivationLost, 0) << seed;
        ASSERT_FALSE(attempt.backlash) << seed;
    }
}

// ===========================================================================
// E1 1.3 item 4: the real panel breaks through. Before every press cultivation is reset to 5000
// and hp cut to 300 (a backlash halves hp); a failure moves the day on (the seed mixes day).
// ===========================================================================
TEST_F(Ch13Fixture, ThePanelBreaksThroughToCoreMidLiftingVitalsByTheDeltaAndThenReportsTheCap) {
    ASSERT_NO_FATAL_FAILURE(liftCapToCoreMid());
    game::CultivationScene* panel = openPanel(facilityObject("facility_jingshi_wz", {{"kind", "meditate"}}));
    ASSERT_NE(panel, nullptr);

    bool succeeded = false;
    for (int press = 1; press <= 50 && !succeeded; ++press) {
        state().cultivation = 5000;
        state().hp = 300;
        const rules::BreakthroughAttempt attempt = panel->breakthrough(app_);
        ASSERT_EQ(attempt.blocked, BreakthroughBlock::None) << "第 " << press << " 次：" << panel->feedback();
        if (attempt.success) {
            succeeded = true;
            EXPECT_EQ(rules::toValue(state().realm), 32);
            EXPECT_EQ(state().cultivation, 0) << "扣恰 5000";
            EXPECT_EQ(state().maxHp, 900);
            EXPECT_EQ(state().maxMp, 620);
            EXPECT_EQ(state().hp, 500) << "按差额上抬 200，不补满";
            break;
        }
        // A failure keeps the realm and charges 1000 (2000 and half the hp on a backlash).
        EXPECT_EQ(rules::toValue(state().realm), 31) << press;
        EXPECT_EQ(state().cultivation, attempt.backlash ? 3000 : 4000) << press;
        EXPECT_EQ(state().hp, attempt.backlash ? 150 : 300) << press;
        state().day += 1;
    }
    ASSERT_TRUE(succeeded) << "五十次全败：成功率不是 85，或种子不随 day 变";

    const rules::BreakthroughAttempt again = panel->breakthrough(app_);
    EXPECT_EQ(again.blocked, BreakthroughBlock::StoryCap) << "修为此刻是 0：报的必须是上限，不是 NotEnough";
    EXPECT_FALSE(again.success);
    EXPECT_EQ(rules::toValue(state().realm), 32);
    EXPECT_EQ(state().cultivation, 0);
}

// Negative: without n27's realm.cap the panel is StoryCap forever and charges nothing.
TEST_F(Ch13Fixture, NegativeWithoutTheCapThePanelReportsStoryCapAndChargesNothing) {
    game::CultivationScene* panel = openPanel(facilityObject("facility_jingshi_wz", {{"kind", "meditate"}}));
    ASSERT_NE(panel, nullptr);
    state().cultivation = 5000;
    state().hp = 300;
    const rules::BreakthroughAttempt attempt = panel->breakthrough(app_);
    EXPECT_EQ(attempt.blocked, BreakthroughBlock::StoryCap);
    EXPECT_FALSE(attempt.success);
    EXPECT_EQ(rules::toValue(state().realm), 31);
    EXPECT_EQ(state().cultivation, 5000) << "分毫不扣";
    EXPECT_EQ(state().hp, 300);
}

// ===========================================================================
// E1 1.3 item 5: the panel row, as a pair — off at cap 31, on at cap 32, same 5000 points.
// ===========================================================================
TEST_F(Ch13Fixture, ThePushRowIsOffAtCapThirtyOneAndOnAtCapThirtyTwo) {
    state().cultivation = 5000;
    auto rows = game::CultivationScene::buildMainItems(state());
    ASSERT_GE(rows.size(), 2u);
    EXPECT_EQ(rows[1].label, game::cultivationLexicon(game::PanelStage::Immortal).pushLabel);
    EXPECT_FALSE(rows[1].enabled);
    EXPECT_FALSE(rows[1].disabledReason.empty());

    state().realmCap = Realm::CoreMid;
    rows = game::CultivationScene::buildMainItems(state());
    ASSERT_GE(rows.size(), 2u);
    EXPECT_TRUE(rows[1].enabled) << rows[1].disabledReason;
    EXPECT_TRUE(rows[1].disabledReason.empty()) << rows[1].disabledReason;
}

// ===========================================================================
// E1 1.3 item 6: the 150 room pays exactly one and a half cushions — with the remainder ledger
// cleared first. Core early is 6000 / 9000 ten-thousandths a day and an insight is 30 days of
// work, so 360 days divide exactly on both sides and 2y == 3x holds for every seed.
// ===========================================================================
TEST_F(Ch13Fixture, ARoomAtOneHundredFiftyPaysExactlyThreeHalvesOfACushionWhenTheLedgerIsClear) {
    ASSERT_EQ(rules::kDaysPerYear, 360);
    GameState base = state();
    base.cultivation = 0;
    base.cultivationRemainder = 0;
    for (const std::uint32_t seed : {1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 0xC0FFEEu, 0xFFFFFFFFu}) {
        GameState room = base;
        GameState cushion = base;
        const int y = game::CultivationScene::bankMeditation(room, rules::kDaysPerYear, seed, 150).cultivation;
        const int x = game::CultivationScene::bankMeditation(cushion, rules::kDaysPerYear, seed, 100).cultivation;
        EXPECT_GT(x, 0) << seed;
        EXPECT_EQ(2 * y, 3 * x) << "seed " << seed << ": " << y << " vs " << x;
        EXPECT_EQ(y, rules::meditate(Realm::CoreEarly, kAptitude, 150, 360, seed).cultivation) << seed;
        EXPECT_EQ(x, rules::meditate(Realm::CoreEarly, kAptitude, 100, 360, seed).cultivation) << seed;
        EXPECT_EQ(room.cultivation, y) << seed;
        EXPECT_EQ(room.cultivationRemainder, 360) << seed;
    }
}

// Control: with the fixture's ledger left at 1350 the takings are a difference on the same curve
// (days 1350..1710), not meditate(…, 360, …) — the premise of "逐点相等" is a cleared ledger.
TEST_F(Ch13Fixture, ControlAnUnclearedLedgerPaysTheDifferenceOnTheSameCurve) {
    ASSERT_EQ(state().cultivationRemainder, 1350) << "先验：夹具的余数账";
    bool differsFromAFreshYear = false;
    for (std::uint32_t seed = 1; seed <= 32; ++seed) {
        GameState carried = state();
        carried.cultivation = 0;
        const int y = game::CultivationScene::bankMeditation(carried, 360, seed, 150).cultivation;
        const int expected = rules::meditate(Realm::CoreEarly, kAptitude, 150, 1710, seed).cultivation -
                             rules::meditate(Realm::CoreEarly, kAptitude, 150, 1350, seed).cultivation;
        EXPECT_EQ(y, expected) << seed;
        EXPECT_EQ(carried.cultivationRemainder, 1710) << seed;
        if (y != rules::meditate(Realm::CoreEarly, kAptitude, 150, 360, seed).cultivation) differsFromAFreshYear = true;
    }
    EXPECT_TRUE(differsFromAFreshYear) << "三十二颗种子上差分都恰等于 meditate(360)：这条对照什么也没区分";
}

// The real facility: gated by ch13.lifu (locked message, then the panel), the status line says
// +50, and a year sat through the panel itself (its own meditationSeed) is 3/2 of a cushion's.
// Negative: the same room without effectiveness is a plain cushion (no bonus line).
TEST_F(Ch13Fixture, TheWuzhongRoomIsGatedByLifuAndItsPanelPaysOneHundredFifty) {
    const MapObject room = facilityObject(
        "facility_jingshi_wz", {{"kind", "meditate"}, {"effectiveness", "150"}, {"require_flag", kLifuFlag}});
    const MapObject cushion = facilityObject("facility_jingshi_wz", {{"kind", "meditate"}});
    ASSERT_EQ(state().flag(kLifuFlag), 0) << "先验：夹具里 ch13.lifu 未置";
    EXPECT_EQ(openAndName(room), "Dialogue") << "未置时只播 ui.facility.locked";
    closeTop();

    state().setFlag(kLifuFlag);
    state().cultivation = 0;
    state().cultivationRemainder = 0;
    const GameState saved = state();

    game::CultivationScene* panel = openPanel(room);
    ASSERT_NE(panel, nullptr);
    EXPECT_NE(panel->siteBonusLine(state()).find("50"), std::string::npos) << panel->siteBonusLine(state());
    const int y = panel->meditateFor(app_, rules::kDaysPerYear).cultivation;
    EXPECT_EQ(state().cultivation, y);
    EXPECT_EQ(state().day, saved.day + 360);
    closeTop();

    state() = saved;
    game::CultivationScene* plain = openPanel(cushion);
    ASSERT_NE(plain, nullptr);
    EXPECT_TRUE(plain->siteBonusLine(state()).empty()) << "没写 effectiveness 就是普通蒲团：" << plain->siteBonusLine(state());
    const int x = plain->meditateFor(app_, rules::kDaysPerYear).cultivation;
    EXPECT_GT(x, 0);
    EXPECT_EQ(2 * y, 3 * x) << y << " vs " << x;
    closeTop();
}

// ===========================================================================
// E1 1.3 item 7: n28's gate — realm.at_least(CORE_MID) is false at 31 and true at 32.
// ===========================================================================
TEST_F(Ch13Fixture, TheLeavingGateReadsFalseAtThirtyOneAndTrueAtThirtyTwo) {
    const std::string probe =
        "flag.set(\"probe.ch13.at_least\", realm.at_least(realm.CORE_MID) and 1 or 0)";
    state().flags["probe.ch13.at_least"] = 99;
    runLua(probe);
    EXPECT_EQ(state().flag("probe.ch13.at_least"), 0);

    state().realm = Realm::CoreMid;
    state().flags["probe.ch13.at_least"] = 99;
    runLua(probe);
    EXPECT_EQ(state().flag("probe.ch13.at_least"), 1);
}

// ===========================================================================
// E1 1.3 item 8: the real data the content track landed (a missing file fails).
// ===========================================================================
TEST(Ch13CoreMidData, TheWuzhongRoomIsAMeditateFacilityAtOneHundredFiftyGatedByLifu) {
    const fs::path root = projectRoot();
    ASSERT_FALSE(root.empty());
    const fs::path tmj = root / "maps/ch13_wuzhong.tmj";
    if (!fs::exists(tmj)) FAIL() << "缺 maps/ch13_wuzhong.tmj（内容路 A1 已交付，第 13 章已集成）";
    const auto map = io::loadTileMap(tmj.string());
    ASSERT_TRUE(map.ok) << map.error;
    const MapObject* room = findObject(map.value, "facility_jingshi_wz");
    ASSERT_NE(room, nullptr);
    EXPECT_EQ(room->type, "facility");
    EXPECT_EQ(room->property("kind"), "meditate");
    EXPECT_EQ(room->property("effectiveness"), "150");
    EXPECT_EQ(room->property("require_flag"), kLifuFlag);
}

TEST(Ch13CoreMidData, TheTianxingRoomIsAMeditateFacilityAtOneHundredTwentyFiveGatedByRucheng) {
    const fs::path root = projectRoot();
    ASSERT_FALSE(root.empty());
    const fs::path tmj = root / "maps/ch13_tianxing.tmj";
    if (!fs::exists(tmj)) FAIL() << "缺 maps/ch13_tianxing.tmj（内容路 A1 已交付，第 13 章已集成）";
    const auto map = io::loadTileMap(tmj.string());
    ASSERT_TRUE(map.ok) << map.error;
    const MapObject* room = findObject(map.value, "facility_jingshi_39");
    ASSERT_NE(room, nullptr);
    EXPECT_EQ(room->type, "facility");
    EXPECT_EQ(room->property("kind"), "meditate");
    EXPECT_EQ(room->property("effectiveness"), "125");
    EXPECT_EQ(room->property("require_flag"), kRuchengFlag);
}

// scripts/ch13/**: realm.cap exactly once (biguan.lua, CORE_MID), realm.advance / realm.demote
// never, and chuguan.lua opens with the at_least gate.
TEST(Ch13CoreMidData, TheChapterScriptsCapOnceInBiguanNeverAdvanceAndChuguanOpensWithTheGate) {
    const fs::path root = projectRoot();
    ASSERT_FALSE(root.empty());
    const fs::path dir = root / "scripts/ch13";
    for (const char* name : {"biguan.lua", "chuguan.lua"}) {
        if (!fs::exists(dir / name)) FAIL() << "缺 scripts/ch13/" << name << "（内容路 C 已交付，第 13 章已集成）";
    }
    int caps = 0;
    int advances = 0;
    int demotes = 0;
    std::vector<std::string> capFiles;
    for (const auto& entry : fs::recursive_directory_iterator(dir)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".lua") continue;
        const std::string text = readBytes(entry.path());
        const int here = countCalls(text, "realm.cap(");
        if (here > 0) capFiles.push_back(entry.path().filename().string());
        caps += here;
        advances += countCalls(text, "realm.advance(");
        demotes += countCalls(text, "realm.demote(");
    }
    EXPECT_EQ(caps, 1) << "realm.cap 全章恰 1 处";
    EXPECT_EQ(capFiles, std::vector<std::string>{"biguan.lua"});
    EXPECT_EQ(countCalls(readBytes(dir / "biguan.lua"), "realm.cap(realm.CORE_MID)"), 1);
    EXPECT_EQ(advances, 0) << "本章境界只经上限，0 处 realm.advance";
    EXPECT_EQ(demotes, 0);
    const std::string first = firstStatementLine(readBytes(dir / "chuguan.lua"));
    EXPECT_NE(first.find("realm.at_least(realm.CORE_MID)"), std::string::npos)
        << "chuguan.lua 第一条可执行语句：" << first;
}

// Negative for the scanner itself (runs now): a live call is counted, a commented one is not,
// and the first statement skips comments and blank lines.
TEST(Ch13CoreMidData, NegativeTheScannerCountsALiveAdvanceAndIgnoresCommentedOnes) {
    EXPECT_EQ(countCalls("realm.advance(realm.CORE_MID)", "realm.advance("), 1);
    EXPECT_EQ(countCalls("local a = 1\nrealm.advance(realm.CORE_MID)\n", "realm.advance("), 1);
    EXPECT_EQ(countCalls("-- realm.advance(realm.CORE_MID)\n", "realm.advance("), 0);
    EXPECT_EQ(countCalls("x = 1 -- realm.advance(realm.CORE_MID)\n", "realm.advance("), 0);
    EXPECT_EQ(countCalls("--[[ realm.advance(realm.CORE_MID)\nrealm.demote(21) ]]\n", "realm.advance("), 0);
    EXPECT_EQ(countCalls("--[==[ x ]] realm.cap(1) ]==]\nrealm.cap(realm.CORE_MID)\n", "realm.cap("), 1);
    EXPECT_EQ(firstStatementLine("-- 头注释\n\n--[[ 块\n注释 ]]\n  if not realm.at_least(realm.CORE_MID) then\n"),
              "  if not realm.at_least(realm.CORE_MID) then");
    EXPECT_EQ(firstStatementLine("local n = 1\nif not realm.at_least(realm.CORE_MID) then return end\n"),
              "local n = 1");
}

// ===========================================================================
// E2 2.3: one field, two maps. Each case first plants the in-memory crops of the 2.3 head:
// slot 0 a ripe 60-year herb, slot 1 the same herb at age 5 marked unripe (memory only).
// ===========================================================================
class Ch13FieldFixture : public Ch13Fixture {
protected:
    void SetUp() override {
        Ch13Fixture::SetUp();
        if (HasFatalFailure()) return;
        ASSERT_EQ(state().fields.size(), 6u) << "先验：第 11 章终局恰 6 块田";
        rules::SpiritField* field = state().findField(kFieldId);
        ASSERT_NE(field, nullptr);
        ASSERT_EQ(field->slots.size(), 6u);
        for (const rules::FieldSlot& slot : field->slots) ASSERT_TRUE(slot.seedId.empty()) << "先验：真档 6 槽全空";
        field->slots[0].seedId = kHerbId;
        field->slots[0].plantedDay = state().day;
        field->slots[0].age = 60;
        field->slots[0].ripe = true;
        field->slots[1].seedId = kHerbId;
        field->slots[1].plantedDay = state().day;
        field->slots[1].age = 5;
        field->slots[1].ripe = false;
    }

    rules::SpiritField* tianxing() { return state().findField(kFieldId); }

    static MapObject yaoyuan(const char* refId, const char* slots) {
        return facilityObject(kYaoyuanName,
                              {{"kind", "field"}, {"ref_id", refId}, {"slots", slots}, {"require_flag", kRuchengFlag}});
    }
};

// E2 2.3 item 1: the field already in the save opens as it is — no new field, the crop untouched.
TEST_F(Ch13FieldFixture, TheFieldInTheSaveOpensAsItIsWithoutANewFieldAndWithTheCropUntouched) {
    state().setFlag(kRuchengFlag);
    const rules::FieldSlot before0 = tianxing()->slots[0];
    const rules::FieldSlot before1 = tianxing()->slots[1];
    EXPECT_EQ(openAndName(yaoyuan(kFieldId, "6")), "Field");
    EXPECT_EQ(state().fields.size(), 6u) << "已有的田不新建";
    const rules::SpiritField* field = tianxing();
    ASSERT_NE(field, nullptr);
    ASSERT_EQ(field->slots.size(), 6u);
    EXPECT_EQ(field->slots[0].seedId, before0.seedId);
    EXPECT_EQ(field->slots[0].age, before0.age);
    EXPECT_EQ(field->slots[0].ripe, before0.ripe);
    EXPECT_EQ(field->slots[1].seedId, before1.seedId);
    EXPECT_EQ(field->slots[1].age, before1.age);
    EXPECT_EQ(field->slots[1].ripe, before1.ripe);
    closeTop();
}

// Negative: an id the save does not have IS built — "no new field" above comes from "already there".
TEST_F(Ch13FieldFixture, NegativeAnIdTheSaveLacksIsBuiltEmptyAtTheWrittenSize) {
    state().setFlag(kRuchengFlag);
    EXPECT_EQ(openAndName(yaoyuan("field_ch13_probe", "6")), "Field");
    EXPECT_EQ(state().fields.size(), 7u);
    const rules::SpiritField* probe = state().findField("field_ch13_probe");
    ASSERT_NE(probe, nullptr);
    ASSERT_EQ(probe->slots.size(), 6u);
    for (const rules::FieldSlot& slot : probe->slots) EXPECT_TRUE(slot.seedId.empty());
    closeTop();
}

// E2 2.3 item 2: a different slots on the second map does not resize the existing field.
TEST_F(Ch13FieldFixture, ADifferentSlotsOnTheSecondMapDoesNotResizeTheExistingField) {
    state().setFlag(kRuchengFlag);
    EXPECT_EQ(openAndName(yaoyuan(kFieldId, "4")), "Field");
    const rules::SpiritField* field = tianxing();
    ASSERT_NE(field, nullptr);
    EXPECT_EQ(field->slots.size(), 6u) << "已有的田不按这一处的 slots 改尺寸";
    EXPECT_EQ(field->slots[0].seedId, kHerbId);
    EXPECT_EQ(field->slots[0].age, 60);
    EXPECT_EQ(state().fields.size(), 6u);
    closeTop();
}

TEST_F(Ch13FieldFixture, NegativeANewIdWithSlotsFourIsBuiltWithExactlyFour) {
    state().setFlag(kRuchengFlag);
    EXPECT_EQ(openAndName(yaoyuan("field_ch13_probe2", "4")), "Field");
    const rules::SpiritField* probe = state().findField("field_ch13_probe2");
    ASSERT_NE(probe, nullptr);
    EXPECT_EQ(probe->slots.size(), 4u) << "slots 这个属性本身是起作用的";
    closeTop();
}

// E2 2.3 item 3: the gate, as a pair — rucheng 0 is the locked message (no field, none built),
// rucheng 1 is the field panel.
TEST_F(Ch13FieldFixture, RuchengUnsetShowsTheLockedMessageAndSetOpensTheField) {
    ASSERT_EQ(state().flag(kRuchengFlag), 0) << "先验：夹具里 ch13.rucheng 未置";
    EXPECT_EQ(openAndName(yaoyuan(kFieldId, "6")), "Dialogue") << "facility 拦下只播 ui.facility.locked";
    EXPECT_EQ(state().fields.size(), 6u);
    closeTop();
    EXPECT_EQ(openAndName(yaoyuan("field_ch13_probe3", "6")), "Dialogue");
    EXPECT_EQ(state().findField("field_ch13_probe3"), nullptr) << "门闸未开时也不建田";
    EXPECT_EQ(state().fields.size(), 6u);
    closeTop();

    state().setFlag(kRuchengFlag);
    EXPECT_EQ(openAndName(yaoyuan(kFieldId, "6")), "Field");
    EXPECT_EQ(state().fields.size(), 6u);
    closeTop();
}

// E2 2.3 item 4: harvesting carries the age into the bag and empties the slot.
TEST_F(Ch13FieldFixture, HarvestingTheRipeHerbPutsOneSixtyYearStalkInTheBagAndEmptiesTheSlot) {
    const int total = state().itemCount(kHerbId);
    const int aged = state().itemCountOfAge(kHerbId, 60);
    game::FieldScene scene(kFieldId);
    ASSERT_TRUE(scene.harvestAt(app_, 0)) << scene.feedback();
    EXPECT_EQ(state().itemCount(kHerbId), total + 1);
    EXPECT_EQ(state().itemCountOfAge(kHerbId, 60), aged + 1) << "年份随包，不丢";
    EXPECT_TRUE(tianxing()->slots[0].seedId.empty());
    EXPECT_FALSE(tianxing()->slots[0].ripe);
}

// Negative: an unripe slot and an empty slot both refuse, and nothing moves.
TEST_F(Ch13FieldFixture, NegativeAnUnripeSlotAndAnEmptySlotRefuseAndNothingMoves) {
    const int total = state().itemCount(kHerbId);
    game::FieldScene scene(kFieldId);
    EXPECT_FALSE(scene.harvestAt(app_, 1)) << "未熟";
    EXPECT_EQ(state().itemCount(kHerbId), total);
    EXPECT_EQ(tianxing()->slots[1].seedId, kHerbId);
    EXPECT_EQ(tianxing()->slots[1].age, 5);
    EXPECT_FALSE(tianxing()->slots[1].ripe);
    EXPECT_FALSE(scene.harvestAt(app_, 2)) << "空槽";
    EXPECT_EQ(state().itemCount(kHerbId), total);
    EXPECT_TRUE(tianxing()->slots[2].seedId.empty());
}

// ===========================================================================
// E2 2.3 item 5: the real maps. The ch13_tianxing half reads the landed map (a missing file fails);
// the helper and its negative run now on the precedent (shenshougu_yaopu: ch01 6, ch02 8) and
// on an in-memory copy of ch11_dongfu standing in for a second map at 8.
// ===========================================================================
TEST(Ch13SharedFieldData, TheHelperSeesThePrecedentAtSixAndEightAndAnEightCopyOfTheField) {
    const fs::path root = projectRoot();
    ASSERT_FALSE(root.empty());
    std::vector<LoadedMap> maps = loadMaps(root, {"ch01_shenshougu", "ch02_yaopu", "ch11_dongfu"});
    ASSERT_EQ(maps.size(), 3u);
    const auto slots = fieldSlotsByRefId(maps);
    ASSERT_EQ(slots.count("shenshougu_yaopu"), 1u);
    EXPECT_EQ(slots.at("shenshougu_yaopu"), (std::set<int>{6, 8})) << "先例本来如此：本条不立全仓同尺寸的规矩";
    ASSERT_EQ(slots.count(kFieldId), 1u);
    EXPECT_EQ(slots.at(kFieldId), (std::set<int>{6}));

    // Negative: a second map carrying the same facility at 8 must show up as {6, 8}.
    LoadedMap copy = *findMap(maps, "ch11_dongfu");
    copy.id = "ch13_tianxing_probe";
    MapObject* field = findObject(copy.map, kYaoyuanName);
    ASSERT_NE(field, nullptr);
    field->properties["slots"] = "8";
    maps.push_back(copy);
    EXPECT_EQ(fieldSlotsByRefId(maps).at(kFieldId), (std::set<int>{6, 8}));
}

TEST(Ch13SharedFieldData, TheTianxingFacilityCarriesExactlyTheFourPropertiesAndTheFieldIsSixEverywhere) {
    const fs::path root = projectRoot();
    ASSERT_FALSE(root.empty());
    if (!fs::exists(root / "maps/ch13_tianxing.tmj")) {
        FAIL() << "缺 maps/ch13_tianxing.tmj（内容路 A1 已交付，第 13 章已集成）";
    }
    std::vector<LoadedMap> maps = loadAllMaps(root);
    LoadedMap* tianxingMap = findMap(maps, "ch13_tianxing");
    LoadedMap* dongfuMap = findMap(maps, "ch11_dongfu");
    ASSERT_NE(tianxingMap, nullptr);
    ASSERT_NE(dongfuMap, nullptr);

    const MapObject* here = findObject(tianxingMap->map, kYaoyuanName);
    ASSERT_NE(here, nullptr);
    EXPECT_EQ(here->type, "facility");
    std::set<std::string> keys;
    for (const auto& [key, value] : here->properties) keys.insert(key);
    EXPECT_EQ(keys, (std::set<std::string>{"kind", "ref_id", "slots", "require_flag"}))
        << "name 之外恰四个属性；facility 不读 deny_text_key，写了就是一条播不出的文案";
    EXPECT_EQ(here->property("kind"), "field");
    EXPECT_EQ(here->property("ref_id"), kFieldId);
    EXPECT_EQ(here->property("slots"), "6");
    EXPECT_EQ(here->property("require_flag"), kRuchengFlag);

    const MapObject* there = findObject(dongfuMap->map, kYaoyuanName);
    ASSERT_NE(there, nullptr);
    EXPECT_EQ(there->property("ref_id"), kFieldId);
    EXPECT_EQ(there->property("slots"), "6");

    EXPECT_EQ(fieldSlotsByRefId(maps).at(kFieldId), (std::set<int>{6}));
    // Negative: the same helper on a copy with ch13_tianxing's facility at 8.
    MapObject* edited = findObject(tianxingMap->map, kYaoyuanName);
    ASSERT_NE(edited, nullptr);
    edited->properties["slots"] = "8";
    EXPECT_EQ(fieldSlotsByRefId(maps).at(kFieldId), (std::set<int>{6, 8}));
}

// ===========================================================================
// E2 2.3 item 6 (read-only gate): validate.py never reads ref_id (contract section 0), so the
// same field on two maps cannot trip it. The premise is pinned here; with ch13_tianxing landed, the
// only gate rules that read the two objects — rule 11 (object name unique within its map) and
// rule 8 (require_flag registered in data/flags.json) — are checked on the real files. The gate
// run itself (python tools/validate.py, 0 lines about field_tianxing_39) is in the record (2.4).
// ===========================================================================
int countWord(const std::string& text, const std::string& word) {
    int found = 0;
    for (std::size_t at = text.find(word); at != std::string::npos; at = text.find(word, at + word.size())) ++found;
    return found;
}

TEST(Ch13SharedFieldData, TheGateNeverReadsRefIdAndBothFieldFacilitiesPassTheRulesThatReadThem) {
    const fs::path root = projectRoot();
    ASSERT_FALSE(root.empty());
    const std::string gate = readBytes(root / "tools/validate.py");
    ASSERT_FALSE(gate.empty());
    EXPECT_EQ(countWord(gate, "ref_id"), 0) << "validate.py 开始读 ref_id 了：跨图同一块田的结论要重核（契约 2.1）";
    // Negative for the counter: one mention is one.
    EXPECT_EQ(countWord("props.get(\"ref_id\")", "ref_id"), 1);

    if (!fs::exists(root / "maps/ch13_tianxing.tmj")) {
        FAIL() << "缺 maps/ch13_tianxing.tmj（内容路 A1 已交付，第 13 章已集成；"
                  "python tools/validate.py 与 field_tianxing_39 相关的报错行数应为 0）";
    }
    const std::string flags = readBytes(root / "data/flags.json");
    std::vector<LoadedMap> maps = loadMaps(root, {"ch11_dongfu", "ch13_tianxing"});
    ASSERT_EQ(maps.size(), 2u);
    for (const LoadedMap& loaded : maps) {
        const auto named = std::count_if(loaded.map.objects.begin(), loaded.map.objects.end(),
                                         [](const MapObject& o) { return o.name == kYaoyuanName; });
        EXPECT_EQ(named, 1) << loaded.id << "：对象名只要求图内唯一";
        const MapObject* field = findObject(loaded.map, kYaoyuanName);
        ASSERT_NE(field, nullptr) << loaded.id;
        const std::string need = field->property("require_flag");
        EXPECT_FALSE(need.empty()) << loaded.id;
        EXPECT_NE(flags.find("\"" + need + "\""), std::string::npos) << loaded.id << " 的 require_flag 未登记：" << need;
    }
}

}  // namespace
