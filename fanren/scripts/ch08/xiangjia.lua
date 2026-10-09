-- @hook ch05_nancheng trigger_xiangjia interact once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.junling") == 0 or flag.get("ch08.xiangjia") ~= 0 then return end
talk("", "ch08.xiangjia.t1")
talk("", "ch08.xiangjia.f1")
flag.set("ch08.xiangjia", 1)
