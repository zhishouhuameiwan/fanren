#include "core/rules/Crafting.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <map>
#include <random>
#include <string>
#include <utility>
#include <vector>

namespace {

using fanren::core::BagEntry;
using fanren::core::GameState;
using fanren::rules::canCraft;
using fanren::rules::craft;
using fanren::rules::CraftKind;
using fanren::rules::CraftResult;
using fanren::rules::FailurePolicy;
using fanren::rules::failurePolicyOf;
using fanren::rules::Ingredient;
using fanren::rules::proficiencyGain;
using fanren::rules::Recipe;
using fanren::rules::successChance;

namespace rules = fanren::rules;

// ---- 测试夹具 ----

Ingredient herb(const std::string& id, int count, int minAge) {
    Ingredient ing;
    ing.itemId = id;
    ing.count = count;
    ing.minAge = minAge;
    return ing;
}

Ingredient part(const std::string& id, int count) {
    Ingredient ing;
    ing.itemId = id;
    ing.count = count;
    return ing;
}

// 「引」：妖丹、阵旗之类，入不了火，失败时能捡回来。
Ingredient catalystPart(const std::string& id, int count) {
    Ingredient ing = part(id, count);
    ing.catalyst = true;
    return ing;
}

// 四张方子各取一张，数值贴 data/recipes/ 里的同名条目。
Recipe alchemyRecipe() {
    Recipe r;
    r.id = "recipe_jinchuang_yao";
    r.name = "金疮药方";
    r.kind = CraftKind::Alchemy;
    r.productId = "pill_jinchuang_yao";
    r.productCount = 1;
    r.difficulty = 5;
    r.requiredProficiency = 0;
    r.inputs = {herb("herb_qingfeng_cao", 2, 10)};
    return r;
}

Recipe talismanRecipe() {
    Recipe r;
    r.id = "recipe_hushen_fu";
    r.name = "护身符方";
    r.kind = CraftKind::Talisman;
    r.productId = "talisman_hushen_fu";
    r.productCount = 1;
    r.difficulty = 15;
    r.requiredProficiency = 3;
    r.inputs = {part("material_fuzhi", 2), part("material_lingmo", 1),
                catalystPart("material_yaodan_1", 1)};
    return r;
}

Recipe forgeRecipe() {
    Recipe r;
    r.id = "recipe_qingfeng_jian";
    r.name = "青锋剑图";
    r.kind = CraftKind::Forge;
    r.productId = "weapon_qingfeng_jian";
    r.productCount = 1;
    r.difficulty = 32;
    r.requiredProficiency = 16;
    r.inputs = {part("material_jingtie", 3), part("material_xuantie", 1)};
    return r;
}

Recipe formationRecipe() {
    Recipe r;
    r.id = "recipe_juling_zhen";
    r.name = "聚灵阵图";
    r.kind = CraftKind::Formation;
    r.productId = "formation_juling_zhen";
    r.productCount = 1;
    r.difficulty = 25;
    r.requiredProficiency = 8;
    r.inputs = {catalystPart("material_zhenqi", 4), catalystPart("material_zhenpan_pei", 1),
                part("material_lingshi", 2)};
    return r;
}

std::vector<Recipe> allFourArts() {
    return {alchemyRecipe(), talismanRecipe(), forgeRecipe(), formationRecipe()};
}

// 备料充足的工坊。灵草一律按方子的门槛年份进货。
GameState stockedFor(const Recipe& recipe, int multiplier = 3) {
    GameState state;
    for (const Ingredient& need : recipe.inputs) {
        state.addItem(need.itemId, need.count * multiplier, need.minAge);
    }
    return state;
}

GameState bagOf(std::vector<BagEntry> entries) {
    GameState state;
    state.bag = std::move(entries);
    return state;
}

// 熟练度一律按方子门槛再加一点，免得测别的东西时被熟练度闸绊住。
int comfortableProficiency(const Recipe& recipe) { return recipe.requiredProficiency + 10; }

constexpr int kAptitude = 50;
constexpr int kToolGrade = 2;

CraftResult craftWith(const Recipe& recipe, const GameState& state, std::uint32_t seed) {
    return craft(recipe, state, comfortableProficiency(recipe), kAptitude, kToolGrade, seed);
}

// 找一颗能产出指定结果的种子。成功率永远夹在 [5, 95]，两种结果都必然存在；
// 找不到就说明夹紧坏了，直接让用例挂掉，而不是悄悄跳过。
std::uint32_t seedWhere(const Recipe& recipe, const GameState& state, bool wantSuccess) {
    for (std::uint32_t seed = 1; seed < 20000; ++seed) {
        if (craftWith(recipe, state, seed).success == wantSuccess) return seed;
    }
    ADD_FAILURE() << "找不到能产出指定结果的种子：成功率夹紧坏了？";
    return 0;
}

// 用固定种子的发生器产出一串种子：整条统计结果依然可复现，
// 又不像 0,1,2,… 那样在 mt19937 的首次输出上带相关性。
std::vector<std::uint32_t> spreadSeeds(int count, std::uint32_t seed) {
    std::mt19937 gen(seed);
    std::vector<std::uint32_t> seeds;
    seeds.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) seeds.push_back(gen());
    return seeds;
}

int consumedCountOf(const CraftResult& result, const std::string& itemId) {
    int total = 0;
    for (const Ingredient& gone : result.consumed) {
        if (gone.itemId == itemId) total += gone.count;
    }
    return total;
}

// 逐字段快照比对：被拒的炼制之后，这两个状态必须一模一样。
void expectStateIdentical(const GameState& before, const GameState& after) {
    ASSERT_EQ(before.bag.size(), after.bag.size());
    for (std::size_t i = 0; i < before.bag.size(); ++i) {
        EXPECT_EQ(before.bag[i].itemId, after.bag[i].itemId) << "bag " << i;
        EXPECT_EQ(before.bag[i].count, after.bag[i].count) << "bag " << i;
        EXPECT_EQ(before.bag[i].herbAge, after.bag[i].herbAge) << "bag " << i;
    }
    EXPECT_EQ(before.mapId, after.mapId);
    EXPECT_EQ(before.position.x, after.position.x);
    EXPECT_EQ(before.position.y, after.position.y);
    EXPECT_EQ(before.facing, after.facing);
    EXPECT_EQ(before.realm, after.realm);
    EXPECT_EQ(before.cultivation, after.cultivation);
    EXPECT_EQ(before.hp, after.hp);
    EXPECT_EQ(before.maxHp, after.maxHp);
    EXPECT_EQ(before.mp, after.mp);
    EXPECT_EQ(before.maxMp, after.maxMp);
    EXPECT_EQ(before.day, after.day);
    EXPECT_EQ(before.chapter, after.chapter);
    EXPECT_EQ(before.flags, after.flags);
    EXPECT_EQ(before.playSecondsGameplay, after.playSecondsGameplay);
    EXPECT_EQ(before.playSecondsSystem, after.playSecondsSystem);
}

