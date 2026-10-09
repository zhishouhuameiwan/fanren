-- @hook ch08_lingkuang trigger_jingshi interact once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.lingkuang") == 0 or flag.get("ch08.kuilei") ~= 0 then return end
talk("", "ch08.jingshi.t1")
talk("", "ch08.jingshi.t2")
advance_days(105)
if not party.add("kuilei_shou") then return end
talk("", "ch08.jingshi.t3")
talk("lv_tianmeng", "ch08.jingshi.t4")
if flag.get("ch07.zhongwu") == 1 then
talk("zhong_wu", "ch08.jingshi.t5")
else
talk("zhong_wu", "ch08.jingshi.t6")
end
local pick = choice({"ch08.jingshi.choice1", "ch08.jingshi.choice2"})
if not pick then return end
flag.set("ch08.kuilei", pick)
talk("", "ch08.jingshi.f1")
talk("", "ch08.jingshi.f2")
talk("", "ch08.jingshi.f3")
if flag.get("ch08.kuilei") == 0 then flag.set("ch08.kuilei", 1) end
