-- @hook ch08_dongfu trigger_quanyan interact once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.nuoyi") == 0 or flag.get("ch08.zhongqi") ~= 0 then return end
talk("", "ch08.quanyan.t1")
talk("", "ch08.quanyan.t2")
if not realm.advance(22) then return end
advance_days(7)
talk("", "ch08.quanyan.t3")
talk("", "ch08.quanyan.f1")
talk("", "ch08.quanyan.f2")
talk("", "ch08.quanyan.f3")
flag.set("ch08.zhongqi", 1)
