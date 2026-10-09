-- @hook ch07_jindi_wai trigger_pojin enter once
-- 第七章节点 20「黄土坡：破禁」。挂在 ch07_jindi_wai 风刃之墙上那条通道的北口 (23,29)(24,29)
-- （mode=enter，once=true，guard_flag=ch07.qipai，set_flag=ch07.pojin）。七件法宝轰开的通道。
--
-- 框架件（施工图 13.1 第 20 段）：飞了几个时辰，到一片寸草不生的黄土坡；巨剑门的结丹掷剑试禁，风刃之墙现形；
-- 约一个时辰一次，第四次明显弱了；七位结丹各放法宝，三四个时辰打出丈许通道；弟子穿插进去；一进去同门也不可信；
-- 出口的挪移阵把人随机送走。（七件法宝逐一点名、试剑的造法这些原著质感不用。）
-- 末尾 teleport ch07_jindi_waiwei 落点 (5,31)（genmaps_ch07.py 的 WAIWEI_LUODIAN）；登 kScriptTransfers。

talk("", "ch07.pojin.test")
talk("", "ch07.pojin.wall")
talk("", "ch07.pojin.wait")
talk("", "ch07.pojin.fourth")
talk("", "ch07.pojin.seven")
talk("", "ch07.pojin.hole")
talk("", "ch07.pojin.go")
talk("", "ch07.pojin.enter")
talk("", "ch07.pojin.trust")
talk("", "ch07.pojin.spin")

flag.set("ch07.pojin")
teleport("ch07_jindi_waiwei", 5, 31)
