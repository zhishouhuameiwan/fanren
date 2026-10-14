-- @hook ch12_dadian npc_xuan_gu npc
-- Facts and participants: docs/ch12-design.md 13.1 n09. 3.2：开头 jiumeng 已置 → return；「再想」一句后回选单；取消 return（本段无状态改动）。
if flag.get("ch12.jiumeng") ~= 0 then return end
talk("xuan_gu", "ch12.jiumeng.voice")
talk("", "ch12.jiumeng.who")
talk("xuan_gu", "ch12.jiumeng.offer")
talk("xuan_gu", "ch12.jiumeng.term")
talk("", "ch12.jiumeng.ban")
talk("", "ch12.jiumeng.miao")
while true do
    local pick = choice({"ch12.jiumeng.opt_yes", "ch12.jiumeng.opt_think"})
    if not pick then return end
    if pick == 1 then break end
    talk("", "ch12.jiumeng.think")
end
talk("hanli", "ch12.jiumeng.yes")
talk("xuan_gu", "ch12.jiumeng.seal")
flag.set("ch12.jiumeng")
