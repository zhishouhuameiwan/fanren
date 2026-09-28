-- @hook ch02_wairentang trigger_renqing interact
-- 第二章节点 9「同伴还人情」，兼段四的段末事件。
-- 挂在 ch02_wairentang 台阶处的 trigger（mode=interact）。
--
-- 厉飞雨开口先说那副药这一年用得怎么样——三种配法各有各的说法，
-- 都是好话，但好在不同的地方。这就是节点 8 那次三选一的全部回响：
-- 不改走向，只改他这一年是怎么过来的。
--
-- 二选一：收下那箱药材 / 不收。
-- 不收的那一条不许把箱子写没：厉飞雨把它搁在自己屋里，说什么时候要什么时候
-- 来拿。一个「讲义气、吃了亏也不记仇」的人不会因为被推回来就翻脸，
-- 而玩家往后也不该因为一次客气就永久少掉一箱药材。
--
-- 硬约束（docs/ch02-design.md 第 1 节第 6 条）：没有结义。
-- 两个人在台阶上并排坐了一会儿，说了几句话，就各走各的——分寸到此为止。
--
-- 他那句「这条命还剩几年，自己心里有数」是他知道抽髓丸的账怎么算，
-- 不是预告他的结局。第 4 章他还活着，还要接外刃堂堂主，本章不得透出别的。

if flag.get("ch02.duan4_start") ~= 1 then
    talk("", "ch02.renqing.gate")
    return
end

if flag.get("ch02.duan5_start") == 1 then
    talk("", "ch02.renqing.again")
    return
end

-- 口诀要一层一层上去，不能从头一层直接跳到第三层。
--
-- 这道闸是补的：ceng2（口诀第二层）挂在段四的居所，而本节点是段四通向段五的
-- 出口，从前谁也没要求先过 ceng2。于是玩家整个段四只跑外刃堂、不回屋里坐，
-- 就能把那一节整段错过，koujue_ceng 从 1 直接跳到 3——「四年苦修一层一层上去」
-- 这条线正是本章的骨架，断一节玩家就再也接不上了。
--
-- 闸在这里而不是在 ceng3 上：ceng2 整个段四都在，随时回屋就能补；
-- 挡在 ceng3 上则要等玩家已经进了段五才发现自己缺了一节，那时居所那处
-- 触发的段四前置已经不成立，会把人卡死。
if flag.get("ch02.koujue_ceng") < 2 then
    talk("", "ch02.renqing.gate_koujue")
    return
end

talk("", "ch02.renqing.arrive")
talk("li_feiyu", "ch02.renqing.lfy_see")

if flag.get("ch02.zhitong_fangzi") == 1 then
    talk("li_feiyu", "ch02.renqing.lfy_meng")
elseif flag.get("ch02.zhitong_fangzi") == 3 then
    talk("li_feiyu", "ch02.renqing.lfy_bu")
else
    talk("li_feiyu", "ch02.renqing.lfy_wen")
end

talk("", "ch02.renqing.lfy_bring")
talk("li_feiyu", "ch02.renqing.lfy_say")

local pick = choice{
    "ch02.renqing.opt_take",
    "ch02.renqing.opt_no",
}

if pick == 2 then
    -- 不收（pick == 2）。箱子寄存在厉飞雨那里，没有消失。
    flag.set("ch02.renqing_huan", 0)
    talk("", "ch02.renqing.no_1")
    talk("li_feiyu", "ch02.renqing.no_2")
else
    -- 收下（pick == 1）与取消（pick == nil）走同一条：
    -- 眼下最缺的就是药材，不接才是反常。
    flag.set("ch02.renqing_huan", 1)
    talk("", "ch02.renqing.take_1")
    talk("li_feiyu", "ch02.renqing.take_2")
    -- 一箱够他撑一阵的药材。段五的试药与催熟都要药材垫底。
    give("herb_huangjing_cao", 6)
    give("herb_zishen_cao", 2)
end

talk("li_feiyu", "ch02.renqing.lfy_life")
talk("li_feiyu", "ch02.renqing.lfy_life2")
talk("", "ch02.renqing.hanli")
talk("", "ch02.renqing.duan_end")

advance_days(365)

flag.set("ch02.duan5_start")
