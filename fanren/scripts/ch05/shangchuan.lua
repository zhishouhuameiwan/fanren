-- @hook ch05_dukou trigger_shangchuan interact once
-- 第五章节点 1b「上船」。挂在 ch05_dukou 栈桥尽头拴着的那条船 (36,14)(36,15)
-- （mode=interact，once=true，guard_flag=ch05.kaipian，set_flag=ch05.shangchuan）。
-- 包一条船顺流而下是他自己动的念头（ch101），所以是交互型。
--
-- 船家在这一路上说出嘉元城码头的行规（ch101：操舟的船夫提醒过他）——节点 2 他照规矩雇人，
-- 根子在这里。二十九天水路一次拨完（施工图 3.3：d = 29 到码头），然后 teleport 到西城
-- 码头的栈桥上：那一格就是 ch05_xicheng 的默认入口 spawn_from_dukou，往岸上走必踩节点 2。
--
-- 船钱不经 take：施工图第 9 节的碎银进出表里没有这一笔，这一节只讲价不记账。

talk("chuanjia", "ch05.shangchuan.boat")
talk("", "ch05.shangchuan.pay")
talk("", "ch05.shangchuan.river")

advance_days(29)

talk("chuanjia", "ch05.shangchuan.rule")
talk("", "ch05.shangchuan.arrive")

flag.set("ch05.shangchuan")
teleport("ch05_xicheng", 44, 17)
