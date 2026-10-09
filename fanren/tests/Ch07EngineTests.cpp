// 第 7 章引擎增补（契约 docs/interfaces-p3-ch07.md）：E1 配方按旗标列出、E2 道具施法与「削架势」、
// E3 瓶子容量随境界、E4 按年份下限计数与扣物。
//
// 判据照契约原文的数写死（五张门闸方子、架势 3 削到 0、法力 50 一点不少、44 年 ×2 ＋ 11 年 ×3 ＋ 0 年 ×1），
// 不从被测物推；走得到真实入口的（炼制面板开炉、战斗菜单用符、打坐面板突破、存档读档）就走真实入口。
// 另两处：E3 的脚本升境与 E4 走真 api.lua 的那几条在 tests/ScriptApiTests.cpp（那里有跑真脚本的装置）；
// E7 的曲线锚点在 tests/RealmTests.cpp。
#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include "ChapterFixture.h"
#include "TempDir.h"
#include "core/battle/Battle.h"
#include "core/model/Types.h"
#include "core/rules/Bottle.h"
#include "core/rules/Crafting.h"
#include "core/rules/Cultivation.h"
#include "core/rules/Realm.h"
#include "game/AlchemyScene.h"
#include "game/Application.h"
#include "game/BattleScene.h"
#include "game/CultivationScene.h"
#include "game/Wording.h"
#include "io/DataLoader.h"
#include "io/RecipeLoader.h"
#include "io/SaveFile.h"
#include "script/Command.h"

namespace {

namespace fs = std::filesystem;

using fanren::core::GameData;
using fanren::core::GameState;
using fanren::core::Item;
using fanren::core::Magic;
using fanren::core::MagicEffect;
using fanren::core::battle::Action;
using fanren::core::battle::ActionKind;
using fanren::core::battle::BattleEvent;
using fanren::core::battle::BattleEventKind;
using fanren::core::battle::BattleState;
using fanren::core::battle::Unit;
using fanren::game::AlchemyScene;
using fanren::game::Application;
using fanren::game::BattleMenuMode;
using fanren::game::BattleScene;
using fanren::game::CultivationScene;
using fanren::rules::CraftKind;
using fanren::rules::Realm;
using fanren::rules::RealmTier;
using fanren::test::TempDir;

constexpr int kFist = fanren::core::kCategoryFist;
constexpr int kFire = fanren::core::kCategoryFire;

constexpr const char* kLaterFlag = "story.recipe_later";

std::string assetRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "data" / "items" / "talismans" / "tianleizi.json")) return candidate;
    }
    return ".";
}

void writeFile(const fs::path& path, const std::string& content) {
    fs::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out << content;
}

std::vector<BattleEventKind> kindsOf(const std::vector<BattleEvent>& events) {
    std::vector<BattleEventKind> out;
    for (const BattleEvent& e : events) out.push_back(e.kind);
    return out;
}

bool hasLabel(const std::vector<fanren::ui::ListItem>& rows, const std::string& label) {
    return std::any_of(rows.begin(), rows.end(), [&](const fanren::ui::ListItem& r) { return r.label == label; });
}

int indexOfLabel(const fanren::ui::ListView& list, const std::string& label) {
    for (int i = 0; i < list.count(); ++i) {
        if (list.items()[static_cast<std::size_t>(i)].label == label) return i;
    }
    return -1;
}

// ===========================================================================
// E1 配方按旗标列出（契约第 1 节）
// ===========================================================================

// 1.6 第 1 条：requireFlag 读得进；空串、数字各报错且点名文件与字段。
TEST(Ch07RecipeGateData, RequireFlagIsReadAndAnEmptyOrNumericOneIsRefused) {
    const std::string head = R"({"id":"recipe_probe","name":"探针方","kind":"alchemy","productId":"pill_probe",)"
                             R"("inputs":[{"itemId":"herb_probe","count":1}])";
    TempDir tmp{"fanren_ch07_recipe"};
    const fs::path file = tmp.path() / "recipe_probe.json";

    writeFile(file, head + R"(,"requireFlag":"probe.flag"})");
    const auto gated = fanren::io::loadRecipe(file.string());
    ASSERT_TRUE(gated.ok) << gated.error;
    EXPECT_EQ(gated.value.requireFlag, "probe.flag");

    writeFile(file, head + "}");
    const auto plain = fanren::io::loadRecipe(file.string());
    ASSERT_TRUE(plain.ok) << plain.error;
    EXPECT_TRUE(plain.value.requireFlag.empty()) << "不写就是没有门闸：现有方子行为零变化";

    for (const char* bad : {R"(,"requireFlag":""})", R"(,"requireFlag":3})"}) {
        writeFile(file, head + bad);
        const auto loaded = fanren::io::loadRecipe(file.string());
        EXPECT_FALSE(loaded.ok) << bad << "：悄悄收下，这张方子就成了谁都看得见";
        EXPECT_NE(loaded.error.find("requireFlag"), std::string::npos) << loaded.error;
        EXPECT_NE(loaded.error.find("recipe_probe.json"), std::string::npos) << loaded.error;
    }
}

// 1.3 / 1.6 第 3 条：未得的方子理由是「手边还没有这张方子」，而且排在炉鼎之前。
TEST(Ch07RecipeGateRule, AnUnknownRecipeSaysSoBeforeTheFurnace) {
    fanren::rules::Recipe recipe;
    recipe.id = "recipe_probe";
    recipe.name = "探针方";
    recipe.kind = CraftKind::Alchemy;
    recipe.productId = "pill_probe";
    recipe.inputs = {{"herb_probe", 1, 0}};
    recipe.requireFlag = "probe.flag";
    GameState state;
    state.addItem("herb_probe", 1, 0);

    const auto noFurnace = fanren::rules::canCraft(recipe, state, 100, /*hasTool=*/false);
    ASSERT_FALSE(noFurnace.ok);
    EXPECT_EQ(noFurnace.error, "手边还没有这张方子。") << "无炉 ＋ 未得：先报未得";
    EXPECT_FALSE(fanren::rules::recipeKnown(recipe, state));

    state.setFlag("probe.flag");
    EXPECT_TRUE(fanren::rules::recipeKnown(recipe, state));
    const auto known = fanren::rules::canCraft(recipe, state, 100, /*hasTool=*/false);
    ASSERT_FALSE(known.ok);
    EXPECT_EQ(known.error, std::string(fanren::rules::toolMissingMessage(CraftKind::Alchemy)))
        << "到手之后照旧：没炉子就报炉子";
    EXPECT_TRUE(fanren::rules::canCraft(recipe, state, 100, /*hasTool=*/true).ok) << "配一条肯定：有炉有料就开得了工";
}

