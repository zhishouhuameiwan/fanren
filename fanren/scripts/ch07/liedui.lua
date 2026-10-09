-- @hook ch07_jindi_wai trigger_liedui interact once
-- 第七章节点 19b「荒山：七派到齐」。挂在 ch07_jindi_wai 队列前的旗杆 (26,6)
-- （mode=interact，once=true，guard_flag=ch07.xiang，set_flag=ch07.qipai）。次日上午走进队列的是他。
--
-- 框架件（施工图 13.1 第 19b 段）：次日上午等了数个时辰；清虚门乘雪虹绫到，领队浮云子（结丹）；李、浮云二人打赌
-- （先比灵药多寡、再比质量、最后比活着出来的人数；血线蛟内丹对两块铁精）；穹前辈插进来（黄枫、清虚两家合计胜过掩月宗
-- 就算他输，押三枚无形针符宝）；李师祖对弟子讲正与邪（弱肉强食；七派原是两头倒的小派、正邪两败之后联手拔掉双方所以中立；
-- 尊敬只在实力相近者之间）——**句式全换，不许出现原著那几句总结的骨架**（13 第 4 条）；许诺功劳最大的人筑基后收作弟子（他不打这个主意）；
-- 掩月宗乘天月神舟到，霓裳仙子领队，一对对年轻男女、没有老人；巨剑门、灵兽山、化刀坞、天阙堡到齐；七派每派至多二十五人；
-- 卖符少女在灵兽山队里，被络腮胡子呵斥，他冲胡子做了个鬼脸（节点 22 的由头）。
-- 卖符少女只在脚本里（太南谷那个 NPC 没有撤场旗标，施工图 16.2 第 8 条）；「穹老怪」只在旁白里。
-- 在场：他派的人从这一场起才到，所以荒山那一批只摆黄枫谷的人；飞到黄土坡之后那一批（visible ch07.qipai）就是全员。
-- advance_days(1) 在开头；末尾 teleport 黄土坡（同图，(23,25)，genmaps_ch07.py 的 JINDI_WAI_POXIA）；登 kScriptTransfers。

advance_days(1)

talk("", "ch07.liedui.morning")
talk("", "ch07.liedui.qingxu")
talk("fuyunzi", "ch07.liedui.fuyun")
talk("li_huayuan", "ch07.liedui.li_bet")
talk("fuyunzi", "ch07.liedui.fuyun_bet")
talk("", "ch07.liedui.rules")
talk("", "ch07.liedui.qiong")
talk("qiong_qianbei", "ch07.liedui.qiong_bet")
talk("", "ch07.liedui.qiong_gone")
talk("", "ch07.liedui.li_turn")
talk("li_huayuan", "ch07.liedui.li_world")
talk("li_huayuan", "ch07.liedui.li_seven")
talk("li_huayuan", "ch07.liedui.li_respect")
talk("li_huayuan", "ch07.liedui.li_promise")
talk("", "ch07.liedui.no_master")
talk("", "ch07.liedui.yanyue")
talk("", "ch07.liedui.others")
talk("", "ch07.liedui.girl")
talk("luosai_huzi", "ch07.liedui.huzi")
talk("", "ch07.liedui.face")

-- 太南谷那笔账：第 6 章 8b 以物易物补不齐时，卖符少女让他先欠着的养精丹，ch06.qianyao 记瓶数
-- （第 6 章 16.5 裁决：只记她一人的账）。这是本章她与他头一回照面。欠着才提，提了就了结——她隔着人群
-- 摆手作罢：不要药、不另开买卖、不新建旗标（协调者 2026-09-29 补派，施工图第 18 节）。== 0 一个字不提。
if flag.get("ch06.qianyao") > 0 then
    talk("", "ch07.liedui.qian")
    talk("", "ch07.liedui.qian_done")
    flag.set("ch06.qianyao", 0)
end

talk("", "ch07.liedui.fly")

flag.set("ch07.qipai")
teleport("ch07_jindi_wai", 23, 25)
