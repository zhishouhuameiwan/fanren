-- @hook ch06_baiyaoyuan trigger_chaibao interact once
-- 第七章节点 1「拆包」。挂在 ch06_baiyaoyuan 居室里放包袱的小桌 (22,3)
-- （mode=interact，once=true，guard_flag=ch06.done，set_flag=ch07.chaibao）。拆叶师叔那一包的是他。
--
-- 第 6 章章末的次日（第 6 章收在「第二日醒来」）。框架件照施工图 13.1 第 1 段：
--   叶师叔那一包：中阶灵石两块（火、土）、精钢环、黑雾小旗、黄铜瓶、高阶土牢符两张（ch161 交代给过两张高阶符，ch183 用的正是土牢）；
--   黄铜瓶试液：绿液撑得久些，过时照样没了（bottle.spend 看返回值：瓶里没液就跳过这一句，不设死局）；
--   残片仍盖在瓶上（施工图 1.4）；马师伯交给他打理的苗（黄精 3、紫参 2、血红芝 1、黄精芝 2，全是零年）；
--   园角另开两槽 field_baiyaoyuan_jiao（千年药要避开马师伯的眼——ch161 的伏笔，这里不说破）。
-- 讲述次序是本作自己的：先园子、后包袱、夜里试瓶（原著先写灵石）。
-- 目标链第 2、3 步的 R-3 提示在 data/text/ch07_objectives.json。

talk("", "ch07.chaibao.dawn")
talk("", "ch07.chaibao.seedlings")
give("herb_huangjing_cao", 3, 0)
give("herb_zishen_cao", 2, 0)
give("herb_xuehong_zhi", 1, 0)
give("herb_huangjing_zhi", 2, 0)
talk("", "ch07.chaibao.corner")
field.unlock("field_baiyaoyuan_jiao", 2)

talk("", "ch07.chaibao.bundle")
talk("", "ch07.chaibao.stones")
give("material_lingshi_zhong", 2)
talk("", "ch07.chaibao.tools")
give("story_jinggang_huan")
give("story_heiwu_qi")
give("story_huangtong_ping")
talk("", "ch07.chaibao.fu")
give("talisman_tulao_fu", 2)

if bottle.spend(1) then
    talk("", "ch07.chaibao.test")
    talk("", "ch07.chaibao.test_gone")
else
    talk("", "ch07.chaibao.test_none")
end
talk("", "ch07.chaibao.canpian")
talk("", "ch07.chaibao.plan")

flag.set("ch07.chaibao")
