-- @hook ch10_tiandujie trigger_danyaopu interact once
talk("", "ch10.danyaopu.outer")
talk("", "ch10.danyaopu.formulas")
talk("", "ch10.danyaopu.sea")
local paid = math.min(item.count("material_lingshi"), 15)
if paid > 0 and not take("material_lingshi", paid) then return end
give("story_luanxing_danfang")
talk("", "ch10.danyaopu.zhuji")
local books = math.min(item.count("material_lingshi"), 20)
if books > 0 and not take("material_lingshi", books) then return end
if paid < 15 or books < 20 then talk("", "ch10.danyaopu.short") end
give("story_dandao_pingjian")
talk("", "ch10.danyaopu.books")
flag.set("ch10.danfang")
