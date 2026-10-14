-- @hook ch12_xiaojie trigger_fanlian enter once
-- Facts and participants: docs/ch12-design.md 13.1 n17. 3.2：置 ch12.fanlian；ch12.huanqi == 1 多一句（阵罩被他催红）。
-- 绿影一战只作旁白（拍板 8）。
talk("", "ch12.fanlian.cloud")
talk("hanli", "ch12.fanlian.call")
talk("", "ch12.fanlian.bind")
if flag.get("ch12.huanqi") == 1 then talk("", "ch12.fanlian.red") end
talk("", "ch12.fanlian.bite")
talk("", "ch12.fanlian.lvying")
talk("hanli", "ch12.fanlian.terms")
talk("", "ch12.fanlian.box")
talk("xuan_gu", "ch12.fanlian.mark")
talk("", "ch12.fanlian.dark")
flag.set("ch12.fanlian")
