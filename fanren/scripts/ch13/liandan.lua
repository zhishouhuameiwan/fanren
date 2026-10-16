-- @hook ch13_wuzhong trigger_danfang interact once
-- Facts and participants: docs/ch13-design.md 13.1 段 26。3.2 n26：advance_days(15 ＋ 1080)（3.3 半月回府 ＋ 三年）；
-- 置 ch13.liandan（解锁 recipe_zhiyuan_dan）。炼不炼由玩家，不设门闸；不做「炼丹大师」数值。「至元丹」自本段起（12.2）。
talk("", "ch13.liandan.home")
advance_days(15)
talk("", "ch13.liandan.wall")
talk("", "ch13.liandan.three")
advance_days(1080)
talk("", "ch13.liandan.recipe")
flag.set("ch13.liandan")
