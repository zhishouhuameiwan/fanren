-- @hook ch13_waihai trigger_mishi interact once
-- Facts and participants: docs/ch13-design.md 13.1 段 31。3.2 n31：shop("ch13_mishi_tan") 一次；advance_days(5)（3.3 三日后、交换会）；
-- 铜片三支只列付得起的：五级妖丹 >= 1 → take 1 记 1 / 灵石 >= 5000 → take 5000 记 2 / 以桌上材料抵（旁白不扣）记 3；take false 落第三支；
-- give story_tongpian 1；choice「以整堆材料换她自由 / 再看看」，「再看看」回选单再给一次、第二次只剩「换」；
-- take story_liuqi_cailiao 1（false 换一句照走）；choice 范夫人之邀（两支都是回绝）；flag.set("ch13.tongpian", n) 先于置 ch13.jiaohuan。
-- 取消只在第一次扣物（铜片）之前 return；之后的两处抉择取消按默认支走（换 / 回绝），免得已给的铜片重演一遍。
-- advance_days(5) 写在铜片抉择之后：取消再按时不重复拨日。
-- 尺度：女修作陪只此一句「各人身边都有年轻女修陪坐」，他与文思月之间的身体接触 0 处（13 第 10 条）。
-- 云天啸重伤未死、只写倒地起不来（13 第 12 条）；交换会之后的事 0 处（判据③）。
talk("", "ch13.mishi.enter")
talk("huang_mingli", "ch13.mishi.tingfeng")
shop("ch13_mishi_tan")
talk("", "ch13.mishi.search")
talk("", "ch13.mishi.invite")
talk("", "ch13.mishi.table")
talk("", "ch13.mishi.copper")
talk("", "ch13.mishi.offer")
local can_dan = item.count("material_yaodan_wuji") >= 1
local can_stone = item.count("material_lingshi") >= 5000
local way = nil
if can_dan and can_stone then
    way = choice({"ch13.mishi.opt_dan", "ch13.mishi.opt_stone", "ch13.mishi.opt_pile"})
elseif can_dan then
    local p = choice({"ch13.mishi.opt_dan", "ch13.mishi.opt_pile"})
    if p == 1 then way = 1 elseif p == 2 then way = 3 end
elseif can_stone then
    local p = choice({"ch13.mishi.opt_stone", "ch13.mishi.opt_pile"})
    if p == 1 then way = 2 elseif p == 2 then way = 3 end
else
    local p = choice({"ch13.mishi.opt_pile"})
    if p == 1 then way = 3 end
end
if not way then return end
advance_days(5)
if way == 1 and not take("material_yaodan_wuji", 1) then way = 3 end
if way == 2 and not take("material_lingshi", 5000) then way = 3 end
if way == 1 then
    talk("", "ch13.mishi.paid_dan")
elseif way == 2 then
    talk("", "ch13.mishi.paid_stone")
else
    talk("", "ch13.mishi.paid_pile")
end
give("story_tongpian", 1)
talk("wen_siyue", "ch13.mishi.siyue")
talk("", "ch13.mishi.fire")
local free = choice({"ch13.mishi.opt_swap", "ch13.mishi.opt_wait"})
if free == 2 then
    talk("", "ch13.mishi.wait")
    choice({"ch13.mishi.opt_swap"})
end
talk("hanli", "ch13.mishi.swap")
talk("", "ch13.mishi.strike")
if take("story_liuqi_cailiao", 1) then
    talk("", "ch13.mishi.price")
else
    talk("", "ch13.mishi.price_fail")
end
talk("fan_furen", "ch13.mishi.fan")
local fan = choice({"ch13.mishi.opt_refuse", "ch13.mishi.opt_leave"})
if fan == 2 then
    talk("", "ch13.mishi.leave")
else
    talk("hanli", "ch13.mishi.refuse")
end
talk("", "ch13.mishi.out")
flag.set("ch13.tongpian", way)
flag.set("ch13.jiaohuan")
