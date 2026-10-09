-- @hook ch08_jinguyuan trigger_liesha interact
if flag.get("ch08.yinian") == 0 or flag.get("ch08.nangong") ~= 0 then return end
if flag.get("ch08.liesha") >= 3 then talk("", "ch08.liesha.end") return end
talk("", "ch08.liesha.start")
local won = battle("b08_jinguyuan_qianfeng")
if not won then talk("", "ch08.liesha.lost") return end
give("material_lingshi", 30)
advance_days(10)
flag.set("ch08.liesha", flag.get("ch08.liesha") + 1)
talk("", "ch08.liesha.won")
