-- @hook ch08_tianxing_fangshi trigger_xulao interact once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.fangshi") == 0 or flag.get("ch08.xulao") ~= 0 then return end
talk("xulao_qipu", "ch08.xulao.t1")
talk("wang_ziling", "ch08.xulao.t2")
talk("xulao_qipu", "ch08.xulao.f1")
flag.set("ch08.xulao", 1)
