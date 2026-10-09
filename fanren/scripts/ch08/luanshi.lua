-- @hook ch08_jiayuan_shanlin trigger_luanshi enter once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.lifu") == 0 or flag.get("ch08.bigong") ~= 0 then return end
give("story_yinhun_zhong", 1)
talk("", "ch08.luanshi.t1")
talk("", "ch08.luanshi.aura")
talk("hanli", "ch08.luanshi.t2")
talk("", "ch08.luanshi.t3")
local n = math.min(item.count("talisman_dingshen_fu"), 3)
if n > 0 and not take("talisman_dingshen_fu", n) then return end
talk("", "ch08.luanshi.oath")
talk("yulingzong_canhun", "ch08.luanshi.t4")
talk("yulingzong_canhun", "ch08.luanshi.t5")
talk("yulingzong_canhun", "ch08.luanshi.t6")
talk("", "ch08.luanshi.f1")
talk("", "ch08.luanshi.f2")
talk("yulingzong_canhun", "ch08.luanshi.f3")
flag.set("ch08.bigong", 1)
