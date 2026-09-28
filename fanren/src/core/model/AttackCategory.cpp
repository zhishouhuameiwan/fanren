#include "core/model/AttackCategory.h"

#include <array>
#include <utility>

#include "core/model/Types.h"

namespace fanren::core {
namespace {

// 位序即显示顺序。名字与位号的对照只有这一张表。
constexpr std::array<std::pair<int, const char*>, kCategoryCount> kNames{{
    {kCategorySword, "剑"},
    {kCategoryBlade, "刀"},
    {kCategoryFist, "拳"},
    {kCategoryHidden, "暗器"},
    {kCategoryPoison, "毒"},
    {kCategoryMetal, "金"},
    {kCategoryWood, "木"},
    {kCategoryWater, "水"},
    {kCategoryFire, "火"},
    {kCategoryEarth, "土"},
}};

// 五行 → 类别。core::Element 的位号与类别位号不同（前者是 P1 就定下的存量编号），
// 所以这里逐项对照，不做移位换算：移位换算看着省事，哪天有人调了其中一边的位序，
// 它会悄悄把「火」换算成「水」。
constexpr std::array<std::pair<int, int>, 5> kElementToCategory{{
    {kElementMetal, kCategoryMetal},
    {kElementWood, kCategoryWood},
    {kElementWater, kCategoryWater},
    {kElementFire, kCategoryFire},
    {kElementEarth, kCategoryEarth},
}};

}  // namespace

const char* categoryName(int category) noexcept {
    for (const auto& [bit, name] : kNames) {
        if (bit == category) return name;
    }
    return "";
}

int categoryFromName(const std::string& name) noexcept {
    for (const auto& [bit, text] : kNames) {
        if (name == text) return bit;
    }
    return kCategoryNone;
}

int categoriesOfElement(int elementMask) noexcept {
    int out = kCategoryNone;
    for (const auto& [element, category] : kElementToCategory) {
        if ((elementMask & element) != 0) out |= category;
    }
    return out;
}

std::vector<int> categoryBits(int mask) {
    std::vector<int> bits;
    for (const auto& [bit, name] : kNames) {
        static_cast<void>(name);
        if ((mask & bit) != 0) bits.push_back(bit);
    }
    return bits;
}

std::string categoryNames(int mask, const std::string& separator) {
    std::string out;
    for (const int bit : categoryBits(mask)) {
        if (!out.empty()) out += separator;
        out += categoryName(bit);
    }
    return out;
}

}  // namespace fanren::core
