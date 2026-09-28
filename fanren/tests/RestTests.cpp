// 静养：日子过去了，伤也就养回来了。
//
// ---------------------------------------------------------------------------
// 为什么要有这一条（第 4 章独立校对 MEDIUM-3）
// ---------------------------------------------------------------------------
// 在 `Application::restFor` 出现之前，**这个游戏没有任何一处把气血加回去**：
// 战斗里能吃药，战斗之外一处也没有——打坐面板不回血（突破失败时还会砍半）、
// `advance_days` 一点也不补、没有客栈也没有休息点。
// 于是韩立从第 1 章带的伤会一路带到第 14 章，而第 4 章的三场仗之间
// 隔着的那三天备战在机制上等于不存在。
//
// 口径（论证写在 `Application.cpp` 的 restFor() 上头）：
//   **每过去一天回上限的一成（至少 1 点），封顶满值**，同伴一起。
// 出处：`docs/handoff.md` 第 2 节 MEDIUM-3 那一行、`docs/ch04-reverify.md` 第七节判据 4。
//
// ---------------------------------------------------------------------------
// 这份文件第一版没有牙（第 4 章复验 N-4），这一版改了两处
// ---------------------------------------------------------------------------
// 复验把「一天一成」改成「一天半成」、又把 restFor 从 advanceDays 里摘掉，
// 两次都是全绿。原因都写在旧版自己身上：
//
//   1. **期望值由 `kRestPercentPerDay` 自己算出来**（`before + s.maxHp / kRestPercentPerDay`）。
//      口径一改，判据跟着改——docs/README.md 那张表上「判据从被测物推导出来」那一行。
//      现在每一个期望值都是**字面量**，从设计口径手算：上限 120 的人一天回 12。
//   2. **刻意绕开 `advanceDays`，直接调 restFor**。于是「过日子会不会回血」这件事——
//      玩家真正碰得到的那一条——没有任何东西看着。现在有两条经 `advanceDays` 走的用例，
//      并且先验「他确实带着伤」：满血的人过几天仍是满血，那种绿灯什么也没证明。
//
// 直接调 restFor 的那几条留着：边界（0 天、小上限、同伴记号）只有在那里才碰得干净。
#include <gtest/gtest.h>

#include <filesystem>
#include <string>

#include "core/model/Types.h"
#include "game/Application.h"

namespace {

namespace fs = std::filesystem;

using fanren::core::GameState;
using fanren::core::PartyMember;
using fanren::game::Application;

constexpr const char* kCompanion = "qu_hun";

std::string assetRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "data" / "roles" / "qu_hun.json")) {
            return candidate;
        }
    }
    return ".";
}

