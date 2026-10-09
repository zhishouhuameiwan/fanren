-- @hook ch06_baiyaoyuan trigger_sannian interact once
-- 第七章节点 34「三年」。挂在 ch06_baiyaoyuan 居室书桌的另一格 (26,5)（与节点 1、7 不同格）
-- （mode=interact，once=true，guard_flag=ch07.xieshili，set_flag=ch07.sannian）。坐下排三年的计划是他。
--
-- 框架件（施工图 13.1 第 34 段）：奖赏只一粒筑基丹，说辞是弟子应尽的孝敬（王师叔送来；advance_days(4)）；清点禁地所得——中阶十余、
-- 低阶数百、傀儡弓手（要「分神」秘术、筑基以上才修得）、银色书页（参不透）；三年：催主药、备辅药；陈师妹服了奖赏的丹、一年后筑基；
-- 她大哥第二枚仍败、回了家族；他在谷里有了点名气；叶师叔把拖欠的一股脑送来、还多出；李师祖派人送来亲笔誊写、署名的《青元剑诀》，
-- 此外三年无音讯。蒙太奇压成八句上下（advance_days(1080)）。
-- 账（第 9 节）：筑基丹 +1（本章 give 三处之一）；中阶 +10、低阶 +300（清点）；叶补还中阶 +5、回气丹 +2（ch06.huinuo / ch06.rangdan
-- 只改说法，东西一样，施工图 1.4）；禁地三药余量全部 take，**药粉一律 40 份**（两侧同账；玩家若把幼苗种进了田里也不卡，3.2 节点 34）。

advance_days(4)

talk("", "ch07.sannian.wang")
talk("wang_shishu", "ch07.sannian.wang_say")
give("pill_zhuji_dan", 1)
talk("", "ch07.sannian.fine")
talk("", "ch07.sannian.count")
give("material_lingshi_zhong", 10)
give("material_lingshi", 300)
talk("", "ch07.sannian.kuilei")
if take("story_bubao_faqi", 1) then
    give("story_kuilei_gongshou")
end
talk("", "ch07.sannian.shuye")
give("story_yinse_shuye")

talk("", "ch07.sannian.plan")
local c = item.count("herb_yusui_zhi")
if c > 0 then
    take("herb_yusui_zhi", c)
end
c = item.count("herb_zihou_hua")
if c > 0 then
    take("herb_zihou_hua", c)
end
c = item.count("herb_tianling_guo")
if c > 0 then
    take("herb_tianling_guo", c)
end
advance_days(1080)
talk("", "ch07.sannian.main")
talk("", "ch07.sannian.aux")
give("material_zhuji_yaofen", 40)
talk("", "ch07.sannian.chen")
talk("", "ch07.sannian.dage")
talk("", "ch07.sannian.name")
if flag.get("ch06.huinuo") == 2 or flag.get("ch06.rangdan") == 2 then
    talk("", "ch07.sannian.ye_cold")
else
    talk("", "ch07.sannian.ye_warm")
end
give("material_lingshi_zhong", 5)
give("pill_huiqi_dan", 2)
talk("", "ch07.sannian.book")
give("story_qingyuan_jianjue")
talk("", "ch07.sannian.ready")

flag.set("ch07.sannian")
