// P3 第 4 章前置之三：炼制接线（契约 docs/interfaces-p3-ch04.md 第 3 节）。
//
// `src/core/rules/Crafting.h` 造好了很久，`data/recipes/` 下也有四类方子，
// 但 `src/game/` 与 `src/io/` 里**一次调用都没有**。本文件钉的就是这条新接上的线：
//
//   一、配方加载器（此前压根不存在）：正向一条，坏数据每一种坏法各一条负向；
//   二、真数据读得进来，且引用校验对真数据不是空转；
//   三、炼丹面板：列得出、点不动的写明缺什么、能成能败、失败按策略扣料；
//   四、地图上的 facility kind=alchemy 真的能把面板开起来。
//
// 契约第 4 节第 5 条点名要求：「缺料时点不动且写明缺什么（断言那串字**确实有
// 内容**，不要写成『找不到某个词就算过』）」。本文件每一条禁用理由都先
// ASSERT_FALSE(empty()) 再验内容，而且每一条否定都配一条肯定（同一张方子，
// 补上材料就该亮）——只写一边的话，把整段判定删掉也能全绿。
#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "TempDir.h"
#include "core/model/Types.h"
#include "core/rules/Crafting.h"
#include "game/AlchemyScene.h"
#include "game/Application.h"
#include "io/DataLoader.h"
#include "io/RecipeLoader.h"
#include "ui/Widgets.h"

namespace {

namespace fs = std::filesystem;

using fanren::game::AlchemyScene;
using fanren::game::Application;
using fanren::game::craftProficiency;
using fanren::game::humanizeReason;
using fanren::rules::CraftKind;
using fanren::rules::Recipe;

// 本章真正要用的那张方子：金疮药方。难度 5、熟练度门槛 0、一味十年清风草。
constexpr const char* kEasyRecipe = "recipe_jinchuang_yao";
constexpr const char* kEasyRecipeName = "金疮药方";
constexpr const char* kEasyProduct = "pill_jinchuang_yao";
constexpr const char* kEasyHerb = "herb_qingfeng_cao";
constexpr const char* kEasyHerbName = "清风草";
constexpr int kEasyHerbAge = 10;
// 一张有「引」的制符方：失败时妖丹保得住（failurePolicyOf 的性格差异）。
constexpr const char* kTalismanRecipe = "recipe_hushen_fu";
constexpr const char* kCatalyst = "material_yaodan_1";
constexpr const char* kPaper = "material_fuzhi";
constexpr const char* kInk = "material_lingmo";

void writeFile(const fs::path& path, const std::string& text) {
    fs::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out << text;
}

std::string projectRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "data" / "recipes" / "alchemy" /
                       "jinchuang_yao.json")) {
            return candidate;
        }
    }
    return ".";
}

// ---------------------------------------------------------------------------
// 一、配方加载器
// ---------------------------------------------------------------------------

// 一张结构完整的方子。extra 覆盖其中某一项（后写的覆盖先写的，nlohmann 认最后一个）。
std::string recipeJson(const std::string& body) {
    return "{" + body + "}";
}

const char* kGoodBody =
    R"("id":"recipe_probe","name":"探针方","kind":"alchemy","productId":"pill_jinchuang_yao",)"
    R"("productCount":1,"difficulty":10,"requiredProficiency":0,)"
    R"("inputs":[{"itemId":"herb_qingfeng_cao","count":2,"minAge":10}])";

fanren::core::Result<Recipe> loadProbe(const std::string& body) {
    fanren::test::TempDir tmp{"fanren_ch04_recipe"};
    const fs::path path = tmp.path() / "probe.json";
    writeFile(path, recipeJson(body));
    return fanren::io::loadRecipe(path.string());
}

