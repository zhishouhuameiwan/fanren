-- @hook ch06_shanqiu trigger_xisha enter once
-- 第六章节点 12「袭杀」。三场必打之二、全章唯一的生死仗：b06_shanqiu_xisha。挂在 ch06_shanqiu 凹地的口子 (20,18)(21,18)
-- （mode=enter，once=true，guard_flag=ch06.chugu，set_flag=ch06.xisha）。冰锥是破土而出的，踏入型。
--
-- 凹地打坐——十几道白光破土而出，他腾身躲开；五个火球排成一线打过去（旁白，火弹术蓄满劲的连发）；
-- 黄衣人闪出，「想知道？下辈子吧」；脚下钻出两只黄光大手。原著他丢了一口剑、缩骨抽脚、身上中了两锥——游戏里这些交给那一仗。
-- 战斗：黄衣人 + 土甲大汉（killable_by 金，只有祭剑符要得了命）。can_escape 假、defeat_is_fatal 真：输 → game_over。
-- 胜：黄衣人拍飞行符腾空、剑光追上透心（旁白）；活口没了；搜身：灵石 50、火球符 2、护身符 1
-- （「几件符箓法器」——本作没有法器物品，用符代，施工图 3.2 节点 12）。**本章灵石的第一笔进账在这里**（第 9 节第 3 条）。
--   ch06.jujue == 1（婉拒）：多一句「烧剩的符纸角，画的像是追踪的路引」——只写他看见的，不替他说破是谁。
--   ch06.zhang_done：多一句「加上自己攒的，头一回像个修仙者」（支线 Z2 的回报）。
-- 末尾：天雾台蒙太奇（ch145 追叙）：比胡萍姑说的还惨烈三分；七派引路人现身；他把升仙令给黄枫谷的接引人看；
-- 上一枚升仙令收回是四五百年前；与十名胜者乘小船法器入谷。**升仙大会不做地图、不做擂台**（他只观战）。
-- advance_days(15)；teleport 黄枫谷迎宾楼的厅里 (8,30)（genmaps_ch06.py 的 HFG_YINGBIN）；登 kScriptTransfers。

talk("", "ch06.xisha.sit")
talk("", "ch06.xisha.ice")
talk("", "ch06.xisha.dodge")
talk("", "ch06.xisha.fire")
talk("", "ch06.xisha.yellow")
talk("", "ch06.xisha.who")
talk("huangyi_ren", "ch06.xisha.next")
talk("huangyi_ren", "ch06.xisha.go")
talk("", "ch06.xisha.hands")

local won = battle("b06_shanqiu_xisha")

if not won then
    talk("", "ch06.xisha.lost")
    game_over()
    return
end

talk("", "ch06.xisha.dahan")
talk("", "ch06.xisha.flee")
talk("", "ch06.xisha.dead")
talk("", "ch06.xisha.search")
give("material_lingshi", 50)
give("talisman_huoqiu_fu", 2)
give("talisman_hushen_fu", 1)
talk("", "ch06.xisha.rich")

if flag.get("ch06.jujue") == 1 then
    talk("", "ch06.xisha.scrap")
end
if flag.get("ch06.zhang_done") ~= 0 then
    talk("", "ch06.xisha.zhang")
end
talk("", "ch06.xisha.leave")

talk("", "ch06.xisha.tianwu1")
talk("", "ch06.xisha.tianwu2")
talk("", "ch06.xisha.guide")
talk("wang_shishu", "ch06.xisha.wang")
talk("", "ch06.xisha.boat")

advance_days(15)

flag.set("ch06.xisha")
teleport("ch06_huangfenggu", 8, 30)
