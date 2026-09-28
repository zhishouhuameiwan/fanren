-- @hook ch02_yaopu trigger_renyao enter once
-- 第二章节点 1「入谷第一课：认药、分辨年份」。段一（入谷首年·春）的进场事件。
-- 挂在 ch02_yaopu 的进场 trigger（mode=enter, once=true）。
--
-- 这一节要教的是全作通用的一条规矩：灵草的价值几乎全在年份上。
-- 不写成教程文本，而是让管事拿两株一样大、价钱差好几倍的干药给玩家看——
-- 玩家自己看出差别，比任何说明都牢。年份怎么涨、怎么变钱，留给段一后面
-- 那次「卖六块灵石」和段五那次「卖一百四十五」自己说话。
--
-- field.unlock 在这里调，不在地图上等 openFacility 顺手建：这块田是剧情给的，
-- 剧情没走到就不该存在。已存在时它只补足槽位，不动已种的药（见 P3 契约第 1 节）。
--
-- 段一的推进闸门之一记在 ch02.diyike_day 上：节点 3 用 today() 减它，判断
-- 打坐是否已累计满三十日。时间的流逝必须挂在玩家的操作上，不能是「点一下继续」。

if flag.get("ch02.renyao_done") == 1 then
    talk("yaopu_guanshi", "ch02.renyao.again")
    return
end

talk("", "ch02.renyao.spring")
talk("yaopu_guanshi", "ch02.renyao.guanshi_jian")
talk("yaopu_guanshi", "ch02.renyao.guanshi_ren")
talk("yaopu_guanshi", "ch02.renyao.guanshi_nian")
talk("", "ch02.renyao.hanli_look")
talk("yaopu_guanshi", "ch02.renyao.guanshi_rule")

-- 八槽的灵田，id 见 docs/interfaces-p3-script.md 第 4 节，编剧与关卡共用这一个。
field.unlock("shenshougu_yaopu", 8)

talk("yaopu_guanshi", "ch02.renyao.guanshi_seed")

-- 三株种苗。黄精是一阶药，年份上限 44（herbMaxAgeForGrade），正好是三滴绿液
-- 推到顶的数，段五的算账就落在这个上限上。给贱药不是省事，是让上限成立。
give("herb_huangjing_cao", 3)

talk("yaopu_guanshi", "ch02.renyao.guanshi_dong")
talk("", "ch02.renyao.field")
talk("yaopu_guanshi", "ch02.renyao.guanshi_last")
talk("", "ch02.renyao.hint")

flag.set("ch02.diyike_day", today())
flag.set("ch02.renyao_done")
