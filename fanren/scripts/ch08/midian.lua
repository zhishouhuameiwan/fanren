-- @hook ch08_tianxing_fangshi trigger_midian interact once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.xulao") == 0 or flag.get("ch08.midian") ~= 0 then return end
talk("", "ch08.midian.arrive")
talk("zhang_furen", "ch08.midian.t1")
teleport("ch08_tianxing_fangshi", 37, 29)
talk("midian_zhuchi", "ch08.midian.t2")
talk("", "ch08.midian.t3")
talk("hutou_qingnian", "ch08.midian.t4")
talk("", "ch08.midian.t5")
talk("", "ch08.midian.f1")
talk("midian_zhuchi", "ch08.midian.f2")
talk("", "ch08.midian.f3")
flag.set("ch08.midian", 1)
