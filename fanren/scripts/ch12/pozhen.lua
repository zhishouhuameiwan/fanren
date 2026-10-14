-- @hook ch12_tushan trigger_huangwu interact once
-- Facts and participants: docs/ch12-design.md 13.1 n02. 3.2：阵势三选一循环，错选一句后回选单；取消 return；advance_days(90)。
talk("", "ch12.pozhen.fog")
talk("shi_die", "ch12.pozhen.read")
talk("", "ch12.pozhen.think")
while true do
    local pick = choice({"ch12.pozhen.opt_kun", "ch12.pozhen.opt_sha", "ch12.pozhen.opt_huan"})
    if not pick then return end
    if pick == 3 then break end
    if pick == 1 then
        talk("", "ch12.pozhen.wrong_kun")
    else
        talk("", "ch12.pozhen.wrong_sha")
    end
end
talk("", "ch12.pozhen.right")
talk("hanli", "ch12.pozhen.modest")
advance_days(90)
talk("", "ch12.pozhen.months")
talk("", "ch12.pozhen.chie")
talk("hanli", "ch12.pozhen.split")
talk("", "ch12.pozhen.ready")
flag.set("ch12.pozhen")
