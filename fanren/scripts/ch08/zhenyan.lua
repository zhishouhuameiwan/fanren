-- @hook ch08_dongfu trigger_zhenyan interact once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.quqi") == 0 or flag.get("ch08.buzhen1") ~= 0 then return end
if flag.get("ch08.zhenqi1") ~= 15 then
talk("", "ch08.zhenyan.t1")
return
end
local n = math.floor(item.count("material_lingshi_zhong") / 2)
if n > 0 and not take("material_lingshi_zhong", n) then return end
if n > 0 then
    talk("", "ch08.zhenyan.t2")
else
    talk("", "ch08.zhenyan.t3")
end
flag.set("ch08.buzhen1", 1)
