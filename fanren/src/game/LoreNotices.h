#pragma once
// 见闻解锁提示（第一期地基包，docs/ch07-foundation.md 2.5）：「见闻：XXX」一条，浮在屏幕上沿正中，
// 几秒后自己淡掉。**不挡路、不用按键**：不是场景，不进场景栈，不收输入（与得物卡片同一个做法）。
//
// 分两半：LoreNotices 是纯数据（推、走时钟、淡入淡出的浓度），不开窗口可测；drawLoreNotice 只画。
#include <deque>
#include <string>

#include "engine/Engine.h"
#include "ui/Widgets.h"

namespace fanren::game {

namespace hud {
class Geometry;
}

class LoreNotices {
public:
    static constexpr float kFadeIn = 0.25f;
    static constexpr float kHold = 2.6f;
    static constexpr float kFadeOut = 0.6f;
    static constexpr std::size_t kMaxQueued = 8;   // 一次解锁一大批时最多排这么多条，再多挤掉最早的

    // 一条一条地放：前一条淡完下一条才出来。
    void push(std::string text);
    void advance(float deltaSeconds);
    void clear() { queue_.clear(); age_ = 0.f; }

    [[nodiscard]] bool active() const { return !queue_.empty(); }
    // 此刻在放的那一条；没有为空串。
    [[nodiscard]] const std::string& current() const;
    [[nodiscard]] float alpha() const;
    [[nodiscard]] const std::deque<std::string>& queue() const { return queue_; }

private:
    std::deque<std::string> queue_;
    float age_ = 0.f;
};

void drawLoreNotice(engine::Engine& engine, const ui::Theme& theme, hud::Geometry& geometry,
                    const LoreNotices& notices);

}  // namespace fanren::game
