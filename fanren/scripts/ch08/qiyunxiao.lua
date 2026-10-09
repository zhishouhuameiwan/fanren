-- @hook ch08_tianxing_fangshi trigger_qiyunxiao interact once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.midian") == 0 or flag.get("ch08.qiyunxiao") ~= 0 then return end
if not take_aged("herb_zigui_hua", 1, 1000) then
talk("", "ch08.qiyunxiao.t1")
return
end
talk("qi_yunxiao", "ch08.qiyunxiao.t2")
talk("qi_yunxiao", "ch08.qiyunxiao.ancestry")
talk("qi_yunxiao", "ch08.qiyunxiao.t3")
talk("", "ch08.qiyunxiao.t4")
give("story_diandao_zhenqi", 1)
give("story_yunxiao_xinde", 1)
advance_days(1)
talk("qi_yunxiao", "ch08.qiyunxiao.f1")
talk("qi_yunxiao", "ch08.qiyunxiao.f2")
teleport("ch08_tianxing_fangshi", 8, 18)
flag.set("ch08.qiyunxiao", 1)
