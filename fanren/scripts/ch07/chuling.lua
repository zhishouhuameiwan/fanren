-- @hook ch06_huangfenggu trigger_chuling interact once
-- 第七章节点 10「外出令牌」。挂在 ch06_huangfenggu 百机堂柜台、于执事面前那一格 (7,12)（与第 6 章 trigger_zawu 不同格）
-- （mode=interact，once=true，guard_flag=ch07.qiannian，set_flag=ch07.chuling）。向于执事领令牌的是他。
--
-- 框架件（施工图 13.1 第 10 段）：外出令牌每年一次，少有人申请。置 ch07.chuling 即黄枫谷南缘通坊市的门开。
-- 令牌不做物品（施工图没给），只在旁白里。

talk("", "ch07.chuling.ask")
talk("yu_zhishi", "ch07.chuling.yu")
talk("", "ch07.chuling.token")
talk("", "ch07.chuling.where")

flag.set("ch07.chuling")
