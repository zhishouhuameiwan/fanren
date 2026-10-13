-- @hook ch11_dongfu trigger_kuilei interact once
-- Facts and participants: docs/ch11-design.md 13.1 n06.
talk("", "ch11.kuilei.start")
advance_days(180)
talk("", "ch11.kuilei.half")
talk("", "ch11.kuilei.bowl")
talk("", "ch11.kuilei.shut")
advance_days(720)
talk("", "ch11.kuilei.done")
give("story_kuilei_qun", 1)
talk("qu_hun_shadan", "ch11.kuilei.next")
talk("hanli", "ch11.kuilei.reply")
flag.set("ch11.kuilei")
