-- @hook ch06_huangfenggu trigger_ce_linggen enter once
-- 第六章节点 13「测灵根」。挂在 ch06_huangfenggu 迎宾楼房间的门 (8,28)
-- （mode=enter，once=true，guard_flag=ch06.xisha，set_flag=ch06.linggen）。
-- 王师叔安排他住下、当场测了属性，是发生在他身上的——落点 (8,30) 往北两格就是房门，踏入型。
--
-- 施工图第 4 节末段的取舍：迎宾楼的「禁法」只在旁白里说，不做门。于是本节不 teleport：他进了房门就是住下了，
-- 等三四天是 advance_days(4)，玩家不必真被关着（施工图原写「13 的脚本末尾 teleport 到房间外的走廊」，
-- 本作改成「落点在厅里、房门上挂 13」，效果一样：出了这一节他在房里，14 在房里那张床上）。已记进第 18 节。
--
-- 王师叔当面只说「四属性缺金，伪灵根」；「十八九岁的样子」「九层初期」「筑基的可能只有百分之一」那些话，
-- 原著是他在大殿上对掌门说的（ch144），韩立没听见，游戏不演（施工图第 17 节第 16 条）；
-- 「希望渺茫」留给节点 14 叶师叔说。修炼面板从 ch06.linggen 置起多一行灵根（引擎路 E3）。
-- 他黯然了一整天；黄龙丹金髓丸（本作的养精丹）的效力减了——第 7 章炼丹的伏笔。

talk("", "ch06.linggen.room")
talk("wang_shishu", "ch06.linggen.test")
talk("", "ch06.linggen.palm")
talk("wang_shishu", "ch06.linggen.result")
talk("wang_shishu", "ch06.linggen.stay")
talk("", "ch06.linggen.gloom")
talk("", "ch06.linggen.pill")
talk("", "ch06.linggen.days")

advance_days(4)

flag.set("ch06.linggen")
