-- @hook ch05_dukou trigger_dongqu_lu enter once
-- 第五章节点 1a「东去路上（开篇补种）」。十场必打之一：b05_yesu_yelang。
-- 挂在 ch05_dukou 峡口两格 (13,14)(13,15)（mode=enter，once=true，set_flag=ch05.kaipian），
-- 出生点到渡口唯一的一条路上：狼扑营、毒发作，都是找上他的（docs/ch05-design.md 3.1）。
--
-- 这一节要种下第 5 章的前提，第 3、4 章都没有种（施工图 1.2、handoff 7.3）：
--   · 遗书。原著藏在墨大夫的香囊里（ch62），游戏第 3 章 ch03.tiezhe.find1 写死了身上
--     「一共四样」没有香囊，改成那串钥匙里最小的一把开了床下的暗格，时间落在曲魂拆石屋
--     （ch03.ligu.clean）之前——用户已同意的改编。暗格里是遗书、给严氏的亲笔信、纹龙戒
--     三样（原著暗格里本来就放着信物与亲笔证明信，ch62）。
--   · 阴毒第一次动。ch66 他就知道丹田里伏着一丝阴寒之物；ch104 说它「一个月前」开始扩散。
--     「寒毒」这个词本节一个字不提：它首见 ch117，本作放到节点 9 严氏嘴里才第一次说出来。
--   · 寒毒起表：ch05.yindu_qi 记下这一天的绝对日数（施工图 3.3）。
--
-- 遗书的四件事照 ch62 转述，**顺序打乱**（先要他去、再开价、最后才是那颗解药），
-- 细节换成他自己的：他翻过纸背看了一眼，背面什么也没写。
-- 接第 3 章的钩子：ch03.jieyao_xuan == 2（先验了三天）与 == 1（当场吞了），两条各一句。
--
-- 逃跑：①号仗 can_escape 真。逃了在置旗标之前 return，挂点留着，回头再踩一次还能打；
-- 那六十天的路与起表只走一回——ch05.yindu_qi 已经有值，就不再往前拨日子。
-- 输：defeat_is_fatal 真，脚本收 game_over（第 3、4 章同一口径，引擎不自己消费这一位）。

if flag.get("ch05.yindu_qi") == 0 then
    talk("", "ch05.dongqu.road1")
    talk("", "ch05.dongqu.road2")
    advance_days(60)
    -- 起表：狼来的这一夜，就是阴毒第一次动的那一夜（施工图 3.3：「60，狼，然后发作」）。
    -- 记在开打之前，是为了逃跑之后回头再来不必再走一遍这六十天。
    flag.set("ch05.yindu_qi", today())
    talk("", "ch05.dongqu.road3")
    talk("", "ch05.dongqu.camp")
    talk("", "ch05.dongqu.wolf")
else
    talk("", "ch05.dongqu.retry")
end

local won, how = battle("b05_yesu_yelang")

if not won then
    if how == "escaped" then
        talk("", "ch05.dongqu.escape")
        return
    end
    -- "lost" 与任何不是「赢」也不是「逃」的落点收在这里：六条狼咬死一个炼气八层的人，
    -- 在数值上不该发生（狼打他 1 点），真发生了就是引擎侧回归，按输算。
    talk("", "ch05.dongqu.lost")
    game_over()
    return
end

talk("", "ch05.dongqu.win")
talk("", "ch05.dongqu.cold1")
talk("", "ch05.dongqu.cold2")
talk("", "ch05.dongqu.cold3")
talk("", "ch05.dongqu.box")
talk("", "ch05.dongqu.key")

give("story_mo_yishu", 1)
give("story_mo_qinbixin", 1)
give("story_wenlong_jie", 1)

talk("", "ch05.dongqu.read1")
talk("", "ch05.dongqu.read2")
talk("", "ch05.dongqu.read3")

if flag.get("ch03.jieyao_xuan") == 2 then
    talk("", "ch05.dongqu.hook_test")
else
    -- == 1（吞下去）；读出 0 的存档没走过第 3 章那一节，按吞了写——那一句只提黑丸，不提验药。
    talk("", "ch05.dongqu.hook_eat")
end

talk("", "ch05.dongqu.late")
talk("", "ch05.dongqu.letters")
talk("", "ch05.dongqu.end")

flag.set("ch05.kaipian")
