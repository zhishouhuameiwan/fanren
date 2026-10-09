-- @hook ch08_dongfu trigger_chuanyin interact once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.biguan") == 0 or flag.get("ch08.hongfu") ~= 0 then return end
talk("wu_xuan", "ch08.chuanyin.t1")
talk("", "ch08.chuanyin.t2")
talk("li_huayuan_shifu", "ch08.chuanyin.t3")
talk("li_huayuan_shifu", "ch08.chuanyin.f1")
talk("", "ch08.chuanyin.f2")
advance_days(24)
teleport("ch08_yanlingbao", 28, 37)
flag.set("ch08.hongfu", 1)
