-- @hook ch08_dongfu trigger_shichuang interact once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.buzhen1") == 0 or flag.get("ch08.yexi") ~= 0 then return end
talk("", "ch08.shichuang.night")
talk("qianzhujiao_jiaozhong", "ch08.shichuang.t1")
talk("", "ch08.shichuang.t2")
local pick = choice({"ch08.shichuang.choice1", "ch08.shichuang.choice2"})
if not pick then return end
flag.set("ch08.yexi", pick)
if pick == 1 then
talk("hanli", "ch08.shichuang.ask")
talk("lin_shishu_yuanshen", "ch08.shichuang.t3")
talk("lin_shishu_yuanshen", "ch08.shichuang.t4")
talk("lin_shishu_yuanshen", "ch08.shichuang.f1")
end
talk("", "ch08.shichuang.t5")
talk("", "ch08.shichuang.f2")
if flag.get("ch08.yexi") == 0 then flag.set("ch08.yexi", 1) end
