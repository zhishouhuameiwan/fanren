-- @hook ch06_huangfenggu trigger_laozu interact once
-- Facts and participants: docs/ch09-design.md 13.1.

talk("", "ch09.laozu.entry")
talk("", "ch09.laozu.seat")
talk("", "ch09.laozu.ask")
local pick
repeat pick = choice({"ch09.laozu.opt1", "ch09.laozu.opt2"}) until pick
if pick == 1 then
    talk("hanli", "ch09.laozu.honest")
else
    talk("hanli", "ch09.laozu.vague")
    talk("", "ch09.laozu.stop")
end
talk("zhong_lingdao", "ch09.laozu.name")
talk("linghu_laozu", "ch09.laozu.defeat")
talk("linghu_laozu", "ch09.laozu.raid")
talk("linghu_laozu", "ch09.laozu.oath")
talk("linghu_laozu", "ch09.laozu.leave")
talk("", "ch09.laozu.back")
talk("linghu_laozu", "ch09.laozu.seeds")
talk("linghu_laozu", "ch09.laozu.time")
talk("linghu_laozu", "ch09.laozu.secret")
talk("", "ch09.laozu.thought")
talk("", "ch09.laozu.watch")
flag.set("ch09.laozu", pick)
teleport("ch08_dongfu", 45, 24)
