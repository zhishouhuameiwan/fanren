// Chapter 11 E1–E5: docs/interfaces-p3-ch11.md sections 1.5, 2.2, 3.2, 4.2, 5.2.
// E1 (guaranteed recipe) is the only item that touched src/; E2–E5 pin existing engine
// behaviour the content track leans on. The ch11 data files these cases read have all
// landed, so absence is a failure, never a skip (the coordinator lifted the guards).
#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "ChapterFixture.h"
#include "TempDir.h"
#include "core/battle/Battle.h"
#include "core/model/AttackCategory.h"
#include "core/model/Types.h"
#include "core/rules/Bottle.h"
#include "core/rules/Calendar.h"
#include "core/rules/Crafting.h"
#include "core/rules/Cultivation.h"
#include "core/rules/Economy.h"
#include "core/rules/Field.h"
#include "core/rules/Realm.h"
#include "game/AlchemyScene.h"
#include "game/Application.h"
#include "game/CultivationScene.h"
#include "game/Scene.h"
#include "game/ShopScene.h"
#include "game/Wording.h"
#include "io/BattleLoader.h"
#include "io/DataLoader.h"
#include "io/RecipeLoader.h"
#include "io/SaveFile.h"
#include "io/ShopLoader.h"
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
using rules::CraftKind;
using rules::Realm;
using rules::Recipe;

constexpr const char* kRecipeId = "recipe_qingzhu_fengyun_jian";
constexpr const char* kSwordId = "weapon_qingzhu_fengyun_jian";
constexpr const char* kBambooId = "material_jinleizhu";
constexpr const char* kAuxId = "material_lianjian_fuliao";
constexpr const char* kStoneId = "material_lingshi";
constexpr const char* kForgeFlag = "ch11.liugen";
constexpr int kBambooCount = 6;
constexpr int kAuxCount = 1;
constexpr int kFixtureDay = 16003;
constexpr double kFrame = 1.0 / 60.0;

fs::path projectRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        const fs::path root(candidate);
        if (fs::exists(root / "maps/ch01_hanjiacun.tmj") && fs::exists(root / "data/recipes")) return root;
    }
    return {};
}

void writeFile(const fs::path& path, const std::string& content) {
    fs::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out << content;
}

// The 1.4 data shape, built by hand: the test never depends on the content track's file.
Recipe guaranteedSwordRecipe() {
    Recipe recipe;
    recipe.id = kRecipeId;
    recipe.name = "青竹蜂云剑图";
    recipe.kind = CraftKind::Forge;
    recipe.productId = kSwordId;
    recipe.productCount = 1;
    recipe.difficulty = 0;
    recipe.requiredProficiency = 0;
    recipe.requireFlag = kForgeFlag;
    recipe.guaranteed = true;
    recipe.inputs = {{kBambooId, kBambooCount}, {kAuxId, kAuxCount}};
    return recipe;
}

// Bag exactly as 1.5 item 2 says: six bamboo, one auxiliary, gate flag set.
GameState forgeReadyState() {
    GameState state;
    state.addItem(kBambooId, kBambooCount);
    state.addItem(kAuxId, kAuxCount);
    state.setFlag(kForgeFlag);
    state.forgeProficiency = 0;
    state.aptitude = 50;
    return state;
}

int countOf(const std::vector<rules::Ingredient>& consumed, const std::string& itemId) {
    int total = 0;
    for (const rules::Ingredient& gone : consumed) {
        if (gone.itemId == itemId) total += gone.count;
    }
    return total;
}

// ===========================================================================
// E1 1.5 items 1, 7: the rule returns the constant, never a literal, and never the clamp.
// ===========================================================================
TEST(Ch11GuaranteedRule, AGuaranteedRecipeReturnsTheConstantAndTheSameRecipeUngatedReturnsTheFloor) {
    Recipe recipe = guaranteedSwordRecipe();
    recipe.difficulty = 100;
    EXPECT_EQ(rules::successChance(recipe, 0, 0, 1), rules::kGuaranteedSuccessChance);
    // Clamp-independent: the early return sits before the clamp, so no input combination moves it.
    EXPECT_EQ(rules::successChance(recipe, 100, 100, 5), rules::kGuaranteedSuccessChance);
    EXPECT_EQ(rules::successChance(recipe, -7, 999, 0), rules::kGuaranteedSuccessChance);

    recipe.guaranteed = false;
    EXPECT_EQ(rules::successChance(recipe, 0, 0, 1), rules::kMinSuccessChance);
    EXPECT_EQ(rules::kMinSuccessChance, 5);
    EXPECT_GT(rules::kGuaranteedSuccessChance, rules::kMaxSuccessChance);
    static_assert(rules::kGuaranteedSuccessChance > rules::kMaxSuccessChance);
}

// 1.5 item 2: a thousand seeds, not one failure; product, consumption and gain are exact.
TEST(Ch11GuaranteedRule, EveryOneOfAThousandSeedsSucceedsAndConsumesExactlyTheInputs) {
    const Recipe recipe = guaranteedSwordRecipe();
    const GameState state = forgeReadyState();
    ASSERT_TRUE(rules::canCraft(recipe, state, 0, true).ok);
    for (std::uint32_t seed = 0; seed < 1000; ++seed) {
        const rules::CraftResult result = rules::craft(recipe, state, 0, 50, 3, seed);
        ASSERT_TRUE(result.success) << "seed " << seed << ": " << result.log;
        ASSERT_EQ(result.productId, kSwordId) << seed;
        ASSERT_EQ(result.productCount, 1) << seed;
        ASSERT_EQ(result.consumed.size(), 2u) << seed;
        ASSERT_EQ(result.consumed[0].itemId, kBambooId) << seed;
        ASSERT_EQ(result.consumed[0].count, kBambooCount) << seed;
        ASSERT_EQ(result.consumed[1].itemId, kAuxId) << seed;
        ASSERT_EQ(result.consumed[1].count, kAuxCount) << seed;
        ASSERT_EQ(result.proficiencyGain, 3) << seed;
    }
}

