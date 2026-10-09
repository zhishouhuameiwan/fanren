-- @hook ch08_dongfu trigger_zhenwei_shui interact
if flag.get("ch08.quqi") == 0 or flag.get("ch08.buzhen1") ~= 0 then return end
local function place(flag_name, bit)
    local mask = flag.get(flag_name)
    if (mask & bit) ~= 0 then talk("", "ch08.zhen.already") return end
    flag.set(flag_name, mask | bit)
    talk("", "ch08.zhen.insert")
end
place("ch08.zhenqi1", 4)
