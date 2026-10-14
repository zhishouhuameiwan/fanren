-- @hook ch12_hanlitai trigger_dongkou interact once
-- Facts and participants: docs/ch12-design.md 13.1 n31. 3.2：领丹方玉简、虚天鼎；战前 give 神雷 2；战斗 ④；战后 take 神雷余数；
-- 领乾蓝冰珠、玄骨肋骨、金雷竹小箭；置 ch12.xuangu；败 game_over。
-- 死亡口径：乌丑只写倒下；玄骨是圣火反噬、他以剑芒封路（B 表 3），全章 0 处「杀死玄骨」「自焚」。
-- 神雷余数为 0 时不调 take（契约第 4 节）。「修罗圣火」「乾蓝冰珠」从本段起解禁（12.2）。
talk("wu_chou", "ch12.xuangu.push")
talk("", "ch12.xuangu.strike")
talk("xuan_gu", "ch12.xuangu.spider")
talk("xuan_gu", "ch12.xuangu.truth")
give("story_jiuqu_danfang", 1)
talk("", "ch12.xuangu.recipe")
talk("xuan_gu", "ch12.xuangu.deal")
talk("", "ch12.xuangu.pull")
give("story_xutian_ding", 1)
talk("xuan_gu", "ch12.xuangu.turn")
talk("", "ch12.xuangu.fuse")
give("talisman_bixie_shenlei", 2)
talk("", "ch12.xuangu.thunder")
local won = battle("b12_hanlitai_xuangu")
if not won then game_over(); return end
talk("", "ch12.xuangu.win")
talk("", "ch12.xuangu.ash")
local n = item.count("talisman_bixie_shenlei")
if n > 0 then
    if take("talisman_bixie_shenlei", n) then
        talk("", "ch12.xuangu.spent")
    else
        talk("", "ch12.xuangu.keep")
    end
else
    talk("", "ch12.xuangu.spent")
end
give("story_qianlan_zhu", 1)
talk("", "ch12.xuangu.pearl")
give("story_xuangu_leigu", 1)
give("story_jinlei_xiaojian", 1)
talk("", "ch12.xuangu.rib")
flag.set("ch12.xuangu")
