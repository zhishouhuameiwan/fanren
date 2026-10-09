-- @hook ch08_yuejing trigger_qiuling enter once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.qb_shouyan") == 0 or flag.get("ch08.qb_xuezhou") ~= 0 then return end
talk("", "ch08.qiuling.t1")
talk("mengshan_laoda", "ch08.qiuling.t2")
talk("", "ch08.qiuling.t3")
advance_days(5)
talk("", "ch08.qiuling.f1")
flag.set("ch08.qb_xuezhou", 1)
