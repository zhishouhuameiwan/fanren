-- @hook ch12_lingquan npc_yuan_yao_lq npc
-- Facts and participants: docs/ch12-design.md 13.1 n33. 3.2：开头 ch12.done 已置 → return（元瑶章末不撤，留一句）；
-- 领万年灵乳、养魂木根须；advance_days(3)；ch12.xinjing 各位多一句；flag.set("ch12.done")。
-- 章末停在鼎盖：不 ending()（拍板 24）；末行 teleport("ch13_nanming") 是第 13 章补的（ch13 施工图 16.1 第 1 条）。
if flag.get("ch12.done") ~= 0 then talk("", "ch12.lingquan.again"); return end
talk("", "ch12.lingquan.pool")
talk("", "ch12.lingquan.her")
talk("", "ch12.lingquan.door")
talk("yuan_yao_jiedan", "ch12.lingquan.offer")
talk("hanli", "ch12.lingquan.ask")
talk("yuan_yao_jiedan", "ch12.lingquan.tell")
talk("hanli", "ch12.lingquan.want")
talk("yuan_yao_jiedan", "ch12.lingquan.deal")
advance_days(3)
give("story_wannian_lingru", 1)
give("story_yanghun_mugen", 1)
talk("", "ch12.lingquan.break")
talk("", "ch12.lingquan.part")
local xinjing = flag.get("ch12.xinjing")
if xinjing % 2 == 1 then talk("", "ch12.lingquan.se") end
if math.floor(xinjing / 2) % 2 == 1 then talk("", "ch12.lingquan.ju") end
talk("", "ch12.lingquan.ding")
talk("", "ch12.lingquan.ruyi")
talk("", "ch12.lingquan.plan")
flag.set("ch12.done")
teleport("ch13_nanming")