TEST(Ch04RecipeLoader, TheGoodFixtureLoads) {
    // **先验判据。** 这一条要是红的，下面十几条负向用例的红灯一个都不作数——
    // 它们分不清「这项校验抓住了坏数据」与「我这份夹具本来就读不进来」。
    const auto loaded = loadProbe(kGoodBody);
    ASSERT_TRUE(loaded.ok) << loaded.error;
    EXPECT_EQ(loaded.value.id, "recipe_probe");
    EXPECT_EQ(loaded.value.name, "探针方");
    EXPECT_EQ(loaded.value.kind, CraftKind::Alchemy);
    EXPECT_EQ(loaded.value.productId, "pill_jinchuang_yao");
    EXPECT_EQ(loaded.value.productCount, 1);
    EXPECT_EQ(loaded.value.difficulty, 10);
    ASSERT_EQ(loaded.value.inputs.size(), 1u);
    EXPECT_EQ(loaded.value.inputs[0].itemId, "herb_qingfeng_cao");
    EXPECT_EQ(loaded.value.inputs[0].count, 2);
    EXPECT_EQ(loaded.value.inputs[0].minAge, 10);
    EXPECT_FALSE(loaded.value.inputs[0].catalyst);
}

TEST(Ch04RecipeLoader, TheCatalystFlagIsRead) {
    const auto loaded = loadProbe(
        R"("id":"r","name":"n","kind":"talisman","productId":"talisman_hushen_fu",)"
        R"("inputs":[{"itemId":"material_yaodan_1","count":1,"catalyst":true}])");
    ASSERT_TRUE(loaded.ok) << loaded.error;
    ASSERT_EQ(loaded.value.inputs.size(), 1u);
    EXPECT_TRUE(loaded.value.inputs[0].catalyst) << "「引」的标记丢了，失败时妖丹就会跟着烧掉";
}

// 每一种坏法各一条。判据统一：读不进来 + 错误消息真有内容 + 错误消息点名是哪一项。
struct BadCase {
    const char* what;
    const char* body;
    const char* needle;
};

TEST(Ch04RecipeLoader, EveryShapeOfBrokenRecipeIsRefusedAndSaysWhich) {
    const BadCase cases[] = {
        {"缺 id",
         R"("name":"n","kind":"alchemy","productId":"pill_jinchuang_yao",)"
         R"("inputs":[{"itemId":"herb_qingfeng_cao","count":1}])",
         "id"},
        {"缺 name（它要直接上屏）",
         R"("id":"r","kind":"alchemy","productId":"pill_jinchuang_yao",)"
         R"("inputs":[{"itemId":"herb_qingfeng_cao","count":1}])",
         "name"},
        {"kind 认不出来",
         R"("id":"r","name":"n","kind":"lianqi","productId":"pill_jinchuang_yao",)"
         R"("inputs":[{"itemId":"herb_qingfeng_cao","count":1}])",
         "kind"},
        {"缺 kind",
         R"("id":"r","name":"n","productId":"pill_jinchuang_yao",)"
         R"("inputs":[{"itemId":"herb_qingfeng_cao","count":1}])",
         "kind"},
        {"缺 productId",
         R"("id":"r","name":"n","kind":"alchemy",)"
         R"("inputs":[{"itemId":"herb_qingfeng_cao","count":1}])",
         "productId"},
        {"productCount 为 0（凭空出产物的反面：白开一炉）",
         R"("id":"r","name":"n","kind":"alchemy","productId":"pill_jinchuang_yao",)"
         R"("productCount":0,"inputs":[{"itemId":"herb_qingfeng_cao","count":1}])",
         "productCount"},
        {"productCount 不是整数",
         R"("id":"r","name":"n","kind":"alchemy","productId":"pill_jinchuang_yao",)"
         R"("productCount":"两份","inputs":[{"itemId":"herb_qingfeng_cao","count":1}])",
         "productCount"},
        {"difficulty 越上界（规则层会夹到 100，于是 900 与 100 一模一样）",
         R"("id":"r","name":"n","kind":"alchemy","productId":"pill_jinchuang_yao",)"
         R"("difficulty":900,"inputs":[{"itemId":"herb_qingfeng_cao","count":1}])",
         "difficulty"},
        {"difficulty 越下界",
         R"("id":"r","name":"n","kind":"alchemy","productId":"pill_jinchuang_yao",)"
         R"("difficulty":-1,"inputs":[{"itemId":"herb_qingfeng_cao","count":1}])",
         "difficulty"},
        {"requiredProficiency 越界（熟练度封顶 100，写 500 就是一张永远开不了的方子）",
         R"("id":"r","name":"n","kind":"alchemy","productId":"pill_jinchuang_yao",)"
         R"("requiredProficiency":500,"inputs":[{"itemId":"herb_qingfeng_cao","count":1}])",
         "requiredProficiency"},
        {"缺 inputs",
         R"("id":"r","name":"n","kind":"alchemy","productId":"pill_jinchuang_yao")",
         "inputs"},
        {"inputs 是空数组（不耗材料就是凭空出产物）",
         R"("id":"r","name":"n","kind":"alchemy","productId":"pill_jinchuang_yao","inputs":[])",
         "inputs"},
        {"材料缺 itemId",
         R"("id":"r","name":"n","kind":"alchemy","productId":"pill_jinchuang_yao",)"
         R"("inputs":[{"count":1}])",
         "itemId"},
        {"材料 count 为 0（规则层当作「不需要这一味」）",
         R"("id":"r","name":"n","kind":"alchemy","productId":"pill_jinchuang_yao",)"
         R"("inputs":[{"itemId":"herb_qingfeng_cao","count":0}])",
         "count"},
        {"材料 minAge 为负",
         R"("id":"r","name":"n","kind":"alchemy","productId":"pill_jinchuang_yao",)"
         R"("inputs":[{"itemId":"herb_qingfeng_cao","count":1,"minAge":-5}])",
         "minAge"},
        {"材料 count 不是整数",
         R"("id":"r","name":"n","kind":"alchemy","productId":"pill_jinchuang_yao",)"
         R"("inputs":[{"itemId":"herb_qingfeng_cao","count":"两株"}])",
         "count"},
    };

    for (const BadCase& bad : cases) {
        const auto loaded = loadProbe(bad.body);
        EXPECT_FALSE(loaded.ok) << bad.what << "：这份坏数据被收下了";
        if (loaded.ok) continue;
        // 先验错误消息真有内容，再验它点名了是哪一项——只写后一句的话，
        // 错误串变成空的时候这些断言照样全绿。
        ASSERT_FALSE(loaded.error.empty()) << bad.what;
        EXPECT_NE(loaded.error.find(bad.needle), std::string::npos)
            << bad.what << " 的错误没点名是哪一项：" << loaded.error;
    }
}

