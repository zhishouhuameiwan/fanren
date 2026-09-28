-- @hook ch05_xicheng trigger_matou enter once
-- 第五章节点 2「码头」。挂在 ch05_xicheng 栈桥最里那一格 (42,17)(42,18)
-- （mode=enter，once=true，set_flag=ch05.matou）。一上岸苦力就围上来——事情找上他，所以是踏入型；
-- 节点 1b 的 teleport 把人落在栈桥外头，往岸上走必踩这一格。
--
-- 叙述，无选择（施工图 3.2）。原著 ch101 这一段的几件事：两座竹棚、照规矩雇人、一个人扛不动
-- 那只包叫来第二个、两双眼睛盯上了。本作照留事件，细节换成自己的（鱼腥与桐油、脚钱翻倍的账），
-- **不写「数千两」**；孙二狗与黑熊的分账与击掌整段删（施工图第 13 节转写高风险点 6）。
-- 这时他还不知道那两个人叫什么，文案里只写「眉眼长歪了的瘦子」「黑大个」。

talk("", "ch05.matou.land")
talk("", "ch05.matou.smell")
talk("", "ch05.matou.one")
talk("", "ch05.matou.second")
talk("", "ch05.matou.eyes")

flag.set("ch05.matou")
