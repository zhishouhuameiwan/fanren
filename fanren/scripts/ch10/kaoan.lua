-- @hook ch10_haichuan trigger_kaoan interact once
local mask = flag.get("ch10.xuehua")
local n = 0
for _, bit in ipairs({1, 2, 4}) do
    if (mask & bit) ~= 0 then n = n + 1 end
end
advance_days(3 - n)
if n < 3 then talk("", "ch10.kaoan.less") end
talk("", "ch10.kaoan.port")
if flag.get("ch10.yuyan") >= 1 then
    talk("gu_dongzhu", "ch10.kaoan.land.clear")
else talk("gu_dongzhu", "ch10.kaoan.land.garbled") end
flag.set("ch10.kaoan")
teleport("ch10_kuixing", 52, 40)
