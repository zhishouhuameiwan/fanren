-- @hook ch12_dadian trigger_qunxiang enter once
-- Facts and participants: docs/ch12-design.md 13.1 n10. 3.2：advance_days(8)；让 / 不让（不让则威压后照让）合流；取消 return。
-- 抉择在拨日之前，取消即 return，重踩不会重复拨日。元婴诸人只写「看不透」（16.1 第 9 条）。
talk("", "ch12.qunxiang.enter")
talk("", "ch12.qunxiang.guess")
talk("", "ch12.qunxiang.glance")
talk("qingyi_jushi", "ch12.qunxiang.qing")
talk("wen_furen", "ch12.qunxiang.wen")
talk("", "ch12.qunxiang.days")
talk("", "ch12.qunxiang.wan")
talk("", "ch12.qunxiang.man")
talk("man_huzi", "ch12.qunxiang.claim")
local pick = choice({"ch12.qunxiang.opt_yield", "ch12.qunxiang.opt_stay"})
if not pick then return end
if pick == 1 then
    talk("", "ch12.qunxiang.yield")
else
    talk("", "ch12.qunxiang.stay")
end
advance_days(8)
talk("", "ch12.qunxiang.whisper")
talk("xinggong_zhifa_leng", "ch12.qunxiang.rules")
flag.set("ch12.qunxiang")
