// 存盘：F5 那一下真的写出了一份读得回来的档。
//
// ---------------------------------------------------------------------------
// 为什么要有这一条
// ---------------------------------------------------------------------------
// `io::saveGame` / `io::loadGame` 造好了、有版本号、有 1→5 的迁移表、有往返测试，
// 但在 `Application::quickSave` 与 `main.cpp` 的 `--load` 出现之前，
// **游戏里一个调用方都没有**：存不了也读不了，想手测第 4 章就得把前三章
// 一口气打完。又是 `docs/README.md` 那张「一个模块可用需要三样齐备」的表——
// 规则层与加载器都在，缺的是游戏层的入口。
//
// 这一条钉的正是那第三段。`SaveFile` 自己的往返、迁移、坏档拒绝由
// `tests/IoTests.cpp` 与 `tests/Ch03PartyTests.cpp` 管着，这里不重复，
// 只问三件这一层才回答得了的事：
//   1. 存下去的是**当下这一局**（改过的旗标、背包、境界都在里头）；
//   2. 剧情演到一半时**存不了**，而且说得出为什么；
//   3. 失败时不静默——`Result` 带着话回来。
#include <gtest/gtest.h>

#include <filesystem>
#include <memory>
#include <string>

#include "TempDir.h"
#include "core/model/Types.h"
#include "core/rules/Realm.h"
#include "game/Application.h"
#include "io/SaveFile.h"

namespace {

namespace fs = std::filesystem;

using fanren::core::GameState;
using fanren::game::Application;
using fanren::rules::Realm;

std::string repoRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "data" / "text" / "ch01_main.json")) {
            return candidate;
        }
    }
    return ".";
}

// 存档写在 <资产根>/saves/quick.sav。从前这一批的资产根就是真仓库，收尾再把 quick.sav 删掉——
// 删的正是手测留下、标题画面「继续旅程」要读的那一份（终审 LOW-8）。现在把 Application 读的那三样
//（data/、scripts/、maps/）拷进一个只属于这一批的临时根：读的是同一份内容，写的是自己的地方，
// 收尾连根删掉，真仓库的 saves/ 一个字节也不碰。
class QuickSave : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        root_ = std::make_unique<fanren::test::TempDir>("fanren_quicksave");
        for (const char* dir : {"data", "scripts", "maps"}) {
            fs::copy(fs::path(repoRoot()) / dir, root_->path() / dir, fs::copy_options::recursive);
        }
    }
    static void TearDownTestSuite() { root_.reset(); }

    void SetUp() override {
        auto ready = app_.init(root_->path().string(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
        // 先验：存档确实落在临时根里。这一条不成立，下面每一条都在写真仓库。
        ASSERT_EQ(fs::path(app_.quickSavePath()).parent_path().parent_path(), root_->path())
            << app_.quickSavePath();
    }
    void TearDown() override { app_.shutdown(); }

    GameState& state() { return app_.state(); }

    static std::unique_ptr<fanren::test::TempDir> root_;
    Application app_;
    std::string savedPath_;
};

std::unique_ptr<fanren::test::TempDir> QuickSave::root_;

// ---------------------------------------------------------------------------
// 正题：存下去的是当下这一局
// ---------------------------------------------------------------------------
// 判据不是「文件存在」——一份全是缺省值的空档也满足「文件存在」。
// 要问的是：**我刚才改的那几样，读回来还在不在。**
TEST_F(QuickSave, WritesWhatIsActuallyOnTheScreenRightNow) {
    GameState& s = state();
    s.mapId = "ch04_luorifeng";
    s.position = fanren::core::Point{12, 9};
    s.realm = Realm::QiRefining8;
    s.realmCap = s.realm;  // 手设境界的保存夹具需满足v9上限约束。
    s.hp = s.maxHp = fanren::rules::realmMaxHp(s.realm);
    s.mp = s.maxMp = fanren::rules::realmMaxMp(s.realm);
    s.setFlag("ch04.huodan_xue");
    s.learnMagic("magic_huodan_shu");
    s.addItem("pill_yangjing_dan", 7, 0);

    auto saved = app_.quickSave();
    ASSERT_TRUE(saved.ok) << saved.error;
    savedPath_ = saved.value;
    ASSERT_TRUE(fs::exists(savedPath_)) << savedPath_;

    auto back = fanren::io::loadGame(savedPath_);
    ASSERT_TRUE(back.ok) << back.error;
    const GameState& r = back.value;
    EXPECT_EQ(r.mapId, "ch04_luorifeng");
    EXPECT_EQ(r.position.x, 12);
    EXPECT_EQ(r.position.y, 9);
    EXPECT_EQ(r.realm, Realm::QiRefining8);
    EXPECT_EQ(r.maxMp, 80);
    EXPECT_EQ(r.flag("ch04.huodan_xue"), 1);
    EXPECT_TRUE(r.knowsMagic("magic_huodan_shu"));
    EXPECT_EQ(r.itemCount("pill_yangjing_dan"), 7);
}

// [配对的负向] 把存下去的那一局改掉，读回来必须**不**一样——
// 少了这一条，上面那串相等可能只是因为两边都是缺省值。
TEST_F(QuickSave, ADifferentRunWritesADifferentSave) {
    state().realm = Realm::QiRefining3;
    state().realmCap = state().realm;
    auto first = app_.quickSave();
    ASSERT_TRUE(first.ok) << first.error;
    savedPath_ = first.value;
    auto a = fanren::io::loadGame(savedPath_);
    ASSERT_TRUE(a.ok);

    state().realm = Realm::QiRefining8;
    state().realmCap = state().realm;
    auto second = app_.quickSave();
    ASSERT_TRUE(second.ok) << second.error;
    auto b = fanren::io::loadGame(savedPath_);
    ASSERT_TRUE(b.ok);

    EXPECT_NE(a.value.realm, b.value.realm)
        << "两局不一样，存出来的档却一样——这一条存的多半不是当下这一局";
    EXPECT_EQ(b.value.realm, Realm::QiRefining8) << "第二次该把第一次盖掉";
}

// ---------------------------------------------------------------------------
// 剧情演到一半时存不了，而且说得出为什么
// ---------------------------------------------------------------------------
// `Application::canSave()` 的理由是 Lua 协程栈无法序列化（方案 3.5 规则 4）。
// 这一条走的是同一道闸：**静默存一份读回来对话演到一半却没有对话的坏档**，
// 比干脆不给存难查得多。
TEST_F(QuickSave, RefusesMidSceneAndSaysSo) {
    // 先确认现在是能存的——否则下面那个「不能存」可能一直都不能存。
    ASSERT_TRUE(app_.canSave()) << "先验：没有脚本在跑的时候本来就该存得了";
    auto fine = app_.quickSave();
    ASSERT_TRUE(fine.ok) << fine.error;
    savedPath_ = fine.value;

    // 起一段真的脚本，让它挂在 talk 上等玩家按键。
    auto started = app_.scripts().startEvent("ch04/huodan.lua");
    ASSERT_TRUE(started.ok) << started.error;
    app_.tick(1.0 / 60.0);
    ASSERT_FALSE(app_.canSave()) << "先验：脚本挂起时本来就不该存得了";

    auto refused = app_.quickSave();
    EXPECT_FALSE(refused.ok) << "剧情演到一半竟然存下去了";
    EXPECT_FALSE(refused.error.empty())
        << "回绝了却一个字不说——静默失败与静默成功在屏幕上长得一模一样";
}

}  // namespace
