#pragma once

// 章与章之间的交接存档：上一章通关测试的终局，就是下一章通关测试的起点。
//
// ---------------------------------------------------------------------------
// 为什么要有这个文件（第 4 章复验判据 1）
// ---------------------------------------------------------------------------
// 第 4 章的通关测试从前自己手搭章首，与第 3 章真实的终局差着九个背包堆、日期、
// 掌天瓶与二十三个旗标，而一行注释也没有。复验把第 3 章的终局**真的读进来**，
// 通关测试当场红。「一条通关测试若可以自己挑起点，它就不再是通关测试」
//（docs/handoff.md 第 8 节）——手搭起点的毛病不在手搭，在于它可以悄悄漂开。
//
// 这里的做法是**两头各钉一半**：
//   · 第 3 章那一头（tests/Ch03SliceTests.cpp）：两条通关用例走到终点时，
//     把自己的终局与 tests/fixtures/ch03-end-*.sav **逐字段**比一遍，差一个字段就红；
//   · 第 4 章那一头（tests/Ch04SliceTests.cpp）：直接读同一份文件起步。
// 于是两章之间只剩一份存档，它的两边各有一条测试看着，谁也漂不开。
//
// 重新生成（第 3 章的剧情真的改了，终局本就该变的时候）：
//   set FANREN_WRITE_CH03_FIXTURES=1
//   build-<槽>\fanren_tests.exe --gtest_filter=Ch03Walkthrough.*
// 生成之后**先跑一遍不带这个变量的全套**：第 4 章从新的终局起步还走不走得通，
// 正是这份存档要问的问题。
//
// ---------------------------------------------------------------------------
// 「逐字段」的口径
// ---------------------------------------------------------------------------
// 比的是 io::saveGame 写出来的整份存档（SaveFile.cpp 把 GameState 全部可持久化字段
// 照单序列化，不挑着存），逐行比。**默认全比**：将来存档多了一个字段，这里不必改
// 就会一起比进来。只有三行不比，每一行都写明为什么：
//   · checksum —— 它是其余一切的摘要，任何一处不同它都不同，比它只会多一行噪声；
//   · playSecondsGameplay / playSecondsSystem —— 游戏时长计时器（方案 2.2 的十小时
//     计时），在无头测试里数的是驱动走位推了多少帧。下一章一处也不读它们，
//     而通关测试换一条 BFS 路线它们就变，比进来只会让交接存档因为走位细节而改。
//     比较前两边都置 0（存档文件里保留真值）。
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

#include "TempDir.h"
#include "core/model/Types.h"
#include "io/SaveFile.h"

