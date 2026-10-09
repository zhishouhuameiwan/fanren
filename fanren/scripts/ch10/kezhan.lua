-- @hook ch10_tiandujie trigger_kezhan interact once
talk("", "ch10.kezhan.book")
talk("", "ch10.kezhan.weigh")
talk("", "ch10.kezhan.plan")
local pick = choice({"ch10.kezhan.opt_ask", "ch10.kezhan.opt_go"})
if not pick then return end
if pick == 1 then talk("", "ch10.kezhan.ask")
else talk("", "ch10.kezhan.go") end
advance_days(16)
talk("", "ch10.kezhan.board")
flag.set("ch10.jueding", pick)
teleport("ch10_haichuan", 21, 17)
