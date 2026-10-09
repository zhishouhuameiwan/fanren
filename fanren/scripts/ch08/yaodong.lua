-- @hook ch08_lingkuang trigger_yaodong enter once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.tuoshen") == 0 or flag.get("ch08.lingkuang") ~= 0 then return end
talk("yu_xing", "ch08.yaodong.t1")
talk("", "ch08.yaodong.t2")
talk("xuan_le", "ch08.yaodong.f1")
flag.set("ch08.lingkuang", 1)
