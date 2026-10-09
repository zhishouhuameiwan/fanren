#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

// Inspect original save metadata with the existing vendored parser, without changing test targets.
#include "../vendor/json.hpp"

#include "TempDir.h"
#include "core/model/Types.h"
#include "core/rules/Cultivation.h"
#include "game/Application.h"
#include "game/BattleScene.h"
#include "game/MenuScene.h"
#include "game/Wording.h"
#include "io/SaveFile.h"

namespace {

namespace fs = std::filesystem;
using fanren::core::GameState;
using fanren::game::Application;
using fanren::game::MenuScene;
using fanren::rules::Realm;

std::string readFile(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    EXPECT_TRUE(in.is_open()) << path;
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

std::string savedBytes(const GameState& state) {
    fanren::test::TempDir dir("fanren_ch09_snapshot");
    const fs::path path = dir.path() / "state.sav";
    const auto saved = fanren::io::saveGame(state, path.string());
    EXPECT_TRUE(saved.ok) << saved.error;
    return saved.ok ? readFile(path) : std::string{};
}

void expectPreservedFields(const GameState& after, const GameState& before) {
    EXPECT_EQ(after.mapId, before.mapId);
    EXPECT_EQ(after.position, before.position);
    EXPECT_EQ(after.facing, before.facing);
    EXPECT_EQ(after.chapter, before.chapter);
    EXPECT_EQ(after.day, before.day);
    EXPECT_EQ(after.lastPracticeDay, before.lastPracticeDay);
    EXPECT_EQ(after.aptitude, before.aptitude);
    EXPECT_EQ(after.alchemyProficiency, before.alchemyProficiency);
    EXPECT_EQ(after.talismanProficiency, before.talismanProficiency);
    EXPECT_EQ(after.forgeProficiency, before.forgeProficiency);
    EXPECT_EQ(after.formationProficiency, before.formationProficiency);
    EXPECT_EQ(after.playSecondsGameplay, before.playSecondsGameplay);
    EXPECT_EQ(after.playSecondsSystem, before.playSecondsSystem);
    EXPECT_EQ(after.learnedMagics, before.learnedMagics);
    EXPECT_EQ(after.flags, before.flags);
    EXPECT_EQ(after.knownWeaknesses, before.knownWeaknesses);
    EXPECT_EQ(after.bottle.owned, before.bottle.owned);
    EXPECT_EQ(after.bottle.matureKnown, before.bottle.matureKnown);
    EXPECT_EQ(after.bottle.drops, before.bottle.drops);
    EXPECT_EQ(after.bottle.lastChargeDay, before.bottle.lastChargeDay);
    EXPECT_EQ(after.bottle.capacity, before.bottle.capacity);
    EXPECT_EQ(after.encounter.stepsSinceLast, before.encounter.stepsSinceLast);
    EXPECT_EQ(after.encounter.stepsUntilNext, before.encounter.stepsUntilNext);
    EXPECT_EQ(after.encounter.triggeredToday, before.encounter.triggeredToday);
    EXPECT_EQ(after.encounter.lastDay, before.encounter.lastDay);
    ASSERT_EQ(after.bag.size(), before.bag.size());
    for (std::size_t i = 0; i < before.bag.size(); ++i) {
        SCOPED_TRACE("bag " + std::to_string(i));
        EXPECT_EQ(after.bag[i].itemId, before.bag[i].itemId);
        EXPECT_EQ(after.bag[i].count, before.bag[i].count);
        EXPECT_EQ(after.bag[i].herbAge, before.bag[i].herbAge);
    }
    ASSERT_EQ(after.party.size(), before.party.size());
    for (std::size_t i = 0; i < before.party.size(); ++i) {
        SCOPED_TRACE("party " + std::to_string(i));
        EXPECT_EQ(after.party[i].roleId, before.party[i].roleId);
        EXPECT_EQ(after.party[i].hp, before.party[i].hp);
        EXPECT_EQ(after.party[i].active, before.party[i].active);
    }
    ASSERT_EQ(after.fields.size(), before.fields.size());
    for (std::size_t i = 0; i < before.fields.size(); ++i) {
        SCOPED_TRACE("field " + std::to_string(i));
        EXPECT_EQ(after.fields[i].id, before.fields[i].id);
        ASSERT_EQ(after.fields[i].slots.size(), before.fields[i].slots.size());
        for (std::size_t j = 0; j < before.fields[i].slots.size(); ++j) {
            SCOPED_TRACE("slot " + std::to_string(j));
            const auto& a = after.fields[i].slots[j];
            const auto& b = before.fields[i].slots[j];
            EXPECT_EQ(a.seedId, b.seedId);
            EXPECT_EQ(a.plantedDay, b.plantedDay);
            EXPECT_EQ(a.age, b.age);
            EXPECT_EQ(a.ripe, b.ripe);
            EXPECT_EQ(a.maxAge, b.maxAge);
        }
    }
}

void expectUnchangedState(const GameState& after, const GameState& before) {
    expectPreservedFields(after, before);
    EXPECT_EQ(after.realm, before.realm);
    EXPECT_EQ(after.realmCap, before.realmCap);
    EXPECT_EQ(after.formerRealm, before.formerRealm);
    EXPECT_EQ(after.hp, before.hp);
    EXPECT_EQ(after.maxHp, before.maxHp);
    EXPECT_EQ(after.mp, before.mp);
    EXPECT_EQ(after.maxMp, before.maxMp);
    EXPECT_EQ(after.cultivation, before.cultivation);
    EXPECT_EQ(after.cultivationRemainder, before.cultivationRemainder);
}

// Hand-built saves use the same independent checksum fixture as existing save tests.
fanren::core::Result<GameState> loadPayload(int version, const std::string& payload) {
    std::uint64_t hash = 0xcbf29ce484222325ULL;
    for (const char c : std::to_string(version) + "|" + payload) {
        hash ^= static_cast<unsigned char>(c);
        hash *= 0x100000001b3ULL;
    }
    std::ostringstream checksum;
    checksum << std::hex << std::setw(16) << std::setfill('0') << hash;
    fanren::test::TempDir dir("fanren_ch09_payload");
    const fs::path path = dir.path() / "state.sav";
    {
        std::ofstream out(path, std::ios::binary);
        EXPECT_TRUE(out.is_open());
        out << "{\"save_version\":" << version << ",\"checksum\":\"" << checksum.str()
            << "\",\"payload\":" << payload << "}";
    }
    return fanren::io::loadGame(path.string());
}

std::string payload(int realm = 3, int cap = 3, const std::string& former = {}) {
    return R"({"bag":[],"chapter":9,"cultivation":7,"day":77,"facing":2,"flags":{})" +
           (former.empty() ? std::string{} : R"(,"formerRealm":)" + former) +
           R"(,"hp":50,"mapId":"ch03_guwai","maxHp":60,"maxMp":30,"mp":17,)"
           R"("playSecondsGameplay":23.0,"playSecondsSystem":5.0,"position":{"x":3,"y":4},"realm":)" +
           std::to_string(realm) + R"(,"realmCap":)" + std::to_string(cap) + "}";
}

fs::path projectRoot() {
    fs::path dir = fs::current_path();
    for (int depth = 0; depth < 8; ++depth) {
        if (fs::exists(dir / "scripts/common/api.lua") && fs::is_directory(dir / "data")) return dir;
        const fs::path parent = dir.parent_path();
        if (parent == dir) break;
        dir = parent;
    }
    return {};
}

void expectSavedFormerRealm(const fs::path& path) {
    SCOPED_TRACE(path.string());
    const auto loaded = fanren::io::loadGame(path.string());
    ASSERT_TRUE(loaded.ok) << loaded.error;
    const auto original = nlohmann::json::parse(readFile(path), nullptr, false);
    ASSERT_FALSE(original.is_discarded());
    ASSERT_TRUE(original.contains("payload") && original.at("payload").is_object());
    const auto& stored = original.at("payload");
    if (!stored.contains("formerRealm")) {
        EXPECT_EQ(loaded.value.formerRealm, Realm::Mortal);
    } else {
        ASSERT_TRUE(stored.at("formerRealm").is_number_integer());
        EXPECT_EQ(fanren::rules::toValue(loaded.value.formerRealm),
                  stored.at("formerRealm").get<std::int64_t>());
    }
}

// Explicit instantiation permits naming a private member without changing the production header.
struct MenuMagicRows {
    using Pointer = fanren::ui::ListView MenuScene::*;
    friend Pointer menuMagicRows(MenuMagicRows);
};
template <typename Tag, typename Tag::Pointer Member>
struct MemberProbe {
    friend typename Tag::Pointer menuMagicRows(Tag) { return Member; }
};
template struct MemberProbe<MenuMagicRows, &MenuScene::magics_>;

const fanren::ui::ListView& magicList(const MenuScene& menu) {
    return menu.*menuMagicRows(MenuMagicRows{});
}

class Ch09EngineTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        const fs::path source = projectRoot();
        ASSERT_FALSE(source.empty());
        assets_ = std::make_unique<fanren::test::TempDir>("fanren_ch09_engine");
        fs::copy(source / "data", assets_->path() / "data", fs::copy_options::recursive);
        fs::create_directories(assets_->path() / "scripts/common");
        fs::create_directories(assets_->path() / "scripts/t");
        fs::copy_file(source / "scripts/common/api.lua", assets_->path() / "scripts/common/api.lua");
    }