class Ch07RecipePanel : public ::testing::Test {
protected:
    void SetUp() override {
        auto ready = app_.init(assetRoot(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
    }
    void TearDown() override { app_.shutdown(); }

    std::vector<std::string> visibleIds(CraftKind kind) {
        std::vector<std::string> ids;
        for (const auto* recipe : AlchemyScene::visibleRecipes(app_.recipes(), kind, app_.state())) {
            ids.push_back(recipe->id);
        }
        return ids;
    }

    std::vector<fanren::ui::ListItem> rows(CraftKind kind) {
        return AlchemyScene::buildRecipeItems(app_.data(), app_.state(), kind, 1,
                                              AlchemyScene::visibleRecipes(app_.recipes(), kind, app_.state()));
    }

    // 面板真开一次：列表的行号 → 方子 id 那张表（recipeIds_）就是这时候建的。
    std::vector<std::string> panelIds(CraftKind kind) {
        AlchemyScene scene(kind, 1);
        scene.onEnter(app_);
        return scene.recipeIds();
    }

    Application app_;
};

// 1.6 第 1 条后半：五份门闸方子读出来都是 story.recipe_later，定神符方没有门闸。
TEST_F(Ch07RecipePanel, TheFiveGatedRecipesCarryThePlaceholderAndTheDingshenOneDoesNot) {
    for (const char* id : {"recipe_jiedan_lingyao", "recipe_huoqiu_fu", "recipe_hushen_fu", "recipe_jinci_fu",
                           "recipe_leiming_fu"}) {
        const auto found = app_.recipes().find(id);
        ASSERT_NE(found, app_.recipes().end()) << id;
        EXPECT_EQ(found->second.requireFlag, kLaterFlag) << id;
    }
    const auto dingshen = app_.recipes().find("recipe_dingshen_fu");
    ASSERT_NE(dingshen, app_.recipes().end());
    EXPECT_TRUE(dingshen->second.requireFlag.empty()) << "第 6 章的定神符方不加门闸";
}

// 1.6 第 2 条：第 6 章终局那份存档（story.recipe_later 未置），两个面板都不露那五张；置了之后都在。
// 两态下列表行数、面板里行号 → 方子 id 那张表，都与 visibleRecipes 同源。
TEST_F(Ch07RecipePanel, TheChapterSixSaveSeesNoneOfTheFiveAndTheFlagBringsThemAllBack) {
    const fs::path fixture =
        fanren::test::chapterFixturePath(assetRoot(), fanren::test::kChapterSixEndingFirst);
    auto loaded = fanren::io::loadGame(fixture.string());
    ASSERT_TRUE(loaded.ok) << loaded.error;
    app_.state() = loaded.value;
    ASSERT_EQ(app_.state().flag(kLaterFlag), 0) << "先验：第 6 章终局没置这面旗";

    const auto contains = [](const std::vector<std::string>& ids, const std::string& id) {
        return std::find(ids.begin(), ids.end(), id) != ids.end();
    };
    const std::vector<std::string> talismanGated{"recipe_huoqiu_fu", "recipe_hushen_fu", "recipe_jinci_fu",
                                                 "recipe_leiming_fu"};

    for (const int flag : {0, 1}) {
        app_.state().setFlag(kLaterFlag, flag);
        const bool shown = flag != 0;

        const std::vector<std::string> alchemy = visibleIds(CraftKind::Alchemy);
        EXPECT_EQ(contains(alchemy, "recipe_jiedan_lingyao"), shown) << "旗标 " << flag << "：结丹灵药方";
        EXPECT_EQ(hasLabel(rows(CraftKind::Alchemy), "结丹灵药方"), shown) << "旗标 " << flag;
        EXPECT_TRUE(contains(alchemy, "recipe_jinchuang_yao")) << "没有门闸的方子照旧在";

        const std::vector<std::string> talisman = visibleIds(CraftKind::Talisman);
        for (const std::string& id : talismanGated) {
            EXPECT_EQ(contains(talisman, id), shown) << "旗标 " << flag << "：" << id;
        }
        EXPECT_TRUE(contains(talisman, "recipe_dingshen_fu")) << "定神符方两态都在";

        for (const CraftKind kind : {CraftKind::Alchemy, CraftKind::Talisman}) {
            const std::vector<std::string> ids = visibleIds(kind);
            EXPECT_EQ(rows(kind).size(), ids.size() + 1) << "行数与 visibleRecipes 同源（末项「离开」）";
            EXPECT_EQ(panelIds(kind), ids) << "面板里行号 → 方子 id 那张表与列表同源";
        }
    }
    EXPECT_EQ(visibleIds(CraftKind::Talisman).size(), 5u) << "置了之后制符面板五张都在";
}

// 1.6 第 4 条：同一个面板，旗标 0 / 1 两态下，craftAt(i) 开的都是第 i 行写的那张方子。
// 判据是开炉那一句（成与不成都带着「《方子名》」）：它说出口的方子，就是真开的那一张。
TEST_F(Ch07RecipePanel, EveryRowOpensTheRecipeItNamesWithTheGateShutAndOpen) {
    for (const int flag : {0, 1}) {
        GameState& state = app_.state();
        state = GameState{};
        state.setFlag(kLaterFlag, flag);
        state.talismanProficiency = 100;
        // 每一张方子的料各备一份：哪一行开出来都不会卡在缺料上。
        for (const auto& [id, recipe] : app_.recipes()) {
            if (recipe.kind != CraftKind::Talisman) continue;
            for (const auto& need : recipe.inputs) state.addItem(need.itemId, need.count, need.minAge);
        }

        AlchemyScene scene(CraftKind::Talisman, 1);
        scene.onEnter(app_);
        const std::vector<fanren::ui::ListItem> items = rows(CraftKind::Talisman);
        ASSERT_EQ(items.size(), scene.recipeIds().size() + 1);
        ASSERT_EQ(scene.recipeIds().size(), flag != 0 ? 5u : 1u) << "先验：两态的行数真的不一样";
        for (std::size_t i = 0; i < scene.recipeIds().size(); ++i) {
            ASSERT_TRUE(scene.craftAt(app_, static_cast<int>(i))) << "第 " << i << " 行：" << scene.feedback();
            EXPECT_NE(scene.feedback().find("《" + items[i].label + "》"), std::string::npos)
                << "旗标 " << flag << "，第 " << i << " 行写的是「" << items[i].label << "」，开出来的却是："
                << scene.feedback();
        }
    }
}

// ===========================================================================
// E2 道具施法与「削架势」（契约第 2 节）
// ===========================================================================

// ---- 数据（2.6 第 1 条）----

TEST_F(Ch07RecipePanel, TheFourTalismansCastWhatTheContractSays) {
    // 夹具借用上面那一组的 Application：只读 data。
    const GameData& data = app_.data();
    const struct {
        const char* item;
        const char* magic;
    } expected[]{{"talisman_tianleizi", "magic_tianleizi"},
                 {"talisman_tulao_fu", "magic_tulao_shu"},
                 {"talisman_huoqiu_fu", "magic_huoqiu_shu"},
                 {"talisman_dingshen_fu", "magic_dingshen_shu"}};
    for (const auto& row : expected) {
        const Item* item = data.findItem(row.item);
        ASSERT_NE(item, nullptr) << row.item;
        EXPECT_EQ(item->castMagic, row.magic) << row.item;
        EXPECT_TRUE(fanren::core::battleUsable(*item)) << row.item << " 进得了战斗";
        EXPECT_EQ(item->kind, fanren::core::ItemKind::Talisman) << row.item;
    }
    EXPECT_FALSE(data.findItem("talisman_tianleizi")->tradeable);
    EXPECT_FALSE(data.findItem("talisman_tulao_fu")->tradeable);

    const Magic* tulao = data.findMagic("magic_tulao_shu");
    ASSERT_NE(tulao, nullptr);
    EXPECT_EQ(tulao->effect, MagicEffect::Stagger);
    EXPECT_EQ(tulao->stagger, 3);
    EXPECT_EQ(tulao->element, fanren::core::kElementEarth);
    EXPECT_EQ(tulao->power, 0);
    EXPECT_EQ(tulao->needMp, 0);

    const Magic* dingshen = data.findMagic("magic_dingshen_shu");
    ASSERT_NE(dingshen, nullptr);
    EXPECT_EQ(dingshen->effect, MagicEffect::Stagger);
    EXPECT_EQ(dingshen->stagger, 1);
    EXPECT_EQ(dingshen->element, fanren::core::kElementNone);
    EXPECT_EQ(dingshen->power, 0);

    const Magic* tianlei = data.findMagic("magic_tianleizi");
    ASSERT_NE(tianlei, nullptr);
    EXPECT_EQ(tianlei->effect, MagicEffect::None) << "天雷子是伤人的";
    EXPECT_EQ(tianlei->element, fanren::core::kElementFire);
    EXPECT_GT(tianlei->power, 0);
    EXPECT_EQ(tianlei->needMp, 0);
}

// 一件探针符：extra 拼进物品对象里。同一份夹具里摆一门伤人的与一门护身罡那样的（不伤人、没有效果）。
fanren::core::Result<GameData> loadProbeTalisman(const TempDir& tmp, const std::string& extra) {
    writeFile(tmp.path() / "data" / "magics" / "probe_bolt.json",
              R"({"id":"probe_bolt","name":"探针雷","element":8,"needMp":0,"power":30})");
    writeFile(tmp.path() / "data" / "magics" / "probe_shield.json",
              R"({"id":"probe_shield","name":"探针罡","element":0,"needMp":10,"power":0})");
    std::string json = R"({"id":"probe_fu","name":"探针符","kind":"talisman")";
    if (!extra.empty()) json += "," + extra;
    writeFile(tmp.path() / "data" / "items" / "probe_fu.json", json + "}");
    return fanren::io::loadGameData((tmp.path() / "data").string());
}

TEST(Ch07ItemCastData, ACastMagicThatIsMisspelledAShieldOrBesideAPotionIsRefused) {
    TempDir good{"fanren_ch07_cast"};
    const auto ok = loadProbeTalisman(good, R"("castMagic":"probe_bolt")");
    ASSERT_TRUE(ok.ok) << ok.error;
    EXPECT_EQ(ok.value.findItem("probe_fu")->castMagic, "probe_bolt") << "先验：写对了读得进来";

    for (const char* bad : {R"("castMagic":"probe_bu_cun_zai")", R"("castMagic":"probe_shield")",
                            R"("castMagic":"probe_bolt","restoreHp":10)", R"("castMagic":"")"}) {
        TempDir tmp{"fanren_ch07_cast"};
        const auto loaded = loadProbeTalisman(tmp, bad);
        EXPECT_FALSE(loaded.ok) << bad;
        EXPECT_NE(loaded.error.find("castMagic"), std::string::npos) << loaded.error;
        EXPECT_NE(loaded.error.find("probe_fu.json"), std::string::npos) << loaded.error;
    }
}

fanren::core::Result<GameData> loadProbeMagic(const TempDir& tmp, int power, const std::string& extra) {
    std::string json = R"({"id":"probe_magic","name":"探针术","element":0,"needMp":5,"power":)" +
                       std::to_string(power);
    if (!extra.empty()) json += "," + extra;
    writeFile(tmp.path() / "data" / "magics" / "probe_magic.json", json + "}");
    return fanren::io::loadGameData((tmp.path() / "data").string());
}

TEST(Ch07ItemCastData, StaggerIsOneToNineAndOnlyBesideEffectStagger) {
    TempDir plain{"fanren_ch07_stagger"};
    const auto a = loadProbeMagic(plain, 0, R"("effect":"stagger")");
    ASSERT_TRUE(a.ok) << a.error;
    EXPECT_EQ(a.value.findMagic("probe_magic")->effect, MagicEffect::Stagger);
    EXPECT_EQ(a.value.findMagic("probe_magic")->stagger, 1) << "effect stagger 而不写 stagger → 取 1";

    TempDir three{"fanren_ch07_stagger"};
    const auto b = loadProbeMagic(three, 0, R"("effect":"stagger","stagger":3)");
    ASSERT_TRUE(b.ok) << b.error;
    EXPECT_EQ(b.value.findMagic("probe_magic")->stagger, 3);

    struct Bad {
        int power;
        const char* extra;
        const char* field;
    };
    for (const Bad& bad : {Bad{0, R"("effect":"stagger","stagger":0)", "stagger"},
                           Bad{0, R"("effect":"stagger","stagger":10)", "stagger"},
                           Bad{0, R"("effect":"stagger","stagger":"3")", "stagger"},
                           Bad{0, R"("stagger":3)", "stagger"},
                           Bad{12, R"("effect":"stagger","stagger":3)", "effect"},
                           Bad{0, R"("effect":"Stagger")", "effect"}}) {
        TempDir tmp{"fanren_ch07_stagger"};
        const auto loaded = loadProbeMagic(tmp, bad.power, bad.extra);
        EXPECT_FALSE(loaded.ok) << bad.extra << " power " << bad.power;
        EXPECT_NE(loaded.error.find(bad.field), std::string::npos) << loaded.error;
        EXPECT_NE(loaded.error.find("probe_magic.json"), std::string::npos) << loaded.error;
    }
}

// ---- 规则（2.6 第 2–4 条）：就地造的场子 ----

// 测试里的两门法术**法力写成非 0**：数据里它们是 0，道具施法扣不扣法力就看不出来（负向自检要咬得住）。
Magic tianleiSpell() {
    Magic m;
    m.id = "magic_tianleizi";
    m.name = "天雷子";
    m.element = fanren::core::kElementFire;
    m.needMp = 12;
    m.power = 60;
    return m;
}

Magic tulaoSpell() {
    Magic m;
    m.id = "magic_tulao_shu";
    m.name = "土牢术";
    m.element = fanren::core::kElementEarth;
    m.needMp = 6;
    m.power = 0;
    m.effect = MagicEffect::Stagger;
    m.stagger = 3;
    return m;
}

Item talisman(const std::string& id, const std::string& name, const std::string& castMagic) {
    Item item;
    item.id = id;
    item.name = name;
    item.kind = fanren::core::ItemKind::Talisman;
    item.castMagic = castMagic;
    return item;
}

Unit hero() {
    Unit u;
    u.id = "han_li";
    u.name = "韩立";
    u.hp = u.maxHp = 150;
    u.mp = u.maxMp = 50;
    u.attack = 10;
    u.defence = 5;
    u.speed = 9;   // 最快：每回合头一个出手
    u.realm = Realm::QiRefining11;
    u.ally = true;
    u.weapons = kFist;
    u.magicsExhaustive = true;   // 一门也没学：用符不看习得
    return u;
}

Unit foe(int toughness, int weaknesses) {
    Unit u;
    u.id = "probe_foe";
    u.name = "探针敌";
    u.hp = u.maxHp = 400;
    u.attack = 10;
    u.defence = 5;
    u.speed = 2;
    u.realm = Realm::QiRefining11;
    u.weapons = kFist;
    u.maxToughness = toughness;
    u.toughness = toughness;
    u.weaknesses = weaknesses;
    u.magicsExhaustive = true;
    return u;
}

BattleState talismanField(const Unit& enemy) {
    BattleState state;
    state.setup({hero(), enemy}, 11);
    state.addMagic(tianleiSpell());
    state.addMagic(tulaoSpell());
    state.addItem(talisman("talisman_tianleizi", "天雷子", "magic_tianleizi"));
    state.addItem(talisman("talisman_tulao_fu", "土牢符", "magic_tulao_shu"));
    return state;
}

Action use(const std::string& itemId, int target, int boost = 0) {
    Action a;
    a.kind = ActionKind::Item;
    a.actorIndex = 0;
    a.targetIndex = target;
    a.magicId = itemId;   // 物品借用这个槽位
    a.boost = boost;
    return a;
}

// 推到韩立的下一手（中间的敌人不出手）。
void toHeroAgain(BattleState& state) {
    const int from = state.round();
    for (int guard = 0; guard < 64 && (state.round() == from || state.currentActor() != 0); ++guard) state.endTurn();
}

TEST(Ch07ItemCast, TianleiziThenTulaoBreaksTheFoeAndTheManaNeverMoves) {
    BattleState state = talismanField(foe(3, kFire));
    ASSERT_EQ(state.currentActor(), 0) << "先验：韩立先手";

    // 天雷子：按那门法术结算——掉血、打中火揭开、架势 −1；法力一点不少。
    const auto first = state.apply(use("talisman_tianleizi", 1));
    ASSERT_TRUE(first.ok) << first.error;
    const Unit& enemy = state.units()[1];
    EXPECT_LT(enemy.hp, enemy.maxHp) << "天雷子是伤人的";
    EXPECT_EQ(enemy.revealed, kFire) << "「火」揭开";
    EXPECT_EQ(enemy.toughness, 2) << "打中破绽削一点";
    EXPECT_EQ(state.units()[0].mp, 50) << "道具施法不耗法力";
    EXPECT_NE(first.value.find("韩立 使用 天雷子——"), std::string::npos) << first.value;
    const std::vector<BattleEvent> hit = state.lastActionEvents();
    ASSERT_FALSE(hit.empty());
    EXPECT_EQ(hit.front().kind, BattleEventKind::Act);
    EXPECT_EQ(hit.front().value, static_cast<int>(ActionKind::Item)) << "Act 那一行记的是 Item";
    EXPECT_EQ(hit.front().category, kFire) << "画面拿它挑那门法术的施法光";
    EXPECT_EQ(kindsOf(hit), (std::vector<BattleEventKind>{BattleEventKind::Act, BattleEventKind::Hit}));

    // 土牢符：架势 2 → 0，破势；不伤人、不出伤害数字；法力仍 50。
    state.endTurn();
    toHeroAgain(state);
    ASSERT_EQ(state.currentActor(), 0);
    const int hpBefore = state.units()[1].hp;
    const auto second = state.apply(use("talisman_tulao_fu", 1));
    ASSERT_TRUE(second.ok) << second.error;
    EXPECT_EQ(state.units()[1].toughness, 0);
    EXPECT_TRUE(state.units()[1].broken()) << "削到 0 就破势";
    EXPECT_EQ(state.units()[1].hp, hpBefore) << "削架势不伤人";
    EXPECT_EQ(state.units()[0].mp, 50) << "道具施法不耗法力";
    const std::vector<BattleEvent> cut = state.lastActionEvents();
    ASSERT_EQ(kindsOf(cut), (std::vector<BattleEventKind>{BattleEventKind::Act, BattleEventKind::Stagger,
                                                          BattleEventKind::Break}));
    EXPECT_EQ(cut[1].value, 2) << "削掉的点数（架势只剩 2）";
    EXPECT_EQ(cut[1].toughness, 0) << "削后的架势";
    EXPECT_EQ(cut[2].value, fanren::core::battle::kBreakRecoverAfterRounds) << "与打中破绽削到 0 同样的破势回合数";
    EXPECT_NE(second.value.find("破势"), std::string::npos) << second.value;
}

TEST(Ch07ItemCast, AStaggerOnABrokenFoeOrOneWithoutStanceIsRefusedFirst) {
    // 没有架势的：拦在前头，说清楚；背包闸在前、规则闸在后，理由原样上屏（refusePlayerAction）。
    BattleState bare = talismanField(foe(0, 0));
    std::string why;
    EXPECT_FALSE(bare.isLegal(use("talisman_tulao_fu", 1), &why));
    EXPECT_NE(why.find("没有架势可削"), std::string::npos) << why;
    GameState bag;
    bag.setFlag(fanren::game::kXiuxianKnownFlag);
    bag.addItem("talisman_tulao_fu", 1, 0);
    const std::string refused = fanren::game::refusePlayerAction(bag, bare, use("talisman_tulao_fu", 1));
    EXPECT_NE(refused.find("没有架势可削"), std::string::npos) << refused;
    EXPECT_TRUE(bare.isLegal(use("talisman_tianleizi", 1))) << "伤人的那一件不看架势";

    // 已经破势的：同样拦在前头。
    BattleState state = talismanField(foe(3, kFire));
    ASSERT_TRUE(state.apply(use("talisman_tulao_fu", 1)).ok);
    ASSERT_TRUE(state.units()[1].broken());
    state.endTurn();
    toHeroAgain(state);
    ASSERT_TRUE(state.units()[1].broken()) << "先验：下一手时它还在破势里";
    EXPECT_FALSE(state.isLegal(use("talisman_tulao_fu", 1), &why));
    EXPECT_NE(why.find("已经破势"), std::string::npos) << why;
}

TEST(Ch07ItemCast, ABoostedTalismanIsRefusedWithTheOldSentence) {
    BattleState state = talismanField(foe(3, kFire));
    state.endTurn();
    toHeroAgain(state);
    ASSERT_GE(state.units()[0].bp, 2) << "先验：劲够蓄 2 点，回绝只可能是因为它是物品";
    std::string why;
    EXPECT_FALSE(state.isLegal(use("talisman_tianleizi", 1, 2), &why));
    EXPECT_EQ(why, "只有攻击与法术蓄得了劲");
}

// 一门就地造的削架势法术（学过的）：蓄了劲也只当没蓄——效果一样、劲不扣、不算蓄过。
TEST(Ch07Stagger, BoostingAStaggerSpellDoesNothingAndCostsNothing) {
    Magic spell = tulaoSpell();
    spell.id = "probe_stagger";
    spell.name = "探针困";
    spell.stagger = 2;
    const auto castAt = [&spell](int boost) {
        Unit caster = hero();
        caster.magics = {spell.id};
        BattleState state;
        state.setup({caster, foe(3, kFire)}, 5);
        state.addMagic(spell);
        state.endTurn();
        toHeroAgain(state);
        EXPECT_EQ(state.units()[0].bp, 2) << "先验：第 2 回合开始，劲 1 + 1";
        Action cast;
        cast.kind = ActionKind::Cast;
        cast.actorIndex = 0;
        cast.targetIndex = 1;
        cast.magicId = spell.id;
        cast.boost = boost;
        const auto result = state.apply(cast);
        EXPECT_TRUE(result.ok) << result.error;
        return state;
    };
    const BattleState plain = castAt(0);
    const BattleState boosted = castAt(2);
    EXPECT_EQ(boosted.units()[1].toughness, 1) << "3 − 2：蓄了劲也只削 2 点";
    EXPECT_EQ(boosted.units()[1].toughness, plain.units()[1].toughness);
    EXPECT_EQ(boosted.units()[0].bp, 2) << "劲一点没扣";
    EXPECT_FALSE(boosted.units()[0].boosted) << "下一回合照常加劲";
    EXPECT_EQ(boosted.units()[0].mp, plain.units()[0].mp) << "法力照扣，扣的一样多";
    EXPECT_EQ(plain.units()[0].mp, 50 - spell.needMp) << "施法（不是用符）要扣法力";
    EXPECT_EQ(kindsOf(boosted.lastActionEvents()), kindsOf(plain.lastActionEvents())) << "没有 BoostSpent";
}

// 我方 AI 不施削架势的法术：拳试过了、只剩「火」没试，而会的那门削架势正好是火——
// 它不判破绽，拿它去「探」只会白打一手，还永远探不出来。
TEST(Ch07Stagger, TheAllyAiNeverReachesForAStaggerSpell) {
    Magic spell = tulaoSpell();
    spell.id = "probe_stagger_fire";
    spell.name = "探针火困";
    spell.element = fanren::core::kElementFire;
    Unit caster = hero();
    caster.magics = {spell.id};
    Unit target = foe(3, kFire);
    target.tested = kFist;   // 拳已经砍过了，没揭开什么
    BattleState state;
    state.setup({caster, target}, 9);
    state.addMagic(spell);
    Action cast;
    cast.kind = ActionKind::Cast;
    cast.actorIndex = 0;
    cast.targetIndex = 1;
    cast.magicId = spell.id;
    ASSERT_TRUE(state.isLegal(cast)) << "先验：它施展得了";
    const Action decided = state.decideAi(0);
    EXPECT_EQ(decided.kind, ActionKind::Attack) << "施展了削架势：" << decided.magicId;
}

// 装着看破的符（本章没有这样的数据，契约 2.3 给了口径）：不挑目标、不耗法力、全场揭开。
TEST(Ch07ItemCast, ARevealTalismanLaysTheFieldBareWithoutMana) {
    Magic eye;
    eye.id = "probe_eye";
    eye.name = "探针眼";
    eye.needMp = 5;
    eye.power = 0;
    eye.effect = MagicEffect::Reveal;
    BattleState state;
    state.setup({hero(), foe(3, kFire)}, 3);
    state.addMagic(eye);
    state.addItem(talisman("probe_eye_fu", "探针眼符", "probe_eye"));
    const auto result = state.apply(use("probe_eye_fu", -1));
    ASSERT_TRUE(result.ok) << result.error;
    EXPECT_EQ(state.units()[1].revealed, kFire);
    EXPECT_EQ(state.units()[0].mp, 50);
    EXPECT_NE(result.value.find("韩立 使用 探针眼符——探针敌 的破绽"), std::string::npos) << result.value;
}

// ---- 真入口（2.6 第 3、5、6 条）：既有编成 b04_qiecuo_maliu（马六架势 2、卢春架势 2）----

constexpr const char* kTwoFoeBattle = "b04_qiecuo_maliu";

class Ch07ItemCastScene : public ::testing::Test {
protected:
    void SetUp() override {
        auto ready = app_.init(assetRoot(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
        GameState& s = app_.state();
        s.setFlag(fanren::game::kXiuxianKnownFlag);   // 第 7 章是修仙用词
        s.realm = Realm::QiRefining11;
        s.hp = s.maxHp = 156;
        s.mp = s.maxMp = 110;
    }
    void TearDown() override { app_.shutdown(); }

    GameState& state() { return app_.state(); }

    // 菜单上把一件东西用到指定的人身上：物品 → 那一行 → 择敌 → 那个人。返回那一行的说明（行尾）。
    std::string useFromMenu(BattleScene& scene, const std::string& itemName, const std::string& targetName) {
        const int actor = scene.runToAllyTurn();
        EXPECT_GE(actor, 0);
        scene.openMenu(app_);
        EXPECT_TRUE(scene.menuChoose(app_, fanren::game::kBattleMenuItem));
        const int row = indexOfLabel(scene.menuList(), itemName);
        EXPECT_GE(row, 0) << "物品列表里没有「" << itemName << "」";
        if (row < 0) return {};
        const fanren::ui::ListItem entry = scene.menuList().items()[static_cast<std::size_t>(row)];
        EXPECT_TRUE(entry.enabled) << entry.disabledReason;
        EXPECT_TRUE(scene.menuChoose(app_, row));
        EXPECT_EQ(scene.menuMode(), BattleMenuMode::Target) << "伤人的与削架势的都要择敌";
        const int who = indexOfLabel(scene.menuList(), targetName);
        EXPECT_GE(who, 0) << "择敌列表里没有「" << targetName << "」";
        if (who < 0) return entry.detail;
        EXPECT_TRUE(scene.menuChoose(app_, who)) << scene.feedback();
        EXPECT_EQ(scene.menuMode(), BattleMenuMode::Closed);
        return entry.detail;
    }

    std::string foeStanding(const BattleScene& scene) const {
        for (const Unit& u : scene.battle().units()) {
            if (!u.ally && u.alive()) return u.name;
        }
        return {};
    }

    static bool logHas(const BattleState& battle, const std::string& needle) {
        return std::any_of(battle.log().begin(), battle.log().end(),
                           [&](const std::string& line) { return line.find(needle) != std::string::npos; });
    }

    Application app_;
};

// 2.6 第 5 条：天雷子一件、火球符一件，走真菜单用掉——背包各少一件、法力一点不少、日志里有「使用 天雷子」。
TEST_F(Ch07ItemCastScene, BothTalismansGoThroughTheRealMenuAndOnlyTheBagPays) {
    state().addItem("talisman_tianleizi", 1, 0);
    state().addItem("talisman_huoqiu_fu", 1, 0);
    BattleScene scene(kTwoFoeBattle);
    scene.onEnter(app_);
    ASSERT_TRUE(scene.battle().hasItem("talisman_tianleizi")) << "带 castMagic 的符登记进了这一场";
    ASSERT_TRUE(scene.battle().hasMagic("magic_tianleizi")) << "它施展的那门法术一起登记";
    const int mpBefore = scene.battle().units()[0].mp;

    const std::string detail = useFromMenu(scene, "天雷子", foeStanding(scene));
    EXPECT_NE(detail.find("火"), std::string::npos) << "行尾写那门法术的类别：" << detail;
    EXPECT_EQ(detail.find("耗"), std::string::npos) << "不写法力：" << detail;
    EXPECT_EQ(state().itemCount("talisman_tianleizi"), 0) << "用掉一件背包就少一件";
    EXPECT_TRUE(logHas(scene.battle(), "使用 天雷子——"));
    EXPECT_EQ(scene.battle().units()[0].mp, mpBefore) << "一点法力也不耗";

    ASSERT_FALSE(foeStanding(scene).empty()) << "先验：还有人站着，火球符有处可用";
    useFromMenu(scene, "火球符", foeStanding(scene));
    EXPECT_EQ(state().itemCount("talisman_huoqiu_fu"), 0);
    EXPECT_TRUE(logHas(scene.battle(), "使用 火球符——"));
    EXPECT_EQ(scene.battle().units()[0].mp, mpBefore) << "火球术施法要 8 点，用符一点不耗";
}

// 2.6 第 3 条：对已破势的敌人用土牢符——回绝、理由含「已经破势」、背包件数不变（走 BattleScene 真入口）。
TEST_F(Ch07ItemCastScene, TulaoOnABrokenFoeIsRefusedAndTheBagKeepsIt) {
    state().addItem("talisman_tulao_fu", 2, 0);
    BattleScene scene(kTwoFoeBattle);
    scene.onEnter(app_);
    const std::string maliu = app_.data().findRole("tongmen_maliu")->name;

    const std::string detail = useFromMenu(scene, "土牢符", maliu);
    EXPECT_NE(detail.find("削架势 3"), std::string::npos) << detail;
    EXPECT_EQ(state().itemCount("talisman_tulao_fu"), 1);
    int index = -1;
    for (std::size_t i = 0; i < scene.battle().units().size(); ++i) {
        if (scene.battle().units()[i].name == maliu) index = static_cast<int>(i);
    }
    ASSERT_GE(index, 0);
    ASSERT_TRUE(scene.battle().units()[static_cast<std::size_t>(index)].broken()) << "架势 2 被削 3：破势";

    ASSERT_GE(scene.runToAllyTurn(), 0);
    ASSERT_TRUE(scene.battle().units()[static_cast<std::size_t>(index)].broken()) << "先验：下一手时它还在破势里";
    Action again;
    again.kind = ActionKind::Item;
    again.actorIndex = scene.battle().currentActor();
    again.targetIndex = index;
    again.magicId = "talisman_tulao_fu";
    const auto refused = scene.issuePlayerAction(app_, again);
    EXPECT_FALSE(refused.ok);
    EXPECT_NE(refused.error.find("已经破势"), std::string::npos) << refused.error;
    EXPECT_EQ(state().itemCount("talisman_tulao_fu"), 1) << "被回绝的动作不扣";

    // 菜单上同一句话：择敌那一行置灰，理由就是它。
    scene.openMenu(app_);
    ASSERT_TRUE(scene.menuChoose(app_, fanren::game::kBattleMenuItem));
    ASSERT_TRUE(scene.menuChoose(app_, indexOfLabel(scene.menuList(), "土牢符")));
    const int row = indexOfLabel(scene.menuList(), maliu);
    ASSERT_GE(row, 0);
    const fanren::ui::ListItem target = scene.menuList().items()[static_cast<std::size_t>(row)];
    EXPECT_FALSE(target.enabled);
    EXPECT_NE(target.disabledReason.find("已经破势"), std::string::npos) << target.disabledReason;
}

// 定神符：一张削一点架势，不伤人，扣一件（第 6 章设计 10.1 E2 的口径）。
TEST_F(Ch07ItemCastScene, ADingshenTalismanTakesOnePointOfStanceAndNoBlood) {
    state().addItem("talisman_dingshen_fu", 1, 0);
    BattleScene scene(kTwoFoeBattle);
    scene.onEnter(app_);
    const std::string luchun = app_.data().findRole("tongmen_luchun")->name;
    int index = -1;
    for (std::size_t i = 0; i < scene.battle().units().size(); ++i) {
        if (scene.battle().units()[i].name == luchun) index = static_cast<int>(i);
    }
    ASSERT_GE(index, 0);
    const int toughness = scene.battle().units()[static_cast<std::size_t>(index)].toughness;
    ASSERT_GE(toughness, 2) << "先验：一点削不到破势";

    const std::string detail = useFromMenu(scene, "定神符", luchun);
    EXPECT_NE(detail.find("削架势 1"), std::string::npos) << detail;
    const Unit& after = scene.battle().units()[static_cast<std::size_t>(index)];
    EXPECT_EQ(after.toughness, toughness - 1);
    EXPECT_EQ(after.hp, after.maxHp) << "不伤人";
    EXPECT_EQ(state().itemCount("talisman_dingshen_fu"), 0);
}

// 法术栏里一门削架势的法术（内容路的祭青凝镜就是这一种）：点下去要择敌，不能像看破那样不挑目标直接发——
// 从前菜单对「带效果的法术」一律直接发，削架势的会被当成看破、无目标发出去再被回绝。
// 行尾写「削架势 N」、不写「蓄劲加威」。这里借定神术当学会的那一门（只为走菜单，不是剧情）。
TEST_F(Ch07ItemCastScene, AStaggerSpellFromTheSpellMenuAsksForATarget) {
    ASSERT_TRUE(state().learnMagic("magic_dingshen_shu"));
    BattleScene scene(kTwoFoeBattle);
    scene.onEnter(app_);
    ASSERT_GE(scene.runToAllyTurn(), 0);
    scene.openMenu(app_);
    ASSERT_TRUE(scene.menuChoose(app_, fanren::game::kBattleMenuCast));
    const int row = indexOfLabel(scene.menuList(), "定神术");
    ASSERT_GE(row, 0) << "法术栏里没有定神术";
    const fanren::ui::ListItem entry = scene.menuList().items()[static_cast<std::size_t>(row)];
    EXPECT_TRUE(entry.enabled) << entry.disabledReason;
    EXPECT_NE(entry.detail.find("削架势 1"), std::string::npos) << entry.detail;
    EXPECT_EQ(entry.detail.find("蓄劲"), std::string::npos) << "削架势不吃劲：" << entry.detail;
    ASSERT_TRUE(scene.menuChoose(app_, row)) << scene.feedback();
    EXPECT_EQ(scene.menuMode(), BattleMenuMode::Target) << "削架势要挑一个敌人：" << scene.feedback();
}

// 2.6 第 6 条：无头 runToCompletion 跑一场背包里有天雷子与土牢符的仗，两件一件不少，也没有一手是用物品。
TEST_F(Ch07ItemCastScene, TheHeadlessAllyNeverSpendsATalisman) {
    state().addItem("talisman_tianleizi", 1, 0);
    state().addItem("talisman_tulao_fu", 1, 0);
    BattleScene scene(kTwoFoeBattle);
    scene.onEnter(app_);
    ASSERT_TRUE(scene.battle().hasItem("talisman_tulao_fu")) << "先验：两件都登记在场上，AI 够得着";
    scene.runToCompletion(200);
    EXPECT_EQ(state().itemCount("talisman_tianleizi"), 1);
    EXPECT_EQ(state().itemCount("talisman_tulao_fu"), 1);
    for (const BattleEvent& e : scene.battle().events()) {
        if (e.kind != BattleEventKind::Act || !scene.battle().units()[static_cast<std::size_t>(e.actor)].ally) continue;
        EXPECT_NE(e.value, static_cast<int>(ActionKind::Item)) << "我方 AI 掏了一件东西：" << e.text;
    }
}

// ===========================================================================
// E3 瓶子容量随境界（契约第 3 节）
// ===========================================================================

// 3.4 第 1 条：四档各一条。
TEST(Ch07BottleFloor, EachTierHasItsFloor) {
    EXPECT_EQ(fanren::rules::bottleCapacityFloor(RealmTier::Mortal), 3);
    EXPECT_EQ(fanren::rules::bottleCapacityFloor(RealmTier::QiRefining), 3);
    EXPECT_EQ(fanren::rules::bottleCapacityFloor(RealmTier::Foundation), 6);
    EXPECT_EQ(fanren::rules::bottleCapacityFloor(RealmTier::Core), 9);
}

// 3.4 第 4 条：读档。筑基初期、容量 3 的老档读进来是 6；容量 8 的读进来仍是 8。滴数不动。
TEST(Ch07BottleFloor, ALoadedFoundationSaveGetsSixAndABiggerBottleKeepsItsOwn) {
    for (const auto [written, expected] : {std::pair{3, 6}, std::pair{8, 8}}) {
        GameState before;
        before.realm = Realm::FoundationEarly;
        before.bottle.owned = true;
        before.bottle.capacity = written;
        before.bottle.drops = 3;
        const fs::path path = fanren::test::uniqueTempPath("fanren_ch07_bottle", ".json");
        ASSERT_TRUE(fanren::io::saveGame(before, path.string()).ok);
        const auto loaded = fanren::io::loadGame(path.string());
        std::error_code ec;
        fs::remove(path, ec);
        ASSERT_TRUE(loaded.ok) << loaded.error;
        EXPECT_EQ(loaded.value.bottle.capacity, expected) << "存档里写的是 " << written;
        EXPECT_EQ(loaded.value.bottle.drops, 3) << "抬上限不送液";
    }
}

// 3.4 第 3 条：面板突破。十二层 → 十三层同档，容量仍 3；十三层 → 筑基初期跨档，容量 6（负向自检咬的是这一半）。
class Ch07BottlePanel : public ::testing::Test {
protected:
    void SetUp() override {
        auto ready = app_.init(assetRoot(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
    }
    void TearDown() override { app_.shutdown(); }

    // 按到第一次成为止：突破是骰子，换一天换一颗种子（breakthroughSeed 是纯函数，循环是确定的）。
    bool breakFrom(Realm from, Realm cap) {
        for (int day = 1; day <= 400; ++day) {
            GameState& s = app_.state();
            s = GameState{};
            s.day = day;
            s.realm = from;
            s.realmCap = cap;
            s.aptitude = 100;
            s.cultivation = fanren::rules::cultivationNeeded(from) * 3;
            s.bottle.owned = true;
            s.bottle.capacity = 3;
            s.bottle.drops = 3;
            s.bottle.lastChargeDay = day;
            CultivationScene panel;
            if (panel.breakthrough(app_).success) return true;
        }
        return false;
    }

    Application app_;
};

TEST_F(Ch07BottlePanel, ABreakthroughWithinQiRefiningKeepsThreeAndIntoFoundationMakesSix) {
    ASSERT_TRUE(breakFrom(Realm::QiRefining12, Realm::QiRefining13));
    EXPECT_EQ(app_.state().realm, Realm::QiRefining13);
    EXPECT_EQ(app_.state().bottle.capacity, 3) << "同档内升层，容量不变";

    ASSERT_TRUE(breakFrom(Realm::QiRefining13, Realm::FoundationEarly));
    EXPECT_EQ(app_.state().realm, Realm::FoundationEarly);
    EXPECT_EQ(app_.state().bottle.capacity, 6) << "面板突破到筑基：容量跟着到 6";
    EXPECT_EQ(app_.state().bottle.drops, 3) << "抬上限不送液";
}

// ===========================================================================
// E4 按年份下限计数与扣物（契约第 4 节）：GameState 那一层；走真脚本的在 tests/ScriptApiTests.cpp
// ===========================================================================

constexpr const char* kHuangjing = "herb_huangjing_cao";

GameState agedBag() {
    GameState s;
    s.addItem(kHuangjing, 2, 44);
    s.addItem(kHuangjing, 3, 11);
    s.addItem(kHuangjing, 1, 0);
    return s;
}

// 4.4 第 1 条。
TEST(Ch07AgedItems, TheAgedCountSeesOnlyThePilesOldEnough) {
    const GameState s = agedBag();
    EXPECT_EQ(s.itemCountAtLeastAge(kHuangjing, 44), 2);
    EXPECT_EQ(s.itemCountAtLeastAge(kHuangjing, 0), 6);
    EXPECT_EQ(s.itemCountAtLeastAge(kHuangjing, 45), 0);
}

// 4.4 第 2 条。
TEST(Ch07AgedItems, TakingAgedIsAllOrNothingAndTakesTheYoungestPileOldEnough) {
    GameState s = agedBag();
    EXPECT_FALSE(s.removeItemAtLeastAge(kHuangjing, 3, 44)) << "够格的只有两株";
    EXPECT_EQ(s.itemCountOfAge(kHuangjing, 44), 2) << "一株不动";
    EXPECT_EQ(s.itemCountOfAge(kHuangjing, 11), 3);
    EXPECT_EQ(s.itemCountOfAge(kHuangjing, 0), 1);

    EXPECT_TRUE(s.removeItemAtLeastAge(kHuangjing, 2, 20));
    EXPECT_EQ(s.itemCountOfAge(kHuangjing, 44), 0) << "够格的只有四十四年那两株";
    EXPECT_EQ(s.itemCountOfAge(kHuangjing, 11), 3) << "十一年的原样";
    EXPECT_EQ(s.itemCountOfAge(kHuangjing, 0), 1);

    GameState fresh = agedBag();
    EXPECT_TRUE(fresh.removeItemAtLeastAge(kHuangjing, 2, 5));
    EXPECT_EQ(fresh.itemCountOfAge(kHuangjing, 11), 1) << "够格里年份最低的先扣";
    EXPECT_EQ(fresh.itemCountOfAge(kHuangjing, 44), 2) << "更老的留给玩家";
    EXPECT_EQ(fresh.itemCountOfAge(kHuangjing, 0), 1) << "不够格的不碰";
}

// 4.4 第 4 条：TakeItemAged 追加在枚举末尾——存档与测试按序号比对，插在中间会让旧命令静默错位。
TEST(Ch07AgedItems, TheNewCommandIsAppendedRightAfterPlayBgm) {
    EXPECT_EQ(static_cast<int>(fanren::script::CommandKind::TakeItemAged),
              static_cast<int>(fanren::script::CommandKind::PlayBgm) + 1);
}

}  // namespace
