-- @hook ch08_yanlingbao trigger_xifeng enter once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.tianheju") == 0 or flag.get("ch08.tuoshen") ~= 0 then return end
if not magic.learn("magic_ji_wulongduo") then return end
if not magic.learn("magic_ji_xiaodao") then return end
if item.count("story_wulong_duo") == 0 then give("story_wulong_duo", 1) end
talk("", "ch08.xifeng.f1")
talk("", "ch08.xifeng.t1")
talk("guiling_shaozhu", "ch08.xifeng.t2")
talk("", "ch08.xifeng.t3")
talk("guiling_shaozhu", "ch08.xifeng.f2")
local won = battle("b08_guiling_shaozhu")
if not won then
    game_over()
    return
end
give("story_jin_kuloutou", 1)
talk("", "ch08.xifeng.t4")
talk("", "ch08.xifeng.f3")
talk("xuan_le", "ch08.xifeng.t5")
advance_days(6)
teleport("ch08_lingkuang", 8, 34)
flag.set("ch08.tuoshen", 1)
