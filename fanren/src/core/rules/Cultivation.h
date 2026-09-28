#pragma once
// 打坐与突破。纯逻辑，不依赖 SDL / JSON / Lua / 文件 IO。
//
// 境界阶梯与各关口所需修为已由 core/rules/Realm.h 给出，本文件只负责
// 「怎么攒」与「怎么冲」，不再另立一套境界表。
#include <cstdint>

#include "core/rules/Realm.h"

namespace fanren::rules {

// 修炼一次的产出。分开返回而不是直接改 GameState，是为了让调用方决定
// 是否接受（例如闭关中途被打断、或剧情要求这一段修为不作数）。
struct CultivationGain {
    int cultivation = 0;    // 修为增量
    int days = 0;           // 消耗天数
    bool insight = false;   // 是否顿悟（额外收益，低概率）
};

// 单次打坐的天数上限（十年）。超出部分由调用方分段调用。
//
// 这既是防脏数据（days 传 INT_MAX 会让内部逐日循环跑到天荒地老），也是玩法
// 需要：第 8 章「闭关四年」、第 11 章「六十年闭关」都要在中途插入门派急报、
// 魔道入侵这类打断事件，分段才有插入点。
inline constexpr int kMaxMeditateDays = 3600;

// effectiveness 的上限。功法、灵根、洞府灵气、聚灵阵叠满也不该超过十倍基准；
// 再高就说明合成公式出了错，夹住比让修为一夜爆表好查。
inline constexpr int kMaxEffectiveness = 1000;

// 打坐若干天。effectiveness 由功法、灵根、洞府灵气等外部因素合成后传入，
// 100 为基准；rules 层不关心它怎么来的。
//
// 注意收益粒度：炼气期升一层只要几十点修为（见 Realm::cultivationNeeded），
// 折算到每天不足 1 点。这里**不设每日保底 1 点**——保底会被玩家拆成一天
// 一次打坐刷成十倍收益。打坐的最小有意义粒度是一旬，UI 也应按旬/月/年出价。
[[nodiscard]] CultivationGain meditate(Realm realm, int aptitude, int effectiveness,
                                       int days, std::uint32_t seed);

// 这一次突破为什么**按不下去**（骰子都不摇）。None 表示按得下去——成不成是骰子的事。
//
// 第 4 章二次整改（技术债 G-14）加的第三种：**剧情给的境界上限**。它与「修为不够」
// 必须分开报：修为不够是「再坐一阵就行」，到了上限是「坐多久都不行，得等剧情」。
// 两句话说成一句，玩家会对着一个永远按不动的按钮一直打坐下去。
enum class BreakthroughBlock : std::int32_t {
    None = 0,
    SeriesMax,     // 已至本作上限（结丹后期，大纲 4.1）
    StoryCap,      // 剧情最近一次给的上限到了——瓶颈，不是运气
    NotEnough,     // 修为不够这一关的门槛
};

// 突破判定。
struct BreakthroughAttempt {
    bool success = false;
    int cultivationSpent = 0;
    int cultivationLost = 0;   // 失败时的倒扣
    bool backlash = false;     // 走火入魔（失败且重伤）
    // 追加在末尾（按字段顺序的聚合初始化不受影响）。只有 tryBreakthrough 会填它；
    // 不是 None 时上面四项全是默认值——没摇骰子，也就没有代价。
    BreakthroughBlock blocked = BreakthroughBlock::None;
};

// 成功率永远夹在 [5, 95]：留 5% 失手与 5% 侥幸。全成功或全失败都会让
// 「冲关」这个动作失去张力，这一点与炼制四艺的判定口径一致。
inline constexpr int kBreakthroughMinChance = 5;
inline constexpr int kBreakthroughMaxChance = 95;

// 失败的代价：倒扣门槛修为的两成；走火入魔再翻倍到四成。
//
// 没有代价的突破会被无脑重刷——成功率再低也只是多点几次鼠标，玩家不会
// 去炼丹、不会去找洞府、不会攒资源，整条经济循环随之失效。两成的量级是
// 按「失手一次退回大半旬的打坐」定的，肉痛但不至于劝退。
inline constexpr int kFailureLossPercent = 20;
inline constexpr int kBacklashLossPercent = 40;
inline constexpr int kBacklashChance = 15;   // 失败之后再判一次

// 当前成功率（百分数）。修为不足或已达本作上限时返回 0。
// 单列出来供 UI 显示「有几成把握」，也让平衡可以脱离随机直接单测。
[[nodiscard]] int breakthroughChance(Realm realm, int cultivation, int aptitude, int pillBonus);

// pillBonus 为丹药加成的百分点（筑基丹、结丹辅药等），无丹药传 0。
// 允许传负值，用来表达心魔、旧伤、真元被夺（第 9 章）这类减益。
//
// **这是骰子本身，不是入口。** 它不知道剧情上限；游戏层的任何突破入口都必须走下面的
// tryBreakthrough（tests/RealmCapTests.cpp 有一条扫 src/ 的用例钉着这件事）。
[[nodiscard]] BreakthroughAttempt attemptBreakthrough(Realm realm, int cultivation, int aptitude,
                                                      int pillBonus, std::uint32_t seed);

// ---------------------------------------------------------------------------
// 剧情境界上限（第 4 章二次整改，技术债 G-14；契约 docs/interfaces-p2.md 第 7 节）
// ---------------------------------------------------------------------------
//
// 玩家自己在打坐面板上按的突破，**不能超过剧情最近一次给的上限**（GameState::realmCap）。
// 上限只由剧情抬：脚本命令 realm.cap，或者 realm.advance 顺带抬到目标层。修为照常累积，
// 只是按不过去；上限一抬，攒下的修为当场就按得动。
//
// 为什么要有它：没有它的时候，章与章之间的境界不是确定值——第 4 章的奖励与日课加起来
// 够玩家自己按到炼气九层，而设计写死「本章结束时是炼气八层」；第 2 章多打坐几十次就能
// 把面板刷到第五、第六层，而师父刚说他到第三层（第 2 章复审 N5）。

// 这一关按不按得下去，按不下去是为什么。判定次序是**本作上限 → 剧情上限 → 修为**：
// 到了剧情上限时哪怕修为也不够，报的也是上限——那时说「尚差几点」是在骗人，差的不是点数。
//
// storyCap 按境界编号比：下一境界的编号大于它就算越过。编号不合法的 cap 照样按编号比，
// 不另开口子（存档读入时已经把关，见 io/SaveFile.cpp）。
[[nodiscard]] BreakthroughBlock breakthroughBlock(Realm realm, int cultivation, Realm storyCap);

// **突破的唯一入口。** 先问 breakthroughBlock，按不下去就原样返回（blocked 写明为什么，
// 一分不扣）；按得下去才交给 attemptBreakthrough 摇骰子。打坐面板与以后任何突破入口
// 都只调这一个，不许在面板里另写一份上限判定。
[[nodiscard]] BreakthroughAttempt tryBreakthrough(Realm realm, int cultivation, Realm storyCap,
                                                  int aptitude, int pillBonus,
                                                  std::uint32_t seed);

}  // namespace fanren::rules
