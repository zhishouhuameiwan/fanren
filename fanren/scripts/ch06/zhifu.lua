-- @hook ch06_sanxiu_lou trigger_zhifu interact once
-- 第六章节点 9a「制符」。挂在 ch06_sanxiu_lou 韩立房里制符桌的左格 (16,2)
-- （mode=interact，once=true，guard_flag=ch06.jinzhubi，set_flag=ch06.zhifu）。摆开符纸丹砂的是他（ch138）。
-- 右格 (17,2) 是制符设施 facility_zhifu（kind=talisman，grade=2，require_flag=ch06.zhifu）：一格一个对象（规则 17）。
--
-- 他照定神术的法子注灵入笔、蘸丹砂画符；第一张灵气紊乱、自燃；半日十二张全废（自燃、小爆、灵力散尽）；
-- 仰头骂老天。然后去问苦桑——制符之道那段问答（新手连败上百次是常事；合格的制符师要数万次；符术与符箓的分别；
-- 劝他花小钱买几张现成的定神符备用）。**本章制符是失败的**：解锁的是「他有了笔、有了桌子」，不是「他会了」。
-- 苦桑那句劝 → 支线 Z1「一张定神符」接取：ch06.dingshen_qiu = 1。
-- 用掉符纸 12、丹砂 3（看返回值）：不够一打就说一句、return，不置旗标——坊市一块一张买得到。

-- 先数够了再扣：两样有一样不够，就一样也不动。
if item.count("material_fuzhi") < 12 or item.count("material_dansha") < 3 then
    talk("", "ch06.zhifu.short")
    return
end
if not (take("material_fuzhi", 12) and take("material_dansha", 3)) then
    talk("", "ch06.zhifu.short")
    return
end

talk("", "ch06.zhifu.lay")
talk("", "ch06.zhifu.first")
talk("", "ch06.zhifu.burn")
talk("", "ch06.zhifu.more")
talk("", "ch06.zhifu.last")
talk("", "ch06.zhifu.curse")

talk("", "ch06.zhifu.ask")
talk("kusang", "ch06.zhifu.nobody")
talk("kusang", "ch06.zhifu.hundred")
talk("kusang", "ch06.zhifu.master")
talk("", "ch06.zhifu.why")
talk("kusang", "ch06.zhifu.fushu")
talk("kusang", "ch06.zhifu.advice")
talk("", "ch06.zhifu.bow")
talk("", "ch06.zhifu.desk")

flag.set("ch06.dingshen_qiu")
flag.set("ch06.zhifu")
