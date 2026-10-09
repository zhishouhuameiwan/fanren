-- @hook ch08_dongfu trigger_chucang interact once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.kaifu") == 0 or flag.get("ch08.qiannian") ~= 0 then return end
if item.count_aged("herb_zigui_hua", 1000) < 1 then
talk("", "ch08.chucang.t1")
    return
end
talk("", "ch08.chucang.t2")
flag.set("ch08.qiannian", 1)
