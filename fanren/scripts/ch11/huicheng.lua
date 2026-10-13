-- @hook ch11_honghu trigger_huicheng interact once
-- Facts and participants: docs/ch11-design.md 13.1 n14. 门闸 ch11.lieyao >= 3 在这里判。
if flag.get("ch11.lieyao") < 3 then talk("qu_hun_shadan", "ch11.huicheng.more"); return end
talk("", "ch11.huicheng.count")
local pick = choice({"ch11.huicheng.opt_stop", "ch11.huicheng.opt_hunt"})
if pick ~= 1 then return end
talk("hanli", "ch11.huicheng.decide")
if item.count("story_kuilei_qun") > 0 and not take("story_kuilei_qun", 1) then talk("", "ch11.error.take"); return end
talk("", "ch11.huicheng.years")
talk("", "ch11.huicheng.puppets")
give("story_kuilei_can", 1)
give("material_yaodan_wuji", 8)
give("story_yaoshou_cailiao", 1)
talk("", "ch11.huicheng.loot")
advance_days(1800)
talk("", "ch11.huicheng.back")
flag.set("ch11.huicheng")
teleport("ch11_dongfu")
