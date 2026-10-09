-- @hook ch08_tianxing_fangshi trigger_quqi interact once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.qiyunxiao") == 0 or flag.get("ch08.quqi") ~= 0 then return end
local n = math.min(item.count("material_mojiao_cailiao"), 1)
if n > 0 and not take("material_mojiao_cailiao", n) then return end
local n = math.min(item.count("material_wugong_ke"), 3)
if n > 0 and not take("material_wugong_ke", n) then return end
advance_days(15)
talk("xulao_qipu", "ch08.quqi.t1")
talk("xulao_qipu", "ch08.quqi.f1")
give("story_shenfeng_zhou", 1)
give("story_qinghuo_zhang", 1)
give("talisman_mojiao_chujiao", 1)
if not magic.learn("magic_ji_qinghuozhang") then return end
talk("", "ch08.quqi.t2")
talk("", "ch08.quqi.t3")
advance_days(1)
talk("", "ch08.quqi.f2")
talk("", "ch08.quqi.f3")
teleport("ch08_dongfu", 38, 24)
flag.set("ch08.quqi", 1)
