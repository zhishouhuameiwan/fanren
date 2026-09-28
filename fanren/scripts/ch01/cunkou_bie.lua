-- @hook ch01_hanjiacun npc_han_mu_cunkou npc
-- @hook ch01_hanjiacun npc_han_sanshu_cunkou npc
-- @hook ch01_hanjiacun trigger_cunkou_bie enter once
-- 第一章节点 2「村口话别」。玩家从家里走到韩家村村口时触发。
--
-- 教移动：这一节是玩家第一次自己走路的终点。脚本里不出现任何「按方向键」
-- 之类的字样——教程藏在剧情里，三叔在村口等人本身就是走过去的理由。
--
-- 三处挂点都在北墙村口：门洞里侧那一排的踏入触发（guard_flag=ch01.sanshu_met），
-- 与老槐树下的娘、三叔（npc_han_mu_cunkou / npc_han_sanshu_cunkou，都从 ch01.sanshu_met 起
-- 在场、到 ch01.muqin_bie 为止）。三处都只在见过三叔之后够得着，所以这里不查前置；
-- 从前这个脚本还挂在村里那个从开局就在场的韩母身上，没见三叔先找娘就能演完话别。
--
-- 文案一律引 key，实际文字在 data/text/ch01_main.json。

talk("", "ch01.cunkou.dawn")
talk("han_sanshu", "ch01.cunkou.sanshu_wait")
talk("han_mu", "ch01.cunkou.mu_bing")
talk("han_mu", "ch01.cunkou.mu_suoshi")

-- 节点 1 的选择在这里第一次有回响：母亲听三叔转述过他当时答得干脆还是犹豫。
-- 只改一句台词的口吻，不改任何走向——本章的回响一律是这个量级。
if flag.get("ch01.sanshu_accepted") == 1 then
    talk("han_mu", "ch01.cunkou.mu_echo_yes")
else
    talk("han_mu", "ch01.cunkou.mu_echo_no")
end

talk("", "ch01.cunkou.count_bing")

local pick = choice{
    "ch01.cunkou.opt_leave",
    "ch01.cunkou.opt_takeall",
}

if pick == 1 then
    -- 留了饼，节点 4 分给张铁时他包里就真的只剩大半块。选择要在别处看得见。
    flag.set("ch01.liu_bing")
    talk("han_mu", "ch01.cunkou.leave_reply")
else
    -- 全带走（pick == 2）与取消（pick == nil）走同一条：不做选择按默认的「都带上」算。
    talk("han_mu", "ch01.cunkou.takeall_reply")
end

talk("han_mu", "ch01.cunkou.mu_last")
talk("han_sanshu", "ch01.cunkou.sanshu_cui")
talk("", "ch01.cunkou.depart")

flag.set("ch01.muqin_bie")
