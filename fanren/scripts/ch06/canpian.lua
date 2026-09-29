-- @hook ch06_tainan_gu trigger_canpian enter once
-- 第六章节点 10a「法宝残片」。挂在 ch06_tainan_gu 小楼门前那条路靠广场的一头 (27,19)(27,20)
-- （mode=enter，once=true，guard_flag=ch06.jiuceng，set_flag=ch06.canpian）。争吵声是传过来的（ch139）。
--
-- 开头 advance_days(2)：太南会的最后两天（施工图 3.3）。广场挤了两千多人，高层的也露面了；他九层了，还只算中等。
-- 黑汉（十层，皮肤黝黑像个庄稼人）拿「破布」换叶家摊主的回风钵，摊主要三十块灵石；黑汉演示吃金鼠（一级妖兽）与一把小刀，
-- 天眼术也看不出。摊主不换（「隐头还是隐脚」）。有人出十块，抬到二十；喊价的圆脸青年其实只有两块，被灵压压得冒汗。
-- 他出声拦下。二选一（施工图 3.2 节点 10）：两条都换。
--   1 先试它是不是真不妨碍吸灵气（原著：酒杯 + 指尖一豆灵气，揭开布光团还在）
--   2 直接换
-- 节点 18b 埋瓶时前者多一句。取消按 1 算。
-- take 飞行符看返回值——飞行符 tradeable 假、本章只在这里扣，没有就是存档坏了：开头先判，判过才拨日子，
-- 免得 return 之后重踩时把那两天再拨一遍。
-- **「升仙令」三个字本节不许出现**（12.2）。

if item.count("talisman_feixing_fu") < 1 then
    talk("", "ch06.canpian.nofu")
    return
end

advance_days(2)

talk("", "ch06.canpian.crowd")
talk("", "ch06.canpian.middle")
talk("", "ch06.canpian.quarrel")
talk("", "ch06.canpian.heihan")
talk("hei_han", "ch06.canpian.boast")
talk("", "ch06.canpian.mouse")
talk("", "ch06.canpian.knife")
talk("yejia_tanzhu", "ch06.canpian.refuse")
talk("", "ch06.canpian.bid")
talk("yuanlian_qingnian", "ch06.canpian.none")
talk("", "ch06.canpian.press")
talk("", "ch06.canpian.halt")
talk("", "ch06.canpian.why")
talk("", "ch06.canpian.fu")
talk("hei_han", "ch06.canpian.deal")

local pick = choice{
    "ch06.canpian.opt_test",
    "ch06.canpian.opt_swap",
}

local canpian = 1
if pick == 2 then
    canpian = 2
    talk("", "ch06.canpian.swap")
else
    talk("", "ch06.canpian.test1")
    talk("", "ch06.canpian.test2")
end

if not take("talisman_feixing_fu", 1) then
    talk("", "ch06.canpian.nofu")
    return
end
give("story_fabao_canpian", 1)
talk("", "ch06.canpian.fool")
talk("yuanlian_qingnian", "ch06.canpian.chase")
give("story_qingxi_bilu", 1)
talk("", "ch06.canpian.bilu")

flag.set("ch06.canpian", canpian)
