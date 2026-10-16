-- @hook ch13_tianxing trigger_dongfu39 interact once
-- Facts and participants: docs/ch13-design.md 13.1 段 8。3.2 n08：只置 ch13.qifu。
-- 药园不由脚本搬作物：玩家经 facility_yaoyuan_39 自采（契约 E2），文案只点一句药园在旁，不点名按键。
talk("", "ch13.qifu.street")
talk("", "ch13.qifu.sense")
talk("", "ch13.qifu.insects")
talk("", "ch13.qifu.garden")
talk("", "ch13.qifu.leave")
talk("", "ch13.qifu.worry")
talk("", "ch13.qifu.plan")
flag.set("ch13.qifu")
