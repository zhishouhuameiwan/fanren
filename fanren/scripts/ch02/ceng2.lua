-- @hook ch02_jusuo trigger_ceng2 interact
-- 第二章段四（第三年）的进场事件「口诀第二层」。
-- 挂在 ch02_jusuo 打坐处旁的 trigger（mode=interact）。
--
-- 硬约束（docs/ch02-design.md 第 1 节第 5 条）：四年苦修，口诀至第三层。
-- 所以第二层落在段四、第三层落在段五，本章到此为止，不许更高。
--
-- 同样的硬约束（第 7 条）：这段口诀此时仍然没有名字。本文件与它引用的每一条
-- 文案都不得出现那三个字，师父也不主动提。第 1 章已经立下这条规矩。
--
-- 「第二层」写成什么样子要拿捏：不许有暖流、气感、灵气。
-- 能写的只有他自己身上的变化——腿麻、人清楚、院里的虫叫从一片变成一声一声。
-- 这是感官变敏锐，凡人练气也讲得通，玩家此刻不必知道更多。
--
-- 师父在这里把钱翻一倍。这条出自原著 ch14：他看准了这孩子对钱的渴望，
-- 一句话就把人绑上了苦修的车。写在本章是尽责，不是可疑——一个把家底都
-- 花在弟子身上的师父，用加月钱来催进度，句句站得住。
-- 韩立这边的算盘也就此打开：钱→药材→口诀，这条路是他自己顺出来的。

if flag.get("ch02.duan4_start") ~= 1 then
    talk("", "ch02.jusuo.zuobuchu")
    return
end

if flag.get("ch02.koujue_ceng") >= 2 then
    talk("", "ch02.jusuo.zuobuchu")
    return
end

talk("", "ch02.ceng2.night")
talk("", "ch02.ceng2.feel")

talk("mo_daifu", "ch02.ceng2.mo")
talk("mo_daifu", "ch02.ceng2.mo2")

talk("", "ch02.ceng2.money")
talk("", "ch02.ceng2.plan")
talk("", "ch02.ceng2.hint")

flag.set("ch02.koujue_ceng", 2)
realm.cap(realm.QI_REFINING_2)   -- 剧情说到第二层，上限跟着到第二层（技术债 G-14）
