-- @hook ch12_tushan trigger_guaizhao enter once
-- Facts and participants: docs/ch12-design.md 13.1 n03. 3.2：二选一（两支都以噬金虫收场）；magic.learn 看返回值；领花篮、红光画轴；置 ch12.tuizhu 后 portal_dixue 可过。
-- 看守怪人一战只作旁白（拍板 8）。抉择在一切状态改动之前，取消即 return。
talk("", "ch12.guairen.drop")
talk("dao_guairen", "ch12.guairen.claim")
talk("", "ch12.guairen.read")
talk("", "ch12.guairen.basket")
talk("hu_yue", "ch12.guairen.huyue")
local pick = choice({"ch12.guairen.opt_hide", "ch12.guairen.opt_now"})
if not pick then return end
if pick == 1 then
    talk("", "ch12.guairen.hide")
else
    talk("", "ch12.guairen.now")
end
talk("", "ch12.guairen.swarm")
talk("", "ch12.guairen.flee")
give("story_hualan", 1)
give("story_zhiyang_huazhou", 1)
talk("", "ch12.guairen.loot")
local learned = magic.learn("magic_shijin_chongqun")
if learned then
    talk("", "ch12.guairen.learn")
else
    talk("", "ch12.guairen.learn_fail")
end
talk("", "ch12.guairen.zhu")
talk("qu_hun_shadan", "ch12.guairen.push")
talk("", "ch12.guairen.door")
flag.set("ch12.tuizhu")
