-- @hook ch01_shenshougu trigger_ru_gu_anding enter once
-- 第一章节点 8 上半「入神手谷、安顿」。进入神手谷触发。
--
-- 七玄门山门在本章只在崖顶回望一眼：门开着，里头有人喊号子，如此而已。
-- 门主王绝楚首见于原著 ch82，本章不得出场，这里连提都不提。
-- 入谷的路照地图写：从崖底顺石阶上崖顶，翻过去就是谷南口（炼骨崖北口通这里）。
-- 从前写的是「从崖底绕过去、沿山门墙根往东」，地图上根本不经过山门，2026-09-28 改。
--
-- 张铁与韩立是被墨大夫一同收下的（原著 ch5：他一次要走两名记名弟子；
-- ch6：口诀是传给二人的；ch9：大半年后两人仍在谷中比邻而居，象甲功是
-- 后来墨大夫在谷里传给张铁的，不是入门当天的去向）。
-- 旧稿让张铁在岔路口被叫走，等于把人提前写没了——第 3 章的曲魂线
-- 要他一直在韩立近旁。这里改为同行、同住一间屋。
--
-- 结尾墨大夫打发他去谷口平石上坐一个时辰，把玩家推向打坐处的 facility；
-- 坐完由 shenshougu_dazuo.lua 接手。他为什么单打发韩立去坐，理由就在
-- 他自己方才说过的话里：抓药的那个得坐得住，采药的那个跟药童上山认路。

talk("", "ch01.shenshougu.road")
talk("", "ch01.shenshougu.gate")

talk("", "ch01.shenshougu.zt_along")

talk("", "ch01.shenshougu.valley")
talk("mo_daifu", "ch01.shenshougu.yaopu")
talk("", "ch01.shenshougu.medicine_boy")
talk("mo_daifu", "ch01.shenshougu.room")

-- 节点 4 分没分干粮，在这里最后回响一次。原先这是道别的话，
-- 现在两人同住一屋，同一个选择改的是他怎么分床——量级不变，人没走。
if flag.get("ch01.zhangtie_shared") == 1 then
    talk("zhang_tie", "ch01.shenshougu.zt_along_shared")
else
    talk("zhang_tie", "ch01.shenshougu.zt_along_plain")
end

talk("mo_daifu", "ch01.shenshougu.rule")

-- 久咳由药童一句「老毛病，谷里人都听惯了」定性：可怜，不可疑。
talk("", "ch01.shenshougu.mo_cough")

talk("mo_daifu", "ch01.shenshougu.sit_hint")
talk("", "ch01.shenshougu.sit_hint2")

flag.set("ch01.shenshougu_arrived")
