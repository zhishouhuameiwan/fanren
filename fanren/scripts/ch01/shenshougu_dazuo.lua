-- @hook ch01_shenshougu trigger_dazuo_jiaoxue enter once
-- 第一章节点 8 下半「第一次打坐」。挂在谷口平石那处 facility（kind=meditate）旁。
--
-- 节点 8 拆成两个脚本，是因为中间要等玩家自己走到打坐处去交互——
-- 一段脚本没办法在中途把控制权交还给玩家再收回来。
--
-- 这一坐什么都不会发生，只有腿麻。本章的七玄门是个凡人武馆，
-- 不许出现气感、灵气、暖流之类的东西；墨大夫还要专门说一句
-- 「觉出来的多半是自己想出来的」，把玩家的期待按回去。

if flag.get("ch01.dazuo_done") == 1 then
    talk("", "ch01.dazuo.again")
    return
end

talk("", "ch01.dazuo.sit")
talk("", "ch01.dazuo.nothing")
talk("", "ch01.dazuo.leg")

talk("", "ch01.dazuo.mo_back")
talk("mo_daifu", "ch01.dazuo.mo_reply")
talk("mo_daifu", "ch01.dazuo.mo_reply2")
talk("", "ch01.dazuo.after")

flag.set("ch01.dazuo_done")
