-- @hook ch02_wairentang npc_tongmen_maliu npc
-- 外刃堂杂役弟子马六的闲聊。挂在 ch02_wairentang 的 npc 对象上
-- （role_id=tongmen_maliu）。
--
-- 他与卢春是一对照：同一件事，一个人抢着说，一个人闭着嘴。
-- 玩家两个都问过，才拼得出厉飞雨是什么人——卢春给的是他的处境，
-- 马六给的是他的为人（谁家里出事，开口借钱他从没问过第二句）。
--
-- 为人那一句同样要等人情结下之后：在那之前马六守他们自己的规矩，
-- 当值不问闲事，问也问不出什么来。

if flag.get("ch02.liao_maliu") == 1 then
    if flag.get("ch02.renqing_jiexia") == 1 then
        talk("tongmen_maliu", "ch02.npc.maliu_lfy")
    end
    talk("tongmen_maliu", "ch02.npc.maliu_again")
    return
end

talk("", "ch02.npc.maliu_1")
talk("tongmen_maliu", "ch02.npc.maliu_2")

flag.set("ch02.liao_maliu")
