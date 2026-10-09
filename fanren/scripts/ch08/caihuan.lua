-- @hook ch08_yanlingbao trigger_caihuan interact once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.biwu") == 0 or flag.get("ch08.caihuan") ~= 0 then return end
local old = flag.get("ch05.jianmianli")
if old == 1 then
talk("mo_caihuan", "ch08.caihuan.t1")
elseif old == 2 then
talk("mo_caihuan", "ch08.caihuan.t2")
else
talk("mo_caihuan", "ch08.caihuan.t3")
end
talk("yan_shi", "ch08.caihuan.t4")
local n = math.min(item.count("pill_qingling_san"), 1)
if n > 0 and not take("pill_qingling_san", n) then return end
talk("", "ch08.caihuan.t5")
talk("mo_caihuan", "ch08.caihuan.f2")
local pick = choice({"ch08.caihuan.choice1", "ch08.caihuan.choice2"})
if not pick then return end
flag.set("ch08.caihuan", pick)
if pick == 1 then
    if not take("pill_dingyan_dan", 1) then return end
end
local n = math.min(item.count("material_lingshi"), 40)
if n > 0 and not take("material_lingshi", n) then return end
advance_days(1)
talk("", "ch08.caihuan.t6")
talk("", "ch08.caihuan.f1")
talk("", "ch08.caihuan.f3")
if flag.get("ch08.caihuan") == 0 then flag.set("ch08.caihuan", 1) end
