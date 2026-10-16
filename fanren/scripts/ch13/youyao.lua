-- @hook ch13_wuzhong trigger_youyao interact
-- Facts and participants: docs/ch13-design.md 13.1 段 24。3.2 n24 / 7.1「诱妖」行，照第 11 章口径（once 否、无 set_flag）：
-- ch13.biguan 已置 → 一句 return；
-- 缺草：灵石 >= 10 → 一句（指寸金阁）return；灵石 < 10 → 一句、advance_days(30)（3.3）、give 霓裳草 1（兜底）return；
-- 缺绿液 → 一句、advance_days(5) return；否则 bottle.spend(1)（看返回值）、advance_days(3)、battle("b13_waixinghai_lieyao")。
-- 胜：take 草 1（false 换一句照记）、ch13.lieyao ＋1；逃：不扣草（液已扣）；败 game_over。妖丹由编成 drops 给。
if flag.get("ch13.biguan") ~= 0 then talk("", "ch13.youyao.closed"); return end
if item.count("herb_nichang_cao") < 1 then
    if item.count("material_lingshi") >= 10 then talk("", "ch13.youyao.no_herb"); return end
    advance_days(30)
    give("herb_nichang_cao", 1)
    talk("", "ch13.youyao.grow")
    return
end
if bottle.drops() < 1 then talk("", "ch13.youyao.no_drop"); advance_days(5); return end
if not bottle.spend(1) then talk("", "ch13.youyao.no_drop"); advance_days(5); return end
advance_days(3)
talk("", "ch13.youyao.drip")
local won, how = battle("b13_waixinghai_lieyao")
if not won and how == "escaped" then talk("", "ch13.youyao.escaped"); return end
if not won then game_over(); return end
if take("herb_nichang_cao", 1) then
    talk("", "ch13.youyao.win")
else
    talk("", "ch13.youyao.herb_gone")
end
flag.set("ch13.lieyao", flag.get("ch13.lieyao") + 1)
