-- @hook ch08_yuejing trigger_chufa interact once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.huayuan") == 0 or flag.get("ch08.jingong") ~= 0 then return end
talk("", "ch08.chufa.t1")
teleport("ch08_yuejing", 29, 10)
flag.set("ch08.jingong", 1)
