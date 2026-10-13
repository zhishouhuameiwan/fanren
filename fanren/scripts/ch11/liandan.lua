-- @hook ch11_dongfu trigger_liandan interact once
-- Facts and participants: docs/ch11-design.md 13.1 n15. 散功只写旁白，不 realm.demote。
talk("", "ch11.liandan.shut")
advance_days(720)
talk("", "ch11.liandan.pills")
give("material_lingshi_zhong", 30)
talk("", "ch11.liandan.sell")
talk("", "ch11.liandan.move")
talk("", "ch11.liandan.worms")
talk("qu_hun_shadan", "ch11.liandan.ask")
talk("hanli", "ch11.liandan.sangong")
advance_days(15)
talk("", "ch11.liandan.fall")
flag.set("ch11.sangong")