TEST(Ch04RecipeLoader, DuplicateIdsAcrossFilesAreRefusedInsteadOfOneOverwritingTheOther) {
    fanren::test::TempDir tmp{"fanren_ch04_recipe_dup"};
    writeFile(tmp.path() / "a.json", recipeJson(kGoodBody));
    writeFile(tmp.path() / "sub" / "b.json", recipeJson(kGoodBody));

    const auto data = fanren::io::loadGameData(projectRoot() + "/data");
    ASSERT_TRUE(data.ok) << data.error;

    const auto loaded = fanren::io::loadRecipes(tmp.path().string(), data.value);
    EXPECT_FALSE(loaded.ok) << "同一个 id 两份文件，后者覆盖前者是最糟的处理";
    ASSERT_FALSE(loaded.error.empty());
    EXPECT_NE(loaded.error.find("recipe_probe"), std::string::npos) << loaded.error;
}

TEST(Ch04RecipeLoader, DanglingItemReferencesAreRefused) {
    const auto data = fanren::io::loadGameData(projectRoot() + "/data");
    ASSERT_TRUE(data.ok) << data.error;

    const auto load = [&data](const std::string& body) {
        fanren::test::TempDir tmp{"fanren_ch04_recipe_ref"};
        writeFile(tmp.path() / "probe.json", recipeJson(body));
        return fanren::io::loadRecipes(tmp.path().string(), data.value);
    };

    // 先验：同一条路径上，一份引用都对得上的方子读得进来。
    {
        const auto sane = load(kGoodBody);
        ASSERT_TRUE(sane.ok) << "夹具本身要是读不进来，下面两条就什么也没证明: " << sane.error;
        EXPECT_EQ(sane.value.size(), 1u);
    }

    // 材料悬空：这张方子永远配不出来，而禁用理由里会写着一个玩家从没见过的 id。
    const auto badInput = load(
        R"("id":"r","name":"n","kind":"alchemy","productId":"pill_jinchuang_yao",)"
        R"("inputs":[{"itemId":"herb_bu_cun_zai","count":1}])");
    EXPECT_FALSE(badInput.ok);
    ASSERT_FALSE(badInput.error.empty());
    EXPECT_NE(badInput.error.find("herb_bu_cun_zai"), std::string::npos) << badInput.error;

    // 产出悬空：炼成之后背包里多出一件物品册查不到的东西。
    const auto badProduct = load(
        R"("id":"r","name":"n","kind":"alchemy","productId":"pill_bu_cun_zai",)"
        R"("inputs":[{"itemId":"herb_qingfeng_cao","count":1}])");
    EXPECT_FALSE(badProduct.ok);
    ASSERT_FALSE(badProduct.error.empty());
    EXPECT_NE(badProduct.error.find("pill_bu_cun_zai"), std::string::npos) << badProduct.error;
}

