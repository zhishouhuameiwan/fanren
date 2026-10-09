-- @hook ch08_tianxing_fangshi trigger_goucai interact once
-- Facts and participants: docs/ch09-design.md 13.1.
local n = math.min(60, item.count("material_lingshi"))
if n > 0 and not take("material_lingshi", n) then
    talk("", "ch09.goucai.error")
    return
end
if n < 60 then
    talk("", "ch09.goucai.short")
end
give("material_xiuzhen_liao", 1)
talk("", "ch09.goucai.list")
advance_days(5)
flag.set("ch09.goucai")
teleport("ch08_lingkuang", 39, 34)
