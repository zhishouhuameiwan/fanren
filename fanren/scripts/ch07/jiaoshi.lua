-- @hook ch07_dixia_zhaoze trigger_jiaoshi interact once
-- 第七章节点 31 前半「醒来；金箱；灵药」，伙伴改版样板 4.2 末与 4.3 开头「困在地下」。挂在 ch07_dixia_zhaoze 西岸蛟尸旁那一格 (9,15)
-- （mode=interact，once=true，guard_flag=ch07.mojiao，set_flag=ch07.xinglai）。拿银剑剖蛟尸的是他。
--
-- 框架件（施工图 13.1 第 31 段前半）：他剖开蛟尸，从蛟腹里掏出一团东西，她一碰炸开粉红烟雾——**暗场**（16.1 第 3 条、13 第 8 条）：
-- 只一行「两个人都失去了知觉」；醒来她的外貌长大了好几岁；她冷冷交代就当没发生过、传出去就杀他（说法自拟），他应下；
-- 她修素女轮回功，损了几年法力（只说「损」）；那团东西只叫「墨蛟的一个毒囊」（那个词 0 处，12.2）；金箱归她；灵药她不要，他全收了。
-- give：成熟的玉髓芝 8（400 年）、紫猴花 7（400 年）、天灵果 7（300 年）——二十二株，节点 32b 上交用；玉髓芝幼苗 2；墨蛟材料 1。
--
-- 伙伴改版样板（docs/ch07-companions-design.md 4.3、4.6）：
--   · 毒囊之后（暗场）默契 +12，演到这里交还操作（内容审查 2026-11-18 第 5 条：不再一口气演完）。
--   · 4.3 开头（回石阶口看封死的通道、评银剑、商定出路）在 fengkou.lua；破禁那几句在 poding.lua；问名在 wenming.lua。
-- 置 ch07.xinglai：石阶口那一格（trigger_fengkou）从此备着。

talk("", "ch07.jiaoshi.sword")
talk("", "ch07.jiaoshi.lump")
talk("", "ch07.jiaoshi.smoke")
talk("", "ch07.jiaoshi.dark")
talk("", "ch07.jiaoshi.wake")
talk("baiyi_shaonv", "ch07.jiaoshi.dream")
talk("", "ch07.jiaoshi.promise")
talk("baiyi_shaonv", "ch07.jiaoshi.gong")
talk("", "ch07.jiaoshi.box")
talk("", "ch07.jiaoshi.herbs")
give("herb_yusui_zhi", 8, 400)
give("herb_zihou_hua", 7, 400)
give("herb_tianling_guo", 7, 300)
give("herb_yusui_zhi", 2, 1)
give("material_mojiao_cailiao", 1)
bond.add("nangong_wan", 12)
talk("", "ch07.jiaoshi.after")

flag.set("ch07.xinglai")