TEST(Ch04RecipeLoader, AMissingDirectoryIsZeroRecipesNotAFailure) {
    const auto data = fanren::io::loadGameData(projectRoot() + "/data");
    ASSERT_TRUE(data.ok) << data.error;
    const auto loaded = fanren::io::loadRecipes("no_such_directory_ch04", data.value);
    ASSERT_TRUE(loaded.ok) << loaded.error;
    EXPECT_TRUE(loaded.value.empty());
}

// ---------------------------------------------------------------------------
// 二、真数据
// ---------------------------------------------------------------------------

TEST(Ch04RecipeLoader, TheShippedRecipesAllLoadAndTheReferenceCheckIsNotVacuous) {
    const auto data = fanren::io::loadGameData(projectRoot() + "/data");
    ASSERT_TRUE(data.ok) << data.error;
    const auto loaded = fanren::io::loadRecipes(projectRoot() + "/data/recipes", data.value);
    ASSERT_TRUE(loaded.ok) << "仓库里现成的方子读不进来：" << loaded.error;

    // 先验：确实读到了东西，否则「全都合法」在空表上恒真。
    ASSERT_GE(loaded.value.size(), 16u) << "只读到 " << loaded.value.size() << " 张方子";
    int alchemy = 0;
    for (const auto& entry : loaded.value) {
        if (entry.second.kind == CraftKind::Alchemy) ++alchemy;
    }
    EXPECT_GE(alchemy, 6) << "data/recipes/alchemy 下现有六张方子，本章要用的就是它们";
    ASSERT_NE(loaded.value.find(kEasyRecipe), loaded.value.end());
    EXPECT_EQ(loaded.value.at(kEasyRecipe).name, kEasyRecipeName);
}

// ---------------------------------------------------------------------------
// 三、面板
// ---------------------------------------------------------------------------

class Ch04AlchemyPanelTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto ready = app_.init(projectRoot(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
        ASSERT_FALSE(app_.recipes().empty()) << "Application 没把配方读进来";
    }
    void TearDown() override { app_.shutdown(); }

    fanren::core::GameState& state() { return app_.state(); }

    // 面板的列表就地建一份：不开场景也能问「这一行点不点得动」。
    std::vector<fanren::ui::ListItem> rows(CraftKind kind, int toolGrade) {
        return AlchemyScene::buildRecipeItems(
            app_.data(), app_.state(), kind, toolGrade,
            AlchemyScene::visibleRecipes(app_.recipes(), kind, app_.state()));
    }

    // 取某一行，**按值返回**。
    //
    // 从前这里写的是 `const ui::ListItem& row = rows(...)[index];` —— 绑到一个
    // 临时 vector 的元素上，整条表达式一结束就悬空了（生存期延长不穿过
    // operator[]）。症状是禁用理由读出来是空串，而那恰好就是这些用例要拦的
    // 那种缺陷的长相：一条钉着「理由必须有内容」的测试，自己因为悬空引用而
    // 永远看到空内容。留着这段注释，免得后来人再写一遍。
    fanren::ui::ListItem rowAt(CraftKind kind, int toolGrade, int index) {
        const std::vector<fanren::ui::ListItem> items = rows(kind, toolGrade);
        EXPECT_LT(static_cast<std::size_t>(index), items.size());
        return items[static_cast<std::size_t>(index)];
    }

    int indexOfRecipe(CraftKind kind, const std::string& recipeId) {
        const auto picked = AlchemyScene::visibleRecipes(app_.recipes(), kind, app_.state());
        for (std::size_t i = 0; i < picked.size(); ++i) {
            if (picked[i]->id == recipeId) return static_cast<int>(i);
        }
        return -1;
    }

    Application app_;
};

