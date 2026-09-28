#include "core/rules/Calendar.h"

#include <limits>

namespace fanren::rules {
namespace {

// day 的合法域是 [1, INT_MAX]。存档被改、结构体忘了初始化都可能给出 0 或
// 负数，此时按第一天处理：否则 (day - 1) / kDaysPerYear 会得到负商，UI 上
// 出现「第 0 年」乃至「第 -1 月」，而这种显示错误极难回溯到源头。
[[nodiscard]] int normalized(int day) noexcept {
    return day < 1 ? 1 : day;
}

}  // namespace

int Calendar::year() const {
    return (normalized(day) - 1) / kDaysPerYear + 1;
}

int Calendar::monthOfYear() const {
    return (normalized(day) - 1) / kDaysPerMonth % kMonthsPerYear + 1;
}

int Calendar::dayOfMonth() const {
    return (normalized(day) - 1) % kDaysPerMonth + 1;
}

int advanceDays(Calendar& calendar, int days) {
    if (days <= 0) return 0;

    calendar.day = normalized(calendar.day);

    // 第 11 章「六十年闭关」这类跳时会把 day 推到很大，但仍远不到 INT_MAX。
    // 真正会溢出的只有脚本传进来的脏数据，夹住即可：溢出后 day 变负，
    // 上面所有周期判断都会反向，比少推几天危险得多。
    const int room = std::numeric_limits<int>::max() - calendar.day;
    const int step = days < room ? days : room;
    calendar.day += step;
    return step;
}

}  // namespace fanren::rules
