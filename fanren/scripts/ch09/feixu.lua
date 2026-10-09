-- @hook ch09_yuanwu trigger_feixu interact once
-- Facts and participants: docs/ch09-design.md 13.1.

talk("", "ch09.feixu.ruin")
talk("", "ch09.feixu.servant")
talk("", "ch09.feixu.arrive")
local pick
repeat pick = choice({"ch09.feixu.opt1", "ch09.feixu.opt2"}) until pick
if pick == 1 then
    talk("hanli", "ch09.feixu.ask")
    talk("fujia_qingnian", "ch09.feixu.answer")
else
    talk("fujia_qingnian", "ch09.feixu.claim")
end
talk("", "ch09.feixu.memory")
local won = battle("b09_fujia")
if not won then game_over() return end
give("story_tongqian", 1)
give("material_lingshi", 60)
talk("", "ch09.feixu.spoils")
talk("", "ch09.feixu.burn")
talk("", "ch09.feixu.message")
talk("xin_ruyin", "ch09.feixu.challenge")
talk("hanli", "ch09.feixu.explain")
talk("", "ch09.feixu.admit")
flag.set("ch09.fujia", pick)
teleport("ch09_wumingshan", 7, 22)
