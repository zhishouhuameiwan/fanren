-- @hook ch10_xiaohuan trigger_sanzhuan interact once
if item.count("pill_zhenyuan_dan") < 2 then
    talk("", "ch10.sanzhuan.lock")
    if field.planted("field_xiaohuan") == 0 and item.count("herb_zishen_cao") == 0 and item.count("herb_xuehong_zhi") == 0 then
        talk("", "ch10.sanzhuan.seeds")
        give("herb_zishen_cao", 1, 1)
        give("herb_xuehong_zhi", 1, 1)
    end
    return
end
if not take("pill_zhenyuan_dan", 2) then return end
talk("", "ch10.sanzhuan.method")
talk("", "ch10.sanzhuan.fall")
talk("", "ch10.sanzhuan.vow")
talk("", "ch10.sanzhuan.avatar")
talk("", "ch10.sanzhuan.limit")
local d = 7560 - (today() - flag.get("ch10.kaifu_ri"))
advance_days(math.max(360, d))
if today() - flag.get("ch10.kaifu_ri") > 8640 then talk("", "ch10.sanzhuan.long")
else talk("", "ch10.sanzhuan.years") end
local paid = math.min(item.count("material_lingshi"), 120)
if paid > 0 and not take("material_lingshi", paid) then return end
if paid == 120 then talk("", "ch10.sanzhuan.paid")
else talk("", "ch10.sanzhuan.short") end
talk("", "ch10.sanzhuan.hei")
local ok, code = realm.advance(realm.FOUNDATION_LATE)
if not ok then talk("", "ch10.error.realm"); return end
local added = party.add("qu_hun_huashen")
if not added then talk("", "ch10.error.party"); return end
talk("", "ch10.sanzhuan.ready")
talk("", "ch10.sanzhuan.words")
flag.set("ch10.shizi")
flag.set("ch10.chuguan")
