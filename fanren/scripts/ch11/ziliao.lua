-- @hook ch11_fangshi npc_baimen_zhanggui npc
-- Facts and participants: docs/ch11-design.md 13.1 n07.
if flag.get("ch11.ziliao") ~= 0 then talk("baimen_zhanggui", "ch11.ziliao.again"); return end
talk("baimen_zhanggui", "ch11.ziliao.greet")
talk("qu_hun_shadan", "ch11.ziliao.want")
talk("baimen_zhanggui", "ch11.ziliao.price")
if take("material_lingshi_zhong", 1) then
    talk("", "ch11.ziliao.paid")
elseif take("material_lingshi", 100) then
    talk("", "ch11.ziliao.paid_low")
else
    talk("baimen_zhanggui", "ch11.ziliao.credit")
end
give("story_yaoshou_ziliao", 1)
talk("", "ch11.ziliao.copy")
talk("", "ch11.ziliao.leave")
flag.set("ch11.ziliao")