void expectCraftResultIdentical(const CraftResult& a, const CraftResult& b) {
    EXPECT_EQ(a.success, b.success);
    EXPECT_EQ(a.productId, b.productId);
    EXPECT_EQ(a.productCount, b.productCount);
    EXPECT_EQ(a.proficiencyGain, b.proficiencyGain);
    EXPECT_EQ(a.log, b.log);
    ASSERT_EQ(a.consumed.size(), b.consumed.size());
    for (std::size_t i = 0; i < a.consumed.size(); ++i) {
        EXPECT_EQ(a.consumed[i].itemId, b.consumed[i].itemId) << "consumed " << i;
        EXPECT_EQ(a.consumed[i].count, b.consumed[i].count) << "consumed " << i;
        EXPECT_EQ(a.consumed[i].minAge, b.consumed[i].minAge) << "consumed " << i;
        EXPECT_EQ(a.consumed[i].herbAge, b.consumed[i].herbAge) << "consumed " << i;
        EXPECT_EQ(a.consumed[i].catalyst, b.consumed[i].catalyst) << "consumed " << i;
    }
}

// ---- 1. 成功率公式与夹紧 ----

TEST(CraftingChance, FollowsTheContractFormula) {
    Recipe recipe = alchemyRecipe();
    recipe.difficulty = 30;
    // 40（底数）+ 40/2 + 50/5 + 2×5 − 30 = 50
    EXPECT_EQ(successChance(recipe, 40, 50, 2), 50);
    // 熟练度折半计入：多 20 点熟练度只折 10 点成功率。
    EXPECT_EQ(successChance(recipe, 60, 50, 2), 60);
    // 炉鼎每品阶 5 点。
    EXPECT_EQ(successChance(recipe, 40, 50, 4), 60);
}

TEST(CraftingChance, NeverDropsBelowFivePercent) {
    Recipe recipe = alchemyRecipe();
    recipe.difficulty = 100;
    EXPECT_EQ(successChance(recipe, 0, 0, 0), rules::kMinSuccessChance);
    // 哪怕再离谱的负向输入，也仍旧留 5% 的侥幸。
    EXPECT_EQ(successChance(recipe, 0, 0, 1), rules::kMinSuccessChance);
}

TEST(CraftingChance, NeverRisesAboveNinetyFivePercent) {
    Recipe recipe = alchemyRecipe();
    recipe.difficulty = 0;
    EXPECT_EQ(successChance(recipe, 100, 100, 5), rules::kMaxSuccessChance);
    EXPECT_EQ(successChance(recipe, 100, 100, 99), rules::kMaxSuccessChance);
}

TEST(CraftingChance, NegativeInputsCountAsZero) {
    Recipe recipe = alchemyRecipe();
    recipe.difficulty = 20;
    EXPECT_EQ(successChance(recipe, -50, -80, -3), successChance(recipe, 0, 0, 0));
}

TEST(CraftingChance, ProficiencyAptitudeAndToolAllMoveTheNeedle) {
    Recipe recipe = alchemyRecipe();
    recipe.difficulty = 50;
    const int base = successChance(recipe, 20, 20, 1);
    EXPECT_GT(successChance(recipe, 40, 20, 1), base);
    EXPECT_GT(successChance(recipe, 20, 40, 1), base);
    EXPECT_GT(successChance(recipe, 20, 20, 2), base);
    // 难方子压成功率，这是四艺唯一的难度旋钮。
    Recipe harder = recipe;
    harder.difficulty = 70;
    EXPECT_LT(successChance(harder, 20, 20, 1), base);
}

TEST(CraftingChance, ExtremeInputsStayInsideTheClamp) {
    // 只夹下界是不对称的防御：上界缺席时，脏数据灌进来的巨值会让夹紧之前的乘加
    // 先溢出，clamp 拿到垃圾反而跳出 [5, 95]——而那是本模块的硬要求。
    // 这里把四个入参各自取遍极端值，逐一确认返回值仍在闸内。
    constexpr int kIntMin = std::numeric_limits<int>::min();
    constexpr int kIntMax = std::numeric_limits<int>::max();
    const int extremes[] = {kIntMin, kIntMin + 1, -1000000, -1, 0, 1, 1000000, kIntMax - 1, kIntMax};

    Recipe recipe = alchemyRecipe();
    for (int difficulty : extremes) {
        recipe.difficulty = difficulty;
        for (int prof : extremes) {
            for (int apt : extremes) {
                for (int grade : extremes) {
                    const int chance = successChance(recipe, prof, apt, grade);
                    ASSERT_GE(chance, rules::kMinSuccessChance)
                        << difficulty << '/' << prof << '/' << apt << '/' << grade;
                    ASSERT_LE(chance, rules::kMaxSuccessChance)
                        << difficulty << '/' << prof << '/' << apt << '/' << grade;
                }
            }
        }
    }
}

TEST(CraftingChance, InputsAreCappedAtTheirDesignRange) {
    Recipe recipe = alchemyRecipe();
    recipe.difficulty = 0;
    // 超出量程的入参与量程上限等效：一件品阶被写坏的炉鼎不该把难方子顶到 95%。
    EXPECT_EQ(successChance(recipe, 10, 10, rules::kMaxToolGrade + 50),
              successChance(recipe, 10, 10, rules::kMaxToolGrade));
    EXPECT_EQ(successChance(recipe, rules::kMaxProficiency + 500, 10, 1),
              successChance(recipe, rules::kMaxProficiency, 10, 1));
    EXPECT_EQ(successChance(recipe, 10, rules::kMaxAptitude + 500, 1),
              successChance(recipe, 10, rules::kMaxAptitude, 1));

    // 挑一个结果落在闸内的难度，免得两边都被 5% 地板吃掉、看不出上界有没有生效：
    // 40 + 5×5 − 50 = 15，若品阶不封顶，这里会被顶到 95。
    Recipe hard = alchemyRecipe();
    hard.difficulty = 50;
    EXPECT_EQ(successChance(hard, 0, 0, rules::kMaxToolGrade), 15);
    EXPECT_EQ(successChance(hard, 0, 0, 100000), 15);
}

