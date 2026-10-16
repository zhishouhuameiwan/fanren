// 跨平台确定性：UniformInt（替代 std::uniform_int_distribution）与 portableSort（替代
// GameState::removeItem 里的 std::sort）。两者都逐位复刻 MSVC STL，让 macOS / libc++
// 掷出、排出与 Windows 上同样的结果——通关用例与 tests/fixtures 里的交接存档全是在
// MSVC 上生成的（2026-10-15 移植 macOS 时补）。
//
// 钉死的期望值由本实现算出；它与 MSVC 一致的证据是第 4–13 章的通关用例在 macOS 上
// 与 MSVC 写出的交接存档逐行相等。在 MSVC 上另有两条用例直接与标准库对拍。

#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <random>
#include <utility>
#include <vector>

#include "core/model/PortableSort.h"
#include "core/rules/UniformInt.h"

namespace {

using fanren::rules::UniformInt;

struct Tagged {
    int key = 0;
    int tag = 0;
};

bool byKey(const Tagged& a, const Tagged& b) { return a.key < b.key; }

// 绝大多数元素同键——正是背包里「年份全是 0」的样子。
std::vector<Tagged> tieHeavy(int count, std::uint32_t seed) {
    std::mt19937 rng(seed);
    std::vector<Tagged> v;
    for (int i = 0; i < count; ++i) {
        v.push_back(Tagged{rng() % 5 == 0 ? static_cast<int>(rng() % 4) * 10 : 0, i});
    }
    return v;
}

std::vector<int> tagsOf(const std::vector<Tagged>& v) {
    std::vector<int> tags;
    for (const Tagged& t : v) tags.push_back(t.tag);
    return tags;
}

TEST(UniformInt, PinsTheMsvcSequenceForTheRangesTheGameRolls) {
    std::mt19937 rng(20260929u);
    UniformInt percent(0, 99);
    UniformInt d100(1, 100);
    UniformInt signedRange(-5, 5);
    UniformInt insight(0, 9999);

    std::vector<int> got;
    for (int i = 0; i < 8; ++i) got.push_back(percent(rng));
    for (int i = 0; i < 4; ++i) got.push_back(d100(rng));
    for (int i = 0; i < 4; ++i) got.push_back(signedRange(rng));
    for (int i = 0; i < 4; ++i) got.push_back(insight(rng));

    const std::vector<int> expected{30, 47, 89, 50, 8, 41, 68, 59,   // [0, 99]
                                    61, 44, 25, 16,                  // [1, 100]
                                    4, 5, 1, 1,                      // [-5, 5]
                                    3027, 5385, 4869, 2126};         // [0, 9999]
    EXPECT_EQ(got, expected);
}

TEST(UniformInt, StaysInsideTheClosedRangeAndReachesBothEnds) {
    std::mt19937 rng(7u);
    UniformInt roll(-3, 3);
    bool sawLow = false;
    bool sawHigh = false;
    for (int i = 0; i < 2000; ++i) {
        const int v = roll(rng);
        ASSERT_GE(v, -3);
        ASSERT_LE(v, 3);
        sawLow = sawLow || v == -3;
        sawHigh = sawHigh || v == 3;
    }
    EXPECT_TRUE(sawLow);
    EXPECT_TRUE(sawHigh);
}

TEST(UniformInt, ASingleValueRangeAlwaysGivesThatValue) {
    std::mt19937 rng(1u);
    UniformInt only(42, 42);
    for (int i = 0; i < 50; ++i) EXPECT_EQ(only(rng), 42);
}

TEST(UniformInt, TheFullIntRangeTakesOneRawDrawPerRoll) {
    std::mt19937 rng(99u);
    std::mt19937 twin(99u);
    UniformInt full(INT32_MIN, INT32_MAX);
    for (int i = 0; i < 10; ++i) {
        const auto raw = static_cast<std::uint32_t>(twin());
        EXPECT_EQ(full(rng), static_cast<int>(raw ^ 0x80000000u));
    }
}

TEST(PortableSort, OrdersEveryTieHeavySizeAndKeepsTheSameElements) {
    for (int count = 0; count <= 200; ++count) {
        std::vector<Tagged> v = tieHeavy(count, static_cast<std::uint32_t>(count) + 1u);
        fanren::core::portableSort(v.begin(), v.end(), byKey);
        ASSERT_TRUE(std::is_sorted(v.begin(), v.end(), byKey)) << count << " 个";
        std::vector<int> tags = tagsOf(v);
        std::sort(tags.begin(), tags.end());
        for (int i = 0; i < count; ++i) ASSERT_EQ(tags[static_cast<std::size_t>(i)], i) << count << " 个";
    }
}

TEST(PortableSort, TheHeapFallbackAlsoOrders) {
    // 正常输入走不到堆排序那一支（要连续 1.5 log2(N) 层划分都极偏）；直接把层数额度给 0。
    for (const int count : {33, 64, 117}) {
        std::vector<Tagged> v = tieHeavy(count, 3u);
        fanren::core::portable_sort_detail::sortUnchecked(v.begin(), v.end(), std::ptrdiff_t{0}, byKey);
        EXPECT_TRUE(std::is_sorted(v.begin(), v.end(), byKey)) << count << " 个";
    }
}

TEST(PortableSort, UpToThirtyTwoElementsTiesKeepTheirOrder) {
    // MSVC 在 32 个以内只做插入排序，相等元素不挪——第 6 章及以前背包都在这一档。
    std::vector<Tagged> v = tieHeavy(32, 11u);
    std::vector<Tagged> stable = v;
    fanren::core::portableSort(v.begin(), v.end(), byKey);
    std::stable_sort(stable.begin(), stable.end(), byKey);
    EXPECT_EQ(tagsOf(v), tagsOf(stable));
}

TEST(PortableSort, PinsTheMsvcTieOrderPastThirtyTwoElements) {
    std::vector<Tagged> v;
    for (int i = 0; i < 48; ++i) v.push_back(Tagged{(i * 7) % 3 == 0 ? 5 : 0, i});
    fanren::core::portableSort(v.begin(), v.end(), byKey);
    const std::vector<int> expected{17, 1,  2,  22, 4,  5,  41, 7,  8,  19, 10, 11, 20, 13, 14, 23,
                                    16, 25, 26, 28, 29, 31, 32, 34, 35, 37, 38, 40, 43, 44, 46, 47,
                                    18, 33, 21, 6,  36, 0,  3,  39, 27, 24, 42, 9,  12, 45, 30, 15};
    EXPECT_EQ(tagsOf(v), expected);
}

#if defined(_MSC_VER)
// 只在 MSVC 上编：直接与 MSVC 的标准库对拍，复刻一旦走样就在 Windows 上当场红。
TEST(PortableDeterminismOnMsvc, UniformIntMatchesTheStandardDistribution) {
    for (const auto& [lo, hi] : {std::pair{0, 99}, std::pair{1, 100}, std::pair{-5, 5}, std::pair{0, 9999},
                                 std::pair{50, 150}, std::pair{0, 2}, std::pair{-1000000, 1000000}}) {
        std::mt19937 a(12345u);
        std::mt19937 b(12345u);
        UniformInt mine(lo, hi);
        std::uniform_int_distribution<int> standard(lo, hi);
        for (int i = 0; i < 1000; ++i) ASSERT_EQ(mine(a), standard(b)) << "[" << lo << ", " << hi << "] 第 " << i;
    }
}

TEST(PortableDeterminismOnMsvc, PortableSortMatchesStdSort) {
    for (int count = 0; count <= 300; ++count) {
        std::vector<Tagged> mine = tieHeavy(count, static_cast<std::uint32_t>(count) * 31u + 5u);
        std::vector<Tagged> standard = mine;
        fanren::core::portableSort(mine.begin(), mine.end(), byKey);
        std::sort(standard.begin(), standard.end(), byKey);
        ASSERT_EQ(tagsOf(mine), tagsOf(standard)) << count << " 个";
    }
}

TEST(PortableDeterminismOnMsvc, TheHeapFallbackMatchesMakeHeapThenSortHeap) {
    // MSVC 的 std::sort 层数用尽时就是 make_heap + sort_heap；正常输入走不到，单独对拍。
    for (int count = 33; count <= 200; ++count) {
        std::vector<Tagged> mine = tieHeavy(count, static_cast<std::uint32_t>(count) * 17u + 3u);
        std::vector<Tagged> standard = mine;
        fanren::core::portable_sort_detail::sortUnchecked(mine.begin(), mine.end(), std::ptrdiff_t{0}, byKey);
        std::make_heap(standard.begin(), standard.end(), byKey);
        std::sort_heap(standard.begin(), standard.end(), byKey);
        ASSERT_EQ(tagsOf(mine), tagsOf(standard)) << count << " 个";
    }
}
#endif

}  // namespace
