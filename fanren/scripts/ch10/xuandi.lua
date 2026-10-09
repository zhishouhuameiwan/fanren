-- @hook ch10_kuixing trigger_xuandi interact once
talk("", "ch10.xuandi.master")
talk("dengxiange_zhishi", "ch10.xuandi.promise")
talk("hanli", "ch10.xuandi.agree")
local pick
repeat
    talk("", "ch10.xuandi.book")
    pick = choice({"ch10.xuandi.opt_peak", "ch10.xuandi.opt_valley", "ch10.xuandi.opt_island"})
    if not pick then return end
    if pick == 1 then talk("dengxiange_zhishi", "ch10.xuandi.peak") end
    if pick == 2 then talk("", "ch10.xuandi.valley") end
until pick == 3
local old = math.min(item.count("story_lvse_yupai"), 1)
if old > 0 and not take("story_lvse_yupai", old) then return end
give("story_lanse_yupei")
talk("", "ch10.xuandi.form")
talk("dengxiange_zhishi", "ch10.xuandi.island")
talk("dengxiange_zhishi", "ch10.xuandi.thin")
talk("dengxiange_zhishi", "ch10.xuandi.return")
talk("hanli", "ch10.xuandi.owner")
give("story_xiaohuan_yujian")
talk("", "ch10.xuandi.deed")
talk("", "ch10.xuandi.secret")
local paid = math.min(item.count("material_lingshi"), 5)
if paid > 0 and not take("material_lingshi", paid) then return end
if paid < 5 then talk("", "ch10.xuandi.short") end
give("story_haiyu_tu")
talk("", "ch10.xuandi.leave")
advance_days(2)
flag.set("ch10.xuandi")
teleport("ch10_xiaohuan", 49, 36)
