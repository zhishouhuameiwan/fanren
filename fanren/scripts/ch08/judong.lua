-- @hook ch08_lingkuang trigger_judong enter once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.shouzhen") == 0 or flag.get("ch08.zhanzhu") ~= 0 then return end
talk("", "ch08.judong.f1")
talk("", "ch08.judong.t1")
talk("", "ch08.judong.t2")
talk("", "ch08.judong.f2")
talk("xuan_le", "ch08.judong.t3")
talk("", "ch08.judong.t4")
talk("", "ch08.judong.f3")
local chujiao_before = item.count("talisman_mojiao_chujiao")
local won = battle("b08_zhongrudong")
if not won then
    game_over()
    return
end
if item.count("talisman_mojiao_chujiao") < chujiao_before then
    talk("", "ch08.judong.used")
elseif chujiao_before > 0 then
    talk("", "ch08.judong.notused")
else
    talk("", "ch08.judong.absent")
end
talk("", "ch08.judong.t5")
advance_days(1)
flag.set("ch08.zhanzhu", 1)
