-- @hook ch12_baoguang trigger_yuqiao enter once
-- Facts and participants: docs/ch12-design.md 13.1 n24. 3.2：灵石启阵三档兜底（低阶 4 → 中阶 1 → advance_days(1) 照走，契约第 4 节）；
-- 二选一取舍 → ch12.quba_xian 1 披风 / 2 铜环；两件都到手（拍板 14）；置 ch12.quba；teleport("ch12_taqian")。
-- kScriptTransfers 三元行：flag.set 与 teleport 写在顶格。抉择在付灵石之后：取消按「先取披风」一支走，不 return。
-- 本段只写「暗红披风」，「血色披风」留到 lingquan（12.2）。
talk("", "ch12.quba.bridge")
talk("", "ch12.quba.array")
if take("material_lingshi", 4) then
    talk("", "ch12.quba.paid")
elseif take("material_lingshi_zhong", 1) then
    talk("", "ch12.quba.paid_zhong")
else
    talk("", "ch12.quba.paid_none")
    advance_days(1)
end
talk("", "ch12.quba.dome")
talk("", "ch12.quba.rule")
local pick = choice({"ch12.quba.opt_cloak", "ch12.quba.opt_ring"})
if pick == 2 then
    flag.set("ch12.quba_xian", 2)
    talk("", "ch12.quba.first_ring")
    talk("", "ch12.quba.armor")
    talk("", "ch12.quba.second_cloak")
else
    flag.set("ch12.quba_xian", 1)
    talk("", "ch12.quba.first_cloak")
    talk("", "ch12.quba.armor")
    talk("", "ch12.quba.second_ring")
end
give("story_xuese_pifeng", 1)
give("story_wuse_tonghuan", 1)
talk("", "ch12.quba.mirror")
talk("", "ch12.quba.moved")
flag.set("ch12.quba")
teleport("ch12_taqian")
