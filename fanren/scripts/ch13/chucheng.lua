-- @hook ch13_waihai trigger_chucheng enter once
-- Facts and participants: docs/ch13-design.md 13.1 段 16。3.2 n16：give story_qiyuan_haitu 1；advance_days(14)（3.3 北飞十余日、转西两日）；
-- 顶格 flag.set("ch13.chucheng")、teleport("ch13_wuzhong")（kScriptTransfers #4，落 andeng）。
-- 银翼鸟、雾海留给 lifu（13.1 段 17）。
talk("", "ch13.chucheng.chart")
give("story_qiyuan_haitu", 1)
talk("", "ch13.chucheng.south")
talk("", "ch13.chucheng.north")
advance_days(14)
flag.set("ch13.chucheng")
teleport("ch13_wuzhong")