// 1.5 item 3: the control group. Same state, same recipe, guaranteed off: failure appears and
// costs three bamboo (ceil(6 × 50 / 100)) — the dead end 1.1 describes.
TEST(Ch11GuaranteedRule, TheSameRecipeWithoutGuaranteedFailsAtLeastOnceAndLosesHalfTheBamboo) {
    Recipe recipe = guaranteedSwordRecipe();
    recipe.guaranteed = false;
    const GameState state = forgeReadyState();
    ASSERT_LE(rules::successChance(recipe, 0, 50, 3), rules::kMaxSuccessChance);
    ASSERT_EQ(rules::failurePolicyOf(CraftKind::Forge), (rules::FailurePolicy{50, false}));

    int failures = 0;
    for (std::uint32_t seed = 0; seed < 1000; ++seed) {
        const rules::CraftResult result = rules::craft(recipe, state, 0, 50, 3, seed);
        if (result.success) continue;
        ++failures;
        EXPECT_TRUE(result.productId.empty()) << seed;
        EXPECT_EQ(countOf(result.consumed, kBambooId), 3) << seed;
        EXPECT_EQ(countOf(result.consumed, kAuxId), 1) << seed;
        EXPECT_EQ(result.proficiencyGain, rules::kProficiencyGainOnFailure) << seed;
    }
    EXPECT_GT(failures, 0) << "guaranteed=false 下一千颗种子全成：字段没有在起作用，或者有人把必成删了还全绿";
}

// ===========================================================================
// E1 1.5 item 4: the loader. Three contract cases plus the negative shapes that must be caught.
// ===========================================================================
TEST(Ch11GuaranteedData, TheLoaderRequiresABoolAndAGateFlagAndDefaultsToFalse) {
    const std::string head = R"({"id":"recipe_probe","name":"探针方","kind":"forge","productId":"weapon_probe",)"
                             R"("inputs":[{"itemId":"material_probe","count":1}])";
    TempDir tmp{"fanren_ch11_recipe"};
    const fs::path file = tmp.path() / "recipe_probe.json";

    // guaranteed without a gate: refused, and the message names the missing field.
    writeFile(file, head + R"(,"guaranteed":true})");
    const auto ungated = io::loadRecipe(file.string());
    EXPECT_FALSE(ungated.ok) << "没有门闸的必成方子会从第 7 章起就列在面板上";
    EXPECT_NE(ungated.error.find("requireFlag"), std::string::npos) << ungated.error;
    EXPECT_NE(ungated.error.find("recipe_probe.json"), std::string::npos) << ungated.error;

    // Wrong type: refused, names the field (same policy as "difficulty": "很难").
    for (const char* bad : {R"(,"requireFlag":"probe.flag","guaranteed":"yes"})",
                            R"(,"requireFlag":"probe.flag","guaranteed":1})",
                            R"(,"requireFlag":"probe.flag","guaranteed":null})"}) {
        writeFile(file, head + bad);
        const auto loaded = io::loadRecipe(file.string());
        EXPECT_FALSE(loaded.ok) << bad;
        EXPECT_NE(loaded.error.find("guaranteed"), std::string::npos) << bad << ": " << loaded.error;
    }

    // Absent: default false, existing recipes unchanged.
    writeFile(file, head + "}");
    const auto plain = io::loadRecipe(file.string());
    ASSERT_TRUE(plain.ok) << plain.error;
    EXPECT_FALSE(plain.value.guaranteed);

    // Explicit false without a gate is fine: the invariant binds only true.
    writeFile(file, head + R"(,"guaranteed":false})");
    const auto off = io::loadRecipe(file.string());
    ASSERT_TRUE(off.ok) << off.error;
    EXPECT_FALSE(off.value.guaranteed);

    // The sanctioned shape loads and keeps both fields.
    writeFile(file, head + R"(,"requireFlag":"probe.flag","guaranteed":true})");
    const auto gated = io::loadRecipe(file.string());
    ASSERT_TRUE(gated.ok) << gated.error;
    EXPECT_TRUE(gated.value.guaranteed);
    EXPECT_EQ(gated.value.requireFlag, "probe.flag");
}

