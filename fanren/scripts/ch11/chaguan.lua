-- @hook ch11_fangshi npc_fan_furen npc
-- Facts and participants: docs/ch11-design.md 13.1 n20. 雅间对象 ch11.zhuzhan 起退场（18.4 第 7 条），n25 不经这里；演过之后一句打发。
if flag.get("ch11.chaguan") ~= 0 then talk("fan_furen", "ch11.chaguan.wait"); return end
talk("", "ch11.chaguan.room")
talk("fan_furen", "ch11.chaguan.greet")
talk("zhao_zhanglao", "ch11.chaguan.zhao")
talk("", "ch11.chaguan.probe")
talk("qu_hun_shadan", "ch11.chaguan.block")
talk("fan_furen", "ch11.chaguan.deal")
local had = item.count("story_yaoshou_cailiao") > 0
if had and not take("story_yaoshou_cailiao", 1) then talk("", "ch11.error.take"); return end
give("material_lingshi_zhong", 40)
if had then
    talk("", "ch11.chaguan.paid")
else
    talk("", "ch11.chaguan.paid_odd")
end
talk("fan_furen", "ch11.chaguan.bamboo")
talk("hanli", "ch11.chaguan.ask")
talk("fan_furen", "ch11.chaguan.robbed")
talk("", "ch11.chaguan.leave")
flag.set("ch11.chaguan")
