-- @hook ch08_dongfu trigger_kaifu interact once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.lingquan") == 0 or flag.get("ch08.kaifu") ~= 0 then return end
talk("", "ch08.kaifu.t1")
talk("", "ch08.kaifu.t2")
local n = math.min(item.count("material_lingshi"), 12)
if n > 0 and not take("material_lingshi", n) then return end
local n = math.min(item.count("story_mizong_qi"), 5)
if n > 0 and not take("story_mizong_qi", n) then return end
give("herb_zigui_hua", 1, 1)
give("herb_zishen_cao", 3, 1)
give("herb_xuehong_zhi", 2, 1)
field.unlock("field_dongfu", 4)
advance_days(3)
talk("", "ch08.kaifu.t3")
talk("", "ch08.kaifu.f1")
flag.set("ch08.kaifu", 1)
