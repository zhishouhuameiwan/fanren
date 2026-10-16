-- @hook ch13_wuzhong trigger_shichuang interact once
-- Facts and participants: docs/ch13-design.md 13.1 段 19。3.2 n19：take story_xuangu_leigu 1（false 换一句照给，不设死局）；
-- give story_xuanyin_jing 1；advance_days(15)（3.3 试鼎半月）；置 ch13.xuanyin。
-- 玄阴经只是剧情物品，秘术只在文案（E 表 9）；「辟邪神雷」自本段起（12.2）。暗场人物 0 处。
talk("", "ch13.xuanyin.rib")
talk("", "ch13.xuanyin.yin")
talk("", "ch13.xuanyin.open")
if take("story_xuangu_leigu", 1) then
    talk("", "ch13.xuanyin.jing")
else
    talk("", "ch13.xuanyin.jing_fail")
end
give("story_xuanyin_jing", 1)
talk("", "ch13.xuanyin.first")
talk("", "ch13.xuanyin.second")
talk("", "ch13.xuanyin.corpsefire")
advance_days(15)
talk("", "ch13.xuanyin.ding")
talk("", "ch13.xuanyin.pearl")
flag.set("ch13.xuanyin")