TEST(CraftingChance, HopelessRecipeStillWinsAboutFivePercent) {
    Recipe recipe = alchemyRecipe();
    recipe.difficulty = 100;
    recipe.requiredProficiency = 0;
    const GameState state = stockedFor(recipe, 1);
    ASSERT_EQ(successChance(recipe, 0, 0, 1), rules::kMinSuccessChance);

    int wins = 0;
    const std::vector<std::uint32_t> seeds = spreadSeeds(2000, 20260920);
    for (std::uint32_t seed : seeds) {
        if (craft(recipe, state, 0, 0, 1, seed).success) ++wins;
    }
    EXPECT_GT(wins, 0) << "难到极处也该留一线侥幸";
    EXPECT_GE(wins, 20);
    EXPECT_LE(wins, 240);
}

TEST(CraftingChance, SureThingStillFailsAboutFivePercent) {
    Recipe recipe = alchemyRecipe();
    recipe.difficulty = 0;
    recipe.requiredProficiency = 0;
    const GameState state = stockedFor(recipe, 1);
    ASSERT_EQ(successChance(recipe, 100, 100, 5), rules::kMaxSuccessChance);

    int losses = 0;
    const std::vector<std::uint32_t> seeds = spreadSeeds(2000, 19990101);
    for (std::uint32_t seed : seeds) {
        if (!craft(recipe, state, 100, 100, 5, seed).success) ++losses;
    }
    EXPECT_GT(losses, 0) << "再稳的手也该有失手的一天";
    EXPECT_GE(losses, 20);
    EXPECT_LE(losses, 240);
}

// ---- 2. canCraft 的每条失败路径 ----

TEST(CraftingGate, AcceptsAStockedWorkshop) {
    for (const Recipe& recipe : allFourArts()) {
        const GameState state = stockedFor(recipe);
        const auto gate = canCraft(recipe, state, comfortableProficiency(recipe), true);
        EXPECT_TRUE(gate.ok) << recipe.name << ' ' << gate.error;
        EXPECT_TRUE(gate.error.empty());
        EXPECT_TRUE(gate.value.empty());
    }
}

TEST(CraftingGate, RejectsWhenToolMissing) {
    // 四艺缺的不是同一件器具，提示语也不该是同一句。
    std::vector<std::string> seen;
    for (const Recipe& recipe : allFourArts()) {
        const GameState state = stockedFor(recipe);
        const auto gate = canCraft(recipe, state, comfortableProficiency(recipe), false);
        EXPECT_FALSE(gate.ok) << recipe.name;
        EXPECT_FALSE(gate.error.empty());
        seen.push_back(gate.error);
    }
    EXPECT_EQ(seen[0], std::string(rules::toolMissingMessage(CraftKind::Alchemy)));
    for (std::size_t i = 0; i < seen.size(); ++i) {
        for (std::size_t j = i + 1; j < seen.size(); ++j) {
            EXPECT_NE(seen[i], seen[j]) << "缺器具的提示语撞车了";
        }
    }
}

TEST(CraftingGate, RejectsWhenProficiencyTooLow) {
    const Recipe recipe = forgeRecipe();          // requiredProficiency = 16
    const GameState state = stockedFor(recipe);
    const auto gate = canCraft(recipe, state, 15, true);
    EXPECT_FALSE(gate.ok);
    // 提示里要同时有「需多少」和「有多少」，玩家才知道还差几点。
    EXPECT_NE(gate.error.find("16"), std::string::npos) << gate.error;
    EXPECT_NE(gate.error.find("15"), std::string::npos) << gate.error;
    EXPECT_NE(gate.error.find(std::string(rules::kindName(CraftKind::Forge))), std::string::npos);

    EXPECT_TRUE(canCraft(recipe, state, 16, true).ok) << "门槛是「达到即可」，不是「超过」";
}

TEST(CraftingGate, RejectsWhenMaterialMissing) {
    const Recipe recipe = talismanRecipe();
    GameState state = stockedFor(recipe);
    state.bag.clear();
    state.addItem("material_fuzhi", 2, 0);        // 灵墨与妖丹一概没有
    const auto gate = canCraft(recipe, state, comfortableProficiency(recipe), true);
    EXPECT_FALSE(gate.ok);
    EXPECT_NE(gate.error.find("material_lingmo"), std::string::npos) << gate.error;
    EXPECT_NE(gate.error.find("材料不足"), std::string::npos) << gate.error;
}

TEST(CraftingGate, RejectsWhenEveryHerbIsTooYoung) {
    Recipe recipe = alchemyRecipe();
    recipe.inputs = {herb("herb_qingfeng_cao", 2, 50)};
    // 总数绰绰有余，年份一株不够：这条路径必须与「材料不足」区分开。
    const GameState state = bagOf({BagEntry{"herb_qingfeng_cao", 9, 10}});
    const auto gate = canCraft(recipe, state, 0, true);
    EXPECT_FALSE(gate.ok);
    EXPECT_NE(gate.error.find("年份"), std::string::npos) << gate.error;
    EXPECT_NE(gate.error.find("50"), std::string::npos) << gate.error;
    EXPECT_EQ(gate.error.find("材料不足"), std::string::npos) << "年份卡住不该报成材料不足";
}

TEST(CraftingGate, RejectsWhenOnlySomeHerbsAreOldEnough) {
    Recipe recipe = alchemyRecipe();
    recipe.inputs = {herb("herb_qingfeng_cao", 3, 50)};
    const GameState state =
        bagOf({BagEntry{"herb_qingfeng_cao", 5, 10}, BagEntry{"herb_qingfeng_cao", 2, 80}});
    const auto gate = canCraft(recipe, state, 0, true);
    EXPECT_FALSE(gate.ok);
    EXPECT_NE(gate.error.find("年份"), std::string::npos) << gate.error;
}

TEST(CraftingGate, RejectsBrokenRecipe) {
    const GameState state = stockedFor(alchemyRecipe());

    Recipe noInputs = alchemyRecipe();
    noInputs.inputs.clear();
    EXPECT_FALSE(canCraft(noInputs, state, 99, true).ok);

    Recipe noProduct = alchemyRecipe();
    noProduct.productId.clear();
    EXPECT_FALSE(canCraft(noProduct, state, 99, true).ok);

    Recipe noYield = alchemyRecipe();
    noYield.productCount = 0;
    EXPECT_FALSE(canCraft(noYield, state, 99, true).ok);
}

