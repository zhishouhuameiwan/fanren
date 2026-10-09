-- @hook ch07_jindi_wai trigger_xiang enter once
-- 第七章节点 19a「荒山：向之礼」。挂在 ch07_jindi_wai 荒山石坳的口子 (10,4)(10,5)
-- （mode=enter，once=true，guard_flag=ch07.jihe，set_flag=ch07.xiang）。向之礼找上他。
--
-- 框架件（施工图 13.1 第 19a 段）：向之礼带着一个少年来找他结伙（法力低的抱团）；他拒绝（或敷衍）。
-- 二选一：1 从不与人同行（原著）/ 2 到时再说（向之礼听出是推托，走开）；取消按 1 算。节点 32b 向之礼最后一个爬出来时按它多一句。

talk("", "ch07.xiang.come")
talk("xiang_zhili", "ch07.xiang.pitch")
talk("xiang_zhili", "ch07.xiang.pitch2")
talk("", "ch07.xiang.think")

local pick = choice{
    "ch07.xiang.opt_no",
    "ch07.xiang.opt_later",
}
if pick ~= 2 then
    pick = 1
end
if pick == 1 then
    talk("", "ch07.xiang.no")
    talk("xiang_zhili", "ch07.xiang.no_reply")
else
    talk("", "ch07.xiang.later")
    talk("xiang_zhili", "ch07.xiang.later_reply")
end
talk("", "ch07.xiang.gone")

flag.set("ch07.xiang", pick)
