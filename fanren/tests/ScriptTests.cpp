#include "script/ScriptHost.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <system_error>
#include <vector>

#include "TempDir.h"
#include "core/rules/Bottle.h"
#include "core/rules/Field.h"
#include "core/rules/Realm.h"

namespace {

namespace fs = std::filesystem;

using fanren::core::GameState;
using fanren::script::Command;
using fanren::script::CommandKind;
using fanren::script::CommandResult;
using fanren::script::ScriptHost;

// ctest 的工作目录是 build/，从这里往上找带 scripts/common/api.lua 的目录。
// 不用 CMake 注入的路径宏，是因为 CMakeLists.txt 由四个模块共管，
// 测试不该为了自己方便去动它。
fs::path findProjectRoot() {
    fs::path dir = fs::current_path();
    for (int depth = 0; depth < 8; ++depth) {
        if (fs::exists(dir / "scripts" / "common" / "api.lua")) {
            return dir;
        }
        const fs::path parent = dir.parent_path();
        if (parent.empty() || parent == dir) {
            break;
        }
        dir = parent;
    }
    return {};
}

// 文案 key 必须是纯 ASCII。脚本里一旦冒出中文字面量，翻译与润色就得改脚本。
bool isAsciiKey(const std::string& text) {
    for (const char ch : text) {
        if (static_cast<unsigned char>(ch) >= 0x80) {
            return false;
        }
    }
    return true;
}

class ScriptHostTest : public ::testing::Test {
protected:
    void SetUp() override {
        const fs::path root = findProjectRoot();
        ASSERT_FALSE(root.empty()) << "找不到工程根目录（应含 scripts/common/api.lua）";

        // 每个用例一个独立的临时脚本根。夹具脚本要和 api.lua 同根才能被加载，
        // 而正式的 scripts/ 目录不该被测试写脏，故在临时目录里拼一份出来。
        //
        // 光靠用例名不够：用例名在进程之间是一样的，两个测试进程同时跑到同一条
        // 用例，就会互相 remove_all 掉对方刚铺好的脚本根。名字里还得有 pid，
        // 口径见 tests/TempDir.h。
        const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
        temp_ = fanren::test::uniqueTempPath("fanren_script_" + std::string(info->test_suite_name()) +
                                             "_" + info->name());
        std::error_code ec;
        fs::remove_all(temp_, ec);
        fs::create_directories(temp_ / "common", ec);
        fs::create_directories(temp_ / "ch01", ec);
        fs::create_directories(temp_ / "t", ec);
        ASSERT_TRUE(fs::is_directory(temp_ / "t")) << "建不出临时脚本根: " << temp_.string();

        // 测试跑的是真的 api.lua 与真的章节脚本，不是复刻品。
        copyInto(root / "scripts" / "common" / "api.lua", temp_ / "common" / "api.lua");
        copyInto(root / "scripts" / "ch01" / "sanshu.lua", temp_ / "ch01" / "sanshu.lua");
        for (const auto& entry : fs::directory_iterator(root / "tests" / "scripts")) {
            if (entry.path().extension() == ".lua") {
                copyInto(entry.path(), temp_ / "t" / entry.path().filename());
            }
        }

        host_.setLogSink([this](const std::string& line) { logs_.push_back(line); });
        const auto ready = host_.init(temp_.string(), &state_);
        ASSERT_TRUE(ready.ok) << ready.error;
    }

    void TearDown() override {
        std::error_code ec;
        fs::remove_all(temp_, ec);
    }

    void copyInto(const fs::path& from, const fs::path& to) {
        std::error_code ec;
        fs::copy_file(from, to, fs::copy_options::overwrite_existing, ec);
        ASSERT_FALSE(ec) << "复制 " << from.string() << " 失败: " << ec.message();
    }

    // 模拟 game 层：有状态副作用的命令落到 GameState，其余只是演出。
    void applyToState(const Command& command) {
        switch (command.kind) {
            case CommandKind::SetFlag:
                state_.setFlag(command.a, command.x);
                break;
            case CommandKind::GiveItem:
                state_.addItem(command.a, command.x, command.y);
                break;
            case CommandKind::TakeItem:
                static_cast<void>(state_.removeItem(command.a, command.x));
                break;
            default:
                break;
        }
    }

