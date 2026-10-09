-- @hook ch07_fangshi trigger_fangshi_chu enter once
-- 第七章节点 12「绕路；山洞过夜」。挂在 ch07_fangshi 南口门洞 (23,28)(24,28)
-- （mode=enter，once=true，guard_flag=ch07.wanbaolou，set_flag=ch07.likai）。出南口就走。
--
-- 框架件（施工图 13.1 第 12 段）：出坊市就走；怕被跟，绕了三四天；傍晚歇在太岳山脉外围半山腰的一个石洞。
-- advance_days(6)（施工图 3.3：三四天＋三日）；teleport ch07_shandong 洞内 (8,9)（genmaps_ch07.py 的 SHANDONG_DONGNEI）；
-- 登 tests/ObjectiveTests.cpp 的 kScriptTransfers（坊市 → 山洞）。

talk("", "ch07.fangshi_chu.go")
talk("", "ch07.fangshi_chu.loop")

advance_days(6)

talk("", "ch07.fangshi_chu.cave")

flag.set("ch07.likai")
teleport("ch07_shandong", 8, 9)
