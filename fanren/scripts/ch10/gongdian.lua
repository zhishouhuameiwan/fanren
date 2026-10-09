-- @hook ch10_kuixing trigger_gongdian interact once
talk("", "ch10.gongdian.market")
talk("", "ch10.gongdian.guards")
talk("", "ch10.gongdian.entry")
talk("baiyi_qingnian", "ch10.gongdian.guide")
talk("hanli", "ch10.gongdian.answer")
talk("leitai_laozhe", "ch10.gongdian.rules")
talk("", "ch10.gongdian.draw")
local pick = choice({"ch10.gongdian.opt_wait", "ch10.gongdian.opt_read"})
if not pick then return end
if pick == 1 then talk("", "ch10.gongdian.wait")
else talk("", "ch10.gongdian.read") end
talk("", "ch10.gongdian.first")
talk("", "ch10.gongdian.look")
flag.set("ch10.chouqian", pick)
