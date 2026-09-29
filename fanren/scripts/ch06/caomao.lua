-- @hook ch06_tainan_gu npc_caomao_qingnian npc
-- 第六章节点 5b「以物易物：飞行符」。挂在 ch06_tainan_gu 摊位区草帽青年身上（npc，hidden_flag=ch06.feixingfu）。
-- 拿丹药去换飞行符是他主动的（ch131 末）。施工图 3.1 这一格写的是 interact + once + guard ch06.lingshi +
-- set_flag ch06.feixingfu：NPC 对象没有这几个属性，所以由脚本自己判——没看过行情（ch06.lingshi 未置）只说一句闲话；
-- 换完他收摊（hidden_flag），这张摊就不在了。
--
-- **以物易物是脚本，不是商店**（施工图第 17 节第 15 条）：少了不换，量多就换——五瓶养精丹换飞行符、
-- 一打符纸（12 张）与《基础咒决》残本。咒书是旗标不是物品（ch06.zhoushu），它的用处在 9b。怎么讨价还价以 13.1 为准；
-- 次序是他先算账、先开口要添头，青年再说这张符的来历（复验整改 16.5：原著「装作肉痛→力荐→才要添头」那个次序不留）。
-- 拿药去问的那几句不点瓶数：手里只剩 0-1 瓶的人也走得到这里（复验 N-L4）；身上一瓶也没有的，开口那句与
-- 补不齐时青年那句各换一句零瓶也说得通的（back_none、waive_none；抽查 N3-L1）。
--
-- **补差（校对 HIGH-3，施工图 16.4、第 9 节第 2 条）**：药不够不再 return——交易照成，主线不被一种消耗品卡死。
-- 先扣手上有的药；差几瓶，每瓶按 STONES_PER_PILL 块灵石补（与坊市收药同价：养精丹基价 90 × sellRate 0.08，
-- 取整 7——卖了再补不赚不亏）；灵石也补不齐，**青年把差额免了**：一句话，不记账、不置任何旗标
-- （复验整改 16.5 MEDIUM-N1 ①：他换完就收摊，原著此后也没有他，记下来的账没人收；ch06.qianyao 只记卖符少女一人）。
-- 每一处 take 都看返回值。8b 的补差在 shaonv.lua（那边补不齐是记账）；脚本之间没有 require，各写各的。
-- 帮手函数定义在 guard 之后：3.1 的 guard 没满足时，要在任何 take 之前 return（Ch06TriggerModeTests）。

if flag.get("ch06.lingshi") == 0 then
    talk("caomao_qingnian", "ch06.npc.caomao.idle")
    return
end

local STONES_PER_PILL = 7

local function barter(need)
    local pills = math.min(item.count("pill_yangjing_dan"), need)
    if pills > 0 and not take("pill_yangjing_dan", pills) then
        pills = 0
    end
    local short = need - pills
    if short <= 0 then
        return
    end
    talk("", "ch06.feixingfu.short")
    local paid = math.min(short, math.floor(item.count("material_lingshi") / STONES_PER_PILL))
    if paid > 0 and take("material_lingshi", paid * STONES_PER_PILL) then
        talk("", "ch06.feixingfu.topup")
    else
        paid = 0
    end
    if short - paid > 0 then
        if pills > 0 then
            talk("caomao_qingnian", "ch06.feixingfu.waive")
        else
            talk("caomao_qingnian", "ch06.feixingfu.waive_none")
        end
    end
end

if item.count("pill_yangjing_dan") > 0 then
    talk("", "ch06.feixingfu.back")
else
    talk("", "ch06.feixingfu.back_none")
end
talk("", "ch06.feixingfu.two")
talk("caomao_qingnian", "ch06.feixingfu.sniff")
talk("caomao_qingnian", "ch06.feixingfu.more")
talk("", "ch06.feixingfu.grin")
talk("caomao_qingnian", "ch06.feixingfu.three")
talk("", "ch06.feixingfu.act")
talk("", "ch06.feixingfu.extra")
talk("caomao_qingnian", "ch06.feixingfu.sell")

barter(5)

give("talisman_feixing_fu", 1)
give("material_fuzhi", 12)
flag.set("ch06.zhoushu")
talk("", "ch06.feixingfu.done")
talk("", "ch06.feixingfu.book")
talk("", "ch06.feixingfu.pack")

flag.set("ch06.feixingfu")