// 同一味料写多条不同年份门槛，是「灵草按年份分堆」这条设定最自然的用武之地，
// 也是判定最容易漏掉的一处：以下五条把它钉死。
TEST(CraftingGate, AggregatesDemandForTheSameItemAcrossEntries) {
    Recipe tiered = alchemyRecipe();
    tiered.inputs = {herb("herb_qingfeng_cao", 2, 10), herb("herb_qingfeng_cao", 1, 100)};
    // 十年那条单看够（合格的有 2 株），百年那条单看也够（1 株），
    // 但两条合起来要 3 株，包里统共只有 2 株。不加总就会「材料不够也能开工」。
    const GameState state =
        bagOf({BagEntry{"herb_qingfeng_cao", 1, 10}, BagEntry{"herb_qingfeng_cao", 1, 100}});

    const auto gate = canCraft(tiered, state, 0, true);
    EXPECT_FALSE(gate.ok) << "同一味料的多条需求必须先加总再比";
    EXPECT_NE(gate.error.find("材料不足"), std::string::npos) << gate.error;
    EXPECT_NE(gate.error.find("3"), std::string::npos) << gate.error;   // 需 3
    EXPECT_NE(gate.error.find("2"), std::string::npos) << gate.error;   // 现有 2

    // 被拒了就一点不能动：consumed 汇总不得超过背包实际持有（这里应当为空）。
    for (std::uint32_t seed : {1u, 2u, 3u, 99u}) {
        const CraftResult result = craftWith(tiered, state, seed);
        EXPECT_FALSE(result.success);
        EXPECT_TRUE(result.consumed.empty());
        EXPECT_EQ(consumedCountOf(result, "herb_qingfeng_cao"), 0);
    }
}

TEST(CraftingGate, HighThresholdEntryClaimsItsStackFirst) {
    Recipe tiered = alchemyRecipe();
    tiered.inputs = {herb("herb_qingfeng_cao", 2, 10), herb("herb_qingfeng_cao", 1, 100)};
    const GameState state =
        bagOf({BagEntry{"herb_qingfeng_cao", 2, 10}, BagEntry{"herb_qingfeng_cao", 1, 100}});
    // 总量刚好 3 株。若低门槛那条先把百年那株挑走，高门槛那条就会假性失败。
    const auto gate = canCraft(tiered, state, 0, true);
    EXPECT_TRUE(gate.ok) << gate.error;

    const CraftResult result = craftWith(tiered, state, 1);
    // 呈现顺序仍按方子上的顺序，不因内部的认领顺序而颠倒。
    ASSERT_EQ(result.consumed.size(), 2u);
    EXPECT_EQ(result.consumed[0].minAge, 10);
    EXPECT_EQ(result.consumed[0].herbAge, 10);
    EXPECT_EQ(result.consumed[0].count, 2);
    EXPECT_EQ(result.consumed[1].minAge, 100);
    EXPECT_EQ(result.consumed[1].herbAge, 100);
    EXPECT_EQ(result.consumed[1].count, 1);
    EXPECT_EQ(consumedCountOf(result, "herb_qingfeng_cao"), 3);
}

TEST(CraftingGate, AgeCheckRejectsWhenOnlyTheHighThresholdEntryStarves) {
    Recipe tiered = alchemyRecipe();
    tiered.inputs = {herb("herb_qingfeng_cao", 2, 10), herb("herb_qingfeng_cao", 1, 100)};
    // 总量够（3 株），但全是十年：百年那条无论如何也满足不了。
    const GameState state = bagOf({BagEntry{"herb_qingfeng_cao", 3, 10}});
    const auto gate = canCraft(tiered, state, 0, true);
    EXPECT_FALSE(gate.ok);
    EXPECT_NE(gate.error.find("年份"), std::string::npos) << gate.error;
    EXPECT_NE(gate.error.find("100"), std::string::npos) << gate.error;
}

TEST(CraftingGate, ConsumedNeverExceedsWhatTheBagHolds) {
    // consumed 是要拿去扣背包的账单：按 (id, 年份) 汇总后，任何一堆都不能被超扣。
    Recipe tiered = alchemyRecipe();
    tiered.inputs = {herb("herb_qingfeng_cao", 2, 10), herb("herb_qingfeng_cao", 1, 100)};

    struct Scenario {
        Recipe recipe;
        GameState state;
    };
    std::vector<Scenario> scenarios;
    for (const Recipe& recipe : allFourArts()) {
        scenarios.push_back({recipe, stockedFor(recipe, 1)});   // 一点不多
        scenarios.push_back({recipe, stockedFor(recipe, 4)});   // 备料充裕
    }
    scenarios.push_back({tiered, bagOf({BagEntry{"herb_qingfeng_cao", 2, 10},
                                        BagEntry{"herb_qingfeng_cao", 1, 100}})});
    scenarios.push_back({tiered, bagOf({BagEntry{"herb_qingfeng_cao", 5, 10},
                                        BagEntry{"herb_qingfeng_cao", 5, 100}})});
    scenarios.push_back({tiered, bagOf({BagEntry{"herb_qingfeng_cao", 1, 10},
                                        BagEntry{"herb_qingfeng_cao", 1, 100}})});

    for (const Scenario& scenario : scenarios) {
        for (std::uint32_t seed = 1; seed <= 24; ++seed) {
            const CraftResult result = craftWith(scenario.recipe, scenario.state, seed);
            std::map<std::pair<std::string, int>, int> used;
            for (const Ingredient& gone : result.consumed) {
                used[std::pair<std::string, int>{gone.itemId, gone.herbAge}] += gone.count;
            }
            for (const auto& [key, count] : used) {
                int held = 0;
                for (const BagEntry& entry : scenario.state.bag) {
                    if (entry.itemId == key.first && entry.herbAge == key.second) {
                        held += entry.count;
                    }
                }
                EXPECT_LE(count, held)
                    << scenario.recipe.name << " / " << key.first << '@' << key.second
                    << " 年 / 种子 " << seed;
            }
        }
    }
}

