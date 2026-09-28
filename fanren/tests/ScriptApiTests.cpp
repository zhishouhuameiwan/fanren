// P3 第 2 章脚本 API 增补的端到端验收（契约 docs/interfaces-p3-script.md 第 5 节）。
//
// 驱动的是玩家实际运行的那一整条链路：api.lua 的封装 → ScriptHost 的解析表 →
// Application::dispatch → GameState。只测 dispatch 的话，一个拼错的 kind 字符串
// 就能让脚本与引擎在测试全绿的情况下各说各话。
//
// 最要紧的一条是 BottleGrant 把 lastChargeDay 拨到当日：它默认 0，不拨的话拾瓶
// 那一刻就按「已过一百天」一次性补满绿液，原著「第八日方得一滴」当场作废。
// 这条检查配了一条负向对照（WithoutTheClockReset…），证明它真的抓得住人。
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>

#include "core/battle/Battle.h"
#include "core/model/Types.h"
#include "core/rules/Bottle.h"
#include "core/rules/Calendar.h"
#include "core/rules/Field.h"
#include "core/rules/Realm.h"
#include "game/Application.h"
#include "game/BattleScene.h"

namespace {

namespace fs = std::filesystem;

using fanren::core::GameState;
using fanren::game::Application;

// 契约第 4 节约定的灵田 id，编剧与关卡都按它写。
constexpr const char* kFieldId = "shenshougu_yaopu";
// 第 1 章谷中那味最常见的灵草，data/items/herbs 里实有此条。
constexpr const char* kSeedId = "herb_qingfeng_cao";
// 第 2 章从头到尾在种、在卖、在催的那一味。一阶，上限 44 年（1 → 11 → 22 → 44
// 正好三滴，与炼气期的满瓶容量对上）。
constexpr const char* kHerbId = "herb_huangjing_cao";
// 同为灵草但上限不同的一味（二阶，100 年），用来验「上限按种，不是全局」。
constexpr const char* kHerbId2 = "herb_zishen_cao";

// 本套测试自己的临时资源根。不能直接用仓库根：夹具脚本必须和 api.lua 同根才
// 加载得到，而正式的 scripts/ 目录不该为了测试方便塞进几个假脚本。
fs::path& tempAssetRoot() {
    static fs::path path;
    return path;
}

// 每次跑都换一个目录名。从前这里写死成 %TEMP%\fanren_script_api_root，
// 两个测试进程同时跑（并行的 ctest、两个代理各自 build）就会互相 remove_all
// 对方正在读的 data/，症状是偶发的红灯加一句「复制 data/ 失败」——而重跑一次
// 又是绿的，于是没人查得下去。口径照 tests/IoTests.cpp 的 TempDir：
// 时间戳 + 进程内自增计数，进程之间与进程之内都不会撞。
fs::path uniqueTempRoot() {
    static std::atomic<std::uint64_t> counter{0};
    const auto stamp = static_cast<std::uint64_t>(
        std::chrono::steady_clock::now().time_since_epoch().count());
    return fs::temp_directory_path() /
           ("fanren_script_api_root_" + std::to_string(stamp) + "_" +
            std::to_string(counter.fetch_add(1)));
}

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

class ScriptApiTest : public ::testing::Test {
protected:
    // 资源根只搭一次：data/ 有上百个文件，每个用例复制一遍纯属浪费，
    // 而它全程只读，用例之间不会互相弄脏。可变状态都在各自的 Application 里。
    static void SetUpTestSuite() {
        const fs::path source = findProjectRoot();
        ASSERT_FALSE(source.empty()) << "找不到工程根目录（应含 scripts/common/api.lua 与 data/）";

        fs::path& root = tempAssetRoot();
        root = uniqueTempRoot();
        std::error_code ec;
        fs::remove_all(root, ec);
        fs::create_directories(root / "scripts" / "common", ec);
        fs::create_directories(root / "scripts" / "t", ec);

        fs::copy(source / "data", root / "data", fs::copy_options::recursive, ec);
        ASSERT_FALSE(ec) << "复制 data/ 失败: " << ec.message();

        // 跑的是真的 api.lua，不是复刻品：封装写错了要在这里红，不是等上线。
        fs::copy_file(source / "scripts" / "common" / "api.lua", root / "scripts" / "common" / "api.lua",
                      fs::copy_options::overwrite_existing, ec);
        ASSERT_FALSE(ec) << "复制 api.lua 失败: " << ec.message();

        for (const auto& entry : fs::directory_iterator(source / "tests" / "scripts")) {
            if (entry.path().extension() != ".lua") continue;
            fs::copy_file(entry.path(), root / "scripts" / "t" / entry.path().filename(),
                          fs::copy_options::overwrite_existing, ec);
            ASSERT_FALSE(ec) << "复制 " << entry.path().string() << " 失败: " << ec.message();
        }
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

    // 像主循环那样把脚本推到结束。maxFrames 是死循环闸：脚本卡住时宁可测试
    // 失败，也不要挂住整个 ctest。
    void runScript(const std::string& path) {
        const auto started = app_.startEvent(path);
        ASSERT_TRUE(started.ok) << path << ": " << started.error;
        for (int frame = 0; frame < 64 && app_.scripts().isRunning(); ++frame) {
            app_.tick(1.0 / 60.0);
        }
        ASSERT_FALSE(app_.scripts().isRunning()) << path << " 没能跑到结束";
    }

    // 当场写一个脚本进夹具根再跑它。
    //
    // 为什么不像别的用例那样往 tests/scripts/ 放一个 .lua：`realm_advance` 这一批
    // 的白名单里没有 tests/scripts/，而 scripts/common/api.lua 也不在
    //（第 4 章的编剧正在同一棵树上改 scripts/**）。于是新命令暂时还没有
    // api.lua 的糖衣，这里写的是**糖衣将来要展开成的那个样子**：
    // 一张 { kind = "realm_advance", x = 编号 } 的命令表 + 一次 yield，
    // 与 api.lua 里 emit() 做的事一字不差（见 scripts/common/api.lua 第 18 行）。
    //
    // 这条路仍然走的是玩家那条真链路：ScriptHost 的 kindTable 解析 → dispatch
    // → GameState，只有最外面那层 Lua 函数名是测试自己写的。
    void runInlineScript(const std::string& name, const std::string& luaSource) {
        const fs::path path = tempAssetRoot() / "scripts" / "t" / name;
        {
            std::ofstream out(path, std::ios::binary | std::ios::trunc);
            ASSERT_TRUE(out.good()) << "写不出夹具脚本 " << path.string();
            out << luaSource;
        }
        runScript("t/" + name);
    }

    // 把瓶子直接摆成想要的样子。不走 t/ch02_grant.lua 是因为这些用例要的是
    // 「瓶子处在某个状态时，催熟会怎样」，拾瓶那条链路另有测试钉着。
    void makeBottle(bool owned, bool matureKnown, int drops) {
        state().bottle.owned = owned;
        state().bottle.matureKnown = matureKnown;
        state().bottle.drops = drops;
        state().bottle.lastChargeDay = state().day;
    }

    // data 里登记的年份上限。测试不写 44 这个魔数：上限改了，测试该跟着改，
    // 但不该是「改了 data 却没人发现测试在验一个旧数」。
    [[nodiscard]] int dataMaxAge(const std::string& itemId) {
        const fanren::core::Item* item = app_.data().findItem(itemId);
        return item == nullptr ? 0 : item->maxAge;
    }

    // 造一块灵田。返回 id 而不是引用：fields 再 push 一次引用就悬空了。
    void makeField(const std::string& id, int slots) {
        fanren::rules::SpiritField field;
        field.id = id;
        field.slots.resize(static_cast<std::size_t>(slots));
        state().fields.push_back(std::move(field));
    }

    Application app_;
};

// ---------------------------------------------------------------------------
// BottleGrant：契约第 5.1 / 5.2 条
// ---------------------------------------------------------------------------

TEST_F(ScriptApiTest, BottleGrantResetsTheChargeClockToTheDayItIsPickedUp) {
    GameState& s = state();
    s.day = 100;   // 刻意不是第 1 天：计时不拨的话差别才看得出来

    runScript("t/ch02_grant.lua");

    EXPECT_TRUE(s.bottle.owned);
    EXPECT_EQ(s.bottle.lastChargeDay, 100) << "计时必须拨到当日";
    EXPECT_EQ(s.bottle.drops, 0) << "拾瓶当日一滴绿液也不该有";
    EXPECT_FALSE(s.bottle.matureKnown) << "拾瓶不隐含会用，那是四年后的另一个节点";

    app_.advanceDays(6);   // 第七日
    EXPECT_EQ(s.day, 106);
    EXPECT_EQ(s.bottle.drops, 0) << "第七日仍无";

    app_.advanceDays(1);   // 自拾瓶起第八日
    EXPECT_EQ(s.bottle.drops, 1) << "原著：第八日瓶盖方开，内有一滴绿液";
}

TEST_F(ScriptApiTest, WithoutTheClockResetTheBottleWouldBeFullTheNextDay) {
    // 负向对照：手工复现「忘了拨 lastChargeDay」的实现，证明上一条测试抓得住它。
    // 没有这一条，上面那串 EXPECT_EQ(drops, 0) 有可能只是碰巧成立。
    GameState& s = state();
    s.day = 100;
    runScript("t/ch02_grant.lua");
    ASSERT_EQ(s.bottle.lastChargeDay, 100);

    s.bottle.lastChargeDay = 0;   // 这正是不重置时字段的样子
    app_.advanceDays(1);

    EXPECT_EQ(s.bottle.drops, s.bottle.capacity)
        << "计时留在 0 的话拾瓶次日就直接满瓶——这就是上一条测试要拦的形状";
}

TEST_F(ScriptApiTest, BottleGrantIsIdempotentAndKeepsThePartialCharge) {
    GameState& s = state();
    s.day = 1;
    runScript("t/ch02_grant.lua");
    ASSERT_EQ(s.bottle.lastChargeDay, 1);

    app_.advanceDays(10);   // 第 11 日：凝出一滴，账上还剩三天零头
    ASSERT_EQ(s.bottle.drops, 1);
    ASSERT_EQ(s.bottle.lastChargeDay, 8);

    runScript("t/ch02_grant.lua");   // 剧情回放、读档重跑都可能再走一次
    EXPECT_EQ(s.bottle.lastChargeDay, 8) << "重复拾瓶不该重置计时";
    EXPECT_EQ(s.bottle.drops, 1);

    // 再过四天就凑满第二个周期。计时若被重置到第 11 日，这里只过了 4 天，
    // 一滴也凝不出来——零头被抹掉的后果正是在这一步现形。
    app_.advanceDays(4);
    EXPECT_EQ(s.bottle.drops, 2) << "已攒的三天零头被抹掉了";
}

// ---------------------------------------------------------------------------
// BottleUnlockMature
// ---------------------------------------------------------------------------

TEST_F(ScriptApiTest, BottleUnlockMatureDoesNotImplyOwningTheBottle) {
    GameState& s = state();
    runScript("t/ch02_unlock_mature.lua");

    EXPECT_TRUE(s.bottle.matureKnown);
    EXPECT_FALSE(s.bottle.owned) << "契约：不替脚本兜底，写错的调用顺序要看得见";

    // 没瓶子就不凝液，哪怕过了一百年。
    app_.advanceDays(fanren::rules::kDaysPerYear);
    EXPECT_EQ(s.bottle.drops, 0);

    runScript("t/ch02_unlock_mature.lua");   // 幂等
    EXPECT_TRUE(s.bottle.matureKnown);
    EXPECT_FALSE(s.bottle.owned);
}

// ---------------------------------------------------------------------------
// FieldUnlock：契约第 5.3 条
// ---------------------------------------------------------------------------

TEST_F(ScriptApiTest, FieldUnlockOpensAFieldWithTheRequestedSlots) {
    GameState& s = state();
    s.setFlag("test_slots", 8);   // 契约第 4 节：神手谷药圃 8 槽

    runScript("t/ch02_field_unlock.lua");

    ASSERT_EQ(s.fields.size(), 1u);
    const fanren::rules::SpiritField* field = s.findField(kFieldId);
    ASSERT_NE(field, nullptr);
    EXPECT_EQ(field->slots.size(), 8u);
    for (const fanren::rules::FieldSlot& slot : field->slots) {
        EXPECT_TRUE(slot.seedId.empty());
    }
}

TEST_F(ScriptApiTest, FieldUnlockTopsUpAnExistingFieldWithoutClearingIt) {
    GameState& s = state();
    s.day = 1;
    makeField(kFieldId, 2);
    ASSERT_TRUE(fanren::rules::plant(*s.findField(kFieldId), 0, kSeedId, s.day));

    // 让这株药真的长一年：清空 slots 的实现会把「种了几年」一起抹掉，
    // 而空槽位和被清空的槽位长得一模一样，必须让它带上年份才分得出来。
    app_.advanceDays(fanren::rules::kDaysPerYear);
    ASSERT_EQ(s.findField(kFieldId)->slots[0].age, 1);
    ASSERT_TRUE(s.findField(kFieldId)->slots[0].ripe);

    s.setFlag("test_slots", 8);
    runScript("t/ch02_field_unlock.lua");

    ASSERT_EQ(s.fields.size(), 1u) << "同 id 的田不该再造一块出来";
    const fanren::rules::SpiritField* field = s.findField(kFieldId);
    ASSERT_NE(field, nullptr);
    EXPECT_EQ(field->slots.size(), 8u) << "槽位该补到 8";
    EXPECT_EQ(field->slots[0].seedId, kSeedId) << "重建 slots 会清空玩家种了几年的药";
    EXPECT_EQ(field->slots[0].age, 1);
    EXPECT_TRUE(field->slots[0].ripe);
    EXPECT_EQ(field->slots[0].plantedDay, 1 + fanren::rules::kDaysPerYear)
        << "播种「纪念日」也不该被重置";
    for (std::size_t i = 1; i < field->slots.size(); ++i) {
        EXPECT_TRUE(field->slots[i].seedId.empty()) << "第 " << i << " 槽";
    }
}

TEST_F(ScriptApiTest, FieldUnlockNeverShrinksAnExistingField) {
    GameState& s = state();
    s.day = 1;
    makeField(kFieldId, 8);
    ASSERT_TRUE(fanren::rules::plant(*s.findField(kFieldId), 7, kSeedId, s.day));

    s.setFlag("test_slots", 2);   // 比现有槽位少
    runScript("t/ch02_field_unlock.lua");

    const fanren::rules::SpiritField* field = s.findField(kFieldId);
    ASSERT_NE(field, nullptr);
    ASSERT_EQ(field->slots.size(), 8u) << "只补足，不缩小";
    EXPECT_EQ(field->slots[7].seedId, kSeedId) << "缩小会连同末尾槽位里的药一起丢掉";
}

TEST_F(ScriptApiTest, FieldUnlockWithAnEmptyIdFailsInsideLua) {
    // 负向：api.lua 侧的 assert 该当场炸掉，脚本不再往下走。
    GameState& s = state();
    const auto started = app_.startEvent("t/ch02_field_empty_id.lua");

    EXPECT_FALSE(started.ok);
    EXPECT_FALSE(started.error.empty());
    EXPECT_EQ(s.flag("empty_field_id_slipped_through"), 0)
        << "assert 没拦住，脚本继续往下执行了";
    EXPECT_TRUE(s.fields.empty());
}

TEST_F(ScriptApiTest, FieldUnlockReportsFailureWhenTheLuaGuardIsBypassed) {
    // 负向：命令表将来也可能由存档回放或编辑器生成，那些路径上没有 assert 兜着，
    // 所以 dispatch 自己也得判 —— 契约要求这种情况回填 ok = false。
    GameState& s = state();
    runScript("t/ch02_field_raw_bad.lua");

    EXPECT_EQ(s.flag("no_id_ok"), 0) << "空 id 该报失败";
    EXPECT_EQ(s.flag("no_slots_ok"), 0) << "槽位数 0 该报失败";
    EXPECT_EQ(s.flag("negative_ok"), 0) << "槽位数为负该报失败";
    // 对照组：没有它，上面三个 0 也可能只是「这条命令永远失败」。
    EXPECT_EQ(s.flag("good_ok"), 1) << "参数合法时必须成功";

    ASSERT_EQ(s.fields.size(), 1u) << "只有合法的那一条该建出田来";
    EXPECT_EQ(s.fields[0].id, kFieldId);
    EXPECT_EQ(s.fields[0].slots.size(), 2u);
}

// ---------------------------------------------------------------------------
// AdvanceDays：契约第 5.4 条
// ---------------------------------------------------------------------------

TEST_F(ScriptApiTest, AdvanceDaysIgnoresZeroAndNegativeSpans) {
    GameState& s = state();
    s.day = 50;

    s.setFlag("test_days", 0);
    runScript("t/ch02_advance.lua");
    EXPECT_EQ(s.day, 50) << "传 0 不该算错误，也不该动日历";

    s.setFlag("test_days", -30);
    runScript("t/ch02_advance.lua");
    EXPECT_EQ(s.day, 50) << "日历不许倒退：倒退会让所有「上次结算日」落到未来";
}

TEST_F(ScriptApiTest, AdvanceDaysIsCappedPerCall) {
    // 脚本里多打一个零不该让灵田结算空转几十万次。
    GameState& s = state();
    s.day = 1;
    s.setFlag("test_days", 1000000);

    runScript("t/ch02_advance.lua");

    EXPECT_EQ(s.day, 1 + fanren::game::kMaxScriptAdvanceDays);
}

TEST_F(ScriptApiTest, AdvanceDaysSettlesFieldsAndBottleThroughTheOneTimeEntry) {
    // 证明这条命令是转调 Application::advanceDays 而不是另写了一份结算：
    // 另写的那份必定漏项，而漏掉的项在存档里看不出来。
    GameState& s = state();
    s.day = 1;
    runScript("t/ch02_grant.lua");
    makeField(kFieldId, 2);
    ASSERT_TRUE(fanren::rules::plant(*s.findField(kFieldId), 0, kSeedId, s.day));

    s.setFlag("test_days", fanren::rules::kDaysPerYear);
    runScript("t/ch02_advance.lua");

    EXPECT_EQ(s.day, 1 + fanren::rules::kDaysPerYear);
    const fanren::rules::SpiritField* field = s.findField(kFieldId);
    ASSERT_NE(field, nullptr);
    EXPECT_EQ(field->slots[0].age, 1) << "灵田没跟着长";
    EXPECT_TRUE(field->slots[0].ripe);
    EXPECT_EQ(s.bottle.drops, s.bottle.capacity) << "绿液没跟着凝";
}

// ---------------------------------------------------------------------------
// 查询：剧情闸门的真实用法
// ---------------------------------------------------------------------------

TEST_F(ScriptApiTest, GatesOpenWhenTheQueriedStateSaysSo) {
    GameState& s = state();
    s.day = 77;
    s.bottle.owned = true;
    s.bottle.drops = 1;
    makeField(kFieldId, 2);
    ASSERT_TRUE(fanren::rules::plant(*s.findField(kFieldId), 0, kSeedId, s.day));
    s.findField(kFieldId)->slots[0].ripe = true;

    runScript("t/ch02_gate.lua");

    EXPECT_EQ(s.flag("gate_bottle_ready"), 1);
    EXPECT_EQ(s.flag("gate_planted"), 1);
    EXPECT_EQ(s.flag("gate_day"), 77);
    EXPECT_EQ(s.flag("gate_ripe"), 1);
}

TEST_F(ScriptApiTest, GatesStayShutWhenTheConditionsAreNotMet) {
    // 负向：闸门条件不成立时一个都不该开。只验「开得了」的话，一个永远返回真的
    // 查询也能让上一条测试全绿。
    GameState& s = state();
    s.day = 3;
    s.bottle.owned = true;
    s.bottle.drops = 0;   // 还没攒够一滴
    makeField(kFieldId, 2);   // 田开了，但一株也没种

    runScript("t/ch02_gate.lua");

    EXPECT_EQ(s.flag("gate_bottle_ready"), 0) << "绿液不足，闸门不该开";
    EXPECT_EQ(s.flag("gate_planted"), 0) << "一株未种，闸门不该开";
    EXPECT_EQ(s.flag("gate_ripe"), 0);
    EXPECT_EQ(s.flag("gate_day"), 3);
}

// ---------------------------------------------------------------------------
// TakeItem 的灵草年份（BLOCKER-1）
//
// 契约表里 TakeItem 的 y 一直写着「灵草年份」，dispatch 却从来没读过它。
// 于是章末 take(黄精, 1, 44) 扣走的是背包里年份最低的那株种苗：钱照拿、
// 四十四年那株原封不动留着，商店落地后还能再卖一次。
// ---------------------------------------------------------------------------

TEST_F(ScriptApiTest, TakeWithAnAgeRemovesThatPileNotTheCheapestOne) {
    GameState& s = state();
    s.addItem(kHerbId, 1, 1);    // 段一发的种苗，一年份
    s.addItem(kHerbId, 1, 44);   // 攒满三滴浇出来的那一株
    s.setFlag("test_age", 44);

    runScript("t/ch02_take_age.lua");

    EXPECT_EQ(s.flag("take_ok"), 1) << "背包里明明有这一堆";
    EXPECT_EQ(s.itemCountOfAge(kHerbId, 44), 0) << "脚本要扣的是四十四年那一株";
    EXPECT_EQ(s.itemCountOfAge(kHerbId, 1), 1) << "一年份那株不该被顺手扣掉";
}

TEST_F(ScriptApiTest, TheAgeBlindRemovalWouldHaveTakenTheWrongPile) {
    // 负向对照：手工跑一遍旧实现那一行（GameState::removeItem），证明上一条
    // 测试确实抓得住它。没有这一条，上面那两个 EXPECT 有可能只是碰巧成立。
    GameState& s = state();
    s.addItem(kHerbId, 1, 1);
    s.addItem(kHerbId, 1, 44);

    ASSERT_TRUE(s.removeItem(kHerbId, 1)) << "这正是旧的 TakeItem 那一行";

    EXPECT_EQ(s.itemCountOfAge(kHerbId, 44), 1)
        << "旧实现把四十四年那株留给了玩家——上一条测试要拦的正是这个形状";
    EXPECT_EQ(s.itemCountOfAge(kHerbId, 1), 0) << "旧实现扣的是年份最低的那一株";
}

TEST_F(ScriptApiTest, TakeWithAnAgeFailsInsteadOfFallingBackToAnotherPile) {
    // 负向：指名的那一堆没货时必须如实报失败，而不是转头扣掉别的年份。
    // 「钱照给、货没动」与「钱照给、扣错了货」是同一个坑的两张脸。
    GameState& s = state();
    s.addItem(kHerbId, 2, 1);
    s.setFlag("test_age", 44);

    runScript("t/ch02_take_age.lua");

    EXPECT_EQ(s.flag("take_ok"), 0) << "手上没有四十四年那一株，该报失败";
    EXPECT_EQ(s.itemCountOfAge(kHerbId, 1), 2) << "一株也不该动";
}

TEST_F(ScriptApiTest, TakeWithoutAnAgeStillTakesTheCheapestPile) {
    // y 省略（api.lua 补成 0）是「哪一堆都行」，不是「正好 0 年那一株」。
    // zhitong.lua 段四交药抵人情走的就是这一条：解释成精确年份会让手上只剩
    // 足年药的玩家在那里卡死。
    GameState& s = state();
    s.addItem(kHerbId, 1, 1);
    s.addItem(kHerbId, 1, 44);

    runScript("t/ch02_take_any.lua");

    EXPECT_EQ(s.flag("take_ok"), 1);
    EXPECT_EQ(s.itemCountOfAge(kHerbId, 1), 0) << "先扣年份低的";
    EXPECT_EQ(s.itemCountOfAge(kHerbId, 44), 1) << "高年份留给玩家";
}

TEST_F(ScriptApiTest, TakeWithoutAnAgeWorksWhenNoZeroYearPileExists) {
    // 上一条的另一半：包里一株 0 年的也没有时，不指定年份仍然要能扣得动。
    // 这正是「把 0 当成正好 0 年」会当场断掉的那条路。
    GameState& s = state();
    s.addItem(kHerbId, 1, 11);

    runScript("t/ch02_take_any.lua");

    EXPECT_EQ(s.flag("take_ok"), 1) << "不指定年份时，十一年那株也该扣得动";
    EXPECT_EQ(s.itemCount(kHerbId), 0);
}

TEST_F(ScriptApiTest, TakeReportsFailureWhenTheBagIsEmpty) {
    GameState& s = state();

    runScript("t/ch02_take_any.lua");

    EXPECT_EQ(s.flag("take_ok"), 0) << "空背包该报失败，让脚本能走「你没有这个」";
}

// ---------------------------------------------------------------------------
// BottleSpend：把绿液本身用掉（BLOCKER-2 的前一半）
// ---------------------------------------------------------------------------

TEST_F(ScriptApiTest, SpendingADropTakesItOutOfTheBottle) {
    GameState& s = state();
    makeBottle(/*owned=*/true, /*matureKnown=*/false, /*drops=*/3);
    s.setFlag("test_drops", 1);

    runScript("t/ch02_spend.lua");

    EXPECT_EQ(s.flag("spend_ok"), 1);
    EXPECT_EQ(s.bottle.drops, 2) << "倒出去的那一滴必须真的从瓶里没了";
    EXPECT_EQ(s.flag("drops_after"), 2) << "脚本当场查到的数也要跟着变";
    EXPECT_FALSE(s.bottle.matureKnown) << "倒液不该顺手解锁催熟";
}

TEST_F(ScriptApiTest, SpendingMoreDropsThanTheBottleHoldsTakesNone) {
    // 负向：不够就一滴不扣。扣一半再报失败的话，玩家的绿液会在一次失败的
    // 调用里凭空少掉，而这种账在存档里看不出来。
    GameState& s = state();
    makeBottle(/*owned=*/true, /*matureKnown=*/false, /*drops=*/1);
    s.setFlag("test_drops", 3);

    runScript("t/ch02_spend.lua");

    EXPECT_EQ(s.flag("spend_ok"), 0);
    EXPECT_EQ(s.bottle.drops, 1) << "失败的调用不许动瓶子";
}

// ---------------------------------------------------------------------------
// BottleMature：催熟走规则层（BLOCKER-2 的后一半）
// ---------------------------------------------------------------------------

TEST_F(ScriptApiTest, MaturingAHerbSpendsADropAndRaisesTheAgeByTheRule) {
    GameState& s = state();
    makeBottle(/*owned=*/true, /*matureKnown=*/true, /*drops=*/3);
    s.addItem(kHerbId, 1, 1);
    s.setFlag("test_age", 1);

    runScript("t/ch02_mature.lua");

    EXPECT_EQ(s.flag("mature_ok"), 1);
    EXPECT_EQ(s.flag("code_none"), 1) << "成功时不该带失败原因码";
    // 11 = rules::matureHerb 的「当前 + max(当前, 10)」，不是脚本里写死的数。
    EXPECT_EQ(s.flag("mature_age"), 11);
    EXPECT_EQ(s.bottle.drops, 2) << "一滴绿液必须真的少掉";
    EXPECT_EQ(s.itemCountOfAge(kHerbId, 1), 0) << "浇的是这一株";
    EXPECT_EQ(s.itemCountOfAge(kHerbId, 11), 1) << "变年份的也得是这一株";
}

TEST_F(ScriptApiTest, MaturingTakesTheWateredPileNotTheCheapestOne) {
    // 与 TakeItem 同一个坑：走 removeItem 的话，玩家浇的是这一株、
    // 变年份的是另一株，而背包总数不变，谁也看不出来。
    GameState& s = state();
    makeBottle(/*owned=*/true, /*matureKnown=*/true, /*drops=*/1);
    s.addItem(kHerbId, 1, 0);    // 刚下的种苗
    s.addItem(kHerbId, 1, 22);   // 他要浇的那一株
    s.setFlag("test_age", 22);

    runScript("t/ch02_mature.lua");

    EXPECT_EQ(s.flag("mature_ok"), 1);
    EXPECT_EQ(s.flag("mature_age"), 44);
    EXPECT_EQ(s.itemCountOfAge(kHerbId, 0), 1) << "种苗不该被顺走";
    EXPECT_EQ(s.itemCountOfAge(kHerbId, 22), 0);
    EXPECT_EQ(s.itemCountOfAge(kHerbId, 44), 1);
}

TEST_F(ScriptApiTest, MaturingWithoutTheBottleFailsWithItsOwnReason) {
    GameState& s = state();
    makeBottle(/*owned=*/false, /*matureKnown=*/true, /*drops=*/3);
    s.addItem(kHerbId, 1, 1);
    s.setFlag("test_age", 1);

    runScript("t/ch02_mature.lua");

    EXPECT_EQ(s.flag("mature_ok"), 0);
    EXPECT_EQ(s.flag("code_no_bottle"), 1) << "四条失败路径要分得开";
    EXPECT_EQ(s.flag("mature_age"), 1) << "失败时年份不动";
    EXPECT_EQ(s.bottle.drops, 3) << "失败时一滴也不该扣";
    EXPECT_EQ(s.itemCountOfAge(kHerbId, 1), 1);
}

TEST_F(ScriptApiTest, MaturingBeforeTheAbilityIsUnlockedFailsWithItsOwnReason) {
    // 「绿瓶四年」那四年里玩家撞见的就是这一条：瓶子在手、绿液也有，
    // 就是还不知道它能催熟。
    GameState& s = state();
    makeBottle(/*owned=*/true, /*matureKnown=*/false, /*drops=*/3);
    s.addItem(kHerbId, 1, 1);
    s.setFlag("test_age", 1);

    runScript("t/ch02_mature.lua");

    EXPECT_EQ(s.flag("mature_ok"), 0);
    EXPECT_EQ(s.flag("code_mature_unknown"), 1);
    EXPECT_EQ(s.bottle.drops, 3);
    EXPECT_EQ(s.itemCountOfAge(kHerbId, 1), 1);
}

TEST_F(ScriptApiTest, MaturingWithAnEmptyBottleFailsWithItsOwnReason) {
    GameState& s = state();
    makeBottle(/*owned=*/true, /*matureKnown=*/true, /*drops=*/0);
    s.addItem(kHerbId, 1, 1);
    s.setFlag("test_age", 1);

    runScript("t/ch02_mature.lua");

    EXPECT_EQ(s.flag("mature_ok"), 0);
    EXPECT_EQ(s.flag("code_no_drops"), 1);
    EXPECT_EQ(s.itemCountOfAge(kHerbId, 1), 1) << "没浇成就不该改年份";
}

TEST_F(ScriptApiTest, MaturingASeedlingThatIsNotAYearOldFailsWithItsOwnReason) {
    // 规则层后加的那一条（未足年不准催）也要有自己的码，不能落进「空原因」。
    // matureFailureCode() 的 switch 漏掉一个枚举值时，脚本收到的是一个空串：
    // 编译期不报、运行期不响，只有这条测试看得见。
    GameState& s = state();
    makeBottle(/*owned=*/true, /*matureKnown=*/true, /*drops=*/3);
    s.addItem(kHerbId, 1, 0);   // 刚下种的苗
    s.setFlag("test_age", 0);

    runScript("t/ch02_mature.lua");

    EXPECT_EQ(s.flag("mature_ok"), 0);
    EXPECT_EQ(s.flag("code_not_ripe"), 1) << "回填成空串的话，夹具置的会是 code_none";
    EXPECT_EQ(s.bottle.drops, 3) << "浇不进去就不该扣绿液";
    EXPECT_EQ(s.itemCountOfAge(kHerbId, 0), 1);
}

TEST_F(ScriptApiTest, MaturingAHerbAlreadyAtItsCeilingFailsAndKeepsTheDrop) {
    // 第四滴那一下：`ch02.cuanman.stop`「再往下滴，纹路不添了」说的就是它。
    // 不添年份的同时**也不许扣绿液**，否则玩家会为一次什么都没发生的操作付钱。
    GameState& s = state();
    const int ceiling = dataMaxAge(kHerbId);
    ASSERT_GT(ceiling, 0) << "data/items/herbs 里应当写着黄精的 maxAge";
    makeBottle(/*owned=*/true, /*matureKnown=*/true, /*drops=*/2);
    s.addItem(kHerbId, 1, ceiling);
    s.setFlag("test_age", ceiling);

    runScript("t/ch02_mature.lua");

    EXPECT_EQ(s.flag("mature_ok"), 0);
    EXPECT_EQ(s.flag("code_at_max_age"), 1);
    EXPECT_EQ(s.flag("mature_age"), ceiling) << "到顶了就是到顶了";
    EXPECT_EQ(s.bottle.drops, 2) << "白浇一次不该扣绿液";
    EXPECT_EQ(s.itemCountOfAge(kHerbId, ceiling), 1);
}

TEST_F(ScriptApiTest, MaturingSomethingNotInTheBagFailsWithItsOwnReason) {
    // 第五种失败：规则层不认识背包，「浇不动」和「根本没这株药」得分得开。
    GameState& s = state();
    makeBottle(/*owned=*/true, /*matureKnown=*/true, /*drops=*/3);
    s.addItem(kHerbId, 1, 1);
    s.setFlag("test_age", 44);   // 手上那株是一年份的

    runScript("t/ch02_mature.lua");

    EXPECT_EQ(s.flag("mature_ok"), 0);
    EXPECT_EQ(s.flag("code_no_item"), 1);
    EXPECT_EQ(s.bottle.drops, 3) << "没药可浇时不该把绿液倒掉";
    EXPECT_EQ(s.itemCountOfAge(kHerbId, 1), 1);
}

TEST_F(ScriptApiTest, TheCeilingIsPerSpeciesNotGlobal) {
    // 同样四十四年、同样一滴：黄精（上限 44）到顶了，紫参草（上限 100）还推得动。
    // 若改回全局上限 kMaxHerbAge，a 那一半会变成「成功，88 年」而当场红。
    GameState& s = state();
    makeBottle(/*owned=*/true, /*matureKnown=*/true, /*drops=*/2);
    s.addItem(kHerbId, 1, 44);
    s.addItem(kHerbId2, 1, 44);
    ASSERT_EQ(dataMaxAge(kHerbId), 44);
    ASSERT_EQ(dataMaxAge(kHerbId2), 100);

    runScript("t/ch02_mature_species.lua");

    EXPECT_EQ(s.flag("a_ok"), 0) << "黄精四十四年已至上限";
    EXPECT_EQ(s.flag("a_code_at_max_age"), 1);
    EXPECT_EQ(s.flag("a_age"), 44);

    EXPECT_EQ(s.flag("b_ok"), 1) << "紫参草上限一百年，这一滴还推得动";
    EXPECT_EQ(s.flag("b_age"), 88);
    EXPECT_EQ(s.itemCountOfAge(kHerbId2, 88), 1);

    EXPECT_EQ(s.bottle.drops, 1) << "只该扣掉推得动的那一滴";
}

TEST_F(ScriptApiTest, ThreeDropsOnOneHerbEmptyTheBottleAndReachTheCeiling) {
    // 第 2 章节点 11 的机制原型：1 → 11 → 22 → 44，三滴正好到顶、正好倒空。
    GameState& s = state();
    makeBottle(/*owned=*/true, /*matureKnown=*/true, /*drops=*/3);
    s.addItem(kHerbId, 1, 1);
    s.setFlag("test_age", 1);

    runScript("t/ch02_mature_three.lua");

    EXPECT_EQ(s.flag("age_after_1"), 11);
    EXPECT_EQ(s.flag("age_after_2"), 22);
    EXPECT_EQ(s.flag("age_after_3"), 44);
    EXPECT_EQ(s.flag("final_age"), dataMaxAge(kHerbId)) << "三滴正好推到这一味药的顶";
    EXPECT_EQ(s.bottle.drops, 0) << "三滴全用掉了，瓶子该是空的";
    EXPECT_EQ(s.itemCountOfAge(kHerbId, 44), 1);
    EXPECT_EQ(s.itemCount(kHerbId), 1) << "始终是同一株，不该变出第二株来";
}

TEST_F(ScriptApiTest, TheOldGiveFortyFourWouldHaveLeftTheBottleFull) {
    // 负向对照：复现旧脚本那一行（give(黄精, 1, 44)，不催熟），
    // 证明上一条测试抓得住「台词说浇了三滴、绿液一滴没少」这个形状。
    GameState& s = state();
    makeBottle(/*owned=*/true, /*matureKnown=*/true, /*drops=*/3);

    s.addItem(kHerbId, 1, 44);   // 这正是旧的 cuishu.lua 第二场那一句

    EXPECT_EQ(s.itemCountOfAge(kHerbId, 44), 1) << "药照样到手";
    EXPECT_EQ(s.bottle.drops, s.bottle.capacity)
        << "而绿液一滴没少——旧写法在面板上就是「绿液 3 / 3」，与旁白正好打架";
}

// ---------------------------------------------------------------------------
// RealmAdvance：提升境界（契约 docs/interfaces-p3-ch04.md 第 6 节）
//
// 在这条命令之前，第 3、4 两章的脚本没有任何一处能改境界，脚本 API 里只有
// 查询 `realm_at_least`，于是「这一章他修为涨了」这件事在游戏里无处发生
//（docs/ch04-review.md CRITICAL-1）。
// ---------------------------------------------------------------------------

using fanren::rules::Realm;

// 夹具脚本：读一个旗标当目标境界，发一条命令，把结果的每一位都记进旗标。
// 命令表的形状与 api.lua 的 emit() 一字不差，理由见 runInlineScript 的注释。
constexpr const char* kAdvanceScript = R"lua(
local before = __host.realm_value()
local result = coroutine.yield{ kind = "realm_advance", x = flag.get("test_target") } or {}
flag.set("adv_ok", result.ok and 1 or 0)
flag.set("adv_code_empty", (result.code == nil or result.code == "") and 1 or 0)
flag.set("adv_code_no_realm", result.code == "no_realm" and 1 or 0)
flag.set("adv_code_not_higher", result.code == "not_higher" and 1 or 0)
flag.set("realm_before", before)
flag.set("realm_after", __host.realm_value())
)lua";

// 把主角摆成某个境界的满血满蓝状态。走 realmMaxHp/realmMaxMp 而不是写死数字：
// 这几条用例问的是「突破之后上限有没有跟着长」，不是「上限是几」——后者由
// tests/RealmTests.cpp 钉着，两处各钉一遍只会在改曲线时红两次。
void seatAt(GameState& s, Realm realm) {
    s.realm = realm;
    s.hp = s.maxHp = fanren::rules::realmMaxHp(realm);
    s.mp = s.maxMp = fanren::rules::realmMaxMp(realm);
}

TEST_F(ScriptApiTest, RealmAdvanceRaisesTheRealmAndTheCapsThatComeWithIt) {
    GameState& s = state();
    seatAt(s, Realm::QiRefining3);   // 第 3 章章末真正交过来的那一个
    s.setFlag("test_target", fanren::rules::toValue(Realm::QiRefining7));

    // 先验：这两档的上限**真的不一样**。相等的话下面那几条 EXPECT_EQ 会在
    // 「一个字也没改」的实现上照样全绿 —— 那正是这条命令要修的毛病。
    ASSERT_GT(fanren::rules::realmMaxHp(Realm::QiRefining7),
              fanren::rules::realmMaxHp(Realm::QiRefining3));
    ASSERT_GT(fanren::rules::realmMaxMp(Realm::QiRefining7),
              fanren::rules::realmMaxMp(Realm::QiRefining3));

    runInlineScript("realm_advance.lua", kAdvanceScript);

    EXPECT_EQ(s.flag("adv_ok"), 1);
    EXPECT_EQ(s.flag("adv_code_empty"), 1) << "成功时不该带原因码";
    EXPECT_EQ(s.realm, Realm::QiRefining7);
    EXPECT_EQ(s.maxHp, fanren::rules::realmMaxHp(Realm::QiRefining7)) << "上限没跟着长";
    EXPECT_EQ(s.maxMp, fanren::rules::realmMaxMp(Realm::QiRefining7)) << "法力上限没跟着长";
    EXPECT_EQ(s.hp, s.maxHp) << "满血突破之后还该是满的";
    EXPECT_EQ(s.mp, s.maxMp);
}

TEST_F(ScriptApiTest, TheScriptSeesTheNewRealmNotJustWhetherItIsFarEnough) {
    // `realm_at_least` 只答是非。剧情要写「他现在是第几层」就得有个读数，
    // 而那个读数必须在同一条脚本里、命令回填之后立刻就是新的。
    GameState& s = state();
    seatAt(s, Realm::QiRefining3);
    s.setFlag("test_target", fanren::rules::toValue(Realm::QiRefining8));

    runInlineScript("realm_advance.lua", kAdvanceScript);

    EXPECT_EQ(s.flag("realm_before"), 3) << "炼气期的境界编号就是层数";
    EXPECT_EQ(s.flag("realm_after"), 8) << "命令回填之后，脚本再读该看到新的";
}

TEST_F(ScriptApiTest, RealmAdvanceCrossesTheGapBetweenQiRefiningAndFoundation) {
    // 编号刻意不连续（13 之后是 21）。比较的是编号大小，所以这一跳必须走得通，
    // 而且中间那段空号不能被当成「可以落脚的地方」。
    GameState& s = state();
    seatAt(s, Realm::QiRefining13);
    s.setFlag("test_target", fanren::rules::toValue(Realm::FoundationEarly));

    runInlineScript("realm_advance.lua", kAdvanceScript);

    EXPECT_EQ(s.flag("adv_ok"), 1);
    EXPECT_EQ(s.realm, Realm::FoundationEarly);
    EXPECT_EQ(s.flag("realm_after"), 21);
    EXPECT_EQ(s.maxHp, fanren::rules::realmMaxHp(Realm::FoundationEarly));
}

TEST_F(ScriptApiTest, RealmAdvanceRefusesANumberThatIsNotARealmAtAll) {
    // 负向：落在 enum 空段里的编号。静默收下的话，主角的境界会变成一个
    // isValid 判 false 的数，气血法力全退回凡人档，而现场看不出是哪句脚本干的。
    GameState& s = state();
    seatAt(s, Realm::QiRefining3);
    const int hpBefore = s.maxHp;
    const int mpBefore = s.maxMp;
    s.setFlag("test_target", 24);   // 13 与 21 之间的空号

    ASSERT_FALSE(fanren::rules::isValid(fanren::rules::fromValue(24))) << "先验：24 真的是空号";

    runInlineScript("realm_advance.lua", kAdvanceScript);

    EXPECT_EQ(s.flag("adv_ok"), 0);
    EXPECT_EQ(s.flag("adv_code_no_realm"), 1) << "回填成空串的话这一位会是 0";
    EXPECT_EQ(s.realm, Realm::QiRefining3) << "回绝了就一个字节都不该动";
    EXPECT_EQ(s.maxHp, hpBefore);
    EXPECT_EQ(s.maxMp, mpBefore);
}

TEST_F(ScriptApiTest, RealmAdvanceRefusesToWalkBackDown) {
    // 只许升不许降。跌落是第 9 章 rules::Realm::demote 的事：那一章要决定
    // 掉境界跟不跟着削气血，而这条命令什么也决定不了，只会留下一个上限远高于
    // 境界基准的主角。
    GameState& s = state();
    seatAt(s, Realm::QiRefining7);
    const int hpBefore = s.maxHp;
    s.setFlag("test_target", fanren::rules::toValue(Realm::QiRefining3));

    runInlineScript("realm_advance.lua", kAdvanceScript);

    EXPECT_EQ(s.flag("adv_ok"), 0) << "降级要如实回绝，不能跟着 PartyAdd 判 true";
    EXPECT_EQ(s.flag("adv_code_not_higher"), 1);
    EXPECT_EQ(s.realm, Realm::QiRefining7) << "编号不许往回拨";
    EXPECT_EQ(s.maxHp, hpBefore) << "更不许把上限削下去";
}

TEST_F(ScriptApiTest, RealmAdvanceIsIdempotentAtTheRealmHeAlreadyHolds) {
    // 幂等口径与 PartyAdd 逐条对齐：「让他到炼气七层」这件事的结果已经成立，
    // 所以 ok 保持 true。剧情回放、读档重跑同一段都不该走进失败分支。
    //
    // 与上一条合起来才说得完整：**相等判 true、更低判 false**。
    // ok 回答的是「这条命令要的那个结果成不成立」，不是「有没有改动过东西」。
    GameState& s = state();
    seatAt(s, Realm::QiRefining7);
    s.hp = s.maxHp - 5;   // 带点伤，顺便验幂等那一路不会顺手治好他
    const int hpBefore = s.hp;
    s.setFlag("test_target", fanren::rules::toValue(Realm::QiRefining7));

    runInlineScript("realm_advance.lua", kAdvanceScript);

    EXPECT_EQ(s.flag("adv_ok"), 1) << "已经在那个境界上了，结果已经成立";
    EXPECT_EQ(s.flag("adv_code_empty"), 1) << "幂等不是失败，不该带原因码";
    EXPECT_EQ(s.realm, Realm::QiRefining7);
    EXPECT_EQ(s.hp, hpBefore);
    EXPECT_EQ(s.maxHp, fanren::rules::realmMaxHp(Realm::QiRefining7));
}

TEST_F(ScriptApiTest, RealmAdvanceCarriesTheWoundAcrossTheBreakthrough) {
    // 当前值按**差额上抬**（rules::liftToFloor），不补满。与打坐面板突破
    // 同一条口径（CultivationScene::applyRealmAttributes）：同一件事经脚本发生
    // 与经面板发生必须是同一个结果。补满会让这条命令顺带成为一次免费全恢复，
    // 而 BattleScene::finish 会把气血写回存档，这个差别玩家看得见。
    GameState& s = state();
    seatAt(s, Realm::QiRefining3);
    constexpr int kWound = 20;
    s.hp = s.maxHp - kWound;
    s.mp = s.maxMp - 7;
    const int hpCapBefore = s.maxHp;
    const int mpCapBefore = s.maxMp;
    s.setFlag("test_target", fanren::rules::toValue(Realm::QiRefining7));

    runInlineScript("realm_advance.lua", kAdvanceScript);

    ASSERT_EQ(s.flag("adv_ok"), 1);
    const int hpGain = s.maxHp - hpCapBefore;
    const int mpGain = s.maxMp - mpCapBefore;
    ASSERT_GT(hpGain, 0) << "先验：上限真的长了，否则下面的差额判据恒真";
    ASSERT_GT(mpGain, 0) << "先验：法力上限真的长了";
    EXPECT_EQ(s.hp, hpCapBefore - kWound + hpGain)
        << "补上去的那一截是根基不是疗伤：那 20 点伤该原样带过去";
    EXPECT_EQ(s.mp, mpCapBefore - 7 + mpGain);
    EXPECT_LT(s.hp, s.maxHp) << "补满的话这一条会红 —— 突破不是疗伤";
}

// ---------------------------------------------------------------------------
// G-10 的第三段：曲线到底有没有走到战场上
//
// docs/README.md「一个模块可用需要三样齐备」：规则层算得出攻防（RealmTests 钉着）、
// 脚本改得动境界（上面几条钉着），而战斗里那位韩立仍旧可以是写死的 6/3 ——
// 那正是这条债原本的样子。
//
// **这不是假想的**：把 BattleScene.cpp 里那两行改回字面量 6/3，全套 802 条测试
// 一条也不会红（本批实测，变异 C）。所以下面这两条必须存在。
//
// 它们长在这个文件里只是因为本批的文件白名单：战斗断言的正经去处是
// tests/Ch03TutorialBattleTests.cpp 那一类，而那几个文件归别人。
// ---------------------------------------------------------------------------

// data/battles 里第 3 章谷外那场教学战（普通地形）与识海那场（terrain = mind）。
constexpr const char* kPlainBattleId = "b03_gu_wai_elang";
constexpr const char* kMindBattleId = "b03_shihai_duoshe";

TEST_F(ScriptApiTest, TheRealmCurveActuallyReachesTheBattlefield) {
    GameState& s = state();
    seatAt(s, Realm::QiRefining8);

    fanren::game::BattleScene scene(kPlainBattleId);
    scene.onEnter(app_);

    ASSERT_FALSE(scene.battle().units().empty()) << "先验：这一场真的建起来了";
    const fanren::core::battle::Unit& hero = scene.battle().units()[0];
    ASSERT_EQ(hero.id, "hanli") << "先验：0 号位就是主角";

    // 先验：这一档与写死的 6/3 **真的不一样**。相同的话下面两条在旧实现上
    // 照样绿 —— 那是「判据自己会说谎」里最常见的一种长相。
    ASSERT_NE(fanren::rules::realmAttack(Realm::QiRefining8), 6);
    ASSERT_NE(fanren::rules::realmDefence(Realm::QiRefining8), 3);

    EXPECT_EQ(hero.attack, fanren::rules::realmAttack(Realm::QiRefining8))
        << "战场上的攻还是写死的，境界白涨了";
    EXPECT_EQ(hero.defence, fanren::rules::realmDefence(Realm::QiRefining8))
        << "战场上的防还是写死的，境界白涨了";
}

TEST_F(ScriptApiTest, TheMindBattleKeepsItsOwnVolumeButNotItsOwnAttack) {
    // 识海那一场的两条口径刻意不同，这里把分界钉住：
    //   * **体积仍然特判**（kDevourPlayerVolume）—— 它真的被 devourBite 读，
    //     而原著那三条体积关系是剧情硬约束，不能跟着存档漂。
    //   * **攻防不特判** —— 吞噬模式下 applyAttack 压根不读它们
    //     （core/battle/BattleAction.cpp 的 `devour_ ? devourBite(...) : ...`）。
    //     再写一条 `mind ? 6 : ...` 只会造出一段永远不被读、却长得像规则的死代码。
    GameState& s = state();
    seatAt(s, Realm::QiRefining8);

    fanren::game::BattleScene scene(kMindBattleId);
    scene.onEnter(app_);

    ASSERT_FALSE(scene.battle().units().empty());
    const fanren::core::battle::Unit& hero = scene.battle().units()[0];
    ASSERT_EQ(hero.id, "hanli");

    EXPECT_EQ(hero.maxHp, fanren::core::battle::kDevourPlayerVolume)
        << "先验：这确实是识海那一场，体积那一条特判还在";
    EXPECT_NE(hero.maxHp, s.maxHp) << "先验：体积与肉身气血真的不是一个数";
    EXPECT_EQ(hero.attack, fanren::rules::realmAttack(Realm::QiRefining8));
    EXPECT_EQ(hero.defence, fanren::rules::realmDefence(Realm::QiRefining8));
}

}  // namespace
