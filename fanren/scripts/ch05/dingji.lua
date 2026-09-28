-- @hook ch05_kezhan trigger_dingji interact once
-- 第五章节点 10a「定计夺帮」。挂在 ch05_kezhan 上房八仙桌上两格 (6,5)(7,5)
-- （mode=interact，once=true，guard_flag=ch05.duizhi，set_flag=ch05.dingji）。
-- 孙二狗在等，定计的是他（ch119-120）。**寒毒检查点之一**（施工图 3.3：10a / 11c / 12b / 12c / 12e）。
--
-- 孙二狗报「神仙大会」（ch119）：可以说，「升」字头的那个词不行（施工图 0 第 5 条、12.2）。
-- 原著那只双头怪鹰、树洞、龟息功一概不照搬，只留「帮里有人撞见一对骑着大鸟的男女，听见了这四个字，
-- 帮主下了封口令」。
-- 定计（ch120）：扶孙二狗当帮主；沈重山这几日下午在潇湘院；**把曲魂借给孙二狗**。
--   原著交的是引魂钟；游戏交第 3 章手札末页那张四字纸条（ch03.tiezhe.word：念那四个字，曲魂就转过身来）——改编。
--   曲魂离队看返回值：这是本章唯一一处离队（施工图验收 11），此后本章不再让它入队。
-- 孙二狗接纸条时的嘴脸按 ch05.shoufu 分三种（施工图第 7 节）。
-- 末尾他自己说「要毒人，自己先得有一颗」——潇湘院必败分支的提示二（施工图 8.4），同时挂起支线 Z3。

-- ---------------------------------------------------------------------------
-- 寒毒检查点（docs/ch05-design.md 3.3）。本章五个检查点（dingji / anpai / zhuwu / tancha /
-- huanyu）写的是同一段，改一处就改五处；ScriptHost 关了 package 与 dofile，脚本之间共用不了函数。
-- 发作一次吞一颗养精丹顶住是游戏规则，原著没有这一笔（改编）；超期不判输（用户拍板），只多一句话。
-- ---------------------------------------------------------------------------
local HANDU_DUAN2 = 65    -- 发作起第 65 日入第二段：进段后的第一个检查点发作一次
local HANDU_DUAN3 = 80    -- 第 80 日入第三段：此后每个检查点各发作一次
local HANDU_QIXIAN = 90   -- ch104「顶多再有两个月」是 d≈30 时说的，即 d = 90

local function handu_fazuo(chaoqi)
    talk("", "ch05.handu.attack")
    if item.count("pill_yangjing_dan") > 0 and take("pill_yangjing_dan", 1) then
        talk("", "ch05.handu.pill")
    else
        talk("", "ch05.handu.nopill")
    end
    if chaoqi then
        talk("", "ch05.handu.over")
    end
end

local function handu_jiancha()
    local qi = flag.get("ch05.yindu_qi")
    if qi <= 0 then return end
    local d = today() - qi
    if d >= HANDU_DUAN3 then
        flag.set("ch05.handu_fa", 3)
        handu_fazuo(d >= HANDU_QIXIAN)
    elseif d >= HANDU_DUAN2 and flag.get("ch05.handu_fa") < 2 then
        flag.set("ch05.handu_fa", 2)
        handu_fazuo(false)
    end
end

handu_jiancha()

talk("sun_ergou", "ch05.dingji.sun")
talk("sun_ergou", "ch05.dingji.news")
talk("", "ch05.dingji.han")
talk("", "ch05.dingji.when")
talk("sun_ergou", "ch05.dingji.shock")
talk("sun_ergou", "ch05.dingji.where")
talk("", "ch05.dingji.qu1")

if party.remove("qu_hun") then
    talk("", "ch05.dingji.qu2")
else
    -- 曲魂不在队伍里（旧存档、手搭的起点）：纸条照样交出去，镜头里少了它走过去那一下。
    talk("", "ch05.dingji.qu_gone")
end

local shoufu = flag.get("ch05.shoufu")
if shoufu == 1 then
    talk("", "ch05.dingji.sun_money")
elseif shoufu == 2 then
    talk("", "ch05.dingji.sun_hand")
else
    talk("sun_ergou", "ch05.dingji.sun_pill")
end

talk("", "ch05.dingji.witness")
talk("", "ch05.dingji.poison")

flag.set("ch05.qingling_qiu")
flag.set("ch05.dingji")
