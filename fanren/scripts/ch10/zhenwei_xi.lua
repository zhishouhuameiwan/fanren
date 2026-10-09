-- @hook ch10_xiaohuan trigger_zhenwei_xi interact
-- Source: scripts/ch08/templates/zhenwei.lua.in
local function place(flag_name, bit)
    local mask = flag.get(flag_name)
    if (mask & bit) ~= 0 then talk("", "ch10.zhenwei.already") return end
    flag.set(flag_name, mask | bit)
    talk("", "ch10.zhenwei.xi")
end
place("ch10.zhenqi", 4)