TEST_F(Ch04AlchemyPanelTest, EveryRecipeOfTheCraftIsListedEvenWhenItCannotBeMade) {
    // 契约第 3.2 节：**配不出来的列出来但点不动，并写明缺什么。**
    // 直接不显示，玩家会以为这门手艺根本没有这张方子，掉头去别处找。
    const std::vector<fanren::ui::ListItem> items = rows(CraftKind::Alchemy, 1);
    const auto picked = AlchemyScene::visibleRecipes(app_.recipes(), CraftKind::Alchemy, app_.state());
    ASSERT_FALSE(picked.empty());
    ASSERT_EQ(items.size(), picked.size() + 1) << "末项固定是「离开」";
    EXPECT_EQ(items.back().label, "离开");

    bool sawEasy = false;
    for (std::size_t i = 0; i + 1 < items.size(); ++i) {
        EXPECT_EQ(items[i].label, picked[i]->name);
        if (picked[i]->id == kEasyRecipe) sawEasy = true;
    }
    EXPECT_TRUE(sawEasy) << "金疮药方没列出来";

    // 这一门只列这一门的方子：制符方混进炼丹面板是最容易写出来的一个错。
    for (const Recipe* recipe : picked) EXPECT_EQ(recipe->kind, CraftKind::Alchemy);
}

TEST_F(Ch04AlchemyPanelTest, WithoutMaterialsTheRowIsDisabledAndNamesWhatIsMissing) {
    ASSERT_EQ(state().itemCount(kEasyHerb), 0) << "先验：背包里确实一株也没有";

    const int index = indexOfRecipe(CraftKind::Alchemy, kEasyRecipe);
    ASSERT_GE(index, 0);
    const fanren::ui::ListItem row = rowAt(CraftKind::Alchemy, 1, index);

    EXPECT_FALSE(row.enabled);
    // 契约第 4 节第 5 条：**断言那串字确实有内容**，不要写成「找不到某个词就算过」。
    ASSERT_FALSE(row.disabledReason.empty()) << "点不动却一个字不说，玩家一律当成 bug";
    // 缺什么：要报**物品名**而不是 id。规则层不认识物品册，所以 canCraft 那句话里
    // 嵌的是 herb_qingfeng_cao，由面板换成「清风草」。
    EXPECT_NE(row.disabledReason.find(kEasyHerbName), std::string::npos) << row.disabledReason;
    EXPECT_EQ(row.disabledReason.find(kEasyHerb), std::string::npos)
        << "屏幕上不该出现物品 id：" << row.disabledReason;
    // 缺多少也要写：只说「材料不足」的话，玩家不知道该去采一株还是十株。
    EXPECT_NE(row.disabledReason.find("1"), std::string::npos) << row.disabledReason;

    // **配一条肯定。** 补上材料，同一张方子就该亮——否则「点不动」有可能是
    // 这一行压根一直点不动，与缺不缺料无关。
    state().addItem(kEasyHerb, 1, kEasyHerbAge);
    const fanren::ui::ListItem now = rowAt(CraftKind::Alchemy, 1, index);
    EXPECT_TRUE(now.enabled) << now.disabledReason;
    EXPECT_TRUE(now.disabledReason.empty());
}

