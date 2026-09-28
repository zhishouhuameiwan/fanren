-- @hook ch03_mishi trigger_jieyao interact once
-- 第三章节点 9「解药」。
-- 挂在 ch03_mishi 太师椅前的 trigger（mode=interact，once=true，set_flag=ch03.jieyao_xuan）。
--
-- 取材角度：原著 ch44 这一场是墨大夫堆着笑脸迎出门、韩立小心提防、
-- 一句「你如果敢把门给关上」把气氛顶起来。本作留下「门开着」这一个动作，
-- 其余全换掉，换成**一个明显撑不住了的人**：
-- 他坐下去要扶扶手，扶了两下才坐稳；一年前他还看得出是四十几岁的人硬撑成六十，
-- 这一回撑不住了。玩家这时才第一次替这个要他命的人觉得不忍，
-- 而这份不忍正是节点 11 之后他自己也害怕的那种冷漠的反面。
--
-- 分支 ch03.jieyao_xuan：1 吞下去 / 2 先验一验。
-- 设计文档写的是「用不用」，本作落成「当场吃 / 验过再吃」，理由是：
-- 真让他不吃，一个月后他就死了，这一章没有第二条路可走；
-- 而**把不吃写成一个能选的选项、再让它不生效**，比不给这个选项更糟。
-- 落成「验」之后两条都真的走得到，也都有各自的收获：
--   吞下去——这一年的账先清一笔，他睡得着；
--   先验一验——他确认了三件事，第三件是「那边还要他活着，所以那边快了」。
-- 第二条那句「不识药不入口」是师父教他的，墨居仁一个字不差地还给了他。
-- 这是本章「同一个人」那条要求的最后一次兑现：他的客气话从头到尾没变过。
--
-- 两条都清掉尸虫丸这件事本身（韩立确实把药吞了），但**旗标 ch03.shichong_wan
-- 不清**：它登记的语义是「尸虫丸是否已服下」，不是「是否还在身上」。
-- 清掉会让节点 10、11 的闸门读出错误的世界状态。

if flag.get("ch03.andao_zhan") ~= 1 then
    talk("", "ch03.jieyao.gate")
    return
end

if flag.get("ch03.jieyao_xuan") > 0 then
    talk("", "ch03.jieyao.again")
    return
end

-- 从暗道那一仗到这一天隔着大半年。把它走完，纸包上那个日子才真的只剩一个月：
--   节点 5 起算 30 + 60 + 45 + 3 + 120 = 258 天，再走 77 天正好是第 335 天，
--   距 365 天的期限还剩 30。台词断言了「还剩一个月」，引擎里就得真是一个月。
advance_days(77)

talk("", "ch03.jieyao.day")
talk("mo_juren", "ch03.jieyao.room")
talk("", "ch03.jieyao.look")
talk("mo_juren", "ch03.jieyao.pill")

local pick = choice{
    "ch03.jieyao.opt_eat",
    "ch03.jieyao.opt_test",
}

-- 本节点一共走 22 天，两条路一样。验药那三天从这 22 天里扣，不外加——
-- 理由同 miji.lua：日历不随选择漂移。
local elapsed = 22

if pick == 2 then
    flag.set("ch03.jieyao_xuan", 2)
    talk("", "ch03.jieyao.test1")
    talk("mo_juren", "ch03.jieyao.test2")
    advance_days(3)
    elapsed = elapsed - 3
    talk("", "ch03.jieyao.test3")
    talk("", "ch03.jieyao.test4")
else
    -- 吞下去（pick == 1）与取消（pick == nil）走同一条。
    -- 没做选择按吞算：日子只剩一个月，站在那间屋子里的人这时候不会拖。
    flag.set("ch03.jieyao_xuan", 1)
    talk("", "ch03.jieyao.eat1")
    talk("", "ch03.jieyao.eat2")
end

talk("mo_juren", "ch03.jieyao.warn")
talk("", "ch03.jieyao.hanli")

-- 那一夜他把家伙都验一遍。有没有那把软剑，是节点 7 那次买卖的结果，
-- 也是第 2 章章末那次选择一路传下来的。两条台词说的是两件不同的事：
-- 有剑的那一条写他不擅使剑却要知道它抽得出来；
-- 没剑的那一条写他把手摊在灯下看，除了两只筒子什么也没有——
-- **不写成惩罚**，写成他早就知道的一件事，看一遍也还是要看。
if item.count("weapon_yudai_duanjian") > 0 then
    talk("", "ch03.jieyao.check_sword")
else
    talk("", "ch03.jieyao.check_bare")
end

talk("", "ch03.jieyao.end")

-- 走完本节点是第 357 天。节点 10 再过四天，第 361 天——
-- 离纸包上写的那个日子还差四天，而解药已经下去了。
advance_days(elapsed)
