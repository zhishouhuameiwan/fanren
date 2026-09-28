-- @hook ch02_jusuo npc_zhang_tie npc
-- 张铁的闲聊。挂在 ch02_jusuo 的 npc 对象上（role_id=zhang_tie）。
--
-- 第 1 章已经定下：张铁与韩立是被墨大夫一同收下的，同住一屋（ch01 设计第 5 节
-- 的改编裁决，为的是第 3 章曲魂线要他一直在韩立近旁）。本章沿用这条，
-- 他不走，也不写他走。
--
-- 他练的是象甲功，第一层就要泡药汤、挨木棒（原著 ch9、ch13）。
-- 他的疼要写具体：挨了七棒、一按一个坑、泡完站起来眼前发黑。
-- 这份具体是本章的一条暗线——同一座山上，有人拿身子换武功，有人拿寿数换武功，
-- 只有韩立那段口诀什么也换不到。他后来肯下死力气种药攒钱，根子在这儿。
--
-- 拾瓶之后多一句：张铁问他脖子上那道口子（抽髓丸那回留下的）。
-- 他随口扯了个谎——这是他「有事别让人知道」的自觉在长出来，不是他学坏了。

if flag.get("ch02.liao_zhangtie") == 1 then
    if flag.get("ch02.chousui_jian") == 1 then
        talk("zhang_tie", "ch02.npc.zhangtie_ping")
    end
    talk("zhang_tie", "ch02.npc.zhangtie_again")
    return
end

talk("zhang_tie", "ch02.npc.zhangtie_1")
talk("zhang_tie", "ch02.npc.zhangtie_2")
talk("zhang_tie", "ch02.npc.zhangtie_3")

flag.set("ch02.liao_zhangtie")
