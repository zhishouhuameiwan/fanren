-- @hook ch01_caixiashan npc_zhang_tie npc
-- @hook ch01_caixiashan trigger_yu_zhangtie enter once
-- 第一章节点 4「彩霞山道遇张铁」。走到山道中段触发。
-- 时间在报名的次日：天不亮从青牛镇镇口动身，晌午在山道上遇见同去应考的张铁，午后到崖下。
-- 台词从前写的是先上山道、日落前进镇报名，与地图（青牛镇北口要先报名才开）相反，
-- 2026-09-28 按地图次序改过（docs/ch01-design.md 第 2 节修订）。
--
-- 三叔在路上把七玄门的来历交代掉：这些数字（两百年、一百年、三四千人、
-- 十几个镇子）都是原著给死的，放在闲聊里说，比开场字幕好消化。
--
-- 本节点的二选一是分不分干粮。它同时承接节点 2 的留饼选择，
-- 又在节点 5（攀爬时张铁的那句话）与节点 8（同住一屋时张铁分床的那句话）里各回响一次。

-- 山道上那处触发是 once；张铁本人（npc_zhang_tie）一直站到考核（hidden_flag=ch01.climb_done），
-- 挂在他身上的这个脚本没有 once：结识之后再找他，从前会把相遇与分干粮的二选一整段重演。
if flag.get("ch01.zhangtie_met") == 1 then
    talk("", "ch01.shandao.together")
    return
end

talk("", "ch01.shandao.noon")
talk("han_sanshu", "ch01.shandao.sanshu_qixuan")
talk("han_sanshu", "ch01.shandao.sanshu_history")
talk("han_sanshu", "ch01.shandao.sanshu_guimo")

talk("", "ch01.shandao.meet")
talk("zhang_tie", "ch01.shandao.zt_greet")
talk("zhang_tie", "ch01.shandao.zt_ask")
talk("", "ch01.shandao.hanli_reply")
talk("zhang_tie", "ch01.shandao.zt_hungry")

-- 节点 2 留没留饼，决定他这会儿摸出来的是大半块还是两块。
if flag.get("ch01.liu_bing") == 1 then
    talk("", "ch01.shandao.bag_half")
else
    talk("", "ch01.shandao.bag_full")
end

local pick = choice{
    "ch01.shandao.opt_share",
    "ch01.shandao.opt_keep",
}

if pick == 1 then
    flag.set("ch01.zhangtie_shared")
    talk("zhang_tie", "ch01.shandao.share_reply")
    talk("", "ch01.shandao.share_after")
else
    -- 自己留着（pick == 2）与取消（pick == nil）走同一条。
    talk("zhang_tie", "ch01.shandao.keep_reply")
    talk("", "ch01.shandao.keep_after")
end

talk("han_sanshu", "ch01.shandao.sanshu_cui")
talk("", "ch01.shandao.together")

flag.set("ch01.zhangtie_met")
