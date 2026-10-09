// Chapter 10 E3: ordinary all-opponent spells, interfaces-p3-ch10.md E3.
#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

#include "../vendor/json.hpp"
#include "TempDir.h"
#include "core/battle/Battle.h"
#include "core/model/Types.h"
#include "game/Application.h"
#include "game/BattleScene.h"
#include "game/Wording.h"
#include "io/DataLoader.h"

namespace {

namespace fs = std::filesystem;
namespace core = fanren::core;
namespace battle = fanren::core::battle;
namespace game = fanren::game;
using fanren::rules::Realm;
using fanren::test::TempDir;
using nlohmann::json;

battle::Unit unit(const std::string& id, bool ally, int speed = 1) {
    battle::Unit result;
    result.id = result.name = id;
    result.ally = ally;
    result.hp = result.maxHp = 1000;
    result.mp = result.maxMp = 100;
    result.attack = 10;
    result.defence = 5;
    result.speed = speed;
    result.realm = Realm::QiRefining3;
    return result;
}

core::Magic spell(core::MagicTarget target = core::MagicTarget::All) {
    core::Magic result;
    result.id = "probe_magic";
    result.name = "Probe magic";
    result.element = 8;
    result.needMp = 7;
    result.power = 12;
    result.target = target;
    return result;
}

battle::Action cast(int target = 1, int boost = 0) {
    battle::Action result;
    result.kind = battle::ActionKind::Cast;
    result.actorIndex = 0;
    result.targetIndex = target;
    result.magicId = "probe_magic";
    result.boost = boost;
    return result;
}

battle::BattleState field(const core::Magic& magic, bool allyCaster = true) {
    battle::BattleState result;
    result.setup({unit("caster", allyCaster, 100), unit("first", !allyCaster),
                  unit("second", !allyCaster)}, 7);
    result.addMagic(magic);
    return result;
}

std::vector<battle::BattleEvent> eventsOf(const battle::BattleState& state,
                                         battle::BattleEventKind kind) {
    std::vector<battle::BattleEvent> result;
    for (const auto& event : state.lastActionEvents()) {
        if (event.kind == kind) result.push_back(event);
    }
    return result;
}

TEST(Ch10AllMagicRule, OldSpellsDefaultToSingleAndHitOnlyTheChosenOpponent) {
    core::Magic magic = spell(core::MagicTarget::Single);
    EXPECT_EQ(core::Magic{}.target, core::MagicTarget::Single);
    auto state = field(magic);
    const auto applied = state.apply(cast(2));
    ASSERT_TRUE(applied.ok) << applied.error;
    EXPECT_EQ(state.units()[0].hp, 1000);
    EXPECT_EQ(state.units()[1].hp, 1000);
    EXPECT_LT(state.units()[2].hp, 1000);
    EXPECT_EQ(state.units()[0].mp, 100 - magic.needMp);
    const auto hits = eventsOf(state, battle::BattleEventKind::Hit);
    ASSERT_EQ(hits.size(), 1u);
    EXPECT_EQ(hits[0].target, 2);
    EXPECT_EQ(applied.value.find("横扫全场"), std::string::npos);
}

TEST(Ch10AllMagicRule, AllFiltersBothSidesDeadFledAndFutureWavesInStableIndexOrder) {
    auto dead = unit("dead", false);
    dead.hp = 0;
    auto fled = unit("fled", false);
    fled.fled = true;
    auto future = unit("future", false);
    future.wave = 1;
    battle::BattleState state;
    state.setup({unit("caster", true, 100), unit("first", false), unit("friend", true),
                 unit("second", false), dead, fled, future}, 7);
    state.addMagic(spell());
    const int bpBefore = state.units()[0].bp;
    const auto applied = state.apply(cast(3));
    ASSERT_TRUE(applied.ok) << applied.error;
    EXPECT_LT(state.units()[1].hp, 1000);
    EXPECT_LT(state.units()[3].hp, 1000);
    for (const int untouched : {0, 2, 5, 6}) EXPECT_EQ(state.units()[untouched].hp, 1000);
    EXPECT_EQ(state.units()[4].hp, 0);
    EXPECT_EQ(state.units()[0].mp, 93);
    EXPECT_EQ(state.units()[0].bp, bpBefore);
    EXPECT_FALSE(state.units()[0].charging);
    EXPECT_EQ(state.currentWave(), 0);
    const auto hits = eventsOf(state, battle::BattleEventKind::Hit);
    ASSERT_EQ(hits.size(), 2u);
    EXPECT_EQ(hits[0].target, 1);
    EXPECT_EQ(hits[1].target, 3);
    EXPECT_EQ(hits[0].category, core::kCategoryFire);
    EXPECT_NE(applied.value.find("横扫全场"), std::string::npos);
    EXPECT_NE(applied.value.find("first"), std::string::npos);
    EXPECT_NE(applied.value.find("second"), std::string::npos);
}

TEST(Ch10AllMagicRule, TheEnemyAiReallyCastsOrdinaryAllMagicWithoutCharging) {
    const auto magic = spell();
    auto state = field(magic, false);
    const auto action = state.decideAi(0);
    ASSERT_EQ(action.kind, battle::ActionKind::Cast);
    ASSERT_EQ(action.magicId, magic.id);
    ASSERT_TRUE(state.isLegal(action));
    ASSERT_TRUE(state.apply(action).ok);
    EXPECT_EQ(state.units()[0].hp, 1000);
    EXPECT_EQ(state.units()[0].mp, 93);
    EXPECT_FALSE(state.units()[0].charging);
    EXPECT_LT(state.units()[1].hp, 1000);
    EXPECT_LT(state.units()[2].hp, 1000);
    EXPECT_EQ(eventsOf(state, battle::BattleEventKind::Hit).size(), 2u);
}

TEST(Ch10AllMagicRule, HitsBoostAndPoisonUseOneMpAndBpPaymentForEveryOpponent) {
    auto magic = spell();
    magic.boost = core::MagicBoost::Hits;
    magic.poison = 2;
    magic.poisonPower = 3;
    auto first = unit("first", false);
    auto second = unit("second", false);
    first.maxToughness = second.maxToughness = 1;
    first.weaknesses = second.weaknesses = core::kCategoryFire;
    battle::BattleState state;
    state.setup({unit("caster", true, 100), first, second}, 7);
    state.addMagic(magic);
    const int bpBefore = state.units()[0].bp;
    ASSERT_GE(bpBefore, 1);
    ASSERT_TRUE(state.apply(cast(1, 1)).ok);
    EXPECT_EQ(state.units()[0].mp, 93);
    EXPECT_EQ(state.units()[0].bp, bpBefore - 1);
    EXPECT_EQ(eventsOf(state, battle::BattleEventKind::BoostSpent).size(), 1u);
    EXPECT_EQ(eventsOf(state, battle::BattleEventKind::Poisoned).size(), 2u);
    EXPECT_EQ(eventsOf(state, battle::BattleEventKind::Break).size(), 2u);
    const auto hits = eventsOf(state, battle::BattleEventKind::Hit);
    ASSERT_EQ(hits.size(), 4u);
    for (std::size_t i = 0; i < hits.size(); ++i) {
        EXPECT_EQ(hits[i].target, i < 2 ? 1 : 2);
        EXPECT_EQ(hits[i].hit, static_cast<int>(i % 2) + 1);
        EXPECT_EQ(hits[i].hits, 2);
    }
    for (const int target : {1, 2}) {
        EXPECT_EQ(state.units()[target].poison, 2);
        EXPECT_EQ(state.units()[target].poisonPower, 3);
        EXPECT_EQ(state.units()[target].toughness, 0);
        EXPECT_TRUE(state.units()[target].broken());
    }
}

TEST(Ch10AllMagicRule, PowerBoostUsesTheSameDamageAsSingleAndPaysOnlyOnce) {
    auto single = field(spell(core::MagicTarget::Single));
    auto all = field(spell());
    const int bpBefore = all.units()[0].bp;
    ASSERT_TRUE(single.apply(cast(1, 1)).ok);
    ASSERT_TRUE(all.apply(cast(1, 1)).ok);
    EXPECT_EQ(all.units()[1].hp, single.units()[1].hp);
    EXPECT_EQ(all.units()[2].hp, single.units()[1].hp);
    EXPECT_EQ(all.units()[0].mp, single.units()[0].mp);
    EXPECT_EQ(all.units()[0].bp, bpBefore - 1);
    EXPECT_EQ(eventsOf(all, battle::BattleEventKind::Hit).size(), 2u);
    EXPECT_EQ(eventsOf(all, battle::BattleEventKind::BoostSpent).size(), 1u);
}

void finishRoundWithGuards(battle::BattleState& state) {
    while (state.currentActor() >= 0) {
        battle::Action guard;
        guard.kind = battle::ActionKind::Defend;
        guard.actorIndex = state.currentActor();
        ASSERT_TRUE(state.apply(guard).ok);
        state.endTurn();
    }
    state.endTurn();
}

TEST(Ch10AllMagicRule, ChargedAllAndOrdinaryAllFormAUnionWithoutDoubleHits) {
    for (const auto target : {core::MagicTarget::Single, core::MagicTarget::All}) {
        auto caster = unit("caster", false, 100);
        caster.chargeEvery = 2;
        caster.chargeMult = 2.0;
        caster.chargeAll = true;
        battle::BattleState state;
        state.setup({caster, unit("first", true), unit("second", true)}, 7);
        state.addMagic(spell(target));
        finishRoundWithGuards(state);
        ASSERT_EQ(state.currentActor(), 0);
        battle::Action charge;
        charge.kind = battle::ActionKind::Charge;
        charge.actorIndex = 0;
        ASSERT_TRUE(state.apply(charge).ok);
        state.endTurn();
        finishRoundWithGuards(state);
        while (state.currentActor() != 0) {
            ASSERT_GE(state.currentActor(), 0);
            battle::Action guard;
            guard.kind = battle::ActionKind::Defend;
            guard.actorIndex = state.currentActor();
            ASSERT_TRUE(state.apply(guard).ok);
            state.endTurn();
        }
        ASSERT_EQ(state.currentActor(), 0);
        ASSERT_TRUE(state.units()[0].charging);
        const int mpBefore = state.units()[0].mp;
        ASSERT_TRUE(state.apply(cast()).ok);
        EXPECT_EQ(eventsOf(state, battle::BattleEventKind::Hit).size(), 2u);
        EXPECT_EQ(state.units()[0].mp, mpBefore - 7);
        EXPECT_FALSE(state.units()[0].charging);
        EXPECT_LT(state.units()[1].hp, 1000);
        EXPECT_LT(state.units()[2].hp, 1000);
    }
}

TEST(Ch10AllMagicRule, IllegalAnchorsAndResourcesCannotChangeAnyCombatState) {
    for (const int badTarget : {-1, 0, 3, 99}) {
        auto state = field(spell());
        const auto before = state.units();
        const auto beforeEvents = state.events().size();
        const auto action = cast(badTarget);
        ASSERT_FALSE(state.isLegal(action));
        ASSERT_FALSE(state.apply(action).ok);
        for (std::size_t i = 0; i < before.size(); ++i) {
            EXPECT_EQ(state.units()[i].hp, before[i].hp);
            EXPECT_EQ(state.units()[i].mp, before[i].mp);
            EXPECT_EQ(state.units()[i].bp, before[i].bp);
        }
        EXPECT_EQ(state.events().size(), beforeEvents);
        EXPECT_TRUE(state.log().empty());
    }
    for (const int denied : {0, 1, 2, 3}) {
        auto caster = unit("caster", true, 100);
        if (denied == 0) caster.mp = 0;
        if (denied == 1) caster.realm = Realm::Mortal;
        if (denied == 2) caster.magicsExhaustive = true;
        battle::BattleState state;
        state.setup({caster, unit("first", false), unit("second", false)}, 7);
        state.addMagic(spell());
        const auto before = state.units();
        ASSERT_FALSE(state.apply(cast(1, denied == 3 ? 99 : 0)).ok);
        for (std::size_t i = 0; i < before.size(); ++i) {
            EXPECT_EQ(state.units()[i].hp, before[i].hp);
            EXPECT_EQ(state.units()[i].mp, before[i].mp);
            EXPECT_EQ(state.units()[i].bp, before[i].bp);
        }
    }
}

TEST(Ch10AllMagicRule, InvalidAllEffectModelsAreRefusedBeforeAnyPayment) {
    for (const auto effect : {core::MagicEffect::Reveal, core::MagicEffect::Stagger,
                              core::MagicEffect::None}) {
        auto magic = spell();
        magic.power = 0;
        magic.effect = effect;
        auto state = field(magic);
        const auto before = state.units();
        ASSERT_FALSE(state.isLegal(cast()));
        ASSERT_FALSE(state.apply(cast()).ok);
        for (std::size_t i = 0; i < before.size(); ++i) {
            EXPECT_EQ(state.units()[i].hp, before[i].hp);
            EXPECT_EQ(state.units()[i].mp, before[i].mp);
            EXPECT_EQ(state.units()[i].bp, before[i].bp);
        }
    }
}

void writeJson(const fs::path& path, const json& value) {
    fs::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output << value.dump() << '\n';
}

json magicJson() {
    return { {"id", "probe_magic"}, {"name", "Probe magic"}, {"element", 8},
             {"needMp", 7}, {"power", 12} };
}

core::Result<core::GameData> loadMagic(const TempDir& temp, const json& value) {
    writeJson(temp.path() / "data/magics/probe.json", value);
    return fanren::io::loadGameData((temp.path() / "data").string());
}

TEST(Ch10AllMagicLoading, MissingSingleAndAllRoundTripThroughTheRealLoader) {
    TempDir temp{"fanren_ch10_magic_target"};
    auto value = magicJson();
    auto loaded = loadMagic(temp, value);
    ASSERT_TRUE(loaded.ok) << loaded.error;
    EXPECT_EQ(loaded.value.findMagic("probe_magic")->target, core::MagicTarget::Single);
    for (const std::string target : {"single", "all"}) {
        value["target"] = target;
        loaded = loadMagic(temp, value);
        ASSERT_TRUE(loaded.ok) << loaded.error;
        EXPECT_EQ(loaded.value.findMagic("probe_magic")->target,
                  target == "all" ? core::MagicTarget::All : core::MagicTarget::Single);
    }
}

TEST(Ch10AllMagicLoading, WrongTypesUnknownValuesAndWhitespaceAreRefusedWithTheFieldAndFile) {
    TempDir temp{"fanren_ch10_magic_target_bad"};
    const std::vector<json> bad{nullptr, true, 1, 1.5, json::array(), json::object(),
                               "", "ALL", "all ", " single", "friends"};
    for (const auto& target : bad) {
        auto value = magicJson();
        value["target"] = "all";
        ASSERT_TRUE(loadMagic(temp, value).ok) << "Positive control before " << target.dump();
        value["target"] = target;
        const auto loaded = loadMagic(temp, value);
        EXPECT_FALSE(loaded.ok) << target.dump();
        EXPECT_NE(loaded.error.find("target"), std::string::npos) << loaded.error;
        EXPECT_NE(loaded.error.find("probe.json"), std::string::npos) << loaded.error;
    }
}

TEST(Ch10AllMagicLoading, AllCannotTurnRevealStaggerOrAnInertSpellIntoAnAreaAttack) {
    TempDir temp{"fanren_ch10_magic_target_effect"};
    for (const std::string effect : {"reveal", "stagger", ""}) {
        auto value = magicJson();
        value["power"] = 0;
        value["target"] = "single";
        if (!effect.empty()) value["effect"] = effect;
        ASSERT_TRUE(loadMagic(temp, value).ok) << effect;
        value["target"] = "all";
        const auto loaded = loadMagic(temp, value);
        EXPECT_FALSE(loaded.ok) << effect;
        EXPECT_NE(loaded.error.find("target"), std::string::npos) << loaded.error;
    }
}

TEST(Ch10AllMagicMenu, AllIsOneGroupAndSingleStillListsEachOpponent) {
    core::GameData data;
    core::GameState save;
    for (const auto scope : {core::MagicTarget::Single, core::MagicTarget::All}) {
        const auto magic = spell(scope);
        data.magics[magic.id] = magic;
        const auto state = field(magic);
        std::vector<int> indices;
        const auto targets = game::BattleScene::buildTargetItems(save, state, cast(), indices);
        if (scope == core::MagicTarget::All) {
            ASSERT_EQ(indices.size(), 1u);
            ASSERT_EQ(targets.size(), 2u);
            EXPECT_EQ(indices[0], 1);
            EXPECT_EQ(targets[0].label, "敌方全体");
            EXPECT_TRUE(targets[0].enabled);
        } else {
            EXPECT_EQ(indices, (std::vector<int>{1, 2}));
            ASSERT_EQ(targets.size(), 3u);
            EXPECT_EQ(targets[0].label, "first");
            EXPECT_EQ(targets[1].label, "second");
        }
        std::vector<std::string> ids;
        const auto magics = game::BattleScene::buildMagicItems(data, save, state, 0, ids);
        ASSERT_EQ(ids.size(), 1u);
        EXPECT_EQ(magics[0].detail.find("敌方全体") != std::string::npos,
                  scope == core::MagicTarget::All);
    }
}

fs::path projectRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "scripts/common/api.lua")) return candidate;
    }
    return {};
}

