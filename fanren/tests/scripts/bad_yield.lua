-- 绕开 api.lua 直接 yield 一个非命令值。ScriptHost 应判失败而不是崩。
coroutine.yield("这不是命令表")
