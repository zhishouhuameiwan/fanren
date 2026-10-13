-- @hook ch11_fangshi npc_lu_er npc
-- Facts and participants: docs/ch11-design.md 13.1 n03.
if flag.get("ch11.luer") ~= 0 then return end
talk("lu_er", "ch11.luer.greet")
talk("lu_er", "ch11.luer.offer")
talk("qu_hun_shadan", "ch11.luer.ask")
talk("lu_er", "ch11.luer.answer")
local pick = choice({"ch11.luer.opt_hire", "ch11.luer.opt_pass"})
if not pick then return end
if pick == 1 then
    local n = math.min(item.count("material_lingshi"), 5)
    if n > 0 and not take("material_lingshi", n) then talk("", "ch11.error.take"); return end
    talk("hanli", "ch11.luer.hire")
    talk("lu_er", "ch11.luer.thanks")
    talk("", "ch11.luer.guide")
    talk("", "ch11.luer.guide_rent")
    flag.set("ch11.luer", 1)
else
    talk("hanli", "ch11.luer.pass")
    talk("lu_er", "ch11.luer.leave")
    talk("", "ch11.luer.alone")
    flag.set("ch11.luer", 2)
end
