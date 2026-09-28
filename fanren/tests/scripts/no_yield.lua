-- 完全不挂起的脚本：startEvent 里一口气跑完。
local total = 0
for i = 1, 10 do total = total + i end
assert(total == 55)
