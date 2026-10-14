-- @hook ch12_hanlitai trigger_jitan enter once
-- Facts and participants: docs/ch12-design.md 13.1 n30. 3.2：「溜向台阶」被玄骨叫回，与「不动」合流；置 ch12.butian。
-- 本段无状态改动，取消即 return。补天丹不归他（落入蛮胡子手中）。「天罗真雷」「化身符」「贯日剑」从本段起解禁（12.2）。
talk("", "ch12.butian.ice")
talk("man_huzi", "ch12.butian.shout")
talk("", "ch12.butian.found")
talk("", "ch12.butian.strike")
talk("", "ch12.butian.fu")
talk("", "ch12.butian.dan")
local pick = choice({"ch12.butian.opt_slip", "ch12.butian.opt_stay"})
if not pick then return end
if pick == 1 then
    talk("", "ch12.butian.slip")
    talk("xuan_gu", "ch12.butian.call")
    talk("", "ch12.butian.slip_back")
else
    talk("", "ch12.butian.stay")
end
talk("", "ch12.butian.man")
talk("jiyin_zushi", "ch12.butian.order")
flag.set("ch12.butian")
