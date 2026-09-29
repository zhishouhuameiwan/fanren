-- @hook ch06_huangfenggu trigger_dadian interact once
-- 第六章节点 15「掌门殿」。挂在 ch06_huangfenggu 议事大殿的殿门 (23,8)(24,8)
-- （mode=interact，once=true，guard_flag=ch06.rangdan，set_flag=ch06.rumen）。走进那三道门的是他。
--
-- 三道有弟子守着的门；十几位管事各看一眼就移开目光；钟灵道问他是不是叫韩立；他要大礼参拜，被一股无形之力托住；
-- 掌门问让丹是否属实——资质低劣，让给更需要的师兄；「叶师弟要好好补偿」；先跟传功弟子修行。
-- 王师叔带他出殿讲**门规，压成 5 句**（施工图 3.2 节点 15、第 13 节第 3 条：执事 / 领事是事实，总结换说法）：
-- 一万多弟子、筑基数百、结丹几位常年闭关、元婴师叔祖九百多岁周游在外；十年一选、三十岁以下；
-- 执事 / 领事 / 筑基三层；管事是自知结丹无望的筑基弟子；掌门也是管事里的一个。
-- 大殿争丹（慕容衫、吴师兄、红拂师叔的后人）韩立不在场，游戏不演。

talk("", "ch06.dadian.gates")
talk("", "ch06.dadian.eyes")
talk("zhong_lingdao", "ch06.dadian.name")
talk("", "ch06.dadian.bow")
talk("zhong_lingdao", "ch06.dadian.ask")
talk("", "ch06.dadian.answer")
talk("zhong_lingdao", "ch06.dadian.repay")
talk("zhong_lingdao", "ch06.dadian.chuangong")

talk("", "ch06.dadian.out")
talk("wang_shishu", "ch06.dadian.rule1")
talk("wang_shishu", "ch06.dadian.rule2")
talk("wang_shishu", "ch06.dadian.rule3")
talk("wang_shishu", "ch06.dadian.rule4")
talk("", "ch06.dadian.rule5")

flag.set("ch06.rumen")
