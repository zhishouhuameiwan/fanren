-- @hook ch13_tianxing trigger_zhaiyuan interact once
-- Facts and participants: docs/ch13-design.md 13.1 段 10。3.2 n10：advance_days(1)（3.3 次日尾随、夜里）；置 ch13.qianru。
-- 「易敬」全名 daoqiyuan 起才出口：本段只称「易兄」「锦衣大汉」（12.2）；ch508 把张姓误作另一姓的那处笔误 0 处（16.1 第 56 条）。
talk("", "ch13.qianru.follow")
talk("", "ch13.qianru.enter")
talk("", "ch13.qianru.argue")
talk("yi_jing", "ch13.qianru.decide")
talk("liu_furen", "ch13.qianru.lend")
advance_days(1)
talk("", "ch13.qianru.side_door")
talk("", "ch13.qianru.two_faces")
talk("", "ch13.qianru.doubt")
talk("", "ch13.qianru.night")
flag.set("ch13.qianru")
