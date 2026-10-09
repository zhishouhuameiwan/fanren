-- @hook ch08_yuejing trigger_jiulou_yj interact once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.fengwu") == 0 or flag.get("ch08.qb_mengshan") ~= 0 then return end
talk("", "ch08.jiulou_yj.t1")
advance_days(60)
talk("", "ch08.jiulou_yj.t2")
talk("", "ch08.jiulou_yj.f1")
flag.set("ch08.qb_mengshan", 1)