TEST(CraftingGate, ChecksAreReportedInTheDocumentedOrder) {
    // Crafting.h 对外承诺的顺序：配方 → 炉鼎 → 熟练度 → 材料 → 年份。
    // 每条用例只让单一条件失败是测不出顺序的，这里让多条同时不满足。
    Recipe recipe = alchemyRecipe();
    recipe.requiredProficiency = 20;
    recipe.inputs = {herb("herb_qingfeng_cao", 3, 50)};

    // 配方残缺压过没炉鼎（下面四项此时也统统不满足）。
    Recipe broken = recipe;
    broken.inputs.clear();
    EXPECT_EQ(canCraft(broken, GameState{}, 0, false).error, "这张方子残缺不全，无从下手。");

    // 没炉鼎压过熟练度与材料。
    const GameState empty;
    EXPECT_EQ(canCraft(recipe, empty, 0, false).error,
              std::string(rules::toolMissingMessage(CraftKind::Alchemy)));

    // 熟练度压过材料。
    const auto lowProficiency = canCraft(recipe, empty, 0, true);
    ASSERT_FALSE(lowProficiency.ok);
    EXPECT_NE(lowProficiency.error.find("火候未到"), std::string::npos) << lowProficiency.error;

    // 材料总量压过年份：一株都没有时先说「没料」，而不是绕到年份上去。
    const auto noStock = canCraft(recipe, empty, 20, true);
    ASSERT_FALSE(noStock.ok);
    EXPECT_NE(noStock.error.find("材料不足"), std::string::npos) << noStock.error;

    // 料齐了才轮到年份。
    const auto tooYoung = canCraft(recipe, bagOf({BagEntry{"herb_qingfeng_cao", 3, 10}}), 20, true);
    ASSERT_FALSE(tooYoung.ok);
    EXPECT_NE(tooYoung.error.find("年份"), std::string::npos) << tooYoung.error;
}

TEST(CraftingGate, ZeroCountIngredientIsTreatedAsNotNeeded) {
    Recipe recipe = alchemyRecipe();
    recipe.inputs = {herb("herb_qingfeng_cao", 1, 10), part("material_lingshi", 0),
                     part("material_yaodan_3", -2)};
    // 包里一块灵石、一颗妖丹都没有，照样开得了工。
    const GameState state = bagOf({BagEntry{"herb_qingfeng_cao", 1, 10}});
    const auto gate = canCraft(recipe, state, 0, true);
    EXPECT_TRUE(gate.ok) << gate.error;

    const CraftResult result = craftWith(recipe, state, 1);
    EXPECT_EQ(consumedCountOf(result, "herb_qingfeng_cao"), 1);
    EXPECT_EQ(consumedCountOf(result, "material_lingshi"), 0);
    EXPECT_EQ(consumedCountOf(result, "material_yaodan_3"), 0);
}

TEST(CraftingGate, RecipeWhoseEveryIngredientIsZeroCountIsBroken) {
    // 一味材料被写成 count: 0 而方子只有这一味，等于凭空出产物——
    // 与「材料不够也能开工」是同一类经济漏洞，按残缺配方拦下。
    Recipe recipe = alchemyRecipe();
    recipe.inputs = {part("material_lingshi", 0), part("material_yaodan_3", 0)};
    const auto gate = canCraft(recipe, GameState{}, 99, true);
    EXPECT_FALSE(gate.ok);
    EXPECT_EQ(gate.error, "这张方子残缺不全，无从下手。");

    const CraftResult result = craft(recipe, GameState{}, 99, kAptitude, kToolGrade, 1);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.productCount, 0);
    EXPECT_TRUE(result.productId.empty());
}

TEST(CraftingGate, EveryRejectionCarriesADisplayableReason) {
    Recipe youngHerb = alchemyRecipe();
    youngHerb.inputs = {herb("herb_qingfeng_cao", 1, 90)};
    Recipe broken = alchemyRecipe();
    broken.inputs.clear();

    const Recipe forge = forgeRecipe();
    const struct Probe {
        Recipe recipe;
        GameState state;
        int proficiency;
        bool hasTool;
    } probes[] = {
        {forge, stockedFor(forge), comfortableProficiency(forge), false},   // 没炉鼎
        {forge, stockedFor(forge), 0, true},                                // 熟练度不够
        {forge, GameState{}, comfortableProficiency(forge), true},          // 材料不足
        {youngHerb, bagOf({BagEntry{"herb_qingfeng_cao", 5, 10}}), 0, true},  // 年份不够
        {broken, GameState{}, 0, true},                                     // 配方残缺
    };

    for (const Probe& probe : probes) {
        const auto gate = canCraft(probe.recipe, probe.state, probe.proficiency, probe.hasTool);
        ASSERT_FALSE(gate.ok);
        EXPECT_FALSE(gate.error.empty());
        EXPECT_TRUE(gate.value.empty());
        // 「可直接显示」的最低要求：是中文句子，不是裸字段名或错误码。
        EXPECT_NE(gate.error.find("。"), std::string::npos) << gate.error;
    }
}

// ---- 3. canCraft 拒绝的，craft 不执行也不消耗 ----

TEST(CraftingGate, RejectedCraftDoesNothingAndConsumesNothing) {
    Recipe youngHerb = alchemyRecipe();
    youngHerb.inputs = {herb("herb_qingfeng_cao", 1, 90)};
    const Recipe forge = forgeRecipe();

    const struct Probe {
        Recipe recipe;
        GameState state;
        int proficiency;
        int toolGrade;
    } probes[] = {
        {forge, stockedFor(forge), comfortableProficiency(forge), 0},   // 炉鼎品阶 0 = 没炉鼎
        {forge, stockedFor(forge), 0, 3},                               // 熟练度不够
        {forge, GameState{}, comfortableProficiency(forge), 3},         // 材料不足
        {youngHerb, bagOf({BagEntry{"herb_qingfeng_cao", 5, 10}}), 0, 3},  // 年份不够
    };

    for (const Probe& probe : probes) {
        const GameState before = probe.state;
        const auto gate =
            canCraft(probe.recipe, probe.state, probe.proficiency, probe.toolGrade > 0);
        ASSERT_FALSE(gate.ok) << probe.recipe.name;

        // 换一批种子反复试：被拒的炼制不该因为运气好就偷偷开了工。
        for (std::uint32_t seed : {1u, 7u, 99u, 20260920u}) {
            const CraftResult result = craft(probe.recipe, probe.state, probe.proficiency,
                                             kAptitude, probe.toolGrade, seed);
            EXPECT_FALSE(result.success);
            EXPECT_TRUE(result.productId.empty());
            EXPECT_EQ(result.productCount, 0);
            EXPECT_EQ(result.proficiencyGain, 0) << "没开工就不该长经验";
            EXPECT_TRUE(result.consumed.empty());
            EXPECT_EQ(result.log, gate.error) << "拒绝的理由要原样带给玩家";

            GameState after = probe.state;
            rules::applyConsumption(after, result.consumed);
            expectStateIdentical(before, after);
        }
    }
}

