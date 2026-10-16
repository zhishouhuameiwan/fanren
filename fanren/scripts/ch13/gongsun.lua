-- @hook ch13_wuzhong npc_gongsun_xing npc
-- Facts and participants: docs/ch13-design.md 13.1 段 29。3.2 n29：已置 → 一句 return；前置 ch13.chuguan 开头判；
-- choice 回话（两支只换一句后合流）；choice 至元丹从哪出（只列有的）：① pill_zhiyuan_dan >= 3 → take 3 记 1；
-- ② material_yaodan_liuji >= 1 → take 1 记 2；③ advance_days(30)（开炉一月）记 3（永远在）；take false 落第三支；
-- 取消在扣物、置旗前 return；advance_days(3)（3.3 两日后）；末尾依次置 ch13.shouchao、flag.set("ch13.zhiyuan", n)、ch13.gongsun
-- （shouchao、zhiyuan 必在 gongsun 之前，第 11 节末表）。改编尺度最严（13 第 9 条、16.1 第 21 条）：唯一的约是「入阵为仆」。
-- 兽潮只经孙师叔之口（判据③，E 表 23）；「公孙杏」「青灵玄功」「金真功」「缠玉决」自本段起（12.2）。
if flag.get("ch13.gongsun") ~= 0 then talk("", "ch13.gongsun.gone"); return end
if flag.get("ch13.chuguan") == 0 then talk("", "ch13.gongsun.early"); return end
talk("sun_shishu", "ch13.gongsun.tide")
talk("sun_shishu", "ch13.gongsun.tide2")
talk("", "ch13.gongsun.kneel")
talk("gongsun_xing", "ch13.gongsun.plea")
talk("hanli", "ch13.gongsun.cause")
talk("sun_shishu", "ch13.gongsun.cause2")
local reply = choice({"ch13.gongsun.opt_terms", "ch13.gongsun.opt_test"})
if not reply then return end
if reply == 2 then talk("hanli", "ch13.gongsun.test") end
talk("hanli", "ch13.gongsun.terms")
local can_pill = item.count("pill_zhiyuan_dan") >= 3
local can_dan = item.count("material_yaodan_liuji") >= 1
local way = nil
if can_pill and can_dan then
    way = choice({"ch13.gongsun.opt_pill", "ch13.gongsun.opt_dan", "ch13.gongsun.opt_furnace"})
elseif can_pill then
    local p = choice({"ch13.gongsun.opt_pill", "ch13.gongsun.opt_furnace"})
    if p == 1 then way = 1 elseif p == 2 then way = 3 end
elseif can_dan then
    local p = choice({"ch13.gongsun.opt_dan", "ch13.gongsun.opt_furnace"})
    if p == 1 then way = 2 elseif p == 2 then way = 3 end
else
    local p = choice({"ch13.gongsun.opt_furnace"})
    if p == 1 then way = 3 end
end
if not way then return end
if way == 1 and not take("pill_zhiyuan_dan", 3) then way = 3 end
if way == 2 and not take("material_yaodan_liuji", 1) then way = 3 end
if way == 1 then
    talk("", "ch13.gongsun.pill")
    talk("sun_shishu", "ch13.gongsun.got_pill")
elseif way == 2 then
    talk("", "ch13.gongsun.dan")
    talk("sun_shishu", "ch13.gongsun.got_dan")
else
    advance_days(30)
    talk("", "ch13.gongsun.furnace")
    talk("sun_shishu", "ch13.gongsun.got_furnace")
end
advance_days(3)
talk("", "ch13.gongsun.chart")
talk("", "ch13.gongsun.note")
talk("", "ch13.gongsun.why")
flag.set("ch13.shouchao")
flag.set("ch13.zhiyuan", way)
flag.set("ch13.gongsun")