void prepareMenuProbe(const TempDir& temp, bool heroPresent) {
    auto magic = magicJson();
    magic["target"] = "all";
    writeJson(temp.path() / "data/magics/probe.json", magic);
    writeJson(temp.path() / "data/items/probe_scroll.json",
              {{"id", "probe_scroll"}, {"name", "Probe scroll"}, {"kind", "artifact"},
               {"castMagic", "probe_magic"}});
    for (const std::string id : {"caster", "first", "second"}) {
        json role{{"id", id}, {"name", id}, {"maxHp", 1000}, {"maxMp", 100},
                  {"attack", 10}, {"defence", 5}, {"speed", id == "caster" ? 100 : 1},
                  {"realm", "QiRefining3"}};
        if (id == "caster") role["magics"] = json::array({"probe_magic"});
        writeJson(temp.path() / "data/roles" / (id + ".json"), role);
    }
    json units = json::array();
    if (!heroPresent) units.push_back({{"role_id", "caster"}, {"faction", "ally"}});
    units.push_back({{"role_id", "first"}, {"faction", "enemy"}});
    units.push_back({{"role_id", "second"}, {"faction", "enemy"}});
    writeJson(temp.path() / "data/battles/probe.json",
              {{"id", "probe_all"}, {"hero_absent", !heroPresent}, {"can_escape", false},
               {"units", units}});
    const auto root = projectRoot();
    ASSERT_FALSE(root.empty());
    fs::create_directories(temp.path() / "scripts/common");
    fs::copy_file(root / "scripts/common/api.lua", temp.path() / "scripts/common/api.lua");
}

