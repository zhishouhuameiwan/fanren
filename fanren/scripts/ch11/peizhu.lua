-- @hook ch11_dongfu trigger_peizhu interact
-- Facts and participants: docs/ch11-design.md 13.1 n26、n29. 同一触发器，ch11.jinlei 后换分支。
-- n26：三滴（白 → 雷电变蓝 → 金），第一根在这里计数；n29：第 2–6 根，第六根补足到开店起 7200 日。
if flag.get("ch11.liugen") ~= 0 then talk("", "ch11.peizhu.done"); return end
if flag.get("ch11.jinlei") == 0 then
    if not bottle.spend(1) then talk("", "ch11.peizhu.wait"); return end
    local zhu = flag.get("ch11.zhu") + 1
    flag.set("ch11.zhu", zhu)
    if zhu == 1 then talk("", "ch11.peizhu.white"); return end
    if zhu == 2 then talk("", "ch11.peizhu.blue"); return end
    talk("", "ch11.peizhu.gold")
    talk("", "ch11.peizhu.sword")
    if item.count("material_tianleizhu") > 0 and not take("material_tianleizhu", 1) then talk("", "ch11.error.take"); return end
    give("material_jinleizhu", 1)
    flag.set("ch11.zhu_gen", 1)
    talk("hanli", "ch11.peizhu.plan")
    local zhong = item.count("material_lingshi_zhong")
    if zhong > 0 and not take("material_lingshi_zhong", zhong) then talk("", "ch11.error.take"); return end
    local dan = item.count("material_yaodan_wuji")
    if dan > 0 and not take("material_yaodan_wuji", dan) then talk("", "ch11.error.take"); return end
    advance_days(720)
    talk("", "ch11.peizhu.fuliao")
    give("material_lianjian_fuliao", 1)
    talk("", "ch11.peizhu.ready")
    flag.set("ch11.jinlei")
    return
end
if flag.get("ch11.kaidian") == 0 then talk("", "ch11.peizhu.shop_first"); return end
if not bottle.spend(1) then talk("", "ch11.peizhu.wait"); return end
advance_days(1080)
local gen = flag.get("ch11.zhu_gen") + 1
flag.set("ch11.zhu_gen", gen)
give("material_jinleizhu", 1)
talk("", "ch11.peizhu.gen")
if gen < 6 then return end
talk("", "ch11.peizhu.sixth")
local d = math.max(360, 7200 - (today() - flag.get("ch11.kaidian_ri")))
if d > 3650 then
    advance_days(3600)
    d = d - 3600
end
advance_days(d)
talk("", "ch11.peizhu.close")
give("material_lingshi_zhong", 20)
talk("", "ch11.peizhu.sold")
flag.set("ch11.liugen")
