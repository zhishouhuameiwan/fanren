-- @hook ch05_nancheng trigger_sipingbang interact once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.xiangjia") == 0 or flag.get("ch08.sunergou") ~= 0 then return end
local style = flag.get("ch05.shoufu")
if style == 1 then
talk("sun_ergou", "ch08.sipingbang.t1")
elseif style == 2 then
talk("sun_ergou", "ch08.sipingbang.t2")
else
talk("sun_ergou", "ch08.sipingbang.t3")
end
if flag.get("ch05.anpai") == 2 then
talk("sun_ergou", "ch08.sipingbang.t4")
end
local n = math.min(item.count("pill_qingling_san"), 1)
if n > 0 and not take("pill_qingling_san", n) then return end
talk("", "ch08.sipingbang.t5")
talk("sun_ergou", "ch08.sipingbang.t6")
talk("sun_ergou", "ch08.sipingbang.f1")
flag.set("ch08.sunergou", 1)
