-- 由 ScriptApiTests 驱动：三滴摞在同一株上，1 → 11 → 22 → 44。
--
-- 这是第 2 章节点 11 的机制原型（cuishu.lua 第二场跑的是同一串调用）：
-- 每一滴的落点都问引擎要，脚本一个年份也不自己推。
local age = flag.get("test_age")

for drop = 1, 3 do
    local ok, new_age, why = bottle.mature("herb_huangjing_cao", age)
    flag.set("drop_" .. drop .. "_ok", ok and 1 or 0)
    if not ok then
        flag.set("stopped_at", drop)
        flag.set("code_" .. why, 1)
        break
    end
    age = new_age
    flag.set("age_after_" .. drop, age)
end

flag.set("final_age", age)
