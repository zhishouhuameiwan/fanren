-- @hook ch08_lingkuang trigger_zhenyan_jiu interact once
-- Facts and participants: docs/ch09-design.md 13.1.
if flag.get("ch09.zhenqi") ~= 15 then
    talk("", "ch09.zhenyan_jiu.missing")
    return
end
talk("", "ch09.zhenyan_jiu.occupation")
talk("", "ch09.zhenyan_jiu.entry")
talk("", "ch09.zhenyan_jiu.hide")
local n = math.min(2, item.count("material_lingshi_zhong"))
if n > 0 and not take("material_lingshi_zhong", n) then
    talk("", "ch09.zhenyan_jiu.error")
    return
end
if n < 2 then
    local low = math.min(2 - n, item.count("material_lingshi"))
    if low > 0 and not take("material_lingshi", low) then
        talk("", "ch09.zhenyan_jiu.error")
        return
    end
    talk("", "ch09.zhenyan_jiu.weak")
end
talk("", "ch09.zhenyan_jiu.ready")
flag.set("ch09.buzhen")
