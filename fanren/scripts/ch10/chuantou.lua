-- @hook ch10_haichuan trigger_chuantou interact once
talk("", "ch10.chuantou.crowd")
talk("huafu_zhongnian", "ch10.chuantou.greet.garbled")
talk("", "ch10.chuantou.failed")
talk("", "ch10.chuantou.name")
talk("wang_changqing", "ch10.chuantou.old")
talk("", "ch10.chuantou.land")
talk("wang_changqing", "ch10.chuantou.owner")
local pick = choice({"ch10.chuantou.opt_wait", "ch10.chuantou.opt_yes"})
if not pick then return end
if pick == 1 then
    talk("hanli", "ch10.chuantou.wait")
else
    talk("hanli", "ch10.chuantou.yes")
    talk("wang_changqing", "ch10.chuantou.translate")
end
talk("", "ch10.chuantou.cabin")
talk("hanli", "ch10.chuantou.where")
talk("wang_changqing", "ch10.chuantou.region")
talk("hanli", "ch10.chuantou.ask_islands")
talk("wang_changqing", "ch10.chuantou.islands")
talk("hanli", "ch10.chuantou.ask_rule")
talk("wang_changqing", "ch10.chuantou.lord")
talk("hanli", "ch10.chuantou.ask_cost")
talk("wang_changqing", "ch10.chuantou.cost")
talk("", "ch10.chuantou.door")
flag.set("ch10.guyu", pick)
