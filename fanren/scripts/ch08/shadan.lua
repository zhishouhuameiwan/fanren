-- @hook ch08_yuejing trigger_shadan interact once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.yuehuang") == 0 or flag.get("ch08.junling") ~= 0 then return end
give("pill_xuening_wuxing_dan", 5)
give("story_juhun_bo", 1)
give("talisman_xuelingzuan", 1)
give("story_heisha_yujian", 1)
give("story_jinpa_ditu", 1)
talk("", "ch08.shadan.t1")
talk("", "ch08.shadan.t2")
talk("", "ch08.shadan.f1")
talk("", "ch08.shadan.f2")
talk("ma_shizhi", "ch08.shadan.t3")
talk("chen_qiaoqian", "ch08.shadan.t4")
talk("chen_qiaoqian", "ch08.shadan.f3")
local pick = choice({"ch08.shadan.choice1", "ch08.shadan.choice2"})
if not pick then return end
flag.set("ch08.junling", pick)
talk("", "ch08.shadan.t5")
advance_days(6)
teleport("ch05_nancheng", 41, 17)
if flag.get("ch08.junling") == 0 then flag.set("ch08.junling", 1) end
