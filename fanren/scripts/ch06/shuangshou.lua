-- @hook ch06_tainan_gu trigger_shuangshou enter once
-- 第六章节点 8a「双首鹜」。挂在 ch06_tainan_gu 小楼门前通广场的路上 (31,19)(31,20)
-- （mode=enter，once=true，guard_flag=ch06.yishi，set_flag=ch06.shuangshou）。鸟是从头顶飞过的（ch136）。
--
-- 燕家只是过场（施工图 1.1 第 9 条、1.2）：全是路人议论，燕家人一句台词也没有。
-- 「燕」字只在本节（ch06.shuangshou.*）的文案里出现（施工图 12.2）。
-- 接第 5 章：席铁牛撞见的大鸟与鸟背上的一对男女（ch05.anpai.xi / xi2）——那对男女多半就是燕家兄妹。燕家到此为止。

talk("", "ch06.shuangshou.road")
talk("", "ch06.shuangshou.shadow")
talk("", "ch06.shuangshou.bird")
talk("", "ch06.shuangshou.crowd")
talk("", "ch06.shuangshou.name")
talk("", "ch06.shuangshou.pair")
talk("", "ch06.shuangshou.sigh")
talk("", "ch06.shuangshou.xi")

flag.set("ch06.shuangshou")
