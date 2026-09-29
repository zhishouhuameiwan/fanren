-- @hook ch06_baiyaoyuan trigger_maiping interact once
-- 第六章节点 18b「埋瓶、送物、章末」。挂在 ch06_baiyaoyuan 药田西南角那一格翻土 (11,19)
-- （mode=interact，once=true，guard_flag=ch06.renyao，set_flag=ch06.done）。夜里把小瓶埋下去的是他。
--
-- 埋瓶：掌天瓶的状态一样不动——它还是他的，只是「埋着」；引擎没有「瓶子不在身上」的概念，本章不造（施工图 3.2 节点 18）。
-- 盖上法宝残片：灵气不外泄、外头的灵气照样进去（ch139、ch151）。ch06.canpian == 1（先试过）多一句。
-- advance_days(4)；叶师叔送物（施工图 3.2 节点 18b，不到原说的五分之一，丹药一字不提）：
--   huinuo == 1 → 灵石 60、火球符 2、护身符 1、金刺符 1；huinuo == 2 → 灵石 30、火球符 1；
--   rangdan == 2 → 在上面的基础上灵石再少 10（他没许杂务、别的许得多，悔得也多）。
--   原著是「中阶两块、低阶数十、法器三件、符箓若干」：本作灵石不分阶、没有法器物品（施工图第 7 节），折成灵石与符。
-- 送完他仍一下子阔了起来（他这会儿的灵石比谷外多数散修多）；台词以施工图 13.1 为准。
-- 开灵田 field_baiyaoyuan、6 槽：开出来给第 7 章（槽位 6 是估的，第 7 章可再补，本章只此一处，验收 14）。
-- 章末次序（校对 MEDIUM-3，施工图 16.4）：「当上黄枫谷弟子的头一天」过完（first_day）排在埋瓶之前——
-- 原著是当天睡下、新弟子的头一天过去，接下来一段日子夜里埋瓶，几天后才送物；从前把它放在「几天后」之后，日历倒了。
-- 末句 end 留在送物之后，写成一笔账（不说「第二天醒来」）。章末落在这笔账上，**不给志向**（沿用第 3–5 章口径）。不 ending()。

talk("", "ch06.maiping.first_day")
talk("", "ch06.maiping.night")
talk("", "ch06.maiping.dig")
talk("", "ch06.maiping.cover")
if flag.get("ch06.canpian") == 1 then
    talk("", "ch06.maiping.tested")
end
talk("", "ch06.maiping.still")

advance_days(4)

talk("", "ch06.maiping.ye")
local stones, huoqiu, hushen, jinci = 60, 2, 1, 1
if flag.get("ch06.huinuo") == 2 then
    stones, huoqiu, hushen, jinci = 30, 1, 0, 0
    talk("ye_shishu", "ch06.maiping.cold")
else
    talk("ye_shishu", "ch06.maiping.warm")
end
if flag.get("ch06.rangdan") == 2 then
    stones = stones - 10
    talk("", "ch06.maiping.less")
end
give("material_lingshi", stones)
give("talisman_huoqiu_fu", huoqiu)
if hushen > 0 then
    give("talisman_hushen_fu", hushen)
end
if jinci > 0 then
    give("talisman_jinci_fu", jinci)
end
talk("", "ch06.maiping.fifth")
talk("", "ch06.maiping.nopill")
talk("", "ch06.maiping.rich")

field.unlock("field_baiyaoyuan", 6)
talk("", "ch06.maiping.field")
talk("", "ch06.maiping.end")

flag.set("ch06.done")
