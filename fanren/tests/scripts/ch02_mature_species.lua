-- 由 ScriptApiTests 驱动：同样的年份、同样的一滴，两味药的落点不同。
--
-- 年份上限是**按种**的（data/items/herbs/*.json 的 maxAge，缺字段时按品阶推）：
-- 四十四年的黄精已经到顶，再浇也不添年份、绿液也不该被扣走；
-- 同样四十四年的紫参草上限是一百年，这一滴还推得动。
-- 若哪天改回全局上限，这条测试会同时在两边红。
local ok_a, age_a, code_a = bottle.mature("herb_huangjing_cao", 44)
flag.set("a_ok", ok_a and 1 or 0)
flag.set("a_age", age_a)
flag.set("a_code_" .. (code_a == "" and "none" or code_a), 1)

local ok_b, age_b, code_b = bottle.mature("herb_zishen_cao", 44)
flag.set("b_ok", ok_b and 1 or 0)
flag.set("b_age", age_b)
flag.set("b_code_" .. (code_b == "" and "none" or code_b), 1)
