-- @hook ch06_huangfenggu trigger_baoming interact once
-- 第七章节点 16「报名：王师叔」。挂在 ch06_huangfenggu 大殿前王师叔身边那张案 (25,12)
-- （mode=interact，once=true，guard_flag=ch07.fudan_fa，set_flag=ch07.baoming）。去报名的是他。
--
-- 施工图写「百机堂另一格」：王师叔的 NPC 一直站在大殿前 (26,12)（第 6 章，没有撤场旗标），本作把收报名的案摆在他身边，
-- 脚本说的与地图上站的是同一处（第 6 章校对 LOW-7 的教训；记第 18 节）。
-- 框架件（施工图 13.1 第 16 段）：王师叔收报名；新人来送死、九层到十一层太快——拉到一边再测：仍是伪灵根；
-- 二选一：1 误食异果（龙鳞果，王师叔信了一半，原著）/ 2 不解释（王师叔不再问，多看了一眼）；取消按 1 算；
-- 王师叔交代注意事项。节点 32b 王师叔在出口有一句按它分（原著翻古书对上那一段不用）。

talk("", "ch07.baoming.desk")
talk("wang_shishu", "ch07.baoming.wang_new")
talk("wang_shishu", "ch07.baoming.wang_fast")
talk("", "ch07.baoming.test")
talk("wang_shishu", "ch07.baoming.wang_ask")

local pick = choice{
    "ch07.baoming.opt_fruit",
    "ch07.baoming.opt_plain",
}
if pick ~= 2 then
    pick = 1
end
if pick == 1 then
    talk("", "ch07.baoming.fruit")
    talk("", "ch07.baoming.fruit_wang")
else
    talk("", "ch07.baoming.plain")
    talk("wang_shishu", "ch07.baoming.plain_wang")
end
talk("wang_shishu", "ch07.baoming.rules")
talk("", "ch07.baoming.done")

flag.set("ch07.baoming", pick)
