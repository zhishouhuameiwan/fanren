-- @hook ch07_dixia_zhaoze trigger_jiaoshi interact once
-- 第七章节点 31 前半「醒来；金箱；灵药」，伙伴改版样板 4.2 末与 4.3 开头「困在地下」。挂在 ch07_dixia_zhaoze 西岸蛟尸旁那一格 (9,15)
-- （mode=interact，once=true，guard_flag=ch07.mojiao，set_flag=ch07.kundi）。拿银剑剖蛟尸的是他。
--
-- 框架件（施工图 13.1 第 31 段前半）：他剖开蛟尸，从蛟腹里掏出一团东西，她一碰炸开粉红烟雾——**暗场**（16.1 第 3 条、13 第 8 条）：
-- 只一行「两个人都失去了知觉」；醒来她的外貌长大了好几岁；她冷冷交代就当没发生过、传出去就杀他（说法自拟），他应下；
-- 她修素女轮回功，损了几年法力（只说「损」）；那团东西只叫「墨蛟的一个毒囊」（那个词 0 处，12.2）；金箱归她；灵药她不要，他全收了。
-- give：成熟的玉髓芝 8（400 年）、紫猴花 7（400 年）、天灵果 7（300 年）——二十二株，节点 32b 上交用；玉髓芝幼苗 2；墨蛟材料 1。
--
-- 伙伴改版样板（docs/ch07-companions-design.md 4.3、4.6）：
--   · 毒囊之后（暗场）默契 +12；接着 4.3 开头：回头的路封死了（ch206 小五行须弥禁法），她评他的银剑（ch207，约束 6）+8——
--     演完是 22，到二档「相熟」，追击从这一路上就用得上。
--   · 两人商定出去要做的两件事：替她找火灵石回气（ch206 她握着火灵石回法力）、找岩层最薄的地方（ch209 打穿地面）。
--     这段「在地下找路」是改编：起点（通道被封）、终点（合力打穿）都是原著的（施工图第 4 节）。
--   · 破禁那几句（两人合力、渡法力、打穿、金光砖成废纸、折损功力）挪到 poding.lua；问名挪到 wenming.lua。
-- 置 ch07.kundi：地下沼泽通沼泽深处的两道门从此开。

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

-- ---- 4.3 困在地下 ----
talk("", "ch07.kundi.passage")
talk("baiyi_shaonv", "ch07.kundi.no_way")
talk("baiyi_shaonv", "ch07.kundi.see_sword")
talk("", "ch07.kundi.hand_over")
talk("baiyi_shaonv", "ch07.jiaoshi.silver")
talk("", "ch07.kundi.take_back")
bond.add("nangong_wan", 8)
talk("baiyi_shaonv", "ch07.kundi.plan")
talk("baiyi_shaonv", "ch07.kundi.stone")
talk("", "ch07.kundi.ok")
talk("", "ch07.kundi.follow")

flag.set("ch07.kundi")
