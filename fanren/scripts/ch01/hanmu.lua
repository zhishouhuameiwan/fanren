-- @hook ch01_hanjiacun npc_han_mu npc
-- 第一章开场，没见三叔之前找娘说话：她只把人往三叔那儿推。
--
-- 这一位是村里的韩母（地图上 hidden_flag=ch01.sanshu_met）。见过三叔之后她天亮去了村口，
-- 在老槐树下的是另一个对象 npc_han_mu_cunkou，挂的是话别（cunkou_bie.lua）。
-- 从前村里这一位直接挂着话别脚本，而她从开局就在场：没见三叔先找娘，
-- 话别演完、ch01.muqin_bie 置上，出村的门就开了，节点 1 整个跳过。
--
-- 文案一律引 key，实际文字在 data/text/ch01_main.json。

talk("han_mu", "ch01.cunkou.mu_wait")
