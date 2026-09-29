-- @hook ch06_huangfenggu trigger_lin_lingqu interact once
-- 第六章节点 16a「领装备」。挂在 ch06_huangfenggu 林师弟屋前的案 (38,28)(39,28)
-- （mode=interact，once=true，guard_flag=ch06.rumen，set_flag=ch06.chuwudai）。
--
-- 石屋群空着（十年一轮换，新弟子还没进门）；林师弟屋里乱、雕木猴；「资质也太差了点」「掌门眼界这么低」——王师叔岔开；
-- 「升仙令！」；听说他放弃了筑基丹，神色一黯：做人要有自知之明（他自己的往事，不解释）。
-- 虚空抓物：黄丝衫（不做物品，旁白）、青叶法器、日常精炼工具一套（旁白）、烈阳剑、冷月刀、十倍储物袋。
-- 储物袋三忌照原著三条内容，句式全换（施工图第 13 节第 4 条）。背包标题从 ch06.chuwudai 置起叫「储物袋」（引擎路 O1）。
-- 他第一次用储物袋、第一次踩青叶法器飞起来（比骏马快那么一点）——旁白。

talk("", "ch06.lingqu.empty")
talk("", "ch06.lingqu.room")
talk("lin_shidi", "ch06.lingqu.poor")
talk("wang_shishu", "ch06.lingqu.token")
talk("lin_shidi", "ch06.lingqu.surprise")
talk("wang_shishu", "ch06.lingqu.gaveup")
talk("lin_shidi", "ch06.lingqu.wise")
talk("", "ch06.lingqu.grab")
give("story_qingye_faqi", 1)
give("weapon_lieyang_jian", 1)
give("weapon_lengyue_dao", 1)
give("story_chuwudai", 1)
talk("lin_shidi", "ch06.lingqu.list")
talk("", "ch06.lingqu.door")
talk("wang_shishu", "ch06.lingqu.bag")
talk("wang_shishu", "ch06.lingqu.taboo1")
talk("wang_shishu", "ch06.lingqu.taboo2")
talk("wang_shishu", "ch06.lingqu.taboo3")
talk("", "ch06.lingqu.fly")

flag.set("ch06.chuwudai")
