#include "core/rules/Cultivation.h"

#include <algorithm>
#include <limits>
#include <random>

namespace fanren::rules {
namespace {

// 内部按「万分之一修为」累加。
//
// 直接按天取整会让炼气期的日收益永远归零（升一层只要几十点，摊到每天是
// 0.1 量级），累加完再除回来才既能表达小数收益，又不给拆分刷收益的空子。
constexpr long long kUnitScale = 10000;

// 顿悟一次相当于白得多少天的功。给 30 天是因为它要在玩家的记忆里留下痕迹
// ——少于一旬的话，顿悟提示弹出来而数字没动，比不给还糟。
constexpr int kInsightDaysWorth = 30;

// 每百天的基准修为，按大境界给（资质 50、effectiveness 100 时的值）。
//
// 数字的来源是让境界曲线落在原著的时间尺度上：凡人引气 20 点，约 250 天，
// 对得上第 1-2 章；筑基后期冲结丹 3500 点，在 effectiveness 拉满前要几十年，
// 对得上第 11 章的「六十年闭关」。境界越高日收益越大，但所需修为涨得更快，
// 净效果仍是越往上越慢——这正是修仙小说的时间感。
[[nodiscard]] int baseRatePer100Days(RealmTier tier) noexcept {
    switch (tier) {
        case RealmTier::Mortal:     return 8;
        case RealmTier::QiRefining: return 12;
        case RealmTier::Foundation: return 30;
        case RealmTier::Core:       return 60;
    }
    return 8;
}

// 大境界关口的基准成功率。这两个数字直接决定了「筑基」「结丹」在玩家
// 记忆里的分量：小境界随手过，大关口要备丹药、要挑洞府、要挑日子。
[[nodiscard]] int baseChanceFor(Realm realm) noexcept {
    Realm next{};
    if (!tryNext(realm, next)) return 0;

    const RealmTier from = tierOf(realm);
    const RealmTier to = tierOf(next);
    if (from == RealmTier::Mortal) return 85;          // 凡人感气，剧情必过
    if (from == to) return 60;                          // 同档内升层
    if (to == RealmTier::Foundation) return 25;         // 炼气圆满 → 筑基
    return 15;                                          // 筑基后期 → 结丹
}

[[nodiscard]] int saturateToInt(long long value) noexcept {
    constexpr long long kMax = std::numeric_limits<int>::max();
    return static_cast<int>(value < kMax ? value : kMax);
}

}  // namespace

bool reclimbing(Realm current, Realm former) noexcept {
    if (!isValid(current) || !isValid(former)) return false;
    if (former == Realm::Mortal || current == Realm::Mortal) return false;
    return toValue(current) < toValue(former);
}

CultivationGain meditate(Realm realm, int aptitude, int effectiveness, int days,
                         std::uint32_t seed) {
    CultivationGain gain;
    if (!isValid(realm) || days <= 0) return gain;

    const int spent = std::min(days, kMaxMeditateDays);
    gain.days = spent;

    const int apt = std::clamp(aptitude, 0, 100);
    const int eff = std::clamp(effectiveness, 0, kMaxEffectiveness);

    // effectiveness <= 0：功法不契或洞府无灵气，坐穿蒲团也是白坐。天数照扣
    // ——时间是真的过去了，这一点必须让玩家吃到，否则「换个洞府」就没有
    // 任何紧迫性可言。
    if (eff == 0) return gain;

    // 资质 0..100 映射到 50%..150%：伪灵根（韩立，大纲 4.1）慢一截但不至于
    // 修不动，天灵根快一截但不至于一骑绝尘——差距靠时间尺度体现，不靠倍率。
    const long long perDay =
        static_cast<long long>(baseRatePer100Days(tierOf(realm))) * (50 + apt) * eff / 100;
    long long units = perDay * spent;

    // 顿悟逐日独立判定，命中次数越多加成越多。整段只判一次的话，玩家把
    // 一次长闭关拆成十次短打坐就能多摇十次骰子；逐日判定则拆不出便宜。
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> roll(0, 9999);
    const int insightOdds = 20 + apt / 2;   // 万分之 20..70
    for (int d = 0; d < spent; ++d) {
        if (roll(rng) < insightOdds) {
            gain.insight = true;
            units += perDay * kInsightDaysWorth;
        }
    }

    gain.cultivation = saturateToInt(units / kUnitScale);
    return gain;
}

int breakthroughChance(Realm realm, int cultivation, int aptitude, int pillBonus) {
    const int need = cultivationNeeded(realm);
    if (need < 0 || cultivation < need) return 0;

    int chance = baseChanceFor(realm);
    chance += std::clamp(aptitude, 0, 100) / 2;      // 资质最多 +50
    // 负的 pillBonus 用来表达心魔、旧伤、被夺真元（第 9 章）这类减益。夹住
    // 而不是丢弃：调用方传进来的惩罚如果被悄悄吞掉，平衡就无从下手。
    chance += std::clamp(pillBonus, -100, 100);

    // 修为超出门槛越多越稳，最多 +20。鼓励玩家多攒一阵再冲，而不是一到线
    // 就赌：这一项是「闭关」这个玩法存在的理由。
    const long long excess = static_cast<long long>(cultivation) - need;
    chance += static_cast<int>(std::min<long long>(20, excess * 20 / need));

    return std::clamp(chance, kBreakthroughMinChance, kBreakthroughMaxChance);
}

BreakthroughAttempt attemptBreakthrough(Realm realm, int cultivation, int aptitude, int pillBonus,
                                        std::uint32_t seed) {
    BreakthroughAttempt attempt;

    const int need = cultivationNeeded(realm);
    // 已达本作上限（结丹后期，大纲 4.1）或境界编号非法：连试都不该试。
    if (need < 0) return attempt;
    // 修为不够不算一次尝试，一分不扣。否则「点了没反应还掉修为」在 UI 上
    // 无从解释，玩家只会认为是 bug。
    if (cultivation < need) return attempt;

    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> roll(0, 99);

    // 成功判定必须是同一种子下的第一次取数：这样提高成功率只可能把失败翻成
    // 成功，绝不会反过来。「嗑了丹药反而炸炉」是玩家最不能接受的随机。
    const int chance = breakthroughChance(realm, cultivation, aptitude, pillBonus);
    if (roll(rng) < chance) {
        attempt.success = true;
        attempt.cultivationSpent = need;
        return attempt;
    }

    attempt.backlash = roll(rng) < kBacklashChance;
    const int lossPercent = attempt.backlash ? kBacklashLossPercent : kFailureLossPercent;
    const long long loss = static_cast<long long>(need) * lossPercent / 100;
    // 倒扣不会把修为扣成负数：负修为会让下一次 breakthroughChance 的除法
    // 与 UI 进度条同时失去意义。
    attempt.cultivationLost = static_cast<int>(std::min<long long>(loss, cultivation));
    return attempt;
}

BreakthroughBlock breakthroughBlock(Realm realm, int cultivation, Realm storyCap) {
    Realm next{};
    // 本作上限（或编号非法）：没有下一关可言，上限与修为都无从谈起。
    if (!tryNext(realm, next)) return BreakthroughBlock::SeriesMax;
    // 剧情上限先于修为：到了瓶颈时修为也不够，说「尚差几点」就是在骗人。
    if (toValue(next) > toValue(storyCap)) return BreakthroughBlock::StoryCap;
    if (cultivation < cultivationNeeded(realm)) return BreakthroughBlock::NotEnough;
    return BreakthroughBlock::None;
}

BreakthroughAttempt tryBreakthrough(Realm realm, int cultivation, Realm storyCap, int aptitude,
                                    int pillBonus, std::uint32_t seed) {
    const BreakthroughBlock blocked = breakthroughBlock(realm, cultivation, storyCap);
    if (blocked != BreakthroughBlock::None) {
        // 按不下去就连骰子都不摇：这不算一次尝试，一分不扣。
        BreakthroughAttempt refused;
        refused.blocked = blocked;
        return refused;
    }
    return attemptBreakthrough(realm, cultivation, aptitude, pillBonus, seed);
}

}  // namespace fanren::rules
