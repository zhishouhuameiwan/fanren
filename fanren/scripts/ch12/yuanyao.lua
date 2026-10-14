-- @hook ch12_guiwu trigger_huayuan enter once
-- Facts and participants: docs/ch12-design.md 13.1 n14. 3.2：拒 / 问来意（多一段后仍拒）合流；取消 return（本段无状态改动）。
-- 名牌：揭面之前是「黑袍人」（yuan_yao_heipao），之后是元瑶（yuan_yao_jiedan）；「元瑶」二字只在本段及以后（12.2）。
talk("", "ch12.yuanyao.hood")
talk("hanli", "ch12.yuanyao.name")
talk("yuan_yao_heipao", "ch12.yuanyao.deny")
talk("hanli", "ch12.yuanyao.remind")
talk("", "ch12.yuanyao.recall")
talk("yuan_yao_jiedan", "ch12.yuanyao.same")
talk("", "ch12.yuanyao.garden")
talk("", "ch12.yuanyao.lvpao")
talk("yuan_yao_jiedan", "ch12.yuanyao.ask")
local pick = choice({"ch12.yuanyao.opt_no", "ch12.yuanyao.opt_why"})
if not pick then return end
if pick == 2 then
    talk("hanli", "ch12.yuanyao.why")
    talk("yuan_yao_jiedan", "ch12.yuanyao.why_reply")
end
talk("hanli", "ch12.yuanyao.no")
talk("", "ch12.yuanyao.leave")
flag.set("ch12.yuanyao")
