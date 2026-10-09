-- @hook ch08_yuejing trigger_zhulin_zhenyan interact once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.qb_yuanbing") == 0 or flag.get("ch08.buzhen2") ~= 0 then return end
if flag.get("ch08.zhenqi2") ~= 15 then
talk("", "ch08.zhulin_zhenyan.t1")
return
end
talk("", "ch08.zhulin_zhenyan.t2")
advance_days(1)
teleport("ch08_yuejing", 45, 30)
flag.set("ch08.buzhen2", 1)
