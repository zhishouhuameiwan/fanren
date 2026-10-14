-- @hook ch12_binghuo trigger_guangbi enter once
-- Facts and participants: docs/ch12-design.md 13.1 n20. 3.2：先记 ch12.binghuo_ri = today()，再 advance_days(1)；
-- 两处「罗烟步（不拨日）/ 常速（+1 日）」；置 ch12.rongyan。永不超时（拍板 12）。
-- 两处抉择都在拨日之后：取消按罗烟步一支走（不 return），免得重踩时把 binghuo_ri 往后挪、再多拨一日。
-- Z1 取了辟火宝衣的，旁白多一句（第 6 节）；「玉真人」「辟火宝衣」在本段只此一句（12.2）。
flag.set("ch12.binghuo_ri", today())
advance_days(1)
talk("", "ch12.rongyan.wall")
talk("", "ch12.rongyan.rule")
if item.count("story_bihuo_baoyi") >= 1 then talk("", "ch12.rongyan.bihuo") end
talk("", "ch12.rongyan.grass")
local first = choice({"ch12.rongyan.opt_luo1", "ch12.rongyan.opt_slow1"})
if first == 2 then
    talk("", "ch12.rongyan.slow1")
    advance_days(1)
else
    talk("", "ch12.rongyan.luo1")
end
talk("", "ch12.rongyan.swamp")
local second = choice({"ch12.rongyan.opt_luo2", "ch12.rongyan.opt_slow2"})
if second == 2 then
    talk("", "ch12.rongyan.slow2")
    advance_days(1)
else
    talk("", "ch12.rongyan.luo2")
end
talk("", "ch12.rongyan.tree")
flag.set("ch12.rongyan")
