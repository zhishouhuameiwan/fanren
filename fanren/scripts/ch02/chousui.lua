-- @hook ch02_wairentang trigger_chousui enter once
-- 第二章节点 7「抽髓丸之祸」。段三（第二年）的进场事件。
-- 挂在 ch02_wairentang 后院的 trigger（mode=enter, once=true）。
--
-- 出处 ch20。抽髓丸是一味秘药：吃下去把往后的力气提前支出来，
-- 吃一回就断不得，断了轻则瘫、重则送命。它属于凡人武馆这一层的东西——
-- 不是法术，不是丹药修行，是一个少年拿寿数换刀法。本章不许往修仙上靠。
--
-- 设计文档给这一节定的玩法是「对话（旁人的痛苦，主角旁观）」。
-- 所以这里没有选择：韩立救人是学医的人的反射，不是他挑出来的路。
-- 他真正要做选择的地方在下一节——配哪一种药。
--
-- 厉飞雨的痛苦要写得具体：手指在砖缝里抠出血、咬着牙还是吼了出来、
-- 吼声在巷子里撞来撞去。不许只说「很痛」。
--
-- 结尾那句「往后再撞上这种事，该不该伸手」是本章人物变化的落点：
-- 第 1 章他是被安排的孩子，这一章他开始自己算账。
-- 注意分寸——他只是起了这个念头，本章不给他答案。
--
-- 硬约束（docs/ch02-design.md 第 1 节第 6 条）：结识只到配药结下的人情为止，
-- 没有结义。这里连「兄弟」两个字都不出现。

if flag.get("ch02.duan3_start") ~= 1 then
    return
end

if flag.get("ch02.chousui_jian") == 1 then
    talk("", "ch02.chousui.again")
    return
end

talk("", "ch02.chousui.arrive")
talk("tongmen_luchun", "ch02.chousui.tm1")
talk("tongmen_maliu", "ch02.chousui.tm2")
talk("", "ch02.chousui.scream")

talk("", "ch02.chousui.go")
talk("", "ch02.chousui.face")
talk("", "ch02.chousui.needle")
talk("", "ch02.chousui.needle2")
talk("", "ch02.chousui.wake")
talk("", "ch02.chousui.smell")
talk("", "ch02.chousui.know")
talk("", "ch02.chousui.give")
talk("", "ch02.chousui.pain")
talk("", "ch02.chousui.wait")
talk("", "ch02.chousui.after")

talk("li_feiyu", "ch02.chousui.knife")
talk("", "ch02.chousui.reply")
talk("", "ch02.chousui.reply2")
talk("", "ch02.chousui.oath")

talk("li_feiyu", "ch02.chousui.lfy_ask")
talk("", "ch02.chousui.promise")
talk("", "ch02.chousui.end")

flag.set("ch02.chousui_jian")