    static void TearDownTestSuite() { assets_.reset(); }

    void SetUp() override {
        const auto ready = app_.init(assets_->path().string(), true);
        ASSERT_TRUE(ready.ok) << ready.error;
    }
    void TearDown() override { app_.shutdown(); }

    GameState fullState() {
        GameState state;
        state.mapId = "ch03_guwai";
        state.position = {3, 4};
        state.facing = 1;
        state.realm = state.realmCap = Realm::FoundationMid;
        state.hp = state.maxHp = 380;
        state.mp = state.maxMp = 250;
        state.cultivation = 1500;
        state.cultivationRemainder = 7;
        state.day = 777;
        state.chapter = 9;
        state.lastPracticeDay = 775;
        state.aptitude = 43;
        state.playSecondsGameplay = 23.0;
        state.playSecondsSystem = 5.0;
        state.bottle = {true, true, 4, 775, 6};
        state.bag = {{"material_lingshi", 173, 0}, {"herb_huangjing_cao", 2, 44}};
        state.flags = {{fanren::game::kXiuxianKnownFlag, 1}, {"ch09.probe.keep", 2}};
        state.party = {{"ch09_probe_puppet", 83, true}};
        state.fields = {{"ch09_probe_field", {{"herb_huangjing_cao", 700, 44, true, 44}}}};
        state.knownWeaknesses = {{"wild_wolf", fanren::core::kCategoryFist}};
        state.encounter = {2, 11, 1, 777};
        state.alchemyProficiency = 17;
        state.talismanProficiency = 23;
        state.forgeProficiency = 31;
        state.formationProficiency = 47;
        for (const auto& [id, magic] : app_.data().magics) {
            static_cast<void>(magic);
            if (state.learnedMagics.size() == 12u) break;
            state.learnedMagics.push_back(id);
        }
        EXPECT_EQ(state.learnedMagics.size(), 12u);
        return state;
    }

