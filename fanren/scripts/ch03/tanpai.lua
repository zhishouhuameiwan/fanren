-- @hook ch03_mishi trigger_tanpai enter once
-- 第三章节点 3「摊牌：他是墨居仁」。本章第一个转折。
-- 挂在 ch03_mishi 甬道里口 (11,14) 那两格的 trigger（mode=enter，once=true，set_flag=ch03.tanpai_done）。
-- （石阶甬道 2026-09-27 从北墙挪到了南墙，map_spec 规则 29：往北走进堂屋门，就从南墙上来。）
-- 这一行从前写的是 interact，与地图对不上。enter 是对的：他是被叫到石屋后头那间来的
-- （上一节 ch03.mogui.end 那句「不必带药箱」），进门这场戏就开演，他没得推辞。
-- 口径以首行的 @hook 为准（tools/validate.py 比着 maps/*.tmj 校验）。
--
-- ---------------------------------------------------------------------------
-- 全章最难写的一处，写法在这里交待清楚，改动请照这个路子改。
-- ---------------------------------------------------------------------------
--
-- 1. **他摊牌前后是同一个人。** 他不换脸，不狞笑，不阴恻恻。他仍旧话慢、用词
--    讲究、该说请的地方说请、把两只空茶盏挪正。变的不是他，是玩家的解释——
--    ch03.tanpai.polite 那一条是全章的题眼：「这四年他听的那些客气话原本就是
--    这个意思，只是他一直听成了别的。」
--
-- 2. **不让他自述身世，让他考韩立。** 他是大夫，大夫问诊就是一路设问把病家
--    问到自己说出病名。所以年龄这件事是韩立自己推出来的（q1-a3），推的凭据
--    是节点 2 那只凉手——玩家手里早就有这条线索。由受害人亲口说出结论，比由
--    加害人宣布残酷得多，也把「回头找得到线索」这一条兑现在同一场戏里。
--
-- 3. **点穴不写成一道残影。** 韩立是靠「咽不下这口气、下巴也压不动」才发现
--    自己动不了的，而对方早在他坐下时就点了。发现得越晚越难受。
--
-- 4. **要那只铁筒用的是掌心朝上的手势**——节点 2 那个要他递药杵的手势。
--    没有搜身，没有夺。他伸手，韩立自己交。
--
-- 名牌的换法：ch03.tanpai.name 那一句仍由 mo_daifu 说出，名牌上还写着「墨大夫」；
-- 从下一句起才换成 mo_juren。名牌是**因为**他说了那句话才变的，不能提前半秒。
--
-- 分支：认不认进度。两条都往下走，改的是墨居仁对他的估量，以及节点 5、9 的口吻。
-- 原著里韩立正是因为瞒了进度才逼出这场摊牌（ch31），本作把这一层交还给玩家。

if flag.get("ch03.mo_gui_gu") ~= 1 then
    talk("", "ch03.tanpai.gate")
    return
end

if flag.get("ch03.tanpai_done") == 1 then
    talk("", "ch03.tanpai.again")
    return
end

talk("", "ch03.tanpai.door")
talk("", "ch03.tanpai.sit")

talk("mo_daifu", "ch03.tanpai.q1")
talk("", "ch03.tanpai.a1")
talk("mo_daifu", "ch03.tanpai.q2")
talk("", "ch03.tanpai.a2")
talk("mo_daifu", "ch03.tanpai.q3")
talk("", "ch03.tanpai.a3")
talk("mo_daifu", "ch03.tanpai.nod")
talk("", "ch03.tanpai.pause")

-- 名牌在这一句之后才换。
talk("mo_daifu", "ch03.tanpai.name")

talk("mo_juren", "ch03.tanpai.hui")
talk("mo_juren", "ch03.tanpai.hurt")
talk("mo_juren", "ch03.tanpai.rate")
talk("", "ch03.tanpai.why")
talk("mo_juren", "ch03.tanpai.chang")
talk("", "ch03.tanpai.hundreds")

talk("", "ch03.tanpai.swallow")
talk("mo_juren", "ch03.tanpai.told")
talk("", "ch03.tanpai.polite")

talk("mo_juren", "ch03.tanpai.reask")

local pick = choice{
    "ch03.tanpai.opt_admit",
    "ch03.tanpai.opt_deny",
}

-- 旗标已补登（ch03.tanpai_ren：1 认 / 0 咬住没认）。
-- 当场那两句之外，它还往后传：矞过一次的人，
-- 节点 9 那次「先验一验那颗黑丸」就更有理由。
--
-- 先置后说：两条分支的台词都在这一句之后，
-- 写在前面才不会在日后有人往分支里加 return 时漏掉。
flag.set("ch03.tanpai_ren", pick == 2 and 0 or 1)

if pick == 2 then
    -- 咬住不松口。对面不拆穿也不再问，韩立从此不知道他信了几分——
    -- 这比被拆穿难办。
    talk("", "ch03.tanpai.deny1")
    talk("mo_juren", "ch03.tanpai.deny2")
else
    -- 认（pick == 1）与取消（pick == nil）走同一条。
    -- 没做选择按认算：他方才刚亲口推出对方只剩一年，这种时候还在盘算怎么瞒，
    -- 不像一个刚被吓住的十五岁孩子，倒像个老江湖。
    talk("", "ch03.tanpai.admit1")
    talk("mo_juren", "ch03.tanpai.admit2")
end

talk("mo_juren", "ch03.tanpai.palm")

-- 那只铁筒。**交还是不交，由玩家定。**
--
-- 这一处原先写成「照单 take，扣不到就走另一条台词」，而穷举证明那条台词
-- 在全部 18432 条路径上一次也走不到：五毒水是节点 1 那一仗**打完之后**才配的，
-- 从拿到它到摊牌之间本章没有第二场战斗，玩家没有任何办法把它花掉。
-- 第 1 章出过「某结局 81 条路径 0 次可达」的事故，不能在这里重演，
-- 所以把它改成一次真选择：交出去 / 说没有。两条都由玩家自己选得到。
--
-- 两条的分别是**信息与实物的对调**：
--   交出去——他当场掂了掂、闻了闻，点出方子该是七味、后两味没让你抄。
--     这是节点 6「短了两味」与节点 7 备毒的引信，玩家早一步知道要找什么。
--   说没有——筒子留在袖子里（实物），可这条引信就没有了；
--     那两味要等节点 6 自己从偷来的方子里对出来。
-- 墨居仁两条都不追不搜不拆穿，只说一句「也好」——他连问都懒得问，
-- 这比搜身更凉，也与他摊牌前后一以贯之的那份客气是同一件东西。
--
-- 交出去那一条仍旧**看 take 的返回值**。它现在是守卫而不是分支：
-- 正常流程下这一刻袖子里必有一筒。真失败了也得照实说话，
-- 不能让墨居仁一本正经地点评一筒不存在的毒水而玩家手上什么也没少——
-- 第 2 章在这上面栽过两次。
local hand = choice{
    "ch03.tanpai.opt_hand",
    "ch03.tanpai.opt_lie",
}

if hand == 2 then
    talk("", "ch03.tanpai.lie")
    talk("mo_juren", "ch03.tanpai.gone")
elseif take("pill_wudu_shui", 1) then
    -- 交出去（hand == 1）与取消（hand == nil）走同一条。
    -- 没做选择按交出去算：他刚发现自己连下巴都压不动，这会儿不会去赌一只筒子。
    talk("", "ch03.tanpai.give")
    talk("mo_juren", "ch03.tanpai.take")
else
    talk("", "ch03.tanpai.nothing")
    talk("mo_juren", "ch03.tanpai.gone")
end

talk("", "ch03.tanpai.end")

flag.set("ch03.tanpai_done")
