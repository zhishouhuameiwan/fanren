-- @hook ch08_yanlingbao trigger_biwuchang enter once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.hongfu") == 0 or flag.get("ch08.biwu") ~= 0 then return end
talk("", "ch08.biwuchang.t1")
talk("", "ch08.biwuchang.t2")
talk("yanjia_laozhe", "ch08.biwuchang.t3")
talk("", "ch08.biwuchang.t4")
advance_days(1)
talk("", "ch08.biwuchang.f1")
talk("", "ch08.biwuchang.f2")
flag.set("ch08.biwu", 1)
