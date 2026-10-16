-- @hook ch13_nanming trigger_qiuling enter once
-- Facts and participants: docs/ch13-design.md 13.1 段 3。3.2 n03：choice「看下去 / 退回坊市」，两支只换一句；
-- 取消在置旗前 return（once 不兑现，再踏照响）；置 ch13.gongdao。
-- 挂点名 trigger_qiuling 只是地点：文案不写他藏在丘陵上（8.3）。「苍云龙」自本段起（12.2）。
talk("", "ch13.gongdao.dusk")
talk("", "ch13.gongdao.array")
talk("", "ch13.gongdao.who")
local pick = choice({"ch13.gongdao.opt_watch", "ch13.gongdao.opt_back"})
if not pick then return end
if pick == 1 then
    talk("", "ch13.gongdao.watch")
else
    talk("", "ch13.gongdao.back")
end
talk("", "ch13.gongdao.crowd")
talk("", "ch13.gongdao.break")
talk("", "ch13.gongdao.outpost")
talk("", "ch13.gongdao.notice")
talk("", "ch13.gongdao.wait")
flag.set("ch13.gongdao")
