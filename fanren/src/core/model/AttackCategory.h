#pragma once
// 攻击类别：「破势与蓄劲」里一击打的是哪一类（docs/octopath-battle.md 第 2 节）。
//
// 十种，顺序固定：兵刃 剑 / 刀 / 拳 / 暗器 / 毒，五行 金 / 木 / 水 / 火 / 土。
// 位掩码的位序就是这个顺序——界面上的破绽格、菜单里的兵刃行、存档里写出来的名字
// 全按位序排，于是「第二格是刀」在哪里都一样，不跟着数据文件里的书写顺序走。
//
// 放在 core/model 而不是 core/battle：角色模板（RoleTemplate::weaknesses）、物品
// （Item::weapon）与存档（GameState::knownWeaknesses）都要用它，而 model 不该反过来
// 依赖战斗规则层。
//
// 数据里写的是中文名（"weapons": ["剑"]），不写位号：位号是实现细节，名字才是
// 设计文档与玩家看得见的那一层。名字与位号的对照只在这里有一份。
#include <string>
#include <vector>

namespace fanren::core {

enum AttackCategory : int {
    kCategoryNone = 0,
    kCategorySword = 1 << 0,    // 剑
    kCategoryBlade = 1 << 1,    // 刀
    kCategoryFist = 1 << 2,     // 拳（空手也算：谁都打得出一拳）
    kCategoryHidden = 1 << 3,   // 暗器
    kCategoryPoison = 1 << 4,   // 毒
    kCategoryMetal = 1 << 5,    // 金
    kCategoryWood = 1 << 6,     // 木
    kCategoryWater = 1 << 7,    // 水
    kCategoryFire = 1 << 8,     // 火
    kCategoryEarth = 1 << 9,    // 土
};

inline constexpr int kCategoryCount = 10;

// 兵刃五类：角色的 weapons、兵器物品的 weapon 只许写这几样。
inline constexpr int kWeaponCategories =
    kCategorySword | kCategoryBlade | kCategoryFist | kCategoryHidden | kCategoryPoison;
// 五行五类：法术的类别由它的五行给出。
inline constexpr int kElementCategories =
    kCategoryMetal | kCategoryWood | kCategoryWater | kCategoryFire | kCategoryEarth;
inline constexpr int kAllCategories = kWeaponCategories | kElementCategories;

// 单个类别位的中文名；传入多位或 0 时返回空串（调用方拿它当「这不是一个类别」）。
[[nodiscard]] const char* categoryName(int category) noexcept;

// 中文名 → 类别位；认不出返回 kCategoryNone。加载器靠它把拼错的名字当场报出来。
[[nodiscard]] int categoryFromName(const std::string& name) noexcept;

// 五行位掩码（core::Element）→ 类别位掩码。法术的类别就是这么来的。
[[nodiscard]] int categoriesOfElement(int elementMask) noexcept;

// 掩码里的每一位，按位序（剑 → 土）拆开。
[[nodiscard]] std::vector<int> categoryBits(int mask);

// 掩码的中文名，按位序用 separator 连起来（"剑、火"）。空掩码返回空串。
[[nodiscard]] std::string categoryNames(int mask, const std::string& separator = "、");

// 恰好是一个类别位（不是 0，也不是好几位并在一起）。
[[nodiscard]] constexpr bool isSingleCategory(int category) noexcept {
    return category != 0 && (category & (category - 1)) == 0 && (category & ~kAllCategories) == 0;
}

}  // namespace fanren::core
