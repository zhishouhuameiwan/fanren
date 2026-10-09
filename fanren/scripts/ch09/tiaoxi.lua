-- @hook ch09_huangshan trigger_tiaoxi interact once
-- Facts and participants: docs/ch09-design.md 13.1.

talk("", "ch09.tiaoxi.rest")
talk("", "ch09.tiaoxi.decide")
talk("", "ch09.tiaoxi.business")
talk("", "ch09.tiaoxi.war")
talk("", "ch09.tiaoxi.array")
talk("", "ch09.tiaoxi.promise")
talk("", "ch09.tiaoxi.journey")
advance_days(6)
flag.set("ch09.tiaoxi")
teleport("ch09_yuanwu", 8, 32)
