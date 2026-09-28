-- @hook ch02_jusuo trigger_zaping interact
-- 第二章节点 5 上半「砸不开、泡不开」。
-- 挂在 ch02_jusuo 桌边的 trigger（mode=interact）。
--
-- 节点 5 拆成两个脚本：这一段是他把能想到的法子都试了一遍，
-- 下一段（kaigai.lua）是第八日瓶盖自开。中间隔着七天，
-- 那七天要玩家自己过——一段脚本没办法在中途把控制权交还给玩家再收回来。
--
-- 原著三章（ch11 拧不动、ch12 砸不烂、ch13 夜里起异象）在这里压成一段。
-- 保住的是三件事：他想找张铁帮忙又自己打消了；他开始把这瓶子当成不能让人
-- 知道的东西；以及暴力手段彻底失败。第三件是为下一段那「随手一拧就开了」
-- 做落差的，没有这一段的白费力气，第八日就不成其为第八日。
--
-- 皮袋与平安符沿用第 1 章：那是他娘用兽皮缝的，里头原本只有一枚野猪牙符。
-- 段五他会因为去摸这枚符，才重新翻出这只瓶子。

if flag.get("ch02.shiping_done") ~= 1 then
    talk("", "ch02.zaping.gate")
    return
end

if flag.get("ch02.zaping_done") == 1 then
    talk("", "ch02.zaping.again")
    return
end

talk("", "ch02.zaping.room")
talk("", "ch02.zaping.clean")
talk("", "ch02.zaping.twist")

talk("", "ch02.zaping.zt")
talk("", "ch02.zaping.zt2")

talk("", "ch02.zaping.soak")
talk("", "ch02.zaping.hammer")
talk("", "ch02.zaping.hit")
talk("", "ch02.zaping.result")

talk("", "ch02.zaping.decide")
talk("", "ch02.zaping.pouch")
talk("", "ch02.zaping.sleep")

flag.set("ch02.zaping_done")
