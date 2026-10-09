-- @hook ch07_shandong trigger_fanhui interact once
-- 第七章节点 14「返回」。挂在 ch07_shandong 洞外空地那块大石 (20,10)
-- （mode=interact，once=true，guard_flag=ch07.yeyu，set_flag=ch07.fanhui）。收拾残局、带她走的是他。
--
-- 框架件（施工图 13.1 第 14 段）：她事后记不清他的脸；他想过灭口，没下手；马师伯说过元阳之体筑基机会大，他不碰她（不写欲念）；
-- 焚尸、抹掉打斗痕迹；带她往西百余里放下，喂清灵散解药毒（有则扣，没有换一句，不设死局）；
-- 回园闭关三日驱尽杂质；伤了元气，一个来月才复原；两粒筑基丹装进黄铜瓶、原来的盒子瓶子毁掉；陈师妹那一粒不还。
-- 日子：advance_days(1)（连夜赶回）＋ 3（闭关）＋ 30（养伤）（施工图 3.3）。
-- 中途 teleport ch06_baiyaoyuan (24,10)（genmaps_ch07.py 的 BAIYAOYUAN_LUKOU）：回园之后的几句在园子里说。登 kScriptTransfers（山洞 → 百药园）。

talk("", "ch07.fanhui.dawn")
talk("", "ch07.fanhui.face")
talk("", "ch07.fanhui.kill")
talk("", "ch07.fanhui.yuanyang")
talk("", "ch07.fanhui.clean")
talk("", "ch07.fanhui.carry")
if take("pill_qingling_san", 1) then
    talk("", "ch07.fanhui.qingling")
else
    talk("", "ch07.fanhui.noqingling")
end

advance_days(1)
teleport("ch06_baiyaoyuan", 24, 10)

talk("", "ch07.fanhui.home")
talk("", "ch07.fanhui.seclusion")
advance_days(3)
talk("", "ch07.fanhui.pills")
talk("", "ch07.fanhui.hers")
talk("", "ch07.fanhui.ledger")
talk("", "ch07.fanhui.month")
advance_days(30)

flag.set("ch07.fanhui")
