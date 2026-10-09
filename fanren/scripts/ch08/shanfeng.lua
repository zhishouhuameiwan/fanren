-- @hook ch08_dongfu trigger_shanfeng interact once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.dengji") == 0 or flag.get("ch08.lingquan") ~= 0 then return end
talk("", "ch08.shanfeng.t1")
talk("", "ch08.shanfeng.t2")
talk("", "ch08.shanfeng.t3")
advance_days(1)
talk("", "ch08.shanfeng.f1")
flag.set("ch08.lingquan", 1)
