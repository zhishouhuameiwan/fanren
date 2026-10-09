-- @hook ch07_huanxingshan trigger_sichu interact once
-- 第七章节点 27「四处灵药」（蒙太奇）。挂在 ch07_huanxingshan 洞外的岔路石 (19,28)
-- （mode=interact，once=true，guard_flag=ch07.zihouhua，set_flag=ch07.sichu）。照资料把另外几处跑一遍的是他。
--
-- 框架件（施工图 13.1 第 27 段）：此后几处没有守护兽，一处接一处；第三日将尽，最后一站：近山顶的小石殿（天灵果在里头）。
-- give（幼苗，herb_age 1）：玉髓芝 4、天灵果 2、紫猴花 2（施工图 3.2 节点 27）。

talk("", "ch07.sichu.stone")
talk("", "ch07.sichu.one")
talk("", "ch07.sichu.two")
give("herb_yusui_zhi", 4, 1)
give("herb_tianling_guo", 2, 1)
give("herb_zihou_hua", 2, 1)
talk("", "ch07.sichu.last")

flag.set("ch07.sichu")
