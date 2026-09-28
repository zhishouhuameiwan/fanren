-- 二选一分支。三条路各设不同旗标，测试据此判断脚本走了哪边。
local pick = choice{ "t.opt_a", "t.opt_b" }
if pick == 1 then
    flag.set("picked_first")
elseif pick == 2 then
    flag.set("picked_second")
else
    flag.set("cancelled")
end
