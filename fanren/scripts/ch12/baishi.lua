-- @hook ch12_xiaojie npc_jiyin npc
-- Facts and participants: docs/ch12-design.md 13.1 n18. 3.2：开头 baishi 已置 → return；三选一循环（拜青易 / 都不拜各接一句传音后回选单，
-- 只有拜极阴往下走，拍板 16）；领玄阴环；置 ch12.baishi；蜘蛛不扣。取消 return（领环之前，无状态改动）。
-- 极阴开出的具体条件一样都不写（13 第 4 条）。
if flag.get("ch12.baishi") ~= 0 then return end
talk("", "ch12.baishi.cloud")
talk("jiyin_zushi", "ch12.baishi.silk")
talk("", "ch12.baishi.spider")
talk("hanli", "ch12.baishi.claim")
talk("qingyi_jushi", "ch12.baishi.qingyi")
talk("", "ch12.baishi.three")
talk("qingyi_jushi", "ch12.baishi.qing_v")
talk("jiyin_zushi", "ch12.baishi.ji_v")
talk("xuan_gu", "ch12.baishi.xg_v")
while true do
    local pick = choice({"ch12.baishi.opt_ji", "ch12.baishi.opt_qing", "ch12.baishi.opt_none"})
    if not pick then return end
    if pick == 1 then break end
    if pick == 2 then
        talk("jiyin_zushi", "ch12.baishi.qing_back")
    else
        talk("xuan_gu", "ch12.baishi.none_back")
    end
end
talk("", "ch12.baishi.bow")
talk("jiyin_zushi", "ch12.baishi.accept")
give("story_xuanyin_huan", 1)
talk("", "ch12.baishi.ring")
talk("xuan_gu", "ch12.baishi.xg_warn")
talk("", "ch12.baishi.road")
flag.set("ch12.baishi")
