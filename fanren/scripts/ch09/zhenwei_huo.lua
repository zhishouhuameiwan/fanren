-- @hook ch08_lingkuang trigger_zhenwei_jiu_huo interact
-- Facts and participants: docs/ch09-design.md 13.1.
local function place(flag_name, bit)
    local mask = flag.get(flag_name)
    if (mask & bit) ~= 0 then talk("", "ch09.zhen.already") return end
    flag.set(flag_name, mask | bit)
    talk("", "ch09.zhen.insert")
end
place("ch09.zhenqi", 8)
