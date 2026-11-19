-- @hook lilian_jindi_waiwei trigger_lilian_chen enter once
-- 伙伴改版样板第 7 节：历练「禁地旧境」里陈师妹头一回开口。挂在外围副本落点前一格 (5,30)
-- （mode=enter，once=true，set_flag=ch07.lilian_chen，require_party=chen_qiaoqian）。
-- 内容审查 2026-11-18 第 6 条：历练要在玩家看得见的文字里落地——这是打坐时的神游、不是真事；
-- 她在正史里不知道是韩立救的她，还看不起他（ch199、ch273），口吻照 ch173 之后的冷淡。
-- 和南宫婉同队时多两句：两人在正史里没见过面，当作梦里遇见的人。

talk("", "lilian.jindi.chen.see")
talk("chen_shimei", "lilian.jindi.chen.say")
local her = nil
if party.has("nangong_wan_ch07") then
    her = "nangong_wan_ch07"
elseif party.has("nangong_wan") then
    her = "nangong_wan"
end
if her ~= nil then
    talk("chen_shimei", "lilian.jindi.chen.ask")
    talk(her, "lilian.jindi.chen.answer")
end

flag.set("ch07.lilian_chen")
