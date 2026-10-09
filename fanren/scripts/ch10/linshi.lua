-- @hook ch10_haiyuandao trigger_linshi interact once
talk("", "ch10.linshi.island")
talk("", "ch10.linshi.cave")
talk("", "ch10.linshi.guess")
talk("", "ch10.linshi.rules")
talk("", "ch10.linshi.time")
talk("", "ch10.linshi.try")
talk("", "ch10.linshi.avatar")
talk("", "ch10.linshi.three")
local pick = choice({"ch10.linshi.opt_stop", "ch10.linshi.opt_wait"})
if not pick then return end
local liquid = math.min(item.count("material_tianhuoye"), 2)
if liquid > 0 and not take("material_tianhuoye", liquid) then return end
local pills = math.min(item.count("pill_xuening_wuxing_dan"), 5)
if pills > 0 and not take("pill_xuening_wuxing_dan", pills) then return end
advance_days(2)
advance_days(1080)
if pick == 1 then
    talk("", "ch10.linshi.failed")
    talk("", "ch10.linshi.stop")
else
    talk("", "ch10.linshi.continue")
end
advance_days(360)
if pick == 2 then
    talk("", "ch10.linshi.wait")
    talk("", "ch10.linshi.failed")
end
local removed = party.remove("qu_hun_huashen")
if not removed then talk("", "ch10.error.party"); return end
local added = party.add("qu_hun_shadan")
if not added then talk("", "ch10.error.party"); return end
talk("", "ch10.linshi.success")
talk("", "ch10.linshi.sky")
talk("qu_hun_shadan", "ch10.linshi.warn")
talk("", "ch10.linshi.gone")
local bowl = item.count("story_hunyuan_bo") > 0
if bowl and not take("story_hunyuan_bo", 1) then return end
local sword = item.count("talisman_jinjian_fubao") > 0
if sword and not take("talisman_jinjian_fubao", 1) then return end
local skull = item.count("story_jin_kuloutou") > 0
if skull and not take("story_jin_kuloutou", 1) then return end
talk("", "ch10.linshi.give")
talk("", "ch10.linshi.return")
advance_days(40)
flag.set("ch10.shadan", pick)
teleport("ch10_xiaohuan", 49, 36)
talk("", "ch10.linshi.quiet")
