-- @hook ch08_yuejing trigger_huangmiao enter once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.qb_wumei") == 0 or flag.get("ch08.qb_huangmiao") ~= 0 then return end
talk("", "ch08.huangmiao.t1")
talk("", "ch08.huangmiao.t2")
advance_days(7)
talk("", "ch08.huangmiao.f1")
talk("", "ch08.huangmiao.f2")
flag.set("ch08.qb_huangmiao", 1)
