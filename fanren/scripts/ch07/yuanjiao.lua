-- @hook ch06_baiyaoyuan trigger_yuanjiao interact once
-- 第七章节点 9「园角两株千年灵草」。挂在 ch06_baiyaoyuan 园角田边那一格翻土 (10,20)
-- （mode=interact，once=true，guard_flag=ch07.lianqi，set_flag=ch07.qiannian）。收成的是他。
--
-- 框架件（施工图 13.1 第 9 段）：四个多月——白天练敛气术，夜里在园角催两株黄精芝，避开马师伯来的日子；
-- 灵石留着保命不动，拿草去换；只卖给外来修仙者，不与本门交易。
-- item.count_aged 数到两株 ≥ 1000 年才置旗标；**药不扣**，节点 11b 才交出去（验收 12：园角的字面量 1000）。
-- 采下来收在身上才算：还在地里的数不到（count_aged 只数背包），不够就一句「还差着」return。
-- 日子由玩家在蒲团上自己推（施工图 3.3「玩家推进的三段」），本脚本不拨日子。

if item.count_aged("herb_huangjing_zhi", 1000) < 2 then
    talk("", "ch07.yuanjiao.short")
    return
end

talk("", "ch07.yuanjiao.months")
talk("", "ch07.yuanjiao.ma")
talk("", "ch07.yuanjiao.done")
talk("", "ch07.yuanjiao.why")
talk("", "ch07.yuanjiao.who")
talk("", "ch07.yuanjiao.next")

flag.set("ch07.qiannian")
