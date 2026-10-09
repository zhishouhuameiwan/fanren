-- @hook ch08_dongfu trigger_shuan interact once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.lvbo") == 0 or flag.get("ch08.shuye") ~= 0 then return end
talk("", "ch08.shuan.t1")
local n = math.min(item.count("story_yinse_shuye"), 1)
if n > 0 and not take("story_yinse_shuye", n) then return end
local n = math.min(item.count("story_jinye"), 1)
if n > 0 and not take("story_jinye", n) then return end
advance_days(3)
talk("", "ch08.shuan.t2")
talk("", "ch08.shuan.t3")
talk("", "ch08.shuan.f1")
talk("", "ch08.shuan.f2")
talk("", "ch08.shuan.f3")
flag.set("ch08.shuye", 1)
