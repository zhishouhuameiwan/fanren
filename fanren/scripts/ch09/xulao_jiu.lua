-- @hook ch08_tianxing_fangshi trigger_xulao_jiu interact once
-- Facts and participants: docs/ch09-design.md 13.1.

talk("", "ch09.xulao_jiu.crowd")
talk("", "ch09.xulao_jiu.mask")
talk("xulao_qipu", "ch09.xulao_jiu.know")
local n = math.min(2, item.count("material_tangbi"))
if n > 0 and not take("material_tangbi", n) then
    talk("", "ch09.xulao_jiu.error")
    return
end
n = math.min(2, item.count("material_tangchi"))
if n > 0 and not take("material_tangchi", n) then
    talk("", "ch09.xulao_jiu.error")
    return
end
talk("", "ch09.xulao_jiu.parts")
talk("hanli", "ch09.xulao_jiu.order")
talk("xulao_qipu", "ch09.xulao_jiu.day")
flag.set("ch09.weituo")
