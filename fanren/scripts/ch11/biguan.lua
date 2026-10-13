-- @hook ch11_dongfu trigger_biguan interact once
-- Facts and participants: docs/ch11-design.md 13.1 n16. 本章唯一一处 realm.advance。
local dan_fu = 0
if item.count("pill_jiangchen_dan") >= 5 and take("pill_jiangchen_dan", 5) then dan_fu = dan_fu + 1 end
if item.count("material_xuelingshui") >= 2 and take("material_xuelingshui", 2) then dan_fu = dan_fu + 2 end
flag.set("ch11.dan_fu", dan_fu)
if dan_fu == 3 then
    talk("", "ch11.biguan.both")
elseif dan_fu == 0 then
    talk("", "ch11.biguan.none")
else
    talk("", "ch11.biguan.part")
end
talk("", "ch11.biguan.enter")
talk("qu_hun_shadan", "ch11.biguan.guard")
fade.out()
talk("", "ch11.biguan.ten")
advance_days(3600)
advance_days(3600)
talk("qu_hun_shadan", "ch11.biguan.worms")
advance_days(3600)
advance_days(3600)
talk("", "ch11.biguan.thirty")
advance_days(3600)
advance_days(3600)
talk("", "ch11.biguan.sky")
local ok, code = realm.advance(realm.CORE_EARLY)
if not ok then fade.in_(); talk("", "ch11.error.realm"); return end
fade.in_()
talk("", "ch11.biguan.dan")
flag.set("ch11.jiedan")
