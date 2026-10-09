-- @hook ch08_dongfu trigger_dating interact once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.yexi") == 0 or flag.get("ch08.gufang") ~= 0 then return end
talk("lei_wanhe", "ch08.dating.t1")
local pick = choice({"ch08.dating.choice1", "ch08.dating.choice2"})
if not pick then return end
flag.set("ch08.leiwen", pick)
talk("lei_wanhe", "ch08.dating.t2")
talk("", "ch08.dating.t3")
talk("lei_wanhe", "ch08.dating.t4")
give("material_lingshi", 5)
give("story_dayan_yujian", 1)
talk("", "ch08.dating.t5")
if flag.get("ch08.yexi") == 2 then talk("", "ch08.dating.later") end
advance_days(1)
talk("lei_wanhe", "ch08.dating.f1")
talk("", "ch08.dating.f2")
talk("", "ch08.dating.f3")
flag.set("ch08.gufang", 1)
