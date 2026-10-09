#include "core/rules/Realm.h"

#include <algorithm>
#include <array>

namespace fanren::rules {
namespace {

constexpr std::int32_t kQiMin = toValue(Realm::QiRefining1);
constexpr std::int32_t kQiMax = toValue(Realm::QiRefining13);
constexpr std::int32_t kFoundationMin = toValue(Realm::FoundationEarly);
constexpr std::int32_t kFoundationMax = toValue(Realm::FoundationLate);
constexpr std::int32_t kCoreMin = toValue(Realm::CoreEarly);
constexpr std::int32_t kCoreMax = toValue(Realm::CoreLate);

constexpr std::array<std::string_view, 13> kQiNames{
    "炼气一层", "炼气二层", "炼气三层", "炼气四层", "炼气五层",
    "炼气六层", "炼气七层", "炼气八层", "炼气九层", "炼气十层",
    "炼气十一层", "炼气十二层", "炼气十三层",
};

// ---- 境界给出的气血 / 法力基准（口径与证据见 Realm.h）----
//
// 炼气期写成 截距 + 斜率 × 层数 而不是一张十三行的表：表能被逐行改成任何形状，
// 于是「曲线本身是不是单调、是不是一条线」就没人守着了；写成两个常数，那两条
// 性质在类型上就是真的，单测只需再钉住它落在 data 的哪几个点上。
constexpr std::int32_t kMortalMaxHp = 10;
constexpr std::int32_t kQiHpIntercept = 24;
constexpr std::int32_t kQiHpPerLevel = 12;
constexpr std::int32_t kQiMpPerLevel = 10;

// ---- 境界给出的攻 / 防基准（口径、证据与逐层取值表见 Realm.h）----
//
// 与气血那一组同一个写法：炼气期写成常数 + 斜率，不写一张十三行的表。
// 表能被逐行改成任何形状，于是「曲线是不是单调、是不是一条线」就没人守着了。
//
// 攻的定义是 `max(6, round(0.6 + 1.8 × 层数))`。这里用整数算，不用 double：
// round(x) = floor(x + 0.5)，于是 round(0.6 + 1.8n) = floor((11 + 18n) / 10)。
// 炼气三层、八层、十三层三处的括号里恰好是整十（60 / 150 / 240），
// 浮点算法在这三处要靠 lround 的舍入方向才落对地方——而这三处偏偏是锚点与
// 表里加粗的那几格。整数除法没有这个悬念。
constexpr std::int32_t kMortalAttack = 6;
constexpr std::int32_t kMortalDefence = 3;
constexpr std::int32_t kQiAttackRoundedIntercept = 11;   // (0.6 + 0.5) × 10
constexpr std::int32_t kQiAttackPerLevelTenths = 18;     // 1.8 × 10
constexpr std::int32_t kQiAttackTenths = 10;
// 防的定义是 `max(3, 层数)`：未锚定的线是 4 + 层数，减去锚定位移 4 之后
// 截距归零，于是整条线就是层数本身。
constexpr std::int32_t kQiDefencePerLevel = 1;

// 同一大境界内的层数序号，0 起算。非法编号返回 -1。
[[nodiscard]] std::int32_t indexWithinTier(Realm realm) noexcept {
    const std::int32_t v = toValue(realm);
    if (v >= kQiMin && v <= kQiMax) return v - kQiMin;
    if (v >= kFoundationMin && v <= kFoundationMax) return v - kFoundationMin;
    if (v >= kCoreMin && v <= kCoreMax) return v - kCoreMin;
    return v == 0 ? 0 : -1;
}

}  // namespace

bool isValid(Realm realm) noexcept {
    const std::int32_t v = toValue(realm);
    return v == 0 || (v >= kQiMin && v <= kQiMax) || (v >= kFoundationMin && v <= kFoundationMax) ||
           (v >= kCoreMin && v <= kCoreMax);
}

RealmTier tierOf(Realm realm) noexcept {
    const std::int32_t v = toValue(realm);
    if (v >= kCoreMin && v <= kCoreMax) return RealmTier::Core;
    if (v >= kFoundationMin && v <= kFoundationMax) return RealmTier::Foundation;
    if (v >= kQiMin && v <= kQiMax) return RealmTier::QiRefining;
    return RealmTier::Mortal;
}

std::string_view nameOf(Realm realm) noexcept {
    const std::int32_t v = toValue(realm);
    if (v == 0) return "凡人";
    if (v >= kQiMin && v <= kQiMax) {
        return kQiNames[static_cast<std::size_t>(v - kQiMin)];
    }
    switch (realm) {
        case Realm::FoundationEarly: return "筑基初期";
        case Realm::FoundationMid:   return "筑基中期";
        case Realm::FoundationLate:  return "筑基后期";
        case Realm::CoreEarly:       return "结丹初期";
        case Realm::CoreMid:         return "结丹中期";
        case Realm::CoreLate:        return "结丹后期";
        default:                     return "未知";
    }
}

bool tryNext(Realm realm, Realm& out) noexcept {
    if (!isValid(realm)) return false;
    const std::int32_t v = toValue(realm);

    if (v == 0) {
        out = Realm::QiRefining1;
        return true;
    }
    if (v >= kQiMin && v < kQiMax) {
        out = fromValue(v + 1);
        return true;
    }
    if (v == kQiMax) {
        out = Realm::FoundationEarly;
        return true;
    }
    if (v >= kFoundationMin && v < kFoundationMax) {
        out = fromValue(v + 1);
        return true;
    }
    if (v == kFoundationMax) {
        out = Realm::CoreEarly;
        return true;
    }
    if (v >= kCoreMin && v < kCoreMax) {
        out = fromValue(v + 1);
        return true;
    }
    // 结丹后期是本作上限（大纲 4.1 硬约束）。
    return false;
}

std::int32_t cultivationNeeded(Realm realm) noexcept {
    if (!isValid(realm)) return -1;
    const std::int32_t v = toValue(realm);

    // 曲线要求：全程严格单调递增，且在两个大境界关口（炼气圆满→筑基、
    // 筑基后期→结丹）出现约 2 倍的跃升，让玩家明确感到"卡关"。
    //
    // 原型 fanren-kys 的数值直接迁移过来是断裂的：凡人→炼气一层要 20，
    // 而炼气一层→二层只要 13；炼气十二层要 442，十三层反而只要 300。
    // 单调性单测第一时间抓到了这两处倒挂，此处按新曲线重排。
    if (v == 0) return 20;                      // 凡人感气
    if (v >= kQiMin && v < kQiMax) {
        // 炼气各层平方增长：23, 32, 47 ... 452。
        return 20 + v * v * 3;
    }
    if (v == kQiMax) return 900;                // 炼气圆满 → 筑基（约 2 倍跃升）

    switch (realm) {
        case Realm::FoundationEarly: return 1200;
        case Realm::FoundationMid:   return 1800;
        case Realm::FoundationLate:  return 3500;   // 筑基后期 → 结丹（约 2 倍跃升）
        case Realm::CoreEarly:       return 5000;
        case Realm::CoreMid:         return 7000;
        default:                     return -1;     // 结丹后期为本作上限
    }
}

DemoteCheck checkDemote(Realm current, Realm target) noexcept {
    if (!isValid(current) || !isValid(target) || target == Realm::Mortal) return DemoteCheck::NoRealm;
    if (target == current) return DemoteCheck::AlreadyThere;
    if (toValue(target) > toValue(current)) return DemoteCheck::NotLower;
    return DemoteCheck::Ok;
}

Realm demote(Realm realm, std::int32_t levels) noexcept {
    if (!isValid(realm) || levels <= 0) return realm;

    std::int32_t remaining = levels;
    Realm current = realm;

    while (remaining > 0) {
        const std::int32_t v = toValue(current);

        if (v > kCoreMin && v <= kCoreMax) {
            current = fromValue(v - 1);
        } else if (v == kCoreMin) {
            current = Realm::FoundationLate;
        } else if (v > kFoundationMin && v <= kFoundationMax) {
            current = fromValue(v - 1);
        } else if (v == kFoundationMin) {
            current = Realm::QiRefining13;
        } else if (v > kQiMin && v <= kQiMax) {
            current = fromValue(v - 1);
        } else {
            // 已到炼气一层：根基仍在，不会跌回凡人。
            break;
        }
        --remaining;
    }
    return current;
}

double suppressionFactor(Realm attacker, Realm defender) noexcept {
    if (!isValid(attacker) || !isValid(defender)) return 1.0;

    const auto attackerTier = static_cast<std::int32_t>(tierOf(attacker));
    const auto defenderTier = static_cast<std::int32_t>(tierOf(defender));
    const std::int32_t tierGap = attackerTier - defenderTier;

    if (tierGap != 0) {
        // 跨大境界压制显著，但夹逼在 [0.4, 2.2] 内，避免高境界一击必杀
        // 让战棋失去博弈空间。
        const double raw = tierGap > 0 ? 1.0 + 0.6 * tierGap : 1.0 + 0.3 * tierGap;
        return std::clamp(raw, 0.4, 2.2);
    }

    // 同档内按层数差微调。
    const std::int32_t levelGap = indexWithinTier(attacker) - indexWithinTier(defender);
    return std::clamp(1.0 + 0.04 * levelGap, 0.7, 1.4);
}

std::int32_t realmMaxHp(Realm realm) noexcept {
    if (!isValid(realm)) return kMortalMaxHp;
    const std::int32_t v = toValue(realm);
    // 凡人保持 GameState 的默认值，一字不差（理由见头文件）。
    if (v == 0) return kMortalMaxHp;
    // 炼气期是一条直线，同时穿过 data/roles 的炼气四层 / 十一层 / 十三层三个点。
    if (v >= kQiMin && v <= kQiMax) return kQiHpIntercept + kQiHpPerLevel * v;
    switch (realm) {
        case Realm::FoundationEarly: return 260;
        case Realm::FoundationMid:   return 380;
        case Realm::FoundationLate:  return 480;
        case Realm::CoreEarly:       return 700;
        case Realm::CoreMid:         return 900;
        case Realm::CoreLate:        return 1300;
        default:                     return kMortalMaxHp;
    }
}

std::int32_t realmMaxMp(Realm realm) noexcept {
    if (!isValid(realm)) return 0;
    const std::int32_t v = toValue(realm);
    // 凡人没有法力，与 GameState 的默认值一致：他还不是修士。
    if (v == 0) return 0;
    if (v >= kQiMin && v <= kQiMax) return kQiMpPerLevel * v;
    switch (realm) {
        case Realm::FoundationEarly: return 180;
        case Realm::FoundationMid:   return 250;
        case Realm::FoundationLate:  return 320;
        case Realm::CoreEarly:       return 470;
        case Realm::CoreMid:         return 620;
        case Realm::CoreLate:        return 860;
        default:                     return 0;
    }
}

std::int32_t realmAttack(Realm realm) noexcept {
    if (!isValid(realm)) return kMortalAttack;
    const std::int32_t v = toValue(realm);
    // 凡人保持写死的现值，一字不差（理由见头文件）。
    if (v == 0) return kMortalAttack;
    if (v >= kQiMin && v <= kQiMax) {
        const std::int32_t raw =
            (kQiAttackRoundedIntercept + kQiAttackPerLevelTenths * v) / kQiAttackTenths;
        // 下夹在凡人值上：锚定之后炼气一、二层的线性值落在 6 以下，而那两层
        // 在第 1-3 章里就是 6——曲线可以从锚点往下延，三章的数值不行。
        return std::max(kMortalAttack, raw);
    }
    switch (realm) {
        case Realm::FoundationEarly: return 32;   // 第 8 章六战标定，保留原值
        case Realm::FoundationMid:   return 37;   // 第 8 章六战标定，保留原值
        case Realm::FoundationLate:  return 44;   // 暂定
        case Realm::CoreEarly:       return 62;   // 暂定
        case Realm::CoreMid:         return 76;   // 暂定
        case Realm::CoreLate:        return 102;  // 暂定
        default:                     return kMortalAttack;
    }
}

std::int32_t realmDefence(Realm realm) noexcept {
    if (!isValid(realm)) return kMortalDefence;
    const std::int32_t v = toValue(realm);
    if (v == 0) return kMortalDefence;
    if (v >= kQiMin && v <= kQiMax) {
        return std::max(kMortalDefence, kQiDefencePerLevel * v);
    }
    switch (realm) {
        case Realm::FoundationEarly: return 20;   // 第 8 章六战标定，保留原值
        case Realm::FoundationMid:   return 26;   // 第 8 章六战标定，保留原值
        case Realm::FoundationLate:  return 30;   // 暂定
        case Realm::CoreEarly:       return 46;   // 暂定
        case Realm::CoreMid:         return 58;   // 暂定
        case Realm::CoreLate:        return 82;   // 暂定
        default:                     return kMortalDefence;
    }
}

Vitals liftToFloor(Vitals v, std::int32_t floor) noexcept {
    // 只补不削：上限已经够高就一个字节都不动，连当前值也不夹——夹了的话，
    // 一份「上限 60、当前 61」的脏存档会在这里被悄悄改写，而这条函数的职责
    // 只是补齐境界那一截，不是给存档做体检。
    if (floor <= v.max) return v;
    const std::int32_t gain = floor - v.max;
    v.max = floor;
    // 补上去的那一截是根基，不是伤势：当前值同步抬高相同的量。
    v.current += gain;
    if (v.current > v.max) v.current = v.max;
    return v;
}

}  // namespace fanren::rules
