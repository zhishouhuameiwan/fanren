-- @hook ch05_kezhan trigger_shuijiao interact once
-- 第五章节点 10c「夺帮一夜」。十场必打之五：b05_duobang。
-- 挂在 ch05_kezhan 上房的床 (2,2)(3,2)（mode=interact，once=true，guard_flag=ch05.xiaoxiang，
-- set_flag=ch05.duobang）。回客栈倒头就睡是他的老习惯（ch122「一干完某件大事，他就特别的嗜睡」）；
-- 镜头切到四平帮总舵。内视在蒲团上，不与这张床同格。
--
-- 原著 ch122：沈重山与三大护法死的当夜，四平帮的头目为帮主之位火拼；孙二狗靠曲魂一夜杀光反对他的高层，
-- 第二天就往西城各帮发帖。**韩立那一夜在客栈睡觉**——所以这一仗是「韩立不在场的战斗」：
-- 编成 b05_duobang 写 hero_absent 真（docs/interfaces-p3-ch05.md 2.1），不建韩立、不带队伍、不登记背包，
-- 玩家操纵曲魂与孙二狗手下两个脚夫（ch104：他管着四五十个苦力）。
--
-- 这一夜过去算一天（施工图 3.3：10c 夺帮一夜 1，输一回再 ＋1），不论输赢都拨。
--   赢 → 天亮，孙二狗已是帮主、发了帖 → 置 ch05.duobang。
--   输 → 孙二狗带人退回码头，第二夜再来：在置旗标之前 return，挂点留着。
--        defeat_is_fatal 假——这一仗韩立在睡觉，不该让他死在里头。
-- can_escape 假，所以 "escaped" 不该出现；真出现了按输算（退回码头，改日再来）。

talk("", "ch05.duobang.sleep")
talk("", "ch05.duobang.cut")
talk("sun_ergou", "ch05.duobang.go")

local won = battle("b05_duobang")

advance_days(1)

if not won then
    talk("", "ch05.duobang.lost1")
    return
end

talk("", "ch05.duobang.won1")
talk("", "ch05.duobang.won2")

flag.set("ch05.duobang")
