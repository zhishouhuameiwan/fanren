-- 夹具：t_npc_visible 上 (6,1) 那一格的前一位（带 hidden_flag，旗标置上前在场）。
--
-- 他和 npc_late 同格，各带互补的旗标 —— 一个 npc 对象只挂得住一个 script，
-- 「同一个位置在不同章节由不同的人站着」只能这么写。他在对象表里排在前面，
-- 从前会把 npc_late 永久压死。
flag.set("t.early_spoke")
