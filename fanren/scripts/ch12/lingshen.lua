-- @hook ch12_xiaojie trigger_shishan interact once
-- Facts and participants: docs/ch12-design.md 13.1 n16. 3.2：用他的旗 / 换自备的旗 → ch12.huanqi 1 / 2；领九曲灵参；置 ch12.lingshen；取消 return。
-- 抉择在一切状态改动之前，取消即 return。「呲唬」「四象玄武阵」「麝兰兽」从本段起解禁（12.2）。
talk("", "ch12.lingshen.hill")
talk("xuan_gu", "ch12.lingshen.see")
talk("xuan_gu", "ch12.lingshen.net")
talk("xuan_gu", "ch12.lingshen.flags")
local pick = choice({"ch12.lingshen.opt_his", "ch12.lingshen.opt_mine"})
if not pick then return end
if pick == 1 then
    talk("", "ch12.lingshen.his")
    flag.set("ch12.huanqi", 1)
else
    talk("", "ch12.lingshen.mine")
    flag.set("ch12.huanqi", 2)
end
talk("xuan_gu", "ch12.lingshen.bait")
talk("", "ch12.lingshen.wait")
talk("", "ch12.lingshen.catch")
give("story_jiuqu_lingshen", 1)
talk("", "ch12.lingshen.hold")
flag.set("ch12.lingshen")
