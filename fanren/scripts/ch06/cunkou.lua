-- @hook ch06_tainan_cun trigger_cunkou enter once
-- 第六章节点 1b「村口遇万小山」。挂在 ch06_tainan_cun 林子东缘土坡的豁口 (14,20)(14,21)
-- （mode=enter，once=true，guard_flag=ch06.chudu，set_flag=ch06.wan_met）。他一进借住的小村就撞见了（ch126），
-- 所以是踏入型；豁口是林子通村子唯一的路。
--
-- **天眼术在这里第一次真正施展**（施工图 1.1 第 2 条、10.1 E1；第 5 章拍板第 10 条推过来的）：
-- 看的是万小山，灵光只比他略弱一点。本章学天眼术只此一处（验收 8）。
-- 看返回值：学不到（数据缺失）就说一句，戏照演。天眼术的数据由引擎路建（docs/interfaces-p3-ch06.md 1.2）。
-- 演完万小山站在村口（npc_wan_xiaoshan visible_flag=ch06.wan_met），可以跟他说话；领他去怪坡是节点 2。

talk("", "ch06.cunkou.boy")
talk("", "ch06.cunkou.look")

local ok = magic.learn("magic_tianyan_shu")
if ok then
    talk("", "ch06.cunkou.eye")
else
    talk("", "ch06.cunkou.eye_fail")
end

talk("", "ch06.cunkou.run")
talk("wan_xiaoshan", "ch06.cunkou.hello")
talk("", "ch06.cunkou.face")
talk("", "ch06.cunkou.where")
talk("wan_xiaoshan", "ch06.cunkou.dunno")
talk("", "ch06.cunkou.first")
talk("wan_xiaoshan", "ch06.cunkou.yes")
talk("", "ch06.cunkou.lead")

flag.set("ch06.wan_met")