    void runLua(const std::string& source) {
        const fs::path path = assets_->path() / "scripts/t/demote.lua";
        {
            std::ofstream out(path, std::ios::binary | std::ios::trunc);
            ASSERT_TRUE(out.is_open());
            out << source;
        }
        const auto started = app_.startEvent("t/demote.lua");
        ASSERT_TRUE(started.ok) << started.error;
        for (int frame = 0; frame < 64 && app_.scripts().isRunning(); ++frame) app_.tick(0.0);
        ASSERT_FALSE(app_.scripts().isRunning());
        ASSERT_TRUE(app_.scripts().lastError().empty()) << app_.scripts().lastError();
    }

    void learnMenuProbes() {
        fanren::core::Magic high;
        high.id = "magic_ch09_high";
        high.name = "High spell";
        high.needRealm = Realm::FoundationEarly;
        high.needMp = 7;
        app_.data().magics.emplace(high.id, high);
        fanren::core::Magic low = high;
        low.id = "magic_ch09_low";
        low.name = "Low spell";
        low.needRealm = Realm::QiRefining1;
        low.needMp = 5;
        app_.data().magics.emplace(low.id, low);
        app_.state().learnedMagics = {high.id, low.id};
    }

    static inline std::unique_ptr<fanren::test::TempDir> assets_;
    Application app_;
};

TEST(Ch09Realm, TargetChecksCoverInvalidEqualHigherAndLowerRealms) {
    using fanren::rules::DemoteCheck;
    struct Case { int current; int target; DemoteCheck expected; };
    for (const Case& row : std::vector<Case>{
             {22, 3, DemoteCheck::Ok}, {3, 3, DemoteCheck::AlreadyThere},
             {3, 22, DemoteCheck::NotLower}, {22, 0, DemoteCheck::NoRealm},
             {22, 15, DemoteCheck::NoRealm}, {15, 3, DemoteCheck::NoRealm},
             {3, 1, DemoteCheck::Ok}, {0, 0, DemoteCheck::NoRealm},
             {0, 1, DemoteCheck::NotLower}, {-1, 3, DemoteCheck::NoRealm},
             {3, -1, DemoteCheck::NoRealm}, {34, 3, DemoteCheck::NoRealm},
             {3, 34, DemoteCheck::NoRealm}, {33, 31, DemoteCheck::Ok}}) {
        SCOPED_TRACE(std::to_string(row.current) + " -> " + std::to_string(row.target));
        EXPECT_EQ(fanren::rules::checkDemote(fanren::rules::fromValue(row.current),
                                           fanren::rules::fromValue(row.target)), row.expected);
    }
}

