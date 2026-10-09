-- @hook ch08_dongfu trigger_lvbo interact once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.gufang") == 0 or flag.get("ch08.lvbo") ~= 0 then return end
fade.out(250)
talk("", "ch08.lvbo.t1")
if flag.get("ch07.baishi") == 1 then
talk("li_huayuan_shifu", "ch08.lvbo.t2")
else
talk("li_huayuan_shifu", "ch08.lvbo.t3")
end
talk("li_huayuan_shifu", "ch08.lvbo.t4")
local pick = choice({"ch08.lvbo.choice1", "ch08.lvbo.choice2"})
if not pick then return end
flag.set("ch08.lvbo", pick)
talk("li_shiniang", "ch08.lvbo.t5")
talk("li_huayuan_shifu", "ch08.lvbo.t6")
talk("li_huayuan_shifu", "ch08.lvbo.f2")
talk("", "ch08.lvbo.t7")
give("story_jinye", 1)
advance_days(2)
talk("li_shiniang", "ch08.lvbo.f1")
talk("", "ch08.lvbo.f3")
fade.in_(250)
if flag.get("ch08.lvbo") == 0 then flag.set("ch08.lvbo", 1) end
