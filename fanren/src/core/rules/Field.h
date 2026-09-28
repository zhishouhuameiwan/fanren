#pragma once
// 灵田种植。纯逻辑，不依赖 SDL / JSON / Lua / 文件 IO。
//
// 与掌天瓶合起来构成第 2 章闭合的核心循环：种下 → 自然生长 → 绿液催熟 →
// 按年份计价出售（计价见 core/rules/Bottle.h 的 herbPrice）。
// 第 7 章的百药园是同一套规则，只是槽位更多、daysPerYear 更小。
#include <string>
#include <vector>

namespace fanren::rules {

struct FieldSlot {
    std::string seedId;      // 空表示空闲
    int plantedDay = 0;      // 播种日；生长结算只按整年推进它，故播种「纪念日」不变
    int age = 0;             // 当前年份
    bool ripe = false;
    // 这一株（按种）的年份上限，播种时由 game 层查 data 写入（追加在末尾，
    // 不破坏既有的聚合初始化）。0 表示「未标定」，退回全局的 kMaxHerbAge：
    // 老存档与不带上限的调用方由此保持原样，不会被静默改数。
    int maxAge = 0;
};

struct SpiritField {
    std::string id;
    std::vector<FieldSlot> slots;
};

// 满一年方可采。之后仍继续长年份，采与不采由玩家决定——「什么时候收」
// 正是灵田玩法唯一的决策点，提前锁死收获时机等于把玩法拿掉。
inline constexpr int kRipeAge = 1;

// 一个槽位实际生效的年份上限：未标定（maxAge <= 0）时退回全局溢出闸。
// 「0 即未标定」这条约定只在这里落地一次，免得各处各写一遍判断。
[[nodiscard]] int slotAgeCeiling(const FieldSlot& slot);

// 在指定槽位播种。下标越界、种子 id 为空、槽位已占用都返回 false。
// 契约里没有这一条，属追加（不改动已有签名）：播种时 age/ripe/plantedDay
// 三个字段的约定只有一处实现，测试与 game 层才不会各写一份。
//
// maxAge 是这一味药自己的年份上限（见 core/rules/Bottle.h 的 herbMaxAgeForGrade），
// 由 game 层查 data 传入；缺省的 0 表示未标定，自然生长退回全局上限。
// 之所以给默认值而不是改签名：已有的调用方与测试不必跟着改，而真正要紧的
// 那条路径（灵田面板播种）是显式传值的。
bool plant(SpiritField& field, int slotIndex, const std::string& seedId, int currentDay,
           int maxAge = 0);

// 按日历推进全部槽位，返回**本次新成熟**的槽位下标。
//
// 只返回状态翻转的那一批，而不是所有已熟槽位：后者会让调用方每天收到同一批
// 下标，提示与音效天天重放。已熟未收的槽位靠 slot.ripe 自行扫描。
//
// 年份是增量累加而不是由 plantedDay 反算的，这样绿液催熟加上去的年份不会在
// 下一次生长结算时被抹平——催熟与自然生长必须能叠加，否则瓶子等于没用。
//
// 年份长到这一株自己的上限（slotAgeCeiling）就停住。按种封顶不只管绿液：
// 上限若只拦催熟，「种下去搁上两百年再收」会原样刷出同一份天价，只是慢一些，
// 而跳时在剧情里是免费的（第 2 章一章就是四年）。何况上限是物种属性——
// 一株一阶灵草不会因为在地里躺得久就成了万年灵药。
std::vector<int> growField(SpiritField& field, int currentDay, int daysPerYear);

}  // namespace fanren::rules
