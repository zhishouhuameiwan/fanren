#pragma once
// 遭遇表规则。签名以 docs/interfaces-p2.md 第 3 节为准。
//
// core 层硬约束：不含 SDL / JSON / Lua / 文件 IO；随机数种子由调用方传入，
// 保证「同种子 + 同输入」必得同结果。
#include <cstdint>
#include <string>
#include <vector>

#include "core/rules/Realm.h"

namespace fanren::rules {

struct EncounterEntry {
    std::string battleId;
    int weight = 1;
    Realm minRealm = Realm::Mortal;
    Realm maxRealm = Realm::CoreLate;
};

struct EncounterTable {
    std::string id;
    std::vector<EncounterEntry> entries;
    int stepsMin = 20;
    int stepsMax = 60;
    int dailyCap = 8;      // 每日触发上限，防刷
};

// 遭遇计数器，随存档持久化。
struct EncounterState {
    int stepsSinceLast = 0;
    int stepsUntilNext = 0;
    int triggeredToday = 0;
    int lastDay = 0;
};

// 走一步。返回非空表示触发了遭遇，值为 battleId。
[[nodiscard]] std::string step(const EncounterTable& table, EncounterState& state,
                               Realm realm, int currentDay, std::uint32_t seed);

}  // namespace fanren::rules
