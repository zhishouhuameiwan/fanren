// P3 第 3 章：脚本 API 的端到端验收（契约 docs/interfaces-p3-ch03.md 1.3 / 1.4 / 3.4）。
//
// 两件事：队伍（party.*）与识海之战的战果（battle() 的第二、三个返回值）。
//
// 驱动的是玩家实际跑的那一整条链路：api.lua 的封装 → ScriptHost 的 kind 解析表 →
// Application::dispatch → GameState → BattleScene 的编成。只测 dispatch 的话，
// 一个拼错的 kind 字符串就能让脚本与引擎在测试全绿的情况下各说各话。
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>

#include "TempDir.h"
#include "core/model/Types.h"
#include "game/Application.h"
#include "game/BattleScene.h"

namespace {

namespace fs = std::filesystem;

using fanren::core::GameState;
using fanren::game::Application;

// 第 3 章章末入队的那一位，data/roles/qu_hun.json 实有此条。
constexpr const char* kCompanion = "qu_hun";
// 教学关：一张普通战场，队伍该在这种仗里上场。
constexpr const char* kTutorialBattle = "b03_gu_wai_elang";

fs::path findProjectRoot() {
    fs::path dir = fs::current_path();
    for (int depth = 0; depth < 8; ++depth) {
        if (fs::exists(dir / "scripts" / "common" / "api.lua") && fs::is_directory(dir / "data")) {
            return dir;
        }
        const fs::path parent = dir.parent_path();
        if (parent.empty() || parent == dir) break;
        dir = parent;
    }
    return {};
}

// 本套测试自己的资源根。夹具脚本必须与 api.lua 同根才加载得到，而正式的
// scripts/ 目录不该为了测试方便塞进几个假脚本。目录名走 TempDir.h 的唯一命名
// （pid + 时间戳 + 计数）：写死路径在并行的 ctest 里会互相 remove_all。
fs::path& tempAssetRoot() {
    static fs::path path;
    return path;
}

void writeScript(const fs::path& path, const std::string& body) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out << body;
}

