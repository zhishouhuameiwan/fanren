-- @hook ch09_milin trigger_huicheng interact once
-- Facts and participants: docs/ch09-design.md 13.1.
if flag.get("ch09.heyuan") == 2 then
    local n = math.min(40, item.count("material_lingshi"))
    if n > 0 and not take("material_lingshi", n) then
        talk("", "ch09.huicheng.error")
        return
    end
    if n < 40 then
        talk("", "ch09.huicheng.barter")
    end
    give("material_heyuanyu", 3)
    talk("", "ch09.huicheng.detour")
    advance_days(6)
end
talk("", "ch09.huicheng.weak")
talk("", "ch09.huicheng.slow")
talk("", "ch09.huicheng.cave")
advance_days(2)
flag.set("ch09.huicheng")
teleport("ch08_lingkuang", 39, 34)
