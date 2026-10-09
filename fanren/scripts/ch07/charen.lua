-- @hook ch07_huanxingshan trigger_charen interact once
-- 第七章节点 26b「紫猴花洞：插刃」。挂在 ch07_huanxingshan 竖洞道中段 (8,24)(9,24)
-- （mode=interact，once=true，guard_flag=ch07.kuitan，set_flag=ch07.mairen）。退回洞道埋刃的是他。
--
-- 框架件（施工图 13.1 第 26 段 ④）：硬拼不划算，退回洞道，把八柄子刃刃口朝上埋在地里（原著抹黑泥那一手不用）。
-- **玩家自己插**：一个选项「插下一柄」按八次（choice 循环，不是新机制；施工图 3.2 节点 26b）。选项不点名按键，
-- 所以没有新的 {key.*} 提示（施工图 13 第 9 条预计的那一条不需要，记第 18 节）。
-- 中途取消 = 收手：一柄也不算、不置旗标、触发器留着，回来重插（不设死局）。

talk("", "ch07.charen.back")
talk("", "ch07.charen.how")

local planted = 0
while planted < 8 do
    local pick = choice{ "ch07.charen.opt_plant" }
    if pick == nil then
        talk("", "ch07.charen.stop")
        return
    end
    planted = planted + 1
    if planted == 1 then
        talk("", "ch07.charen.first")
    elseif planted == 4 then
        talk("", "ch07.charen.half")
    elseif planted == 7 then
        talk("", "ch07.charen.seventh")
    end
end
talk("", "ch07.charen.done")

flag.set("ch07.mairen")
