-- @hook ch05_nancheng trigger_jiulou interact once
-- 第五章节点 5「香家酒楼：蓝衣人与墨玉珠」。挂在 ch05_nancheng 香家酒楼的门 (21,21)(22,21)
-- （mode=interact，once=true，guard_flag=ch05.jieren，set_flag=ch05.jiulou）。
-- 他自己上楼挑了临街的座（ch106）。无选择。本章压迫感的顶点之一。
--
-- 蓝衣人（ch106）：邻桌坐下，看了他一眼，他觉得里外都被看穿；对方身上那股灵力不用看也压得住人；
-- 他不敢回看；那人吃完就走，**一个字也不跟他说**（所以本节没有一句 talk 的说话人是 lanyi_ren）。
-- 他怎么「看穿」的，台词一个字不点——那一门本章不提（施工图 1.1 第 14 条）。
-- 蓝衣人与黄衫人那段对话韩立没听见（ch107 首句），游戏不演：那段话里有两个本章不许出现的词。
--
-- 墨玉珠与吴剑鸣打猎回来（ch107）：原著那一段的几件细节（火红猎装、耳语、脸红、捶肩、跑进门）
-- 一件不照搬，换成背弓、扔缰绳、冲看热闹的人拱手——吴剑鸣的虚荣要从他自己的动作里出来。

talk("", "ch05.jiulou.street")
talk("", "ch05.jiulou.up")
talk("", "ch05.jiulou.hear")
talk("", "ch05.jiulou.blue1")
talk("", "ch05.jiulou.blue2")
talk("", "ch05.jiulou.blue3")
talk("", "ch05.jiulou.blue4")
talk("", "ch05.jiulou.blue5")
talk("", "ch05.jiulou.ride")
talk("", "ch05.jiulou.ride2")
talk("", "ch05.jiulou.decide")

flag.set("ch05.jiulou")
