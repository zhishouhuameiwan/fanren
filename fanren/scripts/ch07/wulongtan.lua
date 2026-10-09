-- @hook ch07_jindi_waiwei trigger_wulongtan enter once
-- 第七章节点 21a「乌龙潭」。挂在 ch07_jindi_waiwei 潭边那道矮树丛的缺口 (38,12)(39,12)
-- （mode=enter，once=true，guard_flag=ch07.pojin，set_flag=ch07.wulongtan）。潭边的事是撞上的。
--
-- 框架件（施工图 13.1 第 21a 段）：落点在禁地外围（地貌本作自出）；资料——挪移阵随机送人，乌龙潭在东北角、潭边长寒烟草，
-- 潭里的寒冰蟾温顺不伤人；一个天阙堡弟子去采寒烟草，被灵兽山二人借寒冰蟾偷袭杀死；二人搜走东西、毁尸离去，不肯守在潭边；
-- 他敛气不动，以一敌二不去想；寒烟草一株也拿不到。三个人都只在旁白里，不建 role、不点名。
-- 幕三节奏（施工图 3.2 幕三开头）：每进一处先交代「这里刚死过人」；尸体只写姿态，不写血腥细节。

talk("", "ch07.wulongtan.first")
talk("", "ch07.wulongtan.notes")
talk("", "ch07.wulongtan.pond")
talk("", "ch07.wulongtan.man")
talk("", "ch07.wulongtan.dead")
talk("", "ch07.wulongtan.two")
talk("", "ch07.wulongtan.argue")
talk("", "ch07.wulongtan.still")
talk("", "ch07.wulongtan.empty")

flag.set("ch07.wulongtan")