// 1.5 item 5: real data. Only this one recipe may be guaranteed; an empty set is green too.
TEST(Ch11GuaranteedData, TheWholeRepositoryHasAtMostTheSwordRecipeGuaranteedAndItIsGatedForge) {
    const fs::path root = projectRoot();
    ASSERT_FALSE(root.empty());
    const auto data = io::loadGameData((root / "data").string());
    ASSERT_TRUE(data.ok) << data.error;
    const auto recipes = io::loadRecipes((root / "data/recipes").string(), data.value);
    ASSERT_TRUE(recipes.ok) << recipes.error;
    ASSERT_FALSE(recipes.value.empty());

    std::set<std::string> guaranteed;
    for (const auto& [id, recipe] : recipes.value) {
        if (!recipe.guaranteed) continue;
        guaranteed.insert(id);
        EXPECT_FALSE(recipe.requireFlag.empty()) << id;
        EXPECT_EQ(recipe.kind, CraftKind::Forge) << id;
    }
    const std::set<std::string> allowed{kRecipeId};
    EXPECT_TRUE(std::includes(allowed.begin(), allowed.end(), guaranteed.begin(), guaranteed.end()))
        << "guaranteed 的方子不止 " << kRecipeId << " 一张（共 " << guaranteed.size() << "）";
}

