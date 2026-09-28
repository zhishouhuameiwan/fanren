-- 负向：yield 一个解析表里根本没有的 kind。
--
-- 绕开 api.lua 是故意的 —— api.lua 里没有哪个函数造得出这种命令，而「kind 拼错」
-- 恰恰只会以这种形状出现。第 1 章的教训：静默通过的闸门等于没有闸门，所以
-- ScriptHost 必须当场报错。
coroutine.yield{ kind = "no_such_command", a = "x" }

-- 这一行是探针：命令若被静默忽略，脚本会继续往下跑并留下旗标。
-- 测试断言它**不存在**，从而证明失败是真的中断了脚本，而不只是记了条日志。
flag.set("unknown_kind_slipped_through")
