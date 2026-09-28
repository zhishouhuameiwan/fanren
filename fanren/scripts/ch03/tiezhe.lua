-- @hook ch03_andao trigger_tiezhe interact once
-- 第三章节点 12 之一「搜尸；三大铁则」。
-- 挂在 ch03_andao 南口内侧 (19,27) 起两格的 trigger
-- （mode=interact，once=true，set_flag=ch03.tiezhe_zhi）。
--
-- **独立校对 MEDIUM-2 裁决过一次：地图是对的，错的是文案，所以改的是文案。**
-- 设计第 5 节节点 12 写的就是「暗道/谷口」，暗道是这一章唯一一条不经谷口、
-- 不经宗门的出路，章末离谷也走它。而处决在 ch02_jusuo，两处隔着两三张图，
-- 从前那三条文案却把它们写成同一个地点连着发生：
--   ch03.tiezhe.gate  「没了结之前**他哪儿也不去**」
--   ch03.tiezhe.search「他**回到师父身边**」
--   ch03.tiezhe.word  「**屋外头**那人就转过身来」（暗道里没有屋）
-- 现已按地图重写：搜尸这一场发生在天黑之后，师父是他一个人从石屋背下石阶、
-- 背到这条道快到头那一段的——**他为什么要把人搬到这儿来**，与他章末从这条道
-- 出谷是同一个理由：这是谷里唯一没有人下来的地方。gate 那一条也跟着改了，
-- 因为玩家在节点 8 那趟暗道之行里就会撞见这一格，那时候道上什么也没有。
--
-- 三大铁则（硬约束第 9 条，出处 ch59）在原著里是余子童口述的。
-- 本作不能照办：节点 11 他已经被处决了，而设计第 5 节把三大铁则排在节点 12。
-- 于是换了个载体——**墨居仁自己抄在册子上的三条**，字写得比别处都工整，
-- 像是抄给自己看的。这一改有三样好处：
--   1. 它解释了墨居仁为什么敢赌：他抄过这三条，也自以为算过；
--   2. 它让韩立在没有人向他解释的情况下自己读懂昨天发生了什么——
--      第二条他昨天已经在自己身上验过了，验的时候他还在睡觉（ch03.tiezhe.realize）；
--   3. 三条底下那一行小字是手札里那只年轻的手写的（「记牢，这三条没有人破得了」），
--      于是余子童就算死了，也还在那本册子里说最后一句话。
--
-- 搜尸这件事本身要冷。他蹲下去一寸一寸地摸，摸的时候并没有觉得不该摸——
-- **这一句后面不接任何自我辩解**，只接一句「这件事后来在他心里过了很久」。
-- 他今年十六，节点 12 下半（quhun_rudui.lua）他会因为自己不够难过而害怕，
-- 这一句是那一段的引信。
--
-- 家书（story_mo_jiashu）是大纲点名要搜出来的遗物，落款嘉元城，通向第 5 章墨府。
-- 本章不解释它的用途，韩立收下它的理由写成「那个地方现在没有人当家了」——
-- 是盘算，不是志向。

if flag.get("ch03.yuzitong_chujue") ~= 1 then
    talk("", "ch03.tiezhe.gate")
    return
end

if flag.get("ch03.tiezhe_zhi") == 1 then
    talk("", "ch03.tiezhe.again")
    return
end

talk("", "ch03.tiezhe.search")
talk("", "ch03.tiezhe.find1")

give("story_mo_jiashu", 1)
talk("", "ch03.tiezhe.find2")

talk("", "ch03.tiezhe.book")
talk("", "ch03.tiezhe.rule1")
talk("", "ch03.tiezhe.rule2")
talk("", "ch03.tiezhe.rule3")
talk("", "ch03.tiezhe.margin")
talk("", "ch03.tiezhe.realize")
talk("", "ch03.tiezhe.word")

flag.set("ch03.tiezhe_zhi")
