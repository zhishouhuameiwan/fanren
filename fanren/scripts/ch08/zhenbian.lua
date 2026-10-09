-- @hook ch08_lingkuang trigger_zhenbian interact once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.kuilei") == 0 or flag.get("ch08.shouzhen") ~= 0 then return end
talk("", "ch08.zhenbian.t1")
local won = battle("b08_lingkuang_shouzhen")
if not won then
    game_over()
    return
end
talk("", "ch08.zhenbian.t2")
teleport("ch08_lingkuang", 33, 29)
talk("", "ch08.zhenbian.f1")
talk("", "ch08.zhenbian.f2")
talk("", "ch08.zhenbian.f3")
flag.set("ch08.shouzhen", 1)
