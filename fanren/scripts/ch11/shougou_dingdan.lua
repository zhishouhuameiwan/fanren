-- @hook ch11_ningcui_jie npc_fan_xiaozi npc
-- Facts and participants: docs/ch11-design.md 13.1 Z1（改编：订单代替猎妖许可）。
if flag.get("ch11.ningcui") == 0 then talk("fan_xiaozi", "ch11.z1.busy"); return end
local z1 = flag.get("ch11.z1")
if z1 >= 2 then talk("fan_xiaozi", "ch11.z1.done"); return end
if z1 == 0 then
    talk("fan_xiaozi", "ch11.z1.offer")
    talk("fan_xiaozi", "ch11.z1.terms")
    local pick = choice({"ch11.z1.opt_take", "ch11.z1.opt_no"})
    if pick ~= 1 then talk("qu_hun_shadan", "ch11.z1.later"); return end
    talk("qu_hun_shadan", "ch11.z1.accept")
    flag.set("ch11.z1", 1)
    return
end
if item.count("material_yaodan_wuji") < 3 then talk("fan_xiaozi", "ch11.z1.short"); return end
if not take("material_yaodan_wuji", 3) then talk("", "ch11.error.take"); return end
give("material_lingshi_zhong", 10)
give("material_lianqi_yao", 2)
talk("fan_xiaozi", "ch11.z1.deliver")
talk("", "ch11.z1.reward")
flag.set("ch11.z1", 2)
