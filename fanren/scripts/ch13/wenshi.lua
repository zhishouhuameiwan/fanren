-- @hook ch13_waihai npc_wen_siyue npc
-- Facts and participants: docs/ch13-design.md 13.1 段 32。3.2 n32：已置 → 一句 return（NPC 随后撤，hidden ch13.wenshi）；
-- 前置 ch13.jiaohuan 开头判；advance_days(1)（3.3）；置 ch13.wenshi。
-- 她的对象在秘市荒岛东侧绿山小洞府前（4.1），文案就把洞府开在这座岛上（施工偏差：原著是两日后另一座荒岛）。
-- 侍妾之说他作罢，只要巢穴（E 表 26）；「厉先生」自本段起（12.2）；妙音门惊变只从她口中知道结果（判据③）。
if flag.get("ch13.wenshi") ~= 0 then talk("", "ch13.wenshi.gone"); return end
if flag.get("ch13.jiaohuan") == 0 then talk("", "ch13.wenshi.early"); return end
advance_days(1)
talk("", "ch13.wenshi.cave")
talk("wen_siyue", "ch13.wenshi.name")
talk("hanli", "ch13.wenshi.ok")
talk("wen_siyue", "ch13.wenshi.coup")
talk("", "ch13.wenshi.infer")
talk("hanli", "ch13.wenshi.release")
talk("wen_siyue", "ch13.wenshi.warn")
flag.set("ch13.wenshi")
