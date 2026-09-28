-- @hook ch02_jusuo trigger_cangping interact
-- 第二章节点 6「绿液是什么？藏起来」，兼段二的段末事件。
-- 挂在 ch02_jusuo 床边的 trigger（mode=interact）。
--
-- 二选一：拿给师父看 / 一个字也不提。
--
-- 两条分支有三处是共同的，不许因为选项不同而改变：
--   1. 瓶子始终留在韩立手里。师父这一章不能对它起任何心思。
--   2. 那一滴绿液他哪一条都没说。节点标题的「藏起来」藏的是绿液，不是瓶子。
--   3. 师父的反应必须句句站得住：一个见多识广的谷中大夫，看见弟子捡了个
--      做工不错的旧瓶子，本来就该是「收好便是，别拿去换钱惹口舌」这种反应。
--
-- 硬约束（docs/ch02-design.md 第 1 节第 9 条）：墨大夫此时仍不得显得可疑。
-- 他在本章是个尽责的师父——教医术、问功课、替弟子攒钱。玩家现在就觉得他
-- 有鬼，第 3 章的落差就没有了。所以这里不写打量、不写停顿、不写意味深长，
-- 他看完瓶子转头就问口诀坐到第几遍，从头到尾没再多看一眼。
--
-- 段末推进半年，接上段三（第二年）。

if flag.get("ch02.kaigai_done") ~= 1 then
    talk("", "ch02.cangping.gate")
    return
end

if flag.get("ch02.duan3_start") == 1 then
    talk("", "ch02.cangping.again")
    return
end

talk("", "ch02.cangping.night")
talk("mo_daifu", "ch02.cangping.mo_ask")
talk("", "ch02.cangping.think")

local pick = choice{
    "ch02.cangping.opt_tell",
    "ch02.cangping.opt_hide",
}

if pick == 1 then
    flag.set("ch02.ping_gaozhi", 1)
    talk("", "ch02.cangping.tell_1")
    talk("mo_daifu", "ch02.cangping.tell_2")
    talk("", "ch02.cangping.tell_3")
    talk("", "ch02.cangping.tell_after")
else
    -- 一个字不提（pick == 2）与取消（pick == nil）走同一条：
    -- 什么都没说出口，就是没说。
    flag.set("ch02.ping_gaozhi", 0)
    talk("mo_daifu", "ch02.cangping.hide_1")
    talk("", "ch02.cangping.hide_2")
    talk("", "ch02.cangping.hide_after")
end

talk("", "ch02.cangping.duan_end")

advance_days(180)

flag.set("ch02.duan3_start")
