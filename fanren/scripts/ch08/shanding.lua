-- @hook ch08_jiayuan_shanlin trigger_shanding enter once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.yaolang") == 0 or flag.get("ch08.quhun") ~= 0 then return end
talk("", "ch08.shanding.t1")
talk("", "ch08.shanding.f1")
local pick = choice({"ch08.shanding.choice1", "ch08.shanding.choice2"})
if not pick then return end
if not party.add("kuilei_shou") then return end
local zuan_before = item.count("talisman_xuelingzuan")
local won = battle("b08_quhun")
if not won then
    game_over()
    return
end
if item.count("talisman_xuelingzuan") < zuan_before then
    talk("", "ch08.shanding.used")
elseif zuan_before > 0 then
    talk("", "ch08.shanding.notused")
else
    talk("", "ch08.shanding.absent")
end
talk("", "ch08.shanding.t2")
if pick == 2 then
talk("", "ch08.shanding.t3")
end
give("story_lvhuang_jian", 1)
talk("", "ch08.shanding.t4")
talk("", "ch08.shanding.t5")
talk("", "ch08.shanding.f2")
flag.set("ch08.quhun", pick)
