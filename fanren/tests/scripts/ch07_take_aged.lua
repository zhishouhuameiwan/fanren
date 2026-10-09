-- 由 ScriptApiTests 驱动：按年份下限计数与扣物（第 7 章契约 docs/interfaces-p3-ch07.md 第 4 节）。
--
-- 先数、再扣、再数一遍，三样都记进旗标，测试对着背包核。test_count 为 0 时只数不扣。
-- 走的是真的 api.lua：item.count_aged → __host.item_count_aged，take_aged → 命令 take_item_aged。
local min_age = flag.get("test_min_age")
flag.set("aged_before", item.count_aged("herb_huangjing_cao", min_age))
local count = flag.get("test_count")
if count > 0 then
    local ok = take_aged("herb_huangjing_cao", count, min_age)
    flag.set("take_ok", ok and 1 or 0)
end
flag.set("aged_after", item.count_aged("herb_huangjing_cao", min_age))