TEST(Ch09Realm, TheCommandIsAppendedAfterTakeItemAged) {
    using fanren::script::CommandKind;
    EXPECT_EQ(static_cast<int>(CommandKind::RealmDemote), static_cast<int>(CommandKind::TakeItemAged) + 1);
}

TEST(Ch09Realm, ANewStateHasNoFormerRealm) {
    const GameState state;
    EXPECT_EQ(state.formerRealm, Realm::Mortal);
}

TEST_F(Ch09EngineTest, RealLuaDemotionResetsOnlyTheContractFields) {
    app_.state() = fullState();
    const GameState before = app_.state();
    GameState expected = before;
    expected.realm = expected.realmCap = Realm::QiRefining3;
    expected.formerRealm = Realm::FoundationMid;
    expected.hp = expected.maxHp = 60;
    expected.maxMp = 30;
    expected.mp = expected.cultivation = expected.cultivationRemainder = 0;
    runLua("local ok, code = realm.demote(realm.QI_REFINING_3)\nassert(ok and code == '')\n");
    const GameState& state = app_.state();
    EXPECT_EQ(state.realm, Realm::QiRefining3);
    EXPECT_EQ(state.realmCap, Realm::QiRefining3);
    EXPECT_EQ(state.formerRealm, Realm::FoundationMid);
    EXPECT_EQ(state.hp, 60);
    EXPECT_EQ(state.maxHp, 60);
    EXPECT_EQ(state.mp, 0);
    EXPECT_EQ(state.maxMp, 30);
    EXPECT_EQ(state.cultivation, 0);
    EXPECT_EQ(state.cultivationRemainder, 0);
    expectPreservedFields(state, before);
    EXPECT_EQ(savedBytes(state), savedBytes(expected));
    EXPECT_EQ(fanren::game::bottleChargeDays(state.realm), 7);
}

TEST_F(Ch09EngineTest, DemotionClampsHealthWithoutHealingAWound) {
    for (const int hp : {0, 1, 50, 200, 380}) {
        SCOPED_TRACE(hp);
        app_.state() = fullState();
        app_.state().hp = hp;
        runLua("local ok, code = realm.demote(3)\nassert(ok and code == '')\n");
        EXPECT_EQ(app_.state().hp, std::min(hp, 60));
        EXPECT_EQ(app_.state().maxHp, 60);
    }
}

TEST_F(Ch09EngineTest, RefusalAndAnEqualTargetLeaveTheEntireStateUnchanged) {
    for (const int target : {22, 0, 15, -1, 34, 3}) {
        SCOPED_TRACE(target);
        app_.state() = fullState();
        app_.state().realm = Realm::QiRefining3;
        app_.state().formerRealm = Realm::CoreEarly;
        const std::string before = savedBytes(app_.state());
        const std::string reply = target == 3 ? "ok and code == ''" :
                                 target == 22 ? "not ok and code == 'not_lower'" :
                                                "not ok and code == 'no_realm'";
        runLua("local ok, code = realm.demote(" + std::to_string(target) + ")\nassert(" + reply + ")\n");
        EXPECT_EQ(savedBytes(app_.state()), before);
    }
}

TEST_F(Ch09EngineTest, AnInvalidCurrentRealmIsRefusedWithoutChangingState) {
    app_.state() = fullState();
    app_.state().realm = fanren::rules::fromValue(15);
    const std::string before = savedBytes(app_.state());
    runLua("local ok, code = realm.demote(3)\nassert(not ok and code == 'no_realm')\n");
    EXPECT_EQ(savedBytes(app_.state()), before);
}

TEST_F(Ch09EngineTest, MortalToMortalIsRefusedBeforeTheEqualityCheck) {
    app_.state() = fullState();
    app_.state().realm = Realm::Mortal;
    const std::string before = savedBytes(app_.state());
    runLua("local ok, code = realm.demote(0)\nassert(not ok and code == 'no_realm')\n");
    EXPECT_EQ(savedBytes(app_.state()), before);
}

