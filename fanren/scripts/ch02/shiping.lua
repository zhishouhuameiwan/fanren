-- @hook ch02_yabi trigger_shiping enter once
-- 第二章节点 4「山路拾瓶」。段二（第一年·秋）的进场事件。
-- 挂在 ch02_yabi 半途的 trigger（mode=enter, once=true）。
--
-- 硬约束（docs/ch02-design.md 第 1 节第 2 条）：瓶子是他自己在山路上踢到的，
-- 不是任何人所赠。这里不许出现第二个人，也不许有任何暗示「有人放在这里」。
--
-- 二选一：捡 / 不捡。两条都会捡到——不捡的那条是走出十来步又折回来，
-- 一个从小缺东西的孩子，不会真把这么个东西留在路上。差别只在往后的口吻上。
-- 把「结果相同、态度不同」写清楚，好过给玩家一个假的岔路。
--
-- bottle.grant() 会把 lastChargeDay 拨到当日（见 P3 契约第 1 节）。
-- 这一条不能省：不拨的话拾瓶那一刻就按「已过 N 天」一次性补发绿液，
-- 原著「第八日方得一滴」当场作废。

if flag.get("ch02.shiping_done") == 1 then
    talk("", "ch02.shiping.again")
    return
end

talk("", "ch02.shiping.autumn")
talk("", "ch02.shiping.walk")
talk("", "ch02.shiping.kick")
talk("", "ch02.shiping.pain")
talk("", "ch02.shiping.find")
talk("", "ch02.shiping.look")

local pick = choice{
    "ch02.shiping.opt_take",
    "ch02.shiping.opt_leave",
}

if pick == 2 then
    -- 放回去（pick == 2）：走出十来步又折回来。捡到的时刻晚了一会儿，
    -- 心里那道坎他自己迈过去了，往后提起这瓶子的口吻也就不一样。
    flag.set("ch02.shiping_taidu", 2)
    talk("", "ch02.shiping.leave_after")
    talk("", "ch02.shiping.leave_after2")
else
    -- 当场就捡（pick == 1）与取消（pick == nil）走同一条：没做选择按顺手捡了算。
    flag.set("ch02.shiping_taidu", 1)
    talk("", "ch02.shiping.take_after")
end

talk("", "ch02.shiping.detail")
talk("", "ch02.shiping.shake")

-- 瓶子到手。这是本章两个独立解锁节点的头一个，另一个（催熟）在四年之后。
bottle.grant()

talk("", "ch02.shiping.hide")

flag.set("ch02.shiping_done")
