-- @hook ch08_yuejing trigger_wangfu interact once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.qb_mengshan") == 0 or flag.get("ch08.qb_shouyan") ~= 0 then return end
talk("", "ch08.wangfu.t1")
talk("", "ch08.wangfu.t2")
flag.set("ch08.xiao_qiu", 1)
talk("", "ch08.wangfu.f1")
flag.set("ch08.qb_shouyan", 1)
