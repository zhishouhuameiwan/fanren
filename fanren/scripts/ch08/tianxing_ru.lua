-- @hook ch08_tianxing_fangshi trigger_tianxing_ru enter once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.qiannian") == 0 or flag.get("ch08.fangshi") ~= 0 then return end
talk("", "ch08.tianxing_ru.t1")
talk("", "ch08.tianxing_ru.height")
talk("", "ch08.tianxing_ru.t2")
talk("", "ch08.tianxing_ru.f1")
flag.set("ch08.fangshi", 1)
