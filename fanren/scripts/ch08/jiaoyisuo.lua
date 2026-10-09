-- @hook ch08_jinguyuan trigger_jiaoyisuo interact once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.yinian") == 0 or flag.get("ch08.qiaoqian") ~= 0 then return end
give("material_lingshi", 100)
talk("chen_pangzi", "ch08.jiaoyisuo.t1")
talk("chen_qiaoqian", "ch08.jiaoyisuo.t2")
talk("chen_qiaoqian", "ch08.jiaoyisuo.f1")
flag.set("ch08.qiaoqian", 1)
