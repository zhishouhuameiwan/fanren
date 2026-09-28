-- @hook ch02_yaopu npc_yaopu_guanshi npc
-- 药圃管事的闲聊。挂在 ch02_yaopu 的 npc 对象上（role_id=yaopu_guanshi）。
--
-- 脚本无从得知是哪个 NPC 触发了它，所以每个说不同话的 NPC 各挂各的脚本。
-- 这条限制是第 1 章踩出来的，本章的七个闲聊 NPC 一律照此办理。
--
-- 他嘴里翻来覆去只有一件事：什么时候起。这正是灵田玩法唯一的决策点——
-- 采早了年份薄，采晚了白占一畦。教程不写在面板上，写在这个人的口头禅里。

if flag.get("ch02.liao_guanshi") == 1 then
    talk("yaopu_guanshi", "ch02.npc.guanshi_field")
    talk("yaopu_guanshi", "ch02.npc.guanshi_again")
    return
end

talk("yaopu_guanshi", "ch02.npc.guanshi_1")
talk("yaopu_guanshi", "ch02.npc.guanshi_2")

flag.set("ch02.liao_guanshi")
