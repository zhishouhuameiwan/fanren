-- @hook ch05_xicheng trigger_xiaoxiangyuan interact once
-- 第五章节点 10b「潇湘院」。挂在 ch05_xicheng 潇湘院的院门 (35,28)(36,28)
-- （mode=interact，once=true，guard_flag=ch05.dingji，set_flag=ch05.xiaoxiang）。
-- 扮小厮敲包房的门是他（ch121）。
--
-- 原著 ch121-122：他扮小厮送酒；沈三看出面生；范沮让小厮先尝；他事先吃过清灵散，尝了没事；
-- 一盏茶后屋里接连笑倒（笑魂散）；连小金芝和另三个陪酒的女子一并毒死。
-- **自动核清灵散**（施工图 3.2）：
--   有 → take 1（看返回值）→ 照原著毒杀 → ch05.xiaoxiang = 1。
--        **陪酒的四个女子一并毒死，照原著一句带过，不渲染**（用户拍板）。
--   没有 → 条件战 b05_xiaoxiangyuan（不计入十场必打）：
--        赢 → ch05.xiaoxiang = 2（天霸门会来问，节点 11c 孙二狗说一句）；
--        逃 → 拨一天，次日下午再来，在置旗标之前 return，挂点留着；
--        输 → game_over。
-- **转写高风险点 7**：三大护法的绰号与绝技不一字排开。2026-09-25 整改（第 5 章校对 HIGH-1）之后，
-- 原著那三笔（刀囊、黑痣、胖子嚷酒）也不用了：本节只剩靠门的黑衣人一直在看他的手、另一个剔着牙叫他先尝，
-- 认帮主看的是谁的杯子先有人添，计时用的是楼下那支曲子；条件战的开场白里再一句飞刀与剑。

talk("", "ch05.xiaoxiang.dress")
talk("", "ch05.xiaoxiang.knock")
talk("", "ch05.xiaoxiang.shen3")
talk("", "ch05.xiaoxiang.excuse")
talk("", "ch05.xiaoxiang.fan")

if item.count("pill_qingling_san") > 0 and take("pill_qingling_san", 1) then
    talk("", "ch05.xiaoxiang.taste")
    talk("", "ch05.xiaoxiang.leave")
    talk("", "ch05.xiaoxiang.laugh")
    talk("", "ch05.xiaoxiang.women")
    talk("", "ch05.xiaoxiang.name")
    flag.set("ch05.xiaoxiang", 1)
    return
end

talk("", "ch05.xiaoxiang.nopill")
talk("", "ch05.xiaoxiang.draw")

local won, how = battle("b05_xiaoxiangyuan")

if won then
    talk("", "ch05.xiaoxiang.fought")
    flag.set("ch05.xiaoxiang", 2)
    return
end

if how == "escaped" then
    talk("", "ch05.xiaoxiang.retreat")
    advance_days(1)
    return
end

talk("", "ch05.xiaoxiang.lost")
game_over()
