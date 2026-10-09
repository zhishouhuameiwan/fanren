-- @hook ch07_huanxingshan trigger_youdi enter once
-- 第七章节点 26c「紫猴花洞：诱敌」。挂在 ch07_huanxingshan 石厅口 (8,16)(9,16)
-- （mode=enter，once=true，guard_flag=ch07.mairen，set_flag=ch07.zihouhua）。去石厅口把它引出来的是他。
--
-- 框架件（施工图 13.1 第 26 段 ⑤⑥）：把它引出来，它追进洞道，腹部一路被剖开；采苗；割几块背壳；毒气入鼻——服清灵散（有则扣）。
-- **不是战斗**（施工图 8.3：这是全章最「韩立」的一场，做成一仗反而把算计变成了数值）。
-- 引它的法子本作自出（原著火球挑衅、红雾、倒飞那几样是质感，不用）。
-- give：紫猴花幼苗 4（herb_age 1）、蜈蚣背壳 3（施工图 3.2 节点 26）。

talk("", "ch07.youdi.walk")
talk("", "ch07.youdi.stone")
talk("", "ch07.youdi.chase")
talk("", "ch07.youdi.blades")
talk("", "ch07.youdi.dead")
talk("", "ch07.youdi.dig")
give("herb_zihou_hua", 4, 1)
talk("", "ch07.youdi.shell")
give("material_wugong_ke", 3)
if take("pill_qingling_san", 1) then
    talk("", "ch07.youdi.qingling")
else
    talk("", "ch07.youdi.noqingling")
end
talk("", "ch07.youdi.next")

flag.set("ch07.zihouhua")
