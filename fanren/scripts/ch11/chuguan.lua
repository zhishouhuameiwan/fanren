-- @hook ch11_dongfu trigger_chuguan enter once
-- Facts and participants: docs/ch11-design.md 13.1 n17. E27：自此外人直接与韩立说话。
talk("", "ch11.chuguan.out")
talk("", "ch11.chuguan.life")
talk("", "ch11.chuguan.sword")
talk("", "ch11.chuguan.worms")
talk("", "ch11.chuguan.spider")
talk("qu_hun_shadan", "ch11.chuguan.qu")
talk("hanli", "ch11.chuguan.reply")
flag.set("ch11.chuguan")
