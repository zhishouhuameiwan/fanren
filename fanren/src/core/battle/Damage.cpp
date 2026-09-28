#include "core/battle/Damage.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

#include "core/battle/Battle.h"   // kDevourBiteDivisor / kFleeBitten*

namespace fanren::core::battle {
namespace {

// 相克对：前者克后者。
constexpr std::array<std::pair<int, int>, 5> kBeats{{
    {kElementMetal, kElementWood},
    {kElementWood, kElementEarth},
    {kElementEarth, kElementWater},
    {kElementWater, kElementFire},
    {kElementFire, kElementMetal},
}};

[[nodiscard]] bool restrains(int a, int b) noexcept {
    for (const auto& [winner, loser] : kBeats) {
        if ((a & winner) != 0 && (b & loser) != 0) return true;
    }
    return false;
}

// 基础伤害先夹到 1 以上再乘系数：破防时基数为负，若先乘再夹，
// 境界压制与五行系数会被 max(1) 整个吞掉，玩家看不出强弱差别。
[[nodiscard]] int finish(double base, rules::Realm attackerRealm, rules::Realm defenderRealm,
                         int attackerElement, int defenderElement) noexcept {
    if (base < 1.0) base = 1.0;
    base *= rules::suppressionFactor(attackerRealm, defenderRealm);
    base *= elementFactor(attackerElement, defenderElement);
    const auto rounded = static_cast<int>(std::lround(base));
    return rounded < 1 ? 1 : rounded;
}

}  // namespace

double elementFactor(int attackerElement, int defenderElement) noexcept {
    if (attackerElement == kElementNone || defenderElement == kElementNone) return 1.0;

    const bool forward = restrains(attackerElement, defenderElement);
    const bool backward = restrains(defenderElement, attackerElement);
    if (forward == backward) return 1.0;   // 都不克或互克：相持
    return forward ? kRestrainBonus : kRestrainPenalty;
}

int physicalDamage(int attack, int defence, rules::Realm attackerRealm,
                   rules::Realm defenderRealm, int attackerElement,
                   int defenderElement) noexcept {
    const double base = attack * 2.0 - defence;
    return finish(base, attackerRealm, defenderRealm, attackerElement, defenderElement);
}

int magicDamage(int power, int defence, rules::Realm attackerRealm, rules::Realm defenderRealm,
                int magicElement, int defenderElement) noexcept {
    const double base = power * 2.0 - defence * 0.5;
    return finish(base, attackerRealm, defenderRealm, magicElement, defenderElement);
}

int devourBite(int attackerMaxHp) noexcept {
    if (attackerMaxHp <= 0) return 1;
    return std::max(1, attackerMaxHp / kDevourBiteDivisor);
}

bool devourShouldFlee(int currentMaxHp, int startMaxHp) noexcept {
    if (startMaxHp <= 0) return false;
    // 「剩下的不足三分之二」写成整数乘法，避免浮点在边界上左右横跳：
    // 恰好剩三分之二时还不跑，再咬一口才跑。
    const int remainingNumerator = kFleeBittenDenominator - kFleeBittenNumerator;   // 2
    return currentMaxHp * kFleeBittenDenominator < startMaxHp * remainingNumerator;
}

bool devourCanBreakAway(int targetStartMaxHp, int attackerStartMaxHp) noexcept {
    if (attackerStartMaxHp <= 0) return false;
    return targetStartMaxHp > attackerStartMaxHp;
}

int devourBittenPercent(int currentMaxHp, int startMaxHp) noexcept {
    if (startMaxHp <= 0) return 0;
    const int remaining = std::clamp(currentMaxHp, 0, startMaxHp);
    const double bitten = static_cast<double>(startMaxHp - remaining) / startMaxHp;
    return static_cast<int>(std::lround(bitten * 100.0));
}

}  // namespace fanren::core::battle
