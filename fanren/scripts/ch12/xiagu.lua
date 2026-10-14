-- @hook ch12_xiaojie trigger_xiagu enter once
-- Facts and participants: docs/ch12-design.md 13.1 n19. 3.2：「玄晶道」一句后回选单；领白犀佩、寒冰珠；置 ch12.xiagu；teleport("ch12_binghuo")。
-- kScriptTransfers 三元行：flag.set 与 teleport 写在顶格。抉择在领物之前，取消即 return。
talk("", "ch12.xiagu.peak")
talk("", "ch12.xiagu.crowd")
talk("", "ch12.xiagu.man")
talk("man_huzi", "ch12.xiagu.want")
talk("", "ch12.xiagu.deal")
talk("", "ch12.xiagu.stele")
while true do
    local pick = choice({"ch12.xiagu.opt_xuanjing", "ch12.xiagu.opt_rongyan"})
    if not pick then return end
    if pick == 2 then break end
    talk("jiyin_zushi", "ch12.xiagu.xuanjing_back")
end
talk("jiyin_zushi", "ch12.xiagu.lend")
give("story_baixi_pei", 1)
talk("man_huzi", "ch12.xiagu.lend2")
give("story_hanbing_zhu", 1)
talk("", "ch12.xiagu.in")
flag.set("ch12.xiagu")
teleport("ch12_binghuo")
