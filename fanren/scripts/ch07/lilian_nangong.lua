-- @hook lilian_jindi_waiwei trigger_lilian_nangong enter once
-- 伙伴改版样板第 7 节：历练「禁地旧境」里南宫婉头一回开口。挂在外围副本落点前两格 (5,29)
-- （mode=enter，once=true，set_flag=ch07.lilian_nangong，require_party=nangong_wan）。
-- 内容审查 2026-11-18 第 6 条：把「打坐时以神识重走旧地、这一趟不是真事」从她嘴里带出来。
-- 说话人用队伍里她此刻的那个角色（第 7 章里是 nangong_wan_ch07，第 8 章起是 nangong_wan）。

local her = "nangong_wan_ch07"
if party.has("nangong_wan") then
    her = "nangong_wan"
end
talk("", "lilian.jindi.nangong.see")
talk(her, "lilian.jindi.nangong.say")

flag.set("ch07.lilian_nangong")
