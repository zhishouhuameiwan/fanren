// 第 2 章掌天瓶那条线的端到端验收：用无头的真 Application 跑 scripts/ch02/ 下
// 真正会上线的那几个脚本，断言绿液真的少了、药材真的扣对了。
//
// 为什么非得跑真脚本不可：第 2 章校对报告的两条 BLOCKER 都不是「某个函数算错
// 了」，而是「脚本说的事在机制上没发生」——
//   * take() 的年份参数被 dispatch 丢掉，章末卖掉的是背包里最便宜的那株种苗，
//     四十四年那株原封不动留着，还能拿去商店再卖一次；
//   * 绿液从头到尾一滴没被消耗，旁白播着「瓶底空了」，灵田面板上写着「绿液 3/3」。
// 这两条只要有人真跑一遍本章就会撞上，而在此之前 tests/ 里对 scripts/ch02/*.lua
// 的引用数是 0。单测再多也挡不住这一类：它们各自都对，错的是拼起来之后。
//
// 与 ScriptApiTests 的分工：那边用 tests/scripts/ 下的夹具脚本逐条钉住 API 的
// 语义与失败路径，这边把玩家实际会走的那几场戏按顺序跑一遍。
#include <gtest/gtest.h>

#include <filesystem>
#include <string>

#include "core/model/Types.h"
#include "core/rules/Bottle.h"
#include "core/rules/Calendar.h"
#include "game/Application.h"
#include "script/Command.h"

namespace {

namespace fs = std::filesystem;

using fanren::core::GameState;
using fanren::game::Application;

constexpr const char* kHerbId = "herb_huangjing_cao";
constexpr const char* kStoneId = "material_lingshi";

// 章末那场算账的三个数，照 herbPrice 实算（黄精基价 5，(age+10)^2/100 倍）：
// 一年份 6、十一年 22、四十四年 145。这里只用后两个——段一那 6 块在别处收的。
constexpr int kElevenYearPrice = 22;
constexpr int kFortyFourYearPrice = 145;

// 仓库根：测试可能从 build/ 或工程根启动。判据用的是本章自己的脚本，
// 而不是随便一个文件——找错根目录时报的是「找不到 ch02 脚本」，不是一串空断言。
std::string assetRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "scripts" / "ch02" / "cuishu.lua")) return candidate;
    }
    return ".";
}

class Ch02BottleTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto ready = app_.init(assetRoot(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
    }

    void TearDown() override { app_.shutdown(); }

    GameState& state() { return app_.state(); }

    // 像主循环那样把脚本推到结束，遇到要玩家回应的命令就替他按确认。
    // 选择一律取第一项：本章的选择只改口吻不改走向（校对报告第四节 4.1），
    // 取哪一项都到得了章末。
    void runScript(const std::string& path) {
        const auto started = app_.startEvent(path);
        ASSERT_TRUE(started.ok) << path << ": " << started.error;
        for (int frame = 0; frame < 4000 && app_.scripts().isRunning(); ++frame) {
            app_.tick(1.0 / 60.0);
            if (app_.awaitingCommand()) {
                fanren::script::CommandResult result;
                result.ok = true;
                result.choiceIndex = 0;
                app_.completeCommand(result);
            }
        }
        ASSERT_FALSE(app_.scripts().isRunning()) << path << " 没能跑到结束";
    }

    // 段五开场时玩家的实际状态：瓶子拿了四年（四年里没有任何消耗口，所以
    // 满瓶三滴），催熟还没解锁，手上有段一发的种苗。
    // 这正是校对报告第四节 4.3 穷举出来的那个状态：走到 shiyao 时 drops 恒为 3。
    void enterChapterFiveState() {
        GameState& s = state();
        s.day = 4 * fanren::rules::kDaysPerYear;
        s.bottle.owned = true;
        s.bottle.matureKnown = false;
        s.bottle.drops = s.bottle.capacity;
        s.bottle.lastChargeDay = s.day;
        s.setFlag("ch02.xiangqi_ping", 1);   // 段二拾瓶那条线已经走完
    }

    Application app_;
};

// ---------------------------------------------------------------------------
// BLOCKER-2：两场戏真的把绿液用掉
// ---------------------------------------------------------------------------

TEST_F(Ch02BottleTest, TheRabbitExperimentReallyPoursADropOutOfTheBottle) {
    GameState& s = state();
    enterChapterFiveState();
    const int before = s.bottle.drops;
    ASSERT_EQ(before, 3) << "段五开场该是满瓶，否则下面的差值没有意义";

    runScript("ch02/shiyao.lua");

    EXPECT_EQ(s.flag("ch02.shiyao_done"), 1) << "这一场该跑完";
    EXPECT_EQ(s.bottle.drops, before - 1)
        << "`ch02.shiyao.dilute`「把那一滴绿的倒进去」——倒出去的那一滴必须真的没了";
    EXPECT_FALSE(s.bottle.matureKnown) << "试药不该顺手解锁催熟，那是明早的事";
}

TEST_F(Ch02BottleTest, TheDiscoverySceneUnlocksMaturingAndHandsOverTheElevenYearHerb) {
    GameState& s = state();
    enterChapterFiveState();
    runScript("ch02/shiyao.lua");

    runScript("ch02/cuishu.lua");   // 第一场：次日清晨，发现催熟

    EXPECT_TRUE(s.bottle.matureKnown);
    EXPECT_EQ(s.flag("ch02.cuishou_unlocked"), 1);
    EXPECT_EQ(s.itemCountOfAge(kHerbId, 11), 1) << "被泼到的那几株里最壮的一株";
    EXPECT_EQ(s.bottle.drops, 2) << "这一场不该再扣绿液：那一滴昨天就倒进碗里了";
}