TEST_F(Ch04AlchemyPanelTest, WithoutAFurnaceEveryRowIsDisabledAndSaysItIsTheFurnace) {
    // 材料给足，只把炉子拿掉：这样「点不动」只可能是因为缺炉。
    state().addItem(kEasyHerb, 10, kEasyHerbAge);
    const int index = indexOfRecipe(CraftKind::Alchemy, kEasyRecipe);
    ASSERT_GE(index, 0);

    // 先验：有炉子时它是亮的。
    ASSERT_TRUE(rowAt(CraftKind::Alchemy, 1, index).enabled);

    const fanren::ui::ListItem row = rowAt(CraftKind::Alchemy, 0, index);
    EXPECT_FALSE(row.enabled);
    ASSERT_FALSE(row.disabledReason.empty());
    EXPECT_NE(row.disabledReason.find("丹炉"), std::string::npos) << row.disabledReason;
}

TEST_F(Ch04AlchemyPanelTest, TooLowAProficiencySaysSoInsteadOfBlamingTheMaterials) {
    // 结丹灵药方：难度 88、熟练度门槛 72。材料给足，只有火候不够。
    // 它挂着占位门闸 story.recipe_later（第 7 章契约 1.2），先让它「到手」才列得出来；判据不变。
    state().setFlag("story.recipe_later");
    const auto found = app_.recipes().find("recipe_jiedan_lingyao");
    ASSERT_NE(found, app_.recipes().end());
    for (const fanren::rules::Ingredient& need : found->second.inputs) {
        state().addItem(need.itemId, need.count + 2, need.minAge);
    }
    ASSERT_EQ(state().alchemyProficiency, 0) << "先验：熟练度确实是 0";

    const int index = indexOfRecipe(CraftKind::Alchemy, "recipe_jiedan_lingyao");
    ASSERT_GE(index, 0);
    const fanren::ui::ListItem row = rowAt(CraftKind::Alchemy, 1, index);
    EXPECT_FALSE(row.enabled);
    ASSERT_FALSE(row.disabledReason.empty());
    EXPECT_NE(row.disabledReason.find("火候"), std::string::npos) << row.disabledReason;
    EXPECT_NE(row.disabledReason.find("72"), std::string::npos)
        << "差多少要写出来，那是玩家下一步的目标：" << row.disabledReason;

    // 配一条肯定：把熟练度补到门槛上，同一行就该亮。
    state().alchemyProficiency = found->second.requiredProficiency;
    EXPECT_TRUE(rowAt(CraftKind::Alchemy, 1, index).enabled);
}

TEST_F(Ch04AlchemyPanelTest, TheDisabledReasonIsTheOneTheRulesLayerActuallyGives) {
    // 面板画的那句话与规则层回绝时给的那一句，必须是同一个判据的同一个返回值
    //（灵田面板、商店、战斗菜单三处都写死了这条纪律）。
    // 这里验的是「只换了物品名，别的一字未改」。
    const auto found = app_.recipes().find(kEasyRecipe);
    ASSERT_NE(found, app_.recipes().end());
    const fanren::core::Result<std::string> gate =
        fanren::rules::canCraft(found->second, state(), state().alchemyProficiency, true);
    ASSERT_FALSE(gate.ok) << "先验：这时候确实开不了工";
    ASSERT_FALSE(gate.error.empty());

    const int index = indexOfRecipe(CraftKind::Alchemy, kEasyRecipe);
    ASSERT_GE(index, 0);
    const std::string shown = rowAt(CraftKind::Alchemy, 1, index).disabledReason;
    EXPECT_EQ(shown, humanizeReason(app_.data(), found->second, gate.error))
        << "面板另写了一套说法，而那正是「面板说能点、点下去被回绝」的来处";
}

TEST_F(Ch04AlchemyPanelTest, HumanizeOnlyTouchesTheIdsThatAreActuallyInTheRecipe) {
    // 负向：一句本来就没提到物品的话，一个字也不该被改。
    const auto found = app_.recipes().find(kEasyRecipe);
    ASSERT_NE(found, app_.recipes().end());
    const std::string untouched = "这张方子残缺不全，无从下手。";
    EXPECT_EQ(humanizeReason(app_.data(), found->second, untouched), untouched);
    EXPECT_TRUE(humanizeReason(app_.data(), found->second, "").empty());
}

