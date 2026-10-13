-- @hook ch11_fangshi npc_baiyi_xiushi npc
-- Facts and participants: docs/ch11-design.md 13.1 n02.
if flag.get("ch11.zhihuan") ~= 0 then return end
talk("baiyi_xiushi", "ch11.zhihuan.greet")
talk("qu_hun_shadan", "ch11.zhihuan.answer")
talk("baiyi_xiushi", "ch11.zhihuan.free")
talk("baiyi_xiushi", "ch11.zhihuan.rule")
talk("qu_hun_shadan", "ch11.zhihuan.choose")
if take("material_lingshi_zhong", 8) then
    talk("", "ch11.zhihuan.red")
    flag.set("ch11.zhihuan", 1)
elseif take("material_lingshi", 3) then
    talk("", "ch11.zhihuan.blue")
    flag.set("ch11.zhihuan", 2)
else
    talk("baiyi_xiushi", "ch11.zhihuan.owe")
    talk("", "ch11.zhihuan.blue_owe")
    flag.set("ch11.zhihuan", 2)
end
talk("", "ch11.zhihuan.sky")
