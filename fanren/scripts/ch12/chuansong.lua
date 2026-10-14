-- @hook ch12_dadian trigger_chuansongzhen interact once
-- Facts and participants: docs/ch12-design.md 13.1 n11. 3.2：advance_days(3)；置 ch12.chuansong；teleport("ch12_guiwu")。
-- kScriptTransfers 三元行：flag.set 与 teleport 写在顶格。
talk("", "ch12.chuansong.wait")
advance_days(3)
talk("", "ch12.chuansong.door")
talk("", "ch12.chuansong.array")
talk("", "ch12.chuansong.first")
talk("", "ch12.chuansong.look")
talk("", "ch12.chuansong.step")
flag.set("ch12.chuansong")
teleport("ch12_guiwu")
