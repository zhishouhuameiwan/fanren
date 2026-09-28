-- @hook ch01_qingniuzhen npc_zhen_min_jiuke npc
-- 第一章节点 3 的闲聊 NPC 之一：镇上喝闲酒的汉子，讲崖上的铃声规矩。
-- 三个闲人各自一个脚本，因为地图上是三个 npc 对象，脚本无从知道是谁触发的。
-- 重复交互只说一句收尾，免得玩家反复点出整段。

if flag.get("ch01.zhenmin_jiuke") == 1 then
    talk("zhen_min_jiuke", "ch01.zhenmin.jiuke_again")
    return
end

talk("zhen_min_jiuke", "ch01.zhenmin.jiuke_1")
talk("zhen_min_jiuke", "ch01.zhenmin.jiuke_2")

flag.set("ch01.zhenmin_jiuke")
