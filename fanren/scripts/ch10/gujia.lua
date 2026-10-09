-- @hook ch10_kuixing trigger_gujia interact once
if flag.get("ch10.gujia") ~= 0 or flag.get("ch10.chuhai") ~= 0 then return end
talk("", "ch10.gujia.house")
talk("", "ch10.gujia.old")
talk("gu_dongzhu_lao", "ch10.gujia.kai")
talk("", "ch10.gujia.end")
give("material_lingshi", 120)
flag.set("ch10.gujia")
