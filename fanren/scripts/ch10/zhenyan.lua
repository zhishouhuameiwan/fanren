-- @hook ch10_xiaohuan trigger_zhenyan interact once
if flag.get("ch10.zhenqi") ~= 15 then talk("", "ch10.zhenyan.missing"); return end
talk("", "ch10.zhenyan.old")
talk("", "ch10.zhenyan.done")
flag.set("ch10.buzhen")
