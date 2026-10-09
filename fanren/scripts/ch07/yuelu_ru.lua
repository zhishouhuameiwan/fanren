-- @hook ch07_yuelu_dian trigger_yuelu_ru enter once
-- 第七章节点 4a「岳麓殿：传送阵」。挂在 ch07_yuelu_dian 传送阵小间北口 (23,32)(24,32)
-- （mode=enter，once=true，guard_flag=ch07.danbao，set_flag=ch07.yuelu）。传送阵把他送进大厅，是发生在他身上的。
--
-- 框架件（施工图 13.1 第 4a 段）：巫钧山；未筑基者须担保人信物；两名筑基红衣弟子验玉符，担保人是马师兄——
-- 马师伯即马师兄在这里点破；传送阵每趟一块、头一回免；大厅三条通道：器、丹、无标记。
-- 红衣弟子不建 role：他们只在旁白里（原著那一句「痴迷炼丹的马师兄也会作保」是质感，不用）。

talk("", "ch07.yuelu.stone")
talk("", "ch07.yuelu.guards")
talk("", "ch07.yuelu.check")
talk("", "ch07.yuelu.shixiong")
talk("", "ch07.yuelu.fee")
talk("", "ch07.yuelu.jump")
talk("", "ch07.yuelu.hall")
talk("", "ch07.yuelu.ways")

flag.set("ch07.yuelu")
