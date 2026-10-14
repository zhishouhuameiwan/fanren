-- @hook ch12_binghuo trigger_shamo enter once
-- Facts and participants: docs/ch12-design.md 13.1 n21. 3.2：读 today() − ch12.binghuo_ri 只换台词（≥ 4 日一律「时限将尽」，不设失败）；
-- 「绕路」一句后回选单；谈妥即领啼魂兽、鸣魂珠（16.1 第 47 条）；战斗 ②；advance_days(1)；置 ch12.tihun；败 game_over。
-- 抉择在领物之前，取消即 return。「黑沙漠」「铁火蚁」「鸣魂珠」「青火雷」从本段起解禁（12.2）。
talk("", "ch12.tihun.sand")
local used = today() - flag.get("ch12.binghuo_ri")
if used <= 1 then
    talk("", "ch12.tihun.days_1")
elseif used == 2 then
    talk("", "ch12.tihun.days_2")
elseif used == 3 then
    talk("", "ch12.tihun.days_3")
else
    talk("", "ch12.tihun.days_4")
end
while true do
    local pick = choice({"ch12.tihun.opt_round", "ch12.tihun.opt_cross"})
    if not pick then return end
    if pick == 2 then break end
    talk("", "ch12.tihun.round")
end
talk("", "ch12.tihun.yellow")
talk("", "ch12.tihun.yellow_fall")
talk("", "ch12.tihun.ant")
talk("", "ch12.tihun.groups")
talk("yuan_yao_jiedan", "ch12.tihun.call")
talk("", "ch12.tihun.saved")
talk("yuan_yao_jiedan", "ch12.tihun.offer")
talk("hanli", "ch12.tihun.want")
give("story_tihun_shou", 1)
give("story_minghun_zhu", 1)
talk("", "ch12.tihun.handover")
talk("", "ch12.tihun.again")
local won = battle("b12_heishamo_tiehuoyi")
if not won then game_over(); return end
talk("", "ch12.tihun.win")
advance_days(1)
flag.set("ch12.tihun")
