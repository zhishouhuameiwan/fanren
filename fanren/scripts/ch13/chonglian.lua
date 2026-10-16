-- @hook ch13_wuzhong trigger_lianjianshi interact once
-- Facts and participants: docs/ch13-design.md 13.1 段 20。3.2 n20：take material_lianjing 1（false 换一句照走）；
-- advance_days(360)（3.3 近一年）；magic.learn("magic_fengyun_heji")（false 换一句照走）；置 ch13.chonglian。
-- 重炼不走炼器面板（E 表 8）；「灵乳」不扣（B6）。
talk("", "ch13.chonglian.plan")
if take("material_lianjing", 1) then
    talk("", "ch13.chonglian.seal")
else
    talk("", "ch13.chonglian.no_crystal")
end
talk("", "ch13.chonglian.year")
talk("", "ch13.chonglian.ape")
advance_days(360)
local learned = magic.learn("magic_fengyun_heji")
if learned then
    talk("", "ch13.chonglian.join")
else
    talk("", "ch13.chonglian.join_fail")
end
flag.set("ch13.chonglian")
