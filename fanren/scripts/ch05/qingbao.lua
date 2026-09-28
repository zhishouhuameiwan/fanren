-- @hook ch05_kezhan trigger_qingbao interact once
-- 第五章节点 4a「汇源客栈：情报」。挂在 ch05_kezhan 上房书案左格 (8,2)
-- （mode=interact，once=true，guard_flag=ch05.zhuishao，set_flag=ch05.qingbao）。
-- 他自己坐下把遗书摊开，等孙二狗上门（ch104）——所以是交互型。
--
-- 这一节三件事：
--   · 遗书后半截：墨府的人。**转写高风险点 1**（施工图第 13 节）：原著 ch104 那张名单列了十来个人，
--     这里只取五个（严氏、燕歌、王氏、刘氏、墨凤舞）。原著的评语（性情 / 出身 / 年岁 / 可信与否）一条不用：
--     2026-09-25 整改后改成韩立自己像分药材那样分三堆（能用的 / 要防的 / 看不透的），
--     只留后文用得着的两件事——严氏掌会，王氏另有一股会里名册上查不着的人（施工图 13.1）。
--     孙二狗报帮派用的是桌上三只茶杯，也是这一次整改换上的。
--   · 内视：「至多六十来天」——倒计时头一回被说成数（目标链第 8 步随之写明「顶多两个月」）。
--   · 孙二狗报城里的格局（ch105）。**转写高风险点 2**：只要三大帮与「惊蛟会最弱」，中帮一个都不点名。
--     「惊蛟会」三个字在本节第一次出现（施工图 12.2：节点 1-3 不许有）。
--     墨会主已死、一年前关门弟子吴剑鸣持遗书与信物报丧、已与墨玉珠定亲（ch105）。
--
-- 孙二狗进门时的嘴脸按 ch05.shoufu 分三种（施工图第 7 节「孙二狗的忠心」）。
-- 二选一 → ch05.qingbao：1 再丢他一小袋（原著 ch105；take(10) 看返回值，扣不下就不给，记 2）/ 2 不给。
-- 首尾各拨一天（施工图 3.3：「1，末尾再 1」）；末尾那一天孙二狗没来，来的是报信的小脚夫——
-- 他被铁拳会拖进了码头后头的仓房（节点 4b）。

advance_days(1)

talk("", "ch05.qingbao.morning")
talk("", "ch05.qingbao.list1")
talk("", "ch05.qingbao.list2")
talk("", "ch05.qingbao.list3")
talk("", "ch05.qingbao.old")
talk("", "ch05.qingbao.look")
talk("", "ch05.qingbao.knock")

local shoufu = flag.get("ch05.shoufu")
if shoufu == 1 then
    talk("sun_ergou", "ch05.qingbao.sun_money")
elseif shoufu == 2 then
    talk("", "ch05.qingbao.sun_hand")
else
    talk("sun_ergou", "ch05.qingbao.sun_pill")
end

talk("sun_ergou", "ch05.qingbao.gang1")
talk("sun_ergou", "ch05.qingbao.gang2")
talk("sun_ergou", "ch05.qingbao.head")
talk("sun_ergou", "ch05.qingbao.dead")
talk("sun_ergou", "ch05.qingbao.wu")
talk("", "ch05.qingbao.think")
talk("", "ch05.qingbao.q")

local pick = choice{
    "ch05.qingbao.opt_pay",
    "ch05.qingbao.opt_none",
}

local qingbao = 2
if pick == 1 then
    if take("material_lingshi", 10) then
        talk("", "ch05.qingbao.pay")
        qingbao = 1
    else
        talk("", "ch05.qingbao.pay_short")
    end
else
    -- 不赏（pick == 2）与取消走同一条：没伸手，就是没给。
    talk("", "ch05.qingbao.none")
end

advance_days(1)

talk("", "ch05.qingbao.nextday")
talk("", "ch05.qingbao.why")

flag.set("ch05.qingbao", qingbao)
