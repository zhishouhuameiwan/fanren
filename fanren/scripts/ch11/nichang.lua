-- @hook ch11_dongfu trigger_nichang interact once
-- Facts and participants: docs/ch11-design.md 13.1 n08.
talk("", "ch11.nichang.read")
talk("", "ch11.nichang.herb")
talk("", "ch11.nichang.idea")
local dripped = bottle.spend(1)
if not dripped then
    talk("", "ch11.nichang.wait")
    advance_days(5)
    dripped = bottle.spend(1)
end
if dripped then
    talk("", "ch11.nichang.test")
else
    talk("", "ch11.nichang.dry")
end
talk("", "ch11.nichang.spider")
advance_days(180)
talk("", "ch11.nichang.half")
give("herb_nichang_cao", 3)
talk("", "ch11.nichang.stock")
talk("hanli", "ch11.nichang.go")
flag.set("ch11.nichang")
