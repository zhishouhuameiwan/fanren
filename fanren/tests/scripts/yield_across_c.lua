-- 回归用例：故意把挂起塞到 C 函数底下。
-- table.sort 是 C 函数，比较器是它回调的 Lua 函数；此时挂起点上压着一层 C 帧，
-- Lua 只能报 "attempt to yield across a C-call boundary"。
-- 这正是设计约束 2 要绕开的路：正常 API 从不经过这种形状，而万一经过，
-- ScriptHost 必须把错误收住而不是让进程死。
local names = { "b", "a", "c" }
table.sort(names, function(x, y)
    talk("boom", "t.never")
    return x < y
end)
talk("boom", "t.unreachable")
