#pragma once
// 伤害与五行相克。
//
// 刻意做成不依赖 BattleState 的纯函数：系数是全局平衡里最容易出错的一环，
// 单测能直接逐项校验，不必先摆一个战场出来。
#include "core/model/Types.h"
#include "core/rules/Realm.h"

namespace fanren::core::battle {

inline constexpr double kRestrainBonus = 1.3;     // 克制方加伤
inline constexpr double kRestrainPenalty = 0.8;   // 被克方减伤

// 五行相克系数：金克木、木克土、土克水、水克火、火克金。
//
// element 是 bitmask，可能同时带多个属性。旧原型按固定表逐行扫描、谁先匹配上
// 谁生效，双方互克时结果取决于表里的行序——同一对单位换个写法就变了系数。
// 这里改为先分别求出两个方向是否成立：只有单向成立才给系数，互克判相持。
[[nodiscard]] double elementFactor(int attackerElement, int defenderElement) noexcept;

// 物理伤害。防御只抵消一半攻击力（攻击先翻倍），保证高防单位不会完全免伤，
// 也保证攻防同档时伤害仍随属性与境界差拉开。
[[nodiscard]] int physicalDamage(int attack, int defence, rules::Realm attackerRealm,
                                 rules::Realm defenderRealm, int attackerElement,
                                 int defenderElement) noexcept;

// 法术伤害。走神识路线，只被半数防御抵消；五行取法术自身属性而非施法者属性。
[[nodiscard]] int magicDamage(int power, int defence, rules::Realm attackerRealm,
                              rules::Realm defenderRealm, int magicElement,
                              int defenderElement) noexcept;

// ---- 识海之战：吞噬（P3 第 3 章，契约第 3 节）----
//
// 这三条同样做成纯函数：倍率、阈值、战果口径是这一场里全部的数值，
// 单测能逐项校验，不必先摆一个识海出来。

// 一口咬多深。识海里没有兵器也没有护甲，咬下多少只取决于谁大——
// 攻防与五行在这里一概不参与，否则就是在「体积即实力」之外又造了一套数值。
// 至少咬下 1：体积再小也还咬得动，否则两团小光球会互相磨到天荒地老。
[[nodiscard]] int devourBite(int attackerMaxHp) noexcept;

// 体积掉到开战时的三分之二以下就脱身逃走。
// startMaxHp <= 0 一律判 false：没有底数就没有「少了三分之一」可言，
// 让一个塌成 0 的分母把每个单位都判成该逃，是这类判据最常见的空转法。
[[nodiscard]] bool devourShouldFlee(int currentMaxHp, int startMaxHp) noexcept;

// 逃得掉逃不掉，先看进来时谁大：**只有开战体积大过吞噬者的那一团才脱得开身。**
//
// 这一条直接来自原著的两场：黄光球比韩立小好几倍，一口就是它的四分之一，
// 第二口下去已经不成形，根本谈不上「把被咬住的那块脱开继续跑」，于是被整团吞掉；
// 绿光球比韩立大一圈，舍掉一块仍剩得下自己，所以它跑得了。
// 没有这一条，上面那个三分之一的阈值会把小的那团也放跑，第一场就成不立了。
[[nodiscard]] bool devourCanBreakAway(int targetStartMaxHp, int attackerStartMaxHp) noexcept;

// 战果：被咬掉的体积百分比（0-100，四舍五入）。脚本按它写旗标。
// 同样在 startMaxHp <= 0 时返回 0，不让分母塌成 0 的除法悄悄给出一个大数。
[[nodiscard]] int devourBittenPercent(int currentMaxHp, int startMaxHp) noexcept;

}  // namespace fanren::core::battle
