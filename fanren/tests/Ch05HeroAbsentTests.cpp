// 韩立不在场的战斗（契约 docs/interfaces-p3-ch05.md 第 2 节，设计 docs/ch05-design.md 10.2 / 验收 16）。
//
// 第 5 章夺帮那一夜（ch122）韩立在客栈睡觉，玩家操纵曲魂与孙二狗手下的两个脚夫。
// 编成里一位开关 hero_absent：开了就不建韩立，胜负只看友军，战后存档的气血、法力一点不动。
//
// 本文件逐条钉：
//   · 加载器：读得进、缺省为假、非布尔报错、开了却无友军报错、友军全在后几波也报错；
//   · 真仓库的每一场编成：开关为假的开场 0 号仍是韩立（「其余编成零变化」扫一片，不钉一场）；
//   · 开了开关：单位表里没有 hanli、没有队伍里的同伴、背包一件不登记；
//   · 友军全倒判负、敌方全倒判胜、友军逃成判逃；
//   · 战后 hp / mp / 同伴 hp 与开战前逐一相同——**先验他确实带着伤、同伴确实不满血**，
//     满血局面下「没变」是一句空话（docs/README.md「比值分母塌成 0」同一个形状）；
//   · 奖励照 docs/interfaces-p2.md 第 6 节：胜发、负不发。
//
// 夹具：data/ 与 scripts/ 原样复制一份，再在副本的 data/roles/ 与 data/battles/ 里放几个
// 只为本文件存在的角色与编成（正式的 data/ 归内容路，不往里写）。
#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <vector>

#include "TempDir.h"
#include "core/battle/Battle.h"
#include "core/model/Types.h"
#include "game/Application.h"
#include "game/BattleScene.h"
#include "io/BattleLoader.h"

namespace {

namespace fs = std::filesystem;

using fanren::core::GameState;
using fanren::core::battle::BattlePhase;
using fanren::core::battle::Unit;
using fanren::game::Application;
using fanren::game::BattleScene;

constexpr const char* kMoney = "material_lingshi";
constexpr const char* kPill = "pill_yangjing_dan";
constexpr const char* kCompanion = "qu_hun";

std::string projectRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "data" / "battles") &&
            fs::exists(fs::path(candidate) / "scripts" / "common" / "api.lua")) {
            return candidate;
        }
    }
    return ".";
}

void writeFile(const fs::path& path, const std::string& content) {
    fs::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    ASSERT_TRUE(out.is_open()) << "无法创建夹具文件: " << path.string();
    out << content;
}

bool hasUnit(const BattleScene& scene, const std::string& id) {
    const auto& units = scene.battle().units();
    return std::any_of(units.begin(), units.end(), [&id](const Unit& u) { return u.id == id; });
}

int allyCount(const BattleScene& scene) {
    const auto& units = scene.battle().units();
    return static_cast<int>(
        std::count_if(units.begin(), units.end(), [](const Unit& u) { return u.ally; }));
}

// ===========================================================================
// 一、加载器
// ===========================================================================

// 写一份只有顶层开关与 units 不同的编成，走**真的加载器**读回来。
fanren::core::Result<fanren::core::BattleSetup> loadProbe(const std::string& heroAbsentField,
                                                          const std::string& unitsJson) {
    fanren::test::TempDir tmp{"fanren_ch05_absent_probe"};
    const fs::path file = tmp.path() / "probe.json";
    std::ofstream out(file, std::ios::binary);
    out << R"({"id":"b_probe","name":"探针","chapter":5,"terrain":"field",)"
        << R"("can_escape":false,"defeat_is_fatal":false,)"
        << heroAbsentField << R"("units":)" << unitsJson
        << R"(,"rewards":{"cultivation":0,"spirit_stones":0,"drops":[]}})";
    out.close();
    return fanren::io::loadBattle(file.string());
}

const std::string kAllyAndFoe =
    R"([{"role_id":"qu_hun","faction":"ally"},{"role_id":"wild_wolf"}])";

TEST(Ch05HeroAbsentLoading, TheSwitchIsOffUnlessWritten) {
    const auto loaded = loadProbe("", kAllyAndFoe);
    ASSERT_TRUE(loaded.ok) << loaded.error;
    EXPECT_FALSE(loaded.value.heroAbsent) << "没写就是假：现有编成一个字不改";
}

TEST(Ch05HeroAbsentLoading, TheSwitchIsReadWhenWritten) {
    const auto on = loadProbe(R"("hero_absent":true,)", kAllyAndFoe);
    ASSERT_TRUE(on.ok) << on.error;
    EXPECT_TRUE(on.value.heroAbsent);
    // 对照：写成 false 与不写一样。只验 true 的话，一份「见字段就开」的实现也能绿。
    const auto off = loadProbe(R"("hero_absent":false,)", kAllyAndFoe);
    ASSERT_TRUE(off.ok) << off.error;
    EXPECT_FALSE(off.value.heroAbsent);
}

