-- @hook ch08_jinguyuan trigger_lizhu enter once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.daji") == 0 or flag.get("ch08.nangong") ~= 0 then return end
talk("nangong_wan", "ch08.lizhu.t2")
talk("li_huayuan_shifu", "ch08.lizhu.t1")
local pick = choice({"ch08.lizhu.choice1", "ch08.lizhu.choice2"})
if not pick then return end
flag.set("ch08.nangong", pick)
talk("li_huayuan_shifu", "ch08.lizhu.t3")
talk("li_huayuan_shifu", "ch08.lizhu.f1")
talk("", "ch08.lizhu.f2")
advance_days(8)
teleport("ch08_jinmacheng", 24, 32)
if flag.get("ch08.nangong") == 0 then flag.set("ch08.nangong", 1) end
