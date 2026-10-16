-- @hook ch13_wuzhong trigger_dizhen enter once
-- Facts and participants: docs/ch13-design.md 13.1 段 21。3.2 n21：advance_days(5)（3.3 地动、荒岛数日）；置 ch13.dujie。
-- 八级只能逃（1.1 第 4 条）；巨龟与毒蛟的形貌自出且从简；末句只写他改主意不迁府、加阵——
-- 他不知道的后事 0 处（16.1 第 42 条）。「化形雷劫」「毒蛟」「金霞」「霍氏」「伪洞府」自本段起（12.2）。
talk("", "ch13.dujie.quake")
talk("", "ch13.dujie.flood")
talk("", "ch13.dujie.turtle")
talk("", "ch13.dujie.jiao")
talk("", "ch13.dujie.three")
talk("", "ch13.dujie.fight")
talk("", "ch13.dujie.flee")
advance_days(5)
talk("", "ch13.dujie.back")
talk("", "ch13.dujie.stay")
talk("", "ch13.dujie.decoy")
flag.set("ch13.dujie")
