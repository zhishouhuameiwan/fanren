-- @hook ch07_fangshi trigger_wanbaolou interact once
-- 第七章节点 11b「万宝楼」。挂在 ch07_fangshi 万宝楼的门 (28,19)(28,20)
-- （mode=interact，once=true，guard_flag=ch07.fangshi，set_flag=ch07.wanbaolou）。挑最气派的一家推门进去的是他。
--
-- 框架件（施工图 13.1 第 11b 段）：掌柜田卜离（凡人）；化名「厉飞雨」；楼里有人暗中护着；四件——金蚨子母刃（母一子八）、
-- 玄铁飞天盾（放大后自动绕身防御）、天雷子（筑基硬抗也化灰）、金光砖符宝；鉴定人丁老断为新出土的千年黄精芝；
-- 符宝五条（至多十分之一威能、谁都能用、筑基前一两成、用一次耗一次、耗尽作废）；两株换四件；
-- 田掌柜求以后优先卖他——他因此定下这种买卖少做。
-- 本作的讲述次序：他先说只换不卖 → 掌柜先亮符宝（他认出和自己那张剑符是一类）→ 其余三件 → 二选一亮药 → 丁老 → 成交。
-- 二选一：1 先亮一株（原著）/ 2 两株一起亮（田掌柜后面那句说得更急）。取消按 1 算。
-- 交换按年份下限（验收 12：万宝楼的字面量 1000）。千年药不够两株就一句、return（不置旗标，触发器留着；回园再养）。
-- give / magic.learn 每一处看返回值：learn 失败说明 id 写错了，照样往下（东西已经到手），但不置旗标以外的任何事都不瞒。

if item.count_aged("herb_huangjing_zhi", 1000) < 2 then
    talk("", "ch07.wanbaolou.lack")
    return
end

talk("", "ch07.wanbaolou.pick")
talk("", "ch07.wanbaolou.boss")
talk("", "ch07.wanbaolou.boss_ask")
talk("", "ch07.wanbaolou.alias")
talk("", "ch07.wanbaolou.guard")
talk("", "ch07.wanbaolou.want")
talk("", "ch07.wanbaolou.brick")
talk("tian_buli", "ch07.wanbaolou.fubao1")
talk("tian_buli", "ch07.wanbaolou.fubao2")
talk("tian_buli", "ch07.wanbaolou.fubao3")
talk("", "ch07.wanbaolou.rest")
talk("tian_buli", "ch07.wanbaolou.items1")
talk("tian_buli", "ch07.wanbaolou.items2")
talk("tian_buli", "ch07.wanbaolou.price")

local pick = choice{
    "ch07.wanbaolou.opt_one",
    "ch07.wanbaolou.opt_two",
}
if pick ~= 2 then
    pick = 1
end
if pick == 1 then
    talk("", "ch07.wanbaolou.one")
else
    talk("", "ch07.wanbaolou.two")
end
talk("", "ch07.wanbaolou.ding")
talk("ding_lao", "ch07.wanbaolou.ding_say")
if pick == 1 then
    talk("", "ch07.wanbaolou.one_more")
else
    talk("", "ch07.wanbaolou.two_quiet")
end

if not take_aged("herb_huangjing_zhi", 2, 1000) then
    talk("", "ch07.wanbaolou.lack")
    return
end
talk("", "ch07.wanbaolou.deal")
give("story_jinfu_zimuren")
give("story_xuantie_dun")
give("talisman_tianleizi")
give("talisman_jinguangzhuan")
magic.learn("magic_ji_jinfu")
magic.learn("magic_ji_jinguangzhuan")
talk("", "ch07.wanbaolou.jue")

if pick == 1 then
    talk("tian_buli", "ch07.wanbaolou.ask1")
else
    talk("tian_buli", "ch07.wanbaolou.ask2")
end
talk("", "ch07.wanbaolou.decide")

flag.set("ch07.wanbaolou", pick)
