-- @hook ch10_haichuan trigger_chuanting interact once
talk("", "ch10.chuanting.luo")
talk("qu_hun_huashen", "ch10.chuanting.qu")
talk("feng_sanniang", "ch10.chuanting.feng")
talk("feng_sanniang", "ch10.chuanting.mao")
talk("feng_sanniang", "ch10.chuanting.yan")
talk("mao_daoyou", "ch10.chuanting.challenge")
local pick = choice({"ch10.chuanting.opt_reply", "ch10.chuanting.opt_ignore"})
if not pick then return end
if pick == 1 then talk("qu_hun_huashen", "ch10.chuanting.reply")
else talk("", "ch10.chuanting.ignore") end
talk("", "ch10.chuanting.practice")
talk("", "ch10.chuanting.island")
advance_days(33)
flag.set("ch10.chuhai", pick)
teleport("ch10_jinhai", 38, 28)
