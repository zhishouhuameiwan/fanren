-- @hook ch09_yuanwu trigger_fengjiao interact once
-- Facts and participants: docs/ch09-design.md 13.1.
talk("", "ch09.fengjiao.two")
local pick
repeat
    pick = choice({"ch09.fengjiao.opt1", "ch09.fengjiao.opt2",
        "ch09.fengjiao.opt3", "ch09.fengjiao.opt4", "ch09.fengjiao.opt5"})
    if pick == 1 then
        talk("", "ch09.fengjiao.east")
    elseif pick == 2 then
        talk("", "ch09.fengjiao.west")
    elseif pick == 3 then
        talk("", "ch09.fengjiao.north")
        talk("", "ch09.fengjiao.sea")
        if flag.get("ch09.path.baichi_sanxiu_b_dating") ~= 0 then
            talk("", "ch09.fengjiao.heardnorth")
        end
    elseif pick == 4 then
        talk("", "ch09.fengjiao.south")
        talk("", "ch09.fengjiao.arts")
        if flag.get("ch09.path.baichi_sanxiu_c_dating") ~= 0 then
            talk("", "ch09.fengjiao.heardsouth")
        end
    end
until pick == 5
talk("", "ch09.fengjiao.decide")
advance_days(2)
flag.set("ch09.qulu")
teleport("ch08_tianxing_fangshi", 24, 33)
