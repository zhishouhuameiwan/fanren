-- @hook ch08_yanlingbao trigger_tianheju interact once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.caihuan") == 0 or flag.get("ch08.tianheju") ~= 0 then return end
talk("wuyouzi", "ch08.tianheju.t1")
talk("wuyouzi", "ch08.tianheju.f1")
talk("", "ch08.tianheju.t2")
local pick = choice({"ch08.tianheju.choice1", "ch08.tianheju.choice2"})
if not pick then return end
flag.set("ch08.tianheju", pick)
talk("dong_xuaner", "ch08.tianheju.t3")
talk("", "ch08.tianheju.t4")
advance_days(1)
talk("", "ch08.tianheju.f2")
if flag.get("ch08.tianheju") == 0 then flag.set("ch08.tianheju", 1) end
