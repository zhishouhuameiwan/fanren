-- @hook ch03_andao trigger_andao_zhan enter once
-- 第三章节点 8 下半「暗道那一仗」，兼战棋教学第二场。
-- 挂在 ch03_andao 深处笼子那一排的 trigger（mode=enter，once=true，set_flag=ch03.andao_zhan）。
-- 这一行从前写的是 interact，与地图对不上；口径以首行的 @hook 为准，那一行由
-- tools/validate.py 比着 maps/*.tmj 校验。踏入即开战是合适的：笼子那一排是必经的，
-- 玩家没有「要不要打」的余地，这一点与谷外那条狼一致。
--
-- 教什么：法术与物品，重点是**用毒**。教法仍旧是让玩家自己撞出来，不写教程文本。
--   两只 long_zhong_shou 防 3，韩立平砍 6*2-3 = 9，三下一只 —— 教移动与站位；
--   一只 jiang_shou 防 9，平砍只掉 3 点，三十点气血要砍十下 —— 砍不动；
--   蚀心散 3 回合 × 6 点不过防，一包毒加四刀就收场 —— 玩家自己得出结论。
--   它的尸毒爪反过来给玩家上毒，清毒散的用处在同一场里教掉。
-- 这套数值是连着 data/roles/jiang_shou.json 与 data/items/pills/shixin_san.json
-- 一起定的，改一处要连着改。
--
-- **这一场可败。** 设计第 2 节写明教学二可败，败了是爬出暗道、腿上一道发青的口子。
-- 所以这里不得把 won == false 当死局，也不得把玩家挡在这一节前面——
-- 输了照样置 ch03.andao_zhan，照样往下走。输的那一条另有它的收获：
-- 他中了毒，吃了两包自己配的清毒散才压下去，于是他得出一个赢家得不到的结论：
-- 「原来它也带毒，那它身上的东西也是配出来的。」
--
-- 无论输赢都要接上的一条是 ch03.andao.giant：第十一只走路时两只脚上的分量一样重，
-- 和节点 2 那个不出声的一样。韩立就是从这一点推出巨汉是什么东西的，
-- 而这条推论到节点 12 掀开帽兜时才结账。
--
-- 笼子有十一只，对得上手札那一页的十一行（前九行「毙」、第十行「逸」、
-- 第十一行「未成」）。这是节点 6 偷来那本册子的第二个用处，也是原著 ch42
-- 「你不都用动物试过了吗」「至于其中死去的一只」与 ch64 炼尸未成的落地。

if flag.get("ch03.yuzitong_lu") ~= 1 then
    talk("", "ch03.andao.gate")
    return
end

if flag.get("ch03.andao_zhan") == 1 then
    talk("", "ch03.andao.again")
    return
end

talk("", "ch03.andao.deep")
talk("", "ch03.andao.cage")
talk("", "ch03.andao.book")
talk("", "ch03.andao.eleven")
talk("", "ch03.andao.ready")
talk("", "ch03.andao.start")

-- 教学二。这一场的三个返回值里只用得上第一个：
-- 赢、逃、败在剧情上分「打完了」与「没打完」两档，逃和败走同一条。
-- b03_andao_shishou 的 can_escape 是 true、defeat_is_fatal 是 false，
-- 所以这里没有 game_over 的余地，也不该有。
local won = battle("b03_andao_shishou")

if won then
    talk("", "ch03.andao.win1")
    talk("", "ch03.andao.win2")
    talk("", "ch03.andao.win3")
else
    talk("", "ch03.andao.lose1")
    -- 输的那一条要吃掉两包清毒散。**看返回值**：玩家完全可能在方才那一仗里
    -- 把三包全用光，那样他就是硬扛过来的——那句台词换一条，
    -- 而不是一边说「吃了两包」一边背包里一包没少。
    if take("pill_qingdu_san", 2) then
        talk("", "ch03.andao.lose2")
    else
        talk("", "ch03.andao.lose_bare")
    end
end

talk("", "ch03.andao.giant")
talk("", "ch03.andao.after")
talk("", "ch03.andao.end")

advance_days(120)

flag.set("ch03.andao_zhan")
