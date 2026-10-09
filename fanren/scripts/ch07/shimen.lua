-- @hook ch07_yuelu_dian trigger_shimen enter once
-- 第七章节点 5b「丑汉与石门」。挂在 ch07_yuelu_dian 五彩石门前一格 (39,1)(40,1)
-- （mode=enter，once=true，guard_flag=ch07.yinsi，set_flag=ch07.chouhan）。丑汉的冷脸是撞上的。
--
-- 框架件（施工图 13.1 第 5b 段）：五彩禁光的石门，门边石屋里的丑汉（炼气顶层）；问清借地火的规矩；
-- 丑汉瞧不起他、背后讥他穷——他听见了，不发作（施工图 3.2：旁白只写背影一冷，不写杀心；节点 35 他也没报复）。
-- 讥他的那句话本作自己写（原著那一句不用）。置 ch07.chouhan：黄枫谷东口那一群人从这时起站在坡上（节点 6）。

talk("", "ch07.shimen.gate")
talk("", "ch07.shimen.hut")
talk("chou_han", "ch07.shimen.rule")
talk("", "ch07.shimen.board")
talk("", "ch07.shimen.back")
talk("chou_han", "ch07.shimen.sneer")
talk("", "ch07.shimen.walk")

flag.set("ch07.chouhan")
