-- 由 ScriptApiTests 驱动：不给年份地扣一株灵草。
--
-- 这是 zhitong.lua「交一株黄精抵人情，哪一株都算数」的写法。省略的年份由
-- api.lua 补成 0，所以它同时钉住了「y = 0 只能解释成没指定」这条语义：
-- 若哪天把 0 当成「正好 0 年那一株」，手上只剩足年药的玩家会在这里卡死。
local ok = take("herb_huangjing_cao", 1)
flag.set("take_ok", ok and 1 or 0)
