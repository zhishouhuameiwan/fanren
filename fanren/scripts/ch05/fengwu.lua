-- @hook ch05_mofu npc_mo_fengwu npc
-- 第五章墨凤舞（npc_mo_fengwu，后园药圃）。支线 Z1「墨凤舞的医书」第二步：交付。
-- 她只在登门之后、章末之前在场（visible_flag=ch05.dengmen，hidden_flag=ch05.done）：
-- 章末置位时没交的抄本，就再也交不出去了——过期（施工图第 6 节）。
--
-- 按她身上这件事走到哪一步说话：
--   已经交过（ch05.fengwu_yigao）         → 她在看那本抄本；
--   抄好了（ch05.fengwu_chao）且身上有抄本 → 交付：扣抄本（看返回值），给药圃的年份药材——
--                                           黄精 6、紫参 3，都是四十年（施工图第 6 节 Z1 奖励），置 ch05.fengwu_yigao；
--   她开过口（ch05.fengwu_qiu）            → 她想问又不好意思问；
--   都还没有                               → 药圃里一个脸红的姑娘，名字还不知道。
-- 奖励只在这里给：任务系统不发奖励（docs/interfaces-p3-ch05.md 1.1），data/quests 那一份 rewards 只拿来对账。

if flag.get("ch05.fengwu_yigao") ~= 0 then
    talk("mo_fengwu", "ch05.npc.fengwu.after")
    return
end

if flag.get("ch05.fengwu_chao") ~= 0 and item.count("story_yigao_chaoben") > 0 then
    if take("story_yigao_chaoben", 1) then
        talk("", "ch05.npc.fengwu.give1")
        talk("mo_fengwu", "ch05.npc.fengwu.give2")
        give("herb_huangjing_cao", 6, 40)
        give("herb_zishen_cao", 3, 40)
        flag.set("ch05.fengwu_yigao")
        return
    end
    talk("", "ch05.npc.fengwu.lost")
    return
end

if flag.get("ch05.fengwu_qiu") ~= 0 then
    talk("mo_fengwu", "ch05.npc.fengwu.wait")
    return
end

talk("", "ch05.npc.fengwu.pre")
