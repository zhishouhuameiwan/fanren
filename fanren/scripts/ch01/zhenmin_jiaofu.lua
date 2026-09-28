-- @hook ch01_qingniuzhen npc_zhen_min_jiaofu npc
-- 第一章节点 3 的闲聊 NPC 之三：镇上等活的脚夫，讲今年来了多少人、留得下几个。
-- 他负责把「三四百人里留二三十个」这个比例说给玩家听，
-- 节点 6 放榜时的落差才有依据。

if flag.get("ch01.zhenmin_jiaofu") == 1 then
    talk("zhen_min_jiaofu", "ch01.zhenmin.jiaofu_again")
    return
end

talk("zhen_min_jiaofu", "ch01.zhenmin.jiaofu_1")
talk("zhen_min_jiaofu", "ch01.zhenmin.jiaofu_2")

flag.set("ch01.zhenmin_jiaofu")
