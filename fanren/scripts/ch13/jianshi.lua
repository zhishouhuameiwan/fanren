-- @hook ch13_waihai trigger_jiaoshi enter once
-- Facts and participants: docs/ch13-design.md 13.1 段 33。3.2 n33：读 ch13.tongpian 择一句；advance_days(180)（3.3 一月到深渊边缘、蹲守数月）；
-- 置 ch13.jianshi。资质只一句、不改数值（E 表 38）；「三头六臂」「妖修」自本段起（12.2）。
talk("", "ch13.jianshi.copper")
talk("", "ch13.jianshi.yaoxiu")
local paid = flag.get("ch13.tongpian")
if paid == 1 then
    talk("", "ch13.jianshi.cost_dan")
elseif paid == 2 then
    talk("", "ch13.jianshi.cost_stone")
else
    talk("", "ch13.jianshi.cost_pile")
end
talk("", "ch13.jianshi.root")
talk("", "ch13.jianshi.leave")
advance_days(180)
talk("", "ch13.jianshi.reef")
talk("", "ch13.jianshi.wait")
flag.set("ch13.jianshi")
