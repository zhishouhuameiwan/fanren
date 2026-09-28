-- 第一条语句就出错：连一次挂起都没发生过，失败必须由 startEvent 报出来。
local t = nil
t.field = 1
talk("a", "t.never")
