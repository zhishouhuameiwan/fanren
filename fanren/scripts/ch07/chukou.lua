-- @hook ch07_jindi_wai trigger_chukou enter once
-- 第七章节点 32b「出禁地；赌局；嗅灵兽；记名弟子」。挂在 ch07_jindi_wai 出口通道第一格 (23,33)(24,33)
-- （mode=enter，once=true，guard_flag=ch07.xiashan，set_flag=ch07.chujindi）。出了通道，一派一派的人站着。
--
-- 框架件（施工图 13.1 第 32b 段）：七位结丹、管事与穹前辈等在出口；一派一派出来、人数报出来（黄枫谷活着五人，别派多是三四人，
-- 巨剑门只剩两人——这一幕唯一的数字，3.2 幕三节奏第 5 条）；他坐到陈氏兄妹旁边，他们以为他躲了一路；掩月宗迟迟不出，他替她担心；
-- 菡云芝在灵兽山那边，平安；最后一刻钟掩月宗十余人整队出来，为首的是南宫婉（霓裳仙子迎上去；别派认不出这位常年遮面的仙子）；
-- 向之礼最后一个爬出来，通道随即碎灭（ch07.xiang == 1 多一句）；嗅灵兽（三丈内闻得出百年以上的灵药，储物袋也藏不住）；
-- 他交出二十几株，幼苗它闻不出（节点 26 那句伏笔在这里兑现）；掩月宗赢了赌局，浮云子交出内丹、李师祖应承送穹前辈铁精；
-- 南宫婉上神舟，自始至终没看他一眼（验收 18：那一句的 key 含「没」「看」）；菡云芝离开时对他一笑；
-- 李师祖问名字、入谷几年（近三年）、药从哪来——他编了一段自己捡了漏的来历（**本作自出**，原著那段两派相斗的故事与比方不用）；
-- 二选一：1 迟疑片刻再应（原著：李师祖不悦）/ 2 当场叩拜——两条都拜，置 ch07.baishi；取消按 1 算；碧光刀；筑基之后收为正式弟子；
-- 王师叔一句按 ch07.baoming 分；骑蟒数日回谷（advance_days(4)）。「李化元」这里仍不出现（节点 33 马师伯口中才有，12.2）。
--
-- **嗅灵兽留幼苗**（验收 13、第 9 节第 5 条）：三味药逐一 item.count_aged(id, 100) ＋ take_aged(id, n, 100)，n 为 0 就不扣
-- （take_aged 的 count ≤ 0 会按 1 算，契约 4.2）；幼苗（< 100 年）一株不动。不用 take(id, item.count(id))。
-- 支线 Z2 两处都去过（ch07.yujian_a、ch07.yujian_b）：门派加赏灵石 50 ＋ 中阶灵石 1（施工图第 6 节）。
-- 在场：出口那一批（visible ch07.qipai / ch07.pojin，hidden ch07.chujindi）是他落地时已经站着的人；南宫婉、掩月宗弟子、向之礼
-- 都在他之后才出来，出口不摆他们（genmaps_ch07.py 第五张图的注释）。
-- 末尾 teleport ch06_baiyaoyuan (24,10)（BAIYAOYUAN_LUKOU）；登 kScriptTransfers。

talk("", "ch07.chukou.out")
talk("", "ch07.chukou.wait")
talk("", "ch07.chukou.first")
talk("", "ch07.chukou.counts")
talk("", "ch07.chukou.sit")
talk("", "ch07.chukou.chen")
talk("", "ch07.chukou.late")
talk("", "ch07.chukou.han")
talk("", "ch07.chukou.yanyue")
talk("nichang_xianzi", "ch07.chukou.nichang")
talk("", "ch07.chukou.veil")
talk("", "ch07.chukou.xiang")
if flag.get("ch07.xiang") == 1 then
    talk("", "ch07.chukou.xiang_old")
end
talk("", "ch07.chukou.close")

talk("", "ch07.chukou.beast")
talk("", "ch07.chukou.others")
talk("", "ch07.chukou.turn")
local handed = 0
local n = item.count_aged("herb_yusui_zhi", 100)
if n > 0 and take_aged("herb_yusui_zhi", n, 100) then
    handed = handed + n
end
n = item.count_aged("herb_zihou_hua", 100)
if n > 0 and take_aged("herb_zihou_hua", n, 100) then
    handed = handed + n
end
n = item.count_aged("herb_tianling_guo", 100)
if n > 0 and take_aged("herb_tianling_guo", n, 100) then
    handed = handed + n
end
if handed >= 20 then
    talk("", "ch07.chukou.pile")
else
    talk("", "ch07.chukou.pile_small")
end
talk("", "ch07.chukou.sniff")
if flag.get("ch07.yujian_a") ~= 0 and flag.get("ch07.yujian_b") ~= 0 then
    talk("", "ch07.chukou.yujian")
    give("material_lingshi", 50)
    give("material_lingshi_zhong", 1)
end

talk("", "ch07.chukou.bet")
talk("", "ch07.chukou.fuyun")
talk("", "ch07.chukou.li_iron")
talk("", "ch07.chukou.ship")
talk("", "ch07.chukou.han_smile")

talk("", "ch07.chukou.last")
talk("li_huayuan", "ch07.chukou.li_ask")
talk("", "ch07.chukou.answer")
talk("li_huayuan", "ch07.chukou.li_where")
talk("", "ch07.chukou.story")
talk("li_huayuan", "ch07.chukou.li_luck")
talk("li_huayuan", "ch07.chukou.li_offer")

local pick = choice{
    "ch07.chukou.opt_pause",
    "ch07.chukou.opt_kneel",
}
if pick ~= 2 then
    pick = 1
end
if pick == 1 then
    talk("", "ch07.chukou.pause")
    talk("li_huayuan", "ch07.chukou.pause_li")
    talk("", "ch07.chukou.pause_kneel")
else
    talk("", "ch07.chukou.kneel")
end
give("weapon_biguang_dao")
talk("li_huayuan", "ch07.chukou.gift")
if flag.get("ch07.baoming") == 2 then
    talk("wang_shishu", "ch07.chukou.wang_plain")
else
    talk("wang_shishu", "ch07.chukou.wang_fruit")
end
talk("", "ch07.chukou.home")

advance_days(4)

flag.set("ch07.baishi", pick)
flag.set("ch07.chujindi")
teleport("ch06_baiyaoyuan", 24, 10)
