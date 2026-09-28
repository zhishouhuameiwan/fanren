-- @hook ch02_jusuo trigger_dazuo interact
-- 第二章节点 2「第一次打坐（正式修炼开始）」。
-- 挂在 ch02_jusuo 里打坐处（facility kind=meditate）旁的 trigger（mode=interact）。
--
-- 为什么是 trigger 而不是写进 facility：一段脚本没办法在中途把控制权交还给
-- 玩家再收回来。这里先把话说完，修炼面板由玩家自己去点——第 1 章踩出来的
-- 那条限制，本章五个段落的推进全是这个结构。
--
-- 本章仍是凡人武馆：坐下去什么也不会有，只有膝盖发木、后颈发酸。
-- 不许出现气感、灵气、暖流一类的东西，师父也不给任何玄乎的解释。
--
-- 章末的二选一定下 ch02.kugong_jiezou，四年苦修的节奏按它走。
-- 这一处与第 1 章的 ch01.xiulian_taidu 是两笔账：那一笔记的是他当时的打算，
-- 这一笔记的是他在谷里住下来之后真正落实的作息。

if flag.get("ch02.renyao_done") ~= 1 then
    talk("", "ch02.dazuo.gate")
    return
end

if flag.get("ch02.dazuo_done") == 1 then
    talk("", "ch02.dazuo.again")
    return
end

talk("", "ch02.dazuo.room")
talk("", "ch02.dazuo.start")
talk("", "ch02.dazuo.nothing")
talk("", "ch02.dazuo.zt")

talk("", "ch02.dazuo.mo_ask")
talk("mo_daifu", "ch02.dazuo.mo_qian")

local pick = choice{
    "ch02.dazuo.opt_daily",
    "ch02.dazuo.opt_more",
}

if pick == 2 then
    flag.set("ch02.kugong_jiezou", 2)
    talk("mo_daifu", "ch02.dazuo.more_reply")
else
    -- 每日一遍（pick == 1）与取消（pick == nil）走同一条：没做选择按稳妥的算。
    flag.set("ch02.kugong_jiezou", 1)
    talk("mo_daifu", "ch02.dazuo.daily_reply")
end

talk("", "ch02.dazuo.after")

flag.set("ch02.dazuo_done")
