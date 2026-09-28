#pragma once
// 跨模块的失败表达。约定：不跨模块边界抛异常，失败一律用 Result 返回。
// error 是 UTF-8 且可直接呈现给玩家或写进日志。
#include <string>
#include <utility>

namespace fanren::core {

template <typename T>
struct Result {
    bool ok = false;
    T value{};
    std::string error;

    [[nodiscard]] static Result success(T v) {
        return Result{true, std::move(v), std::string{}};
    }
    [[nodiscard]] static Result failure(std::string e) {
        return Result{false, T{}, std::move(e)};
    }

    explicit operator bool() const noexcept { return ok; }
};

}  // namespace fanren::core
