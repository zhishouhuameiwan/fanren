-- @hook ch08_yuejing trigger_huayuan_yj enter once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.buzhen2") == 0 or flag.get("ch08.huayuan") ~= 0 then return end
talk("chen_qiaoqian", "ch08.huayuan_yj.t1")
local pick = choice({"ch08.huayuan_yj.choice1", "ch08.huayuan_yj.choice2"})
if not pick then return end
flag.set("ch08.huayuan", pick)
talk("", "ch08.huayuan_yj.t2")
talk("", "ch08.huayuan_yj.f1")
if flag.get("ch08.huayuan") == 0 then flag.set("ch08.huayuan", 1) end
