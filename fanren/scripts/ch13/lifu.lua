-- @hook ch13_wuzhong trigger_lifu enter once
-- Facts and participants: docs/ch13-design.md 13.1 段 17。3.2 n17：advance_days(10)（3.3 六七日炼阵旗阵盘、开府）；
-- field.unlock("field_wuzhong", 6)；give herb_nichang_cao 3；置 ch13.lifu。
-- field.unlock 在 api.lua 里不回传结果（只发命令），3.2「unlock false 换一句」无从判，记施工偏差。
-- 落点 andeng 不触发 enter：这条带在登岸礁北面。往返黑石城自此开（3.1 #6、#7），文案只说码头，不点名按键。
talk("", "ch13.lifu.bird")
talk("", "ch13.lifu.fish")
talk("", "ch13.lifu.vein")
talk("", "ch13.lifu.cave")
talk("", "ch13.lifu.mist")
advance_days(10)
field.unlock("field_wuzhong", 6)
give("herb_nichang_cao", 3)
talk("", "ch13.lifu.rooms")
talk("", "ch13.lifu.dock")
flag.set("ch13.lifu")
