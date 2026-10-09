-- @hook ch07_dixia_zhaoze trigger_jiaoshi interact once
-- 第七章节点 31「醒来；金箱；灵药；破土；南宫婉」。挂在 ch07_dixia_zhaoze 西岸蛟尸旁那一格 (9,15)
-- （mode=interact，once=true，guard_flag=ch07.mojiao，set_flag=ch07.nangong）。拿银剑剖蛟尸的是他。
--
-- 框架件（施工图 13.1 第 31 段）：他用赤脚大汉的银剑剖蛟尸，她说这剑里掺了银精、外人驱使不了；他从蛟腹里掏出一团东西，
-- 她一碰炸开粉红烟雾——**暗场**（16.1 第 3 条、13 第 8 条）：只一行「两个人都失去了知觉」；醒来她的外貌长大了好几岁；
-- 她冷冷交代就当没发生过、传出去就杀他（**说法自拟**），他应下；她修素女轮回功，损了几年法力（只说「损」）；
-- 那团东西只叫「墨蛟的一个毒囊」（那个词 0 处，12.2）；金箱归她；灵药她不要，他全收了；破禁要两人合力：她把被禁的法力暂传给他
-- （他一时有了十三层的法力，面板不动），朱雀环与金光砖轮番打穿地面；**金光砖耗尽成了废纸**：take("talisman_jinguangzhuan")
-- ＋ magic.forget("magic_ji_jinguangzhuan")（验收 7：恰各一处、同在本文件）；她损了二三十年功力；分别时他问她的名字：南宫婉
-- （「南宫」从 ch07.jiaoshi.name* 起才许出现，12.2）；他清楚两人不会再有交集。
-- give：成熟的玉髓芝 8（400 年）、紫猴花 7（400 年）、天灵果 7（300 年）——二十二株，节点 32b 上交用；玉髓芝幼苗 2；墨蛟材料 1。
-- 末尾 teleport ch07_huanxingshan 地面破洞旁 (38,35)（genmaps_ch07.py 的 HXS_POKONG）；登 kScriptTransfers。

talk("", "ch07.jiaoshi.sword")
talk("baiyi_shaonv", "ch07.jiaoshi.silver")
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
talk("baiyi_shaonv", "ch07.jiaoshi.two")
talk("", "ch07.jiaoshi.lend")
talk("", "ch07.jiaoshi.dig")
talk("", "ch07.jiaoshi.brick_gone")
take("talisman_jinguangzhuan", 1)
magic.forget("magic_ji_jinguangzhuan")
talk("", "ch07.jiaoshi.cost")
talk("", "ch07.jiaoshi.name_ask")
talk("nangong_wan", "ch07.jiaoshi.name_say")
talk("", "ch07.jiaoshi.name_after")

flag.set("ch07.nangong")
teleport("ch07_huanxingshan", 38, 35)
