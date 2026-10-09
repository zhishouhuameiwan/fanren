// 第 6 章引擎增补（契约 docs/interfaces-p3-ch06.md）：天眼术「看破」（E1）与修炼面板灵根一行（E3）。
//
// 判据写死成契约原文的数字（法力少 5、两个敌人各两样破绽、蓄 3 点劲），不从被测物推；
// 走得到真实入口的（菜单点天眼术、收场写存档、下一场开局）就走真实入口。
// O1（储物袋）那一条在 tests/GameWiringTests.cpp（TheBagIsCalledTheStorageBagOnceHeHasOne）。
#include <gtest/gtest.h>

#include <bit>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "TempDir.h"
#include "core/battle/Battle.h"
#include "core/model/Types.h"
#include "game/Application.h"
#include "game/BattleScene.h"
#include "game/CultivationScene.h"
#include "game/Wording.h"
#include "io/DataLoader.h"
#include "io/SaveFile.h"

namespace {

namespace fs = std::filesystem;

using fanren::core::GameData;
using fanren::core::GameState;
using fanren::core::Magic;
using fanren::core::MagicEffect;
using fanren::core::battle::Action;
using fanren::core::battle::ActionKind;
using fanren::core::battle::BattleEvent;
using fanren::core::battle::BattleEventKind;
using fanren::core::battle::BattlePhase;
using fanren::core::battle::BattleState;
using fanren::core::battle::Unit;
using fanren::game::BattleScene;
using fanren::game::CultivationScene;
using fanren::game::PanelStage;
using fanren::rules::Realm;
using fanren::test::TempDir;

constexpr int kSword = fanren::core::kCategorySword;
constexpr int kFist = fanren::core::kCategoryFist;
constexpr int kFire = fanren::core::kCategoryFire;
constexpr int kPoison = fanren::core::kCategoryPoison;

constexpr const char* kTianyan = "magic_tianyan_shu";
constexpr const char* kHushen = "magic_hushen_gang";
constexpr const char* kFireball = "magic_huodan_shu";

int bitCount(int mask) { return std::popcount(static_cast<unsigned>(mask)); }

// ---------------------------------------------------------------------------
// 1. 数据：effect 怎么读进来（契约 1.2 / 1.7 第 1 条）
// ---------------------------------------------------------------------------

void writeFile(const fs::path& path, const std::string& content) {
    fs::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out << content;
}

// 一门法术：power 由调用方给（看破的法术得明写 0），extra 拼进对象里。
fanren::core::Result<GameData> loadProbeMagic(const TempDir& tmp, int power, const std::string& extra) {
    std::string json = R"({"id":"probe_magic","name":"探针术","element":0,"needMp":5,"power":)" +
                       std::to_string(power);
    if (!extra.empty()) json += "," + extra;
    writeFile(tmp.path() / "data" / "magics" / "probe_magic.json", json + "}");
    return fanren::io::loadGameData((tmp.path() / "data").string());
}

TEST(Ch06RevealData, EffectRevealIsReadAndLeavingItOutMeansNone) {
    TempDir plain{"fanren_ch06_effect"};
    const auto a = loadProbeMagic(plain, 6, "");
    ASSERT_TRUE(a.ok) << a.error;
    EXPECT_EQ(a.value.findMagic("probe_magic")->effect, MagicEffect::None) << "缺省没有效果：现有法术行为零变化";

    TempDir reveal{"fanren_ch06_effect"};
    const auto b = loadProbeMagic(reveal, 0, R"("effect":"reveal")");
    ASSERT_TRUE(b.ok) << b.error;
    EXPECT_EQ(b.value.findMagic("probe_magic")->effect, MagicEffect::Reveal);
}

TEST(Ch06RevealData, AnythingButExactlyRevealIsRefusedByFileAndField) {
    // 先验在上一条：同一份夹具写 "reveal" 读得进来。这里每一条只改 effect 那一处。
    for (const char* bad : {R"("effect":"reveal ")", R"("effect":"Reveal")", R"("effect":"")"}) {
        TempDir tmp{"fanren_ch06_effect"};
        const auto loaded = loadProbeMagic(tmp, 0, bad);
        EXPECT_FALSE(loaded.ok) << bad << "：拼错了还悄悄收下，天眼术就成了一门点不动的法术";
        EXPECT_NE(loaded.error.find("effect"), std::string::npos) << loaded.error;
        EXPECT_NE(loaded.error.find("probe_magic.json"), std::string::npos) << loaded.error;
    }
}

TEST(Ch06RevealData, ASpellCannotBothHurtAndReveal) {
    TempDir withPower{"fanren_ch06_effect"};
    const auto a = loadProbeMagic(withPower, 12, R"("effect":"reveal")");
    EXPECT_FALSE(a.ok) << "给火弹术挂个 reveal，就成了「打一下顺便全看穿」";
    EXPECT_NE(a.error.find("effect"), std::string::npos) << a.error;
    EXPECT_NE(a.error.find("probe_magic.json"), std::string::npos) << a.error;

    TempDir withPoison{"fanren_ch06_effect"};
    const auto b = loadProbeMagic(withPoison, 0, R"("effect":"reveal","poison":2,"poisonPower":3)");
    EXPECT_FALSE(b.ok) << "带毒也是伤人";
    EXPECT_NE(b.error.find("effect"), std::string::npos) << b.error;
}

// ---------------------------------------------------------------------------
// 2–4. 规则：施展、菜单、蓄劲（契约 1.3 / 1.7 第 2–4 条）
// ---------------------------------------------------------------------------

Magic tianyan() {
    Magic m;
    m.id = kTianyan;
    m.name = "天眼术";
    m.element = fanren::core::kElementNone;
    m.needMp = 5;
    m.power = 0;
    m.needRealm = Realm::QiRefining1;
    m.effect = MagicEffect::Reveal;
    return m;
}

Magic hushen() {
    Magic m;
    m.id = kHushen;
    m.name = "护身罡";
    m.needMp = 10;
    m.power = 0;
    m.needRealm = Realm::QiRefining1;
    return m;
}

Magic fireball() {
    Magic m;
    m.id = kFireball;
    m.name = "火弹术";
    m.element = fanren::core::kElementFire;
    m.needMp = 5;
    m.power = 12;
    m.needRealm = Realm::QiRefining1;
    return m;
}

Unit hero() {
    Unit u;
    u.id = "han_li";
    u.name = "韩立";
    u.hp = u.maxHp = 100;
    u.mp = u.maxMp = 30;
    u.attack = 10;
    u.defence = 5;
    u.speed = 9;   // 最快：第 1 回合头一个出手
    u.realm = Realm::QiRefining3;
    u.ally = true;
    u.weapons = kFist;
    u.magics = {kTianyan, kHushen, kFireball};
    u.magicsExhaustive = true;
    return u;
}

Unit foe(const std::string& id, int weaknesses, int speed) {
    Unit u;
    u.id = id;
    u.name = id;
    u.hp = u.maxHp = 100;
    u.mp = u.maxMp = 30;
    u.attack = 10;
    u.defence = 5;
    u.speed = speed;
    u.realm = Realm::QiRefining3;
    u.weapons = kFist;
    u.maxToughness = 3;
    u.weaknesses = weaknesses;
    u.magicsExhaustive = true;
    return u;
}

// 一场：韩立 ＋ 两个不同 id 的敌人，各两样破绽，开局零揭开。
BattleState twoFoes() {
    BattleState state;
    state.setup({hero(), foe("foe_a", kSword | kFire, 3), foe("foe_b", kFist | kPoison, 2)}, 7);
    state.addMagic(tianyan());
    state.addMagic(hushen());
    state.addMagic(fireball());
    return state;
}

Action reveal(int boost = 0) {
    Action a;
    a.kind = ActionKind::Cast;
    a.actorIndex = 0;
    a.targetIndex = -1;   // 看破不挑目标
    a.magicId = kTianyan;
    a.boost = boost;
    return a;
}

std::vector<BattleEventKind> kindsOf(const std::vector<BattleEvent>& events) {
    std::vector<BattleEventKind> out;
    for (const BattleEvent& e : events) out.push_back(e.kind);
    return out;
}

TEST(Ch06Reveal, OneCastLaysBareEveryFoeWithoutTouchingThem) {
    BattleState state = twoFoes();
    ASSERT_EQ(state.currentActor(), 0) << "先验：韩立身法最高，头一个出手";
    ASSERT_EQ(state.units()[1].revealed, 0);
    ASSERT_EQ(state.units()[2].revealed, 0);

    const auto result = state.apply(reveal());
    ASSERT_TRUE(result.ok) << result.error;

    const std::vector<Unit>& u = state.units();
    EXPECT_EQ(u[1].revealed, kSword | kFire) << "两样全揭开，包括一样都没打过的";
    EXPECT_EQ(u[2].revealed, kFist | kPoison);
    EXPECT_EQ(bitCount(u[1].revealed) + bitCount(u[2].revealed), 4);
    EXPECT_EQ(u[0].mp, 25) << "法力 30 → 25";
    for (const int i : {1, 2}) {
        EXPECT_EQ(u[static_cast<std::size_t>(i)].hp, 100) << "不伤人";
        EXPECT_EQ(u[static_cast<std::size_t>(i)].toughness, 3) << "不削架势";
        EXPECT_FALSE(u[static_cast<std::size_t>(i)].broken());
    }
    // 事件：一手 Act，接着每个敌人一条 Reveal；没有 Hit（不是一击）、没有 BoostSpent。
    const std::vector<BattleEvent> events = state.lastActionEvents();
    EXPECT_EQ(kindsOf(events),
              (std::vector<BattleEventKind>{BattleEventKind::Act, BattleEventKind::Reveal, BattleEventKind::Reveal}));
    ASSERT_EQ(events.size(), 3u);
    EXPECT_EQ(events[1].target, 1);
    EXPECT_EQ(events[1].revealed, kSword | kFire) << "这一下新揭开的";
    EXPECT_EQ(events[2].target, 2);
    EXPECT_EQ(events[2].value, kFist | kPoison) << "揭开之后的全部";
}

TEST(Ch06Reveal, SameKindFoesStillToComeAreLaidBareToo) {
    // 与「打中揭开」同一条口径（docs/octopath-battle.md 2.4）：知道的是「这种人怕什么」。
    // 下一波里同 id 的那一个开场就是亮的；不同 id 的不管（它还没上场，看不见）。
    Unit late = foe("foe_a", kSword | kFire, 3);
    late.wave = 1;
    Unit stranger = foe("foe_c", kFist, 3);
    stranger.wave = 1;
    BattleState state;
    state.setup({hero(), foe("foe_a", kSword | kFire, 3), late, stranger}, 7);
    state.addMagic(tianyan());
    ASSERT_TRUE(state.apply(reveal()).ok);
    EXPECT_EQ(state.units()[2].revealed, kSword | kFire);
    EXPECT_EQ(state.units()[3].revealed, 0);
}

TEST(Ch06Reveal, TheMenuLetsItThroughAndStillRefusesTheShield) {
    GameData data;
    data.magics[kTianyan] = tianyan();
    data.magics[kHushen] = hushen();
    data.magics[kFireball] = fireball();
    GameState state;
    state.setFlag(fanren::game::kXiuxianKnownFlag);   // 修仙用词：理由原样上屏
    BattleState battle = twoFoes();

    std::vector<std::string> ids;
    const auto rows = BattleScene::buildMagicItems(data, state, battle, 0, ids);
    ASSERT_EQ(ids.size(), 3u);
    for (std::size_t i = 0; i < ids.size(); ++i) {
        const auto& row = rows[i];
        if (ids[i] == kTianyan) {
            EXPECT_TRUE(row.enabled) << "天眼术不伤人，却点得动：" << row.disabledReason;
        } else if (ids[i] == kHushen) {
            EXPECT_FALSE(row.enabled) << "护身罡照旧点不动";
            EXPECT_EQ(row.disabledReason, "【护身罡】不是伤人的法术") << "理由还是原来那一句";
        } else {
            EXPECT_TRUE(row.enabled) << "火弹术照旧：" << row.disabledReason;
        }
    }
    EXPECT_TRUE(fanren::core::castableMagic(tianyan()));
    EXPECT_FALSE(fanren::core::offensiveMagic(tianyan())) << "伤害 / 下毒的分支照旧问 offensiveMagic，它不在其中";
}

TEST(Ch06Reveal, BoostingItDoesNothingAndCostsNothing) {
    // 契约 1.3 / 1.6：蓄了劲也只当没蓄——劲不扣、不算「这一回合蓄过劲」、效果与不蓄相同。
    const auto castAt = [](int boost) {
        BattleState state = twoFoes();
        for (int guard = 0; guard < 64 && state.round() < 3; ++guard) state.endTurn();
        for (int guard = 0; guard < 64 && state.currentActor() != 0; ++guard) state.endTurn();
        EXPECT_EQ(state.units()[0].bp, 3) << "先验：第 3 回合开始，劲 1 + 1 + 1";
        const auto result = state.apply(reveal(boost));
        EXPECT_TRUE(result.ok) << result.error;
        return state;
    };
    const BattleState plain = castAt(0);
    const BattleState boosted = castAt(3);
    const Unit& a = plain.units()[0];
    const Unit& b = boosted.units()[0];
    EXPECT_EQ(b.bp, 3) << "蓄的 3 点劲一点没扣";
    EXPECT_FALSE(b.boosted) << "下一回合照常加劲";
    EXPECT_EQ(b.mp, a.mp);
    for (const int i : {1, 2}) {
        EXPECT_EQ(boosted.units()[static_cast<std::size_t>(i)].revealed, plain.units()[static_cast<std::size_t>(i)].revealed);
        EXPECT_EQ(boosted.units()[static_cast<std::size_t>(i)].hp, 100);
        EXPECT_EQ(boosted.units()[static_cast<std::size_t>(i)].toughness, 3);
    }
    EXPECT_EQ(kindsOf(boosted.lastActionEvents()), kindsOf(plain.lastActionEvents())) << "没有 BoostSpent";
}

TEST(Ch06Reveal, TheEnemysAiNeverReachesForIt) {
    // 敌人法术表里本来不写它；万一写了（或留空而登记表里有），AI 也不拿不伤人的一手当「最重的一手」。
    Unit caster = foe("foe_seer", kSword, 30);   // 远快于韩立：头一个出手
    caster.magics = {kTianyan};
    BattleState state;
    state.setup({caster, hero()}, 3);
    state.addMagic(tianyan());
    ASSERT_EQ(state.currentActor(), 0) << "先验：这位敌人先出手";
    ASSERT_TRUE(state.isLegal(Action{ActionKind::Cast, 0, -1, kTianyan})) << "先验：它施展得了";
    const Action decided = state.decideAi(0);
    EXPECT_NE(decided.kind, ActionKind::Cast) << "施展了看破：" << decided.magicId;
}

// ---------------------------------------------------------------------------
// 2 / 5. 走真实入口：菜单点天眼术 → 收场写存档 → 读档 → 下一场开局就亮（契约 1.7 第 2、5 条）
// ---------------------------------------------------------------------------

std::string assetRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "data" / "magics" / "tianyan_shu.json")) return candidate;
    }
    return ".";
}

