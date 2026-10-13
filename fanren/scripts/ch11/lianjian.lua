-- @hook ch11_dongfu trigger_lianjian enter once
-- Facts and participants: docs/ch11-design.md 13.1 n30（面板前的蒙太奇）。主旗 ch11.lianjian 由 chujian.lua 置。
talk("", "ch11.lianjian.sit")
talk("", "ch11.lianjian.liquid")
talk("", "ch11.lianjian.array")
talk("", "ch11.lianjian.day")
advance_days(120)
talk("", "ch11.lianjian.fire")
talk("", "ch11.lianjian.panel")
flag.set("ch11.lianjian_qian")
