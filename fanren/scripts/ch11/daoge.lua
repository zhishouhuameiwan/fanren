-- @hook ch11_yinsha_dao trigger_daoge enter once
-- Facts and participants: docs/ch11-design.md 13.1 n24. 胜写「困住」；败 game_over。
talk("", "ch11.daoge.turn")
talk("", "ch11.daoge.scatter")
talk("", "ch11.daoge.shadan")
talk("", "ch11.daoge.twist")
talk("qu_hun_shadan", "ch11.daoge.qu")
talk("hanli", "ch11.daoge.order")
local won = battle("b11_tiandu_shi")
if not won then game_over(); return end
talk("", "ch11.daoge.trapped")
talk("", "ch11.daoge.flee")
advance_days(30)
talk("", "ch11.daoge.month")
talk("", "ch11.daoge.fu")
flag.set("ch11.yaoshi")
teleport("ch11_dongfu")