    // 像主循环那样把协程驱动到结束，返回经过的全部命令。
    // maxSteps 是死循环闸：脚本写错时宁可测试失败，也不要挂住整个 ctest。
    std::vector<Command> drive(const CommandResult& answer, int maxSteps = 64) {
        std::vector<Command> seen;
        Command command;
        for (int step = 0; step < maxSteps && host_.pollCommand(command); ++step) {
            seen.push_back(command);
            applyToState(command);
            host_.resumeWith(answer);
        }
        return seen;
    }

    fs::path temp_;
    GameState state_;
    std::vector<std::string> logs_;
    // host_ 最后声明：析构最早，日志沉到 logs_ 时那个 vector 还活着。
    ScriptHost host_;
};

// --- 1. 协程 yield / resume 基本往返 -----------------------------------------

TEST_F(ScriptHostTest, YieldsAndResumesOneCommandAtATime) {
    const auto started = host_.startEvent("t/roundtrip.lua");
    ASSERT_TRUE(started.ok) << started.error;
    EXPECT_TRUE(host_.isRunning());

    Command command;
    ASSERT_TRUE(host_.pollCommand(command));
    EXPECT_EQ(command.kind, CommandKind::Talk);
    EXPECT_EQ(command.a, "t.greet");   // a 是主参数：文案 key
    EXPECT_EQ(command.b, "sanshu");    // b 是次参数：说话人

    host_.resumeWith(CommandResult{});
    ASSERT_TRUE(host_.pollCommand(command));
    EXPECT_EQ(command.kind, CommandKind::Talk);
    EXPECT_EQ(command.a, "t.reply");
    EXPECT_EQ(command.b, "hanli");

    host_.resumeWith(CommandResult{});
    ASSERT_TRUE(host_.pollCommand(command));
    EXPECT_EQ(command.kind, CommandKind::SetFlag);
    EXPECT_EQ(command.a, "roundtrip_done");
    EXPECT_EQ(command.x, 1);

    host_.resumeWith(CommandResult{});
    EXPECT_FALSE(host_.pollCommand(command));
    EXPECT_FALSE(host_.isRunning());
}

TEST_F(ScriptHostTest, PollCommandPeeksInsteadOfConsuming) {
    ASSERT_TRUE(host_.startEvent("t/roundtrip.lua").ok);

    Command first;
    Command again;
    ASSERT_TRUE(host_.pollCommand(first));
    ASSERT_TRUE(host_.pollCommand(again));
    // 同一条命令每帧都能取到，game 层不必自己缓存就能把它慢慢演完。
    EXPECT_EQ(first.a, again.a);

    host_.resumeWith(CommandResult{});
    Command next;
    ASSERT_TRUE(host_.pollCommand(next));
    EXPECT_NE(next.a, first.a);
}

// --- 2. choice 分支 -----------------------------------------------------------

TEST_F(ScriptHostTest, ChoiceIndexSelectsTheBranch) {
    struct Case {
        int choiceIndex;
        const char* expectedFlag;
    };
    // -1 是取消：api.lua 把它翻成 nil，脚本走 else。
    const Case cases[] = {{0, "picked_first"}, {1, "picked_second"}, {-1, "cancelled"}};

    for (const Case& scenario : cases) {
        state_.flags.clear();
        const auto started = host_.startEvent("t/branch.lua");
        ASSERT_TRUE(started.ok) << started.error;

        CommandResult answer;
        answer.choiceIndex = scenario.choiceIndex;
        const std::vector<Command> seen = drive(answer);

        ASSERT_EQ(seen.size(), 2u) << "期望 choice + set_flag 两条命令";
        EXPECT_EQ(seen[0].kind, CommandKind::Choice);
        ASSERT_EQ(seen[0].options.size(), 2u);
        EXPECT_EQ(seen[0].options[0], "t.opt_a");
        EXPECT_EQ(seen[0].options[1], "t.opt_b");

        EXPECT_EQ(seen[1].kind, CommandKind::SetFlag);
        EXPECT_EQ(seen[1].a, scenario.expectedFlag);
        EXPECT_EQ(state_.flag(scenario.expectedFlag), 1);
        EXPECT_FALSE(host_.isRunning());
    }
}

// --- 3. canSave 语义 ---------------------------------------------------------

TEST_F(ScriptHostTest, ForbidsSavingWhileACoroutineIsSuspended) {
    EXPECT_TRUE(host_.canSave());   // 无活动事件

    const auto started = host_.startEvent("t/roundtrip.lua");
    ASSERT_TRUE(started.ok) << started.error;
    EXPECT_FALSE(host_.canSave()) << "方案 3.5 规则 4：协程挂起期间禁止存档";

    drive(CommandResult{});
    EXPECT_FALSE(host_.isRunning());
    EXPECT_TRUE(host_.canSave());
}

// --- 4. 语法错误 -------------------------------------------------------------

TEST_F(ScriptHostTest, ReportsSyntaxErrorsWithScriptNameAndLine) {
    const auto started = host_.startEvent("t/syntax_error.lua");
    EXPECT_FALSE(started.ok);
    ASSERT_FALSE(started.error.empty());
    // chunkname 用的是相对路径，报错前缀因此是 "t/syntax_error.lua:<行>:"。
    EXPECT_NE(started.error.find("t/syntax_error.lua:"), std::string::npos) << started.error;

    EXPECT_FALSE(host_.isRunning());
    EXPECT_TRUE(host_.canSave());
    Command command;
    EXPECT_FALSE(host_.pollCommand(command));
}

// --- 5. 运行时错误 -----------------------------------------------------------

TEST_F(ScriptHostTest, ContainsRuntimeErrorsAndMarksTheCoroutineFinished) {
    const auto started = host_.startEvent("t/runtime_error.lua");
    ASSERT_TRUE(started.ok) << started.error;

    Command command;
    ASSERT_TRUE(host_.pollCommand(command));
    EXPECT_EQ(command.kind, CommandKind::Talk);

    // 这一次 resume 会踩到「调用 nil」。进程必须活着，协程必须被判结束。
    host_.resumeWith(CommandResult{});
    EXPECT_FALSE(host_.isRunning());
    EXPECT_TRUE(host_.canSave());
    EXPECT_FALSE(host_.pollCommand(command));

    ASSERT_FALSE(host_.lastError().empty());
    EXPECT_NE(host_.lastError().find("t/runtime_error.lua:3"), std::string::npos)
        << host_.lastError();
}

// --- 6. 脚本文件不存在 -------------------------------------------------------

TEST_F(ScriptHostTest, ReportsMissingScriptFile) {
    const auto started = host_.startEvent("t/no_such_event.lua");
    EXPECT_FALSE(started.ok);
    EXPECT_NE(started.error.find("no_such_event.lua"), std::string::npos) << started.error;
    EXPECT_FALSE(host_.isRunning());
    EXPECT_TRUE(host_.canSave());
}

// --- 7. battle 命令往返 ------------------------------------------------------

TEST_F(ScriptHostTest, BattleOutcomeSelectsTheBranch) {
    for (const bool won : {true, false}) {
        state_.flags.clear();
        const auto started = host_.startEvent("t/battle.lua");
        ASSERT_TRUE(started.ok) << started.error;

        Command command;
        ASSERT_TRUE(host_.pollCommand(command));
        ASSERT_EQ(command.kind, CommandKind::Battle);
        EXPECT_EQ(command.a, "t.wild_dog");

        CommandResult answer;
        answer.battleWon = won;
        const std::vector<Command> seen = drive(answer);

        ASSERT_EQ(seen.size(), 3u) << "期望 battle + set_flag + talk";
        EXPECT_EQ(seen[1].a, won ? "battle_won" : "battle_lost");
        EXPECT_EQ(seen[2].a, won ? "t.victory" : "t.defeat");
        EXPECT_FALSE(host_.isRunning());
    }
}

// --- 8. 跨 C 边界的回归测试 --------------------------------------------------

TEST_F(ScriptHostTest, ReadOnlyHostQueriesReturnWithoutSuspending) {
    // 正面：flag.get / item.count / realm.at_least 都是 C++ 注册的函数。
    // 它们同步返回，挂起点前后不留 C 帧，所以查询可以任意穿插在 yield 之间。
    state_.setFlag("probe_flag", 3);
    state_.addItem("probe_item", 5);
    state_.realm = fanren::rules::Realm::QiRefining5;

    const auto started = host_.startEvent("t/host_query.lua");
    ASSERT_TRUE(started.ok) << started.error;

    const std::vector<Command> seen = drive(CommandResult{});
    ASSERT_EQ(seen.size(), 2u);
    EXPECT_EQ(seen[0].a, "q.3.5.true");
    EXPECT_EQ(seen[1].a, "again.3");
    EXPECT_FALSE(host_.isRunning());
}

TEST_F(ScriptHostTest, YieldUnderACFrameFailsLoudlyInsteadOfCrashing) {
    // 反面：脚本把 talk() 塞进 table.sort 的比较器里，挂起点上就压了一层 C 帧。
    // 这正是设计约束 2 要绕开的形状 —— 事件 API 全在 Lua 侧实现，正常调用路径
    // 永远到不了这里。真到了，ScriptHost 也必须把错误收住而不是让进程死。
    const auto started = host_.startEvent("t/yield_across_c.lua");
    EXPECT_FALSE(started.ok);
    ASSERT_FALSE(started.error.empty());
    EXPECT_NE(started.error.find("C-call boundary"), std::string::npos) << started.error;

    EXPECT_FALSE(host_.isRunning());
    EXPECT_TRUE(host_.canSave());

    // 进程还活着，宿主还能接着跑下一个事件。
    const auto next = host_.startEvent("t/roundtrip.lua");
    EXPECT_TRUE(next.ok) << next.error;
    EXPECT_TRUE(host_.isRunning());
}

// --- 9. P3 第 2 章增补：四条新命令与六个新查询 --------------------------------

TEST_F(ScriptHostTest, ParsesTheChapterTwoCommandKinds) {
    const auto started = host_.startEvent("t/ch02_commands.lua");
    ASSERT_TRUE(started.ok) << started.error;

    const std::vector<Command> seen = drive(CommandResult{});
    ASSERT_EQ(seen.size(), 5u);

    EXPECT_EQ(seen[0].kind, CommandKind::AdvanceDays);
    EXPECT_EQ(seen[0].x, 30) << "天数走 x 槽位";

    EXPECT_EQ(seen[1].kind, CommandKind::BottleGrant);
    EXPECT_EQ(seen[2].kind, CommandKind::BottleUnlockMature);

    EXPECT_EQ(seen[3].kind, CommandKind::FieldUnlock);
    EXPECT_EQ(seen[3].a, "shenshougu_yaopu");   // 契约第 4 节约定的灵田 id
    EXPECT_EQ(seen[3].x, 8);

    EXPECT_EQ(seen[4].kind, CommandKind::FieldUnlock);
    EXPECT_EQ(seen[4].a, "mo_yuan");
    EXPECT_EQ(seen[4].x, 4) << "省略槽位数时 api.lua 该补上默认的 4";

    EXPECT_FALSE(host_.isRunning());
}

TEST_F(ScriptHostTest, RejectsUnknownCommandKindsInsteadOfSkippingThem) {
    // 负向：脚本里塞一个解析表里没有的 kind。静默跳过等于让写错的闸门永远通过，
    // 所以这里必须失败，而且脚本不能继续往下跑。
    const auto started = host_.startEvent("t/unknown_kind.lua");
    EXPECT_FALSE(started.ok);
    EXPECT_FALSE(started.error.empty());
    EXPECT_NE(started.error.find("no_such_command"), std::string::npos)
        << "错误里要点名是哪个 kind 拼错了: " << started.error;

    EXPECT_FALSE(host_.isRunning());
    Command command;
    EXPECT_FALSE(host_.pollCommand(command));

    // 探针旗标：先把协程推到底再看。直接断言是没用的——「未知即忽略」的实现
    // 会把脚本挂在那条被跳过的命令上，探针反而沉默；推完它才会现形。
    drive(CommandResult{});
    EXPECT_EQ(state_.flag("unknown_kind_slipped_through"), 0)
        << "未知命令被跳过了，脚本继续往下执行——这正是本条要拦住的形状";
}

TEST_F(ScriptHostTest, ChapterTwoHostQueriesReadTheGameState) {
    state_.day = 42;
    state_.bottle.owned = true;
    state_.bottle.matureKnown = true;
    state_.bottle.drops = 2;

    fanren::rules::SpiritField field;
    field.id = "shenshougu_yaopu";
    field.slots.resize(3);
    ASSERT_TRUE(fanren::rules::plant(field, 0, "herb_qingfeng_cao", state_.day));
    ASSERT_TRUE(fanren::rules::plant(field, 1, "herb_qingfeng_cao", state_.day));
    field.slots[1].ripe = true;
    state_.fields.push_back(std::move(field));

    const auto started = host_.startEvent("t/ch02_query.lua");
    ASSERT_TRUE(started.ok) << started.error;

    const std::vector<Command> seen = drive(CommandResult{});
    ASSERT_EQ(seen.size(), 5u);
    EXPECT_EQ(seen[0].a, "d.42");
    EXPECT_EQ(seen[1].a, "b.true.true.2");
    EXPECT_EQ(seen[2].a, "one.2.1");
    EXPECT_EQ(seen[3].a, "all.2.1");
    EXPECT_EQ(seen[4].a, "none.0.0") << "不存在的田返回 0，不是报错";
    for (const Command& command : seen) {
        EXPECT_TRUE(isAsciiKey(command.a)) << command.a;
    }
}

TEST_F(ScriptHostTest, FieldQueriesWithAnEmptyIdSumEveryField) {
    // 契约第 5.6 条：两块田各种一株，合计口径要返回 2 而不是第一块的 1。
    for (const char* id : {"shenshougu_yaopu", "mo_yuan"}) {
        fanren::rules::SpiritField field;
        field.id = id;
        field.slots.resize(2);
        ASSERT_TRUE(fanren::rules::plant(field, 0, "herb_qingfeng_cao", 1));
        state_.fields.push_back(std::move(field));
    }
    state_.fields[1].slots[0].ripe = true;

    ASSERT_TRUE(host_.startEvent("t/ch02_query.lua").ok);
    const std::vector<Command> seen = drive(CommandResult{});
    ASSERT_EQ(seen.size(), 5u);
    EXPECT_EQ(seen[2].a, "one.1.0") << "指名一块田时只算那一块";
    EXPECT_EQ(seen[3].a, "all.2.1") << "空 id 是合计，不是第一块";
}

// --- 其余不变量 ---------------------------------------------------------------

TEST_F(ScriptHostTest, WritesGoThroughTheQueueAndLaterReadsSeeThem) {
    const auto started = host_.startEvent("t/flag_rw.lua");
    ASSERT_TRUE(started.ok) << started.error;

    const std::vector<Command> seen = drive(CommandResult{});
    ASSERT_EQ(seen.size(), 3u);
    EXPECT_EQ(seen[0].a, "before.0");
    EXPECT_EQ(seen[1].kind, CommandKind::SetFlag);
    EXPECT_EQ(seen[1].a, "story_bit");
    EXPECT_EQ(seen[1].x, 7);
    // drive 先把 SetFlag 落到 GameState 再 resume，所以脚本随后读得到新值。
    EXPECT_EQ(seen[2].a, "after.7");
    EXPECT_EQ(state_.flag("story_bit"), 7);
}

TEST_F(ScriptHostTest, RejectsYieldsThatAreNotCommandTables) {
    const auto started = host_.startEvent("t/bad_yield.lua");
    EXPECT_FALSE(started.ok);
    EXPECT_FALSE(started.error.empty());
    EXPECT_FALSE(host_.isRunning());
}

TEST_F(ScriptHostTest, FinishesScriptsThatNeverSuspend) {
    const auto started = host_.startEvent("t/no_yield.lua");
    EXPECT_TRUE(started.ok) << started.error;
    EXPECT_FALSE(host_.isRunning());
    EXPECT_TRUE(host_.canSave());
    Command command;
    EXPECT_FALSE(host_.pollCommand(command));
}

TEST_F(ScriptHostTest, DeniesScriptsAccessToTheFilesystem) {
    // io / os / package 不开，dofile / loadfile 摘掉：脚本没有读写磁盘的正当理由。
    const auto started = host_.startEvent("t/sandbox.lua");
    ASSERT_TRUE(started.ok) << started.error;
    Command command;
    ASSERT_TRUE(host_.pollCommand(command));
    EXPECT_EQ(command.a, "t.sandbox_ok");
}

TEST_F(ScriptHostTest, ReportsRuntimeErrorsRaisedBeforeTheFirstSuspend) {
    // 第一次 resume 就炸：失败必须从 startEvent 的返回值出来，而不是等到
    // resumeWith（那时候已经没有返回值通道了）。
    const auto started = host_.startEvent("t/early_error.lua");
    EXPECT_FALSE(started.ok);
    EXPECT_NE(started.error.find("t/early_error.lua:3"), std::string::npos) << started.error;
    EXPECT_FALSE(host_.isRunning());
    Command command;
    EXPECT_FALSE(host_.pollCommand(command));
}

TEST_F(ScriptHostTest, RejectsABareYieldWithNoCommandTable) {
    const auto started = host_.startEvent("t/empty_yield.lua");
    EXPECT_FALSE(started.ok);
    EXPECT_FALSE(started.error.empty());
    EXPECT_FALSE(host_.isRunning());
}

TEST_F(ScriptHostTest, RejectsScriptPathsThatEscapeTheScriptRoot) {
    for (const char* path : {"../secrets.lua", "t/../../secrets.lua", "/etc/passwd",
                             "C:/Windows/win.ini", ""}) {
        const auto started = host_.startEvent(path);
        EXPECT_FALSE(started.ok) << path;
        EXPECT_FALSE(host_.isRunning());
    }
}

TEST_F(ScriptHostTest, TreatsAnOutOfRangeChoiceIndexAsCancel) {
    const auto started = host_.startEvent("t/branch.lua");
    ASSERT_TRUE(started.ok) << started.error;

    CommandResult answer;
    answer.choiceIndex = 7;   // 脚本只给了两个选项
    const std::vector<Command> seen = drive(answer);

    ASSERT_EQ(seen.size(), 2u);
    EXPECT_EQ(seen[1].a, "cancelled");
    EXPECT_FALSE(logs_.empty());
}

TEST_F(ScriptHostTest, IgnoresResumeWithWhenNothingIsPending) {
    // 没有事件时
    logs_.clear();
    host_.resumeWith(CommandResult{});
    EXPECT_FALSE(host_.isRunning());
    EXPECT_FALSE(logs_.empty());

    // 事件跑完之后
    ASSERT_TRUE(host_.startEvent("t/roundtrip.lua").ok);
    drive(CommandResult{});
    ASSERT_FALSE(host_.isRunning());
    logs_.clear();
    host_.resumeWith(CommandResult{});
    EXPECT_FALSE(host_.isRunning());
    EXPECT_FALSE(logs_.empty());
}

TEST_F(ScriptHostTest, LogsCoroutinesDiscardedBeforeTheyFinish) {
    // 方案 3.5 规则 6：场景弹出时协程没跑完，必须留下痕迹。
    ASSERT_TRUE(host_.startEvent("t/roundtrip.lua").ok);
    ASSERT_TRUE(host_.isRunning());

    logs_.clear();
    host_.abortEvent("场景被弹出");
    EXPECT_FALSE(host_.isRunning());
    ASSERT_EQ(logs_.size(), 1u);
    EXPECT_NE(logs_[0].find("t/roundtrip.lua"), std::string::npos) << logs_[0];

    // 上一个协程没结束就起新事件，同样是丢弃，同样要留痕。
    ASSERT_TRUE(host_.startEvent("t/roundtrip.lua").ok);
    logs_.clear();
    ASSERT_TRUE(host_.startEvent("t/branch.lua").ok);
    ASSERT_FALSE(logs_.empty());
    EXPECT_NE(logs_[0].find("未正常结束"), std::string::npos) << logs_[0];

    // 重新 init 会连 lua_State 一起换掉，在跑的协程同样是被丢弃。
    logs_.clear();
    const auto again = host_.init(temp_.string(), &state_);
    ASSERT_TRUE(again.ok) << again.error;
    EXPECT_FALSE(host_.isRunning());
    ASSERT_FALSE(logs_.empty());
    EXPECT_NE(logs_[0].find("未正常结束"), std::string::npos) << logs_[0];
    // 换过 lua_State 之后照样能继续用。
    EXPECT_TRUE(host_.startEvent("t/roundtrip.lua").ok);
}

TEST_F(ScriptHostTest, LogsWhenDestroyedWithACoroutineStillSuspended) {
    // sink 写进的 vector 必须比 ScriptHost 活得久，故先声明。
    std::vector<std::string> sink;
    {
        ScriptHost host;
        host.setLogSink([&sink](const std::string& line) { sink.push_back(line); });
        ASSERT_TRUE(host.init(temp_.string(), &state_).ok);
        ASSERT_TRUE(host.startEvent("t/roundtrip.lua").ok);
        ASSERT_TRUE(host.isRunning());
        sink.clear();
    }
    ASSERT_FALSE(sink.empty());
    EXPECT_NE(sink[0].find("t/roundtrip.lua"), std::string::npos) << sink[0];
}

// --- 第一章「三叔引荐」示例脚本 -----------------------------------------------

TEST_F(ScriptHostTest, RunsARealChapterScriptToCompletion) {
    // 原先这条钉死了第 1 章开场脚本的命令序列（第几条是什么、共几条）。
    // 剧情一润色就红，而脚本宿主什么也没坏。
    //
    // 宿主要证明的是：能加载真实脚本、能把每条 yield 翻成合法命令、能被 resume
    // 推到结束、结束后允许存档。至于剧情写了几句话，那是内容的事。
    auto started = host_.startEvent("ch01/sanshu.lua");
    ASSERT_TRUE(started.ok) << started.error;

    CommandResult reply;
    reply.choiceIndex = 0;

    int guard = 0;
    bool sawTalk = false;
    bool sawChoice = false;
    bool sawSpeaker = false;
    Command command;
    while (host_.pollCommand(command)) {
        ASSERT_LT(++guard, 200) << "脚本没有收敛，可能是死循环";
        if (command.kind == CommandKind::Talk) {
            sawTalk = true;
            EXPECT_FALSE(command.a.empty()) << "对话必须带文案 key";
            // 说话人可以为空：那是旁白，对话框据此不画名字框。
            // 但整段脚本里总得有人开口，否则说话人这条链路等于没验。
            if (!command.b.empty()) sawSpeaker = true;
        }
        if (command.kind == CommandKind::Choice) {
            sawChoice = true;
            EXPECT_GE(command.options.size(), 2u) << "选择至少要有两个选项";
        }
        host_.resumeWith(reply);
    }

    EXPECT_TRUE(sawTalk) << "开场脚本总该说点什么";
    EXPECT_TRUE(sawSpeaker) << "整段脚本里应当至少有一个具名说话人";
    EXPECT_TRUE(sawChoice) << "设计要求第 1 章开场带一次二选一";
    EXPECT_FALSE(host_.isRunning());
    EXPECT_TRUE(host_.canSave()) << "协程结束后应当允许存档";
}

// 「脚本引的文案 key 是否存在」这条检查已经由内容门禁（tools/validate.py）对
// 全部脚本统一执行，且那边还一并查了旗标、物品、地图与说话人。这里再钉一份
// 只会随剧情改动重复变红，故删去，不再重复覆盖。
TEST(ScriptHostInit, RejectsANullGameState) {
    ScriptHost host;
    const auto result = host.init("scripts", nullptr);
    EXPECT_FALSE(result.ok);
    EXPECT_FALSE(result.error.empty());
}

TEST(ScriptHostInit, FailsWhenApiLuaIsMissing) {
    ScriptHost host;
    GameState state;
    const auto result = host.init("no/such/script/root", &state);
    EXPECT_FALSE(result.ok);
    EXPECT_NE(result.error.find("api.lua"), std::string::npos) << result.error;
}

TEST(ScriptHostInit, RefusesToStartEventsBeforeInit) {
    ScriptHost host;
    const auto result = host.startEvent("ch01/sanshu.lua");
    EXPECT_FALSE(result.ok);
    EXPECT_FALSE(host.isRunning());
    EXPECT_TRUE(host.canSave());
}

}  // namespace
