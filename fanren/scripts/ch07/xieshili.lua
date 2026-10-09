-- @hook ch06_baiyaoyuan trigger_xieshili enter once
-- 第七章节点 33「谢师礼」。挂在 ch06_baiyaoyuan 茅屋门内一格 (24,6)
-- （mode=enter，once=true，guard_flag=ch07.chujindi，set_flag=ch07.xieshili）。马师伯在屋里等他。
--
-- 框架件（施工图 13.1 第 33 段）：马师伯没想到他活着回来、还成了李师叔的记名弟子；马要他改口叫师兄，他坚持没筑基还叫师伯；
-- 马点破谢师礼——师傅可抽徒弟上交的一半（只一次）；约十株换一粒，他本该两粒、如今一粒；「李化元」首次出口（马说的，12.2）；
-- 李师叔护短；他反倒放了心：图的是药，一粒筑基丹对要自己开炉的他不算什么。
-- 原著马师伯那两句评语（见了鬼似的、夸他懂事）不用。

talk("", "ch07.xieshili.inside")
talk("ma_shibo", "ch07.xieshili.ma_alive")
talk("ma_shibo", "ch07.xieshili.ma_brother")
talk("", "ch07.xieshili.no")
talk("ma_shibo", "ch07.xieshili.ma_ok")
talk("ma_shibo", "ch07.xieshili.ma_li")
talk("ma_shibo", "ch07.xieshili.ma_rule")
talk("ma_shibo", "ch07.xieshili.ma_math")
talk("ma_shibo", "ch07.xieshili.ma_worth")
talk("", "ch07.xieshili.relief")
talk("", "ch07.xieshili.garden")

flag.set("ch07.xieshili")
