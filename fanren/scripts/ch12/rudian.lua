-- @hook ch12_dadian trigger_rudian enter once
-- Facts and participants: docs/ch12-design.md 13.1 n08. 3.2：advance_days(8)；无门闸（大厅无门，门闸已前移到 n07，16.1 第 44 条）。
talk("", "ch12.rudian.sky")
talk("", "ch12.rudian.wait")
advance_days(8)
talk("", "ch12.rudian.inject")
talk("", "ch12.rudian.gate")
talk("", "ch12.rudian.corridor")
talk("", "ch12.rudian.hall")
talk("", "ch12.rudian.sit")
flag.set("ch12.rudian")