TEST(Ch10AllMagicMenu, ASpellItemUsesTheSameAllScopeAndConsumesOnlyOneItem) {
    TempDir temp{"fanren_ch10_magic_item_menu"};
    prepareMenuProbe(temp, true);
    game::Application app;
    const auto ready = app.init(temp.path().string(), /*headless=*/true);
    ASSERT_TRUE(ready.ok) << ready.error;
    app.state().setFlag(game::kXiuxianKnownFlag);
    app.state().realm = app.state().realmCap = Realm::QiRefining3;
    app.state().hp = app.state().maxHp = 1000;
    app.state().mp = app.state().maxMp = 100;
    app.state().addItem("probe_scroll", 2);
    game::BattleScene scene("probe_all");
    scene.onEnter(app);
    ASSERT_EQ(scene.runToAllyTurn(), 0);
    scene.openMenu(app);
    ASSERT_TRUE(scene.menuChoose(app, game::kBattleMenuItem));
    ASSERT_EQ(scene.menuMode(), game::BattleMenuMode::Item);
    ASSERT_TRUE(scene.menuChoose(app, 0));
    ASSERT_EQ(scene.menuMode(), game::BattleMenuMode::Target);
    ASSERT_EQ(scene.menuList().items().size(), 2u);
    EXPECT_EQ(scene.menuList().items()[0].label, "敌方全体");
    const auto before = scene.battle().units();
    ASSERT_TRUE(scene.menuChoose(app, 0));
    EXPECT_EQ(app.state().itemCount("probe_scroll"), 1);
    EXPECT_EQ(scene.battle().units()[0].mp, before[0].mp);
    EXPECT_EQ(scene.battle().units()[0].bp, before[0].bp);
    EXPECT_LT(scene.battle().units()[1].hp, before[1].hp);
    EXPECT_LT(scene.battle().units()[2].hp, before[2].hp);
    EXPECT_EQ(eventsOf(scene.battle(), battle::BattleEventKind::Hit).size(), 2u);
    app.shutdown();
}

