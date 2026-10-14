-- @hook ch12_binghuo trigger_jingzhu interact once
-- Facts and participants: docs/ch12-design.md 13.1 n22. 3.2：领炼晶；置 ch12.lianjing。「炼晶」「玄黄镜」从本段起解禁（12.2）。
talk("", "ch12.lianjing.way")
talk("", "ch12.lianjing.pillar")
talk("yuan_yao_jiedan", "ch12.lianjing.yuanyao")
talk("", "ch12.lianjing.eat")
give("material_lianjing", 1)
talk("", "ch12.lianjing.what")
talk("", "ch12.lianjing.keep")
flag.set("ch12.lianjing")
