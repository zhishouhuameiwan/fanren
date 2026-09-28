-- @hook ch02_wairentang trigger_suanzhang interact
-- 第二章节点 11「算账：这一株值多少」，章末。
-- 挂在 ch02_wairentang 收药处的 trigger（mode=interact）。
--
-- 三个数摆在一起，账就清了：
--   四年前那株一年份的    6
--   误洒那滴催出来的十一年 22
--   攒满三滴浇出来的四十四年 145
-- 全照 herbPrice 实算（黄精基价 5，(age + 10)^2 / 100 倍）。
--
-- 药商不替玩家报倍数，也不把单滴吹成几十倍。他只说这价他给得住、比去年
-- 收的那些强得多，具体差多少，玩家看着柜台上那两堆钱自己算。
-- 设计文档第 3 节要的就是这个：三次之间不要有任何人解释机制。
--
-- 章末的二选一：把钱寄回家 / 留着买药材。
-- 两条都对，都不改走向。寄回家接的是第 1 章那句「谷里管三顿饭，家里少一张嘴」；
-- 留着买药材接的是段四他自己顺出来的那条路。他这一章从被安排的孩子
-- 变成有自己盘算的人，落点就在这个选择上。
--
-- 收尾不给他任何志向。他想的是这东西值多少钱，以及这事不能让第二个人知道。

if flag.get("ch02.cuanman_done") ~= 1 then
    talk("wairentang_yaoshang", "ch02.suanzhang.gate")
    return
end

if flag.get("ch02.done") == 1 then
    talk("wairentang_yaoshang", "ch02.suanzhang.again")
    return
end

talk("", "ch02.suanzhang.arrive")

-- 两笔都要看 take 的返回值。
--
-- 不看会开一个大洞：玩家可以先把这两株药在同一个柜台上卖掉，再来触发算账，
-- 于是 take 静默失败、两次 give 照常执行，白得 167 块——而本章总收入才 173。
-- 这与本轮刚修好的 maiyao 是同一个洞，只是金额大得多。
--
-- 扣不到就不付钱，而不是不让过：算账是章末，拦在这里会把玩家永久卡在第 2 章。
talk("", "ch02.suanzhang.first")
if take("herb_huangjing_cao", 1, 11) then
    give("material_lingshi", 22)
    talk("", "ch02.suanzhang.first_p")
else
    talk("wairentang_yaoshang", "ch02.suanzhang.gone")
end

talk("", "ch02.suanzhang.second")
if take("herb_huangjing_cao", 1, 44) then
    give("material_lingshi", 145)
    talk("wairentang_yaoshang", "ch02.suanzhang.second_p")
else
    talk("wairentang_yaoshang", "ch02.suanzhang.gone")
end

talk("", "ch02.suanzhang.compare")
talk("", "ch02.suanzhang.count")
talk("wairentang_yaoshang", "ch02.suanzhang.yaoshang")

-- 本章真正的瓶颈是槽位与生长时间，不是绿液。
-- 实算：一年 360 天，段五这最后一年瓶子能凝 51 滴，但灵田只有 8 槽、
-- 一年才长 1 年份，最多过 8 株、用得掉 24 滴，凝出来的一多半没处使。
-- 所以他在这里感慨的必须是「地就这么大、药长得就这么慢」——
-- 写成「绿液不够用」会与玩家的实际手感正好相反，台词和面板对不上。
-- 这两句同时是第 6 章接手百药园的引子：那边的解法就是槽位更多、长得更快。
talk("", "ch02.suanzhang.bottleneck")
talk("", "ch02.suanzhang.bottleneck2")

local pick = choice{
    "ch02.suanzhang.opt_send",
    "ch02.suanzhang.opt_keep",
}

-- 按手头实际有多少钱结算，不写死数目。
--
-- 主线三次交易是 6 + 22 + 145 = 173，但玩家完全可以自己多种多卖几株再回来，
-- 写死就会出现「台词说捎走一大半、钱袋只少了固定的一笔」这种对不上。
--
-- 这个二选一从前只置一个旗标、钱一分不动、药材一件不给：两句台词
-- （「捎了一大半回去」「一块没动，全换成了药材」）说的事情一件也没发生。
-- 本章独立校对把这一类列为要害——台词断言了某个状态，而引擎里不是那样。
local purse = item.count("material_lingshi")

if pick == 2 then
    -- 全换成药材。种苗按收药处的买价 6 块一株（基价 5 × buyRate 1.2），
    -- 除不尽的零头留在身上，免得出现「换完倒欠」。
    local seedling_price = 6
    local bought = purse // seedling_price
    flag.set("ch02.qian_quxiang", 2)
    if bought > 0 then
        take("material_lingshi", bought * seedling_price)
        give("herb_huangjing_cao", bought, 0)
    end
    talk("", "ch02.suanzhang.keep_1")
else
    -- 寄回家（pick == 1）与取消（pick == nil）走同一条。
    -- 没做选择按寄回家算：一个刚从饿肚子的家里出来的孩子，默认就是这个。
    --
    -- 「一大半」取三分之二：留下的那点要够他自己周转，全寄走会让第 3 章
    -- 开局身无分文，而那不是玩家选这一项时想要的意思。
    local sent = purse * 2 // 3
    flag.set("ch02.qian_quxiang", 1)
    if sent > 0 then
        take("material_lingshi", sent)
    end
    talk("", "ch02.suanzhang.send_1")
end

talk("", "ch02.suanzhang.end1")
talk("", "ch02.suanzhang.end2")
talk("", "ch02.suanzhang.end3")

flag.set("ch02.done")
