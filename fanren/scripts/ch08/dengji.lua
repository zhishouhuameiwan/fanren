-- @hook ch06_huangfenggu trigger_dengji interact once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.shixiong") == 0 or flag.get("ch08.dengji") ~= 0 then return end
talk("zhong_lingdao", "ch08.dengji.t1")
talk("qilinge_dizi", "ch08.dengji.t2")
if item.count("material_lingshi") >= 3 then
    if not take("material_lingshi", 3) then return end
else
local pick = choice({"ch08.dengji.choice1", "ch08.dengji.choice2"})
if not pick then return end
flag.set("ch08.dizufang", pick)
    if pick ~= 1 then return end
    advance_days(3)
end
give("material_lingshi_zhong", 3)
give("story_mizong_qi", 5)
give("story_zhuji_yujian", 1)
advance_days(1)
teleport("ch08_dongfu", 45, 24)
flag.set("ch08.dengji", 1)
