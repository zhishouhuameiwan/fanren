-- @hook ch08_jiayuan_shanlin trigger_yindong interact once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.buzhen3") == 0 or flag.get("ch08.shalang") ~= 0 then return end
talk("", "ch08.yindong.t1")
local n = math.min(item.count("material_lingshi"), 20)
if n > 0 and not take("material_lingshi", n) then return end
advance_days(3)
give("material_tangbi", 2)
give("material_tangchi", 2)
give("story_yaolang_luan", 12)
give("story_qichong_yujian", 1)
talk("", "ch08.yindong.t2")
talk("", "ch08.yindong.t3")
advance_days(6)
teleport("ch08_dongfu", 38, 24)
talk("", "ch08.yindong.f1")
talk("", "ch08.yindong.f2")
flag.set("ch08.shalang", 1)