class Ch03ScriptTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        const fs::path source = findProjectRoot();
        ASSERT_FALSE(source.empty()) << "找不到工程根目录（应含 scripts/common/api.lua 与 data/）";

        fs::path& root = tempAssetRoot();
        root = fanren::test::uniqueTempPath("fanren_ch03_party_root");
        std::error_code ec;
        fs::create_directories(root / "scripts" / "common", ec);
        fs::create_directories(root / "scripts" / "t", ec);
        fs::copy(source / "data", root / "data", fs::copy_options::recursive, ec);
        ASSERT_FALSE(ec) << "复制 data/ 失败: " << ec.message();
        // 跑的是真的 api.lua，不是复刻品：封装写错了要在这里红，不是等上线。
        fs::copy_file(source / "scripts" / "common" / "api.lua",
                      root / "scripts" / "common" / "api.lua",
                      fs::copy_options::overwrite_existing, ec);
        ASSERT_FALSE(ec) << "复制 api.lua 失败: " << ec.message();

        // 夹具脚本就地写：它们只服务于本文件，放进 tests/scripts/ 会让别的
        // 测试套也跟着复制一份没人用的东西。
        writeScript(root / "scripts" / "t" / "party_add.lua",
                    "ok = party.add('qu_hun')\n"
                    "flag.set('ch03.quhun_rudui', ok and 1 or 0)\n");
        writeScript(root / "scripts" / "t" / "party_add_twice.lua",
                    "local a = party.add('qu_hun')\n"
                    "local b = party.add('qu_hun')\n"
                    "flag.set('ch03.quhun_rudui', (a and b) and 1 or 0)\n");
        writeScript(root / "scripts" / "t" / "party_add_bogus.lua",
                    "local ok = party.add('no_such_role_at_all')\n"
                    "flag.set('ch03.quhun_rudui', ok and 1 or 0)\n");
        writeScript(root / "scripts" / "t" / "party_query.lua",
                    "flag.set('ch03.quhun_rudui', party.has('qu_hun') and 1 or 0)\n"
                    "flag.set('ch03.tiezhe_zhi', party.size())\n");
        // 识海之战：脚本要读得到「咬下了多少」，并分得清「敌人跑了」与「我输了」。
        writeScript(root / "scripts" / "t" / "shihai.lua",
                    "local won, how, spoils = battle('b03_shihai_duoshe')\n"
                    "flag.set('ch03.shihai_done', won and 1 or 0)\n"
                    "flag.set('ch03.mo_siwang', how == 'enemy_fled' and 1 or 0)\n"
                    "flag.set('ch03.shihai_yaoxia', spoils)\n");
        writeScript(root / "scripts" / "t" / "party_remove.lua",
                    "local first = party.remove('qu_hun')\n"
                    "local second = party.remove('qu_hun')\n"
                    "flag.set('ch03.quhun_rudui', first and 1 or 0)\n"
                    "flag.set('ch03.tiezhe_zhi', second and 1 or 0)\n");
    }

    static void TearDownTestSuite() {
        std::error_code ec;
        fs::remove_all(tempAssetRoot(), ec);
    }

    void SetUp() override {
        auto ready = app_.init(tempAssetRoot().string(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
    }
    void TearDown() override { app_.shutdown(); }

    GameState& state() { return app_.state(); }

    void runScript(const std::string& path, int maxFrames = 64) {
        const auto started = app_.startEvent(path);
        ASSERT_TRUE(started.ok) << path << ": " << started.error;
        for (int frame = 0; frame < maxFrames && app_.scripts().isRunning(); ++frame) {
            app_.tick(1.0 / 60.0);
        }
        ASSERT_FALSE(app_.scripts().isRunning()) << path << " 没能跑到结束";
    }

    Application app_;
};

TEST_F(Ch03ScriptTest, AddPutsTheCompanionInTheParty) {
    runScript("t/party_add.lua");

    ASSERT_EQ(state().party.size(), 1u);
    EXPECT_EQ(state().party[0].roleId, kCompanion);
    EXPECT_EQ(state().party[0].hp, -1) << "刚入队按模板满血";
    EXPECT_TRUE(state().party[0].active);
    EXPECT_EQ(state().flag("ch03.quhun_rudui"), 1) << "party.add 要回填成功";
}

TEST_F(Ch03ScriptTest, AddIsIdempotentAndStillReportsSuccess) {
    runScript("t/party_add_twice.lua");

    EXPECT_EQ(state().party.size(), 1u) << "入两次不该多出一个人";
    EXPECT_EQ(state().flag("ch03.quhun_rudui"), 1)
        << "第二次也算成功：「让他入队」这件事的结果已经成立";
}

TEST_F(Ch03ScriptTest, AddRefusesARoleThatIsNotInTheData) {
    // 负向，契约第 1.3 节点名要求的那一条：拼错的角色 id 不能静默收下，
    // 否则它会在战斗里变成一个没有属性的空位，而那时已经很难追回是谁写错的。
    runScript("t/party_add_bogus.lua");

    EXPECT_TRUE(state().party.empty()) << "查不到的角色不该进队";
    EXPECT_EQ(state().flag("ch03.quhun_rudui"), 0) << "必须回填 ok = false";
}

TEST_F(Ch03ScriptTest, HasAndSizeReadTheLiveState) {
    ASSERT_TRUE(state().partyAdd(kCompanion));
    runScript("t/party_query.lua");

    EXPECT_EQ(state().flag("ch03.quhun_rudui"), 1) << "party.has 应当读到队里那一位";
    EXPECT_EQ(state().flag("ch03.tiezhe_zhi"), 1) << "party.size 应当是 1";
}

TEST_F(Ch03ScriptTest, HasIsFalseForSomebodyWhoNeverJoined) {
    runScript("t/party_query.lua");
    EXPECT_EQ(state().flag("ch03.quhun_rudui"), 0);
    EXPECT_EQ(state().flag("ch03.tiezhe_zhi"), 0);
}

TEST_F(Ch03ScriptTest, RemoveReportsWhetherAnyoneActuallyLeft) {
    ASSERT_TRUE(state().partyAdd(kCompanion));
    runScript("t/party_remove.lua");

    EXPECT_TRUE(state().party.empty());
    EXPECT_EQ(state().flag("ch03.quhun_rudui"), 1) << "第一次确实有人离队";
    EXPECT_EQ(state().flag("ch03.tiezhe_zhi"), 0) << "第二次没人可离，必须报 false";
}

// ---- 战斗编成（契约第 1.4 节）----

TEST_F(Ch03ScriptTest, AnActiveCompanionTakesTheFieldNextToTheHero) {
    ASSERT_TRUE(state().partyAdd(kCompanion));

    fanren::game::BattleScene scene(kTutorialBattle);
    scene.onEnter(app_);

    const auto& units = scene.battle().units();
    int allies = 0;
    int companion = -1;
    for (std::size_t i = 0; i < units.size(); ++i) {
        if (!units[i].ally) continue;
        ++allies;
        if (units[i].id == kCompanion) companion = static_cast<int>(i);
    }
    EXPECT_EQ(allies, 2) << "韩立加曲魂";
    ASSERT_GE(companion, 0) << "队里 active 的成员必须上场";
    const fanren::core::RoleTemplate* role = app_.data().findRole(kCompanion);
    ASSERT_NE(role, nullptr);
    EXPECT_EQ(units[static_cast<std::size_t>(companion)].maxHp, role->maxHp)
        << "属性照角色模板走";
    EXPECT_EQ(units[static_cast<std::size_t>(companion)].hp, role->maxHp)
        << "hp = -1 表示按模板满血";
    // 从前这里还查「站位不能压在别人身上」（两个单位叠在同一格，按格取人只认得出其中一个）。
    // 横版没有格子：站位由界面按我方的先后排成斜队（docs/octopath-battle.md 6.1），叠不上。
    // 留下的那一半——0 号永远是主角，同伴排在他后面——照判。
    EXPECT_GT(companion, 0) << "0 号永远是韩立，同伴不许顶掉他";
}

TEST_F(Ch03ScriptTest, AnInactiveCompanionStaysOffTheField) {
    ASSERT_TRUE(state().partyAdd(kCompanion));
    state().party[0].active = false;

    fanren::game::BattleScene scene(kTutorialBattle);
    scene.onEnter(app_);

    for (const auto& unit : scene.battle().units()) {
        EXPECT_NE(unit.id, kCompanion) << "留着位置不上场的同伴不该被拉上战场";
    }
}

TEST_F(Ch03ScriptTest, ACompanionNeverShowsUpTwice) {
    // 负向，契约第 1.4 节点名要求的那一条。这里把队伍与主角撞在一起：
    // 队里塞进「hanli」——他是 0 号单位，绝不能在场上出现第二个。
    ASSERT_TRUE(state().partyAdd(kCompanion));
    ASSERT_TRUE(state().partyAdd("hanli"));
    // 同一个 id 入两次本就被 partyAdd 挡住，这里直接手工制造一份坏队伍，
    // 逼编成那一层自己也挡得住（存档被手改、两处各加一次，都会长成这样）。
    state().party.push_back(fanren::core::PartyMember{kCompanion, -1, true});
    ASSERT_EQ(state().party.size(), 3u);

    fanren::game::BattleScene scene(kTutorialBattle);
    scene.onEnter(app_);

    int heroes = 0;
    int companions = 0;
    for (const auto& unit : scene.battle().units()) {
        if (unit.id == "hanli") ++heroes;
        if (unit.id == kCompanion) ++companions;
    }
    EXPECT_EQ(heroes, 1) << "主角不占队伍位，场上只能有一个韩立";
    EXPECT_EQ(companions, 1) << "同一个角色不得同时出现两次";
}

// ---- 识海之战的战果回到脚本手里（契约第 3.4 节）----

TEST_F(Ch03ScriptTest, TheScriptCanReadHowMuchWasBittenOff) {
    // 整条链路：脚本 → BattleScene → CommandResult → api.lua 的三个返回值。
    // 无头模式下 BattleScene 会把整场自动打完，所以这一条跑的就是真实的那一场。
    runScript("t/shihai.lua", /*maxFrames=*/600);

    EXPECT_EQ(state().flag("ch03.shihai_done"), 0) << "这一场赢不了：敌人是跑掉的";
    EXPECT_EQ(state().flag("ch03.mo_siwang"), 1)
        << "脚本必须分得清「敌人跑了」和「韩立输了」，本章后面全押在这个分别上";
    const int spoils = state().flag("ch03.shihai_yaoxia");
    EXPECT_GE(spoils, 33) << "原著是咬下三分之一";
    EXPECT_LE(spoils, 50);
}

TEST_F(Ch03ScriptTest, AnOrdinaryBattleReportsNoSpoilsAndNoFlight) {
    // 反向对照：战果与 "enemy_fled" 不是每场仗都会有的东西。若它们恒有值，
    // 上一条的绿灯就说明不了任何事。
    writeScript(tempAssetRoot() / "scripts" / "t" / "elang.lua",
                "local won, how, spoils = battle('b03_gu_wai_elang')\n"
                "flag.set('ch03.mo_siwang', how == 'enemy_fled' and 1 or 0)\n"
                "flag.set('ch03.shihai_yaoxia', spoils)\n");
    runScript("t/elang.lua", /*maxFrames=*/600);

    EXPECT_EQ(state().flag("ch03.mo_siwang"), 0) << "教学关没人逃";
    EXPECT_EQ(state().flag("ch03.shihai_yaoxia"), 0) << "非吞噬战的战果恒为 0";
}

TEST_F(Ch03ScriptTest, CompanionHpComesBackOutOfTheBattle) {
    ASSERT_TRUE(state().partyAdd(kCompanion));

    fanren::game::BattleScene scene(kTutorialBattle);
    scene.onEnter(app_);
    scene.runToCompletion(200);
    scene.update(app_, 1.0 / 60.0);   // 这一帧里 finish() 会跑

    ASSERT_EQ(state().party.size(), 1u);
    EXPECT_GE(state().party[0].hp, 1) << "带伤出战场之后不该是 -1（按模板满血）了";
    const fanren::core::RoleTemplate* role = app_.data().findRole(kCompanion);
    ASSERT_NE(role, nullptr);
    EXPECT_LE(state().party[0].hp, role->maxHp);
}

}  // namespace
