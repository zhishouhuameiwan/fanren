-- @hook ch02_jusuo trigger_ceng3 interact
-- 第二章段五（第四年）的进场事件「口诀第三层，重新想起那只瓶子」。
-- 挂在 ch02_jusuo 床边的 trigger（mode=interact）。
--
-- 这一段要办三件事，缺一件段五就立不住：
--   1. 口诀到第三层，并且卡死在这里（硬约束第 5 条，四年苦修的上限）。
--   2. 师父下山采药去（出处 ch15）。他走后谷里只剩韩立一个人——
--      后面试药、炸兔子、满地血肉，都得没有第二双眼睛才办得成。
--   3. 让他重新想起那只瓶子。
--
-- 第 3 件的由头照原著 ch23：他心烦，去摸皮袋里那枚娘给的平安符，
-- 指头在袋底碰到个硬东西。原著这一段的前因是修道人的心魔入侵，
-- 本作不用——本章的七玄门是凡人武馆，走火入魔、心魔一类的说法一个都不能有。
-- 换成一个十四岁少年练不上去、半夜烦得在院里转圈，落点一样，口径干净。
--
-- 师父下山这一笔仍要写成尽责：他是替弟子去寻药的，谷里的存货让这孩子吃光了。
-- 硬约束第 9 条——本章他不得显得可疑。

if flag.get("ch02.duan5_start") ~= 1 then
    talk("", "ch02.jusuo.zuobuchu")
    return
end

if flag.get("ch02.xiangqi_ping") == 1 then
    talk("", "ch02.jusuo.zuobuchu")
    return
end

talk("", "ch02.ceng3.year4")
talk("", "ch02.ceng3.break")

talk("mo_daifu", "ch02.ceng3.mo")
talk("mo_daifu", "ch02.ceng3.mo_go")

flag.set("ch02.koujue_ceng", 3)
realm.cap(realm.QI_REFINING_3)   -- 四年苦修的终点：上限封在第三层，第 4 章节点 1 才再抬（技术债 G-14）

talk("", "ch02.ceng3.stuck")
talk("", "ch02.ceng3.restless")
talk("", "ch02.ceng3.remember")
talk("", "ch02.ceng3.open")
talk("", "ch02.ceng3.decide")
talk("", "ch02.ceng3.plan")

flag.set("ch02.xiangqi_ping")