TEST(CraftingGate, CraftAgreesWithCanCraftOnEveryProbe) {
    for (const Recipe& recipe : allFourArts()) {
        const GameState stocked = stockedFor(recipe);
        const struct Probe {
            GameState state;
            int proficiency;
            int toolGrade;
        } probes[] = {
            {stocked, comfortableProficiency(recipe), 2},
            {stocked, comfortableProficiency(recipe), 0},
            {stocked, recipe.requiredProficiency - 1, 2},
            {GameState{}, comfortableProficiency(recipe), 2},
        };

        for (const Probe& probe : probes) {
            const auto gate =
                canCraft(recipe, probe.state, probe.proficiency, probe.toolGrade > 0);
            const CraftResult result =
                craft(recipe, probe.state, probe.proficiency, kAptitude, probe.toolGrade, 12345);
            if (gate.ok) {
                // 开了工就一定长经验，不论成败——这是「开工」与「没开工」的分水岭。
                EXPECT_GT(result.proficiencyGain, 0) << recipe.name;
            } else {
                EXPECT_EQ(result.proficiencyGain, 0) << recipe.name;
                EXPECT_FALSE(result.success) << recipe.name;
                EXPECT_TRUE(result.consumed.empty()) << recipe.name;
            }
        }
    }
}

TEST(CraftingGate, ToolGradeZeroIsTheSameAsNoTool) {
    for (const Recipe& recipe : allFourArts()) {
        const GameState state = stockedFor(recipe);
        const auto gate = canCraft(recipe, state, comfortableProficiency(recipe), false);
        const CraftResult result =
            craft(recipe, state, comfortableProficiency(recipe), kAptitude, 0, 5);
        ASSERT_FALSE(gate.ok);
        EXPECT_EQ(result.log, gate.error);
        EXPECT_FALSE(rules::hasToolOfGrade(0));
        EXPECT_TRUE(rules::hasToolOfGrade(1));
    }
}

// ---- 4. 四种技艺的失败代价差异（重点）----

TEST(CraftingFailureCost, AlchemyBurnsTheWholeBatch) {
    const Recipe recipe = alchemyRecipe();
    const GameState state = stockedFor(recipe);
    const CraftResult result = craftWith(recipe, state, seedWhere(recipe, state, false));
    ASSERT_FALSE(result.success);
    // 炸炉，药力尽散：一株灵草都留不下。
    EXPECT_EQ(consumedCountOf(result, "herb_qingfeng_cao"), 2);
    EXPECT_NE(result.log.find("尽毁"), std::string::npos) << result.log;
}

TEST(CraftingFailureCost, AlchemyBurnsEvenTheDemonCore) {
    // 妖丹入了炼丹的方子就只是一味药，不是「引」：炸炉一样烧掉。
    Recipe recipe = alchemyRecipe();
    recipe.inputs = {herb("herb_xuehong_zhi", 1, 100), part("material_yaodan_3", 1)};
    const GameState state = stockedFor(recipe);
    const CraftResult result = craftWith(recipe, state, seedWhere(recipe, state, false));
    ASSERT_FALSE(result.success);
    EXPECT_EQ(consumedCountOf(result, "material_yaodan_3"), 1);
    EXPECT_EQ(consumedCountOf(result, "herb_xuehong_zhi"), 1);
}

TEST(CraftingFailureCost, TalismanKeepsTheDemonCore) {
    const Recipe recipe = talismanRecipe();
    const GameState state = stockedFor(recipe);
    const CraftResult result = craftWith(recipe, state, seedWhere(recipe, state, false));
    ASSERT_FALSE(result.success);
    // 笔锋走偏废的是纸墨，妖丹只是引，一颗不少。
    EXPECT_EQ(consumedCountOf(result, "material_fuzhi"), 2);
    EXPECT_EQ(consumedCountOf(result, "material_lingmo"), 1);
    EXPECT_EQ(consumedCountOf(result, "material_yaodan_1"), 0);
    EXPECT_NE(result.log.find("妖丹"), std::string::npos) << result.log;
}

TEST(CraftingFailureCost, ForgeLosesHalfRoundedUp) {
    const Recipe recipe = forgeRecipe();
    const GameState state = stockedFor(recipe);
    const CraftResult result = craftWith(recipe, state, seedWhere(recipe, state, false));
    ASSERT_FALSE(result.success);
    // 三块精铁折两块（向上取整），一块玄铁折一块：取整不能白送材料回来。
    EXPECT_EQ(consumedCountOf(result, "material_jingtie"), 2);
    EXPECT_EQ(consumedCountOf(result, "material_xuantie"), 1);
    EXPECT_NE(result.log.find("回炉"), std::string::npos) << result.log;
}

TEST(CraftingFailureCost, FormationRecoversEverything) {
    const Recipe recipe = formationRecipe();
    const GameState state = stockedFor(recipe);
    const CraftResult result = craftWith(recipe, state, seedWhere(recipe, state, false));
    ASSERT_FALSE(result.success);
    // 旗插下去没接上灵脉，拔回来还是那面旗。
    EXPECT_TRUE(result.consumed.empty());
    EXPECT_NE(result.log.find("收回"), std::string::npos) << result.log;
}

TEST(CraftingFailureCost, TheFourPoliciesAreFourDistinctCharacters) {
    const FailurePolicy alchemy = failurePolicyOf(CraftKind::Alchemy);
    const FailurePolicy talisman = failurePolicyOf(CraftKind::Talisman);
    const FailurePolicy forge = failurePolicyOf(CraftKind::Forge);
    const FailurePolicy formation = failurePolicyOf(CraftKind::Formation);

    EXPECT_EQ(alchemy, (FailurePolicy{100, false}));
    EXPECT_EQ(talisman, (FailurePolicy{100, true}));
    EXPECT_EQ(forge, (FailurePolicy{50, false}));
    EXPECT_EQ(formation, (FailurePolicy{0, true}));

    const FailurePolicy all[] = {alchemy, talisman, forge, formation};
    for (std::size_t i = 0; i < std::size(all); ++i) {
        for (std::size_t j = i + 1; j < std::size(all); ++j) {
            EXPECT_FALSE(all[i] == all[j]) << "第 " << i << " 与第 " << j << " 门手艺性格撞车了";
        }
    }
}

