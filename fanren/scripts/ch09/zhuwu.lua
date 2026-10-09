-- @hook ch09_wumingshan trigger_zhuwu interact once
-- Facts and participants: docs/ch09-design.md 13.1.

talk("", "ch09.zhuwu.tablet")
talk("xin_ruyin", "ch09.zhuwu.maid")
talk("xin_ruyin", "ch09.zhuwu.marry")
talk("hanli", "ch09.zhuwu.seen")
talk("xin_ruyin", "ch09.zhuwu.death")
talk("xin_ruyin", "ch09.zhuwu.outer")
talk("", "ch09.zhuwu.inner")
talk("hanli", "ch09.zhuwu.pieces")
talk("xin_ruyin", "ch09.zhuwu.buy")
talk("xin_ruyin", "ch09.zhuwu.expose")
talk("xin_ruyin", "ch09.zhuwu.repair")
talk("", "ch09.zhuwu.boxes")
talk("xin_ruyin", "ch09.zhuwu.request")
talk("", "ch09.zhuwu.pulse")
talk("xin_ruyin", "ch09.zhuwu.body")
talk("xin_ruyin", "ch09.zhuwu.formula")
talk("xin_ruyin", "ch09.zhuwu.teacher")
local pick
repeat pick = choice({"ch09.zhuwu.opt1", "ch09.zhuwu.opt2"}) until pick
if pick == 1 then
    talk("hanli", "ch09.zhuwu.medicine")
    talk("xin_ruyin", "ch09.zhuwu.enough")
else
    talk("", "ch09.zhuwu.quiet")
end
talk("xin_ruyin", "ch09.zhuwu.word")
talk("hanli", "ch09.zhuwu.limit")
give("story_qi_yixia", 1)
give("story_xin_yixia", 1)
talk("xin_ruyin", "ch09.zhuwu.month")
advance_days(1)
flag.set("ch09.chengnuo", pick)
teleport("ch08_tianxing_fangshi", 24, 33)
