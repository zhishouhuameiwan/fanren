-- @hook ch02_yaopu npc_mo_daifu npc
-- 墨大夫的闲聊。挂在 ch02_yaopu 的 npc 对象上（role_id=mo_daifu）。
--
-- 硬约束（docs/ch02-design.md 第 1 节第 9 条）：他此时不得显得可疑。
-- 本章他是个尽责的师父：考他辨药、罚他抄方子、问他口诀坐到第几遍。
-- 每一句都要站得住，不许写打量、不许写意味深长的停顿、不许写笑意。
-- 玩家现在就觉得他有鬼，第 3 章的落差就没有了。
--
-- 「学医先学不害人」那一句是他的本行话，也是第 3 章回头看时最刺人的一句。
-- 它此刻不带任何暗示，全靠第 3 章自己去接。
--
-- 他的久咳承自第 1 章（原著 ch5：六十余岁、白发、久咳不止），
-- 是第 3 章的伏笔，这里只当老毛病写，不作任何交代。
--
-- 段五之后他下山采药去了，人不在谷里，这个 NPC 由关卡侧按 ch02.duan5_start
-- 撤掉；脚本这边也挡一道，免得两边有一边忘了。

if flag.get("ch02.xiangqi_ping") == 1 then
    return
end

if flag.get("ch02.liao_modaifu") == 1 then
    talk("mo_daifu", "ch02.npc.modaifu_koujue")
    talk("mo_daifu", "ch02.npc.modaifu_again")
    return
end

talk("", "ch02.npc.modaifu_1")
talk("", "ch02.npc.modaifu_2")
talk("mo_daifu", "ch02.npc.modaifu_3")

flag.set("ch02.liao_modaifu")
