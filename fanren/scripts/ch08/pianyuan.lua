-- @hook ch05_mofu trigger_pianyuan enter once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.sunergou") == 0 or flag.get("ch08.lifu") ~= 0 then return end
talk("", "ch08.pianyuan.t1")
talk("mo_yuzhu", "ch08.pianyuan.t2")
talk("", "ch08.pianyuan.t3")
local pick = choice({"ch08.pianyuan.choice1", "ch08.pianyuan.choice2"})
if not pick then return end
flag.set("ch08.lifu", pick)
if pick == 1 then
talk("", "ch08.pianyuan.t4")
end
talk("mo_yuzhu", "ch08.pianyuan.f1")
teleport("ch08_jiayuan_shanlin", 24, 32)
if flag.get("ch08.lifu") == 0 then flag.set("ch08.lifu", 1) end
