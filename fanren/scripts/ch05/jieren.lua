-- @hook ch05_xicheng trigger_jieren interact once
-- 第五章节点 4b「铁拳会截人」。十场必打之四：b05_tiequanhui_jieren。
-- 挂在 ch05_xicheng 码头后头仓房的门 (36,8)(37,8)（mode=interact，once=true，
-- guard_flag=ch05.qingbao，set_flag=ch05.jieren）。**踹开这扇门的是他**：孙二狗的人跑来报信，
-- 去不去救由他——所以是交互型（施工图 3.1）。
--
-- **改编**（施工图 3.2 / 8.3）：孙二狗是黑水巷唯一活着出来的人，铁拳会要问他黑熊哪去了。
-- 不冲突的依据：铁拳会与四平帮是同盟帮派（ch100），所以铁拳会拖他去问，而不是当场杀他；
-- 原著 ch105-106 对这几天只有「一番打听」四个字；铁拳会此后不再出现。
-- （旧稿写「ch104 孙二狗自己说过想报给上头又不敢」是引错：ch104 那两段是叙述者写他心里想把韩立的事
-- 报给**四平帮**上头，既非自陈，也与铁拳会无关——第 5 章校对 LOW-1。）
--
-- 救下之后孙二狗那一句按 ch05.shoufu 分三种（施工图 3.2：==1 多一句感激、==2 先看曲魂）。
-- 然后他腿软着去打听墨府，拨两天（施工图 3.3，ch106「经过一番打听」）。
-- 输即 game_over（defeat_is_fatal 真；can_escape 假）。

talk("", "ch05.jieren.door")
talk("", "ch05.jieren.in")

local won = battle("b05_tiequanhui_jieren")
if not won then
    talk("", "ch05.jieren.lost")
    game_over()
    return
end

talk("", "ch05.jieren.down")

local shoufu = flag.get("ch05.shoufu")
if shoufu == 1 then
    talk("sun_ergou", "ch05.jieren.sun_money")
elseif shoufu == 2 then
    talk("", "ch05.jieren.sun_hand")
else
    talk("sun_ergou", "ch05.jieren.sun_pill")
end

talk("", "ch05.jieren.go")

advance_days(2)

talk("", "ch05.jieren.back")

flag.set("ch05.jieren")
