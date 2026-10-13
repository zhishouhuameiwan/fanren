-- @hook ch11_fangshi npc_li_dianzhu npc
-- Facts and participants: docs/ch11-design.md 13.1 n27. ch11.kaidian_ri 只在这里写。
if flag.get("ch11.kaidian") ~= 0 then return end
talk("", "ch11.kaidian.why")
talk("li_dianzhu", "ch11.kaidian.greet")
talk("hanli", "ch11.kaidian.ask")
talk("li_dianzhu", "ch11.kaidian.price")
local n = math.min(item.count("material_lingshi"), 50)
if n > 0 and not take("material_lingshi", n) then talk("", "ch11.error.take"); return end
if n == 50 then
    talk("", "ch11.kaidian.paid")
else
    talk("", "ch11.kaidian.short")
end
talk("", "ch11.kaidian.rename")
talk("", "ch11.kaidian.mask")
talk("he_xianshi", "ch11.kaidian.he")
talk("hanli", "ch11.kaidian.reply")
flag.set("ch11.kaidian_ri", today())
flag.set("ch11.jiesuan_ri", today())
flag.set("ch11.kaidian")
