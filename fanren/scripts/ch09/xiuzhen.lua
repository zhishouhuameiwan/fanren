-- @hook ch08_lingkuang trigger_xiuzhen interact once
-- Facts and participants: docs/ch09-design.md 13.1.
local n = math.min(1, item.count("material_xiuzhen_liao"))
if n > 0 and not take("material_xiuzhen_liao", n) then
    talk("", "ch09.xiuzhen.error")
    return
end
talk("", "ch09.xiuzhen.fine")
advance_days(7)
talk("", "ch09.xiuzhen.spent")
talk("", "ch09.xiuzhen.market")
talk("", "ch09.xiuzhen.stay")
advance_days(1)
flag.set("ch09.xiuzhen")
teleport("ch09_milin", 6, 33)
