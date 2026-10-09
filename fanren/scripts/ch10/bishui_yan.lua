-- @hook ch10_jinhai trigger_bishui_yan interact once
if flag.get("ch10.bishui") ~= 15 then talk("", "ch10.bishui.missing"); return end
talk("", "ch10.bishui.done")
advance_days(1)
flag.set("ch10.bishui_cheng")
