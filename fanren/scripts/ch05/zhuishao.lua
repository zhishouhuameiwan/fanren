-- @hook ch05_xicheng trigger_zhuishao enter once
-- 第五章节点 3b「码头盯梢」。十场必打之三：b05_matou_zhuishao。
-- 挂在 ch05_xicheng 往南城去的巷口 (8,31)(9,31)（mode=enter，once=true，guard_flag=ch05.shoufu，
-- set_flag=ch05.zhuishao）。被人跟上是对方的动作；这一格在西城去南城那道门之前，门的闸门
-- 要的就是本节的完成旗标（ch05.block.nancheng）。
--
-- **改编**（施工图 3.2 / 8.3）：码头上黑大个那一伙的人看着他们的头领领人跟这个外乡人走了，
-- 一个也没回来，天擦黑跟了上来。不冲突的依据：铁拳会在 ch104 之后原著不再出现；
-- ch103 韩立吩咐孙二狗「不想有人知道这儿发生的一切」，这一仗打完，知道的人就真没了。
-- 这时他还不知道那一伙叫铁拳会（孙二狗在节点 4a 才说），文案只说「黑大个那一伙」。
--
-- 打完他在南城拣一家客栈住下：汇源客栈（ch104）。节点 4a 从那里起。

talk("", "ch05.zhuishao.dusk")
talk("", "ch05.zhuishao.who")
talk("", "ch05.zhuishao.think")

local won = battle("b05_matou_zhuishao")
if not won then
    talk("", "ch05.zhuishao.lost")
    game_over()
    return
end

talk("", "ch05.zhuishao.win")
talk("", "ch05.zhuishao.inn")

flag.set("ch05.zhuishao")