TEST(Ch05HeroAbsentLoading, ASwitchThatIsNotABooleanIsRefused) {
    for (const char* bad : {R"("hero_absent":"true",)", R"("hero_absent":1,)", R"("hero_absent":null,)"}) {
        const auto loaded = loadProbe(bad, kAllyAndFoe);
        EXPECT_FALSE(loaded.ok) << bad << " 被悄悄收下了——按缺省关着收，韩立就会出现在他不该在的地方";
        if (!loaded.ok) {
            EXPECT_NE(loaded.error.find("hero_absent"), std::string::npos) << loaded.error;
        }
    }
}

TEST(Ch05HeroAbsentLoading, ASwitchedOnFightWithNoAllyIsRefusedAtLoadTime) {
    const auto loaded = loadProbe(R"("hero_absent":true,)",
                                  R"([{"role_id":"wild_wolf"},{"role_id":"wild_wolf"}])");
    EXPECT_FALSE(loaded.ok) << "开了开关却一个友军也没有：那是一场只有敌人的仗";
    if (!loaded.ok) EXPECT_NE(loaded.error.find("ally"), std::string::npos) << loaded.error;

    // 对照：同样的敌人、开关关着（韩立在场），照常读得进。
    const auto withHero = loadProbe("", R"([{"role_id":"wild_wolf"}])");
    EXPECT_TRUE(withHero.ok) << withHero.error;
}

TEST(Ch05HeroAbsentLoading, AlliesThatOnlyArriveInALaterWaveDoNotCount) {
    // 友军全排在第 1 波：开场场上我方无人，setup 那一次 refreshPhase 当场判负。
    const auto loaded = loadProbe(
        R"("hero_absent":true,)",
        R"([{"role_id":"wild_wolf"},{"role_id":"qu_hun","faction":"ally","wave":1}])");
    EXPECT_FALSE(loaded.ok) << "友军全在后几波，开场即判负";
    // 对照：友军挪到第 0 波就读得进。
    const auto fixed = loadProbe(
        R"("hero_absent":true,)",
        R"([{"role_id":"wild_wolf","wave":1},{"role_id":"wild_wolf"},)"
        R"({"role_id":"qu_hun","faction":"ally"}])");
    EXPECT_TRUE(fixed.ok) << fixed.error;
}

// ===========================================================================
// 二、真仓库：开关为假的每一场，开场 0 号仍是韩立
// ===========================================================================

TEST(Ch05HeroAbsentShipped, EveryShippedFightWithoutTheSwitchStillOpensWithHanLiAsUnitZero) {
    const auto battles = fanren::io::loadBattles(projectRoot() + "/data/battles");
    ASSERT_TRUE(battles.ok) << battles.error;

    Application app;
    auto ready = app.init(projectRoot(), /*headless=*/true);
    ASSERT_TRUE(ready.ok) << ready.error;

    int withHero = 0;
    for (const auto& [id, setup] : battles.value) {
        BattleScene scene(id);
        scene.onEnter(app);
        const auto& units = scene.battle().units();
        ASSERT_FALSE(units.empty()) << id;
        if (setup.heroAbsent) {
            EXPECT_FALSE(hasUnit(scene, "hanli")) << id << " 开了 hero_absent，场上却有韩立";
            continue;
        }
        ++withHero;
        EXPECT_EQ(units[0].id, "hanli") << id << " 没开开关，开场 0 号却不是韩立";
        EXPECT_TRUE(units[0].ally);
    }
    // 先验分母：真扫到了开关为假的编成（现有二十几场全是）。
    EXPECT_GE(withHero, 20) << "开关为假的编成只扫到 " << withHero << " 场，这条用例可能在空转";
    app.shutdown();
}

// ===========================================================================
// 三、开了开关的仗
// ===========================================================================

fs::path& fixtureRoot() {
    static fs::path path;
    return path;
}

// 夹具角色：一律凡人，数值只为让胜负确定。
std::string role(const std::string& id, const std::string& name, int hp, int attack, int defence,
                 int speed) {
    return "{\"id\":\"" + id + "\",\"name\":\"" + name + "\",\"realm\":\"Mortal\",\"maxHp\":" +
           std::to_string(hp) + ",\"maxMp\":0,\"attack\":" + std::to_string(attack) +
           ",\"defence\":" + std::to_string(defence) + ",\"speed\":" + std::to_string(speed) +
           ",\"element\":0,\"magics\":[]}";
}

