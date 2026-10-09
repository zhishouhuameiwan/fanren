-- @hook ch09_wumingshan trigger_zhuwu_er interact once
-- Facts and participants: docs/ch09-design.md 13.1.

talk("xin_ruyin", "ch09.zhuwu_er.finished")
give("story_xiufu_yujian", 1)
give("story_xin_zhenqi", 1)
talk("xin_ruyin", "ch09.zhuwu_er.arrays")
talk("xin_ruyin", "ch09.zhuwu_er.life")
talk("hanli", "ch09.zhuwu_er.renew")
talk("", "ch09.zhuwu_er.study")
talk("", "ch09.zhuwu_er.news")
advance_days(3)
advance_days(1)
flag.set("ch09.xiufu")
teleport("ch09_yuanwu", 43, 9)
