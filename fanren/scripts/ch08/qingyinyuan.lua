-- @hook ch08_yuejing trigger_qingyinyuan interact once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.qb_xuezhou") == 0 or flag.get("ch08.qb_wumei") ~= 0 then return end
talk("", "ch08.qingyinyuan.t1")
give("talisman_tianleizi", 1)
talk("", "ch08.qingyinyuan.t2")
talk("", "ch08.qingyinyuan.f1")
flag.set("ch08.qb_wumei", 1)
