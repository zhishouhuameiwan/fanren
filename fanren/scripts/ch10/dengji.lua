-- @hook ch10_kuixing trigger_dengji interact once
talk("", "ch10.dengji.room")
local understood = flag.get("ch10.yuyan") >= 1
if understood then talk("gangkou_xiushi", "ch10.dengji.ask.clear")
else
    talk("gangkou_xiushi", "ch10.dengji.ask.garbled")
    talk("wang_changqing", "ch10.dengji.prompt")
end
talk("hanli", "ch10.dengji.name")
give("story_lvse_yupai")
talk("", "ch10.dengji.card")
if understood then
    talk("gangkou_xiushi", "ch10.dengji.rule.clear")
    talk("gangkou_xiushi", "ch10.dengji.shell.clear")
else
    talk("gangkou_xiushi", "ch10.dengji.rule.garbled")
    talk("gangkou_xiushi", "ch10.dengji.shell.garbled")
end
talk("", "ch10.dengji.ride")
flag.set("ch10.dengji")
teleport("ch10_kuixing", 47, 28)
