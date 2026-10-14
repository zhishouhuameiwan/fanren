-- @hook ch12_hanlitai trigger_xiaojian interact once
-- Facts and participants: docs/ch12-design.md 13.1 n32. 3.2：吞 / 听他说完，两支结果同（他决意灭之、令啼魂兽吞尽）；
-- 灵石启阵三档兜底（同 n24，契约第 4 节）；置 ch12.canhun；teleport("ch12_lingquan")。
-- kScriptTransfers 三元行：flag.set 与 teleport 写在顶格。抉择在付灵石之前，取消即 return。
talk("", "ch12.canhun.arrow")
talk("", "ch12.canhun.soul")
talk("xuan_gu", "ch12.canhun.beg")
local pick = choice({"ch12.canhun.opt_eat", "ch12.canhun.opt_hear"})
if not pick then return end
if pick == 2 then talk("", "ch12.canhun.hear") end
talk("", "ch12.canhun.eat")
talk("", "ch12.canhun.mark")
talk("", "ch12.canhun.out")
talk("", "ch12.canhun.scroll")
talk("", "ch12.canhun.room")
if take("material_lingshi", 4) then
    talk("", "ch12.canhun.paid")
elseif take("material_lingshi_zhong", 1) then
    talk("", "ch12.canhun.paid_zhong")
else
    talk("", "ch12.canhun.paid_none")
    advance_days(1)
end
talk("", "ch12.canhun.go")
flag.set("ch12.canhun")
teleport("ch12_lingquan")
