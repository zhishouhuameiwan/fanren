-- @hook ch13_tianxing npc_gu_laozhe npc
-- Facts and participants: docs/ch13-design.md 13.1 段 12。3.2 n12：付费 choice 只列付得起的支——
-- ① material_lingshi >= 10000 → take 10000；② material_yaodan_wuji >= 4 → take 4；③ 把柄 ＋ 胡月飞刀两柄（旁白，不扣物，永远在）。
-- 取消在扣物前 return；take false 落第三支；flag.set("ch13.toudu", 1/2/3) 写在顶格 flag.set("ch13.chuansong") 之前（第 11 节末表）；
-- 顶格 flag.set("ch13.chuansong")、teleport("ch13_waihai")（kScriptTransfers #3）。意图玩家走 ③。
-- 判据③：他传走之后顾、乾二人的事 0 处。「易敬」全名本段不出口（12.2），只称锦衣大汉。
if flag.get("ch13.chuansong") ~= 0 then talk("", "ch13.chuansong.gone"); return end
if flag.get("ch13.yirong") == 0 then talk("", "ch13.chuansong.early"); return end
talk("", "ch13.chuansong.hall")
talk("gu_laozhe", "ch13.chuansong.count")
talk("", "ch13.chuansong.cap")
talk("qian_xiushi", "ch13.chuansong.seat")
talk("", "ch13.chuansong.whisper")
talk("zhang_xiushi", "ch13.chuansong.yield")
talk("gu_laozhe", "ch13.chuansong.fee")
local can_stone = item.count("material_lingshi") >= 10000
local can_dan = item.count("material_yaodan_wuji") >= 4
local way = nil
if can_stone and can_dan then
    way = choice({"ch13.chuansong.opt_stone", "ch13.chuansong.opt_dan", "ch13.chuansong.opt_blade"})
elseif can_stone then
    local p = choice({"ch13.chuansong.opt_stone", "ch13.chuansong.opt_blade"})
    if p == 1 then way = 1 elseif p == 2 then way = 3 end
elseif can_dan then
    local p = choice({"ch13.chuansong.opt_dan", "ch13.chuansong.opt_blade"})
    if p == 1 then way = 2 elseif p == 2 then way = 3 end
else
    local p = choice({"ch13.chuansong.opt_blade"})
    if p == 1 then way = 3 end
end
if not way then return end
if way == 1 and not take("material_lingshi", 10000) then way = 3 end
if way == 2 and not take("material_yaodan_wuji", 4) then way = 3 end
if way == 1 then
    talk("", "ch13.chuansong.paid_stone")
elseif way == 2 then
    talk("", "ch13.chuansong.paid_dan")
else
    talk("", "ch13.chuansong.paid_blade")
end
talk("", "ch13.chuansong.unseal")
talk("", "ch13.chuansong.stele")
talk("", "ch13.chuansong.go")
flag.set("ch13.toudu", way)
flag.set("ch13.chuansong")
teleport("ch13_waihai")