TEST_F(Ch04AlchemyPanelTest, CraftingWithoutMaterialsChangesNothingAndSaysWhy) {
    AlchemyScene scene(CraftKind::Alchemy, 1);
    scene.onEnter(app_);
    const int index = indexOfRecipe(CraftKind::Alchemy, kEasyRecipe);
    ASSERT_GE(index, 0);

    const int profBefore = state().alchemyProficiency;
    const std::size_t bagBefore = state().bag.size();
    EXPECT_FALSE(scene.craftAt(app_, index)) << "开不了工就不能报成开工了";
    // **一个字节都不动**：「没开工」和「开工失败」是两回事，只有后者该长经验。
    EXPECT_EQ(state().alchemyProficiency, profBefore) << "没开工不该长熟练度";
    EXPECT_EQ(state().bag.size(), bagBefore);
    EXPECT_EQ(state().itemCount(kEasyProduct), 0);
    // 静默失败是明令禁止的。
    ASSERT_FALSE(scene.feedback().empty());
    EXPECT_NE(scene.feedback().find(kEasyHerbName), std::string::npos) << scene.feedback();
}

TEST_F(Ch04AlchemyPanelTest, AlchemyCanSucceedAndCanBlowUpAndBothAreAccountedFor) {
    // 成与败都要走得到。整条链路是确定的（种子由配方 id、日期、熟练度、第几炉
    // 派生），所以这条用例不是「碰运气」——同样的起点每次跑出同样的一串结果。
    AlchemyScene scene(CraftKind::Alchemy, 1);
    scene.onEnter(app_);
    const int index = indexOfRecipe(CraftKind::Alchemy, kEasyRecipe);
    ASSERT_GE(index, 0);

    int successes = 0;
    int failures = 0;
    for (int attempt = 0; attempt < 60 && (successes == 0 || failures == 0); ++attempt) {
        state().addItem(kEasyHerb, 1, kEasyHerbAge);
        const int herbBefore = state().itemCount(kEasyHerb);
        const int productBefore = state().itemCount(kEasyProduct);
        const int profBefore = state().alchemyProficiency;

        ASSERT_TRUE(scene.craftAt(app_, index)) << scene.feedback();
        ASSERT_FALSE(scene.feedback().empty()) << "成也好败也好，都要有一句话";

        const int herbAfter = state().itemCount(kEasyHerb);
        const int productAfter = state().itemCount(kEasyProduct);
        // 炼丹**失败材料尽毁**（rules::failurePolicyOf 的性格差异），所以两条路
        // 上材料都少一株——差别在有没有产物。
        EXPECT_EQ(herbAfter, herbBefore - 1) << "一株清风草该下炉了";
        if (productAfter > productBefore) {
            ++successes;
            EXPECT_EQ(productAfter, productBefore + 1);
        } else {
            ++failures;
            EXPECT_EQ(productAfter, productBefore) << "没成就不该有产物";
        }
        // 失败也长手艺，只是长得慢——两条路上都得涨，否则「炸炉也有所得」这条
        // 设定在游戏里根本看不见。
        EXPECT_GT(state().alchemyProficiency, profBefore)
            << "开了一炉，熟练度一点没涨（成功与否都该涨）";
    }

    EXPECT_GT(successes, 0) << "六十炉一次也没成，成功率那条路是死的";
    EXPECT_GT(failures, 0) << "六十炉一次也没炸，失败那条路是死的";
}

