#include "core/rules/Bottle.h"

#include <algorithm>
#include <limits>

namespace fanren::rules {
namespace {

// 计价曲线的偏移量。取 10 有两个作用：age 为 0 时价格正好等于基价（分母也是
// 10 的平方），以及让「十年一档」成为玩家能口算的参照——十年 4 倍、百年 121 倍。
constexpr long long kPriceOffset = 10;

[[nodiscard]] int saturateToInt(long long value) noexcept {
    constexpr long long kMax = std::numeric_limits<int>::max();
    return static_cast<int>(value < kMax ? value : kMax);
}

}  // namespace

int herbMaxAgeForGrade(int grade) {
    // 下标即品阶，0 与 1 同档（没写品阶的老数据按一阶处理，宁可低估也不留
    // 「无上限」的口子）；超出表尾的品阶一律按表尾。阶梯本身与取值的理由
    // 见 Bottle.h 上方的注释，改数字要连那张表一起改。
    static constexpr int kLadder[] = {44, 44, 100, 300, 800, 2000, 4000, 7000, kMaxHerbAge};
    constexpr int kLastIndex = static_cast<int>(sizeof(kLadder) / sizeof(kLadder[0])) - 1;

    const int index = std::clamp(grade, 0, kLastIndex);
    return kLadder[index];
}

int refill(Bottle& bottle, int currentDay, int chargeDays) {
    if (!bottle.owned) return 0;
    // chargeDays 由调用方按境界折算，除零会直接崩在玩家的存档读取里。
    if (chargeDays <= 0) return 0;

    // 满瓶时把计时拨到当天，连零头一起作废。这一步才是上限真正生效的地方：
    // 只夹住 drops 的话，满瓶那段日子会以欠账的形式留着，玩家一用掉绿液就
    // 被瞬间追认补满，挂机刷钱的口子等于没堵。
    const int capacity = std::max(bottle.capacity, 0);
    if (bottle.drops >= capacity) {
        if (currentDay > bottle.lastChargeDay) bottle.lastChargeDay = currentDay;
        return 0;
    }

    const long long elapsed = static_cast<long long>(currentDay) - bottle.lastChargeDay;
    if (elapsed < chargeDays) return 0;

    const long long cycles = elapsed / chargeDays;
    const long long gained = std::min(cycles, static_cast<long long>(capacity) - bottle.drops);
    bottle.drops = saturateToInt(static_cast<long long>(bottle.drops) + gained);

    if (gained < cycles) {
        // 中途就灌满了：余下的周期连同零头一并作废，同上。
        bottle.lastChargeDay = currentDay;
    } else {
        // 只推进整周期，余下的零头留在账上：否则每天点一次「查看瓶子」就会把
        // 未满周期的进度反复清零，瓶子永远凝不出第二滴。
        bottle.lastChargeDay += static_cast<int>(gained * chargeDays);
    }
    return static_cast<int>(gained);
}

bool spendDrops(Bottle& bottle, int count) {
    if (count <= 0) return true;
    // 没瓶子时 drops 本来就是 0，这里不必单判——不够就是不够，理由由调用方
    // 按自己的语境去说（催熟走 matureHerb 的四条理由，倒液则只有够与不够）。
    if (bottle.drops < count) return false;
    bottle.drops -= count;
    return true;
}

MatureResult matureHerb(Bottle& bottle, int currentAge, int maxAge) {
    MatureResult result;
    result.newAge = currentAge;

    // 五个失败原因刻意分开：玩家在「绿瓶四年」里会依次撞上前两条，提示文案
    // 本身就是叙事的一部分，合并成一句「无法催熟」会把这段解锁过程抹掉。
    // reason 与 failure 一律成对回填：只填一个的那条路迟早被调用方看漏。
    if (!bottle.owned) {
        result.reason = "尚未得到那只小瓶";
        result.failure = MatureFailure::NoBottle;
        return result;
    }
    if (!bottle.matureKnown) {
        result.reason = "尚不知瓶中绿液有催熟之能";
        result.failure = MatureFailure::MatureUnknown;
        return result;
    }
    if (bottle.drops <= 0) {
        result.reason = "瓶中绿液不足";
        result.failure = MatureFailure::NoDrops;
        return result;
    }

    // 未足年的苗浇不进去。理由与药圃管事当着玩家说的那条规矩是同一条
    // （「不足一年的不准动手，采下来就是废的」）；判据也与采收共用 kRipeAge，
    // 免得同一株药在两个动作上得到两种说法。
    //
    // 这一拦同时把「种下就浇」这条劣化路线堵死：没有它，多花一滴绿液就能省掉
    // 整整一年的生长，于是「等足年」永远不值得做，灵田的槽位数与生长时间一起
    // 失去意义（详见 Bottle.h 上 matureHerb 的说明）。
    if (currentAge < kRipeAge) {
        result.reason = "尚未足年，此时催它也是白费";
        result.failure = MatureFailure::NotRipe;
        return result;
    }

    const int ceiling = std::clamp(maxAge, 0, kMaxHerbAge);
    if (currentAge >= ceiling) {
        result.reason = "此草年份已至极限，绿液再多也无用";
        result.failure = MatureFailure::AtMaxAge;
        return result;
    }

    const int base = std::max(currentAge, 0);
    const long long jump = static_cast<long long>(base) + std::max(base, kMatureMinYears);

    // 扣液走 spendDrops，不在这里另写一次减法：上面刚判过 drops > 0，这一句
    // 因此必定成立，但「判断」与「扣除」分散在两处正是漏洞的长相，宁可多一次
    // 函数调用。真返回了 false 就说明前面的判断与它对不上，此时一年也不涨。
    if (!spendDrops(bottle, 1)) {
        result.reason = "瓶中绿液不足";
        result.failure = MatureFailure::NoDrops;
        return result;
    }
    result.ok = true;
    result.newAge = static_cast<int>(std::min<long long>(jump, ceiling));
    return result;
}

int herbPrice(int basePrice, int age) {
    if (basePrice <= 0) return 0;

    const long long years = std::clamp(age, 0, kMaxHerbAge) + kPriceOffset;
    const long long price =
        static_cast<long long>(basePrice) * years * years / (kPriceOffset * kPriceOffset);
    return saturateToInt(price);
}

}  // namespace fanren::rules
