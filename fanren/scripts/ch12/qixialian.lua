-- @hook ch12_dixue trigger_shuichi interact once
-- Facts and participants: docs/ch12-design.md 13.1 n04. 3.2：第一处二选一只换一句；第二处「再等」一句后回选单，直到展画轴。
-- 本段无状态改动，两处取消都在置旗之前 return。同行者只写倒下（拍板 22）。
talk("", "ch12.qixialian.hall")
talk("", "ch12.qixialian.lotus")
talk("hu_yue", "ch12.qixialian.huyue")
talk("jian_xiushi", "ch12.qixialian.jian")
local pick = choice({"ch12.qixialian.opt_stop", "ch12.qixialian.opt_watch"})
if not pick then return end
if pick == 1 then
    talk("hanli", "ch12.qixialian.stop")
    talk("", "ch12.qixialian.stop_after")
else
    talk("", "ch12.qixialian.watch")
end
talk("", "ch12.qixialian.fall")
talk("", "ch12.qixialian.snake")
talk("", "ch12.qixialian.bone")
talk("", "ch12.qixialian.mist")
while true do
    local again = choice({"ch12.qixialian.opt_wait", "ch12.qixialian.opt_scroll"})
    if not again then return end
    if again == 2 then break end
    talk("", "ch12.qixialian.wait")
end
talk("", "ch12.qixialian.scroll")
talk("", "ch12.qixialian.shidie")
talk("jin_qing", "ch12.qixialian.jinqing")
flag.set("ch12.qixialian")