TEST(CraftingFailureCost, SuccessConsumesEverythingForEveryKind) {
    for (const Recipe& recipe : allFourArts()) {
        const GameState state = stockedFor(recipe);
        const CraftResult result = craftWith(recipe, state, seedWhere(recipe, state, true));
        ASSERT_TRUE(result.success) << recipe.name;
        // 成功没有折扣：用掉的就是方子上写的，四艺一视同仁。
        for (const Ingredient& need : recipe.inputs) {
            EXPECT_EQ(consumedCountOf(result, need.itemId), need.count)
                << recipe.name << " / " << need.itemId;
        }
    }
}

// ---- 5. 年份扣料策略 ----

TEST(CraftingHerbAge, SpendsTheCheapestQualifyingStack) {
    Recipe recipe = alchemyRecipe();
    recipe.inputs = {herb("herb_qingfeng_cao", 1, 10)};
    // 十年与百年同在包里，方子只要十年：千年灵草不该被拿去炼金疮药。
    const GameState state =
        bagOf({BagEntry{"herb_qingfeng_cao", 1, 10}, BagEntry{"herb_qingfeng_cao", 1, 100}});
    const CraftResult result = craftWith(recipe, state, 1);
    ASSERT_EQ(result.consumed.size(), 1u);
    EXPECT_EQ(result.consumed[0].itemId, "herb_qingfeng_cao");
    EXPECT_EQ(result.consumed[0].count, 1);
    EXPECT_EQ(result.consumed[0].herbAge, 10);
}

TEST(CraftingHerbAge, StackOrderInTheBagDoesNotMatter) {
    Recipe recipe = alchemyRecipe();
    recipe.inputs = {herb("herb_qingfeng_cao", 1, 10)};
    // 百年那堆排在前面也一样：挑的是年份，不是背包次序。
    const GameState state =
        bagOf({BagEntry{"herb_qingfeng_cao", 1, 100}, BagEntry{"herb_qingfeng_cao", 1, 10}});
    const CraftResult result = craftWith(recipe, state, 1);
    ASSERT_EQ(result.consumed.size(), 1u);
    EXPECT_EQ(result.consumed[0].herbAge, 10);
}

TEST(CraftingHerbAge, SkipsStacksUnderTheThreshold) {
    Recipe recipe = alchemyRecipe();
    recipe.inputs = {herb("herb_qingfeng_cao", 1, 100)};
    const GameState state =
        bagOf({BagEntry{"herb_qingfeng_cao", 5, 10}, BagEntry{"herb_qingfeng_cao", 1, 100}});
    const CraftResult result = craftWith(recipe, state, 1);
    ASSERT_EQ(result.consumed.size(), 1u);
    EXPECT_EQ(result.consumed[0].herbAge, 100) << "门槛之下的堆不能入炉";
}

TEST(CraftingHerbAge, SpillsIntoTheNextStackWhenTheCheapestRunsOut) {
    Recipe recipe = alchemyRecipe();
    recipe.inputs = {herb("herb_qingfeng_cao", 3, 10)};
    const GameState state = bagOf({BagEntry{"herb_qingfeng_cao", 1, 10},
                                   BagEntry{"herb_qingfeng_cao", 5, 50},
                                   BagEntry{"herb_qingfeng_cao", 5, 200}});
    const CraftResult result = craftWith(recipe, state, 1);
    // 一株十年的先下，缺的两株从五十年的里补；两百年那堆一株不动。
    ASSERT_EQ(result.consumed.size(), 2u);
    EXPECT_EQ(result.consumed[0].herbAge, 10);
    EXPECT_EQ(result.consumed[0].count, 1);
    EXPECT_EQ(result.consumed[1].herbAge, 50);
    EXPECT_EQ(result.consumed[1].count, 2);
}

TEST(CraftingHerbAge, PartialLossAlsoTakesTheCheapest) {
    Recipe recipe = forgeRecipe();
    recipe.inputs = {herb("material_jinleizhu", 3, 10)};
    const GameState state = bagOf({BagEntry{"material_jinleizhu", 4, 10},
                                   BagEntry{"material_jinleizhu", 4, 300}});
    const CraftResult result = craftWith(recipe, state, seedWhere(recipe, state, false));
    ASSERT_FALSE(result.success);
    ASSERT_EQ(result.consumed.size(), 1u);
    EXPECT_EQ(result.consumed[0].count, 2) << "折损过半，向上取整";
    EXPECT_EQ(result.consumed[0].herbAge, 10) << "折的也该是便宜的那一堆";
}

TEST(CraftingHerbAge, ApplyConsumptionHitsExactlyThePlannedStacks) {
    Recipe recipe = alchemyRecipe();
    recipe.inputs = {herb("herb_qingfeng_cao", 1, 10)};
    GameState state =
        bagOf({BagEntry{"herb_qingfeng_cao", 1, 10}, BagEntry{"herb_qingfeng_cao", 2, 100}});
    const CraftResult result = craftWith(recipe, state, 1);
    rules::applyConsumption(state, result.consumed);

    // 十年那堆空了被清掉，百年那堆纹丝不动。
    ASSERT_EQ(state.bag.size(), 1u);
    EXPECT_EQ(state.bag[0].itemId, "herb_qingfeng_cao");
    EXPECT_EQ(state.bag[0].herbAge, 100);
    EXPECT_EQ(state.bag[0].count, 2);
}

TEST(CraftingHerbAge, ApplyConsumptionLeavesOtherItemsAlone) {
    const Recipe recipe = talismanRecipe();
    GameState state = stockedFor(recipe, 2);
    state.addItem("pill_jinchuang_yao", 4, 0);
    const CraftResult result = craftWith(recipe, state, seedWhere(recipe, state, false));
    ASSERT_FALSE(result.success);
    rules::applyConsumption(state, result.consumed);

    EXPECT_EQ(state.itemCount("pill_jinchuang_yao"), 4);
    EXPECT_EQ(state.itemCount("material_yaodan_1"), 2) << "妖丹是引，一颗没少";
    EXPECT_EQ(state.itemCount("material_fuzhi"), 2);   // 进货 4 张，废了 2 张
    EXPECT_EQ(state.itemCount("material_lingmo"), 1);  // 进货 2 份，废了 1 份
}

// ---- 6. 熟练度成长 ----

TEST(CraftingProficiency, FailureStillTeachesSomething) {
    for (const Recipe& recipe : allFourArts()) {
        const GameState state = stockedFor(recipe);
        const CraftResult result = craftWith(recipe, state, seedWhere(recipe, state, false));
        ASSERT_FALSE(result.success) << recipe.name;
        EXPECT_GT(result.proficiencyGain, 0) << recipe.name << "：白干一场也该记住火候";
    }
}

