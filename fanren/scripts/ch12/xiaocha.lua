-- @hook ch12_dixue npc_shaonian npc
-- Facts and participants: docs/ch12-design.md 13.1 n06. 3.2：开头 canyuan 已置 → return；二选一（据实 / 搪塞，都不泄底）合流；置 ch12.canyuan。
-- 「萧诧」全章只在这里出现一次（12.2）。本段无状态改动，取消即 return。
if flag.get("ch12.canyuan") ~= 0 then return end
talk("xuangu_shaonian", "ch12.xiaocha.age")
talk("xuangu_shaonian", "ch12.xiaocha.name")
talk("", "ch12.xiaocha.judge")
talk("xuangu_shaonian", "ch12.xiaocha.jade")
local pick = choice({"ch12.xiaocha.opt_truth", "ch12.xiaocha.opt_dodge"})
if not pick then return end
if pick == 1 then
    talk("hanli", "ch12.xiaocha.truth")
else
    talk("hanli", "ch12.xiaocha.dodge")
end
talk("xuangu_shaonian", "ch12.xiaocha.verdict")
talk("", "ch12.xiaocha.rib")
talk("xuangu_shaonian", "ch12.xiaocha.go")
talk("", "ch12.xiaocha.gone")
talk("", "ch12.xiaocha.collect")
talk("", "ch12.xiaocha.jinpa")
flag.set("ch12.canyuan")
