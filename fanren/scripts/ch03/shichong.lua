-- @hook ch03_mishi trigger_shichong interact once
-- 第三章节点 4「尸虫丸」。
-- 挂在 ch03_mishi 抽屉柜处的 trigger（mode=interact，once=true，set_flag=ch03.shichong_wan）。
--
-- 取材角度：**不描写虫，描写开药的手续**。
-- 原著这一节是檀木盒、白丸、然后一段「内脏被活生生吃干净、哀嚎三天三夜」的恐吓。
-- 本作把容器换成药房里天天发出去的那种四方纸包，把恐吓换成**医嘱**：
-- 他先在纸包外头写日子、吹墨、等干，字写得很好看；讲后果时用的是脉案的口气
-- （「头三日腹痛。再后头的我不必说了」），因为那一页韩立自己抄过。
-- 一个大夫最吓人的地方不是他会杀人，是他杀人时的手势和给人治病时一模一样。
--
-- 第二处要害是**杀手锏挂在第 2 章那次选择上**。
-- 原著是泛泛一句「我对你的亲人很挂念」；本作按 ch02.qian_quxiang 分成两句：
--   寄钱回家的（=1）——他问那一笔够不够用，不够只管开口；
--   把钱全换成药材的（=2 或未做选择）——他说听人讲你这两年不往家里捎钱了，
--     问是不是家里已经不缺，说完还笑了一下。
-- 两句都没有一个字提到威胁，两句都让韩立坐不住。玩家上一章随手做的那个决定，
-- 在这里变成对方手里的一份卷宗——这是小说做不到、而游戏必须做的事。
--
-- 分支：当场吞 / 先问清楚。两条都得吞（他没有第二条路），分别在于他知不知道
-- 自己吞的是什么。先问的那一条多出「他看清了那粒东西的样子」，
-- 往后一年他常常想起它——代价是记忆，不是数值。
-- 这一处同样**没有旗标**可置（见 tanpai.lua 的同一段说明）。

if flag.get("ch03.tanpai_done") ~= 1 then
    talk("", "ch03.shichong.gate")
    return
end

if flag.get("ch03.shichong_wan") == 1 then
    talk("", "ch03.shichong.again")
    return
end

talk("mo_juren", "ch03.shichong.packet")
talk("", "ch03.shichong.write")
talk("", "ch03.shichong.date")

local pick = choice{
    "ch03.shichong.opt_eat",
    "ch03.shichong.opt_ask",
}

if pick == 2 then
    talk("", "ch03.shichong.ask1")
    talk("mo_juren", "ch03.shichong.ask2")
    talk("mo_juren", "ch03.shichong.ask3")
    talk("", "ch03.shichong.ask4")
else
    -- 不问就吃（pick == 1）与取消（pick == nil）走同一条。
    -- 没做选择按不问算：屋里那个人刚点了他的穴道又松开，
    -- 一个十五岁的孩子这时候多半是照做，而不是先谈条件。
    talk("", "ch03.shichong.eat1")
    talk("mo_juren", "ch03.shichong.eat2")
end

talk("mo_juren", "ch03.shichong.pledge")
talk("", "ch03.shichong.trust")

-- 杀手锏。按第 2 章那一百六十七块的去向分两句。
-- ch02.qian_quxiang 在第 2 章是 1（寄回家）或 2（全换药材）；
-- 玩家若跳过了那一节，旗标为 0，按「没往家里捎」那一句算——
-- 对面听来是一样的，他手里只有「这孩子近来没捎钱」这一条消息。
if flag.get("ch02.qian_quxiang") == 1 then
    talk("mo_juren", "ch03.shichong.money_send")
else
    talk("mo_juren", "ch03.shichong.money_keep")
end

talk("", "ch03.shichong.money_hit")
talk("", "ch03.shichong.bow")
talk("", "ch03.shichong.after")

flag.set("ch03.shichong_wan")
