-- 由 ScriptApiTests 驱动：按年份精确扣一株灵草。
--
-- test_age > 0 时走「精确扣那一堆」，等于 0 时走「哪一堆都行」——两种请求在
-- 命令里的差别只有一个 y，所以两条路都要从真的 api.lua 走一遍。
local age = flag.get("test_age")
local ok = take("herb_huangjing_cao", 1, age)
flag.set("take_ok", ok and 1 or 0)
