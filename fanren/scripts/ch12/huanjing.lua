-- @hook ch12_taqian trigger_changlang enter once
-- Facts and participants: docs/ch12-design.md 13.1 n25. 3.2：两段心境抉择，错选一句（婆罗珠灼热）后回选单，不失败（拍板 15）；
-- 一次就选对的段置 ch12.xinjing 对应位（1 色、2 惧）；advance_days(1)；置 ch12.huanjing。
-- 两段抉择都在一切状态改动之前，取消即 return。色段原著描写一样不用（13 第 6 条）。
talk("", "ch12.huanjing.test")
talk("", "ch12.huanjing.corridor")
local se_first = true
while true do
    local pick = choice({"ch12.huanjing.opt_listen", "ch12.huanjing.opt_walk"})
    if not pick then return end
    if pick == 2 then break end
    talk("", "ch12.huanjing.listen")
    se_first = false
end
talk("", "ch12.huanjing.walk")
talk("", "ch12.huanjing.hall")
talk("", "ch12.huanjing.girl")
local ju_first = true
while true do
    local pick = choice({"ch12.huanjing.opt_go", "ch12.huanjing.opt_strike"})
    if not pick then return end
    if pick == 2 then break end
    talk("", "ch12.huanjing.go")
    ju_first = false
end
talk("", "ch12.huanjing.strike")
talk("", "ch12.huanjing.after")
local bits = 0
if se_first then bits = bits + 1 end
if ju_first then bits = bits + 2 end
if bits > 0 then flag.set("ch12.xinjing", bits) end
advance_days(1)
talk("", "ch12.huanjing.tower")
flag.set("ch12.huanjing")
