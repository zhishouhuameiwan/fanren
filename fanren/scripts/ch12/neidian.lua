-- @hook ch12_taqian trigger_taqian enter once
-- Facts and participants: docs/ch12-design.md 13.1 n26. 3.2：置 ch12.neidian，无分支。被斩的结丹修士只写倒下（13 第 7 条）。
talk("", "ch12.neidian.stand")
talk("xuan_gu", "ch12.neidian.xuangu")
talk("", "ch12.neidian.door")
talk("wan_tianming", "ch12.neidian.wan")
talk("", "ch12.neidian.cut")
talk("", "ch12.neidian.yuanyao")
talk("wu_chou", "ch12.neidian.wuchou")
talk("", "ch12.neidian.note")
flag.set("ch12.neidian")
