-- 夹具：t_npc_visible 上那个带 visible_flag 的 NPC（(3,1) 那一格）。
--
-- 只置一个旗标。测试要问的是「刚才是谁应的声」，而 interact() 只回一个 bool，
-- 分不出是谁 —— 旗标是这里唯一说得清的证物。
flag.set("t.ghost_spoke")