TEST(Ch10AllMagicMenu, ChoosingTheGroupInARealSceneHitsEveryoneAndPaysOnce) {
    TempDir temp{"fanren_ch10_magic_menu"};
    prepareMenuProbe(temp, false);
    game::Application app;
    const auto ready = app.init(temp.path().string(), /*headless=*/true);
    ASSERT_TRUE(ready.ok) << ready.error;
    app.state().setFlag(game::kXiuxianKnownFlag);
    game::BattleScene scene("probe_all");
    scene.onEnter(app);
    ASSERT_EQ(scene.battle().units().size(), 3u);
    ASSERT_EQ(scene.runToAllyTurn(), 0);
    scene.openMenu(app);
    ASSERT_TRUE(scene.menuChoose(app, game::kBattleMenuCast));
    ASSERT_EQ(scene.menuMode(), game::BattleMenuMode::Magic);
    ASSERT_TRUE(scene.menuList().items()[0].enabled);
    EXPECT_NE(scene.menuList().items()[0].detail.find("敌方全体"), std::string::npos);
    scene.setBoost(1);
    ASSERT_TRUE(scene.menuChoose(app, 0));
    ASSERT_EQ(scene.menuMode(), game::BattleMenuMode::Target);
    ASSERT_EQ(scene.menuList().items().size(), 2u);
    EXPECT_EQ(scene.menuList().items()[0].label, "敌方全体");
    const auto before = scene.battle().units();
    ASSERT_TRUE(scene.menuChoose(app, 0));
    EXPECT_EQ(scene.battle().units()[0].mp, before[0].mp - 7);
    EXPECT_EQ(scene.battle().units()[0].bp, before[0].bp - 1);
    EXPECT_LT(scene.battle().units()[1].hp, before[1].hp);
    EXPECT_LT(scene.battle().units()[2].hp, before[2].hp);
    EXPECT_NE(scene.feedback().find("横扫全场"), std::string::npos) << scene.feedback();
    EXPECT_EQ(eventsOf(scene.battle(), battle::BattleEventKind::Hit).size(), 2u);
    app.shutdown();
}

