-- @hook ch08_jinmacheng trigger_biyun enter once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.jinma") == 0 or flag.get("ch08.jiuren") ~= 0 then return end
talk("", "ch08.biyun.t1")
talk("xin_ruyin", "ch08.biyun.t2")
talk("", "ch08.biyun.f1")
flag.set("ch08.jiuren", 1)
