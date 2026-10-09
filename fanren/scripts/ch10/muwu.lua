-- @hook ch10_kuixing trigger_muwu interact once
talk("", "ch10.muwu.gu")
if flag.get("ch10.yuyan") >= 1 then
    talk("gu_dongzhu", "ch10.muwu.invite.clear")
else talk("gu_dongzhu", "ch10.muwu.invite.garbled") end
talk("", "ch10.muwu.hill")
talk("", "ch10.muwu.month")
talk("", "ch10.muwu.tools")
talk("", "ch10.muwu.speech")
local ok, code = realm.advance(realm.QI_REFINING_5)
if not ok then talk("", "ch10.error.realm"); return end
flag.set("ch10.yuyan", 2)
talk("", "ch10.muwu.old")
talk("gu_dongzhu", "ch10.muwu.opp")
talk("", "ch10.muwu.city")
advance_days(32)
flag.set("ch10.muwu")
teleport("ch10_kuixing", 48, 17)
