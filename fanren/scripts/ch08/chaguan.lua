-- @hook ch08_jinmacheng trigger_chaguan interact once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.nangong") == 0 or flag.get("ch08.jinma") ~= 0 then return end
local n = math.min(item.count("story_yunxiao_xinde"), 1)
if n > 0 and not take("story_yunxiao_xinde", n) then return end
local n = math.min(item.count("story_diandao_zhenqi"), 1)
if n > 0 and not take("story_diandao_zhenqi", n) then return end
give("story_diandao_zhenqi_gai", 1)
talk("qi_yunxiao", "ch08.chaguan.t1")
talk("xin_yahuan", "ch08.chaguan.t2")
talk("", "ch08.chaguan.f1")
flag.set("ch08.jinma", 1)
