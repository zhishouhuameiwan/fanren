-- @hook ch08_jiayuan_shanlin trigger_milin_zhenyan interact once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.quhun") == 0 or flag.get("ch08.buzhen3") ~= 0 then return end
if flag.get("ch08.zhenqi3") ~= 15 then
talk("", "ch08.milin_zhenyan.t1")
return
end
talk("", "ch08.milin_zhenyan.t2")
flag.set("ch08.buzhen3", 1)
