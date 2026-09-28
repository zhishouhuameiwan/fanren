-- @hook ch03_guwai trigger_elang enter once
-- 第三章节点 1「谷外遇狼」，兼战棋教学第一场。
-- 挂在 ch03_guwai 山道处的 trigger（mode=enter，once=true，set_flag=ch03.elang_done）。
-- 这一行从前写的是 interact，与地图对不上。enter 才是对的：狼是扑上来的，
-- 不该要玩家先按一下确认键才肯开打。口径以首行的 @hook 为准（validate.py 校验）。
--
-- 本章之前一场战斗也没有，所以这一场负责把战棋教会，而不是假设玩家已经会了。
-- 教的是移动、攻击、地形三样，教法是让他自己撞上：
-- 两条狼分开站、中间空一大块（逼玩家看站位），身后的干河床比前头远（逼玩家算步数）。
-- b03_gu_wai_elang 可逃可败，败了只是被打退，所以**不得把 won == false 当死局**。
--
-- 这一场真正要交待的不是战斗，是韩立为什么会开始配毒。
-- 他的结论不是「我要变强」，是「从看见到扑过来只有三下，我手上没有一样东西
-- 是三下之内使得出来的」。这条推论直接接上第 2 章那句「学医先学不害人」——
-- 那一页他四年来一直翻过去，这一夜他翻了回去。
-- **不要把这一段写成少年立志**：他自始至终在算自己能不能活。
--
-- 出口：给一筒五毒水（pill_wudu_shui）。它在节点 3 摊牌时会被墨居仁当面要走，
-- 而那一处的 take() 必须看返回值——玩家完全可能在别的战斗里先用掉它。

if flag.get("ch02.done") ~= 1 then
    talk("", "ch03.elang.gate")
    return
end

if flag.get("ch03.elang_done") == 1 then
    talk("", "ch03.elang.again")
    return
end

-- 第 2 章收在他十四岁的春天，本章开在第五年夏天，他十五。
-- 这一年是空的：师父下山采药去了，谷里没有人问他口诀。
-- 时间推在最前面，为的是让灵田与掌天瓶把这一年真的走完——
-- 玩家回药圃时，田里的药确实长了一年。
talk("", "ch03.elang.year")
advance_days(330)

talk("", "ch03.elang.road")
talk("", "ch03.elang.wind")
talk("", "ch03.elang.see")
talk("", "ch03.elang.count")
talk("", "ch03.elang.stand")
talk("", "ch03.elang.stone")

-- 教学一。三个返回值里这里只用得上第一个：这一场赢、逃、败三种落点
-- 在剧情上只分「打退了」与「没打退」两档，逃和败走同一条。
local won = battle("b03_gu_wai_elang")

if won then
    talk("", "ch03.elang.win1")
    talk("", "ch03.elang.win2")
    talk("", "ch03.elang.win3")
else
    talk("", "ch03.elang.flee1")
    talk("", "ch03.elang.flee2")
end

-- 以下两条路合流。赢的那一条他也得出同一个结论——这是有意的：
-- 他赢的那一场是靠一块带棱的石头和两条胳膊，赢完胳膊酸得抬不起来。
-- 赢了就以为自己行了的人，这一章活不到第十一节。
talk("", "ch03.elang.after")
talk("", "ch03.elang.hand")
talk("", "ch03.elang.night")
talk("", "ch03.elang.page")

advance_days(3)

talk("", "ch03.elang.make")
give("pill_wudu_shui", 1)
talk("", "ch03.elang.sew")
talk("", "ch03.elang.end")

flag.set("ch03.elang_done")
