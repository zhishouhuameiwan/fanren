-- @hook ch02_jusuo trigger_yingdui interact once
-- 第三章节点 5「表面顺从，暗中盘算」。本章的第一次三选一。
-- 挂在 ch02_jusuo 床沿处的 trigger（mode=interact，once=true，set_flag=ch03.yingdui_xuan）。
--
-- 这一节是主角第一次动杀心，也是整章语气卡最要紧的一处。
-- 他十五岁。**不许写成少年英雄**：三条路他都是按「我能不能活」选的，
-- 不是按「我该不该」。连最狠的那一条（xian）都写成算账——
-- 「几时、在哪儿、用什么」，而不是握拳立誓。
--
-- 三条路的真正共同点在 ch03.yingdui.same：走到末了都撞在同一处——他知道的太少。
-- 方子是删过的，口诀是给到第几层就停的。于是三条路都通向节点 6 的那面书架，
-- 这是设计上有意的：分支改的是他这一年怎么活，不是改走向。
--
-- ch03.yingdui_xuan 的取值与 data/flags.json 登记一致：1 顺从 / 2 拖延 / 3 领先动手。
-- 取消（choice 返回 nil）按 1 记——一个刚被捏住家人的孩子，默认是先顺着。

if flag.get("ch03.shichong_wan") ~= 1 then
    talk("", "ch03.yingdui.gate")
    return
end

if flag.get("ch03.yingdui_xuan") > 0 then
    talk("", "ch03.yingdui.again")
    return
end

talk("", "ch03.yingdui.room")
talk("", "ch03.yingdui.count")
talk("", "ch03.yingdui.safe")
talk("", "ch03.yingdui.beast")

local pick = choice{
    "ch03.yingdui.opt_shun",
    "ch03.yingdui.opt_tuo",
    "ch03.yingdui.opt_xian",
}

if pick == 2 then
    flag.set("ch03.yingdui_xuan", 2)
    talk("", "ch03.yingdui.tuo1")
    talk("", "ch03.yingdui.tuo2")
elseif pick == 3 then
    flag.set("ch03.yingdui_xuan", 3)
    talk("", "ch03.yingdui.xian1")
    talk("", "ch03.yingdui.xian2")
else
    -- 顺从（pick == 1）与取消（pick == nil）走同一条。
    flag.set("ch03.yingdui_xuan", 1)
    talk("", "ch03.yingdui.shun1")
    talk("", "ch03.yingdui.shun2")
end

talk("", "ch03.yingdui.same")
talk("", "ch03.yingdui.where")
talk("", "ch03.yingdui.end")

-- 从这一夜到他真正动手偷书，隔了一个月。这一个月他在等一场雨（见 miji.lua）。
advance_days(30)
