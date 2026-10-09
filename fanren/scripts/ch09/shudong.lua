-- @hook ch09_milin trigger_shudong interact once
-- Facts and participants: docs/ch09-design.md 13.1.
talk("", "ch09.shudong.pulse")
talk("", "ch09.shudong.drain")
local ok, code = realm.demote(realm.QI_REFINING_3)
if not ok then
    talk("", "ch09.shudong.error")
    return
end
talk("", "ch09.shudong.faint")
fade.out()
advance_days(1)
fade.in_()
talk("", "ch09.shudong.wake")
talk("", "ch09.shudong.wrong")
talk("mengmian_nvzi", "ch09.shudong.know")
talk("", "ch09.shudong.calm")
talk("mengmian_nvzi", "ch09.shudong.name")
talk("nangong_ping", "ch09.shudong.debt")
talk("nangong_ping", "ch09.shudong.offer")
talk("nangong_ping", "ch09.shudong.recover")
talk("nangong_ping", "ch09.shudong.secret")
talk("", "ch09.shudong.doubt")
talk("hanli", "ch09.shudong.refuse")
talk("", "ch09.shudong.bag")
give("material_lingshi_zhong", 30)
local pick
repeat pick = choice({"ch09.shudong.opt1", "ch09.shudong.opt2"}) until pick
if pick == 1 then
    talk("hanli", "ch09.shudong.ask")
    give("material_heyuanyu", 3)
else
    talk("", "ch09.shudong.self")
end
flag.set("ch09.heyuan", pick)
talk("nangong_ping", "ch09.shudong.judge")
talk("hanli", "ch09.shudong.greeting")
talk("", "ch09.shudong.leave")
flag.set("ch09.dieluo")
