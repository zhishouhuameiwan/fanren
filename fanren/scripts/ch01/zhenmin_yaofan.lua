-- @hook ch01_qingniuzhen npc_zhen_min_yaofan npc
-- 第一章节点 3 的闲聊 NPC 之二：镇上卖伤药的。
--
-- 他嘴里带出「谷中有位坐堂大夫」，是墨大夫在本章的第一次间接出场。
-- 这里必须是好话且合情理：一个卖药的夸谷里的大夫手艺好，天经地义。
-- 让玩家此刻对墨大夫起疑，第 3 章的反转就没有落差了。

if flag.get("ch01.zhenmin_yaofan") == 1 then
    talk("zhen_min_yaofan", "ch01.zhenmin.yaofan_again")
    return
end

talk("zhen_min_yaofan", "ch01.zhenmin.yaofan_1")
talk("zhen_min_yaofan", "ch01.zhenmin.yaofan_2")

flag.set("ch01.zhenmin_yaofan")
