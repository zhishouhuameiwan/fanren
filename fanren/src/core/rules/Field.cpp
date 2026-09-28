#include "core/rules/Field.h"

#include <algorithm>

#include "core/rules/Bottle.h"   // kMaxHerbAge：年份上限只留一处定义

namespace fanren::rules {

int slotAgeCeiling(const FieldSlot& slot) {
    // 未标定（老存档、以及不关心上限的调用方）退回全局溢出闸；标定过的也不
    // 允许越过全局闸，否则 herbPrice 的平方曲线会溢出成负数。
    if (slot.maxAge <= 0) return kMaxHerbAge;
    return std::min(slot.maxAge, kMaxHerbAge);
}

bool plant(SpiritField& field, int slotIndex, const std::string& seedId, int currentDay,
           int maxAge) {
    if (slotIndex < 0 || static_cast<std::size_t>(slotIndex) >= field.slots.size()) return false;
    if (seedId.empty()) return false;

    FieldSlot& slot = field.slots[static_cast<std::size_t>(slotIndex)];
    // 不允许覆盖已种下的槽位：覆盖会静默销毁一株可能已经养了几十年的灵草，
    // 而这个操作在 UI 上和「种一株新的」长得一模一样。
    if (!slot.seedId.empty()) return false;

    slot.seedId = seedId;
    slot.plantedDay = currentDay;
    slot.age = 0;
    slot.ripe = false;
    slot.maxAge = std::max(maxAge, 0);   // 负数当未标定处理，不让它反向变成 0 年上限
    return true;
}

std::vector<int> growField(SpiritField& field, int currentDay, int daysPerYear) {
    std::vector<int> ripened;
    // daysPerYear 由调用方按灵脉品阶折算，为零或负数只可能是配置写错；
    // 除零会崩，负数会让年份倒着长，两者都不该带进玩家的存档。
    if (daysPerYear <= 0) return ripened;

    for (std::size_t i = 0; i < field.slots.size(); ++i) {
        FieldSlot& slot = field.slots[i];
        if (slot.seedId.empty()) continue;

        const long long elapsed = static_cast<long long>(currentDay) - slot.plantedDay;
        if (elapsed < daysPerYear) continue;

        const long long years = elapsed / daysPerYear;
        // 只封顶，不回收：已经超过上限的年份（老存档里的、剧情直接给的高年份
        // 灵草）保持原样，只是不再往上长。把玩家账面上的年份改小，看起来就是
        // 掉档，而这条路径每跨一年都会跑一次，掉起来还是悄悄掉。
        const long long ceiling = slotAgeCeiling(slot);
        if (slot.age < ceiling) {
            slot.age = static_cast<int>(std::min<long long>(slot.age + years, ceiling));
        }
        // 只吃掉整年，零头留在账上。plantedDay 因此始终只按整年前移，
        // 播种日在一年中的位置（即「纪念日」）不变，反复调用也不会漂。
        slot.plantedDay += static_cast<int>(years * daysPerYear);

        if (!slot.ripe && slot.age >= kRipeAge) {
            slot.ripe = true;
            ripened.push_back(static_cast<int>(i));
        }
    }
    return ripened;
}

}  // namespace fanren::rules