// 同门切磋那一场：两个不同 id 的师兄（马六、陆春），各两样破绽。
constexpr const char* kTwoFoeBattle = "b04_qiecuo_maliu";

class Ch06RevealScene : public ::testing::Test {
protected:
    void SetUp() override {
        auto ready = app_.init(assetRoot(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
        GameState& s = app_.state();
        s.realm = Realm::QiRefining8;
        s.hp = s.maxHp = 120;
        s.mp = s.maxMp = 80;
        ASSERT_TRUE(s.learnMagic(kTianyan));
    }
    void TearDown() override { app_.shutdown(); }

    // 菜单上把天眼术点下去：施法 → 那一行。看破不挑目标，点下去就施展了。
    void castFromMenu(BattleScene& scene) {
        ASSERT_GE(scene.runToAllyTurn(), 0);
        scene.openMenu(app_);
        ASSERT_TRUE(scene.menuChoose(app_, fanren::game::kBattleMenuCast));
        int row = -1;
        for (int i = 0; i < scene.menuList().count(); ++i) {
            if (scene.menuList().items()[static_cast<std::size_t>(i)].label == "天眼术") row = i;
        }
        ASSERT_GE(row, 0) << "法术列表里没有天眼术";
        EXPECT_TRUE(scene.menuList().items()[static_cast<std::size_t>(row)].enabled)
            << scene.menuList().items()[static_cast<std::size_t>(row)].disabledReason;
        ASSERT_TRUE(scene.menuChoose(app_, row)) << scene.feedback();
        EXPECT_EQ(scene.menuMode(), fanren::game::BattleMenuMode::Closed) << "不经过选目标那一级";
    }

    fanren::game::Application app_;
};

TEST_F(Ch06RevealScene, CastingFromTheMenuLightsEveryFoeAndTheSaveRemembersIt) {
    const auto* maliu = app_.data().findRole("tongmen_maliu");
    const auto* luchun = app_.data().findRole("tongmen_luchun");
    ASSERT_NE(maliu, nullptr);
    ASSERT_NE(luchun, nullptr);
    ASSERT_EQ(bitCount(maliu->weaknesses), 2) << "先验：两个敌人各两样破绽";
    ASSERT_EQ(bitCount(luchun->weaknesses), 2);
    ASSERT_TRUE(app_.state().knownWeaknesses.empty());

    {
        BattleScene scene(kTwoFoeBattle);
        scene.onEnter(app_);
        for (const Unit& u : scene.battle().units()) {
            if (!u.ally) ASSERT_EQ(u.revealed, 0) << "先验：开局零揭开";
        }
        const int mpBefore = scene.battle().units()[0].mp;
        castFromMenu(scene);
        const std::vector<Unit>& units = scene.battle().units();
        EXPECT_EQ(units[0].mp, mpBefore - 5);
        for (const Unit& u : units) {
            if (!u.ally) EXPECT_EQ(u.revealed, u.weaknesses) << u.id;
        }
        scene.runToCompletion(200);
        scene.update(app_, 1.0 / 60.0);   // 这一帧里 finish() 把揭开的并进存档
    }
    EXPECT_EQ(app_.state().knownWeaknesses.size(), 2u);
    EXPECT_EQ(app_.state().knownWeaknessesOf("tongmen_maliu"), maliu->weaknesses);
    EXPECT_EQ(app_.state().knownWeaknessesOf("tongmen_luchun"), luchun->weaknesses);

    // 存档 → 读档 → 同一场再打：开局就是亮的（docs/octopath-battle.md 2.5 那条路）。
    app_.state().realmCap = app_.state().realm;  // 手设境界的保存夹具需满足v9上限约束。
    const fs::path path = fanren::test::uniqueTempPath("fanren_ch06_reveal", ".json");
    ASSERT_TRUE(fanren::io::saveGame(app_.state(), path.string()).ok);
    auto loaded = fanren::io::loadGame(path.string());
    std::error_code ec;
    fs::remove(path, ec);
    ASSERT_TRUE(loaded.ok) << loaded.error;
    app_.state() = loaded.value;

    BattleScene again(kTwoFoeBattle);
    again.onEnter(app_);
    int foes = 0;
    for (const Unit& u : again.battle().units()) {
        if (u.ally) continue;
        ++foes;
        EXPECT_EQ(u.revealed, u.weaknesses) << u.id << "：上一场看破过的，这一场一开场就亮着";
    }
    EXPECT_EQ(foes, 2);
}

TEST_F(Ch06RevealScene, TheDataFileIsTheOneTheContractDescribes) {
    const Magic* magic = app_.data().findMagic(kTianyan);
    ASSERT_NE(magic, nullptr);
    EXPECT_EQ(magic->effect, MagicEffect::Reveal);
    EXPECT_EQ(magic->power, 0);
    EXPECT_EQ(magic->needMp, 5);
    EXPECT_EQ(magic->needRealm, Realm::QiRefining1);
}

// ---------------------------------------------------------------------------
// E3：修炼面板的灵根一行（契约第 2 节）
// ---------------------------------------------------------------------------

constexpr const char* kLinggenFlag = "ch06.linggen";

TEST(Ch06SpiritRoot, TheLineNeedsBothTheImmortalWordsAndTheTest) {
    GameState mortal;
    mortal.setFlag(kLinggenFlag);
    ASSERT_EQ(fanren::game::wordingStage(mortal), PanelStage::Mortal);
    EXPECT_EQ(CultivationScene::spiritRootLine(mortal), "") << "凡人阶段：「灵根」是禁词，旗标置了也不说";

    GameState untested;
    untested.setFlag(fanren::game::kXiuxianKnownFlag);
    EXPECT_EQ(CultivationScene::spiritRootLine(untested), "") << "进了修仙界、还没测过灵根";

    GameState tested = untested;
    tested.setFlag(kLinggenFlag);
    const std::string line = CultivationScene::spiritRootLine(tested);
    EXPECT_NE(line.find("灵根"), std::string::npos) << line;
    EXPECT_NE(line.find("四属性缺金·伪灵根"), std::string::npos) << line;
    EXPECT_EQ(tested.aptitude, 50) << "资质不动：它是修炼速度的输入，与灵根是两件事";
}

TEST(Ch06SpiritRoot, TheMortalTableLeavesBothWordsEmptyAndTheScanSeesThem) {
    const auto& mortal = fanren::game::cultivationLexicon(PanelStage::Mortal);
    EXPECT_STREQ(mortal.spiritRootLabel, "");
    EXPECT_STREQ(mortal.spiritRootValue, "");
    // 两条词进了禁词扫描（PanelWording）扫的那张清单：修仙表那一句在清单里找得到，
    // 说明凡人表同位置的两条也在（同一个函数按同一张结构取）。
    bool found = false;
    for (const std::string& text : fanren::game::cultivationPanelStrings(PanelStage::Immortal)) {
        if (text == "四属性缺金·伪灵根") found = true;
    }
    EXPECT_TRUE(found);
}

}  // namespace
