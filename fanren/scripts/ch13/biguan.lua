-- @hook ch13_wuzhong trigger_jingshimen interact once
-- Facts and participants: docs/ch13-design.md 13.1 段 27。3.2 n27：choice「就此闭关 / 再等等」（确认，不计入主动抉择，16.1 第 52 条），
-- 取消与「再等等」都在拨日、置旗前 return；advance_days(30)（3.3 歇一月）；local ok = realm.cap(realm.CORE_MID)（false 换一句照走）；
-- 置 ch13.biguan。确认文案写明往返、诱妖、Z1 / Z2 自此收口，不点名任何按键；提示静室在旁。
-- 境界只经这一处上限（纪律 69）：realm.advance / realm.demote 0 处，不从脚本发修为。
talk("", "ch13.biguan.door")
local pick = choice({"ch13.biguan.opt_go", "ch13.biguan.opt_wait"})
if not pick then return end
if pick ~= 1 then talk("", "ch13.biguan.wait"); return end
advance_days(30)
talk("", "ch13.biguan.rest")
talk("", "ch13.biguan.feed")
local ok = realm.cap(realm.CORE_MID)
if ok then
    talk("", "ch13.biguan.pill")
else
    talk("", "ch13.biguan.pill_fail")
end
talk("", "ch13.biguan.room")
flag.set("ch13.biguan")