TEST(CraftingProficiency, SuccessTeachesMoreThanFailureAtEveryDifficulty) {
    Recipe recipe = alchemyRecipe();
    for (int difficulty = 0; difficulty <= 100; ++difficulty) {
        recipe.difficulty = difficulty;
        const int win = proficiencyGain(recipe, true);
        const int lose = proficiencyGain(recipe, false);
        EXPECT_GT(win, lose) << "difficulty=" << difficulty;
        EXPECT_GT(lose, 0) << "difficulty=" << difficulty;
    }
}

TEST(CraftingProficiency, HarderRecipesTeachMore) {
    Recipe easy = alchemyRecipe();
    easy.difficulty = 5;
    Recipe hard = alchemyRecipe();
    hard.difficulty = 88;
    EXPECT_GT(proficiencyGain(hard, true), proficiencyGain(easy, true));
    EXPECT_GT(proficiencyGain(hard, false), proficiencyGain(easy, false));
    // 一万炉金疮药也炼不出丹道大师。
    EXPECT_EQ(proficiencyGain(easy, true), rules::kProficiencyGainOnSuccess);
    EXPECT_EQ(proficiencyGain(easy, false), rules::kProficiencyGainOnFailure);
}

TEST(CraftingProficiency, GainMatchesTheCraftResult) {
    for (const Recipe& recipe : allFourArts()) {
        const GameState state = stockedFor(recipe);
        const CraftResult won = craftWith(recipe, state, seedWhere(recipe, state, true));
        const CraftResult lost = craftWith(recipe, state, seedWhere(recipe, state, false));
        EXPECT_EQ(won.proficiencyGain, proficiencyGain(recipe, true)) << recipe.name;
        EXPECT_EQ(lost.proficiencyGain, proficiencyGain(recipe, false)) << recipe.name;
        EXPECT_GT(won.proficiencyGain, lost.proficiencyGain) << recipe.name;
    }
}

// ---- 7. 同种子可复现 ----

TEST(CraftingDeterminism, SameSeedSameResult) {
    for (const Recipe& recipe : allFourArts()) {
        const GameState state = stockedFor(recipe);
        for (std::uint32_t seed : {1u, 2u, 777u, 20260920u}) {
            const CraftResult first = craftWith(recipe, state, seed);
            const CraftResult second = craftWith(recipe, state, seed);
            expectCraftResultIdentical(first, second);
        }
    }
}

TEST(CraftingDeterminism, DifferentSeedsReachBothOutcomes) {
    Recipe recipe = alchemyRecipe();
    recipe.difficulty = 15;   // 配上 craftWith 的熟练度/资质/炉鼎，成功率正好五成
    const GameState state = stockedFor(recipe);
    ASSERT_EQ(successChance(recipe, comfortableProficiency(recipe), kAptitude, kToolGrade), 50);

    bool sawWin = false;
    bool sawLoss = false;
    for (std::uint32_t seed = 1; seed <= 40; ++seed) {
        if (craftWith(recipe, state, seed).success) {
            sawWin = true;
        } else {
            sawLoss = true;
        }
    }
    EXPECT_TRUE(sawWin);
    EXPECT_TRUE(sawLoss);
}

// ---- 8. 产物数量与消耗清单 ----

TEST(CraftingProduct, OnlySuccessYieldsAProduct) {
    Recipe recipe = talismanRecipe();
    recipe.productCount = 3;
    const GameState state = stockedFor(recipe);

    const CraftResult won = craftWith(recipe, state, seedWhere(recipe, state, true));
    ASSERT_TRUE(won.success);
    EXPECT_EQ(won.productId, "talisman_hushen_fu");
    EXPECT_EQ(won.productCount, 3);
    EXPECT_NE(won.log.find(recipe.name), std::string::npos) << won.log;

    const CraftResult lost = craftWith(recipe, state, seedWhere(recipe, state, false));
    ASSERT_FALSE(lost.success);
    EXPECT_TRUE(lost.productId.empty()) << "没成就没有产物，不留半个 id 给调用方误用";
    EXPECT_EQ(lost.productCount, 0);
    EXPECT_FALSE(lost.log.empty());
}

TEST(CraftingProduct, ConsumedListCarriesTheAgeOfEveryStack) {
    Recipe recipe = alchemyRecipe();
    recipe.inputs = {herb("herb_qingfeng_cao", 2, 20), part("material_lingshi", 3)};
    const GameState state = bagOf({BagEntry{"herb_qingfeng_cao", 5, 30},
                                   BagEntry{"material_lingshi", 9, 0}});
    const CraftResult result = craftWith(recipe, state, seedWhere(recipe, state, true));
    ASSERT_TRUE(result.success);
    ASSERT_EQ(result.consumed.size(), 2u);

    EXPECT_EQ(result.consumed[0].itemId, "herb_qingfeng_cao");
    EXPECT_EQ(result.consumed[0].count, 2);
    EXPECT_EQ(result.consumed[0].minAge, 20);   // 方子的门槛原样带出
    EXPECT_EQ(result.consumed[0].herbAge, 30);  // 实际扣的是三十年那一堆
    EXPECT_EQ(result.consumed[1].itemId, "material_lingshi");
    EXPECT_EQ(result.consumed[1].count, 3);
    EXPECT_EQ(result.consumed[1].herbAge, 0);
}

TEST(CraftingProduct, LogIsChineseAndNamesTheRecipe) {
    for (const Recipe& recipe : allFourArts()) {
        const GameState state = stockedFor(recipe);
        for (bool wantSuccess : {true, false}) {
            const CraftResult result =
                craftWith(recipe, state, seedWhere(recipe, state, wantSuccess));
            EXPECT_FALSE(result.log.empty()) << recipe.name;
            EXPECT_NE(result.log.find(recipe.name), std::string::npos) << result.log;
            EXPECT_NE(result.log.find("。"), std::string::npos) << result.log;
        }
    }
}

TEST(CraftingProduct, KindNamesAreDistinct) {
    const std::string names[] = {
        std::string(rules::kindName(CraftKind::Alchemy)),
        std::string(rules::kindName(CraftKind::Talisman)),
        std::string(rules::kindName(CraftKind::Forge)),
        std::string(rules::kindName(CraftKind::Formation)),
    };
    for (std::size_t i = 0; i < std::size(names); ++i) {
        EXPECT_FALSE(names[i].empty());
        for (std::size_t j = i + 1; j < std::size(names); ++j) {
            EXPECT_NE(names[i], names[j]);
        }
    }
}

}  // namespace
