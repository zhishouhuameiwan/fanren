-- @hook ch08_jiayuan_shanlin trigger_shandong enter once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.bigong") == 0 or flag.get("ch08.yaolang") ~= 0 then return end
talk("", "ch08.shandong.bones")
talk("", "ch08.shandong.t1")
local n = math.min(item.count("story_hongsha"), 1)
if n > 0 and not take("story_hongsha", n) then return end
give("story_yinling_sha", 1)
talk("", "ch08.shandong.t2")
teleport("ch08_jiayuan_shanlin", 10, 28)
talk("", "ch08.shandong.f1")
flag.set("ch08.yaolang", 1)
