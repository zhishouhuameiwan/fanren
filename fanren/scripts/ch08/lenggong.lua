-- @hook ch08_yuejing trigger_lenggong enter once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.jingong") == 0 or flag.get("ch08.huanggong") ~= 0 then return end
talk("", "ch08.lenggong.t1")
if flag.get("ch06.jujue") == 1 then
talk("hanli", "ch08.lenggong.t2")
else
talk("hanli", "ch08.lenggong.t3")
end
talk("", "ch08.lenggong.t4")
talk("", "ch08.lenggong.f1")
local lei_before = item.count("talisman_tianleizi")
local n = math.min(item.count("talisman_huoqiu_fu"), 2147483647)
if n > 0 and not take("talisman_huoqiu_fu", n) then return end
local n = math.min(item.count("talisman_tianleizi"), 1)
if n > 0 and not take("talisman_tianleizi", n) then return end
if party.has("kuilei_shou") and not party.remove("kuilei_shou") then return end
if item.count("talisman_tianleizi") < lei_before then
    talk("", "ch08.lenggong.withlei")
else
    talk("", "ch08.lenggong.nolei")
end
talk("", "ch08.lenggong.f2")
teleport("ch08_yuejing", 19, 12)
flag.set("ch08.huanggong", 1)