TEST_F(Ch09EngineTest, ConsecutiveDemotionsRememberTheHighestFormerRealm) {
    app_.state() = fullState();
    runLua("assert(realm.demote(13))\nassert(realm.demote(3))\n");
    EXPECT_EQ(app_.state().realm, Realm::QiRefining3);
    EXPECT_EQ(app_.state().realmCap, Realm::QiRefining3);
    EXPECT_EQ(app_.state().formerRealm, Realm::FoundationMid);
}

TEST_F(Ch09EngineTest, DemotionKeepsAnEvenHigherRecordedFormerRealm) {
    app_.state() = fullState();
    app_.state().formerRealm = Realm::CoreEarly;
    runLua("assert(realm.demote(3))\n");
    EXPECT_EQ(app_.state().formerRealm, Realm::CoreEarly);
}

TEST_F(Ch09EngineTest, AdvanceAfterDemotionKeepsFormerRealmAndTheStoryCap) {
    app_.state() = fullState();
    runLua("assert(realm.demote(3))\nassert(realm.advance(5))\nassert(realm.cap(3))\n");
    EXPECT_EQ(app_.state().realm, Realm::QiRefining5);
    EXPECT_EQ(app_.state().realmCap, Realm::QiRefining5);
    EXPECT_EQ(app_.state().maxHp, 84);
    EXPECT_EQ(app_.state().formerRealm, Realm::FoundationMid);
    EXPECT_EQ(fanren::rules::breakthroughBlock(app_.state().realm, 99999, app_.state().realmCap),
              fanren::rules::BreakthroughBlock::StoryCap);
}

TEST_F(Ch09EngineTest, TheBattleUsesDemotedStatsAndRefusesAHigherRealmSpell) {
    app_.state() = fullState();
    learnMenuProbes();
    runLua("assert(realm.demote(3))\n");
    app_.state().party.clear();
    app_.state().mp = 30;
    ASSERT_NE(app_.battleSetup("b03_gu_wai_elang"), nullptr);
    fanren::game::BattleScene scene("b03_gu_wai_elang");
    scene.onEnter(app_);
    const int actor = scene.runToAllyTurn();
    ASSERT_GE(actor, 0);
    const auto& hero = scene.battle().units()[static_cast<std::size_t>(actor)];
    ASSERT_EQ(hero.id, "hanli");
    EXPECT_EQ(hero.realm, Realm::QiRefining3);
    EXPECT_EQ(hero.attack, fanren::rules::realmAttack(Realm::QiRefining3));
    EXPECT_EQ(hero.defence, fanren::rules::realmDefence(Realm::QiRefining3));
    EXPECT_EQ(hero.maxHp, 60);
    EXPECT_EQ(hero.maxMp, 30);
    scene.openMenu(app_);
    ASSERT_TRUE(scene.menuChoose(app_, fanren::game::kBattleMenuCast));
    bool sawHigh = false;
    bool sawLow = false;
    for (const auto& row : scene.menuList().items()) {
        if (row.label == "High spell") {
            sawHigh = true;
            EXPECT_FALSE(row.enabled);
            ASSERT_FALSE(row.disabledReason.empty());
            EXPECT_NE(row.disabledReason.find("境界不足"), std::string::npos);
        } else if (row.label == "Low spell") {
            sawLow = true;
            EXPECT_TRUE(row.enabled) << row.disabledReason;
        }
    }
    EXPECT_TRUE(sawHigh);
    EXPECT_TRUE(sawLow);
}

TEST_F(Ch09EngineTest, LuaRejectsNonNumericTargetsWithoutChangingState) {
    app_.state() = fullState();
    const std::string before = savedBytes(app_.state());
    runLua("assert(not pcall(realm.demote, '3'))\nassert(not pcall(realm.demote, nil))\n");
    EXPECT_EQ(savedBytes(app_.state()), before);
}

TEST_F(Ch09EngineTest, LuaRejectsAnOversizedTargetThatWouldNarrowToThree) {
    app_.state() = fullState();
    const GameState before = app_.state();
    runLua("local ok, code = realm.demote(4294967299)\nassert(not ok and code == 'no_realm')\n");
    expectUnchangedState(app_.state(), before);
    EXPECT_EQ(savedBytes(app_.state()), savedBytes(before));
}

TEST_F(Ch09EngineTest, LuaRejectsOtherNonIntegralOrOutOfRangeTargets) {
    for (const char* target : {"-4294967293", "2147483648", "-2147483649", "1e100", "3.5",
                               "-0.5", "1/0", "-1/0", "0/0", "math.maxinteger", "math.mininteger"}) {
        SCOPED_TRACE(target);
        app_.state() = fullState();
        const GameState before = app_.state();
        runLua(std::string("local ok, code = realm.demote(") + target +
               ")\nassert(not ok and code == 'no_realm')\n");
        expectUnchangedState(app_.state(), before);
        EXPECT_EQ(savedBytes(app_.state()), savedBytes(before));
    }
}

