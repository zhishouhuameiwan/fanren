-- @hook ch11_dongfu trigger_zhaolan enter once
-- Facts and participants: docs/ch11-design.md 13.1 n18. 中阶灵石有几块扣几块。
talk("", "ch11.zhaolan.fu")
talk("", "ch11.zhaolan.refuse")
talk("", "ch11.zhaolan.puppet")
local n = item.count("material_lingshi_zhong")
if n > 0 and not take("material_lingshi_zhong", n) then talk("", "ch11.error.take"); return end
if n > 0 then
    talk("", "ch11.zhaolan.poor")
else
    talk("", "ch11.zhaolan.empty")
end
give("story_kuilei_yuan_qun", 1)
talk("", "ch11.zhaolan.yuan")
talk("qu_hun_shadan", "ch11.zhaolan.qu")
flag.set("ch11.zhaolan")
