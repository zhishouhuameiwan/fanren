-- @hook ch08_tianxing_fangshi trigger_xingchenge interact once
-- Facts and participants: docs/ch09-design.md 13.1.

talk("", "ch09.xingchenge.floors")
talk("", "ch09.xingchenge.permission")
talk("hanli", "ch09.xingchenge.want")
talk("lan_furen", "ch09.xingchenge.new")
talk("", "ch09.xingchenge.wealth")
talk("", "ch09.xingchenge.mortal")
talk("xiu_er", "ch09.xingchenge.recipes")
talk("", "ch09.xingchenge.known")
talk("xiu_er", "ch09.xingchenge.needles")
talk("lan_furen", "ch09.xingchenge.control")
talk("", "ch09.xingchenge.trial")
talk("xiu_er", "ch09.xingchenge.price")
talk("hanli", "ch09.xingchenge.herb")
talk("", "ch09.xingchenge.pills")
talk("lan_furen", "ch09.xingchenge.deal")
local d = item.count("pill_dingyan_dan")
local s = item.count("material_lingshi")
local stones = math.min(300, s)
local pills = math.min((s >= 300 and 2 or 3), d)
local result = s >= 300 and d >= 2 and 1 or 2
if d < 2 then stones = s end
if pills > 0 and not take("pill_dingyan_dan", pills) then
    talk("", "ch09.xingchenge.error")
    return
end
if stones > 0 and not take("material_lingshi", stones) then
    talk("", "ch09.xingchenge.error")
    return
end
if d < 2 then
    talk("xiu_er", "ch09.xingchenge.few")
elseif s < 300 then
    talk("xiu_er", "ch09.xingchenge.short")
else
    talk("", "ch09.xingchenge.paid")
end
give("story_zhenyuan_danfang", 1)
give("story_hongxian_zhen", 1)
local ok = magic.learn("magic_ji_hongxianzhen")
if not ok then
    talk("", "ch09.xingchenge.learnerror")
    return
end
talk("lan_furen", "ch09.xingchenge.source")
talk("hanli", "ch09.xingchenge.chance")
talk("", "ch09.xingchenge.conceal")
advance_days(1)
flag.set("ch09.xingchen", result)