class Rest : public ::testing::Test {
protected:
    void SetUp() override {
        auto ready = app_.init(assetRoot(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
        GameState& s = state();
        s.maxHp = 120;   // 本章终点炼气八层的气血
        s.maxMp = 80;
        s.hp = 20;
        s.mp = 8;
    }
    void TearDown() override { app_.shutdown(); }

    GameState& state() { return app_.state(); }

    Application app_;
};

// ---------------------------------------------------------------------------
// 设计口径，写成字面量
// ---------------------------------------------------------------------------
// 「一天回上限一成」：上限 120 的气血一天回 12，上限 80 的法力一天回 8。
// 这两个数是照口径手算的，**不许改成从 kRestPercentPerDay 推**——那样改口径的人
// 会顺手把判据一起改掉，而这条用例存在的全部理由就是拦住那一下。
TEST_F(Rest, OneDayGivesBackATenthOfTheCapAsTheDesignSays) {
    GameState& s = state();
    ASSERT_EQ(s.maxHp, 120) << "先验：夹具的上限就是下面手算用的那个数";
    ASSERT_EQ(s.maxMp, 80);
    ASSERT_EQ(s.hp, 20) << "先验：带着伤，离上限还远，一天回多少不会被封顶吃掉";
    ASSERT_EQ(s.mp, 8);

    app_.restFor(1);
    EXPECT_EQ(s.hp, 32) << "上限 120，一天一成该回 12 点（20 → 32）";
    EXPECT_EQ(s.mp, 16) << "上限 80，一天一成该回 8 点（8 → 16）";
}

// ---------------------------------------------------------------------------
// 正题：十天回满，不会超
// ---------------------------------------------------------------------------
TEST_F(Rest, TenDaysIsExactlyFullAndItNeverOvershoots) {
    GameState& s = state();
    s.hp = 0;
    s.mp = 0;
    app_.restFor(9);
    EXPECT_EQ(s.hp, 108) << "九天是九成，还差一天";
    EXPECT_EQ(s.mp, 72);
    app_.restFor(1);
    EXPECT_EQ(s.hp, 120) << "第十天该回满";
    EXPECT_EQ(s.mp, 80);

    // 再躺一年也不会溢出上限。
    app_.restFor(365);
    EXPECT_EQ(s.hp, 120);
    EXPECT_EQ(s.mp, 80);
}

// 三天回三成——这一条是本章的实际用处（jinguang.lua 末尾的 advance_days(3) 是三天备战，
// 殿上那一场之后、攻防战之前；2026-09-23 从 kaizhan.lua 挪过来）。它同时是「不立刻回满」
// 那个取舍的判据：写成回满的话这里会红。
// 注意它守的是**速率**，不是攻防战前的实际气血：通关测试那一趟韩立坊门之后剩 110，
// 三天回 36 正好回满（二次复验 5.1）；带着三成以下的伤进三天备战的，才看得出这「三成」。
TEST_F(Rest, ThreeDaysOfPreparationIsThreeTenthsNotAFullHeal) {
    GameState& s = state();
    s.hp = 0;
    app_.restFor(3);
    EXPECT_EQ(s.hp, 36) << "三天三成：上限 120 回 36";
    EXPECT_LT(s.hp, s.maxHp) << "三天就回满的话，攻防战前那点紧张感就没有了";
}

TEST_F(Rest, ZeroOrNegativeDaysChangeNothing) {
    GameState& s = state();
    const int hp = s.hp;
    const int mp = s.mp;
    app_.restFor(0);
    app_.restFor(-7);
    EXPECT_EQ(s.hp, hp);
    EXPECT_EQ(s.mp, mp);
}

// 上限很小的时候每天至少回 1 点：整数除法会让上限低于十的角色永远回 0 点，
// 而第 1 章的韩立上限正是 10。
TEST_F(Rest, ATinyPoolStillHealsAtLeastOneADay) {
    GameState& s = state();
    s.maxHp = 10;
    s.hp = 1;
    s.maxMp = 0;
    s.mp = 0;
    app_.restFor(1);
    EXPECT_EQ(s.hp, 2);
    EXPECT_EQ(s.mp, 0) << "法力上限是 0 的凡人不该凭空长出法力";

    // 一成不足 1 点的另一头：上限 5，一天仍回 1 点而不是 0 点。
    s.maxHp = 5;
    s.hp = 1;
    app_.restFor(1);
    EXPECT_EQ(s.hp, 2);
}

// ---------------------------------------------------------------------------
// 玩家碰得到的那一条：过日子就回血（经 advanceDays，不直接调 restFor）
// ---------------------------------------------------------------------------
// `advance_days` 脚本命令、打坐面板、剧情跳时全走 Application::advanceDays 这一个入口
//（见 Application.h 那一段注释与 dispatch 里 AdvanceDays 那一支），所以这里验的就是
// 玩家真会遇到的那条路。把 restFor 从 advanceDays 里摘掉，这两条必须红。
TEST_F(Rest, PassingDaysThroughTheCalendarMendsTheWound) {
    GameState& s = state();
    const int dayBefore = s.day;
    // 先验：他确实带着伤。满血的人过几天还是满血，那种绿灯什么也证明不了。
    ASSERT_LT(s.hp, s.maxHp) << "先验：开场必须带伤";
    ASSERT_LT(s.mp, s.maxMp) << "先验：法力也必须没满";
    ASSERT_EQ(s.hp, 20);
    ASSERT_EQ(s.mp, 8);

    app_.advanceDays(3);

    ASSERT_EQ(s.day, dayBefore + 3) << "先验：日历真的往前走了三天";
    EXPECT_EQ(s.hp, 56) << "三天三成：20 + 3 × 12";
    EXPECT_EQ(s.mp, 32) << "三天三成：8 + 3 × 8";
}

// 同伴经日历一样养伤。少了这一条，曲魂那一份可以单独从 advanceDays 里掉出去
// 而不被任何人发现（restFor 里只要把同伴那一段挪走就是）。
TEST_F(Rest, TheCompanionMendsThroughTheCalendarToo) {
    GameState& s = state();
    PartyMember quhun;
    quhun.roleId = kCompanion;
    quhun.hp = 1;
    quhun.active = true;
    s.party.push_back(quhun);

    const fanren::core::RoleTemplate* role = app_.data().findRole(kCompanion);
    ASSERT_NE(role, nullptr) << "先验：data 里得有这个同伴";
    // 下面的期望值按「上限 55、一天一成 = 5 点」手算，先验这个上限没变。
    ASSERT_EQ(role->maxHp, 55) << "先验：曲魂的上限变了，下面手算的 1 + 2 × 5 要跟着重算";
    ASSERT_LT(s.party.front().hp, role->maxHp) << "先验：他确实带着伤";

    app_.advanceDays(2);
    EXPECT_EQ(s.party.front().hp, 11) << "上限 55，一天一成回 5，两天 1 → 11";
}

// ---------------------------------------------------------------------------
// 同伴也养伤（直接调 restFor 的边界）
// ---------------------------------------------------------------------------
// 少了这一条，曲魂会在第 3 章末带着伤一路走到第 14 章，
// 而玩家没有任何办法替他治。
TEST_F(Rest, TheCompanionMendsToo) {
    GameState& s = state();
    PartyMember quhun;
    quhun.roleId = kCompanion;
    quhun.hp = 5;
    quhun.active = true;
    s.party.push_back(quhun);

    const fanren::core::RoleTemplate* role = app_.data().findRole(kCompanion);
    ASSERT_NE(role, nullptr) << "先验：data 里得有这个同伴";
    ASSERT_GT(role->maxHp, 0);

    app_.restFor(1000);
    EXPECT_EQ(s.party.front().hp, role->maxHp) << "封顶在模板血上";
}

// `hp < 0` 是「还没打过仗，按模板满血」的记号（core::PartyMember 的注释）。
// 把它当成一个受了重伤的人抬上来，会让一个满血的同伴变成 −1 + 每日回复。
TEST_F(Rest, TheNotYetFoughtMarkerIsNotTreatedAsAWound) {
    GameState& s = state();
    PartyMember fresh;
    fresh.roleId = kCompanion;
    fresh.hp = -1;
    fresh.active = true;
    s.party.push_back(fresh);

    app_.restFor(30);
    EXPECT_EQ(s.party.front().hp, -1)
        << "「还没打过仗」这个记号被当成伤势治了一遍，那个同伴上场时不再是满血";
}

}  // namespace