std::string battle(const std::string& id, bool heroAbsent, bool canEscape, const std::string& units,
                   int cultivation, int money) {
    return "{\"id\":\"" + id + "\",\"name\":\"夹具\",\"chapter\":5,\"terrain\":\"field\"," +
           "\"can_escape\":" + (canEscape ? "true" : "false") +
           ",\"defeat_is_fatal\":false," + (heroAbsent ? "\"hero_absent\":true," : "") +
           "\"units\":" + units +
           ",\"rewards\":{\"cultivation\":" + std::to_string(cultivation) +
           ",\"spirit_stones\":" + std::to_string(money) + ",\"drops\":[]}}";
}

class Ch05HeroAbsentFight : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        const fs::path source(projectRoot());
        fs::path& root = fixtureRoot();
        root = fanren::test::uniqueTempPath("fanren_ch05_absent_root");
        std::error_code ec;
        fs::create_directories(root, ec);
        for (const char* dir : {"data", "scripts"}) {
            fs::copy(source / dir, root / dir, fs::copy_options::recursive, ec);
            ASSERT_FALSE(ec) << "复制 " << dir << "/ 失败: " << ec.message();
        }
        const fs::path roles = root / "data" / "roles";
        writeFile(roles / "t05_strong_ally.json", role("t05_strong_ally", "夹具强援", 500, 60, 5, 9));
        writeFile(roles / "t05_weak_ally.json", role("t05_weak_ally", "夹具弱援", 6, 1, 0, 1));
        writeFile(roles / "t05_tough_ally.json", role("t05_tough_ally", "夹具耐打", 5000, 1, 50, 9));
        writeFile(roles / "t05_weak_foe.json", role("t05_weak_foe", "夹具弱敌", 5, 1, 0, 1));
        writeFile(roles / "t05_strong_foe.json", role("t05_strong_foe", "夹具强敌", 500, 60, 5, 9));
        writeFile(roles / "t05_tough_foe.json", role("t05_tough_foe", "夹具耐打敌", 5000, 1, 50, 1));

        const fs::path battles = root / "data" / "battles";
        const std::string twoStrongVsTwoWeak =
            R"([{"role_id":"t05_strong_ally","faction":"ally"},)"
            R"({"role_id":"t05_strong_ally","faction":"ally"},)"
            R"({"role_id":"t05_weak_foe"},{"role_id":"t05_weak_foe"}])";
        writeFile(battles / "t05_absent_win.json",
                  battle("t05_absent_win", true, false, twoStrongVsTwoWeak, 7, 3));
        // 对照组：同一份编成，开关关着。
        writeFile(battles / "t05_present_twin.json",
                  battle("t05_present_twin", false, false, twoStrongVsTwoWeak, 7, 3));
        writeFile(battles / "t05_absent_lose.json",
                  battle("t05_absent_lose", true, false,
                         R"([{"role_id":"t05_weak_ally","faction":"ally"},)"
                         R"({"role_id":"t05_strong_foe"}])",
                         7, 3));
        writeFile(battles / "t05_absent_flee.json",
                  battle("t05_absent_flee", true, true,
                         R"([{"role_id":"t05_tough_ally","faction":"ally"},)"
                         R"({"role_id":"t05_tough_foe"}])",
                         7, 3));
    }
    static void TearDownTestSuite() {
        std::error_code ec;
        fs::remove_all(fixtureRoot(), ec);
    }

    void SetUp() override {
        auto ready = app_.init(fixtureRoot().string(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
        // 他带着伤、法力没满，曲魂也带着伤；背包里有一瓶战斗中用得上的药。
        GameState& s = app_.state();
        s.maxHp = 120;
        s.hp = 47;
        s.maxMp = 80;
        s.mp = 13;
        s.party.clear();
        s.party.push_back(fanren::core::PartyMember{kCompanion, 21, true});
        s.addItem(kPill, 2);
    }
    void TearDown() override { app_.shutdown(); }

    GameState& state() { return app_.state(); }

    // 开战前后都要对的那几个数。
    struct Snapshot {
        int hp = 0, mp = 0, maxHp = 0, maxMp = 0, companionHp = 0, pills = 0;
    };
    Snapshot snapshot() {
        const GameState& s = state();
        Snapshot shot;
        shot.hp = s.hp;
        shot.mp = s.mp;
        shot.maxHp = s.maxHp;
        shot.maxMp = s.maxMp;
        shot.companionHp = s.party.empty() ? -99 : s.party[0].hp;
        shot.pills = s.itemCount(kPill);
        return shot;
    }
    void expectUntouched(const Snapshot& before, const char* why) {
        const Snapshot after = snapshot();
        EXPECT_EQ(after.hp, before.hp) << why << "：韩立的气血变了——他在睡觉";
        EXPECT_EQ(after.mp, before.mp) << why << "：韩立的法力变了";
        EXPECT_EQ(after.maxHp, before.maxHp) << why;
        EXPECT_EQ(after.maxMp, before.maxMp) << why;
        EXPECT_EQ(after.companionHp, before.companionHp) << why << "：同伴没上场，气血却变了";
        EXPECT_EQ(after.pills, before.pills) << why << "：药囊在他身上，这一场不许动";
    }

    Application app_;
};

TEST_F(Ch05HeroAbsentFight, NoHanLiNoCompanionAndNothingFromTheBagIsOnTheField) {
    // 先验：背包里真有一件战斗里用得上的东西，同伴真在队里。
    ASSERT_GT(state().itemCount(kPill), 0);
    ASSERT_TRUE(state().partyHas(kCompanion));

    BattleScene scene("t05_absent_win");
    scene.onEnter(app_);
    EXPECT_FALSE(hasUnit(scene, "hanli")) << "开了 hero_absent，场上却有韩立";
    EXPECT_FALSE(hasUnit(scene, kCompanion)) << "同伴跟着他走，不该出现在这一场";
    EXPECT_EQ(allyCount(scene), 2) << "我方就是编成里写死的那两个 ally";
    EXPECT_EQ(scene.battle().registeredItemCount(), 0u) << "背包里的东西登记到了一场他不在的仗里";

    // 对照组：同一份编成、开关关着——韩立是 0 号，同伴上场，药登记上了。
    // 少了这一半，一份「永远不建韩立」的实现也能让上面全绿。
    BattleScene twin("t05_present_twin");
    twin.onEnter(app_);
    ASSERT_FALSE(twin.battle().units().empty());
    EXPECT_EQ(twin.battle().units()[0].id, "hanli");
    EXPECT_TRUE(hasUnit(twin, kCompanion));
    EXPECT_EQ(allyCount(twin), 4) << "韩立 + 同伴 + 编成里两个 ally";
    EXPECT_GT(twin.battle().registeredItemCount(), 0u);
}

TEST_F(Ch05HeroAbsentFight, TheAlliesWinItAndHisStateIsUntouchedButTheRewardIsPaid) {
    const Snapshot before = snapshot();
    ASSERT_LT(before.hp, before.maxHp) << "先验：他带着伤，否则「气血没变」是空话";
    ASSERT_LT(before.mp, before.maxMp) << "先验：法力没满";
    ASSERT_GT(before.companionHp, 0);
    const int cultivationBefore = state().cultivation;
    const int moneyBefore = state().itemCount(kMoney);

    BattleScene scene("t05_absent_win");
    scene.onEnter(app_);
    ASSERT_TRUE(scene.runToCompletion()) << "先验：两个强援打两个弱敌，该赢";
    static_cast<void>(scene.update(app_, 0.0));   // 见到胜负已分，自己 finish
    EXPECT_EQ(scene.battle().phase(), BattlePhase::Won);

    expectUntouched(before, "打赢之后");
    EXPECT_EQ(state().cultivation - cultivationBefore, 7) << "奖励照发（契约 2.2）";
    EXPECT_EQ(state().itemCount(kMoney) - moneyBefore, 3);
    EXPECT_EQ(scene.grantedReward().cultivation, 7);
}

TEST_F(Ch05HeroAbsentFight, WhenEveryAllyFallsItIsALossEvenThoughHeWasNeverThere) {
    const Snapshot before = snapshot();
    const int cultivationBefore = state().cultivation;

    BattleScene scene("t05_absent_lose");
    scene.onEnter(app_);
    ASSERT_FALSE(hasUnit(scene, "hanli"));
    static_cast<void>(scene.runToCompletion());
    EXPECT_EQ(scene.battle().phase(), BattlePhase::Lost) << "友军全倒就是败：胜负只看友军";
    static_cast<void>(scene.update(app_, 0.0));

    expectUntouched(before, "打输之后");
    EXPECT_EQ(state().cultivation, cultivationBefore) << "输了不发";
    EXPECT_TRUE(scene.grantedReward().empty());
}

TEST_F(Ch05HeroAbsentFight, AnAllyThatGetsAwayEndsItAsAnEscapeAndStillTouchesNothing) {
    const Snapshot before = snapshot();

    BattleScene scene("t05_absent_flee");
    scene.onEnter(app_);
    ASSERT_FALSE(hasUnit(scene, "hanli"));
    for (int attempt = 0; attempt < 200; ++attempt) {
        if (scene.battle().phase() != BattlePhase::Ongoing) break;
        if (scene.runToAllyTurn() < 0) break;
        scene.openMenu(app_);
        if (!scene.menuChoose(app_, fanren::game::kBattleMenuEscape)) break;
    }
    ASSERT_EQ(scene.battle().phase(), BattlePhase::Escaped) << "先验：友军真的逃掉了";
    static_cast<void>(scene.update(app_, 0.0));

    expectUntouched(before, "逃掉之后");
    EXPECT_TRUE(scene.grantedReward().empty()) << "逃了不发";
}

}  // namespace