class Ch11ForgePanel : public ::testing::Test {
protected:
    void SetUp() override {
        const fs::path root = projectRoot();
        ASSERT_FALSE(root.empty());
        const auto ready = app_.init(root.string(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
    }
    void TearDown() override { app_.shutdown(); }
    Application app_;
};

// 1.5 item 6: the real panel on the real recipe. The content track's recipe carries guaranteed,
// so the detail row shows 100 and one press forges the sword without a dice roll.
TEST_F(Ch11ForgePanel, TheListShowsOneHundredAndOnePressForgesTheSwordConsumingExactlyTheInputs) {
    const auto found = app_.recipes().find(kRecipeId);
    ASSERT_NE(found, app_.recipes().end()) << "data/recipes/forge/qingzhu_fengyun_jian.json 没加载";
    ASSERT_TRUE(found->second.guaranteed) << "recipe_qingzhu_fengyun_jian 没写 guaranteed true";
    GameState state = forgeReadyState();
    state.addItem(kStoneId, 20);
    app_.state() = state;

    game::AlchemyScene scene(CraftKind::Forge, 3);
    scene.onEnter(app_);
    const auto& ids = scene.recipeIds();
    const auto at = std::find(ids.begin(), ids.end(), std::string(kRecipeId));
    ASSERT_NE(at, ids.end()) << "ch11.liugen 已置，炼器面板该列出这张方子";
    const int index = static_cast<int>(at - ids.begin());
    const auto rows = game::AlchemyScene::buildRecipeItems(
        app_.data(), app_.state(), CraftKind::Forge, 3,
        game::AlchemyScene::visibleRecipes(app_.recipes(), CraftKind::Forge, app_.state()));
    ASSERT_LT(static_cast<std::size_t>(index), rows.size());
    EXPECT_TRUE(rows[static_cast<std::size_t>(index)].enabled) << rows[static_cast<std::size_t>(index)].disabledReason;
    EXPECT_NE(rows[static_cast<std::size_t>(index)].detail.find("100"), std::string::npos)
        << rows[static_cast<std::size_t>(index)].detail;

    ASSERT_TRUE(scene.craftAt(app_, index)) << scene.feedback();
    EXPECT_EQ(app_.state().itemCount(kSwordId), 1) << scene.feedback();
    EXPECT_EQ(app_.state().itemCount(kBambooId), 0);
    EXPECT_EQ(app_.state().itemCount(kAuxId), 0);
    EXPECT_EQ(app_.state().itemCount(kStoneId), 20) << "1.4：inputs 不要灵石";
    EXPECT_EQ(app_.state().forgeProficiency, 3);
}

// ===========================================================================
// Shared fixture for E2 / E5: headless Application on tests/fixtures/ch10-end-first.sav, running a
// temporary .lua through startEvent (ScriptHost has no string entry; Ch02BottleTests pattern).
// ===========================================================================
class Ch11Fixture : public ::testing::Test {
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
        ASSERT_EQ(state().day, kFixtureDay) << "先验：第 10 章终局夹具的日期";
        ASSERT_EQ(state().realm, Realm::FoundationLate);
        ASSERT_EQ(state().realmCap, Realm::FoundationLate);
        ASSERT_EQ(state().formerRealm, Realm::FoundationMid);
        ASSERT_EQ(state().bottle.capacity, 6);

        // startEvent only takes paths relative to the host's scriptRoot, so the probes live in a
        // temporary scriptRoot of their own: the real common/api.lua plus probe_N.lua. Re-init keeps
        // the same host object (Application polls it) and the same GameState pointer.
        const fs::path scripts = tmp_.path() / "scripts";
        fs::create_directories(scripts / "common");
        fs::copy_file(root_ / "scripts/common/api.lua", scripts / "common/api.lua",
                      fs::copy_options::overwrite_existing);
        const auto rehosted = app_.scripts().init(scripts.string(), &app_.state());
        ASSERT_TRUE(rehosted.ok) << rehosted.error;
    }
    void TearDown() override { app_.shutdown(); }

    GameState& state() { return app_.state(); }

    void runLua(const std::string& body) {
        const std::string name = "probe_" + std::to_string(++scriptCount_) + ".lua";
        writeFile(tmp_.path() / "scripts" / name, body + "\n");
        const auto started = app_.startEvent(name);
        ASSERT_TRUE(started.ok) << started.error;
        for (int frame = 0; frame < 4000 && app_.scripts().isRunning(); ++frame) {
            app_.tick(kFrame);
            if (app_.awaitingCommand()) {
                fanren::script::CommandResult result;
                result.ok = true;
                result.choiceIndex = 0;
                app_.completeCommand(result);
            }
        }
        ASSERT_FALSE(app_.scripts().isRunning()) << body << " 没能跑到结束";
    }

    fs::path root_;
    TempDir tmp_{"fanren_ch11_engine"};
    int scriptCount_ = 0;
    Application app_;
};

// 2.2 items 1, 6: one call is clamped to 3650 silently; six calls of 3600 lose nothing; and the
// six-call run is far from a per-day loop.
TEST_F(Ch11Fixture, OneCallClampsToTheMaximumWhileSixCallsOfThreeThousandSixHundredAddUp) {
    ASSERT_EQ(game::kMaxScriptAdvanceDays, 3650);
    runLua("advance_days(21600)");
    EXPECT_EQ(state().day, kFixtureDay + game::kMaxScriptAdvanceDays) << "超出静默夹到 3650：分段是内容路的事";

    state().day = kFixtureDay;
    const auto began = std::chrono::steady_clock::now();
    runLua("advance_days(3600)\nadvance_days(3600)\nadvance_days(3600)\n"
           "advance_days(3600)\nadvance_days(3600)\nadvance_days(3600)");
    const auto spent = std::chrono::steady_clock::now() - began;
    EXPECT_EQ(state().day, kFixtureDay + 21600) << "六句加起来一天都不能少";
    EXPECT_LT(std::chrono::duration_cast<std::chrono::milliseconds>(spent).count(), 2000)
        << "growField / refill 退化成了逐日循环";
}

// 2.2 item 2: a one-year herb planted in the first of six slots ripens over the sixty years; the
// other five stay empty.
TEST_F(Ch11Fixture, AFieldOfSixSlotsRipensTheOnePlantedHerbAcrossSixtyYears) {
    runLua(R"(field.unlock("field_tianxing_39", 6))");
    rules::SpiritField* field = state().findField("field_tianxing_39");
    ASSERT_NE(field, nullptr);
    ASSERT_EQ(field->slots.size(), 6u);
    ASSERT_TRUE(rules::plant(*field, 0, "herb_huangjing_cao", state().day));
    ASSERT_EQ(rules::kDaysPerYear, 360);

    runLua("advance_days(3600)\nadvance_days(3600)\nadvance_days(3600)\n"
           "advance_days(3600)\nadvance_days(3600)\nadvance_days(3600)");
    field = state().findField("field_tianxing_39");
    ASSERT_NE(field, nullptr);
    EXPECT_TRUE(field->slots[0].ripe);
    EXPECT_GE(field->slots[0].age, rules::kRipeAge);
    for (std::size_t slot = 1; slot < 6; ++slot) {
        EXPECT_TRUE(field->slots[slot].seedId.empty()) << slot;
        EXPECT_FALSE(field->slots[slot].ripe) << slot;
    }
}

// 2.2 item 3: the bottle charges by realm — 3 days at Core (capacity stays 6 when the realm is
// written directly, not advanced), 5 days at Foundation.
TEST_F(Ch11Fixture, TheBottleChargesEveryThreeDaysAtCoreAndEveryFiveAtFoundation) {
    state().realm = Realm::CoreEarly;   // 直接改，不走 RealmAdvance：容量仍是夹具的 6
    state().bottle.drops = 0;
    state().bottle.lastChargeDay = state().day;
    runLua("advance_days(30)");
    EXPECT_EQ(state().bottle.capacity, 6);
    EXPECT_EQ(state().bottle.drops, std::min(6, 30 / 3)) << "结丹 3 日一滴，容量 6 封顶";

    state().realm = Realm::FoundationLate;
    state().bottle.drops = 0;
    state().bottle.lastChargeDay = state().day;
    runLua("advance_days(14)");
    EXPECT_EQ(state().bottle.drops, 2) << "筑基 5 日一滴";
}

// 2.2 item 4: rest heals to full across the years.
TEST_F(Ch11Fixture, SixtyYearsOfRestHealToFull) {
    state().hp = 1;
    runLua("advance_days(3600)");
    EXPECT_EQ(state().hp, state().maxHp);
}

// 2.2 item 5: today() stored in a flag survives as an integer; the difference after two pushes is 7200.
TEST_F(Ch11Fixture, TodayStoredInAFlagIsAnIntegerAndTheDifferenceReadsBackAsSevenThousandTwoHundred) {
    runLua(R"(flag.set("ch11.kaidian_ri", today()))");
    EXPECT_EQ(state().flag("ch11.kaidian_ri"), state().day);
    runLua("advance_days(3600)\nadvance_days(3600)");
    EXPECT_EQ(state().day - state().flag("ch11.kaidian_ri"), 7200);
    runLua(R"(flag.set("ch11.probe_cha", today() - flag.get("ch11.kaidian_ri")))");
    EXPECT_EQ(state().flag("ch11.probe_cha"), 7200) << "n29 补足算法在脚本里算的就是这个差";
}

// ===========================================================================
// E3 3.2: "trapped, not killed" is killable_by, built straight on BattleState.
// ===========================================================================
namespace battle = fanren::core::battle;

int categoryOf(const char* name) {
    const int bit = core::categoryFromName(name);
    EXPECT_TRUE(core::isSingleCategory(bit)) << name;
    return bit;
}

battle::Unit corpseHunter() {
    battle::Unit u;
    u.id = u.name = "hunter";
    u.hp = u.maxHp = 1000;
    u.mp = u.maxMp = 100;
    u.attack = 60;
    u.defence = 0;
    u.speed = 100;
    u.realm = Realm::CoreEarly;
    u.ally = true;
    u.weapons = categoryOf("剑") | categoryOf("水");
    return u;
}

battle::Unit corpse(int killableBy) {
    battle::Unit u;
    u.id = u.name = "tiandu_shi_probe";
    u.hp = u.maxHp = 10;
    u.attack = 1;
    u.defence = 0;
    u.speed = 1;
    u.realm = Realm::CoreEarly;
    u.ally = false;
    u.killableBy = killableBy;
    u.weaknesses = killableBy;
    return u;
}

// One strike of the given category from unit 0 on unit 1; returns the target afterwards.
const battle::Unit& strikeOnce(battle::BattleState& state, int category) {
    EXPECT_EQ(state.currentActor(), 0);
    battle::Action attack;
    attack.kind = battle::ActionKind::Attack;
    attack.actorIndex = 0;
    attack.targetIndex = 1;
    attack.category = category;
    std::string why;
    EXPECT_TRUE(state.isLegal(attack, &why)) << why;
    EXPECT_GE(state.estimateHitDamage(attack, 1), 10) << "先验：这一击本该打穿 10 点气血";
    const auto applied = state.apply(attack);
    EXPECT_TRUE(applied.ok) << applied.error;
    return state.units()[1];
}

// 3.2 item 1: a sword strike on a water/fire-only corpse leaves it at 1, alive, battle ongoing.
TEST(Ch11TrappedCorpse, ASwordStrikeStopsAtOneHitPointAndTheBattleGoesOn) {
    const int mask = categoryOf("水") | categoryOf("火");
    battle::BattleState state;
    state.setup({corpseHunter(), corpse(mask)}, 7u);
    const battle::Unit& target = strikeOnce(state, categoryOf("剑"));
    EXPECT_EQ(target.hp, 1);
    EXPECT_TRUE(target.alive());
    EXPECT_EQ(state.phase(), battle::BattlePhase::Ongoing);
}

// 3.2 item 2: the same strike with water kills, and with every foe down the state says Won.
TEST(Ch11TrappedCorpse, AWaterStrikeKillsAndTheBattleIsWon) {
    const int mask = categoryOf("水") | categoryOf("火");
    battle::BattleState state;
    state.setup({corpseHunter(), corpse(mask)}, 7u);
    const battle::Unit& target = strikeOnce(state, categoryOf("水"));
    EXPECT_EQ(target.hp, 0);
    EXPECT_FALSE(target.alive());
    EXPECT_EQ(state.phase(), battle::BattlePhase::Won);
}

// 3.2 item 3: killableBy 0 means anything kills — the control unit dies to the sword.
TEST(Ch11TrappedCorpse, AUnitWithoutKillableByDiesToTheSword) {
    battle::BattleState state;
    state.setup({corpseHunter(), corpse(0)}, 7u);
    const battle::Unit& target = strikeOnce(state, categoryOf("剑"));
    EXPECT_EQ(target.hp, 0);
    EXPECT_FALSE(target.alive());
    EXPECT_EQ(state.phase(), battle::BattlePhase::Won);
}

// 3.2 item 4: the real role and the real battle the content track landed.
TEST(Ch11TrappedCorpse, TheRealRoleIsKillableByWaterAndFireOnlyAndTheBattleHasFourAllies) {
    const fs::path root = projectRoot();
    ASSERT_FALSE(root.empty());
    const fs::path rolePath = root / "data/roles/tiandu_shi.json";
    const fs::path battlePath = root / "data/battles/b11_tiandu_shi.json";
    ASSERT_TRUE(fs::exists(rolePath)) << rolePath;
    ASSERT_TRUE(fs::exists(battlePath)) << battlePath;
    const auto data = io::loadGameData((root / "data").string());
    ASSERT_TRUE(data.ok) << data.error;
    const core::RoleTemplate* role = data.value.findRole("tiandu_shi");
    ASSERT_NE(role, nullptr);
    const int mask = categoryOf("水") | categoryOf("火");
    EXPECT_EQ(role->killableBy, mask) << core::categoryNames(role->killableBy);
    EXPECT_EQ(role->killableBy & ~role->weaknesses, 0) << "killable_by 必须 ⊆ weaknesses";

    const auto setup = io::loadBattle(battlePath.string());
    ASSERT_TRUE(setup.ok) << setup.error;
    EXPECT_EQ(setup.value.chapter, 11);
    EXPECT_TRUE(setup.value.defeatIsFatal);
    EXPECT_FALSE(setup.value.canEscape);
    int spiders = 0, puppets = 0;
    for (const core::BattleUnitSpec& unit : setup.value.units) {
        if (!unit.ally) continue;
        if (unit.roleId == "xueyu_zhizhu_ally") ++spiders;
        if (unit.roleId == "kuilei_yuan") ++puppets;
    }
    EXPECT_EQ(spiders, 2);
    EXPECT_EQ(puppets, 2);
}

// ===========================================================================
// E4 4.2: buy-only shops (empty entries) and the sell price rounding.
// ===========================================================================
// 4.2 item 1: the loader accepts both empty spellings and rejects sellRate == buyRate.
TEST(Ch11BuyOnlyShopData, EmptyEntriesLoadEitherWayAndAnEqualSellRateIsRefused) {
    TempDir tmp{"fanren_ch11_shop"};
    const fs::path file = tmp.path() / "probe_shop.json";

    writeFile(file, R"({"id":"probe_shougou","nameKey":"shop.probe","buyRate":1.0,"sellRate":0.8})");
    const auto absent = io::loadShop(file.string());
    ASSERT_TRUE(absent.ok) << absent.error;
    EXPECT_TRUE(absent.value.entries.empty());
    EXPECT_DOUBLE_EQ(absent.value.sellRate, 0.8);

    writeFile(file, R"({"id":"probe_shougou","nameKey":"shop.probe","buyRate":1.0,"sellRate":1.0})");
    const auto equal = io::loadShop(file.string());
    EXPECT_FALSE(equal.ok) << "买进再卖回即可刷钱";
    EXPECT_NE(equal.error.find("sellRate"), std::string::npos) << equal.error;

    writeFile(file, R"({"id":"probe_shougou","nameKey":"shop.probe","buyRate":1.0,"sellRate":0.6,"entries":[]})");
    const auto empty = io::loadShop(file.string());
    ASSERT_TRUE(empty.ok) << empty.error;
    EXPECT_TRUE(empty.value.entries.empty());

    // Negative shape: entries that is not an array must be refused, not read as empty.
    writeFile(file, R"({"id":"probe_shougou","nameKey":"shop.probe","buyRate":1.0,"sellRate":0.6,"entries":3})");
    const auto wrong = io::loadShop(file.string());
    EXPECT_FALSE(wrong.ok);
    EXPECT_NE(wrong.error.find("entries"), std::string::npos) << wrong.error;
}

// 4.2 item 2: literal prices, rounding to nearest (the contract fixes the direction).
TEST(Ch11BuyOnlyShopRule, SellPricesAreTheContractLiteralsRoundedToNearest) {
    rules::Shop shop;
    shop.id = "probe_shougou";
    shop.buyRate = 1.0;
    core::Item item;
    item.id = "probe_dan";
    item.kind = core::ItemKind::Material;

    shop.sellRate = 0.8;
    item.price = 950;
    EXPECT_EQ(rules::sellPrice(shop, item, 0), 760);
    item.price = 6000;
    EXPECT_EQ(rules::sellPrice(shop, item, 0), 4800);
    item.price = 957;
    EXPECT_EQ(rules::sellPrice(shop, item, 0), 766) << "765.6 四舍五入";
    item.price = 951;
    EXPECT_EQ(rules::sellPrice(shop, item, 0), 761) << "760.8 四舍五入";

    shop.sellRate = 0.6;
    item.price = 10;
    EXPECT_EQ(rules::sellPrice(shop, item, 0), 6);
}

// 4.2 item 3: restock on an empty shelf is a no-op.
TEST(Ch11BuyOnlyShopRule, RestockLeavesAnEmptyShelfEmpty) {
    rules::Shop shop;
    shop.id = "probe_shougou";
    shop.sellRate = 0.8;
    rules::restock(shop, 1);
    rules::restock(shop, 40000);
    EXPECT_TRUE(shop.entries.empty());
}

// 4.2 item 4: the panel's two static builders on an empty shop. The buy page is only「返回」; the
// sell page lists the one tradeable item and hides the story item and the tradeable=false one.
// (ShopScene keeps its main ListView private; the「买入」row's enabled/disabledReason is read by
// no accessor headlessly — see the implementation record in the contract.)
TEST_F(Ch11ForgePanel, AnEmptyShopHasNoBuyRowsAndTheSellPageListsOnlyTradeableGoods) {
    constexpr const char* kDanId = "material_yaodan_wuji";
    constexpr const char* kMapId = "story_waihai_tu";
    constexpr const char* kThunderId = "material_tianleizhu";
    for (const char* id : {kDanId, kMapId, kThunderId}) {
        ASSERT_NE(app_.data().findItem(id), nullptr) << "data/items 里没有 " << id;
    }
    ASSERT_TRUE(app_.data().findItem(kDanId)->tradeable);
    ASSERT_FALSE(app_.data().findItem(kThunderId)->tradeable);

    rules::Shop shop;
    shop.id = "ch11_probe_empty";
    shop.buyRate = 1.0;
    shop.sellRate = 0.8;
    GameState state;
    state.addItem(kDanId, 1);
    state.addItem(kMapId, 1);
    state.addItem(kThunderId, 1);

    const auto buyRows = game::ShopScene::buildBuyItems(app_.data(), state, shop);
    ASSERT_EQ(buyRows.size(), 1u) << "空货架只剩「返回」";
    EXPECT_EQ(buyRows[0].label, "返回");

    const auto stacks = game::ShopScene::buildSellStacks(app_.data(), state, shop);
    ASSERT_EQ(stacks.size(), 1u);
    EXPECT_EQ(stacks[0].itemId, kDanId);
    EXPECT_EQ(stacks[0].unitPrice, rules::sellPrice(shop, *app_.data().findItem(kDanId), 0));
    const auto sellRows = game::ShopScene::buildSellItems(game::PanelStage::Immortal, app_.data(), stacks);
    EXPECT_EQ(sellRows.size(), 2u) << "恰 1 行 + 返回";
}

// 4.2 item 5: the four real shops the content track landed.
TEST(Ch11BuyOnlyShopData, TheFourRealShopsLoadAndTheTwoBuyOnlyOnesAreEmptyAtTheirRates) {
    const fs::path root = projectRoot();
    ASSERT_FALSE(root.empty());
    const fs::path shops = root / "data/shops";
    for (const char* id : {"ch11_fangshi_zahuo", "ch11_ningcui_zahuo", "ch11_ningcui_shougou", "ch11_qingzhu_xiaoxuan"}) {
        ASSERT_TRUE(fs::exists(shops / (std::string(id) + ".json"))) << "data/shops/" << id << ".json 不存在";
    }
    const auto shougou = io::loadShop((shops / "ch11_ningcui_shougou.json").string());
    ASSERT_TRUE(shougou.ok) << shougou.error;
    EXPECT_TRUE(shougou.value.entries.empty());
    EXPECT_DOUBLE_EQ(shougou.value.sellRate, 0.8);
    const auto xiaoxuan = io::loadShop((shops / "ch11_qingzhu_xiaoxuan.json").string());
    ASSERT_TRUE(xiaoxuan.ok) << xiaoxuan.error;
    EXPECT_TRUE(xiaoxuan.value.entries.empty());
    EXPECT_DOUBLE_EQ(xiaoxuan.value.sellRate, 0.6);
    const auto ningcui = io::loadShop((shops / "ch11_ningcui_zahuo.json").string());
    ASSERT_TRUE(ningcui.ok) << ningcui.error;
    const auto fangshi = io::loadShop((shops / "ch11_fangshi_zahuo.json").string());
    ASSERT_TRUE(fangshi.ok) << fangshi.error;
    EXPECT_TRUE(std::any_of(fangshi.value.entries.begin(), fangshi.value.entries.end(),
                            [](const rules::ShopEntry& e) { return e.itemId == "herb_nichang_cao"; }));
}

// ===========================================================================
// E5 5.2: Core-early numbers — story advance, StoryCap, bottle, 125 room, gated forge.
// ===========================================================================
MapObject facilityObject(const std::string& name, std::initializer_list<std::pair<const char*, const char*>> props) {
    MapObject object;
    object.name = name;
    object.type = "facility";
    for (const auto& [key, value] : props) object.properties.emplace(key, value);
    return object;
}

// 5.2 item 1: realm.advance(CORE_EARLY) lifts 23 → 31 with the cap, lifts hp by the delta
// (not to full), keeps formerRealm, and raises the bottle floor to 9 without adding drops.
TEST_F(Ch11Fixture, StoryAdvanceToCoreEarlyLiftsCapVitalsAndBottleFloorWithoutRefilling) {
    const int oldMax = state().maxHp;
    state().hp = oldMax / 2;
    const int halfHp = state().hp;
    const int drops = state().bottle.drops;
    runLua("local ok = realm.advance(realm.CORE_EARLY)\nflag.set(\"ch11.probe_ok\", ok and 1 or 0)");
    EXPECT_EQ(state().flag("ch11.probe_ok"), 1);
    EXPECT_EQ(state().realm, Realm::CoreEarly);
    EXPECT_EQ(rules::toValue(state().realm), 31);
    EXPECT_EQ(state().realmCap, Realm::CoreEarly);
    EXPECT_EQ(state().formerRealm, Realm::FoundationMid) << "formerRealm 仍 22";
    EXPECT_EQ(state().maxHp, rules::realmMaxHp(Realm::CoreEarly));
    EXPECT_GT(state().maxHp, oldMax);
    EXPECT_EQ(state().hp, halfHp + (state().maxHp - oldMax)) << "liftToFloor：抬差额，不补满";
    EXPECT_EQ(state().bottle.capacity, 9) << "max(6, bottleCapacityFloor(Core))";
    EXPECT_EQ(rules::bottleCapacityFloor(rules::RealmTier::Core), 9);
    EXPECT_EQ(state().bottle.drops, drops) << "抬上限不送液";
}

// 5.2 item 2: at 31/31 with 5000 cultivation the next step (Core mid) is StoryCap; the panel row is off.
TEST_F(Ch11Fixture, CoreMidIsBlockedByStoryCapOnTheRuleAndOnThePanel) {
    runLua("realm.advance(realm.CORE_EARLY)");
    ASSERT_EQ(state().realm, Realm::CoreEarly);
    state().cultivation = 5000;
    EXPECT_EQ(rules::breakthroughBlock(state().realm, state().cultivation, state().realmCap),
              rules::BreakthroughBlock::StoryCap);
    const auto rows = game::CultivationScene::buildMainItems(state());
    ASSERT_GE(rows.size(), 2u);
    EXPECT_EQ(rows[1].label, game::cultivationLexicon(game::PanelStage::Immortal).pushLabel);
    EXPECT_FALSE(rows[1].enabled) << rows[1].detail;
    EXPECT_FALSE(rows[1].disabledReason.empty());
}

// 5.2 item 3: before n16 the panel is blocked at 23 as well.
TEST(Ch11CoreEarlyRule, FoundationLateAtItsCapIsStoryCapEvenWithCultivationToSpare) {
    EXPECT_EQ(rules::breakthroughBlock(Realm::FoundationLate, 3500, Realm::FoundationLate),
              rules::BreakthroughBlock::StoryCap);
}

// 5.2 item 4: the charge-days function itself.
TEST(Ch11CoreEarlyRule, BottleChargeDaysAreThreeAtCoreAndFiveAtFoundation) {
    EXPECT_EQ(game::bottleChargeDays(Realm::CoreEarly), 3);
    EXPECT_EQ(game::bottleChargeDays(Realm::FoundationLate), 5);
}

// 5.2 item 5: a 125 room opens a panel that says so, and pays five quarters of a 100 cushion.
TEST_F(Ch11Fixture, TheOneHundredTwentyFiveRoomOpensAPanelWithTheBonusAndPaysFiveQuarters) {
    const MapObject room = facilityObject("facility_jingshi_39", {{"kind", "meditate"}, {"effectiveness", "125"}});
    ASSERT_EQ(room.property("effectiveness"), "125");
    app_.openFacility(room);
    app_.tick(kFrame);
    game::Scene* top = app_.topScene();
    ASSERT_NE(top, nullptr);
    ASSERT_EQ(top->name(), "Cultivation");
    auto* panel = dynamic_cast<game::CultivationScene*>(top);
    ASSERT_NE(panel, nullptr);
    // CultivationScene has no sitePercent() accessor; the status line carries the 25 it adds.
    EXPECT_NE(panel->siteBonusLine(state()).find("25"), std::string::npos) << panel->siteBonusLine(state());
    app_.popScene();
    app_.tick(kFrame);

    GameState base;
    base.realm = Realm::CoreEarly;
    base.aptitude = 50;
    base.cultivation = 0;
    base.cultivationRemainder = 0;
    base.day = 1;
    base.setFlag(game::kXiuxianKnownFlag);
    for (std::uint32_t seed = 1; seed <= 8; ++seed) {
        GameState room125 = base;
        GameState cushion = base;
        const int y = game::CultivationScene::bankMeditation(room125, 60, seed, 125).cultivation;
        const int x = game::CultivationScene::bankMeditation(cushion, 60, seed, 100).cultivation;
        EXPECT_GT(x, 0) << seed;
        EXPECT_GE(y * 4, x * 5 - 4) << "seed " << seed << ": " << y << " vs " << x;
        EXPECT_LE(y * 4, x * 5 + 4) << "seed " << seed << ": " << y << " vs " << x;
    }
}

// 5.2 item 6: the forge is gated by ch11.liugen — a dialogue before, the Forge panel at grade 3 after.
TEST_F(Ch11Fixture, TheForgeShowsTheLockedMessageUntilLiugenAndThenOpensAtGradeThree) {
    const MapObject forge = facilityObject("facility_lianqi_39",
                                          {{"kind", "forge"}, {"grade", "3"}, {"require_flag", kForgeFlag}});
    ASSERT_EQ(state().flag(kForgeFlag), 0) << "先验：夹具里 ch11.liugen 未置";
    app_.openFacility(forge);
    app_.tick(kFrame);
    game::Scene* locked = app_.topScene();
    ASSERT_NE(locked, nullptr);
    EXPECT_EQ(locked->name(), "Dialogue") << "未置时是 ui.facility.locked 的对话框，不是面板也不是行走";
    app_.popScene();
    app_.tick(kFrame);

    state().setFlag(kForgeFlag);
    app_.openFacility(forge);
    app_.tick(kFrame);
    game::Scene* open = app_.topScene();
    ASSERT_NE(open, nullptr);
    ASSERT_EQ(open->name(), "Alchemy");
    auto* panel = dynamic_cast<game::AlchemyScene*>(open);
    ASSERT_NE(panel, nullptr);
    EXPECT_EQ(panel->kind(), CraftKind::Forge);
    EXPECT_EQ(panel->toolGrade(), 3);
    EXPECT_TRUE(rules::hasToolOfGrade(panel->toolGrade()));
    app_.popScene();
    app_.tick(kFrame);
}

// 5.2 item 5 (real map): the two facilities on the ch11_dongfu the content track landed.
TEST(Ch11CoreEarlyData, TheRealDongfuMapCarriesTheRoomAt125AndTheGatedForgeAtGradeThree) {
    const fs::path root = projectRoot();
    ASSERT_FALSE(root.empty());
    const fs::path tmj = root / "maps/ch11_dongfu.tmj";
    ASSERT_TRUE(fs::exists(tmj)) << tmj;
    const auto map = io::loadTileMap(tmj.string());
    ASSERT_TRUE(map.ok) << map.error;
    const auto find = [&](const char* name) -> const MapObject* {
        const auto it = std::find_if(map.value.objects.begin(), map.value.objects.end(),
                                     [&](const MapObject& o) { return o.name == name; });
        return it == map.value.objects.end() ? nullptr : &*it;
    };
    const MapObject* room = find("facility_jingshi_39");
    ASSERT_NE(room, nullptr);
    EXPECT_EQ(room->property("kind"), "meditate");
    EXPECT_EQ(room->property("effectiveness"), "125");
    const MapObject* forge = find("facility_lianqi_39");
    ASSERT_NE(forge, nullptr);
    EXPECT_EQ(forge->property("kind"), "forge");
    EXPECT_EQ(forge->property("grade"), "3") << "grade 0 会被 hasToolOfGrade 挡成「未曾引来地火」";
    EXPECT_EQ(forge->property("require_flag"), kForgeFlag);
}

// 5.2 item 7: the engine does not police the target — CORE_MID goes through; the script scan
// (test track) is what pins "exactly one realm.advance, target 31".
TEST_F(Ch11Fixture, TheEngineLetsAScriptAdvanceToCoreMidSoTheScriptScanMustPinThirtyOne) {
    runLua("realm.advance(realm.CORE_MID)");
    EXPECT_EQ(state().realm, Realm::CoreMid);
    EXPECT_EQ(rules::toValue(state().realm), 32);
}

}  // namespace
