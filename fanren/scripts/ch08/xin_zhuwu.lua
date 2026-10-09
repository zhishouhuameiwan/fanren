-- @hook ch08_jinmacheng trigger_xin_zhuwu interact once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.jiuren") == 0 or flag.get("ch08.yueding") ~= 0 then return end
if item.count("story_chuansong_tatu") > 0 and not take("story_chuansong_tatu", 1) then return end
talk("xin_ruyin", "ch08.xin_zhuwu.t1")
talk("", "ch08.xin_zhuwu.t2")
advance_days(14)
talk("xin_ruyin", "ch08.xin_zhuwu.f1")
talk("", "ch08.xin_zhuwu.f2")
teleport("ch08_yuejing", 58, 40)
flag.set("ch08.yueding", 1)
