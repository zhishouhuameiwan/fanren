-- @hook ch08_yuejing trigger_qingbao interact
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.fengwu") == 0 then return end
if flag.get("ch08.qb_huangmiao") ~= 0 and flag.get("ch08.qb_yuanbing") == 0 then
talk("", "ch08.qingbao.t1")
advance_days(18)
talk("", "ch08.qingbao.reinforcements")
talk("liu_jing", "ch08.qingbao.t2")
talk("wu_xuan", "ch08.qingbao.t3")
talk("zhong_weiniang", "ch08.qingbao.t4")
flag.set("ch08.qb_yuanbing", 1)
return
end
if flag.get("ch08.qb_yuanbing") ~= 0 and flag.get("ch08.buzhen2") == 0 then
local pick = choice({"ch08.qingbao.choice1", "ch08.qingbao.choice2"})
if not pick then return end
flag.set("ch08.jingong_yubei", pick)
if pick ~= 1 then return end
teleport("ch08_yuejing", 18, 12)
return
end
if flag.get("ch08.qb_huangmiao") ~= 0 then
talk("", "ch08.qingbao.t5")
elseif flag.get("ch08.qb_wumei") ~= 0 then
talk("", "ch08.qingbao.t6")
elseif flag.get("ch08.qb_xuezhou") ~= 0 then
talk("", "ch08.qingbao.t7")
elseif flag.get("ch08.qb_shouyan") ~= 0 then
talk("", "ch08.qingbao.t8")
elseif flag.get("ch08.qb_mengshan") ~= 0 then
talk("", "ch08.qingbao.t9")
else
talk("", "ch08.qingbao.t10")
end
