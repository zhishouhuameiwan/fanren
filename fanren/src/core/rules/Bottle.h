#pragma once
// 掌天瓶。纯逻辑，不依赖 SDL / JSON / Lua / 文件 IO。
//
// 全作的经济中枢：凝液 → 催熟灵草 → 炼丹/出售 → 资源 → 修为。
// 原著里瓶子先于催熟能力被发现，两者相隔四年，是两个独立的解锁节点
// （大纲第 2 章「绿瓶四年」：ch10-14 拾瓶，ch22-26 才误洒绿液发现催熟）。
// 因此 owned 与 matureKnown 必须是两个开关，合并成一个会把整章的节奏抹平。
#include <string>

// 催熟要查「足年没有」，而「足年」这条线定义在灵田那边（kRipeAge）。
// 在这里复制一份常量迟早会与那边分家，而分家的样子是采收与催熟对同一株药
// 给出两种说法。Field.h 不反过来依赖本文件，没有环。
#include "core/rules/Field.h"
// 容量下限按大境界分档（bottleCapacityFloor）。Realm.h 不依赖本文件，同样没有环。
#include "core/rules/Realm.h"

namespace fanren::rules {

struct Bottle {
    bool owned = false;
    bool matureKnown = false;   // 是否已发现催熟之能
    int drops = 0;              // 当前绿液滴数
    int lastChargeDay = 0;      // 上次结算凝液的绝对天数
    int capacity = 3;           // 绿液容量上限（追加在末尾，不破坏聚合初始化）
};

// 每多少天凝一滴。境界越高越快，这里只给基准，倍率由调用方按境界传入。
// 基准取 7：拾瓶当日起算，第 8 天正好得到第一滴，对上原著「第八日瓶盖方开，
// 内有一滴绿液」。
inline constexpr int kBaseChargeDays = 7;

// 每个大境界档位的建议容量：炼气 3、筑基 6、结丹 9。
//
// 容量上限本身由 game 层按境界写进 bottle.capacity，rules 层不写死——剧情
// 道具、洞府灵脉都可能再给它加成。这里只给一个具名基准，免得各处各拍一个数。
//
// 这道闸不是平衡微调而是堵漏：没有上限时挂机一百年能凝出五千余滴，配上
// herbPrice 的 121 倍超线性曲线就是无技巧的无限刷钱。
inline constexpr int kBottleCapacityPerTier = 3;

// 瓶子容量随大境界的下限（契约 docs/interfaces-p3-ch07.md 第 3 节）：凡人与炼气 3、筑基 6、
// 结丹 9，即 kBottleCapacityPerTier × 档位；更高的档按结丹算（本作到结丹为止，用不上）。
//
// 上面那句「rules 层不写死」的意思不变：这里给的仍是具名基准，写进 capacity 的是三处调用方——
// Application 的 RealmAdvance（脚本升境）、CultivationScene 的 applyRealmAttributes（面板突破）、
// SaveFile 读档。写法一律 capacity = max(capacity, bottleCapacityFloor(tierOf(realm)))：
// 只补不削，已经更大的（剧情道具、洞府灵脉给的加成）一滴也不动；抬上限不送液，drops 不动。
[[nodiscard]] constexpr int bottleCapacityFloor(RealmTier tier) noexcept {
    switch (tier) {
        case RealmTier::Mortal:
        case RealmTier::QiRefining:
            return kBottleCapacityPerTier;
        case RealmTier::Foundation:
            return kBottleCapacityPerTier * 2;
        case RealmTier::Core:
            return kBottleCapacityPerTier * 3;
    }
    return kBottleCapacityPerTier * 3;
}

// 单株灵草的年份上限（万年），本命法宝的万年份天雷竹即取此上限（大纲 4.1）。
// 同时充当计价与灵田生长的溢出闸。
//
// 这是**绝对**上限，不是每一株药的上限：具体某种灵草能长到多少年，看它自己的
// maxAge（见下面的 herbMaxAgeForGrade 与 data/items/herbs/*.json）。
inline constexpr int kMaxHerbAge = 10000;

// 按品阶给出灵草的默认年份上限。data 里每味药可以用 maxAge 字段各自覆盖，
// 缺字段时就用这里的阶梯。
//
// 为什么要有这道按种设的上限：一滴绿液把年份翻倍，herbPrice 又是平方曲线，
// 两者叠起来会让「把每一滴都浇在同一株上」成为压倒性的最优解——从 1 年起
// 11 滴（炼气期约 77 天）就能把一株一阶灵草推到一万年、卖价八十余万倍，
// 此后全游戏不必再为钱操心，灵田上种什么、什么时候收也都不再有意义。
// 掌天瓶的容量上限只限制绿液的**存量**，对这条**流量**毫无作用，堵不住。
//
// 设了各自的上限之后，浇到顶就得换一株浇，决策从「全浇一株」回到「浇哪一株」；
// 更高的年份要靠后面的章节拿到更好的种子。这同时也合设定：一株一阶灵草不会
// 靠浇水变成万年灵药，而万年份的天雷竹是另一个物种（品阶八，参见
// data/items/materials/jinleizhu.json），自有其极高的上限。
//
// 阶梯（第三列是从「足年」1 年浇到顶所需的绿液滴数，按 matureHerb 实算）：
//   一阶及以下     44 年    3 滴    1→11→22→44
//   二阶          100 年    5 滴
//   三阶          300 年    6 滴
//   四阶          800 年    8 滴
//   五阶         2000 年    9 滴
//   六阶         4000 年   10 滴
//   七阶         7000 年   11 滴
//   八阶及以上  10000 年   11 滴（= kMaxHerbAge）
//
// 一阶的 44 不是随手凑的整数：它正好是 1 → 11 → 22 → 44 三滴的落点，而三滴
// 恰是炼气期的满瓶容量（kBottleCapacityPerTier）——第 2 章「攒满一瓶浇一株」
// 那一课于是正好把一株药推到它的顶。这个数与 docs/ch02-design.md 第 3 节的
// 实算表（44 年 / 145 灵石 / 24 倍）绑定，要改得连那张表一起改。
//
// grade 为 0（老数据没写品阶）按一阶处理：宁可低估，也不能因为漏一个字段就
// 退回「无上限」——那正是这道闸要堵的洞。
[[nodiscard]] int herbMaxAgeForGrade(int grade);

// 一滴绿液把年份翻一倍，且至少推进 10 年。
//
// 翻倍而不是加固定年数：绿液是「拔苗」，对底子越厚的药材推动越大，这与
// 原著里它对高年份灵药同样管用一致。保底 10 年管的是**刚足年那一档**
// （1 年浇一滴得 11 年，不是 2 年），没有它，足年的苗浇一滴只多一岁，
// 玩家会攒着绿液永远不用，经济循环卡在起点。
// 未足年的苗一滴也浇不进去，见下面 matureHerb 的 NotRipe。
inline constexpr int kMatureMinYears = 10;

// 按日历推进补充绿液，返回新增滴数。
//
// 只看 owned，不看 matureKnown：绿液该凝还是凝，玩家只是不知道它能干什么。
// 这正是「绿瓶四年」那四年里瓶子的状态。
// 未凑满一个周期时不动 lastChargeDay，零头会留到下次，不会因为调用得勤而蒸发。
//
// 满瓶时计时归零，不留欠账：满了就是满了，那段时间不该攒着等玩家腾出空位
// 再一次性灌进来。只夹住 drops 而不改计时等于没堵——挂机一百年后用掉一滴，
// 下一次结算立刻补满，漏洞原封不动。
int refill(Bottle& bottle, int currentDay, int chargeDays);

// 消耗绿液。**全项目对 drops 做减法只有这一处**，matureHerb 自己也走它。
//
// 单开这一条不是为了图方便：绿液有两种花法——催熟一株药（走 matureHerb），
// 以及把液体本身用掉（第 2 章试药那一碗，倒进碗里稀释了喂兔子，与年份无关）。
// 两处各写一份 `--drops` 的话，迟早有一处忘了先判够不够，而「凭空多出一滴」
// 在存档里看不出来。
//
// 不够就一滴不扣、返回 false；count <= 0 视作无事发生并返回 true（脚本传 0
// 不算错误，与 AdvanceDays 的口径一致）。
[[nodiscard]] bool spendDrops(Bottle& bottle, int count);

// 催熟失败的原因码。
//
// 与 reason 并存而不是取而代之：reason 是给玩家看的中文，会跟着文案改；
// 这个编码是给调用方分支用的，不随文案变。脚本要按「没瓶子 / 还不会用 /
// 没绿液 / 尚未足年 / 已至上限」说不同的话，比对中文串既脆（改一个字就哑），
// 又等于把 UI 文案钉进了逻辑里。
enum class MatureFailure {
    None = 0,
    NoBottle,        // 尚未得到那只小瓶
    MatureUnknown,   // 有瓶子，但还不知道绿液能催熟
    NoDrops,         // 会用，但瓶里没有绿液
    NotRipe,         // 这一株还不满一年，绿液浇不进去
    AtMaxAge,        // 这一味药的年份已到它自己的上限
};

// 催熟一株灵草：消耗一滴绿液，年份跃升。
struct MatureResult {
    bool ok = false;
    int newAge = 0;
    std::string reason;   // ok 为假时说明原因（没瓶子/不会用/没绿液/未足年/已达上限）
    MatureFailure failure = MatureFailure::None;   // 同一条理由的机器码
};

// currentAge 不足 kRipeAge 的一律回绝（NotRipe）。
//
// 这一条是裁决，不是手滑：没有它，两条路线的对比是
//
//     等足年再浇   1 → 11 → 22 → 44        3 滴 = 21 天，但先要等 360 天
//     种下就浇     0 → 10 → 20 → 40 → 44   4 滴 = 28 天，一天不用等
//
// 多花一滴省掉整整一年，「等足年」于是被严格支配——真实瓶颈退化成绿液流量，
// 与槽位数、与生长时间全都无关，第 2 章章末那场算账、设计文档第 3 节的收益
// 表、乃至第 6 章百药园的引子会同时失准。
//
// 而游戏自己早就立过这条规矩：`ch02.renyao.guanshi_rule` 里药圃管事当着玩家
// 的面说「不足一年的不准动手，采下来就是废的」。让绿液绕过它本来就说不通。
// 拦上之后，「攒满一瓶浇同一株」才真正是最优解，本章的教学才立得住。
//
// 注意剧情里那次误洒走的是脚本 give(..., 1, 11)，不经过这里，不受影响。
[[nodiscard]] MatureResult matureHerb(Bottle& bottle, int currentAge, int maxAge);

// 灵草按年份计价。
//
// 曲线是 (age + 10)^2 / 100 倍基价，严格超线性：十年灵草 4 倍基价，百年灵草
// 121 倍，百年是十年的三十倍而不是十倍。整个掌天瓶经济的动力全在这道曲线上
// ——催熟一次的收益必须显著高于把低年份直接卖掉，否则玩家不会去用瓶子，
// 第 2 章闭合的核心循环就断了。
[[nodiscard]] int herbPrice(int basePrice, int age);

}  // namespace fanren::rules
