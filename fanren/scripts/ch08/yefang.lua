-- @hook ch08_yuejing trigger_yefang interact once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.qinzhai") == 0 or flag.get("ch08.fengwu") ~= 0 then return end
talk("mo_fengwu", "ch08.yefang.t1")
if flag.get("ch05.fengwu_yigao") ~= 0 then
talk("mo_fengwu", "ch08.yefang.t2")
end
talk("mo_fengwu", "ch08.yefang.t3")
talk("hanli", "ch08.yefang.t4")
local pick = choice({"ch08.yefang.choice1", "ch08.yefang.choice2"})
if not pick then return end
flag.set("ch08.fengwu", pick)
talk("mo_fengwu", "ch08.yefang.f1")
talk("mo_fengwu", "ch08.yefang.f2")
if flag.get("ch08.fengwu") == 0 then flag.set("ch08.fengwu", 1) end
