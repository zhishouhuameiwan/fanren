-- @hook ch09_milin trigger_jiuren interact once
-- Facts and participants: docs/ch09-design.md 13.1.
talk("", "ch09.jiuren.rescue")
talk("", "ch09.jiuren.trap")
talk("dong_xuaner_hehuan", "ch09.jiuren.block")
talk("dong_xuaner_hehuan", "ch09.jiuren.woman")
local won = battle("b09_dong_xuaner")
if not won then game_over() return end
talk("", "ch09.jiuren.pass")
talk("", "ch09.jiuren.earth")
talk("", "ch09.jiuren.hours")
flag.set("ch09.jiuren")
teleport("ch09_milin", 37, 12)
