-- @hook ch09_huangshan trigger_fuji interact once
-- Facts and participants: docs/ch09-design.md 13.1.

talk("", "ch09.fuji.cloud")
talk("", "ch09.fuji.miasma")
local n = math.min(1, item.count("story_yinhui_jian"))
if n > 0 and not take("story_yinhui_jian", n) then
    talk("", "ch09.fuji.error")
    return
end
talk("", "ch09.fuji.sword")
talk("", "ch09.fuji.fire")
talk("", "ch09.fuji.force")
talk("", "ch09.fuji.shield")
talk("", "ch09.fuji.ring")
talk("hongfen_kulou_nv", "ch09.fuji.track")
talk("", "ch09.fuji.pair")
talk("huang_shishu", "ch09.fuji.signal")
talk("", "ch09.fuji.weak")
local won = battle("b09_tuwei")
if not won then game_over() return end
talk("", "ch09.fuji.out")
talk("", "ch09.fuji.boat")
flag.set("ch09.tuwei")
teleport("ch09_huangshan", 29, 26)
