#pragma once
// Lua 协程宿主。实现方案 3.5 的并发模型：
//   startEvent 起协程 → 每帧 pollCommand 取命令 → game 层执行 → resumeWith 回填
// 全程没有嵌套阻塞 run()，主循环始终掌握节奏。
//
// 本头文件刻意不出现任何 sol / lua 类型：sol2 的模板展开很重，一旦泄进公共头，
// 每个包含它的 TU 都要付编译代价，还会把 /W3 的豁免需求传染出去。故用 pimpl。
#include <functional>
#include <memory>
#include <string>

#include "core/Result.h"
#include "core/model/Types.h"
#include "script/Command.h"

namespace fanren::script {

class ScriptHost {
public:
    // 诊断输出去向。默认写 stderr；测试与 game 层可以接管。
    using LogSink = std::function<void(const std::string&)>;

    ScriptHost();
    ~ScriptHost();
    ScriptHost(const ScriptHost&) = delete;
    ScriptHost& operator=(const ScriptHost&) = delete;

    // scriptRoot 是 scripts/ 目录；state 供只读查询（flag.get / item.count /
    // realm.at_least）使用，不得为空。init 会加载 scriptRoot/common/api.lua。
    core::Result<bool> init(const std::string& scriptRoot, core::GameState* state);

    // 启动一个事件脚本。scriptPath 相对 scriptRoot，如 "ch01/sanshu.lua"。
    // 文件缺失、语法错误、首次 resume 就抛错，都返回 failure，error 里带脚本名
    // 与行号。任何情况下都不抛异常、不终止进程。
    core::Result<bool> startEvent(const std::string& scriptPath);

    bool isRunning() const;

    // 取出当前待执行命令。这是「看一眼」不是「拿走」：同一条命令在 resumeWith
    // 之前每帧都能取到，game 层可以慢慢把它演完而不必自己缓存。
    // 协程未挂起或已结束时返回 false。
    bool pollCommand(Command& out);

    // game 层执行完命令后回填结果，协程继续跑到下一个 yield 或结束。
    void resumeWith(const CommandResult& result);

    // 协程挂起期间禁止存档（方案 3.5 规则 4）。
    bool canSave() const { return !isRunning(); }

    // ---- 以下为契约之外的附加项，均为新增、不改变上面任何签名 ----

    // 场景弹出时丢弃未结束的协程（方案 3.5 规则 6）。reason 会进日志。
    void abortEvent(const std::string& reason);

    // 最近一次失败的描述。协程跑到一半才出错时 resumeWith 没有返回值通道，
    // 调用方从这里取。
    const std::string& lastError() const;

    void setLogSink(LogSink sink);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace fanren::script