TEST_F(Ch09EngineTest, LuaAcceptsExactlyRepresentableIntegralTargets) {
    for (const char* target : {"3", "3.0", "3e0"}) {
        SCOPED_TRACE(target);
        app_.state() = fullState();
        runLua(std::string("local ok, code = realm.demote(") + target + ")\nassert(ok and code == '')\n");
        EXPECT_EQ(app_.state().realm, Realm::QiRefining3);
        EXPECT_EQ(app_.state().realmCap, Realm::QiRefining3);
        EXPECT_EQ(app_.state().formerRealm, Realm::FoundationMid);
    }
}

TEST_F(Ch09EngineTest, RawDemoteCommandsRejectNonNumericTargetsWithoutChangingState) {
    for (const char* target : {"nil", "'3'", "false", "{}"}) {
        SCOPED_TRACE(target);
        app_.state() = fullState();
        const GameState before = app_.state();
        runLua(std::string("local result = coroutine.yield{kind='realm_demote', x=") + target +
               "}\nassert(not result.ok and result.code == 'no_realm')\n");
        expectUnchangedState(app_.state(), before);
    }
}

TEST_F(Ch09EngineTest, AnActualCastBelowTheRequiredRealmLeavesHpAndMpUnchanged) {
    app_.state() = fullState();
    learnMenuProbes();
    runLua("assert(realm.demote(3))\n");
    app_.state().party.clear();
    app_.state().mp = 30;
    ASSERT_NE(app_.battleSetup("b03_gu_wai_elang"), nullptr);
    fanren::game::BattleScene scene("b03_gu_wai_elang");
    scene.onEnter(app_);
    const int actor = scene.runToAllyTurn();
    ASSERT_GE(actor, 0);
    const auto beforeUnits = scene.battle().units();
    const GameState beforeState = app_.state();
    int enemy = -1;
    for (std::size_t i = 0; i < beforeUnits.size(); ++i) {
        if (!beforeUnits[i].ally && beforeUnits[i].alive()) {
            enemy = static_cast<int>(i);
            break;
        }
    }
    ASSERT_GE(enemy, 0);
    fanren::core::battle::Action action;
    action.kind = fanren::core::battle::ActionKind::Cast;
    action.actorIndex = actor;
    action.targetIndex = enemy;
    action.magicId = "magic_ch09_high";
    const auto result = scene.issuePlayerAction(app_, action);
    ASSERT_FALSE(result.ok);
    EXPECT_NE(result.error.find("境界不足"), std::string::npos) << result.error;
    EXPECT_EQ(scene.battle().currentActor(), actor);
    ASSERT_EQ(scene.battle().units().size(), beforeUnits.size());
    for (std::size_t i = 0; i < beforeUnits.size(); ++i) {
        EXPECT_EQ(scene.battle().units()[i].hp, beforeUnits[i].hp);
        EXPECT_EQ(scene.battle().units()[i].mp, beforeUnits[i].mp);
    }
    expectUnchangedState(app_.state(), beforeState);
}

TEST(Ch09Save, TheCurrentVersionIsNine) {
    EXPECT_EQ(fanren::io::kSaveVersion, 9);
}

TEST_F(Ch09EngineTest, ACompleteDemotedStateRoundTripsIncludingFormerRealm) {
    app_.state() = fullState();
    runLua("assert(realm.demote(3))\n");
    fanren::test::TempDir dir("fanren_ch09_roundtrip");
    const fs::path path = dir.path() / "state.sav";
    ASSERT_TRUE(fanren::io::saveGame(app_.state(), path.string()).ok);
    const std::string text = readFile(path);
    EXPECT_NE(text.find("\"save_version\": 9"), std::string::npos);
    EXPECT_NE(text.find("\"formerRealm\": 22"), std::string::npos);
    const auto loaded = fanren::io::loadGame(path.string());
    ASSERT_TRUE(loaded.ok) << loaded.error;
    EXPECT_EQ(loaded.value.formerRealm, Realm::FoundationMid);
    EXPECT_EQ(savedBytes(loaded.value), savedBytes(app_.state()));
}

