-- @hook ch08_lingkuang trigger_wangong interact once
-- Facts and participants: docs/ch09-design.md 13.1.
talk("", "ch09.wangong.shell")
talk("", "ch09.wangong.time")
talk("", "ch09.wangong.risk")
local n = math.min(3, item.count("material_heyuanyu"))
if n > 0 and not take("material_heyuanyu", n) then
    talk("", "ch09.wangong.error")
    return
end
advance_days(7)
talk("", "ch09.wangong.done")
flag.set("ch09.wangong")
