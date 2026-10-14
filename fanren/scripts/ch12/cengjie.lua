-- @hook ch12_taqian trigger_tamen interact once
-- Facts and participants: docs/ch12-design.md 13.1 n27. 3.2：Z2 收 / 不收傀儡残骸（收则 give + ch12.z2，写在 ch12.cengjie 之前，第 6 节）；
-- 置 ch12.cengjie；teleport("ch12_hanlitai")。kScriptTransfers 三元行：flag.set 与 teleport 写在顶格。
-- 抉择在一切状态改动之前，取消即 return。「狼首傀儡」「蛇卫」「寒骊台」从本段起解禁；「天罡罩」留到 juanzhou（12.2）。
talk("", "ch12.cengjie.maze")
talk("wu_chou", "ch12.cengjie.rule")
talk("", "ch12.cengjie.wolf")
local pick = choice({"ch12.cengjie.opt_take", "ch12.cengjie.opt_leave"})
if not pick then return end
if pick == 1 then
    talk("", "ch12.cengjie.take")
    give("story_kuilei_canhai", 1)
    flag.set("ch12.z2")
else
    talk("", "ch12.cengjie.leave")
end
talk("", "ch12.cengjie.snake")
talk("", "ch12.cengjie.upper")
talk("", "ch12.cengjie.tai")
flag.set("ch12.cengjie")
teleport("ch12_hanlitai")
