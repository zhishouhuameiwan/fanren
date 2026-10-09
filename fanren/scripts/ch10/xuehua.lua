-- @hook ch10_haichuan npc_wang_changqing npc
if flag.get("ch10.guyu") == 0 or flag.get("ch10.kaoan") ~= 0 then return end
if flag.get("ch10.yetan") == 0 then
    talk("wang_changqing", "ch10.xuehua.early")
    return
end
local mask = flag.get("ch10.xuehua")
local bit, lesson
if (mask & 1) == 0 then bit, lesson = 1, 1
elseif (mask & 2) == 0 then bit, lesson = 2, 2
elseif (mask & 4) == 0 then bit, lesson = 4, 3
else
    talk("wang_changqing", "ch10.xuehua.done")
    return
end
local lessons = {
    {"ch10.xuehua.l1a", "ch10.xuehua.l1b", "ch10.xuehua.l1c"},
    {"ch10.xuehua.l2a", "ch10.xuehua.l2b", "ch10.xuehua.l2c"},
    {"ch10.xuehua.l3a", "ch10.xuehua.l3b", "ch10.xuehua.l3c"},
}
talk("wang_changqing", lessons[lesson][1])
talk("", lessons[lesson][2])
talk("", lessons[lesson][3])
advance_days(1)
mask = mask | bit
flag.set("ch10.xuehua", mask)
if mask == 7 then flag.set("ch10.yuyan", 1) end