namespace fanren::test {

// 第 3 章两条通关用例各自的终局。两条分支的背包、旗标都不一样（五毒水交没交、
// 毒配了几副），所以是两份，不是一份。
inline constexpr const char* kChapterThreeEndingFirst = "ch03-end-first.sav";
inline constexpr const char* kChapterThreeEndingSecond = "ch03-end-second.sav";

// 置了它（值非空）第 3 章那一头就改为**写**交接存档，而不是比对。
inline constexpr const char* kWriteChapterThreeFixturesEnv = "FANREN_WRITE_CH03_FIXTURES";

// ---- 第 4 章 → 第 5 章（契约 docs/interfaces-p3-ch05.md 第 3 节）----
//
// 两侧照 tests/Ch04SliceTests.cpp 两条通关用例的实际分支：
//   first  —— Ch04Walkthrough.WalksTheWholeChapterAndEveryGateHoldsThenOpens
//             （从 ch03-end-first 起步，每一处选择取第一项）；
//   second —— Ch04Walkthrough.TheOtherSideOfEveryChoiceAlsoReachesTheEnd
//             （从 ch03-end-second 起步，每一处取第二项；**节点 11 之前那条用例把养精丹
//             全部清掉**，为了走「翻遍箱底也没有」那一条台词——所以这一侧交给第 5 章的
//             终局是 0 瓶养精丹。那是第 4 章测试里的一处手摆，不是玩家的动作，契约 3.1 写明了）。
//
// 重新生成（第 4 章的剧情真的改了，终局本就该变的时候）：
//   set FANREN_WRITE_CH04_FIXTURES=1
//   build-<槽>\fanren_tests.exe --gtest_filter=Ch04Walkthrough.WalksTheWholeChapterAndEveryGateHoldsThenOpens:Ch04Walkthrough.TheOtherSideOfEveryChoiceAlsoReachesTheEnd
// 生成之后**先跑一遍不带这个变量的全套**。
inline constexpr const char* kChapterFourEndingFirst = "ch04-end-first.sav";
inline constexpr const char* kChapterFourEndingSecond = "ch04-end-second.sav";
inline constexpr const char* kWriteChapterFourFixturesEnv = "FANREN_WRITE_CH04_FIXTURES";

// ---- 第 5 章 → 第 6 章（测试路，docs/ch05-design.md 验收第 2 条）----
//
// 两侧照 tests/Ch05SliceTests.cpp 两条通关用例的实际走法：
//   first  —— Ch05Walkthrough.FirstSideWalksTheWholeChapterAndEveryGateHoldsThenOpens
//             （从 ch04-end-first 起步，每一处选择取第一项；清灵散留着、练了剑符、医书交了）；
//   second —— Ch05Walkthrough.SecondSideTakesEveryOtherChoiceWithNothingPrepared
//             （从 ch04-end-second 起步，每一处取第二项；0 瓶养精丹、不练剑符、不做医书——
//             「什么都不备」那一档；没练符那一夜只远远看清欧阳飞天、不照面，退回林子练成了符再来，⑩ 只打一次——第 5 章复验 MEDIUM-B 方案 1）。
// 两条都不在起点上手摆任何字段（第 4 章终局原样读入）。
//
// 重新生成（第 5 章的剧情真的改了，终局本就该变的时候）：
//   set FANREN_WRITE_CH05_FIXTURES=1
//   build-<槽>\fanren_tests.exe --gtest_filter=Ch05Walkthrough.FirstSideWalksTheWholeChapterAndEveryGateHoldsThenOpens:Ch05Walkthrough.SecondSideTakesEveryOtherChoiceWithNothingPrepared
// 生成之后**先跑一遍不带这个变量的全套**。
inline constexpr const char* kChapterFiveEndingFirst = "ch05-end-first.sav";
inline constexpr const char* kChapterFiveEndingSecond = "ch05-end-second.sav";
inline constexpr const char* kWriteChapterFiveFixturesEnv = "FANREN_WRITE_CH05_FIXTURES";

// ---- 第 6 章 → 第 7 章（测试路，docs/ch06-design.md 验收第 2 条）----
//
// 两侧照 tests/Ch06AcceptanceTests.cpp 两条通关用例的实际走法：
//   first  —— Ch06Walkthrough.FirstSideWalksTheWholeChapterAndEveryGateHoldsThenOpens
//             （从 ch05-end-first 起步，每一处二选一取第一项；认药一次认对、竹简直指百药园；
//             坊市卖一瓶清毒散、买一张定神符——支线 Z1 了结、Z2 过期）；
//   second —— Ch06Walkthrough.SecondSideTakesEveryOtherChoice
//             （从 ch05-end-second 起步，每一处取第二项；认药与竹简把四个选项挨个点过去；
//             坊市卖三瓶清毒散攒够十块——Z2 了结、Z1 过期）。
// 两条都不在起点上手摆任何字段（第 5 章终局原样读入）。
//
// 重新生成（第 6 章的剧情真的改了，终局本就该变的时候）：
//   set FANREN_WRITE_CH06_FIXTURES=1
//   build-<槽>\fanren_tests.exe --gtest_filter=Ch06Walkthrough.FirstSideWalksTheWholeChapterAndEveryGateHoldsThenOpens:Ch06Walkthrough.SecondSideTakesEveryOtherChoice
// 生成之后**先跑一遍不带这个变量的全套**。
inline constexpr const char* kChapterSixEndingFirst = "ch06-end-first.sav";
inline constexpr const char* kChapterSixEndingSecond = "ch06-end-second.sav";
inline constexpr const char* kWriteChapterSixFixturesEnv = "FANREN_WRITE_CH06_FIXTURES";

// ---- 第 7 章 → 第 8 章（测试路，docs/ch07-design.md 验收第 2 条）----
//
// 两侧照 tests/Ch07AcceptanceTests.cpp 两条通关用例的实际走法：
//   first  —— Ch07Walkthrough.FirstSideWalksTheWholeChapterAndEveryGateHoldsThenOpens
//             （从 ch06-end-first 起步，每一处二选一取第一项；支线 Z1、Z2 都做；坊市收药摊买两瓶清灵散；
//             ② 开场甩土牢符、③ 开场掷天雷子——施工图 8.2 的原著解法）；
//   second —— Ch07Walkthrough.SecondSideTakesEveryOtherChoice
//             （从 ch06-end-second 起步，每一处取第二项；Z1、Z2 都不做、坊市不买；一张符也不用）。
// 两条都不在起点上手摆任何字段（第 6 章终局原样读入）。药园那几年、地火屋开炉是那只手按 R-3 提示替玩家做的
//（文件头「药园经营」），天数是玩家推的，所以终局的日子随那只手的节奏走；剧情挂点拨的日子逐步判过。
//
// 重新生成（第 7 章的剧情真的改了、或第 6 章的交接存档重生成过，终局本就该变的时候）：
//   set FANREN_WRITE_CH07_FIXTURES=1
//   build-<槽>\fanren_tests.exe --gtest_filter=Ch07Walkthrough.FirstSideWalksTheWholeChapterAndEveryGateHoldsThenOpens:Ch07Walkthrough.SecondSideTakesEveryOtherChoice
// 生成之后**先跑一遍不带这个变量的全套**。第 6 章的交接存档一变（主树整改合回时会变：日子、ch06.qianyao、站位），
// 这两份就要跟着重生成——它们是从那两份起步的。
inline constexpr const char* kChapterSevenEndingFirst = "ch07-end-first.sav";
inline constexpr const char* kChapterSevenEndingSecond = "ch07-end-second.sav";
inline constexpr const char* kWriteChapterSevenFixturesEnv = "FANREN_WRITE_CH07_FIXTURES";

inline std::filesystem::path chapterFixturePath(const std::string& assetRoot,
                                                const std::string& fileName) {
    return std::filesystem::path(assetRoot) / "tests" / "fixtures" / fileName;
}

inline bool environmentFlagSet(const char* name) {
#if defined(_MSC_VER)
    char* value = nullptr;
    std::size_t length = 0;
    if (_dupenv_s(&value, &length, name) != 0 || value == nullptr) return false;
    const bool set = value[0] != '\0';
    std::free(value);
    return set;
#else
    const char* value = std::getenv(name);
    return value != nullptr && value[0] != '\0';
#endif
}

// 一份状态经 io::saveGame 写出来的样子，逐行；口径见文件头（去掉三行）。
inline std::vector<std::string> comparableSaveLines(core::GameState state) {
    state.playSecondsGameplay = 0.0;
    state.playSecondsSystem = 0.0;
    const std::filesystem::path path = uniqueTempPath("fanren_chapter_fixture", ".json");
    std::vector<std::string> lines;
    if (!io::saveGame(state, path.string()).ok) return lines;
    {
        std::ifstream in(path, std::ios::binary);
        std::string line;
        while (std::getline(in, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.find("\"checksum\"") != std::string::npos) continue;
            lines.push_back(line);
        }
    }
    std::error_code ec;
    std::filesystem::remove(path, ec);
    return lines;
}

// 两份存档逐行比，返回每一处不同（最多 limit 条），每条带上它落在 payload 的哪个字段里。
// 空表 = 一字不差。
inline std::vector<std::string> saveLineDifferences(const std::vector<std::string>& expected,
                                                    const std::vector<std::string>& actual,
                                                    std::size_t limit = 12) {
    std::vector<std::string> out;
    // payload 的顶层字段在 dump(2) 里是缩进四格的那一行；记住最近的一个当上下文。
    const auto fieldOf = [](const std::string& line, std::string& current) {
        if (line.rfind("    \"", 0) == 0 && line.size() > 5 && line[4] == '"') {
            const std::size_t close = line.find('"', 5);
            if (close != std::string::npos) current = line.substr(5, close - 5);
        }
    };
    std::string field;
    const std::size_t common = std::min(expected.size(), actual.size());
    for (std::size_t i = 0; i < common && out.size() < limit; ++i) {
        fieldOf(expected[i], field);
        if (expected[i] == actual[i]) continue;
        out.push_back("第 " + std::to_string(i + 1) + " 行（字段 " + field + "）：交接存档是「" +
                      expected[i] + "」，这一趟是「" + actual[i] + "」");
    }
    if (expected.size() != actual.size() && out.size() < limit) {
        out.push_back("行数不同：交接存档 " + std::to_string(expected.size()) + " 行，这一趟 " +
                      std::to_string(actual.size()) + " 行");
    }
    return out;
}

inline std::string joinLines(const std::vector<std::string>& lines) {
    std::ostringstream out;
    for (const std::string& line : lines) out << "\n  " << line;
    return out.str();
}

// 一趟通关的终局对交接存档：写，或者逐字段比。
//
// 第 3 章那一头是 Ch03Walkthrough::settleEndingAgainstFixture 自己写的（那个文件不在
// 第 5 章引擎这一批的白名单里，没有去动）；第 4 章起走这一个，不再每章抄一遍。
// 不含 gtest 断言，调用方拿 problem 去 EXPECT：空串 = 一字不差（或者这一趟是在写）。
struct FixtureVerdict {
    bool wrote = false;     // 置了重写开关，这一趟写出了文件
    std::string problem;    // 空 = 通过；否则写明是哪里不对、下一步该怎么办
};

inline FixtureVerdict settleAgainstFixture(const core::GameState& state, const std::string& assetRoot,
                                           const std::string& fileName, const char* writeEnv,
                                           const std::string& nextChapter) {
    FixtureVerdict verdict;
    const std::filesystem::path fixture = chapterFixturePath(assetRoot, fileName);
    if (environmentFlagSet(writeEnv)) {
        const auto wrote = io::saveGame(state, fixture.string());
        verdict.wrote = wrote.ok;
        if (!wrote.ok) verdict.problem = "交接存档写不出去：" + wrote.error;
        return verdict;
    }
    if (!std::filesystem::exists(fixture)) {
        verdict.problem = "交接存档 " + fixture.string() + " 不在。它是" + nextChapter +
                          "通关测试的起点，用 " + writeEnv + "=1 跑一遍本用例生成";
        return verdict;
    }
    const auto handedOver = io::loadGame(fixture.string());
    if (!handedOver.ok) {
        verdict.problem = "交接存档读不回来：" + handedOver.error;
        return verdict;
    }
    const std::vector<std::string> expected = comparableSaveLines(handedOver.value);
    const std::vector<std::string> actual = comparableSaveLines(state);
    // 先验：两边都真的写出了东西。两份空表逐行比是恒等的。
    if (expected.size() <= 20 || actual.size() <= 20) {
        verdict.problem = "存档写出来的行数不对（交接存档 " + std::to_string(expected.size()) +
                          " 行，这一趟 " + std::to_string(actual.size()) + " 行），比对没有意义";
        return verdict;
    }
    const std::vector<std::string> diff = saveLineDifferences(expected, actual);
    if (!diff.empty()) {
        verdict.problem = "这一趟的终局与交接存档 " + fileName + " 对不上——" + nextChapter +
                          "是从那份存档起步的，两章之间已经漂开了。若是有意改的，按 "
                          "tests/ChapterFixture.h 重生成，再跑一遍" + nextChapter + "：" +
                          joinLines(diff);
    }
    return verdict;
}

}  // namespace fanren::test
