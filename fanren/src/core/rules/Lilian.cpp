#include "core/rules/Lilian.h"

namespace fanren::rules {
namespace {

// FNV-1a（32 位）。不用 std::hash：它的结果随标准库实现变，同一份存档在两台机器上会抽出两种结果。
constexpr std::uint32_t kFnvOffset = 2166136261u;
constexpr std::uint32_t kFnvPrime = 16777619u;

[[nodiscard]] std::uint32_t mixByte(std::uint32_t hash, std::uint8_t byte) {
    return (hash ^ byte) * kFnvPrime;
}

[[nodiscard]] std::uint32_t mixString(std::uint32_t hash, const std::string& text) {
    for (const char c : text) hash = mixByte(hash, static_cast<std::uint8_t>(c));
    return hash;
}

[[nodiscard]] std::uint32_t mixInt(std::uint32_t hash, std::uint32_t value) {
    for (int shift = 0; shift < 32; shift += 8) hash = mixByte(hash, static_cast<std::uint8_t>(value >> shift));
    return hash;
}

// 末尾再搅一遍（murmur3 的 fmix32）：FNV 对只差最后一个字节的输入，低位差得不够开，
// 而「对 100 取余」看的正是低位——日子只差一天时不该几乎抽出同一组。
[[nodiscard]] std::uint32_t finalize(std::uint32_t h) {
    h ^= h >> 16;
    h *= 0x85ebca6bu;
    h ^= h >> 13;
    h *= 0xc2b2ae35u;
    h ^= h >> 16;
    return h;
}

}  // namespace

int lilianTier(const core::LilianSite& site, Realm realm) {
    if (site.tiers.empty()) return 0;
    for (std::size_t i = 0; i < site.tiers.size(); ++i) {
        if (toValue(realm) <= toValue(site.tiers[i].maxRealm)) return static_cast<int>(i) + 1;
    }
    return static_cast<int>(site.tiers.size());
}

std::string tieredBattleId(const std::string& base, int tier) {
    if (tier <= 0) return base;
    return base + "_t" + std::to_string(tier);
}

std::uint32_t lilianSeed(int day, const std::string& siteId) {
    std::uint32_t hash = mixInt(kFnvOffset, static_cast<std::uint32_t>(day));
    hash = mixByte(hash, 0);   // 隔开两段，免得 (12, "3x") 与 (123, "x") 拼成同一串
    return finalize(mixString(hash, siteId));
}

bool lilianChanceShows(std::uint32_t seed, const std::string& objectId, int chance) {
    if (chance >= 100) return true;
    if (chance <= 0) return false;
    const std::uint32_t roll = finalize(mixString(mixInt(kFnvOffset, seed), objectId)) % 100u;
    return roll < static_cast<std::uint32_t>(chance);
}

}  // namespace fanren::rules
