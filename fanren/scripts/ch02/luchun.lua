-- @hook ch02_wairentang npc_tongmen_luchun npc
-- 外刃堂杂役弟子卢春的闲聊。挂在 ch02_wairentang 的 npc 对象上
-- （role_id=tongmen_luchun）。
--
-- 他负责把门里的风声说给玩家听：贫富两拨弟子互相挤、厉飞雨升得太快、
-- 上头好几位堂主的爱徒被他压在底下。这几句是节点 7 的前情——
-- 一个人为什么肯拿寿数换刀法，得先让玩家知道他站在什么位置上。
--
-- 关于厉飞雨的那一句要等人情结下之后才说得出口：在那之前，
-- 韩立与这个名字还没有关系，卢春也没有由头单挑这一桩讲。

if flag.get("ch02.liao_luchun") == 1 then
    if flag.get("ch02.renqing_jiexia") == 1 then
        talk("tongmen_luchun", "ch02.npc.luchun_lfy")
    end
    talk("tongmen_luchun", "ch02.npc.luchun_again")
    return
end

talk("tongmen_luchun", "ch02.npc.luchun_1")
talk("tongmen_luchun", "ch02.npc.luchun_2")
talk("tongmen_luchun", "ch02.npc.luchun_3")

flag.set("ch02.liao_luchun")
