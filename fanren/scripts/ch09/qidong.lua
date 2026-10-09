-- @hook ch08_lingkuang trigger_qidong interact once
-- Facts and participants: docs/ch09-design.md 13.1.
talk("", "ch09.qidong.test")
local pick
repeat pick = choice({"ch09.qidong.opt1", "ch09.qidong.opt2"}) until pick
if pick == 1 then
    local n = math.min(6, item.count("material_lingshi"))
    if n > 0 and not take("material_lingshi", n) then
        talk("", "ch09.qidong.error")
        return
    end
    if n > 0 then
        talk("", "ch09.qidong.failed")
    else
        talk("", "ch09.qidong.empty")
    end
else
    talk("", "ch09.qidong.direct")
end
local n = math.min(6, item.count("material_lingshi_zhong"))
if n > 0 and not take("material_lingshi_zhong", n) then
    talk("", "ch09.qidong.error")
    return
end
if n < 6 then
    talk("", "ch09.qidong.short")
end
talk("", "ch09.qidong.expose")
talk("", "ch09.qidong.roof")
talk("guiling_shaozhu", "ch09.qidong.recognize")
talk("", "ch09.qidong.elders")
talk("guilingmen_zhanglao", "ch09.qidong.seize")
talk("", "ch09.qidong.attack")
talk("", "ch09.qidong.token")
talk("guilingmen_zhanglao", "ch09.qidong.know")
talk("", "ch09.qidong.oneway")
n = math.min(1, item.count("story_diandao_zhenqi_gai"))
if n > 0 and not take("story_diandao_zhenqi_gai", n) then
    talk("", "ch09.qidong.error")
    return
end
flag.set("ch09.shizhen", pick)
flag.set("ch09.done")
teleport("ch08_lingkuang", 40, 33)
talk("", "ch09.qidong.light")
