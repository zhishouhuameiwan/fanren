-- @hook ch07_jindi_waiwei trigger_liangshi interact once
-- 第七章节点 21b「山崖两尸；丝线」。挂在 ch07_jindi_waiwei 崖下两具尸首那一格 (7,6)
-- （mode=interact，once=true，guard_flag=ch07.wulongtan，set_flag=ch07.sixian）。翻两具尸首的是他。
--
-- 框架件（施工图 13.1 第 21b 段）：山崖下，巨剑门弟子与黄枫谷弟子同归于尽；两人的储物袋都不见了——还有第四个人；
-- 他试探一下、没有动静，走人（**茅草地里的人是暗场**，16.1 第 9 条：这里一个字也不提她）；捡走那团透明丝线（十余丈、几乎看不见、锋利）。
-- give("weapon_wuming_sixian")：暗器，揣在身上普攻多一类（施工图 8.2；BATTLE_EXTRA_MEANS 的「暗器」来路）。

talk("", "ch07.liangshi.cliff")
talk("", "ch07.liangshi.bodies")
talk("", "ch07.liangshi.how")
talk("", "ch07.liangshi.bags")
talk("", "ch07.liangshi.probe")
talk("", "ch07.liangshi.thread")
give("weapon_wuming_sixian")

flag.set("ch07.sixian")
