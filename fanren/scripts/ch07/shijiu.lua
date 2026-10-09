-- @hook ch07_dihuo trigger_shijiu enter once
-- 第七章节点 35b「十九号」。挂在 ch07_dihuo 十九号门内第一格 (22,4)
-- （mode=enter，once=true，guard_flag=ch07.dihuo，set_flag=ch07.feidan）。推门进十九号，门一合上。
--
-- 框架件（施工图 13.1 第 35b 段）：圆厅、三十六间地火间，给他十九号（ch215 原文一处写成十八号，照 ch214 取十九）；
-- 屋里的陈设能控火（写法本作自出：原著龙首、葫芦、蒲团那一套陈设不照搬）；门一关，几名结丹合力才进得来；他悬鼎、试火；
-- 二十几炉连凝丹都过不去；几乎要放弃、想回去正经拜师学炼丹；临走前又不甘心开一炉——**地火交到玩家手上**
-- （facility_dihuo，require_flag ch07.feidan；筑基丹方成算约三成，施工图第 7 节）。
-- 账：药粉 −22（第 9 节去向表 35b）、一盒废丹（story_feidan_yuhe）。药粉本章只有地火用得上（别处没有开得了炉的地方），
-- 节点 34 给的 40 份到这里一定还在；take 仍看返回值。

talk("", "ch07.shijiu.hall")
talk("", "ch07.shijiu.nineteen")
talk("", "ch07.shijiu.room")
talk("", "ch07.shijiu.door")
talk("", "ch07.shijiu.hang")
if take("material_zhuji_yaofen", 22) then
    give("story_feidan_yuhe")
end
talk("", "ch07.shijiu.fail")
talk("", "ch07.shijiu.box")
talk("", "ch07.shijiu.quit")
talk("", "ch07.shijiu.again")

flag.set("ch07.feidan")
