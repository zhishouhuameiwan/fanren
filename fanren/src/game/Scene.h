#pragma once
// 场景状态机。
//
// 刻意不沿用 kys-cpp 的嵌套阻塞 run()：那种写法把剧情流程压在 C++ 调用栈上，
// 协程挂起时栈无法序列化，场景提前销毁还会留下悬空引用（见方案 3.5）。
// 这里改为由主循环每帧驱动的场景栈，剧情顺序由 Lua 协程维持。
#include <memory>
#include <string>
#include <vector>

#include "engine/Engine.h"

namespace fanren::game {

class Application;

class Scene {
public:
    virtual ~Scene() = default;

    virtual void onEnter(Application&) {}
    virtual void onExit(Application&) {}

    // 返回 false 表示本场景已结束，应由主循环弹出。
    virtual bool update(Application& app, double deltaSeconds) = 0;
    virtual void render(Application& app) = 0;

    // 下层场景是否继续绘制。对话框等半透明覆盖层返回 false。
    [[nodiscard]] virtual bool opaque() const { return true; }

    [[nodiscard]] virtual std::string name() const = 0;
};

using ScenePtr = std::unique_ptr<Scene>;

}  // namespace fanren::game
