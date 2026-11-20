#pragma once

// 战斗外用药（2026-11-20 用户试玩第 7 章：「储物袋里面的东西并不能使用，比如金创药」）。
//
// 只做回气血、回法力这两样。别的效果都只在战斗里有意义，在菜单里用不上：
// - 毒、解毒：中毒是战斗里的状态，不进存档；
// - 施法的符：要有目标。
// 判据照 battleUsable 的口径，看效果字段，不看 kind、不看物品 id：
// 灵草里也有能当场嚼的（restoreHp > 0），丹药里也有纯剧情用的（全是 0）。
//
// 同伴的法力不进存档、每场仗按模板满额开场（PartyMember 的注释），
// 所以只有主角的法力补得上；同伴只回气血。

#include <algorithm>

#include "core/model/Types.h"

namespace fanren::rules {

[[nodiscard]] inline bool fieldUsable(const core::Item& item) { return item.restoreHp > 0 || item.restoreMp > 0; }

// 用在一个人身上能补回多少。两项都是 0 就是「满着，不必用」：那时不该扣东西。
// maxMp 传 0 表示这个人的法力不在这里记账（同伴）。
struct FieldGain {
    int hp = 0;
    int mp = 0;
    [[nodiscard]] bool any() const { return hp > 0 || mp > 0; }
};

[[nodiscard]] inline FieldGain fieldGain(const core::Item& item, int hp, int maxHp, int mp, int maxMp) {
    FieldGain gain;
    gain.hp = std::max(0, std::min(item.restoreHp, maxHp - hp));
    gain.mp = std::max(0, std::min(item.restoreMp, maxMp - mp));
    return gain;
}

}  // namespace fanren::rules
