-- @hook ch10_jinhai trigger_jiaoshi interact once
talk("", "ch10.jiaoshi.worry")
local pick = choice({"ch10.jiaoshi.opt_sit", "ch10.jiaoshi.opt_array"})
if pick ~= 2 then
    if pick == 1 then talk("", "ch10.jiaoshi.again") end
    return
end
talk("", "ch10.jiaoshi.array")
flag.set("ch10.xinshen")
