-- @hook ch08_dongfu trigger_huifu interact once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.shalang") == 0 or flag.get("ch08.huifu") ~= 0 then return end
local n = math.min(item.count("story_zhuluan"), 2)
if n > 0 and not take("story_zhuluan", n) then return end
give("story_xueyu_zhizhu", 1)
give("talisman_huoqiu_fu", 10)
talk("", "ch08.huifu.t1")
talk("", "ch08.huifu.t2")
advance_days(1)
talk("", "ch08.huifu.f1")
flag.set("ch08.huifu", 1)
