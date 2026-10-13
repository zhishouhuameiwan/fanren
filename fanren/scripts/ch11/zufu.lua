-- @hook ch11_fangshi npc_xinggong_zhishi npc
-- Facts and participants: docs/ch11-design.md 13.1 n04.
if flag.get("ch11.zufu") ~= 0 then return end
talk("xinggong_zhishi", "ch11.zufu.greet")
talk("qu_hun_shadan", "ch11.zufu.ask")
talk("xinggong_zhishi", "ch11.zufu.layers")
if flag.get("ch11.luer") == 2 then
    talk("xinggong_zhishi", "ch11.zufu.extra_fangshi")
    talk("xinggong_zhishi", "ch11.zufu.extra_rule")
end
talk("xinggong_zhishi", "ch11.zufu.list")
talk("xinggong_zhishi", "ch11.zufu.price")
talk("qu_hun_shadan", "ch11.zufu.pick")
talk("", "ch11.zufu.century")
local n = math.min(item.count("material_lingshi_zhong"), 27)
if n > 0 and not take("material_lingshi_zhong", n) then talk("", "ch11.error.take"); return end
if n == 27 then
    talk("", "ch11.zufu.paid")
elseif n > 0 then
    talk("xinggong_zhishi", "ch11.zufu.short")
else
    talk("xinggong_zhishi", "ch11.zufu.credit")
end
give("story_jinzhi_lingpai", 1)
talk("", "ch11.zufu.token")
if flag.get("ch11.zhihuan") == 2 then talk("", "ch11.zufu.ring") end
talk("", "ch11.zufu.up")
flag.set("ch11.zufu")
teleport("ch11_dongfu")
