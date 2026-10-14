-- @hook ch12_guiwu trigger_tingzi interact once
-- Facts and participants: docs/ch12-design.md 13.1 n15. 3.2：advance_days(1)；置 ch12.xiaojie；teleport("ch12_xiaojie")。
-- kScriptTransfers 三元行：flag.set 与 teleport 写在顶格。「冰火道」从本段起解禁（12.2）。
talk("xuan_gu", "ch12.xiaojie.voice")
talk("", "ch12.xiaojie.day")
advance_days(1)
talk("xinggong_zhifa_ci", "ch12.xiaojie.rule")
talk("", "ch12.xiaojie.go")
flag.set("ch12.xiaojie")
teleport("ch12_xiaojie")
talk("", "ch12.xiaojie.arrive")
talk("xuan_gu", "ch12.xiaojie.space")
