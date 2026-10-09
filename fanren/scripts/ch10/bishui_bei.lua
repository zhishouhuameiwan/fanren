-- @hook ch10_jinhai trigger_bishui_bei interact
-- Source: scripts/ch08/templates/zhenwei.lua.in
local function place(flag_name, bit)
    local mask = flag.get(flag_name)
    if (mask & bit) ~= 0 then talk("", "ch10.zhenwei.already") return end
    flag.set(flag_name, mask | bit)
    talk("", "ch10.bishui.bei")
end
place("ch10.bishui", 8)
