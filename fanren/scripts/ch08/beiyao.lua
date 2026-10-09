-- @hook ch08_dongfu trigger_beiyao interact
if flag.get("ch08.gufang") == 0 or flag.get("ch08.biguan") ~= 0 then return end
if item.count("pill_lianqi_san") >= 2 or
   (item.count_aged("herb_zishen_cao", 100) > 0 and item.count("material_lianqi_yao") > 0) then
    talk("", "ch08.beiyao.enough")
    return
end
talk("", "ch08.beiyao.ask")
local pick = choice({"ch08.beiyao.work", "ch08.beiyao.leave"})
if pick ~= 1 then return end
advance_days(20)
if item.count_aged("herb_zishen_cao", 100) == 0 then give("herb_zishen_cao", 1, 100) end
if item.count("material_lianqi_yao") == 0 then give("material_lianqi_yao", 1) end
flag.set("ch08.beiyao_bu", flag.get("ch08.beiyao_bu") + 1)
talk("", "ch08.beiyao.done")
