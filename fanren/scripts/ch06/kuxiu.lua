-- @hook ch06_sanxiu_lou trigger_kuxiu interact once
-- 第六章节点 9b「苦修」。挂在 ch06_sanxiu_lou 韩立房里蒲团的左格 (21,2)
-- （mode=interact，once=true，guard_flag=ch06.zhifu，set_flag=ch06.jiuceng）。右格 (22,2) 是打坐设施，不同格。
--
-- 长春功早练到第八层顶上，得了后几层的心法（ch138；书在 8b 换来，ch06.changchungong）；白天吃药打坐、夜里练法术；
-- advance_days(12)；太南会结束前的最后几日突破第九层（ch138）。
-- **本章升一层只此一处**（验收 5）：realm.advance 到炼气九层，顺带把上限抬到九层（docs/interfaces-p3-script.md 7.2）；
-- realm.cap 本章 0 处。看返回值。
-- **不扣养精丹**：原著「若没有那十来瓶丹药」是他另外的存货，游戏里已算进节点 1a 那 15 瓶的账（施工图第 9 节）。
-- 新法术（咒书在 5b 换来，ch06.zhoushu）：流沙术、冰冻术 magic.learn；匿身术、传音术只在旁白里一学就会，
-- 不进法术表；升空、缠绕、火花、地刺一时领会不了。
-- **祭剑符常驻**（施工图 3.2 节点 9、10.1 E5）：第 5 章那一下是借的——那时驱物生疏，只够乘其不备；
-- 这十二天夜里练的就有它，ch142-143 他已能指挥剑光追杀百丈外的人。本章 magic.forget 0 处；第 7 章 ch171 剑符报废时收回。
-- 三门都看返回值；学不到的说一句，戏照演。

talk("", "ch06.kuxiu.sit")
talk("", "ch06.kuxiu.book")
talk("", "ch06.kuxiu.days")

advance_days(12)

talk("", "ch06.kuxiu.break")
local ok = realm.advance(realm.QI_REFINING_9)
if not ok then
    talk("", "ch06.kuxiu.realm_fail")
end
talk("wu_jiuzhi", "ch06.kuxiu.wu")
talk("qingwen_daoshi", "ch06.kuxiu.qingwen")
talk("", "ch06.kuxiu.pills")

talk("", "ch06.kuxiu.spells")
local learned = 0
if magic.learn("magic_liusha_shu") then
    learned = learned + 1
end
if magic.learn("magic_bingdong_shu") then
    learned = learned + 1
end
if learned == 2 then
    talk("", "ch06.kuxiu.sand")
else
    talk("", "ch06.kuxiu.learn_fail")
end
talk("", "ch06.kuxiu.small")
talk("", "ch06.kuxiu.hard")

talk("", "ch06.kuxiu.sword1")
if magic.learn("magic_ji_jianfu") then
    talk("", "ch06.kuxiu.sword2")
else
    talk("", "ch06.kuxiu.sword_fail")
end

flag.set("ch06.jiuceng")
