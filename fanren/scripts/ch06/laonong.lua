-- @hook ch06_tainan_cun npc_laonong npc
-- 第六章太南山脚借住的那户老农（npc_laonong，自家门口）。闲话，不推进任何东西，不置旗标。
-- 节点 1a 他把包里的碎银整个留给了这户人家（改编）；怪坡进得去出不来的事，老农在打探里讲。

if flag.get("ch06.wan_met") ~= 0 then
    talk("tainan_laonong", "ch06.npc.laonong.boy")
    return
end

talk("tainan_laonong", "ch06.npc.laonong.silver")
