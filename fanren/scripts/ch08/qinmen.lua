-- @hook ch08_yuejing trigger_qinmen interact once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.yueding") == 0 or flag.get("ch08.qinzhai") ~= 0 then return end
talk("", "ch08.qinmen.t1")
talk("qin_gui", "ch08.qinmen.t2")
talk("qinfu_laozhe", "ch08.qinmen.t3")
talk("qin_yan", "ch08.qinmen.t4")
advance_days(1)
talk("qin_yan", "ch08.qinmen.f1")
flag.set("ch08.qinzhai", 1)
