-- @hook ch05_xicheng trigger_heishuixiang enter once
-- 第五章节点 3a「黑水巷：格杀、收服」。十场必打之二：b05_heishuixiang。
-- 挂在 ch05_xicheng 黑水巷巷口 (30,18)(30,19)（mode=enter，once=true，guard_flag=ch05.matou，
-- set_flag=ch05.shoufu）。伏击是对方的动作，所以是踏入型；码头进城只有这一条路。
--
-- 原著 ch102-103：两个脚夫七拐八绕把他领进死巷，前后各堵一簇；曲魂一拳一个；黑熊想拿他当人质，
-- 被他从腰间抽软剑一剑穿喉；孙二狗吓瘫，求饶说自己知道嘉元城的大小消息；他让孙二狗吞腐心丸，
-- 一个月要一回解药。
--
-- 黑熊那一下分两种写法，沿用第 3 章 ch03.jieyao.check_sword / check_bare 的分法：
-- 身上有软剑（weapon_yudai_duanjian）就抽剑，没有就换一掌。第 4 章终局 fixture 两侧都带着软剑，
-- 从那里接过来只走得到抽剑那一条；空手那一条留给手搭的起点（记进施工图末尾「施工偏差」）。
--
-- 三选一 → ch05.shoufu（孙二狗的账，施工图第 7 节；4b、10a、11c 各回读一次）：
--   1 腐心丸 ＋ 一袋碎银（原著）。take(20) 看返回值；不够一袋就把剩的全给，一块都没有就只给药（记 3）。
--   2 腐心丸 ＋ 让曲魂的手在他后颈上停一停（改编，有原著依据：ch103 韩立本已下令「曲魂，杀了他」，
--     曲魂一步步走过去，孙二狗吓瘫在地；本作把那一步停在他后颈上）。不花钱，忠心是怕出来的。
--   3 只给腐心丸。取消也按这一条算——没做选择，就是什么都没多给。
--
-- 这一仗 can_escape 假；输即 game_over（defeat_is_fatal 真，脚本收）。

talk("", "ch05.heishui.lead")
talk("", "ch05.heishui.dead")
talk("", "ch05.heishui.two")
talk("", "ch05.heishui.order")

local won = battle("b05_heishuixiang")
if not won then
    talk("", "ch05.heishui.lost")
    game_over()
    return
end

if item.count("weapon_yudai_duanjian") > 0 then
    talk("", "ch05.heishui.grab_sword")
else
    talk("", "ch05.heishui.grab_bare")
end

talk("", "ch05.heishui.beg")
talk("", "ch05.heishui.listen")
talk("", "ch05.heishui.pill")
talk("sun_ergou", "ch05.heishui.name")
talk("", "ch05.heishui.q")

local pick = choice{
    "ch05.heishui.opt_money",
    "ch05.heishui.opt_hand",
    "ch05.heishui.opt_pill",
}

local shoufu = 3
if pick == 1 then
    local purse = item.count("material_lingshi")
    if purse >= 20 and take("material_lingshi", 20) then
        talk("", "ch05.heishui.money_full")
        shoufu = 1
    elseif purse > 0 and purse < 20 and take("material_lingshi", purse) then
        talk("", "ch05.heishui.money_all")
        shoufu = 1
    else
        -- 钱袋是空的（或扣不下去）：钱没给出去，这一笔记成只给了药。
        talk("", "ch05.heishui.money_none")
        shoufu = 3
    end
elseif pick == 2 then
    talk("", "ch05.heishui.hand")
    shoufu = 2
else
    talk("", "ch05.heishui.pill_only")
    shoufu = 3
end

talk("", "ch05.heishui.end")

flag.set("ch05.shoufu", shoufu)
