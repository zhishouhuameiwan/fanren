#pragma once

// 平台无关的「区间均匀整数」：替代 std::uniform_int_distribution<int>。
//
// 标准只规定了 std::mt19937 的输出序列，没规定 uniform_int_distribution 怎么把它
// 映射到 [lo, hi]——MSVC 的 STL 与 libc++ 用的是两种算法，同一颗种子在 Windows 与
// macOS 上掷出的点数不同。本项目的通关用例、正式交接存档、平衡记录全是在 MSVC 上
// 掷出来的，所以这里逐位复刻 MSVC STL 的做法（stl/inc/random 的
// uniform_int_distribution::_Eval 与 stl/inc/algorithm 的 _Rng_from_urng_v2，
// 即 Lemire 2019 的拒绝采样），只取本项目用到的那一种组合：
// int 区间 × 32 位满幅的 std::mt19937。这样 Windows 上结果逐位不变，别的平台与之相同。
//
// 用法与 std::uniform_int_distribution 相同：UniformInt roll(0, 99); roll(rng)。

#include <cstdint>
#include <random>

namespace fanren::rules {

class UniformInt {
public:
    UniformInt(int lo, int hi) noexcept : lo_(lo), hi_(hi) {}

    int operator()(std::mt19937& rng) const {
        // MSVC 先把有符号区间翻成无符号（异或最高位），区间宽度不变。
        const std::uint32_t umin = adjust(static_cast<std::uint32_t>(lo_));
        const std::uint32_t umax = adjust(static_cast<std::uint32_t>(hi_));
        std::uint32_t offset = 0;
        if (umax - umin == UINT32_MAX) {
            offset = static_cast<std::uint32_t>(rng());
        } else {
            offset = lemire(rng, umax - umin + 1);
        }
        return static_cast<int>(adjust(offset + umin));
    }

private:
    static std::uint32_t adjust(std::uint32_t value) noexcept { return value ^ 0x80000000u; }

    // [0, range)：取 32 位 x，m = x * range；低 32 位落在拒绝带里就重掷，返回 m 的高 32 位。
    static std::uint32_t lemire(std::mt19937& rng, std::uint32_t range) {
        std::uint64_t product = static_cast<std::uint64_t>(static_cast<std::uint32_t>(rng())) * range;
        auto low = static_cast<std::uint32_t>(product);
        if (low < range) {
            const std::uint32_t threshold = (0u - range) % range;   // (2^32 - range) mod range
            while (low < threshold) {
                product = static_cast<std::uint64_t>(static_cast<std::uint32_t>(rng())) * range;
                low = static_cast<std::uint32_t>(product);
            }
        }
        return static_cast<std::uint32_t>(product >> 32);
    }

    int lo_;
    int hi_;
};

}  // namespace fanren::rules
