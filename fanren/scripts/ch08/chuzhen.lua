-- @hook ch08_jinguyuan trigger_chuzhen interact once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.qiaoqian") == 0 or flag.get("ch08.daji") ~= 0 then return end
talk("song_meng", "ch08.chuzhen.t1")
local won = battle("b08_jinguyuan_juezhan")
if not won then
    advance_days(30)
    return
end
advance_days(1)
talk("", "ch08.chuzhen.t2")
talk("", "ch08.chuzhen.f1")
flag.set("ch08.daji", 1)
