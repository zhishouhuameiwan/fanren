-- @hook ch10_xiaohuan trigger_matou interact once
talk("", "ch10.matou.fishers")
talk("hanli", "ch10.matou.claim")
teleport("ch10_xiaohuan", 43, 29)
talk("xiaohuan_zhenzhang", "ch10.matou.town")
local paid = math.min(item.count("material_lingshi"), 6)
if paid > 0 and not take("material_lingshi", paid) then return end
if paid == 0 then talk("", "ch10.matou.zero")
elseif paid < 6 then talk("", "ch10.matou.short")
else talk("", "ch10.matou.paid") end
talk("xiaohuan_zhenzhang", "ch10.matou.mount")
flag.set("ch10.zhenzhang")
