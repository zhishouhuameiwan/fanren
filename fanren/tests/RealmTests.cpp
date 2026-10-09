#include "core/rules/Realm.h"

#include <gtest/gtest.h>

#include <cstdlib>
#include <iterator>
#include <vector>

#include "core/model/Types.h"

namespace {

using fanren::rules::Realm;
using fanren::rules::RealmTier;
// liftToFloor 的第一个实参是花括号初始化列表，不带类型，ADL 找不到它 —— 显式引进来。
using fanren::rules::liftToFloor;

TEST(RealmValidity, AcceptsDefinedRealmsAndRejectsGaps) {
    EXPECT_TRUE(isValid(Realm::Mortal));
    EXPECT_TRUE(isValid(Realm::QiRefining1));
    EXPECT_TRUE(isValid(Realm::QiRefining13));
    EXPECT_TRUE(isValid(Realm::FoundationEarly));
    EXPECT_TRUE(isValid(Realm::CoreLate));

    // 编号刻意留空段，落在空段里的值必须判非法（存档篡改 / 脚本传错）。
    EXPECT_FALSE(isValid(fanren::rules::fromValue(14)));
    EXPECT_FALSE(isValid(fanren::rules::fromValue(20)));
    EXPECT_FALSE(isValid(fanren::rules::fromValue(24)));
    EXPECT_FALSE(isValid(fanren::rules::fromValue(34)));
    EXPECT_FALSE(isValid(fanren::rules::fromValue(-1)));
}

TEST(RealmTierMapping, MapsEachBandToItsTier) {
    EXPECT_EQ(tierOf(Realm::Mortal), RealmTier::Mortal);
    EXPECT_EQ(tierOf(Realm::QiRefining1), RealmTier::QiRefining);
    EXPECT_EQ(tierOf(Realm::QiRefining13), RealmTier::QiRefining);
    EXPECT_EQ(tierOf(Realm::FoundationMid), RealmTier::Foundation);
    EXPECT_EQ(tierOf(Realm::CoreEarly), RealmTier::Core);
}

TEST(RealmNaming, GivesDisplayNamesAndFallsBackSafely) {
    EXPECT_EQ(nameOf(Realm::Mortal), "凡人");
    EXPECT_EQ(nameOf(Realm::QiRefining1), "炼气一层");
    EXPECT_EQ(nameOf(Realm::QiRefining13), "炼气十三层");
    EXPECT_EQ(nameOf(Realm::FoundationEarly), "筑基初期");
    EXPECT_EQ(nameOf(Realm::CoreLate), "结丹后期");
    EXPECT_EQ(nameOf(fanren::rules::fromValue(24)), "未知");
}

TEST(RealmProgression, WalksTheWholeLadderExactlyOnce) {
    // 从凡人一路推到上限，验证链条不断裂、不重复、终点正确。
    std::vector<Realm> visited;
    Realm current = Realm::Mortal;
    visited.push_back(current);

    Realm next{};
    while (tryNext(current, next)) {
        visited.push_back(next);
        current = next;
        ASSERT_LT(visited.size(), 64u) << "境界链条未收敛，可能成环";
    }

    // 凡人 1 + 炼气 13 + 筑基 3 + 结丹 3 = 20
    EXPECT_EQ(visited.size(), 20u);
    EXPECT_EQ(visited.front(), Realm::Mortal);
    EXPECT_EQ(visited.back(), Realm::CoreLate);

    for (Realm realm : visited) {
        EXPECT_TRUE(isValid(realm)) << "链条经过了非法编号 " << toValue(realm);
    }
}

TEST(RealmProgression, StopsAtCoreLateWhichIsTheHardCap) {
    // 大纲 4.1 硬约束：结丹后期是本作上限。
    Realm next{};
    EXPECT_FALSE(tryNext(Realm::CoreLate, next));
    EXPECT_EQ(cultivationNeeded(Realm::CoreLate), -1);
}

TEST(RealmProgression, CrossesTierBoundariesCorrectly) {
    Realm next{};
    ASSERT_TRUE(tryNext(Realm::QiRefining13, next));
    EXPECT_EQ(next, Realm::FoundationEarly);

    ASSERT_TRUE(tryNext(Realm::FoundationLate, next));
    EXPECT_EQ(next, Realm::CoreEarly);
}

TEST(RealmCultivationCost, IsPositiveAndMonotonicAcrossTheLadder) {
    Realm current = Realm::Mortal;
    Realm next{};
    std::int32_t previousCost = 0;

    while (tryNext(current, next)) {
        const std::int32_t cost = cultivationNeeded(current);
        EXPECT_GT(cost, 0) << nameOf(current) << " 的突破消耗必须为正";
        EXPECT_GE(cost, previousCost) << nameOf(current) << " 的突破消耗不应低于前一境界";
        previousCost = cost;
        current = next;
    }
}

TEST(RealmCultivationCost, RejectsInvalidRealms) {
    EXPECT_EQ(cultivationNeeded(fanren::rules::fromValue(24)), -1);
}

TEST(RealmDemotion, ReproducesTheChapterNineFall) {
    // 第 9 章「遁走」：真元被夺，自筑基中期跌回炼气期。
    const Realm fallen = demote(Realm::FoundationMid, 5);
    EXPECT_EQ(tierOf(fallen), RealmTier::QiRefining);
    EXPECT_TRUE(isValid(fallen));
}

TEST(RealmDemotion, NeverFallsBackToMortal) {
    // 修为根基仍在，再怎么跌也不该回到凡人。
    const Realm fallen = demote(Realm::CoreLate, 999);
    EXPECT_EQ(fallen, Realm::QiRefining1);
    EXPECT_NE(fallen, Realm::Mortal);
}

TEST(RealmDemotion, IsAnIdentityForNonPositiveLevels) {
    EXPECT_EQ(demote(Realm::FoundationMid, 0), Realm::FoundationMid);
    EXPECT_EQ(demote(Realm::FoundationMid, -3), Realm::FoundationMid);
}

TEST(RealmDemotion, StepsDownOneRungAtATime) {
    EXPECT_EQ(demote(Realm::CoreEarly, 1), Realm::FoundationLate);
    EXPECT_EQ(demote(Realm::FoundationEarly, 1), Realm::QiRefining13);
    EXPECT_EQ(demote(Realm::QiRefining2, 1), Realm::QiRefining1);
}

TEST(RealmSuppression, IsNeutralBetweenEqualRealms) {
    EXPECT_DOUBLE_EQ(suppressionFactor(Realm::FoundationMid, Realm::FoundationMid), 1.0);
}

TEST(RealmSuppression, FavoursTheHigherTierButStaysBounded) {
    const double strong = suppressionFactor(Realm::CoreEarly, Realm::QiRefining1);
    EXPECT_GT(strong, 1.0);
    EXPECT_LE(strong, 2.2) << "压制过强会让战棋失去博弈空间";

    const double weak = suppressionFactor(Realm::QiRefining1, Realm::CoreEarly);
    EXPECT_LT(weak, 1.0);
    EXPECT_GE(weak, 0.4) << "弱势方仍须有还手余地";
}

TEST(RealmSuppression, AdjustsMildlyWithinATier) {
    const double factor = suppressionFactor(Realm::QiRefining9, Realm::QiRefining1);
    EXPECT_GT(factor, 1.0);
    EXPECT_LE(factor, 1.4);
}

TEST(RealmSuppression, FallsBackToNeutralOnInvalidInput) {
    EXPECT_DOUBLE_EQ(suppressionFactor(fanren::rules::fromValue(24), Realm::CoreEarly), 1.0);
    EXPECT_DOUBLE_EQ(suppressionFactor(Realm::CoreEarly, fanren::rules::fromValue(24)), 1.0);
}

// ===========================================================================
// 境界给出的气血 / 法力基准
// ===========================================================================
//
// 缺陷 B 的规则层：在这之前全作没有任何一处让 maxHp / maxMp 增长，突破只改一个
// 境界编号。数值不是拍的，是从 data/roles 已有的敌人曲线上反解出来的，取值依据
// 逐条列在 core/rules/Realm.h。
//
// 这一组刻意**扫一片**而不是钉一个点：docs/README.md 那张空转法表里写着
// 「只钉一个数据点 —— 另一条同样错的曲线只要穿过该点就蒙混过关」。所以下面同时
// 钉住形状（单调、跨大境界必须跳）、端点（凡人保持 10）与 data 上的五个锚点。

TEST(RealmVitals, MortalKeepsTheGameStateDefaultSoTheFirstTwoChaptersDoNotMove) {
    // 凡人一档必须与 GameState 的默认值一字不差：第 1、2 章一场战斗都没有，
    // 改它只会平白改掉两章的既有行为。
    EXPECT_EQ(realmMaxHp(Realm::Mortal), 10);
    EXPECT_EQ(realmMaxMp(Realm::Mortal), 0) << "凡人还不是修士，没有法力";
    const fanren::core::GameState fresh;
    EXPECT_EQ(fresh.maxHp, realmMaxHp(fresh.realm)) << "新档一开局就该是自洽的";
    EXPECT_EQ(fresh.maxMp, realmMaxMp(fresh.realm));
}

TEST(RealmVitals, RisesStrictlyWithEveryStepOfTheLadder) {
    // 形状先钉死：整条阶梯严格递增。哪一层给的不如上一层多，玩家突破一次反而
    // 变弱 —— 那种错在面板上只是一个数字，玩家要打了一仗才发现。
    Realm current = Realm::Mortal;
    Realm next{};
    while (tryNext(current, next)) {
        EXPECT_LT(realmMaxHp(current), realmMaxHp(next))
            << nameOf(current) << " → " << nameOf(next) << " 气血没涨";
        EXPECT_LT(realmMaxMp(current), realmMaxMp(next))
            << nameOf(current) << " → " << nameOf(next) << " 法力没涨";
        current = next;
    }
    EXPECT_EQ(current, Realm::CoreLate) << "先验：这圈真的走完了整条阶梯";
}

TEST(RealmVitals, LandsOnTheEnemyCurveThatDataAlreadyDefines) {
    // data/roles 里同境界角色的实际数值 —— 主角必须落在同一条曲线上，否则
    // 「境界」这个词在玩家与敌人身上是两个意思。允许的偏差写在每一行后面。
    //
    // 这五个点分散在炼气四层到结丹中期之间：只取一个点的话，任何一条穿过它的
    // 曲线都能蒙混过关（docs/README.md 那张表的「只钉一个数据点」）。
    struct Anchor {
        Realm realm;
        int dataMaxHp;      // data/roles 里该境界角色的 maxHp
        const char* who;
    };
    const Anchor anchors[]{
        {Realm::QiRefining4, 72, "jiexiu"},
        {Realm::QiRefining12, 180, "lu_shixiong"},
        {Realm::QiRefining13, 176, "heishajiao_shigui"},
        {Realm::FoundationLate, 470, "wang_chan"},
        {Realm::CoreMid, 900, "wu_chou"},
    };
    for (const Anchor& a : anchors) {
        const int mine = realmMaxHp(a.realm);
        ASSERT_GT(a.dataMaxHp, 0) << "先验：锚点自己不能是 0，否则下面的比值恒真";
        // 主角与同境界的敌人相差不超过一成半：再宽就等于没有约束了。
        EXPECT_LE(std::abs(mine - a.dataMaxHp) * 100, a.dataMaxHp * 15)
            << nameOf(a.realm) << " 给主角 " << mine << " 点气血，而 data 里同境界的 "
            << a.who << " 是 " << a.dataMaxHp << " 点 —— 差得太远";
    }
}

TEST(RealmVitals, JumpsAtEveryGreatRealmBoundary) {
    // 大境界关口要跳一大截，不是平滑过渡：突破筑基与升一层炼气在玩家手上
    // 必须是两件事。判据取「至少多出上一档的三成」。
    EXPECT_GT(realmMaxHp(Realm::FoundationEarly) * 10, realmMaxHp(Realm::QiRefining13) * 13)
        << "炼气圆满 → 筑基初期没有跳";
    EXPECT_GT(realmMaxHp(Realm::CoreEarly) * 10, realmMaxHp(Realm::FoundationLate) * 13)
        << "筑基后期 → 结丹初期没有跳";
}

TEST(RealmVitals, InvalidRealmsFallBackInsteadOfReturningGarbage) {
    EXPECT_EQ(realmMaxHp(fanren::rules::fromValue(24)), realmMaxHp(Realm::Mortal));
    EXPECT_EQ(realmMaxMp(fanren::rules::fromValue(24)), 0);
}

// ---- liftToFloor：只补不削，当前值同步抬高 ----

TEST(RealmVitalsLift, RaisesBothTheCapAndTheCurrentValueByTheSameAmount) {
    // 补上去的那一截是根基，不是伤势。只抬上限的话，一份满血的老档读进来会变成
    // 10/60，玩家一进战斗就死 —— 那不是修好了，那是换了个坏法。
    const fanren::rules::Vitals lifted = liftToFloor({10, 10}, 60);
    EXPECT_EQ(lifted.max, 60);
    EXPECT_EQ(lifted.current, 60) << "满血的补齐之后还该是满血";

    const fanren::rules::Vitals wounded = liftToFloor({4, 10}, 60);
    EXPECT_EQ(wounded.max, 60);
    EXPECT_EQ(wounded.current, 54) << "带伤的该原样带着那 6 点伤，不是按比例缩水";
}

TEST(RealmVitalsLift, NeverCutsAnythingDown) {
    // 只补不削。读档时按境界**重算**会把日后丹药、功法、装备给的上限悄悄抹掉，
    // 而那种损失在存档里看不出来、也找不回来。第 9 章的境界跌落同理：掉境界要不要
    // 跟着削气血是那一章的设计决定，不该由一条补齐规则替它定。
    const fanren::rules::Vitals rich = liftToFloor({300, 400}, 60);
    EXPECT_EQ(rich.max, 400);
    EXPECT_EQ(rich.current, 300);

    const fanren::rules::Vitals exact = liftToFloor({7, 60}, 60);
    EXPECT_EQ(exact.max, 60);
    EXPECT_EQ(exact.current, 7) << "上限正好等于基准时一个字节都不该动";
}

// ---- 境界给出的攻 / 防（技术债 G-10）----
//
// 在这两条曲线出现之前，韩立的攻 6 / 防 3 写死在 BattleScene.cpp，从第 1 章到
// 第 14 章一个数也不动。取值依据与锚定的原委逐条列在 core/rules/Realm.h。
//
// 这一组**逐层钉死十三层**，不是钉一两个点：docs/README.md 那张空转法表上
// 写着「只钉一个数据点 —— 另一条同样错的曲线只要穿过该点就蒙混过关」，
// 而这里正是最容易发生的那一种：锚定之后炼气一至三层全是 6/3，
// 一个「恒返回 6/3」的实现能穿过前四个点。

// 设计给定的逐层取值（Realm.h 的表，一格不差）。写成字面量而不是再算一遍
// 公式：判据若是从被测物的公式推出来的，它只发现得了实现变了，发现不了
// 实现与设计不一致（docs/README.md 第 3 条坑的那个变体）。
struct QiCombatRow {
    Realm realm;
    int attack;
    int defence;
};
const QiCombatRow kQiCombat[]{
    {Realm::QiRefining1, 6, 3},   {Realm::QiRefining2, 6, 3},   {Realm::QiRefining3, 6, 3},
    {Realm::QiRefining4, 8, 4},   {Realm::QiRefining5, 10, 5},  {Realm::QiRefining6, 11, 6},
    {Realm::QiRefining7, 13, 7},  {Realm::QiRefining8, 15, 8},  {Realm::QiRefining9, 17, 9},
    {Realm::QiRefining10, 19, 10}, {Realm::QiRefining11, 20, 11}, {Realm::QiRefining12, 22, 12},
    {Realm::QiRefining13, 24, 13},
};

TEST(RealmCombat, NailsEveryOneOfTheThirteenQiRefiningLayers) {
    for (const QiCombatRow& row : kQiCombat) {
        EXPECT_EQ(realmAttack(row.realm), row.attack)
            << nameOf(row.realm) << " 的攻不是设计给的 " << row.attack;
        EXPECT_EQ(realmDefence(row.realm), row.defence)
            << nameOf(row.realm) << " 的防不是设计给的 " << row.defence;
    }
    // 先验：这张表真的覆盖了整条炼气期，不是半路断了。
    ASSERT_EQ(std::size(kQiCombat), 13u);
}

TEST(RealmCombat, MortalKeepsTheNumbersTheFirstChaptersWereBuiltOn) {
    // 凡人一律 6 / 3，与写死的现值一字不差：第 1、2 章与识海那一场都靠它。
    EXPECT_EQ(realmAttack(Realm::Mortal), 6);
    EXPECT_EQ(realmDefence(Realm::Mortal), 3);
}

TEST(RealmCombat, IsAnchoredAtQiRefining3SoChapterThreeDoesNotMove) {
    // 这一条是 G-10 的硬要求，不是口味：data/roles/jiang_shou.json 与
    // data/battles/b03_andao_shishou.json 的说明写死了「韩立平砍 6*2-9=3」
    // 「本章的尺度是韩立攻 6」，第 3 章教学战二整场戏建在这个数上；
    // 防那一侧同样敏感——防从 3 抬到 7，教学战一的恶狼（攻 4）一刀只剩保底 1 点。
    EXPECT_EQ(realmAttack(Realm::QiRefining3), 6) << "锚点动了，第 3 章的教学战二作废";
    EXPECT_EQ(realmDefence(Realm::QiRefining3), 3) << "锚点动了，第 3 章的教学战一作废";
    // 凡人到炼气三层这一整段都不许动：第 3 章章末交出来的正是炼气三层。
    EXPECT_EQ(realmAttack(Realm::QiRefining1), 6);
    EXPECT_EQ(realmAttack(Realm::QiRefining2), 6);
    EXPECT_EQ(realmDefence(Realm::QiRefining1), 3);
    EXPECT_EQ(realmDefence(Realm::QiRefining2), 3);

    // 先验：上面那六个 EXPECT 单独成立，一个「恒返回 6/3」的实现也能全绿——
    // 那正是这条债修好之前的样子。所以这里再钉一句「后面真的涨」。
    EXPECT_GT(realmAttack(Realm::QiRefining13), realmAttack(Realm::QiRefining3))
        << "曲线是平的：攻防仍然不随境界动，G-10 没有真的修好";
    EXPECT_GT(realmDefence(Realm::QiRefining13), realmDefence(Realm::QiRefining3))
        << "曲线是平的：攻防仍然不随境界动，G-10 没有真的修好";
}

TEST(RealmCombat, PinsTheProvisionalFoundationAndCoreNumbers) {
    // 筑基以上是**暂定值**（Realm.h 写明「到那一章再标定」）：那几档一场仗都还
    // 没编，没有任何一场战斗能验证它们。钉在这里不是因为它们对，而是为了让
    // 「到那一章标定」这件事发生时必须经过这条测试，而不是悄悄改掉。
    EXPECT_EQ(realmAttack(Realm::FoundationEarly), 32);
    EXPECT_EQ(realmDefence(Realm::FoundationEarly), 20);
    EXPECT_EQ(realmAttack(Realm::FoundationMid), 37);
    EXPECT_EQ(realmDefence(Realm::FoundationMid), 26);
    EXPECT_EQ(realmAttack(Realm::FoundationLate), 44);
    EXPECT_EQ(realmDefence(Realm::FoundationLate), 30);
    EXPECT_EQ(realmAttack(Realm::CoreEarly), 62);
    EXPECT_EQ(realmDefence(Realm::CoreEarly), 46);
    EXPECT_EQ(realmAttack(Realm::CoreMid), 76);
    EXPECT_EQ(realmDefence(Realm::CoreMid), 58);
    EXPECT_EQ(realmAttack(Realm::CoreLate), 102);
    EXPECT_EQ(realmDefence(Realm::CoreLate), 82);
}

TEST(RealmCombat, NeverFallsAsTheLadderRises) {
    // 形状：整条阶梯**单调不降**。注意不是严格递增——凡人到炼气三层那一段
    // 是锚定刻意压平的（见上一条），那四档必须一样。突破一次反而变弱的话，
    // 玩家要打了一仗才发现。
    Realm current = Realm::Mortal;
    Realm next{};
    int steps = 0;
    while (tryNext(current, next)) {
        EXPECT_LE(realmAttack(current), realmAttack(next))
            << nameOf(current) << " → " << nameOf(next) << " 攻掉下来了";
        EXPECT_LE(realmDefence(current), realmDefence(next))
            << nameOf(current) << " → " << nameOf(next) << " 防掉下来了";
        current = next;
        ++steps;
    }
    EXPECT_EQ(current, Realm::CoreLate) << "先验：这圈真的走完了整条阶梯";
    EXPECT_EQ(steps, 19) << "先验：走了 19 步（凡人 + 炼气 13 + 筑基 3 + 结丹 3 共 20 档）";
}

TEST(RealmCombat, InvalidRealmsFallBackToMortalInsteadOfGarbage) {
    // 编号刻意留空段。落在空段里的值一律退回凡人档，不返回垃圾：
    // 一份篡改过的存档不该换来一个攻 0 或者攻上千的韩立。
    EXPECT_EQ(realmAttack(fanren::rules::fromValue(24)), realmAttack(Realm::Mortal));
    EXPECT_EQ(realmDefence(fanren::rules::fromValue(24)), realmDefence(Realm::Mortal));
    EXPECT_EQ(realmAttack(fanren::rules::fromValue(14)), 6);
    EXPECT_EQ(realmDefence(fanren::rules::fromValue(-1)), 3);
}

}  // namespace