TEST(Ch09Save, VersionEightDefaultsFormerRealmAndPreservesEveryOtherField) {
    const auto loaded = loadPayload(8, payload());
    ASSERT_TRUE(loaded.ok) << loaded.error;
    GameState expected;
    expected.mapId = "ch03_guwai";
    expected.position = {3, 4};
    expected.realm = expected.realmCap = Realm::QiRefining3;
    expected.cultivation = 7;
    expected.hp = 50;
    expected.maxHp = 60;
    expected.mp = 17;
    expected.maxMp = 30;
    expected.day = 77;
    expected.chapter = 9;
    expected.playSecondsGameplay = 23.0;
    expected.playSecondsSystem = 5.0;
    EXPECT_EQ(loaded.value.formerRealm, Realm::Mortal);
    EXPECT_EQ(savedBytes(loaded.value), savedBytes(expected));
}

TEST(Ch09Save, TheExistingVersionEightFixtureCanBeLoadedAndResaved) {
    const fs::path source = projectRoot() / "saves/ch07-start.sav";
    ASSERT_NE(readFile(source).find("\"save_version\": 8"), std::string::npos);
    const auto loaded = fanren::io::loadGame(source.string());
    ASSERT_TRUE(loaded.ok) << loaded.error;
    EXPECT_EQ(loaded.value.formerRealm, Realm::Mortal);
    fanren::test::TempDir dir("fanren_ch09_v8_fixture");
    const fs::path path = dir.path() / "v9.sav";
    ASSERT_TRUE(fanren::io::saveGame(loaded.value, path.string()).ok);
    const auto reloaded = fanren::io::loadGame(path.string());
    ASSERT_TRUE(reloaded.ok) << reloaded.error;
    EXPECT_EQ(savedBytes(reloaded.value), savedBytes(loaded.value));
}

TEST(Ch09Save, EveryLegacyVersionHasARegisteredMigrationToNine) {
    for (int version = 1; version <= 8; ++version) {
        SCOPED_TRACE(version);
        const auto loaded = loadPayload(version, payload());
        ASSERT_TRUE(loaded.ok) << loaded.error;
        EXPECT_EQ(loaded.value.formerRealm, Realm::Mortal);
        EXPECT_EQ(loaded.value.realm, Realm::QiRefining3);
        EXPECT_EQ(loaded.value.realmCap, Realm::QiRefining3);
        EXPECT_EQ(loaded.value.hp, 50);
        EXPECT_EQ(loaded.value.mp, 17);
    }
}

TEST(Ch09Save, ACapBelowTheCurrentRealmIsRejectedWithBothNumbers) {
    const auto loaded = loadPayload(9, payload(8, 3, "22"));
    ASSERT_FALSE(loaded.ok);
    EXPECT_NE(loaded.error.find("3"), std::string::npos) << loaded.error;
    EXPECT_NE(loaded.error.find("8"), std::string::npos) << loaded.error;
    EXPECT_NE(loaded.error.find("上限"), std::string::npos) << loaded.error;
}

TEST(Ch09Save, InvalidFormerRealmValuesAreRejected) {
    for (const std::string& former : {"15", "-1", "34", "4294967296", "18446744073709551615",
                                     "\"22\"", "22.5", "true", "null", "[]", "{}"}) {
        SCOPED_TRACE(former);
        const auto loaded = loadPayload(9, payload(3, 3, former));
        ASSERT_FALSE(loaded.ok);
        EXPECT_NE(loaded.error.find("formerRealm"), std::string::npos) << loaded.error;
    }
}

TEST(Ch09Save, ACurrentVersionPayloadMayOmitFormerRealm) {
    const auto loaded = loadPayload(9, payload());
    ASSERT_TRUE(loaded.ok) << loaded.error;
    EXPECT_EQ(loaded.value.formerRealm, Realm::Mortal);
}

TEST(Ch09Save, RebuildingPastFormerRealmRemainsALegalSave) {
    const auto loaded = loadPayload(9, payload(23, 23, "22"));
    ASSERT_TRUE(loaded.ok) << loaded.error;
    EXPECT_EQ(loaded.value.realm, Realm::FoundationLate);
    EXPECT_EQ(loaded.value.formerRealm, Realm::FoundationMid);
}

TEST(Ch09Save, AllExistingFixtureAndPlayerSavesStillLoad) {
    const fs::path root = projectRoot();
    ASSERT_FALSE(root.empty());
    std::size_t count = 0;
    for (const char* relative : {"tests/fixtures", "saves"}) {
        for (const auto& entry : fs::directory_iterator(root / relative)) {
            if (!entry.is_regular_file() || entry.path().extension() != ".sav") continue;
            expectSavedFormerRealm(entry.path());
            ++count;
        }
    }
    EXPECT_GE(count, 10u) << "The compatibility scan must not pass on an empty directory";
}

