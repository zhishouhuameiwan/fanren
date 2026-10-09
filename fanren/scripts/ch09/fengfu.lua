-- @hook ch08_dongfu trigger_fengfu interact once
-- Facts and participants: docs/ch09-design.md 13.1.

local ripe = field.ripe("field_dongfu") + field.ripe("field_dongfu_nei")
    + field.ripe("field_baiyaoyuan") + field.ripe("field_baiyaoyuan_jiao")
local planted = field.planted("field_dongfu") + field.planted("field_dongfu_nei")
    + field.planted("field_baiyaoyuan") + field.planted("field_baiyaoyuan_jiao")
local result = 1
if ripe > 0 then
    talk("", "ch09.fengfu.ripe")
    local pick
repeat pick = choice({"ch09.fengfu.opt1", "ch09.fengfu.opt2"}) until pick
    if pick == 1 then return end
    result = 2
elseif planted > 0 then
    talk("", "ch09.fengfu.young")
end
talk("", "ch09.fengfu.bag")
give("talisman_huoqiu_fu", 6)
talk("", "ch09.fengfu.spring")
talk("", "ch09.fengfu.shell")
if flag.get("ch08.quhun") == 2 then
    talk("", "ch09.fengfu.bell")
end
talk("", "ch09.fengfu.array")
talk("", "ch09.fengfu.collapse")
talk("", "ch09.fengfu.disciples")
talk("linghu_laozu", "ch09.fengfu.near")
talk("huang_shishu", "ch09.fengfu.order")
talk("", "ch09.fengfu.northeast")
if flag.get("ch08.xiao_tuijian") == 1 then
    talk("", "ch09.fengfu.aboard")
end
talk("", "ch09.fengfu.letter")
talk("huang_shishu", "ch09.fengfu.pursuit")
talk("", "ch09.fengfu.rear")
talk("huang_shishu", "ch09.fengfu.harass")
talk("", "ch09.fengfu.plan")
if flag.get("ch08.xiao_tuijian") == 1 then
    talk("", "ch09.fengfu.part")
end
advance_days(1)
flag.set("ch09.fengfu", result)
teleport("ch09_huangshan", 6, 5)
