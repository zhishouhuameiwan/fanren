-- @hook ch08_dongfu trigger_biguan interact once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.shuye") == 0 or flag.get("ch08.biguan") ~= 0 then return end
if item.count("pill_lianqi_san") < 2 then
talk("", "ch08.biguan.t1")
return
end
if not take("pill_lianqi_san", 2) then return end
local n = math.min(item.count("pill_zhuji_dan"), 1)
if n > 0 and not take("pill_zhuji_dan", n) then return end
talk("", "ch08.biguan.t2")
advance_days(1440)
talk("", "ch08.biguan.t3")
talk("", "ch08.biguan.t4")
local n = item.count("pill_dingyan_dan")
if n < 7 then give("pill_dingyan_dan", 7 - n) end
if not take("pill_dingyan_dan", 1) then return end
give("talisman_huoqiu_fu", 20)
give("material_lingshi_zhong", 4)
field.unlock("field_dongfu_nei", 2)
talk("", "ch08.biguan.f1")
talk("", "ch08.biguan.f2")
flag.set("ch08.biguan", 1)
