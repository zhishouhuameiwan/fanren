-- 剧情闸门的真实用法：查询结果决定走哪条分支，结论落进旗标供 C++ 断言。
-- 这是第 2 章要求脚本能做的第三件事——没有它，「攒够一滴绿液」这类条件写不出来。
if bottle.owned() and bottle.drops() >= 1 then
    flag.set("gate_bottle_ready")
end
if field.planted() >= 1 then
    flag.set("gate_planted")
end
flag.set("gate_day", today())
flag.set("gate_ripe", field.ripe("shenshougu_yaopu"))
