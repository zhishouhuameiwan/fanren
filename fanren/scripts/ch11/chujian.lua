-- @hook ch11_dongfu trigger_chujian enter once
-- Facts and participants: docs/ch11-design.md 13.1 n30（面板后）。门闸：剑在身上。
if item.count("weapon_qingzhu_fengyun_jian") < 1 then return end
talk("", "ch11.chujian.months")
advance_days(60)
talk("", "ch11.chujian.body")
talk("", "ch11.chujian.years")
talk("qu_hun_shadan", "ch11.chujian.qu")
talk("hanli", "ch11.chujian.reply")
flag.set("ch11.lianjian")
