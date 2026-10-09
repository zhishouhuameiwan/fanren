-- @hook ch10_xiaohuan trigger_shijin interact once
talk("", "ch10.shijin.rooms")
talk("", "ch10.shijin.shell")
talk("", "ch10.shijin.book")
talk("", "ch10.shijin.rules")
local pick = choice({"ch10.shijin.opt_fire", "ch10.shijin.opt_spider"})
if not pick then return end
if pick == 1 then talk("", "ch10.shijin.fire")
else talk("", "ch10.shijin.spider") end
talk("", "ch10.shijin.catch")
give("story_shijin_chong")
talk("", "ch10.shijin.leave")
flag.set("ch10.shijin", pick)
