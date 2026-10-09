-- @hook ch10_xiaohuan trigger_zhuji interact once
if realm.level() < 9 then talk("", "ch10.zhuji.lock"); return end
local count = math.min(item.count("pill_zhuji_dan"), 3)
if count > 0 and not take("pill_zhuji_dan", count) then return end
if count == 3 then talk("", "ch10.zhuji.pills")
else talk("", "ch10.zhuji.short") end
advance_days(360)
local ok, code = realm.advance(realm.FOUNDATION_EARLY)
if not ok then talk("", "ch10.error.realm"); return end
talk("", "ch10.zhuji.back")
talk("", "ch10.zhuji.next")
flag.set("ch10.zhuji")
