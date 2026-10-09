-- @hook ch08_jinguyuan trigger_daoying enter once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.zhaoji") == 0 or flag.get("ch08.yinian") ~= 0 then return end
advance_days(360)
give("material_lingshi_zhong", 1)
talk("", "ch08.daoying.t1")
talk("", "ch08.daoying.t2")
talk("", "ch08.daoying.f1")
talk("", "ch08.daoying.f2")
flag.set("ch08.yinian", 1)
