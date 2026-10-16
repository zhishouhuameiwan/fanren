-- @hook ch13_wuzhong trigger_chuguan enter once
-- Facts and participants: docs/ch13-design.md 13.1 段 28。3.2 n28：开头 not realm.at_least(realm.CORE_MID) → 一句「瓶颈未破」return
-- （不置旗、不拨日；once 不兑现，往外走每跨一次再判一次）；advance_days(3600) 两段（3.3 二十余年）；置 ch13.chuguan。
-- 坐关的年数由玩家在静室里定（3.3 窗口），文案不写具体年数；「青灵门」「鹰鸢兽」自本段起（12.2）。
if not realm.at_least(realm.CORE_MID) then talk("", "ch13.chuguan.not_yet"); return end
talk("", "ch13.chuguan.broke")
talk("", "ch13.chuguan.ninth")
advance_days(3600)
advance_days(3600)
talk("", "ch13.chuguan.flee")
talk("", "ch13.chuguan.light")
talk("hanli", "ch13.chuguan.ask")
talk("", "ch13.chuguan.who")
talk("sun_shishu", "ch13.chuguan.answer")
flag.set("ch13.chuguan")