TEST_F(Ch02BottleTest, ThreeDropsOnOneHerbEmptyTheBottleAndHitTheSpeciesCeiling) {
    // 本章核心教学的端到端：攒满一瓶、三滴全浇在同一株上。
    // 断言的两件事正是校对报告说「机制层是假的」那两件：绿液真的归零，
    // 年份真的被规则层推到这一味药自己的上限。
    GameState& s = state();
    enterChapterFiveState();
    runScript("ch02/shiyao.lua");
    runScript("ch02/cuishu.lua");   // 第一场

    const int dayBefore = s.day;
    runScript("ch02/cuishu.lua");   // 第二场：攒满一瓶浇同一株

    EXPECT_EQ(s.flag("ch02.cuanman_done"), 1);
    EXPECT_EQ(s.bottle.drops, 0) << "三滴全浇出去了，瓶子该是空的";

    const fanren::core::Item* herb = app_.data().findItem(kHerbId);
    ASSERT_NE(herb, nullptr);
    EXPECT_EQ(s.itemCountOfAge(kHerbId, herb->maxAge), 1)
        << "年份该正好落在这一味药自己的上限上（黄精 44 年），不是脚本写死的数";
    EXPECT_EQ(s.itemCountOfAge(kHerbId, 1), 0) << "被浇的那一株不该同时还留在原地";

    // 那二十来天的等待也必须真的过去。绿液从前不被消耗时，这段 advance_days
    // 在主线上永远不执行（校对报告 MEDIUM-1），玩家点完「发现催熟」转身再点
    // 一次同一个 trigger，四十四年份的药就到手了。
    EXPECT_GE(s.day - dayBefore, 21) << "「他数着日子等瓶里凝满」那二十来天被跳过了";
}

// ---------------------------------------------------------------------------
// BLOCKER-1：章末那场算账扣对了药材
// ---------------------------------------------------------------------------

TEST_F(Ch02BottleTest, TheFinalReckoningSellsTheTwoHerbsItTalksAboutAndKeepsTheSeedlings) {
    GameState& s = state();
    enterChapterFiveState();
    runScript("ch02/shiyao.lua");
    runScript("ch02/cuishu.lua");
    runScript("ch02/cuishu.lua");

    // 玩家手上必然还有的那些零年份的苗：段一管事发的三株、厉飞雨送的六株。
    // 章末那两笔扣的若是它们，背包总数看着没错，错的是扣掉了哪一堆。
    s.addItem(kHerbId, 9, 0);
    const int stonesBefore = s.itemCount(kStoneId);

    runScript("ch02/suanzhang.lua");

    EXPECT_EQ(s.flag("ch02.done"), 1) << "章末该抵达终点";
    EXPECT_EQ(s.itemCountOfAge(kHerbId, 11), 0) << "十一年那株卖掉了";
    EXPECT_EQ(s.itemCountOfAge(kHerbId, 44), 0)
        << "四十四年那株也卖掉了——留在背包里就能去商店再卖一次";
    EXPECT_EQ(s.itemCountOfAge(kHerbId, 0), 9) << "一株种苗也不该被顺走";

    // 两笔钱照旧进账，只是章末那个二选一现在真的会花掉一部分：取第一项
    // 「捎回家」按手头实际有多少钱扣掉三分之二，留三分之一给他自己周转。
    // 所以这里断的是「毛收入减去捎走的那一大半」，不是余额直接等于毛收入——
    // 那个二选一从前只置一个旗标、钱一分不动，正是本章校对认定的要害类别。
    const int gross = kElevenYearPrice + kFortyFourYearPrice;
    EXPECT_EQ(s.itemCount(kStoneId) - stonesBefore, gross - gross * 2 / 3)
        << "这回收的是台词里说的那两株，钱也真的捎走了一大半";
    EXPECT_GT(s.itemCount(kStoneId) - stonesBefore, 0) << "两条分支都该给他留点周转的";
}

TEST_F(Ch02BottleTest, TheAgeBlindRemovalWouldHaveSoldTheSeedlingsInstead) {
    // 负向对照：按旧实现（TakeItem 丢掉 y，走 GameState::removeItem）把章末
    // 那两笔扣一遍，证明上一条测试确实抓得住它。
    GameState& s = state();
    s.addItem(kHerbId, 9, 0);
    s.addItem(kHerbId, 1, 11);
    s.addItem(kHerbId, 1, 44);

    ASSERT_TRUE(s.removeItem(kHerbId, 1));   // 这正是旧的 take(黄精, 1, 11)
    ASSERT_TRUE(s.removeItem(kHerbId, 1));   // 这正是旧的 take(黄精, 1, 44)

    EXPECT_EQ(s.itemCountOfAge(kHerbId, 11), 1)
        << "旧实现把十一年那株留在了背包里——上一条测试要拦的正是这个形状";
    EXPECT_EQ(s.itemCountOfAge(kHerbId, 44), 1)
        << "四十四年那株同样还在，卖价 145 的东西可以再卖一次";
    EXPECT_EQ(s.itemCountOfAge(kHerbId, 0), 7) << "被扣掉的是两株不值钱的种苗";
}

}  // namespace
