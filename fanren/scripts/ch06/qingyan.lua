-- @hook ch06_tainan_gu trigger_qingyan enter once
-- 第六章节点 3「青颜真人」。挂在 ch06_tainan_gu 谷口雾道里、落点往北第一格 (23,32)(24,32)
-- （mode=enter，once=true，guard_flag=ch06.rugu，set_flag=ch06.qingyan）。是万小山在远处叫他（ch129），踏入型。
--
-- 谷内画面、青颜真人一眼看穿「木属性第八层」（他心里知道没那些丹药还在三四层爬）、「心术不正」那一瞥斜向他；
-- 万小山被带走——**此后原著不再出场**（施工图 1.1 第 4 条）。广场上与会的全是十几二十几岁，他在人群里只算中等。
-- 「灵石」二字本节不出现（12.2）。

talk("", "ch06.qingyan.valley")
talk("", "ch06.qingyan.call")
talk("wan_xiaoshan", "ch06.qingyan.intro")
talk("", "ch06.qingyan.face")
talk("qingyan_zhenren", "ch06.qingyan.eighth")
talk("", "ch06.qingyan.inner")
talk("qingyan_zhenren", "ch06.qingyan.family")
talk("wan_xiaoshan", "ch06.qingyan.sister")
talk("qingyan_zhenren", "ch06.qingyan.warn")
talk("", "ch06.qingyan.glance")
talk("", "ch06.qingyan.bye")
talk("", "ch06.qingyan.dragged")
talk("", "ch06.qingyan.night")
talk("", "ch06.qingyan.young")

flag.set("ch06.qingyan")
