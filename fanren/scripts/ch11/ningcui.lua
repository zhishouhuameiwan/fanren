-- @hook ch11_ningcui_jie trigger_ningcui enter once
-- Facts and participants: docs/ch11-design.md 13.1 n10.
talk("", "ch11.ningcui.house")
talk("", "ch11.ningcui.street")
talk("", "ch11.ningcui.invite")
talk("qu_hun_shadan", "ch11.ningcui.refuse")
talk("", "ch11.ningcui.hao")
talk("hao_yuantian", "ch11.ningcui.hao_offer")
talk("qu_hun_shadan", "ch11.ningcui.hao_decline")
talk("", "ch11.ningcui.rule")
talk("zahuo_nc", "ch11.ningcui.map_offer")
local pick = choice({"ch11.ningcui.opt_buy", "ch11.ningcui.opt_skip"})
if pick == 1 and take("material_lingshi_zhong", 1) then
    give("story_waihai_tu", 1)
    talk("", "ch11.ningcui.bought")
elseif pick == 1 then
    talk("", "ch11.ningcui.poor")
else
    talk("", "ch11.ningcui.skip")
end
flag.set("ch11.ningcui")
