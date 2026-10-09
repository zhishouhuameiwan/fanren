-- @hook ch08_lingkuang trigger_chuansongzhen interact once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.zhanzhu") == 0 or flag.get("ch08.nuoyi") ~= 0 then return end
give("material_lingshi", 150)
give("material_lingshi_zhong", 4)
give("talisman_qingchi_fubao", 1)
if not magic.learn("magic_ji_qingchi") then return end
give("story_hongsha", 1)
give("story_danuoyi_ling", 1)
give("story_chuansong_tatu", 1)
give("story_zhuluan", 2)
give("story_wucai_xiaozhu", 8)
talk("", "ch08.chuansongzhen.t1")
talk("", "ch08.chuansongzhen.t2")
advance_days(8)
teleport("ch08_dongfu", 38, 24)
talk("", "ch08.chuansongzhen.f1")
talk("", "ch08.chuansongzhen.f2")
flag.set("ch08.nuoyi", 1)
