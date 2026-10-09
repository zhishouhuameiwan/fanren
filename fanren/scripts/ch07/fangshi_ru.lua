-- @hook ch07_fangshi trigger_fangshi_ru enter once
-- 第七章节点 11a「坊市：北口」。挂在 ch07_fangshi 北口门楼里第一格 (23,2)(24,2)
-- （mode=enter，once=true，guard_flag=ch07.chuling，set_flag=ch07.fangshi）。坊市的规矩是看出来的。
--
-- 框架件（施工图 13.1 第 11a 段）：坊市在太岳山脉东北缘，元武国修士常来、两边修仙界不敌对；五里内禁飞；
-- 换装遮脸（穿什么本作自出）；一条南北街——南段店铺是黄枫谷的产业，北段临时摊位（一块灵石摆一天、有人护着）。
-- 店名不列（只点万宝楼，11b）。收药摊一句带过（facility_yaotan：千年灵草、禁地药收不了）。

talk("", "ch07.fangshi.walk")
talk("", "ch07.fangshi.change")
talk("", "ch07.fangshi.street")
talk("", "ch07.fangshi.people")
talk("", "ch07.fangshi.stall")

flag.set("ch07.fangshi")