TEST(Ch09Save, TheCompatibilityScanAcceptsAVersionNineDemotedSave) {
    fanren::test::TempDir dir("fanren_ch09_v9_scan");
    const fs::path path = dir.path() / "ch09-end-probe.sav";
    GameState state;
    state.mapId = "ch03_guwai";
    state.position = {3, 4};
    state.chapter = 9;
    state.realm = state.realmCap = Realm::QiRefining3;
    state.formerRealm = Realm::FoundationMid;
    state.hp = state.maxHp = 60;
    state.maxMp = 30;
    ASSERT_TRUE(fanren::io::saveGame(state, path.string()).ok);
    const auto original = nlohmann::json::parse(readFile(path), nullptr, false);
    ASSERT_FALSE(original.is_discarded());
    ASSERT_EQ(original.at("save_version"), 9);
    ASSERT_EQ(original.at("payload").at("realm"), 3);
    ASSERT_EQ(original.at("payload").at("realmCap"), 3);
    ASSERT_EQ(original.at("payload").at("formerRealm"), 22);
    expectSavedFormerRealm(path);
}

TEST_F(Ch09EngineTest, MenuShowsRealmShortageOnlyForTheUnavailableSpell) {
    app_.state().realm = Realm::QiRefining3;
    app_.state().setFlag(fanren::game::kXiuxianKnownFlag);
    learnMenuProbes();
    MenuScene menu(MenuScene::Page::Magic, true);
    menu.onEnter(app_);
    ASSERT_EQ(magicList(menu).count(), 2);
    const auto& rows = magicList(menu).items();
    EXPECT_EQ(rows[0].detail, "境界不足");
    EXPECT_EQ(rows[1].detail, "法力　5");
    EXPECT_TRUE(rows[0].enabled);
    EXPECT_TRUE(rows[1].enabled);
}

TEST_F(Ch09EngineTest, MenuKeepsManaCostsWhenTheRealmIsHighEnough) {
    app_.state().realm = Realm::FoundationMid;
    app_.state().setFlag(fanren::game::kXiuxianKnownFlag);
    learnMenuProbes();
    MenuScene menu(MenuScene::Page::Magic, true);
    menu.onEnter(app_);
    ASSERT_EQ(magicList(menu).count(), 2);
    EXPECT_EQ(magicList(menu).items()[0].detail, "法力　7");
    EXPECT_EQ(magicList(menu).items()[1].detail, "法力　5");
}

TEST_F(Ch09EngineTest, MenuAllowsTheExactRequiredRealmAndKeepsUnknownRows) {
    app_.state().realm = Realm::FoundationEarly;
    app_.state().setFlag(fanren::game::kXiuxianKnownFlag);
    learnMenuProbes();
    app_.state().learnedMagics.push_back("magic_ch09_unknown");
    MenuScene menu(MenuScene::Page::Magic, true);
    menu.onEnter(app_);
    ASSERT_EQ(magicList(menu).count(), 3);
    EXPECT_EQ(magicList(menu).items()[0].detail, "法力　7");
    EXPECT_EQ(magicList(menu).items()[2].label, "magic_ch09_unknown");
    EXPECT_TRUE(magicList(menu).items()[2].detail.empty());
}

TEST_F(Ch09EngineTest, TheChapterFourMenuKeepsMortalWordingEvenBelowASpellsRealm) {
    const auto loaded = fanren::io::loadGame((projectRoot() / "tests/fixtures/ch04-end-first.sav").string());
    ASSERT_TRUE(loaded.ok) << loaded.error;
    app_.state() = loaded.value;
    ASSERT_EQ(fanren::game::wordingStage(app_.state()), fanren::game::PanelStage::Mortal);
    learnMenuProbes();
    ASSERT_LT(fanren::rules::toValue(app_.state().realm), fanren::rules::toValue(Realm::FoundationEarly));
    MenuScene menu(MenuScene::Page::Magic, true);
    menu.onEnter(app_);
    ASSERT_EQ(magicList(menu).count(), 2);
    EXPECT_EQ(magicList(menu).items()[0].detail, "气力　7");
    for (const auto& row : magicList(menu).items()) {
        for (const char* forbidden : {"境界", "法力", "修仙", "灵根", "炼气", "筑基", "真元"}) {
            EXPECT_EQ(row.detail.find(forbidden), std::string::npos) << row.detail;
        }
    }
}

}  // namespace
