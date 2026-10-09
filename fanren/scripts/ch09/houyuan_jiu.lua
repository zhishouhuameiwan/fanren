-- @hook ch08_tianxing_fangshi trigger_houyuan_jiu interact once
-- Facts and participants: docs/ch09-design.md 13.1.

advance_days(18)
talk("", "ch09.houyuan_jiu.wait")
talk("xulao_qipu", "ch09.houyuan_jiu.loss")
talk("", "ch09.houyuan_jiu.blame")
give("story_baizhu_feidao", 1)
local ok = magic.learn("magic_ji_baizhudao")
if not ok then
    talk("", "ch09.houyuan_jiu.error")
    return
end
local pick
repeat pick = choice({"ch09.houyuan_jiu.opt1", "ch09.houyuan_jiu.opt2"}) until pick
if pick == 1 then
    talk("hanli", "ch09.houyuan_jiu.comfort")
else
    talk("", "ch09.houyuan_jiu.silent")
end
talk("", "ch09.houyuan_jiu.lesson")
advance_days(3)
flag.set("ch09.feidao", pick)
teleport("ch09_wumingshan", 7, 22)
