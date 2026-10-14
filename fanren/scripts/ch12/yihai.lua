-- @hook ch12_guiwu trigger_yihai interact once
-- Z1 埋骨（可选，docs/ch12-design.md 第 6 节、13.1 Z1）。不在目标链上。
-- ch12.guiwang 已置 → 一句旁白 return、不置 ch12.z1（完成压过失败，免得过期的任务翻成完成）。取消 return。
if flag.get("ch12.guiwang") ~= 0 then talk("", "ch12.yihai.late"); return end
talk("", "ch12.yihai.bones")
-- 紫灵留在坡上等他探路（ch12-review.md C-3），认出遗骸改成旁白。
talk("", "ch12.yihai.ziling")
local pick = choice({"ch12.yihai.opt_take", "ch12.yihai.opt_bury"})
if not pick then return end
if pick == 1 then
    give("story_bihuo_baoyi", 1)
    talk("hanli", "ch12.yihai.take")
    talk("", "ch12.yihai.take_after")
else
    talk("", "ch12.yihai.bury")
end
talk("", "ch12.yihai.done")
flag.set("ch12.z1")
