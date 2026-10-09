-- @hook ch06_huangfenggu trigger_jihe enter once
-- 第七章节点 18「议事大殿」。挂在 ch06_huangfenggu 大殿门前一格 (23,9)(24,9)（与第 6 章 trigger_dadian 不同格）
-- （mode=enter，once=true，guard_flag=ch07.ma_songyao，set_flag=ch07.jihe）。信符召集、踏进大殿。
--
-- 框架件（施工图 13.1 第 18 段）：出发前三天信符召集；陈师妹也在，变得冷，认不出他；陆师兄按失踪了事，没人再提；
-- 本届满额二十五人（十三层顶峰五六个、多为十二层、十一层只三人：他、一个白发老者、一个少年）；中阶灵石任取一块（他取水属性）；
-- 从一只隔绝神识的袋子里摸一件灵器（不说是什么，节点 34 清点时才说）；李师祖（结丹）带队，王师叔等五名管事同行；
-- 银甲角蟒，两天两夜飞到建州北的荒山。「师祖」二字从这一节起才出现（施工图 12.2）；「李化元」这里仍不出现。
-- 李师祖的 NPC 站在殿门前（visible ch07.ma_songyao / hidden ch07.jihe），钟掌门只在台词里（第 6 章已出场）。
-- advance_days(2)；teleport ch07_jindi_wai 荒山的石坳 (5,4)（genmaps_ch07.py 的 JINDI_WAI_HUANGSHAN）；登 kScriptTransfers。

talk("", "ch07.jihe.hall")
talk("", "ch07.jihe.chen")
talk("", "ch07.jihe.lu")
talk("", "ch07.jihe.count")
talk("zhong_lingdao", "ch07.jihe.zhang")
talk("", "ch07.jihe.stone")
give("material_lingshi_zhong", 1)
talk("", "ch07.jihe.bag")
give("story_bubao_faqi")
talk("", "ch07.jihe.li")
talk("li_huayuan", "ch07.jihe.li_go")
talk("", "ch07.jihe.mang")

advance_days(2)

talk("", "ch07.jihe.land")

flag.set("ch07.jihe")
teleport("ch07_jindi_wai", 5, 4)
