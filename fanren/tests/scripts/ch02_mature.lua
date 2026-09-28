-- 由 ScriptApiTests 驱动：催熟一株灵草，把结果原样记在旗标上。
--
-- 失败原因按码分开置位，而不是记一个「失败了」的布尔：四条失败路径是叙事的
-- 一部分（没瓶子 / 还不会用 / 没绿液 / 已至上限），断言必须能分辨错成了哪一种，
-- 否则「永远报同一种失败」的实现也能让测试全绿。
local age = flag.get("test_age")
local ok, new_age, why = bottle.mature("herb_huangjing_cao", age)

flag.set("mature_ok", ok and 1 or 0)
flag.set("mature_age", new_age)
flag.set("code_" .. (why == "" and "none" or why), 1)