TEST_F(Ch04AlchemyPanelTest, ATalismanFailureSparesTheCatalystWhileAlchemyBurnsEverything) {
    // 失败按 failurePolicyOf 决定扣不扣料（契约第 3.2 节）。四艺的性格差异就
    // 落在这张表上：炼丹尽毁，制符只废纸墨、妖丹保得住。
    // 护身符方挂着占位门闸 story.recipe_later（第 7 章契约 1.2）：开面板取行之前先让它「到手」；判据不变。
    state().setFlag("story.recipe_later");
    const auto found = app_.recipes().find(kTalismanRecipe);
    ASSERT_NE(found, app_.recipes().end());
    AlchemyScene scene(CraftKind::Talisman, 1);
    scene.onEnter(app_);
    const int index = indexOfRecipe(CraftKind::Talisman, kTalismanRecipe);
    ASSERT_GE(index, 0);
    state().talismanProficiency = found->second.requiredProficiency;

    bool sawFailure = false;
    for (int attempt = 0; attempt < 60 && !sawFailure; ++attempt) {
        state().addItem(kPaper, 1, 0);
        state().addItem(kInk, 1, 0);
        state().addItem(kCatalyst, 1, 0);
        const int catalystBefore = state().itemCount(kCatalyst);
        const int paperBefore = state().itemCount(kPaper);
        const int productBefore = state().itemCount("talisman_hushen_fu");

        ASSERT_TRUE(scene.craftAt(app_, index)) << scene.feedback();
        if (state().itemCount("talisman_hushen_fu") > productBefore) continue;   // 这一炉成了

        sawFailure = true;
        EXPECT_EQ(state().itemCount(kCatalyst), catalystBefore)
            << "制符失败废的是纸墨，妖丹是「引」，入不了火也烧不掉";
        EXPECT_EQ(state().itemCount(kPaper), paperBefore - 1) << "符纸该废掉一张";
    }
    EXPECT_TRUE(sawFailure) << "六十次一次也没失败，这条对照就什么也没证明";
}

// ---------------------------------------------------------------------------
// 四、地图上的入口
// ---------------------------------------------------------------------------

TEST_F(Ch04AlchemyPanelTest, AnAlchemyFacilityOpensThePanel) {
    // 契约第 3.3 节：地图上的 facility kind=alchemy，Application::openFacility 加这一支。
    fanren::core::MapObject furnace;
    furnace.name = "facility_danlu";
    furnace.type = "facility";
    furnace.properties["kind"] = "alchemy";

    app_.openFacility(furnace);
    app_.tick(0.0);   // 场景栈在帧末才真的改
    ASSERT_NE(app_.topScene(), nullptr);
    EXPECT_EQ(app_.topScene()->name(), "Alchemy");
}

TEST_F(Ch04AlchemyPanelTest, AFacilityWithoutAGradeStillHasAFurnace) {
    // 缺省品阶取 1 而不是 0：0 在规则层的意思是「没有炉鼎」，于是一处忘了写
    // grade 的丹房会变成一间点不动任何东西的屋子，而玩家站在丹炉前看着
    // 「身边没有丹炉」只会当成 bug。
    EXPECT_EQ(fanren::game::kDefaultCraftToolGrade, 1);
    AlchemyScene scene(CraftKind::Alchemy, fanren::game::kDefaultCraftToolGrade);
    EXPECT_TRUE(fanren::rules::hasToolOfGrade(scene.toolGrade()));

    AlchemyScene noFurnace(CraftKind::Alchemy, 0);
    EXPECT_FALSE(fanren::rules::hasToolOfGrade(noFurnace.toolGrade()));
}

TEST_F(Ch04AlchemyPanelTest, TheProficiencyOfEachCraftLandsOnItsOwnField) {
    // 四个字段散在 GameState 里，写错一个的症状是「炼了一天丹，制符熟练度涨了」。
    state().alchemyProficiency = 11;
    state().talismanProficiency = 22;
    state().forgeProficiency = 33;
    state().formationProficiency = 44;
    EXPECT_EQ(craftProficiency(state(), CraftKind::Alchemy), 11);
    EXPECT_EQ(craftProficiency(state(), CraftKind::Talisman), 22);
    EXPECT_EQ(craftProficiency(state(), CraftKind::Forge), 33);
    EXPECT_EQ(craftProficiency(state(), CraftKind::Formation), 44);

    AlchemyScene scene(CraftKind::Alchemy, 1);
    scene.onEnter(app_);
    state().addItem(kEasyHerb, 1, kEasyHerbAge);
    const int index = indexOfRecipe(CraftKind::Alchemy, kEasyRecipe);
    ASSERT_GE(index, 0);
    ASSERT_TRUE(scene.craftAt(app_, index)) << scene.feedback();

    EXPECT_GT(state().alchemyProficiency, 11) << "炼丹该涨炼丹的";
    EXPECT_EQ(state().talismanProficiency, 22) << "别的三门一点也不该动";
    EXPECT_EQ(state().forgeProficiency, 33);
    EXPECT_EQ(state().formationProficiency, 44);
}

}  // namespace
