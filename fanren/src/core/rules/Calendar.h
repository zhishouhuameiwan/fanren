#pragma once
// 游戏内日历。纯逻辑，不依赖 SDL / JSON / Lua / 文件 IO。
//
// 一切周期玩法（小瓶凝液、灵草生长、坊市补货、拍卖档期、秘境冷却）都按
// 「绝对天数」推进，年月日只是显示用的换算。存档里存的是 day 这一个整数：
// 周期结算于是永远只是一次减法与一次除法，不必处理进位，也不会因为月份
// 长短不一而漂移。原型 fanren-kys 把年/月/日各存一份，跨年补货的判断散在
// 三个字段上，改一处就漏一处。
namespace fanren::rules {

// 一年 360 天的理想历。真实农历的闰月对玩法没有任何贡献，只会让「每 7 天
// 凝一滴」这类周期出现无法解释的抖动。
inline constexpr int kDaysPerMonth = 30;
inline constexpr int kMonthsPerYear = 12;
inline constexpr int kDaysPerYear = kDaysPerMonth * kMonthsPerYear;

struct Calendar {
    int day = 1;   // 从 1 起算的绝对天数

    [[nodiscard]] int year() const;          // 第几年，从 1 起算
    [[nodiscard]] int monthOfYear() const;   // 1..12
    [[nodiscard]] int dayOfMonth() const;    // 1..30
};

// 推进 days 天，返回实际跨过的天数（便于调用方批量结算周期事件）。
//
// days <= 0 一律无效果并返回 0。倒退时间会把所有「上次结算日」推到未来，
// 凝液、灵田、补货会集体停摆到追平为止——这种错误在存档里是不可见的，
// 因此宁可在入口处忽略，也不允许时间回头。
int advanceDays(Calendar& calendar, int days);

}  // namespace fanren::rules
