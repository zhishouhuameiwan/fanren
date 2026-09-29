-- @hook ch06_tainan_gu trigger_lingshi enter once
-- 第六章节点 5a「灵石与灵符」。挂在 ch06_tainan_gu 摊位区（「回」字外圈）唯一的口子 (20,18)(20,19)
-- （mode=enter，once=true，guard_flag=ch06.ruhuo，set_flag=ch06.lingshi）。行情是看出来的——第一次走进摊位区就响。
--
-- 灵石：充盈灵气，修炼用、布阵施法用、当钱用。本作只说「灵石」不分阶，文案里没有「中阶灵石」（施工图第 7 节）。
-- 行情：符纸一打一块、下阶符一两块、中阶六到十块；飞行符三十块，或换同等价值的固元培本类丹药。
-- 原著他在这里看见一张与自己那张一模一样的金刚符——**游戏里他没有金刚符**（第 4 章战利品只有剑符与牌子），
-- 改成他数自己怀里能换的东西。叶豹出价二十被摊主顶回去，撂下「大会结束后切磋」。
-- 「秦叶岭」这个地名第 4 章没出现过（施工图 3.2 节点 5），所以原著他听见此名心里一惊那一笔不做。
-- 支线 Z2「太南谷的账」不在这里接了：改到 8b 换完金竺笔之后（q06_zhang 的接取谓词 ch06.jinzhubi）——
-- 在这里接，玩家去坊市卖养精丹，8b 就换不成（校对 HIGH-3，施工图 16.4）；ch06.zhang_qiu 随之删掉。

talk("", "ch06.lingshi.two")
talk("", "ch06.lingshi.stone1")
talk("", "ch06.lingshi.stone2")
talk("", "ch06.lingshi.poor")
talk("", "ch06.lingshi.price1")
talk("", "ch06.lingshi.price2")
talk("", "ch06.lingshi.count")
talk("", "ch06.lingshi.feixing")
talk("ye_bao", "ch06.lingshi.yebao")
talk("caomao_qingnian", "ch06.lingshi.caomao")
talk("ye_bao", "ch06.lingshi.threat")
talk("", "ch06.lingshi.pill")
talk("", "ch06.lingshi.ledger")

flag.set("ch06.lingshi")
