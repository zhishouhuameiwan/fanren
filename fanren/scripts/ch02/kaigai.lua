-- @hook ch02_jusuo trigger_kaigai interact
-- 第二章节点 5 下半「第八日自开」。
-- 挂在 ch02_jusuo 院中空地的 trigger（mode=interact）。
--
-- 硬约束（docs/ch02-design.md 第 1 节第 3 条，出处 ch14）：
-- 得瓶后第八日瓶盖才开，内有一滴绿液。前七天只有夜里的异象，别的什么也没有。
--
-- kBaseChargeDays 取 7，且 bottle.grant() 已把 lastChargeDay 拨到拾瓶当日，
-- 所以「拾瓶当日起算、第八天得第一滴」是引擎算出来的，不是这里写死的。
-- 下面这段补时间只在瓶里还没凝出那一滴时才做：玩家自己在外头晃了七天回来，
-- 绿液早就有了，这时候再推七天等于白白吃掉他一周的灵田生长。
--
-- 瓶身上浮起的白点子只写它冷、写它密，不解释它是什么。
-- 本章的七玄门是凡人武馆，灵气、气感一类的词一个都不许出现——
-- 玩家此刻和韩立一样，只知道这东西怪，不知道它怪在哪里。

if flag.get("ch02.zaping_done") ~= 1 then
    talk("", "ch02.kaigai.gate")
    return
end

if flag.get("ch02.kaigai_done") == 1 then
    talk("", "ch02.kaigai.again")
    return
end

talk("", "ch02.kaigai.days")
talk("", "ch02.kaigai.watch")

-- 还没到第八日就把时间补齐。已经够了就一天也不推。
if bottle.drops() < 1 then
    talk("", "ch02.kaigai.wait7")
    advance_days(7)
end

talk("", "ch02.kaigai.eighth")
talk("", "ch02.kaigai.mark")
talk("", "ch02.kaigai.open")
talk("", "ch02.kaigai.stare")
talk("", "ch02.kaigai.inside")
talk("", "ch02.kaigai.letdown")
talk("", "ch02.kaigai.cap")

flag.set("ch02.kaigai_done")