TEST(Ch10AllMagicContent, TheTwoContractFilesLoadAndActuallyHitAllOpponents) {
    const auto root = projectRoot();
    ASSERT_FALSE(root.empty());
    for (const auto& [file, id] : std::vector<std::pair<std::string, std::string>>{
             {"xuelian_ziyan.json", "magic_xuelian_ziyan"},
             {"ch10_wulang.json", "magic_ch10_wulang"}}) {
        TempDir temp{"fanren_ch10_actual_all_magic"};
        const auto source = root / "data/magics" / file;
        ASSERT_TRUE(fs::exists(source)) << source;
        fs::create_directories(temp.path() / "data/magics");
        fs::copy_file(source, temp.path() / "data/magics" / file);
        const auto loaded = fanren::io::loadGameData((temp.path() / "data").string());
        ASSERT_TRUE(loaded.ok) << loaded.error;
        const auto* magic = loaded.value.findMagic(id);
        ASSERT_NE(magic, nullptr);
        ASSERT_EQ(magic->target, core::MagicTarget::All);
        ASSERT_EQ(magic->effect, core::MagicEffect::None);
        ASSERT_GT(magic->power, 0);
        auto caster = unit("caster", id == "magic_xuelian_ziyan", 100);
        caster.realm = magic->needRealm;
        caster.mp = caster.maxMp = magic->needMp + 100;
        auto first = unit("first", !caster.ally);
        auto second = unit("second", !caster.ally);
        first.hp = first.maxHp = second.hp = second.maxHp = 100000;
        battle::BattleState state;
        state.setup({caster, first, second}, 7);
        state.addMagic(*magic);
        auto action = cast();
        action.magicId = id;
        ASSERT_TRUE(state.apply(action).ok) << id;
        EXPECT_EQ(state.units()[0].mp, 100);
        EXPECT_LT(state.units()[1].hp, first.hp) << id;
        EXPECT_LT(state.units()[2].hp, second.hp) << id;
        EXPECT_EQ(eventsOf(state, battle::BattleEventKind::Hit).size(), 2u) << id;
    }
}

}  // namespace
